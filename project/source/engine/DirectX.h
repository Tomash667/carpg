#pragma once

#include "WindowsIncludes.h"
#define NULL nullptr
#define far
#include <d3dx9.h>
#undef DrawText

#ifdef _DEBUG
extern HRESULT _d_hr;
#	define V(x) assert(SUCCEEDED(_d_hr = (x)))
#else
#	define V(x) (x)
#endif
