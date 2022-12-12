#include "Pch.h"
#include "Game.h"

#include "Language.h"
#include "Utility.h"
#include "Version.h"

#include <AppEntry.h>
#include <clocale>
#include <CrashHandler.h>
#include <Engine.h>
#include <intrin.h>
#include <Render.h>

//-----------------------------------------------------------------------------
cstring RESTART_MUTEX_NAME = "CARPG-RESTART-MUTEX";
string gSystemDir;

//-----------------------------------------------------------------------------
bool ShowPickLanguageDialog(string& lang);

//=================================================================================================
void LogProcessorFeatures()
{
	bool x64 = false;
	bool MMX = false;
	bool SSE = false;
	bool SSE2 = false;
	bool SSE3 = false;
	bool SSSE3 = false;
	bool SSE41 = false;
	bool SSE42 = false;
	bool SSE4a = false;
	bool AVX = false;
	bool XOP = false;
	bool FMA3 = false;
	bool FMA4 = false;

	int info[4];
	__cpuid(info, 0);
	int nIds = info[0];

	__cpuid(info, 0x80000000);
	int nExIds = info[0];

	// detect Instruction Set
	if(nIds >= 1)
	{
		__cpuid(info, 0x00000001);
		MMX = (info[3] & ((int)1 << 23)) != 0;
		SSE = (info[3] & ((int)1 << 25)) != 0;
		SSE2 = (info[3] & ((int)1 << 26)) != 0;
		SSE3 = (info[2] & ((int)1 << 0)) != 0;

		SSSE3 = (info[2] & ((int)1 << 9)) != 0;
		SSE41 = (info[2] & ((int)1 << 19)) != 0;
		SSE42 = (info[2] & ((int)1 << 20)) != 0;

		AVX = (info[2] & ((int)1 << 28)) != 0;
		FMA3 = (info[2] & ((int)1 << 12)) != 0;
	}

	if(nExIds >= 0x80000001)
	{
		__cpuid(info, 0x80000001);
		x64 = (info[3] & ((int)1 << 29)) != 0;
		SSE4a = (info[2] & ((int)1 << 6)) != 0;
		FMA4 = (info[2] & ((int)1 << 16)) != 0;
		XOP = (info[2] & ((int)1 << 11)) != 0;
	}

	LocalString s = "Processor features: ";
	if(x64)
		s += "x64, ";
	if(MMX)
		s += "MMX, ";
	if(SSE)
		s += "SSE, ";
	if(SSE2)
		s += "SSE2, ";
	if(SSE3)
		s += "SSE3, ";
	if(SSSE3)
		s += "SSSE3, ";
	if(SSE41)
		s += "SSE41, ";
	if(SSE42)
		s += "SSE42, ";
	if(SSE4a)
		s += "SSE4a, ";
	if(AVX)
		s += "AVX, ";
	if(XOP)
		s += "XOP, ";
	if(FMA3)
		s += "FMA3, ";
	if(FMA4)
		s += "FMA4, ";

	if(s.at_back(1) == ',')
		s.pop(2);
	else
		s += "(none)";

	Info(s);
}

struct InstallScript
{
	string filename;
	int version;

	bool operator < (const InstallScript& s) const
	{
		return version < s.version;
	}
};

