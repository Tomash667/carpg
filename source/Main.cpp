#include "Pch.h"
#include "Game.h"

#include "Language.h"
#include "SaveSlot.h"
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
string g_system_dir;

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
	g_system_dir = cfg.GetString("system", "system");
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
	cfg.Rename("cl_normalmap", "use_normalmap");
	cfg.Rename("cl_specularmap", "use_specularmap");
	cfg.Rename("cl_postfx", "use_postfx");

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

	engine->LoadConfiguration(cfg);

	// crash reporter
	int crashMode = cfg.GetEnumValue("crash_mode", {
		{ "none", 0 },
		{ "normal", 1 },
		{ "dataseg", 2 },
		{ "full", 3 }
		}, 1);
	Info("Settings: crash_mode = %d", crashMode);
	CrashHandler::Register("CaRpg", VERSION_STR, "http://carpg.pl/crashrpt.php", crashMode);

	// multiplayer mode
	game->player_name = Truncate(cfg.GetString("nick"), 16);
	net->server_name = Truncate(cfg.GetString("server_name"), 16);
	net->password = Truncate(cfg.GetString("server_pswd"), 16);
	net->max_players = Clamp(cfg.GetUint("server_players", DEFAULT_PLAYERS), MIN_PLAYERS, MAX_PLAYERS);
	game->server_ip = cfg.GetString("server_ip");
	game->mp_timeout = Clamp(cfg.GetFloat("timeout", 10.f), 1.f, 3600.f);
	net->server_lan = cfg.GetBool("server_lan");
	net->join_lan = cfg.GetBool("join_lan");
	net->port = Clamp(cfg.GetInt("port", PORT), 0, 0xFFFF);

	// quickstart
	if(game->quickstart == QUICKSTART_NONE)
	{
		const string& mode = cfg.GetString("quickstart");
		if(mode == "single")
			game->quickstart = QUICKSTART_SINGLE;
		else if(mode == "host")
			game->quickstart = QUICKSTART_HOST;
		else if(mode == "join")
			game->quickstart = QUICKSTART_JOIN_LAN;
		else if(mode == "joinip")
			game->quickstart = QUICKSTART_JOIN_IP;
		else if(mode == "load")
			game->quickstart = QUICKSTART_LOAD;
		else if(mode == "loadmp")
			game->quickstart = QUICKSTART_LOAD_MP;
	}
	int slot = cfg.GetInt("loadslot", -1);
	if(slot >= 1 && slot <= SaveSlot::MAX_SLOTS)
		game->quickstart_slot = slot;

	// miscellaneous
	game->testing = cfg.GetBool("test");
	game->use_postfx = cfg.GetBool("use_postfx", true);
	game->use_glow = cfg.GetBool("cl_glow", true);
	game->hardcore_option = cfg.GetBool("hardcore_mode");
	game->inactive_update = cfg.GetBool("inactive_update");
	game->settings.grass_range = max(cfg.GetFloat("grass_range", 40.f), 0.f);
	game->settings.mouse_sensitivity = Clamp(cfg.GetInt("mouse_sensitivity", 50), 0, 100);
	game->settings.mouse_sensitivity_f = Lerp(0.5f, 1.5f, float(game->settings.mouse_sensitivity) / 100);
	game->screenshot_format = ImageFormatMethods::FromString(cfg.GetString("screenshot_format", "jpg"));
	if(game->screenshot_format == ImageFormat::Invalid)
	{
		Warn("Settings: Unknown screenshot format '%s'. Defaulting to jpg.", cfg.GetString("screenshot_format").c_str());
		game->screenshot_format = ImageFormat::JPG;
	}

	// game
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
