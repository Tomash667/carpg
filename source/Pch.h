#pragma once

//-----------------------------------------------------------------------------
// info o kompilacji PCH
#ifdef PRECOMPILED_INFO
#	pragma message("Compiling PCH...")
#endif

//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT

//-----------------------------------------------------------------------------
// warning o liœcie inicjalizacyjnej [struct C { C() : elem() {} int elem[10]; ]
#pragma warning (disable: 4351)

//-----------------------------------------------------------------------------
// obs³uga wersji debug
#ifndef _DEBUG
#	define NDEBUG
#	define _SECURE_SCL 0
#	define _HAS_ITERATOR_DEBUGGING 0
#else
#	define D3D_DEBUG_INFO
#	ifndef COMMON_ONLY
#		include <vld.h>
#	endif
#endif

//-----------------------------------------------------------------------------
#ifndef COMMON_ONLY
#	include <RakPeerInterface.h>
#	include <RakNetTypes.h>
#	include <MessageIdentifiers.h>
#	define __BITSTREAM_NATIVE_END
#	include <BitStream.h>
#else
#	include <cassert>
#endif
#ifndef NO_DIRECT_X
#	include <d3dx9.h>
#else
#	include <Windows.h>
#endif
#include <Shellapi.h>
#include <ctime>
#include <cstdio>
#include <vector>
#include <list>
#include <algorithm>
#include <limits>
#include <string>
#include <fstream>
#include <algorithm>
#ifndef COMMON_ONLY
#	include <fmod.hpp>
#	include <btBulletCollisionCommon.h>
#	include <BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h>
#	include <zlib.h>
#endif
#include <map>
#include <set>
#include <unordered_map>
#include <functional>

//-----------------------------------------------------------------------------
using std::string;
using std::vector;
using std::list;
using std::min;
using std::max;
#ifndef COMMON_ONLY
using namespace RakNet;
#endif

//-----------------------------------------------------------------------------
// undef some useless macros
#undef far
#undef near
#undef small
#undef DrawText
#undef IN
#undef OUT

//-----------------------------------------------------------------------------
// use rand2
#pragma deprecated (rand)
