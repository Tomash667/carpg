// error handling, crash reporting, writing stream log
#pragma once

//-----------------------------------------------------------------------------
#include "Config.h"
#include "File.h"

//-----------------------------------------------------------------------------
enum class CrashMode
{
	None,
	Normal,
	DataSeg,
	Full
};

//-----------------------------------------------------------------------------
inline cstring ToString(CrashMode crash_mode)
{
	switch(crash_mode)
	{
	case CrashMode::None:
		return "none";
	default:
	case CrashMode::Normal:
		return "normal";
	case CrashMode::DataSeg:
		return "dataseg";
	case CrashMode::Full:
		return "full";
	}
}

//-----------------------------------------------------------------------------
enum class StreamLogMode
{
	None,
	Errors,
	Full
};

//-----------------------------------------------------------------------------
inline cstring ToString(StreamLogMode stream_log_mode)
{
	switch(stream_log_mode)
	{
	case StreamLogMode::None:
		return "none";
	default:
	case StreamLogMode::Errors:
		return "errors";
	case StreamLogMode::Full:
		return "full";
	}
}

//-----------------------------------------------------------------------------
class ErrorHandler
{
	friend LONG WINAPI Crash(EXCEPTION_POINTERS* exc);

public:
	static ErrorHandler& Get()
	{
		return handler;
	}

	void RegisterHandler(Config& cfg, const string& log_path);
	void UnregisterHandler();
	void StreamStart(Packet* packet, int type);
	void StreamEnd(bool ok);
	void StreamWrite(const void* data, uint size, int type, const SystemAddress& adr);

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
	CrashMode crash_mode;
	StreamLogMode stream_log_mode;
	string stream_log_file;
	FileWriter stream_log;
	Packet* current_packet;
	int current_stream_type;
	vector<WritePacket*> write_packets;
	bool registered;
};
