#include "Pch.h"
#include "ErrorHandler.h"

#include "Config.h"
#include "Version.h"

//=================================================================================================
void DoCrash()
{
	int* z = nullptr;
#pragma warning(push)
#pragma warning(suppress: 6011)
	*z = 13;
#pragma warning(pop)
}

//=================================================================================================
void RegisterErrorHandler(Config& cfg, const string& log_path)
{
	CrashMode crash_mode = CrashMode::Normal;
	int version = cfg.GetVersion();

	if(version == 0)
	{
		int mode;
		Config::GetResult result = cfg.TryGetInt("crash_mode", mode);
		if(result == Config::GET_MISSING)
			mode = 0;
		else if(result == Config::GET_INVALID || mode < 0 || mode > 2)
		{
			mode = 0;
			Error("Settings: Invalid crash mode '%s'.", cfg.GetString("crash_mode").c_str());
		}
		switch(mode)
		{
		default:
		case 0:
			crash_mode = CrashMode::Normal;
			break;
		case 1:
			crash_mode = CrashMode::DataSeg;
			break;
		case 2:
			crash_mode = CrashMode::Full;
			break;
		}
	}
	else
	{
		Config::GetResult result = cfg.TryGetEnum<CrashMode>("crash_mode", crash_mode, {
			{ "none", CrashMode::None },
			{ "normal", CrashMode::Normal },
			{ "dataseg", CrashMode::DataSeg },
			{ "full", CrashMode::Full }
			});
		if(result != Config::GET_OK)
		{
			crash_mode = CrashMode::Normal;
			if(result == Config::GET_INVALID)
				Error("Settings: Invalid crash mode '%s'.", cfg.GetString("crash_mode").c_str());
		}
	}
	Info("Settings: crash_mode = %s", ToString(crash_mode));
	cfg.Add("crash_mode", ToString(crash_mode));

	// don't use https here!
	RegisterCrashHandler("CaRpg", VERSION_STR, "http://carpg.pl/crashrpt.php",
		log_path.empty() ? nullptr : log_path.c_str(), (int)crash_mode);
}
