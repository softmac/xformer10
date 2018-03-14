/* Precompiled.h */


#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__


#define STRICT

#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <tapi.h>
#include <direct.h>
#include <shlobj.h>
#include "resource.h"
#include "blockdev.h"
#include "gie.h"


#ifdef _DEBUG
	#define ASSERT(f) if (f) NULL; else MessageBox(NULL, #f, "Assert", MB_OK)
#else
	#define ASSERT(f) NULL
#endif


#endif /* __PRECOMPILED_H__ */
