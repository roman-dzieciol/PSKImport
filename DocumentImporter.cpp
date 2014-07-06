#include "DocumentImporter.h"

// TODO: verify data after loadingand before creating

#pragma pack(push,4)


DocumentImporter::DocumentImporter(const TCHAR* FileName, ImpInterface* ImportInterface, Interface* GlobalInterface, BOOL bSuppressPrompts)
: FileName(FileName)
, ImportInterface(ImportInterface)
, GlobalInterface(GlobalInterface)
, bSuppressPrompts(bSuppressPrompts)
, File(NULL)
, FilePtr(0)
, MeshImpNode(NULL)
, MeshINode(NULL)
, MeshObject(NULL)
, MeshDerived(NULL)
, SkinModifier(NULL)
{
}


DocumentImporter::~DocumentImporter()
{
	fclose(File);
}

void DocumentImporter::ParseHeader()
{
	FilePtr += sizeof(VChunkHeader);
}

template<class T> void DocumentImporter::ParseChunk( std::vector<T>& Array )
{
	// Parse header
	const VChunkHeader* header = (const VChunkHeader*) FilePtr;
	FilePtr += sizeof(VChunkHeader);

	// Verify data size
	if( header->DataSize != sizeof(T) )
		throw std::exception("Invalid data size");

	// Verify array size
	const size_t arraySize = header->DataCount * header->DataSize;
	if( FilePtr + arraySize > (BYTE*)&FileData[0] + FileData.size() )
		throw std::exception("Invalid array size");

	// Parse array
	Array.reserve(header->DataCount);
	Array.resize(header->DataCount);
	memcpy(&Array[0], FilePtr, arraySize);
	FilePtr += arraySize;
}

void DocumentImporter::Import()
{
	// open file
	File = _tfopen(FileName, _T("rb"));
	if( File == NULL )
		throw std::exception("File open failed");

	// get size
	fseek(File, 0, SEEK_END);
	FileData.reserve(ftell(File));
	FileData.resize(ftell(File));
	FilePtr = &FileData[0];
	fseek(File, 0, SEEK_SET);

	// read file
	if( fread(FilePtr, 1, FileData.size(), File) != FileData.size() )
		throw std::exception("File read failed");

	// parse file
	ParseHeader();
	ParseChunk(Points);
	ParseChunk(Wedges);
	ParseChunk(Faces);
	ParseChunk(Materials);
	ParseChunk(Bones);
	ParseChunk(Influences);


	InfluencesPerBone = std::vector<int>(Bones.size(), 0);
	InfluencesPerPoint = std::vector<int>(Points.size(), 0);
	InfluencesPerWedge = std::vector<int>(Wedges.size(), 0);

	for( int i=0; i<Influences.size(); ++i)
	{
		++(InfluencesPerBone.at(Influences[i].BoneIndex));
		++(InfluencesPerPoint.at(Influences[i].PointIndex));
		++(InfluencesPerWedge.at(Wedges[Influences[i].PointIndex].PointIndex));
	}

	// Import nodes
	ImportNodes();

	// Import objects
	ImportMesh();
	ImportSkin();
	ImportBones();

	// Finalize importing
	FinalizeMesh();
	FinalizeBones();
}


void DocumentImporter::ImportNodes()
{
	// Create mesh node
	Matrix3 meshTM;
	meshTM.SetTranslate(Point3(0,0,0));
	ImportNode(MeshImpNode, MeshINode, GetString(IDS_PSK_OBJ_NAME), meshTM);

	// Create bone nodes
	BoneImpNodes.resize(Bones.size());
	BoneINodes.resize(Bones.size());
	for( int i=0; i<Bones.size(); ++i)
	{
		TSTR boneName(Bones[i].Name);

		if( i == 0 )
		{
			Bones[i].BonePos.Orientation.W = -Bones[i].BonePos.Orientation.W;
		}

		Matrix3 boneTM;
		boneTM.IdentityMatrix();
		boneTM.SetRotate(ToQuat(Bones[i].BonePos.Orientation));
		boneTM.Translate(ToPoint3(Bones[i].BonePos.Position));

		// MAX
		// Loc: X=1.888 Y=0 Z=42.322
		// Rot: Y=90.0 P=82.079 R=180.0

		// PSK
		// Loc: X=1.888 Y=0 Z=42.322
		// Rot: Y=90.0 P=0 R=-97.921
		// Bone[0] L:1.887664, -0.000002, 42.322094  R:-179.999969, -15.841516, -180.000092  S:0.000000, 0.000000, 0.000000  LEN:0.000000 F:0x0 

		/*Point3 r;
		boneTM.GetYawPitchRoll(&r.x, &r.y, &r.z);
		Point3 p = boneTM.GetTrans();
		r = r / PI * 360.f;
		DebugPrint(_T("Bone[%d] L:%f, %f, %f  R:%f, %f, %f  S:%f, %f, %f  LEN:%f F:0x%x \n")
			, i
			, p.x, p.y, p.z
			, r.x, r.y, r.z
			, Bones[i].BonePos.XSize, Bones[i].BonePos.YSize, Bones[i].BonePos.ZSize
			, Bones[i].BonePos.Length, Bones[i].Flags);*/

		ImportNode(BoneImpNodes[i], BoneINodes[i], boneName, boneTM);
	}

	// Create bone node tree
	for( int i=0; i<Bones.size(); ++i)
	{
		if( i != 0 )
		{
			BoneINodes[Bones[i].ParentIndex]->AttachChild(BoneINodes[i], FALSE);
		}
	}
}


