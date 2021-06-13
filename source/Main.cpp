#include "Pch.h"
#include "Game.h"

#include "ErrorHandler.h"
#include "Language.h"
#include "SaveSlot.h"
#include "Utility.h"
#include "Version.h"

#include <AppEntry.h>
#include <clocale>
#include <Engine.h>
#include <intrin.h>
#include <Render.h>
#include <SceneManager.h>
#include <SoundManager.h>

//-----------------------------------------------------------------------------
cstring RESTART_MUTEX_NAME = "CARPG-RESTART-MUTEX";
string g_system_dir;

//-----------------------------------------------------------------------------
bool ShowPickLanguageDialog(string& lang);

//=================================================================================================
// parse command line (WinMain -> main)
//=================================================================================================
int ParseCmdLine(char* lpCmd, char*** out)
{
	int argc = 0;
	char* str = lpCmd;

	while(*str)
	{
		while(*str == ' ')
			++str;

		if(*str)
		{
			if(*str == '"')
			{
				// find closing quotemark
				++str;
				while(*str && *str != '"')
					++str;
				++str;
				++argc;
			}
			else
			{
				++argc;
				while(*str && *str != ' ')
					++str;
			}
		}
	}

	// split arguments
	char** argv = new char*[argc];
	char** cargv = argv;
	str = lpCmd;
	while(*str)
	{
		while(*str == ' ')
			++str;

		if(*str)
		{
			if(*str == '"')
			{
				++str;
				*cargv = str;
				++cargv;

				while(*str && *str != '"')
					++str;
			}
			else
			{
				*cargv = str;
				++cargv;

				while(*str && *str != ' ')
					++str;
			}
		}

		if(*str)
		{
			*str = 0;
			++str;
		}
	}

	// set
	*out = argv;
	return argc;
}

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
void LoadConfiguration(char* lpCmdLine)
{
	Config& cfg = game->cfg;
	game->cfg_file = "carpg.cfg";
	string log_filename;
	int delay = -1;
	Bool3 windowed = None,
		console = None;
	bool log_to_file, nosound = false, nomusic = false, restarted = false;

	// parse command line
	int cmd_len = strlen(lpCmdLine) + 1;
	char* cmd_line = new char[cmd_len];
	memcpy(cmd_line, lpCmdLine, cmd_len);
	char** argv;
	Info("Parsing command line.");
	int argc = ParseCmdLine(cmd_line, &argv);

	for(int i = 0; i < argc; ++i)
	{
		char c = argv[i][0];
		if(c != '-' && c != '+')
		{
			Warn("Unknown command line parameter '%s'.", argv[i]);
			continue;
		}

		cstring arg = argv[i] + 1;
		if(c == '+')
			cfg.ParseConfigVar(arg);
		else
		{
			if(strcmp(arg, "config") == 0)
			{
				if(argc != i + 1 && argv[i + 1][0] != '-')
				{
					++i;
					game->cfg_file = argv[i];
					Info("Configuration file: %s", game->cfg_file.c_str());
				}
				else
					Warn("No argument for parameter '-config'!");
			}
			else if(strcmp(arg, "single") == 0)
				game->quickstart = QUICKSTART_SINGLE;
			else if(strcmp(arg, "host") == 0)
				game->quickstart = QUICKSTART_HOST;
			else if(strcmp(arg, "join") == 0)
				game->quickstart = QUICKSTART_JOIN_LAN;
			else if(strcmp(arg, "joinip") == 0)
				game->quickstart = QUICKSTART_JOIN_IP;
			else if(strcmp(arg, "load") == 0)
				game->quickstart = QUICKSTART_LOAD;
			else if(strcmp(arg, "loadmp") == 0)
				game->quickstart = QUICKSTART_LOAD_MP;
			else if(strcmp(arg, "loadslot") == 0)
			{
				if(argc != i + 1 && argv[i + 1][0] != '-')
				{
					++i;
					int slot;
					if(!TextHelper::ToInt(argv[i], slot) || slot < 1 || slot > SaveSlot::MAX_SLOTS)
						Warn("Invalid loadslot value '%s'.", argv[i]);
					else
						game->quickstart_slot = slot;
				}
				else
					Warn("No argument for parameter '-loadslot'!");
			}
			else if(strcmp(arg, "console") == 0)
				console = True;
			else if(strcmp(arg, "windowed") == 0)
				windowed = True;
			else if(strcmp(arg, "fullscreen") == 0)
				windowed = False;
			else if(strcmp(arg, "nosound") == 0)
				nosound = true;
			else if(strcmp(arg, "nomusic") == 0)
				nomusic = true;
			else if(strcmp(arg, "test") == 0)
			{
				game->testing = true;
				console = True;
			}
			else if(strcmp(arg, "delay-0") == 0)
				delay = 0;
			else if(strcmp(arg, "delay-1") == 0)
				delay = 1;
			else if(strcmp(arg, "delay-2") == 0)
				delay = 2;
			else if(strcmp(arg, "delay-3") == 0)
				delay = 3;
			else if(strcmp(arg, "delay-4") == 0)
				delay = 4;
			else if(strcmp(arg, "restart") == 0)
			{
				if(!restarted)
				{
					// try to open mutex
					Info("Game restarted.");
					HANDLE mutex = OpenMutex(SYNCHRONIZE, FALSE, RESTART_MUTEX_NAME);
					if(mutex)
					{
						// wait for previous application to close
						WaitForSingleObject(mutex, INFINITE);
						CloseHandle(mutex);
					}
					restarted = true;
				}
			}
			else
				Warn("Unknown switch '%s'.", arg);
		}
	}

	delete[] cmd_line;
	delete[] argv;

	// load configuration file
	Info("Loading config file");
	Config::Result result = cfg.Load(game->cfg_file.c_str());
	if(result == Config::NO_FILE)
		Info("Config file not found '%s'.", game->cfg_file.c_str());
	else if(result == Config::PARSE_ERROR)
		Error("Config file parse error '%s' : %s", game->cfg_file.c_str(), cfg.GetError().c_str());

	// startup delay to synchronize mp game on localhost
	if(delay == -1)
		delay = cfg.GetInt("delay", delay);
	if(delay > 0)
	{
		if(delay == 1)
			utility::InitDelayLock();
		else
			utility::WaitForDelayLock(delay);
	}

	// console
	bool have_console = false;
	if(console == None)
		console = cfg.GetBool3("console", False);
	if(console == True)
		have_console = true;

	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");

	// window resolution & mode
	if(windowed == None)
	{
		Bool3 b = cfg.GetBool3("fullscreen", True);
		if(b == True)
			windowed = False;
		else
			windowed = True;
	}
	Int2 wnd_size = cfg.GetInt2("resolution");
	Info("Settings: Resolution %dx%d (%s).", wnd_size.x, wnd_size.y, windowed == False ? "fullscreen" : "windowed");
	engine->SetFullscreen(windowed == False);
	engine->SetWindowSize(wnd_size);

	// adapter
	int used_adapter = cfg.GetInt("adapter");
	Info("Settings: Adapter %d.", used_adapter);
	render->SetAdapter(used_adapter);

	// feature level
	{
		const string& featureLevel = cfg.GetString("feature_level", "");
		if(!featureLevel.empty())
		{
			if(!app::render->SetFeatureLevel(featureLevel))
				Warn("Settings: Invalid feature level '%s'.", featureLevel.c_str());
		}
	}

	// log
	log_to_file = (cfg.GetBool3("log", True) == True);

	// log file
	if(log_to_file)
		log_filename = cfg.GetString("log_filename", "log.txt");

	game->hardcore_option = cfg.GetBool("hardcore_mode");

	// window inactivity game stop prevention
	if(cfg.GetBool3("inactive_update", False) == True)
		game->inactive_update = true;

	// sound/music settings
	if(nosound || cfg.GetBool("nosound"))
	{
		nosound = true;
		Info("Settings: no sound.");
	}
	if(nomusic || cfg.GetBool("nomusic"))
	{
		nomusic = true;
		Info("Settings: no music.");
	}
	if(nosound || nomusic)
		sound_mgr->Disable(nosound, nomusic);
	sound_mgr->SetSoundVolume(Clamp(cfg.GetInt("sound_volume", 100), 0, 100));
	sound_mgr->SetMusicVolume(Clamp(cfg.GetInt("music_volume", 50), 0, 100));
	Guid soundDevice;
	if(soundDevice.TryParse(cfg.GetString("sound_device", Guid::EmptyString)))
		sound_mgr->SetDevice(soundDevice);

	// mouse settings
	game->settings.mouse_sensitivity = Clamp(cfg.GetInt("mouse_sensitivity", 50), 0, 100);
	game->settings.mouse_sensitivity_f = Lerp(0.5f, 1.5f, float(game->settings.mouse_sensitivity) / 100);

	// multiplayer mode
	game->player_name = cfg.GetString("nick", "");
#define LIMIT(x) if(x.length() > 16) x = x.substr(0,16)
	LIMIT(game->player_name);
	net->server_name = cfg.GetString("server_name", "");
	LIMIT(net->server_name);
	net->password = cfg.GetString("server_pswd", "");
	LIMIT(net->password);
	net->max_players = Clamp(cfg.GetUint("server_players", DEFAULT_PLAYERS), MIN_PLAYERS, MAX_PLAYERS);
	game->server_ip = cfg.GetString("server_ip", "");
	game->mp_timeout = Clamp(cfg.GetFloat("timeout", 10.f), 1.f, 3600.f);
	net->server_lan = cfg.GetBool("server_lan");
	net->join_lan = cfg.GetBool("join_lan");
	net->port = Clamp(cfg.GetInt("port", PORT), 0, 0xFFFF);

	// quickstart
	if(game->quickstart == QUICKSTART_NONE)
	{
		const string& mode = cfg.GetString("quickstart", "");
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

	// window position & size
	Int2 force_size = cfg.GetInt2("wnd_size", Int2(-1, -1)),
		force_pos = cfg.GetInt2("wnd_pos", Int2(-1, -1));
	engine->SetWindowInitialPos(force_pos, force_size);
	engine->HideWindow(cfg.GetBool("hidden_window"));

	// multisampling
	int multisampling = cfg.GetInt("multisampling"),
		multisampling_quality = cfg.GetInt("multisampling_quality");
	render->SetMultisampling(multisampling, multisampling_quality);

	// miscellaneous
	game->use_postfx = cfg.GetBool("use_postfx", "cl_postfx", true);
	scene_mgr->use_normalmap = cfg.GetBool("use_normalmap", "cl_normalmap", true);
	scene_mgr->use_specularmap = cfg.GetBool("use_specularmap", "cl_specularmap", true);
	game->use_glow = cfg.GetBool("cl_glow", true);
	render->SetVsync(cfg.GetBool("vsync", true));
	game->settings.grass_range = cfg.GetFloat("grass_range", 40.f);
	if(game->settings.grass_range < 0.f)
		game->settings.grass_range = 0.f;
	game->screenshot_format = ImageFormatMethods::FromString(cfg.GetString("screenshot_format", "jpg"));
	if(game->screenshot_format == ImageFormat::Invalid)
	{
		Warn("Settings: Unknown screenshot format '%s'. Defaulting to jpg.", cfg.GetString("screenshot_format").c_str());
		game->screenshot_format = ImageFormat::JPG;
	}

	cfg.LoadConfigVars();

	// logger
	PreLogger* plog = static_cast<PreLogger*>(Logger::GetInstance());
	int count = 0;
	if(have_console)
		++count;
	if(log_to_file)
		++count;

	Logger* logger;
	if(count == 2)
		logger = new MultiLogger({ new ConsoleLogger, new TextLogger(log_filename.c_str()) });
	else if(count == 1)
	{
		if(have_console)
			logger = new ConsoleLogger;
		else
			logger = new TextLogger(log_filename.c_str());
	}
	else
		logger = new Logger;
	plog->Apply(logger);
	Logger::SetInstance(logger);

	// pseudorandomness
	Srand((uint)time(nullptr));

	// console position & size
	const Int2 con_pos = cfg.GetInt2("con_pos", Int2(-1, -1));
	const Int2 con_size = cfg.GetInt2("con_size", Int2(-1, -1));
	if(have_console && (con_pos != Int2(-1, -1) || con_size != Int2(-1, -1)))
	{
		HWND con = GetConsoleWindow();
		Rect rect;
		GetWindowRect(con, (RECT*)&rect);
		Int2 pos = rect.LeftTop();
		Int2 size = rect.Size();
		if(con_pos.x != -1)
			pos.x = con_pos.x;
		if(con_pos.y != -1)
			pos.y = con_pos.y;
		if(con_size.x != -1)
			size.x = con_size.x;
		if(con_size.y != -1)
			size.y = con_size.y;
		SetWindowPos(con, 0, pos.x, pos.y, size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}

	game->LoadCfg();

	// crash reporter
	RegisterErrorHandler(cfg, log_filename);
}

//=================================================================================================
// main program function
//=================================================================================================
int AppEntry(char* lpCmdLine)
{
	if(IsDebug() && IsDebuggerPresent() && !io::FileExists("fmod.dll"))
	{
		MessageBox(nullptr, "Invalid debug working directory (missing fmod.dll).", nullptr, MB_OK | MB_ICONERROR);
		return 1;
	}

	// logger (prelogger in this case, because we do not know where to log yet)
	Logger::SetInstance(new PreLogger);

	// beginning
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
	LoadConfiguration(lpCmdLine);

	// instalation scripts
	if(!RunInstallScripts())
	{
		MessageBox(nullptr, "Failed to run installation scripts. Check log for details.", nullptr, MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 3;
	}

	// language
	Language::Init();
	Language::LoadLanguages();
	const string& lang = game->cfg.GetString("language", "");
	if(lang == "")
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
