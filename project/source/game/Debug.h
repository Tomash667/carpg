#pragma once

//-----------------------------------------------------------------------------
#include "KeyStates.h"

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define DEBUG_VALUE 0
#	define DEBUG_BOOL true
#else
#	define DEBUG_VALUE 1
#	define DEBUG_BOOL false
#endif

//-----------------------------------------------------------------------------
inline bool DebugKey(byte k)
{
	return DEBUG_BOOL && Key.Focus() && Key.Down(k);
}
