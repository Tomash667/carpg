#pragma once

#ifdef PROXY_EXPORT
#	define PROXY __declspec(dllexport)
#else
#	define PROXY __declspec(dllimport)
#endif

PROXY const char* AddChange(const char* ver, const char* prevVer, const char* path, unsigned int crc, bool recreate);