void DocumentImporter::ImportNode( ImpNode*& node, INode*& inode, const TCHAR* name, Matrix3 nodeTM )
{
	// Create ode
	node = ImportInterface->CreateNode();
	if( node == NULL ) 
		throw std::exception("Node creation failed");

	// Get INode
	inode = node->GetINode();
	if( inode == NULL ) 
		throw std::exception("INode was not created");

	// Name node
	node->SetName(trim(name).c_str());

	// Position node
	inode->SetNodeTM(TIME_INITIAL_POSE, nodeTM);
}


void DocumentImporter::ImportMesh()
{
	// Create triangle mesh object
	MeshObject = CreateNewTriObject();
	Mesh& mesh = MeshObject->GetMesh();

	// Create verts
	const int pointCount = (int)Points.size();
	mesh.setNumVerts(pointCount);
	for( int i=0; i<pointCount; ++i)
	{
		const VPoint& point = Points[i];
		mesh.setVert(i,ToPoint3(point.Point));
	}

	// Create faces
	const int faceCount = (int)Faces.size();
	mesh.setNumFaces(faceCount);
	for( int i=0; i<faceCount; ++i)
	{
		const VTriangle& face = Faces[i];
		mesh.faces[i].setVerts(
			Wedges.at(face.WedgeIndex[2]).PointIndex,
			Wedges.at(face.WedgeIndex[1]).PointIndex,
			Wedges.at(face.WedgeIndex[0]).PointIndex);

		mesh.faces[i].setSmGroup(face.SmoothingGroups);
		mesh.faces[i].setMatID(face.MatIndex);
	}

	// Setup UVW maps
	mesh.setNumMaps(2); // 2 is minimum, 0 being color and 1 texture
	mesh.setMapSupport(0,FALSE); // no color
	mesh.setMapSupport(1,TRUE); // use texture

	// Create texture verts
	const int wedgeCount = (int)Wedges.size();
	mesh.setNumMapVerts(1,wedgeCount);
	for( int i=0; i<wedgeCount; ++i)
	{
		const VVertex& wedge = Wedges[i];
		mesh.mapVerts(1)[i].Set(fabs(fmod(wedge.U, 1.f)), 1.f-fabs(fmod(wedge.V, 1.f)), 0.f);
	}

	// Create texture faces
	for( int i=0; i<faceCount; ++i)
	{
		const VTriangle& face = Faces[i];
		mesh.mapFaces(1)[i].setTVerts(
			face.WedgeIndex[2],
			face.WedgeIndex[1],
			face.WedgeIndex[0]);
	}

	// Fixup mesh
	mesh.buildNormals();
	mesh.buildBoundingBox();
	mesh.InvalidateEdgeList();
	mesh.InvalidateGeomCache();
}


void DocumentImporter::ImportSkin()
{
	assert(MeshObject);

	// Create the skin modifier
	SkinModifier = (Modifier*) ImportInterface->Create(OSM_CLASS_ID, SKIN_CLASSID);
	if( SkinModifier == NULL )
		throw std::exception("SkinModifier creation failed");

	// Create the derived object for the skin modifier
	MeshDerived = CreateDerivedObject(MeshObject);
	if( MeshDerived == NULL )
		throw std::exception("MeshDerived creation failed");

	// Link modifier to derived object
	MeshDerived->AddModifier(SkinModifier);
	
	// Link node to object
	MeshImpNode->Reference(MeshDerived);
}