//=================================================================================================
bool RunInstallScripts()
{
	Info("Reading install scripts.");

	vector<InstallScript> scripts;

	Tokenizer t;
	t.AddKeyword("install", 0);
	t.AddKeyword("version", 1);
	t.AddKeyword("remove", 2);

	io::FindFiles("install/*.txt", [&](const io::FileInfo& info)
	{
		int major, minor, patch;

		// read file to find version info
		try
		{
			if(t.FromFile(Format("install/%s", info.filename)))
			{
				t.Next();
				if(t.MustGetKeywordId() == 2)
				{
					// old install script
					if(sscanf_s(info.filename, "%d.%d.%d.txt", &major, &minor, &patch) != 3)
					{
						if(sscanf_s(info.filename, "%d.%d.txt", &major, &minor) == 2)
							patch = 0;
						else
						{
							// unknown version
							major = 0;
							minor = 0;
							patch = 0;
						}
					}
				}
				else
				{
					t.AssertKeyword(0);
					t.Next();
					if(t.MustGetInt() != 1)
						t.Throw("Unknown install script version '%d'.", t.MustGetInt());
					t.Next();
					t.AssertKeyword(1);
					t.Next();
					major = t.MustGetInt();
					t.Next();
					minor = t.MustGetInt();
					t.Next();
					patch = t.MustGetInt();
				}

				InstallScript& s = Add1(scripts);
				s.filename = info.filename;
				s.version = (((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF));
			}
		}
		catch(const Tokenizer::Exception& e)
		{
			Warn("Unknown install script '%s': %s", info.filename, e.ToString());
		}

		return true;
	});

	if(scripts.empty())
		return true;

	std::sort(scripts.begin(), scripts.end());

	GetModuleFileName(nullptr, BUF, 256);
	char buf[512], buf2[512];
	char* filename;
	GetFullPathName(BUF, 512, buf, &filename);
	*filename = 0;
	uint len = strlen(buf);

	LocalString s, s2;

	for(vector<InstallScript>::iterator it = scripts.begin(), end = scripts.end(); it != end; ++it)
	{
		cstring path = Format("install/%s", it->filename.c_str());

		try
		{
			if(!t.FromFile(path))
			{
				Error("Failed to load install script '%s'.", it->filename.c_str());
				continue;
			}
			Info("Using install script %s.", it->filename.c_str());

			t.Next();
			t.AssertKeyword();
			if(t.MustGetKeywordId() == 0)
			{
				// skip install 1, version X Y Z W
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertKeyword(1);
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
			}

			while(true)
			{
				if(t.IsEof())
					break;
				t.AssertKeyword(2);

				t.Next();
				s2 = t.MustGetString();

				if(GetFullPathName(s2->c_str(), 512, buf2, nullptr) == 0 || strncmp(buf, buf2, len) != 0)
				{
					Error("Invalid file path '%s'.", s2->c_str());
					return false;
				}

				io::DeleteFile(buf2);
				t.Next();
			}

			io::DeleteFile(path);
		}
		catch(cstring err)
		{
			Error("Failed to parse install script '%s': %s", path, err);
		}
	}

	return true;
}

//=================================================================================================
void LoadResourcesConfig()
{
	Config cfg;
	cfg.Load("resource.cfg");
	Language::dir = cfg.GetString("languages", "lang");
	gSystemDir = cfg.GetString("system", "system");
	render->SetShadersDir(cfg.GetString("shaders", "shaders").c_str());
}

//=================================================================================================
void LoadConfiguration(char* cmdLine)
{
	// parse command line, load config
	Info("Settings: Parsing command line & config.");
	Config& cfg = game->cfg;
	cfg.ParseCommandLine(cmdLine);
	Config::Result result = cfg.Load("carpg.cfg");
	if(result == Config::NO_FILE)
		Info("Settings: Config file not found '%s'.", cfg.GetFileName().c_str());
	else if(result == Config::PARSE_ERROR)
		Error("Settings: Config file parse error '%s' : %s", cfg.GetFileName().c_str(), cfg.GetError().c_str());

	// pre V_0_13 compatibility
	cfg.Rename("cl_normalmap", "useNormalmap");
	cfg.Rename("cl_specularmap", "useSpecularmap");
	cfg.Rename("cl_postfx", "usePostfx");
	// pre V_0_19 compatibility
	cfg.Rename("change_title", "changeTitle");
	cfg.Rename("check_updates", "checkUpdates");
	cfg.Rename("cl_glow", "useGlow");
	cfg.Rename("con_pos", "conPos");
	cfg.Rename("con_size", "conSize");
	cfg.Rename("crash_mode", "crashMode");
	cfg.Rename("feature_level", "featureLevel");
	cfg.Rename("grass_range", "grassRange");
	cfg.Rename("hardcore_mode", "hardcoreOption");
	cfg.Rename("hidden_window", "hiddenWindow");
	cfg.Rename("inactive_update", "inactiveUpdate");
	cfg.Rename("join_lan", "joinLan");
	cfg.Rename("lobby_port", "lobbyPort");
	cfg.Rename("lobby_url", "lobbyUrl");
	cfg.Rename("log_filename", "logFilename");
	cfg.Rename("mouse_sensitivity", "mouseSensitivity");
	cfg.Rename("multisampling_quality", "multisamplingQuality");
	cfg.Rename("music_volume", "musicVolume");
	cfg.Rename("packet_logger", "packetLogger");
	cfg.Rename("players_devmode", "playersDevmode");
	cfg.Rename("proxy_port", "proxyPort");
	cfg.Rename("quickstart_class", "quickstartClass");
	cfg.Rename("quickstart_name", "quickstartName");
	cfg.Rename("screenshot_format", "screenshotFormat");
	cfg.Rename("server_ip", "serverIp");
	cfg.Rename("server_lan", "serverLan");
	cfg.Rename("server_name", "serverName");
	cfg.Rename("server_pswd", "serverPswd");
	cfg.Rename("server_players", "serverPlayers");
	cfg.Rename("skip_cutscene", "skipCutscene");
	cfg.Rename("skip_tutorial", "skipTutorial");
	cfg.Rename("sound_device", "soundDevice");
	cfg.Rename("sound_volume", "soundVolume");
	cfg.Rename("use_normalmap", "useNormalmap");
	cfg.Rename("use_postfx", "usePostfx");
	cfg.Rename("use_specularmap", "useSpecularmap");
	cfg.Rename("wnd_pos", "wndPos");
	cfg.Rename("wnd_size", "wndSize");

	// if restarted game, wait for previous application to close
	if(cfg.GetBool("restart"))
	{
		Info("Game restarted.");
		HANDLE mutex = OpenMutex(SYNCHRONIZE, FALSE, RESTART_MUTEX_NAME);
		if(mutex)
		{
			WaitForSingleObject(mutex, INFINITE);
			CloseHandle(mutex);
		}
	}

	// startup delay to synchronize mp game on localhost
	int delay = cfg.GetInt("delay", -1);
	if(delay > 0)
	{
		if(delay == 1)
			utility::InitDelayLock();
		else
			utility::WaitForDelayLock(delay);
	}

	// crash reporter
	int crashMode = cfg.GetEnumValue("crashMode", {
		{ "none", 0 },
		{ "normal", 1 },
		{ "dataseg", 2 },
		{ "full", 3 }
		}, 1);
	Info("Settings: crashMode = %d", crashMode);
	CrashHandler::Register("CaRpg", VERSION_STR, "http://carpg.pl/crashrpt.php", crashMode);

	// more configuration
	engine->LoadConfiguration(cfg);
	game->LoadCfg();
}

//=================================================================================================
// main program function
//=================================================================================================
int AppEntry(char* cmdLine)
{
	if(IsDebug() && IsDebuggerPresent() && !io::FileExists("fmod.dll"))
	{
		MessageBox(nullptr, "Invalid debug working directory (missing fmod.dll).", nullptr, MB_OK | MB_ICONERROR);
		return 1;
	}

	// logger (prelogger in this case, because we do not know where to log yet)
	Logger::SetInstance(new PreLogger);

	// beginning
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	Srand((uint)time(nullptr));
	time_t t = time(0);
	tm t2;
	localtime_s(&t2, &t);
	Info("CaRpg " VERSION_STR);
	Info("Date: %04d-%02d-%02d", t2.tm_year + 1900, t2.tm_mon + 1, t2.tm_mday);
	Info("Build date: %s", utility::GetCompileTime().c_str());
	Info("Process ID: %d", GetCurrentProcessId());
	Info("Build type: %s", IsDebug() ? "debug" : "release");
	LogProcessorFeatures();

	// settings
	Ptr<Game> game;
	::game = game.Get();
	LoadResourcesConfig();
	LoadConfiguration(cmdLine);

	// instalation scripts
	if(!RunInstallScripts())
	{
		MessageBox(nullptr, "Failed to run installation scripts. Check log for details.", nullptr, MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 3;
	}

	// language
	Language::Init();
	Language::LoadLanguages();
	const string& lang = game->cfg.GetString("language");
	if(lang.empty())
	{
		LocalString s;
		if(!ShowPickLanguageDialog(s.get_ref()))
		{
			Language::Cleanup();
			return 2;
		}
		else
		{
			Language::SetPrefix(s);
			game->cfg.Add("language", s->c_str());
		}
	}
	else
		Language::SetPrefix(lang.c_str());

	// save configuration
	game->SaveCfg();

	// start the game
	Info("Starting game engine.");
	bool ok = game->Start();

	return (ok ? 0 : 1);
}
