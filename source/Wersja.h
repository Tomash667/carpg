#pragma once

//-----------------------------------------------------------------------------
// WARTOŒCI DO MODYFIKACJI
#define WERSJA_MAJOR 0
#define WERSJA_MINOR 3
#define WERSJA_PATCH 0
//#define BETA_BUILD
// WARTOŒCI DO MODYFIKACJI
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define IS_DEBUG
#	define DEBUG_VALUE 0
#	define DEBUG_BOOL true
#else
#	define DEBUG_VALUE 1
#	define DEBUG_BOOL false
#endif
#ifdef BETA_BUILD
#	define IS_BETA true
#else
#	define IS_BETA false
#endif
#if defined(BETA_BUILD) && !defined(DEV_BUILD) && !defined(_DEBUG)
#	define __STR2__(x) #x
#	define __STR1__(x) __STR2__(x)
#	define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "
#	pragma message(__LOC2__ "warning: BETA_BUILD in release build.")
#endif

//-----------------------------------------------------------------------------
#define GET_MAJOR(x) (((x)&(0xFF<<16))>>16)
#define GET_MINOR(x) (((x)&(0xFF<<8))>>8)
#define GET_PATCH(x) ((x)&0xFF)

//-----------------------------------------------------------------------------
#ifndef STRING
#	define _STRING(str) #str
#	define STRING(str) _STRING(str)
#endif

//-----------------------------------------------------------------------------
#define WERSJA ((WERSJA_MAJOR<<16)|(WERSJA_MINOR<<8)|(WERSJA_PATCH))
#define WERSJA_ZPAKOWANA WERSJA_MAJOR,WERSJA_MINOR,WERSJA_PATCH,0

//-----------------------------------------------------------------------------
#ifdef IS_BETA
#	ifdef IS_DEBUG
#		define RELEASE_TYPE VS_FF_PRERELEASE
#	else
#		define RELEASE_TYPE VS_FF_PRERELEASE|VS_FF_DEBUG
#	endif
#else
#	ifdef IS_DEBUG
#		define RELEASE_TYPE VS_FF_DEBUG
#	else
#		define RELEASE_TYPE 0
#	endif
#endif

//-----------------------------------------------------------------------------
#define WERSJA_STR STRING(WERSJA_MAJOR) "." STRING(WERSJA_MINOR) "." STRING(WERSJA_PATCH)
#ifdef BETA_BUILD
#	define VERSION_STR_FULL WERSJA_STR " (beta)"
#else
#	define VERSION_STR_FULL WERSJA_STR
#endif

//-----------------------------------------------------------------------------
cstring VersionToString(uint wersja);
uint StringToVersion(cstring wersja);