Matrix3 DocumentImporter::GetPivotTransform(INode* inode)
{
	Point3 opos = inode->GetObjOffsetPos();
	Quat orot = inode->GetObjOffsetRot();
	ScaleValue oscale = inode->GetObjOffsetScale();

	// this should already be in local space
	Matrix3 tm(1);
	ApplyScaling(tm, oscale);
	RotateMatrix(tm, orot);
	tm.Translate(opos);
	return tm;
}

void DocumentImporter::ImportBones()
{
	BoneObjects.resize(Bones.size());
	for( int i=0; i<Bones.size(); ++i)
	{
		// Create dummy
		//BoneObjects[i] = (DummyObject*) ImportInterface->Create(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0));
		BoneObjects[i] = (SimpleObject2 *)ImportInterface->Create( GEOMOBJECT_CLASS_ID, BONE_OBJ_CLASSID );
		if( BoneObjects[i] == NULL )
			throw std::exception("Bone object creation failed");

		/*// Set bounding box
		Matrix3 pivotTransform = GetPivotTransform(BoneINodes[i]);
		pivotTransform.Invert();
		Box3 boundingBox;
		boundingBox.pmin = Point3(-0.5, -0.5, -0.5);
		boundingBox.pmax = Point3(0.5, 0.5, 0.5);
		boundingBox.pmin = pivotTransform.PointTransform(boundingBox.pmin);
		boundingBox.pmax = pivotTransform.PointTransform(boundingBox.pmax);
		BoneObjects[i]->SetBox(boundingBox);

		// Set display options
		BoneObjects[i]->EnableDisplay();*/

		// Link node to object
		BoneImpNodes[i]->Reference(BoneObjects[i]);
	}
}


void DocumentImporter::FinalizeMesh()
{
	// Add mesh nodes
	ImportInterface->AddNodeToScene(MeshImpNode);
	MeshINode->SetWireColor(RGB(8,61,138));

	// Finalize Mesh
	
	// Create materials
	if( Materials.size() > 0 )
	{
		MultiMtl* multiMaterial = NewDefaultMultiMtl();
		if( multiMaterial == NULL )
			throw std::exception("MultiMtl creation failed");

		multiMaterial->SetNumSubMtls((int)Materials.size());

		for( int i=0; i<Materials.size(); ++i)
		{
			MSTR matName = trim(Materials[i].MaterialName).c_str();

			BitmapTex* matTex = NewDefaultBitmapTex();
			matTex->SetMapName(matName);

			StdMat2* matObj = NewDefaultStdMat();
			matObj->SetSubTexmap(ID_DI, matTex);
			matObj->SetTexmapAmt(ID_DI, 1.f, 0);
			matObj->EnableMap(ID_DI, TRUE);

			multiMaterial->SetSubMtlAndName(i, matObj, matName);
		}

		MeshINode->SetMtl(multiMaterial);

		// Add to the list of scene materials.
		MtlBaseLib* sceneMtls = GlobalInterface->GetSceneMtls();
		sceneMtls->Add(multiMaterial);
	}




	// Finalize skin

	// Get skin import data
	ISkinImportData* importSkin = (ISkinImportData*) SkinModifier->GetInterface(I_SKINIMPORTDATA);
	if( importSkin == NULL )
		throw std::exception("ISkinImportData creation failed");

	// BUG. This call may cause a crash if the EPoly.dlo - MNMath.dll when using an Editable Poly mesh with a Skin Modifier
	MeshINode->EvalWorldState(0);

	// Set the bind shape's initial TM and apply it to the current transform
	// in order to recreate the correct environment
	Matrix3 skinTM;
	skinTM.SetTranslate(Point3(0,0,0));

	if( importSkin->SetSkinTm(MeshINode, skinTM, skinTM) == FALSE )
		throw std::exception("SetSkinTm failed");

	// Create bones
	for( int i=0; i<Bones.size(); ++i)
	{
		// Add bone to skin
		if( importSkin->AddBoneEx(BoneINodes[i], TRUE) == FALSE )
			throw std::exception("AddBoneEx failed");


		Matrix3 localTM;
		localTM.IdentityMatrix();
		localTM.SetRotate(ToQuat(Bones[i].BonePos.Orientation));
		localTM.Translate(ToPoint3(Bones[i].BonePos.Position));

		Matrix3 worldTM;
		if( i > 0 )
		{
			Matrix3 parentWorldTM = BoneINodes[Bones[i].ParentIndex]->GetNodeTM(0);
			worldTM = localTM * parentWorldTM; // calc worldTM
		}
		else
		{	
			worldTM = localTM;
		}

		if( importSkin->SetBoneTm(BoneINodes[i], worldTM, worldTM) == FALSE )
			throw std::exception("SetBoneTm failed");
	}

	// As noted in the Sparks knowledgebase: "Problem importing weights into Physique modifier"
	// For some reason the node must be evaluated after all the bones are added,
	// in order to be able to add the custom weights using the AddWeights function.
	MeshINode->EvalWorldState(0);

	Tab<INode*> allBones;
	allBones.SetCount((int)BoneINodes.size());
	for( int i=0; i<allBones.Count(); ++i)
		allBones[i] = BoneINodes[i];

	for( int i=0; i<Points.size(); ++i)
	{			
		// Compact weights
		Tab<INode*> influencedBones;
		Tab<float> influencedWeights;	

		for( int j=0; j<Influences.size(); ++j)
		{
			VRawBoneInfluence& inf = Influences[j];
			if( inf.PointIndex == i )
			{
				influencedBones.Append(1, &allBones[inf.BoneIndex]);
				influencedWeights.Append(1, &inf.Weight);
			}
		}

		// Print weights
		/*DebugPrint(_T("AddWeights %d %p %s: "), i, MeshINode, MeshINode->GetName());
		for (size_t j = 0; j < influencedWeights.Count(); ++j)
		{
			DebugPrint(_T("[%d] %p %s %f "), j, influencedBones[j], influencedBones[j]->GetName(), influencedWeights[j]);
		}
		DebugPrint(_T("\n"));*/

		if( importSkin->AddWeights(MeshINode, i, influencedBones, influencedWeights) == FALSE )
			throw std::exception("AddWeights failed");
	}
}

