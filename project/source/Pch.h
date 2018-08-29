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
#define _WINSOCKAPI_ 

//-----------------------------------------------------------------------------
// warning o li�cie inicjalizacyjnej [struct C { C() : elem() {} int elem[10]; ]
#pragma warning (disable: 4351)

//-----------------------------------------------------------------------------
// obs�uga wersji debug
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
#include <cassert>
#include <ctime>
#include <vector>
#include <list>
#include <algorithm>
#include <limits>
#include <string>
#include <fstream>
#ifndef COMMON_ONLY
#	include <btBulletCollisionCommon.h>
#	include <BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h>
#endif
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <queue>
#include <random>
#include <DirectXMath.h>

//-----------------------------------------------------------------------------
using std::string;
using std::vector;
using std::list;
using std::min;
using std::max;
