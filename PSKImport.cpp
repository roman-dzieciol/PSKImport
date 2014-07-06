//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "PSKImport.h"
#include "DocumentImporter.h"

#define PSKImport_CLASS_ID	Class_ID(0x6d0f6cbb, 0x7d2b0fc4)


class PSKImport : public SceneImport {
public:

	static HWND hParams;

	ImpInterface* ImportInterface;
	Interface* GlobalInterface;

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file

	//Constructor/Destructor
	PSKImport();
	~PSKImport();		

	BOOL FileRead(const TCHAR *filename, Mesh *msh, TriObject *object);
	void* MaxCreate(SClass_ID sid, Class_ID clid);

};



class PSKImportClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new PSKImport(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return PSKImport_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("PSKImport"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle


};

static PSKImportClassDesc PSKImportDesc;
ClassDesc2* GetPSKImportDesc() { return &PSKImportDesc; }




INT_PTR CALLBACK PSKImportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static PSKImport *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (PSKImport *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- PSKImport -------------------------------------------------------
PSKImport::PSKImport()
{

}

PSKImport::~PSKImport() 
{

}

int PSKImport::ExtCount()
{
	//#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *PSKImport::Ext(int n)
{		
	//#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("PSK");
}

const TCHAR *PSKImport::LongDesc()
{
	//#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("Unreal Engine 3 PSK Mesh File");
}

const TCHAR *PSKImport::ShortDesc() 
{			
	//#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("PSK");
}

const TCHAR *PSKImport::AuthorName()
{			
	//#pragma message(TODO("Return ASCII Author name"))
	return _T("Roman Dzieciol");
}

const TCHAR *PSKImport::CopyrightMessage() 
{	
	//#pragma message(TODO("Return ASCII Copyright message"))
	return _T("Copyright 2009 Roman Dzieciol");
}

const TCHAR *PSKImport::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *PSKImport::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int PSKImport::Version()
{				
	//#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void PSKImport::ShowAbout(HWND hWnd)
{			
	// Optional
}

int PSKImport::DoImport(const TCHAR *filename,ImpInterface *i,
						Interface *gi, BOOL suppressPrompts)
{
	ImportInterface = i;
	GlobalInterface = gi;
	//#pragma message(TODO("Implement the actual file import here and"))	

	/*if(!suppressPrompts)
	DialogBoxParam(hInstance, 
	MAKEINTRESOURCE(IDD_PANEL), 
	GetActiveWindow(), 
	PSKImportOptionsDlgProc, (LPARAM)this);*/

	try
	{
		DocumentImporter importer(filename, i, gi, suppressPrompts);
		importer.Import();

		ImportInterface->RedrawViews();

		return TRUE;
	}
	catch (std::exception& e)
	{
		DebugPrint(_T("exception caught! %hs \n"), e.what());
		return FALSE;
	}

	/*TriObject *object = CreateNewTriObject();
	if(!object)
	{
		return FALSE;
	}

	if(FileRead(filename, &object->GetMesh(), object)) 
	{
		return TRUE;
	}
	return FALSE;*/
}


// Create a max object
/*void* PSKImport::MaxCreate(SClass_ID sid, Class_ID clid)
{
	// Attempt to create
	if(ImportInterface == NULL)
		return NULL;

	void* c = ImportInterface->Create(sid, clid);

	if (c == NULL)
	{
		// This can be caused if the object referred to is in a deffered plugin.
		DllDir* dllDir = GetCOREInterface()->GetDllDirectory();
		ClassDirectory& classDir = dllDir->ClassDir();
		ClassEntry* classEntry = classDir.FindClassEntry(sid, clid);
		if(classEntry == NULL)
			return NULL;

		// This will force the loading of the specified plugin
		classEntry->FullCD();

		if(classEntry == NULL || classEntry->IsLoaded() != TRUE)
			return NULL;

		c = ImportInterface->Create(sid, clid);
	}
	return c;
}*/

BOOL PSKImport::FileRead( const TCHAR *filename, Mesh *msh, TriObject *object )
{
	try
	{
/*		PSKFile psk(filename);

		msh->setNumVerts(psk.Points.DataCount);
		msh->setNumTVerts(psk.Wedges.DataCount);
		msh->setNumFaces(psk.Faces.DataCount);

		for( int i=0; i<psk.Points.DataCount; ++i)
		{
			const VPoint point = psk.Points.Array.at(i);
			msh->setVert(i,point.Point.X, point.Point.Y, point.Point.Z);
		}

		for( int i=0; i<psk.Wedges.DataCount; ++i)
		{
			const VVertex wedge = psk.Wedges.Array.at(i);
			msh->setTVert(wedge.PointIndex, wedge.U, wedge.V, 0);
		}

		for( int i=0; i<psk.Faces.DataCount; ++i)
		{
			const VTriangle tri = psk.Faces.Array.at(i);
			msh->faces[i].setVerts(
				psk.Wedges.Array.at(tri.WedgeIndex[2]).PointIndex,
				psk.Wedges.Array.at(tri.WedgeIndex[1]).PointIndex,
				psk.Wedges.Array.at(tri.WedgeIndex[0]).PointIndex);

			msh->faces[i].setSmGroup(tri.SmoothingGroups);
			msh->faces[i].setMatID(tri.MatIndex);
			//msh->faces[i].setEdgeVisFlags(1, 1, 1);
		}


		msh->buildNormals();
		msh->buildBoundingBox();
		msh->InvalidateEdgeList();

		// Create the skin modifier
		Modifier* skinModifier = (Modifier*) MaxCreate(OSM_CLASS_ID, SKIN_CLASSID);
		if (skinModifier == NULL) return FALSE;

		// Create the derived object for the target and the modifier
		IDerivedObject* derivedObject = NULL;
		if (object->ClassID() == derivObjClassID || object->ClassID() == WSMDerivObjClassID)
		{
			// Object is a derived object, just attach ourselves to it
			derivedObject = (IDerivedObject*) object;
		}
		else
		{
			// Create the derived object for the target and the modifier
			derivedObject = CreateDerivedObject(object);
		}
		derivedObject->AddModifier(skinModifier);


		ImpNode *node = ImportInterface->CreateNode();
		if(!node) 
		{
			delete object;
			return FALSE;
		}

		Matrix3 tm;
		tm.IdentityMatrix();
		node->Reference(object);
		node->SetTransform(0,tm);
		ImportInterface->AddNodeToScene(node);
		node->SetName(GetString(IDS_PSK_OBJ_NAME));
		*/

		ImportInterface->RedrawViews();
		return TRUE;
	}
	catch(std::exception*)
	{
		return FALSE;
	}
}