void DocumentImporter::FinalizeBones()
{
	// Add bone nodes
	for( int i=0; i<BoneImpNodes.size(); ++i)
	{
		ImportInterface->AddNodeToScene(BoneImpNodes[i]);

 		BoneINodes[i]->SetBoneNodeOnOff(TRUE, 0);
		BoneINodes[i]->SetRenderable(FALSE);
		BoneINodes[i]->BoneAsLine(FALSE);
		BoneINodes[i]->ShowBone(0);

		//BoneINodes[i]->SetVisibility(TIME_INITIAL_POSE, 1.0f);
		if( InfluencesPerBone[i] > 0 )
		{
			BoneINodes[i]->SetWireColor(RGB(27,177,27));
		}
		else
		{
			BoneINodes[i]->SetWireColor(RGB(176,26,26));
		}

		//BoneINodes[i]->SetBoneAutoAlign(TRUE); 
		/*BoneINodes[i]->SetBoneAxis(BONE_AXIS_X);
		BoneINodes[i]->SetBoneAxisFlip(FALSE);
		BoneINodes[i]->SetBoneScaleType(BONE_SCALETYPE_SQUASH);
		BoneINodes[i]->SetBoneFreezeLen(TRUE);*/
	}

	const float width = 1.f;
	const float height = 1.f;
	const float length = float(sqrt(width*height));

	for( int i=0; i<BoneImpNodes.size(); ++i)
	{

		if( i > 0 )
		{
			//BoneINodes[i]->RealignBoneToChild(0);
		}

		BoneObjects[i]->pblock2->SetValue(boneobj_width, 0, width);
		BoneObjects[i]->pblock2->SetValue(boneobj_height, 0, height);
		if( Bones[i].NumChildren == 0 )
		{
			BoneObjects[i]->pblock2->SetValue(boneobj_length, 0, length * 3.f);
		}
		/*else
		{
			float currentLength;
			if( BoneObjects[i]->pblock2->GetValue(boneobj_length, 0, currentLength, FOREVER) == TRUE
			&&  currentLength < length )
				BoneObjects[i]->pblock2->SetValue(boneobj_length, 0, length);
		}*/

		/*if( i != 0 )
		{
			Matrix3 parentTM = BoneINodes[Bones[i].ParentIndex]->GetNodeTM(0);
			Matrix3 boneTM = BoneINodes[i]->GetNodeTM(0);
			Point3 offset = parentTM.GetTrans() - boneTM.GetTrans();
			float dist = offset.Length();
			BoneObjects[Bones[i].ParentIndex]->pblock2->SetValue(boneobj_length, 0, dist);
			DebugPrint(_T("%d %f\n"), Bones[i].ParentIndex, dist);
		}*/

	}

	for( int i=0; i<BoneImpNodes.size(); ++i)
	{
		BoneINodes[i]->ResetBoneStretch(0);
	}
}

#pragma pack(pop)