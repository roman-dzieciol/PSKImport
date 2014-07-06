#pragma once

#include "PSKImport.h"
#include <vector>
#include <string>


#pragma pack(push,4)


#include "UnrealAnimDataTypes.h"
#include "UnrealAnimDataStructs.h"


class DocumentImporter
{
private:
	const TCHAR* FileName;
	ImpInterface* ImportInterface;
	Interface* GlobalInterface;
	BOOL bSuppressPrompts;

private:
	FILE* File;
	BYTE* FilePtr;
	std::vector<BYTE> FileData;

private:
	std::vector<VPoint> Points;
	std::vector<VVertex> Wedges;
	std::vector<VTriangle> Faces;
	std::vector<VMaterial> Materials;
	std::vector<VBone> Bones;
	std::vector<VRawBoneInfluence> Influences;

private:
	std::vector<int> InfluencesPerBone;
	std::vector<int> InfluencesPerPoint;
	std::vector<int> InfluencesPerWedge;

private:
	ImpNode* MeshImpNode;
	INode* MeshINode;
	TriObject* MeshObject;
	IDerivedObject* MeshDerived;
	Modifier* SkinModifier;
	std::vector<ImpNode*> BoneImpNodes;
	std::vector<INode*> BoneINodes;
	std::vector<SimpleObject2*> BoneObjects;
	


public:
	DocumentImporter(const TCHAR* FileName, ImpInterface* ImportInterface, Interface* GlobalInterface, BOOL bSuppressPrompts);
	~DocumentImporter();

public:
	void Import();

private:
	void ParseHeader();
	template<class T> void ParseChunk( std::vector<T>& Array );

private:
	Matrix3 GetPivotTransform(INode* inode);
	void ImportNodes();
	void FinalizeMesh();
	void FinalizeBones();
	void ImportNode(ImpNode*& node, INode*& inode, const TCHAR* name, Matrix3 nodeTM);
	void ImportMesh();
	void ImportSkin();
	void ImportBones();
	BOOL AddWeights(ISkinImportData* importSkin, INode *node, int vertexID, Tab<INode*> &nodeList, Tab<float> &weights);
};


inline Point3 ToPoint3(const FVector& v)
{
	return Point3(v.X, v.Y, v.Z);
}
inline Quat ToQuat(const FQuat& v)
{
	return Quat(v.X, v.Y, v.Z, v.W);
}


#define STRING_SPACES " \t\r\n"

inline std::string trim_right (const std::string & s, const std::string & t = STRING_SPACES)
{ 
	std::string d (s); 
	std::string::size_type i (d.find_last_not_of (t));
	if (i == std::string::npos)
		return "";
	else
		return d.erase (d.find_last_not_of (t) + 1) ; 
}

inline std::string trim_left (const std::string & s, const std::string & t = STRING_SPACES) 
{ 
	std::string d (s); 
	return d.erase (0, s.find_first_not_of (t)) ; 
}

inline std::string trim (const std::string & s, const std::string & t = STRING_SPACES)
{ 
	std::string d (s); 
	return trim_left (trim_right (d, t), t) ; 
}

#pragma pack(pop)