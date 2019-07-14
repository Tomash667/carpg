#pragma once

//-----------------------------------------------------------------------------
// debug version handling
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
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <queue>
#include <random>
#include <thread>
#include <DirectXMath.h>
#include <array>

//-----------------------------------------------------------------------------
using std::string;
using std::vector;
using std::list;
using std::min;
using std::max;
using std::thread;
using std::pair;
using std::array;
