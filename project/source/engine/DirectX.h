#pragma once

//-----------------------------------------------------------------------------
#include "WindowsIncludes.h"
#define far
#include <d3dx9.h>
#undef DrawText

//-----------------------------------------------------------------------------
#ifdef _DEBUG
extern HRESULT _d_hr;
#	define V(x) assert(SUCCEEDED(_d_hr = (x)))
#else
#	define V(x) (x)
#endif

//-----------------------------------------------------------------------------
template<typename T>
inline void SafeRelease(T& x)
{
	if(x)
	{
		x->Release();
		x = nullptr;
	}
}
