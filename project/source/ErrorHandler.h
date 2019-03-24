// error handling, crash reporting, writing stream log
#pragma once

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
class ErrorHandler
{
public:
	static ErrorHandler& Get()
	{
		return handler;
	}

	void RegisterHandler(Config& cfg, const string& log_path);

private:
	ErrorHandler();

	static ErrorHandler handler;
	CrashMode crash_mode;
};
