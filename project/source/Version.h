#pragma once

//-----------------------------------------------------------------------------
#define VERSION_MAJOR 0
#define VERSION_MINOR 5
#define VERSION_PATCH 0

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define DEBUG_VALUE 0
#	define DEBUG_BOOL true
#else
#	define DEBUG_VALUE 1
#	define DEBUG_BOOL false
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
#define VERSION ((VERSION_MAJOR<<16)|(VERSION_MINOR<<8)|(VERSION_PATCH))
#define VERSION_PACKED VERSION_MAJOR,VERSION_MINOR,VERSION_PATCH,0

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define RELEASE_TYPE VS_FF_DEBUG
#else
#	define RELEASE_TYPE 0
#endif

//-----------------------------------------------------------------------------
#define VERSION_STR STRING(VERSION_MAJOR) "." STRING(VERSION_MINOR) "." STRING(VERSION_PATCH)

//-----------------------------------------------------------------------------
cstring VersionToString(uint wersja);
uint StringToVersion(cstring wersja);
