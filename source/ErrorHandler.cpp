#include "Pch.h"
#include "ErrorHandler.h"

#include "Config.h"
#include "Version.h"

//=================================================================================================
void DoCrash()
{
	Info("Crash test...");
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
	Info("Settings: crash_mode = %s", ToString(crash_mode));
	cfg.Add("crash_mode", ToString(crash_mode));

	// don't use https here!
	RegisterCrashHandler("CaRpg", VERSION_STR, "http://carpg.pl/crashrpt.php",
		log_path.empty() ? nullptr : log_path.c_str(), (int)crash_mode);
}
