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
void RegisterErrorHandler(Config& cfg, const string& log_path);
