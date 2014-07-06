#pragma once

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
// DESCRIPTION: Includes for Plugins
// AUTHOR: 
//***************************************************************************/

#include "3dsmaxsdk_preinclude.h"
#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "iskin.h"
#include "modstack.h"
#include "mnmesh.h"
#include "dummy.h"
#include "simpobj.h"
#include "stdmat.h"

//SIMPLE TYPE

#include <direct.h>
#include <commdlg.h>


extern TCHAR *GetString(int id);

extern HINSTANCE hInstance;

#define TIME_INITIAL_POSE	0


// boneobj_params IDs
enum { 
	boneobj_width, boneobj_height, boneobj_taper, boneobj_length,
	boneobj_sidefins, boneobj_sidefins_size, boneobj_sidefins_starttaper, boneobj_sidefins_endtaper,
	boneobj_frontfin, boneobj_frontfin_size, boneobj_frontfin_starttaper, boneobj_frontfin_endtaper,
	boneobj_backfin,  boneobj_backfin_size,  boneobj_backfin_starttaper,  boneobj_backfin_endtaper,
	boneobj_genmap };

