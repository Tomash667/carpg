// error handling, crash reporting, writing stream log
#pragma once

//-----------------------------------------------------------------------------
#include "Config.h"
#include "File.h"

// -----------------------------------------------------------------------------
// Crash mode enumerator
// -----------------------------------------------------------------------------
enum class ENUM_CRASH_MODE
{
	E_None,
	E_Normal,
	E_DataSeg,
	E_Full
};

// -----------------------------------------------------------------------------
// Stram log mode enumerator
// -----------------------------------------------------------------------------
enum class ENUM_STREAMLOG_MODE
{
	E_None,
	E_Errors,
	E_Full
};

// -----------------------------------------------------------------------------
// Brief: Converts crash mode enumerator to the string literal. If crash mode is undefined, should have returned "normal"
// Input: CrashMode crash_mode	- enum describing specific crash mode
// Output: crashModeText		- crash mode converted to string literal
// -----------------------------------------------------------------------------
inline cstring ToString(ENUM_CRASH_MODE crash_mode)
{
	cstring cstring_crashMode; // Holds return value

	switch(crash_mode)
	{
	case ENUM_CRASH_MODE::E_None:
		cstring_crashMode = "none";
		break;
	case ENUM_CRASH_MODE::E_Normal:
		cstring_crashMode = "normal";
		break;
	case ENUM_CRASH_MODE::E_DataSeg:
		cstring_crashMode = "dataseg";
		break;
	case ENUM_CRASH_MODE::E_Full:
		cstring_crashMode = "full";
		break;
	default:
		cstring_crashMode = "normal";
		break;
	}
	return cstring_crashMode;
}

// -----------------------------------------------------------------------------
// Brief: Converts stream log mode enumerator to the string literal. If crash mode is undefined, should have returned "normal"
// Input: CrashMode crash_mode	- enum describing specific crash mode
// Output: crashModeText		- crash mode converted to string literal
// -----------------------------------------------------------------------------
inline cstring ToString(ENUM_STREAMLOG_MODE streamLogMode)
{
	cstring cstring_streamLogMode; // Holds return value;
	switch(streamLogMode)
	{
	case ENUM_STREAMLOG_MODE::E_None:
		cstring_streamLogMode = "none";
		break;
	case ENUM_STREAMLOG_MODE::E_Errors:
		cstring_streamLogMode = "errors";
		break;
	case ENUM_STREAMLOG_MODE::E_Full:
		cstring_streamLogMode = "full";
		break;
	default:
		cstring_streamLogMode = "errors";
	}
	return cstring_streamLogMode;
}

//-----------------------------------------------------------------------------
// Type to handle errors.
//-----------------------------------------------------------------------------
class ErrorHandler
{
	friend LONG WINAPI Crash(EXCEPTION_POINTERS* exc);

public:
	static ErrorHandler& Get()
	{
		return handler;
	}

	void HandleRegister();
	void ReadConfiguration(Config& cfg);
	void StartStream(Packet* packet, int type);
	void EndStream(bool ok);
	void WriteStream(const void* data, uint size, int type, const SystemAddress& adr);

private:
	struct WritePacket : ObjectPoolProxy<WritePacket>
	{
		vector<byte> data;
		uint size;
		int type;
		SystemAddress adr;
	};

	ErrorHandler();

	long HandleCrash(EXCEPTION_POINTERS* exc);

	static ErrorHandler handler;
	ENUM_CRASH_MODE crash_mode;
	ENUM_STREAMLOG_MODE stream_log_mode;
	string stream_log_file;
	FileWriter stream_log;
	Packet* current_packet;
	int current_stream_type;
	vector<WritePacket*> write_packets;
};
