#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include <intrin.h>
#include "Version.h"
#include "Language.h"
#include "ErrorHandler.h"

//-----------------------------------------------------------------------------
Logger* logger;
cstring RESTART_MUTEX_NAME = "CARPG-RESTART-MUTEX";
cstring MUTEX_NAME = "CaRpgMutex";
string g_system_dir, g_ctime;

//-----------------------------------------------------------------------------
bool ShowPickLanguageDialog(string& lang);

//=================================================================================================
// Parsuje linie komend z WinMain do main
//=================================================================================================
int ParseCmdLine(char* lpCmd, char*** out)
{
	int argc = 0;
	char* str = lpCmd;

	while(*str)
	{
		while(*str && *str == ' ')
			++str;

		if(*str)
		{
			if(*str == '"')
			{
				// znajdŸ zamykaj¹cy cudzys³ów
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

	// rozdziel argumenty
	char** argv = new char*[argc];
	char** cargv = argv;
	str = lpCmd;
	while(*str)
	{
		while(*str && *str == ' ')
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

	// ustaw
	*out = argv;
	return argc;
}

//=================================================================================================
void LogProcessorFeatures()
{
	bool x64     = false;
	bool MMX     = false;
	bool SSE     = false;
	bool SSE2    = false;
	bool SSE3    = false;
	bool SSSE3   = false;
	bool SSE41   = false;
	bool SSE42   = false;
	bool SSE4a   = false;
	bool AVX     = false;
	bool XOP     = false;
	bool FMA3    = false;
	bool FMA4    = false;

	int info[4];
	__cpuid(info, 0);
	int nIds = info[0];

	__cpuid(info, 0x80000000);
	int nExIds = info[0];

	// Detect Instruction Set
	if(nIds >= 1)
	{
		__cpuid(info,0x00000001);
		MMX   = (info[3] & ((int)1 << 23)) != 0;
		SSE   = (info[3] & ((int)1 << 25)) != 0;
		SSE2  = (info[3] & ((int)1 << 26)) != 0;
		SSE3  = (info[2] & ((int)1 <<  0)) != 0;

		SSSE3 = (info[2] & ((int)1 <<  9)) != 0;
		SSE41 = (info[2] & ((int)1 << 19)) != 0;
		SSE42 = (info[2] & ((int)1 << 20)) != 0;

		AVX   = (info[2] & ((int)1 << 28)) != 0;
		FMA3  = (info[2] & ((int)1 << 12)) != 0;
	}

	if(nExIds >= 0x80000001)
	{
		__cpuid(info,0x80000001);
		x64   = (info[3] & ((int)1 << 29)) != 0;
		SSE4a = (info[2] & ((int)1 <<  6)) != 0;
		FMA4  = (info[2] & ((int)1 << 16)) != 0;
		XOP   = (info[2] & ((int)1 << 11)) != 0;
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

	LOG(s);
}

struct InstallScript
{
	string filename;
	int version;

	inline bool operator < (const InstallScript& s) const
	{
		return version < s.version;
	}
};

//=================================================================================================
bool RunInstallScripts()
{
	LOG("Reading install scripts.");
	WIN32_FIND_DATA data;
	HANDLE find = FindFirstFile(Format("%s/install/*.txt", g_system_dir.c_str()), &data);
	if(find == INVALID_HANDLE_VALUE)
		return true;
	
	vector<InstallScript> scripts;

	Tokenizer t;
	t.AddKeyword("install", 0);
	t.AddKeyword("version", 1);
	t.AddKeyword("remove", 2);
	
	do 
	{
		int major, minor, patch;

		// read file to find version info
		try
		{
			if(t.FromFile(Format("%s/install/%s", g_system_dir.c_str(), data.cFileName)))
			{
				t.Next();
				if(t.MustGetKeywordId() == 2)
				{
					// old install script
					if(sscanf_s(data.cFileName, "%d.%d.%d.txt", &major, &minor, &patch) != 3)
					{
						if(sscanf_s(data.cFileName, "%d.%d.txt", &major, &minor) == 2)
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
						t.Throw(Format("Unknown install script version '%d'.", t.MustGetInt()));
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
				s.filename = data.cFileName;
				s.version = (((major&0xFF)<<16)|((minor&0xFF)<<8)|(patch&0xFF));
			}
		}
		catch(const Tokenizer::Exception& e)
		{
			WARN(Format("Unknown install script '%s': %s", data.cFileName, e.ToString()));
		}
	}
	while(FindNextFile(find, &data));

	FindClose(find);

	if(scripts.empty())
		return true;

	std::sort(scripts.begin(), scripts.end());

	GetModuleFileName(nullptr, BUF, 256);
	char buf[512], buf2[512];
	char* filename;
	GetFullPathName(BUF, 512, buf, &filename);
	*filename = 0;
	DWORD len = strlen(buf);

	LocalString s, s2;

	for(vector<InstallScript>::iterator it = scripts.begin(), end = scripts.end(); it != end; ++it)
	{
		cstring path = Format("%s/install/%s", g_system_dir.c_str(), it->filename.c_str());

		try
		{			
			if(!t.FromFile(path))
			{
				ERROR(Format("Failed to load install script '%s'.", it->filename.c_str()));
				continue;
			}
			LOG(Format("Using install script %s.", it->filename.c_str()));

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
					ERROR(Format("Invalid file path '%s'.", s2->c_str()));
					return false;
				}

				DeleteFile(buf2);
				t.Next();
			}

			DeleteFile(path);
		}
		catch(cstring err)
		{
			ERROR(Format("Failed to parse install script '%s': %s", path, err));
		}
	}

	return true;
}

//=================================================================================================
void LoadSystemDir()
{
	g_system_dir = "system";

	Config cfg;	
	if(cfg.Load("resource.cfg") == Config::OK)
		g_system_dir = cfg.GetString("system", "system");
}

//=================================================================================================
void GetCompileTime()
{
	int len = GetModuleFileName(nullptr, BUF, 256);
	HANDLE file;

	if(len == 256)
	{
		char* b = new char[2048];
		GetModuleFileName(nullptr, b, 2048);
		file = CreateFile(b, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		delete[] b;
	}
	else
		file = CreateFile(BUF, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if(file == INVALID_HANDLE_VALUE)
	{
		g_ctime = "0";
		return;
	}

	// read position of header
	int offset;
	SetFilePointer(file, 0x3C, nullptr, FILE_BEGIN);
	ReadFile(file, &offset, sizeof(offset), &tmp, nullptr);
	SetFilePointer(file, offset + 8, nullptr, FILE_BEGIN);

	// read time
	static_assert(sizeof(time_t) == 8, "time_t must be 64 bit");
	union TimeUnion
	{
		time_t t;
		struct
		{
			uint low;
			uint high;
		};
	};
	TimeUnion datetime = { 0 };
	ReadFile(file, &datetime.low, sizeof(datetime.low), &tmp, nullptr);

	CloseHandle(file);

	tm t;
	errno_t err = gmtime_s(&t, &datetime.t);
	if(err == 0)
	{
		strftime(BUF, 256, "%Y-%m-%d %H:%M:%S", &t);
		g_ctime = BUF;
	}
	else
		g_ctime = "0";
}

//=================================================================================================
// G³ówna funkcja programu
//=================================================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	ErrorHandler& error_handler = ErrorHandler::Get();
	error_handler.RegisterHandler();
	
	GetCompileTime();

	// logger (w tym przypadku prelogger bo jeszcze nie wiemy gdzie to zapisywaæ)
	PreLogger plog;
	logger = &plog;

	// stwórz foldery na zapisy
	CreateDirectory("saves", nullptr);
	CreateDirectory("saves/single", nullptr);
	CreateDirectory("saves/multi", nullptr);

	//-------------------------------------------------------------------------
	// pocz¹tek
	time_t t = time(0);
	tm t2;
	localtime_s(&t2, &t);
	LOG("CaRpg " VERSION_STR);
	LOG(Format("Date: %04d-%02d-%02d", t2.tm_year+1900, t2.tm_mon+1, t2.tm_mday));
	LOG(Format("Build date: %s", g_ctime.c_str()));
	LOG(Format("Process ID: %d", GetCurrentProcessId()));
	{
		cstring build_type =
#ifdef _DEBUG
		"debug ";
#else
		"release ";
#endif
		LOG(Format("Build type: %s", build_type));
	}	
	LogProcessorFeatures();

	Game game;
	Bool3 windowed = None,
		  console = None;
	game.cfg_file = "carpg.cfg";
	bool log_to_file;
	string log_filename;

	//-------------------------------------------------------------------------
	// parsuj linie komend
	int cmd_len = strlen(lpCmdLine)+1;
	char* cmd_line = new char[cmd_len];
	memcpy(cmd_line, lpCmdLine, cmd_len);
	char** argv;
	LOG("Parsing command line.");
	int argc = ParseCmdLine(cmd_line, &argv);
	bool restarted = false;

	for(int i=0; i<argc; ++i)
	{
		char c = argv[i][0];
		if(c != '-' && c != '+')
		{
			WARN(Format("Unknown command line parameter '%s'.", argv[i]));
			continue;
		}

		cstring arg = argv[i] + 1;
		if(c == '+')
			game.ParseConfigVar(arg);
		else
		{
			if(strcmp(arg, "config") == 0)
			{
				if(argc != i + 1 && argv[i + 1][0] != '-')
				{
					++i;
					game.cfg_file = argv[i];
					LOG(Format("Configuration file: %s", game.cfg_file.c_str()));
				}
				else
					WARN("No argument for parameter '-config'!");
			}
			else if(strcmp(arg, "single") == 0)
				game.quickstart = QUICKSTART_SINGLE;
			else if(strcmp(arg, "host") == 0)
				game.quickstart = QUICKSTART_HOST;
			else if(strcmp(arg, "join") == 0)
				game.quickstart = QUICKSTART_JOIN_LAN;
			else if(strcmp(arg, "joinip") == 0)
				game.quickstart = QUICKSTART_JOIN_IP;
			else if(strcmp(arg, "console") == 0)
				console = True;
			else if(strcmp(arg, "windowed") == 0)
				windowed = True;
			else if(strcmp(arg, "fullscreen") == 0)
				windowed = False;
			else if(strcmp(arg, "nosound") == 0)
				game.nosound = true;
			else if(strcmp(arg, "nomusic") == 0)
				game.nomusic = true;
			else if(strcmp(arg, "test") == 0)
			{
				game.testing = true;
				console = True;
			}
			else if(strcmp(arg, "delay-1") == 0)
			{
				HANDLE mutex = CreateMutex(nullptr, TRUE, MUTEX_NAME);
				if(mutex)
				{
					LOG("Created delay mutex.");
					game.mutex = mutex;
				}
				else
					ERROR("Failed to create delay mutex.");
			}
			else if(strcmp(arg, "delay-2") == 0)
			{
				LOG("Waiting for delay mutex creation.");
				HANDLE mutex;
				while(true)
				{
					mutex = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME);
					if(mutex != nullptr)
						break;
					else
						Sleep(250);
				}
				LOG("Waiting for mutex.");
				WaitForSingleObject(mutex, INFINITE);
				CloseHandle(mutex);
			}
			else if(strcmp(arg, "restart") == 0)
			{
				if(!restarted)
				{
					// try to open mutex
					LOG("Game restarted.");
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
				WARN(Format("Unknown switch '%s'.", arg));
		}
	}
	
	LoadSystemDir();

	//-------------------------------------------------------------------------
	// wczytaj plik konfiguracyjny
	LOG("Loading config file");
	Config& cfg = Game::Get().cfg;
	Config::Result result = cfg.Load(game.cfg_file.c_str());
	if(result == Config::NO_FILE)
		LOG(Format("Config file not found '%s'.", game.cfg_file.c_str()));
	else if(result == Config::PARSE_ERROR)
		ERROR(Format("Config file parse error '%s' : %s", game.cfg_file.c_str(), cfg.GetError().c_str()));

	error_handler.ReadConfiguration(cfg);

	// konsola
	if(console == None)
		console = cfg.GetBool3("console", False);
	if(console == True)
	{
		game.have_console = true;

		// konsola
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		// polskie znaki w konsoli, tymczasowe rozwi¹zanie
		SetConsoleCP(1250);
		SetConsoleOutputCP(1250);
	}

	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");

	// tryb okienkowy
	if(windowed == None)
	{
		Bool3 b = cfg.GetBool3("fullscreen", True);
		if(b == True)
			windowed = False;
		else
			windowed = True;
	}

	// rozdzielczoœæ
	int w, h;
	{
		const string& res = cfg.GetString("resolution", "0x0");
		if(sscanf_s(res.c_str(), "%dx%d", &w, &h) != 2)
		{
			WARN(Format("Settings: Invalid resolution value '%s'.", res.c_str()));
			w = 0;
			h = 0;
		}
		else
			LOG(Format("Settings: Resolution %dx%d.", w, h));
	}

	// refresh
	game.wnd_hz = cfg.GetInt("refresh", 0);
	LOG(Format("Settings: Refresh rate %d Hz.", game.wnd_hz));

	// adapter
	game.used_adapter = cfg.GetInt("adapter", 0);
	LOG(Format("Settings: Adapter %d.", game.used_adapter));

	// logowanie
	log_to_file = (cfg.GetBool3("log", True) == True);

	// plik logowania
	if(log_to_file)
		log_filename = cfg.GetString("log_filename", "log.txt");

	game.hardcore_option = ToBool(cfg.GetBool3("hardcore_mode", False));

	// nie zatrzymywanie gry w razie braku aktywnoœci okna
	if(cfg.GetBool3("inactive_update", False) == True)
		game.inactive_update = true;

	// dŸwiêk / muzyka
	bool play_sound = true, play_music = true;
	if(cfg.GetBool3("play_sound", True) == False)
	{
		cfg.Remove("play_sound");
		play_sound = false;
	}
	if(cfg.GetBool3("play_music", True) == False)
	{
		cfg.Remove("play_music");
		play_music = false;
	}
	if(game.nosound || cfg.GetBool3("nosound", False) == True)
	{
		game.nosound = true;
		LOG("Settings: no sound.");
	}
	if(game.nomusic || cfg.GetBool3("nomusic", False) == True)
	{
		game.nomusic = true;
		LOG("Settings: no music.");
	}
	if(game.nomusic && game.nosound)
		game.disabled_sound = true;
	game.sound_volume = clamp(cfg.GetInt("sound_volume", 100), 0, 100);
	game.music_volume = clamp(cfg.GetInt("music_volume", 100), 0, 100);
	if(!play_sound)
	{
		game.sound_volume = 0;
		cfg.Add("sound_volume", "0");
	}
	if(!play_music)
	{
		game.music_volume = 0;
		cfg.Add("music_volume", "0");
	}

	// ustawienia myszki
	game.mouse_sensitivity = clamp(cfg.GetInt("mouse_sensitivity", 50), 0, 100);
	game.mouse_sensitivity_f = lerp(0.5f, 1.5f, float(game.mouse_sensitivity) / 100);

	// tryb multiplayer
	game.player_name = cfg.GetString("nick", "");
#define LIMIT(x) if(x.length() > 16) x = x.substr(0,16)
	LIMIT(game.player_name);
	game.server_name = cfg.GetString("server_name", "");
	LIMIT(game.server_name);
	game.server_pswd = cfg.GetString("server_pswd", "");
	LIMIT(game.server_pswd);
	game.max_players = clamp(cfg.GetInt("server_players", DEFAULT_PLAYERS), MIN_PLAYERS, MAX_PLAYERS);
	game.server_ip = cfg.GetString("server_ip", "");
	game.mp_timeout = clamp(cfg.GetFloat("timeout", 10.f), 1.f, 3600.f);

	// szybki start
	if(game.quickstart == QUICKSTART_NONE)
	{
		const string& mode = cfg.GetString("quickstart", "");
		if(mode == "single")
			game.quickstart = QUICKSTART_SINGLE;
		else if(mode == "host")
			game.quickstart = QUICKSTART_HOST;
		else if(mode == "join")
			game.quickstart = QUICKSTART_JOIN_LAN;
		else if(mode == "joinip")
			game.quickstart = QUICKSTART_JOIN_IP;
	}

	// autopicked class in MP
	{
		const string& clas = cfg.GetString("autopick", "");
		if(!clas.empty())
		{
			if(clas == "random")
				game.autopick_class = Class::RANDOM;
			else
			{
				ClassInfo* ci = ClassInfo::Find(clas);
				if(ci)
				{
					if(ClassInfo::IsPickable(ci->class_id))
						game.autopick_class = ci->class_id;
					else
						WARN(Format("Settings [autopick]: Class '%s' is not pickable by players.", clas.c_str()));
				}
				else
					WARN(Format("Settings [autopick]: Invalid class '%s'.", clas.c_str()));
			}
		}
	}

	// autostart serwera
	game.autostart_count = cfg.GetInt("autostart");
	if(game.autostart_count < -1)
		game.autostart_count = -1;
	else if(game.autostart_count > MAX_PLAYERS || game.autostart_count == 0)
		game.autostart_count = -1;

	//game.kick_timer = max(0, cfg.GetInt("kick_timer", 900));
	game.mp_port = clamp(cfg.GetInt("port", PORT), 0, 0xFFFF);

	// autopicked class in quickstart
	{
		const string& clas = cfg.GetString("class", "");
		if(!clas.empty())
		{
			ClassInfo* ci = ClassInfo::Find(clas);
			if(ci)
			{
				if(ClassInfo::IsPickable(ci->class_id))
					game.quickstart_class = ci->class_id;
				else
					WARN(Format("Settings [class]: Class '%s' is not pickable by players.", clas.c_str()));
			}
			else
				WARN(Format("Settings [class]: Invalid class '%s'.", clas.c_str()));
		}
	}

	game.quickstart_name = cfg.GetString("name", "Test");
	if(game.quickstart_name.empty())
		game.quickstart_name = "Test";

	game.change_title_a = ToBool(cfg.GetBool3("change_title", False));

	// pozycja rozmiar okien
	int con_pos_x  = cfg.GetInt("con_pos_x" ),
		con_pos_y  = cfg.GetInt("con_pos_y" );

	if(game.have_console && (con_pos_x != -1 || con_pos_y != -1))
	{
		HWND con = GetConsoleWindow();
		RECT rect;
		GetWindowRect(con, &rect);
		if(con_pos_x != -1)
			rect.left = con_pos_x;
		if(con_pos_y != -1)
			rect.top = con_pos_y;
		SetWindowPos(con, 0, rect.left, rect.top, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
	}

	game.s_wnd_size.x = cfg.GetInt("wnd_size_x");
	game.s_wnd_size.y = cfg.GetInt("wnd_size_y");
	game.s_wnd_pos.x  = cfg.GetInt("wnd_pos_x" );
	game.s_wnd_pos.y  = cfg.GetInt("wnd_pos_y" );

	// multisampling
	int multisampling = cfg.GetInt("multisampling", 0),
		multisampling_quality = cfg.GetInt("multisampling_quality", 0);
	game.SetStartingMultisampling(multisampling, multisampling_quality);

	// inne
	game.cl_postfx = cfg.GetBool("cl_postfx", true);
	game.cl_normalmap = cfg.GetBool("cl_normalmap", true);
	game.cl_specularmap = cfg.GetBool("cl_specularmap", true);
	game.cl_glow = cfg.GetBool("cl_glow", true);
	game.shader_version = cfg.GetInt("cl_shader_version");
	if(game.shader_version != -1 && game.shader_version != 2 && game.shader_version != 3)
	{
		WARN(Format("Settings: Unknown shader version %d.", game.shader_version));
		game.shader_version = -1;
	}
	game.vsync = cfg.GetBool("vsync", true);
	game.grass_range = cfg.GetFloat("grass_range", 40.f);
	if(game.grass_range < 0.f)
		game.grass_range = 0.f;
	{
		const string& screenshot_format = cfg.GetString("screenshot_format", "jpg");
		if(screenshot_format == "jpg")
			game.screenshot_format = D3DXIFF_JPG;
		else if(screenshot_format == "bmp")
			game.screenshot_format = D3DXIFF_BMP;
		else if(screenshot_format == "tga")
			game.screenshot_format = D3DXIFF_TGA;
		else if(screenshot_format == "png")
			game.screenshot_format = D3DXIFF_PNG;
		else
		{
			WARN(Format("Settings: Unknown screenshot format '%s'. Defaulting to jpg.", screenshot_format.c_str()));
			game.screenshot_format = D3DXIFF_JPG;
		}
	}

	game.SetConfigVarsFromFile();
	game.ApplyConfigVars();

	//-------------------------------------------------------------------------
	// logger
	int ile = 0;
	if(game.have_console)
		++ile;
	if(log_to_file)
		++ile;

	if(ile == 2)
	{
		MultiLogger* multi = new MultiLogger;
		ConsoleLogger* clog = new ConsoleLogger;
		TextLogger* tlog = new TextLogger(log_filename.c_str());
		multi->loggers.push_back(clog);
		multi->loggers.push_back(tlog);
		plog.Apply(multi);
		logger = multi;
	}
	else if(ile == 1)
	{
		if(game.have_console)
		{
			ConsoleLogger* l = new ConsoleLogger;
			plog.Apply(l);
			logger = l;
		}
		else
		{
			TextLogger* l = new TextLogger(log_filename.c_str());
			plog.Apply(l);
			logger = l;
		}
	}
	else
	{
		NullLogger* l = new NullLogger;
		plog.Clear();
		logger = l;
	}

	//-------------------------------------------------------------------------
	// skrypty instalacyjne
	if(!RunInstallScripts())
	{
		MessageBox(nullptr, "Failed to run installation scripts. Check log for details.", nullptr, MB_OK|MB_ICONERROR|MB_TASKMODAL);
		return 3;
	}

	//-------------------------------------------------------------------------
	// jêzyk
	LoadLanguages();
	const string& lang = cfg.GetString("language", "");
	extern string g_lang_prefix;
	if(lang == "")
	{
		LocalString s;
		if(!ShowPickLanguageDialog(s.get_ref()))
		{
			ClearLanguages();
			delete[] cmd_line;
			delete[] argv;
			delete logger;
			return 2;
		}
		else
		{
			g_lang_prefix = s;
			cfg.Add("language", s->c_str());
		}
	}
	else
		g_lang_prefix = lang;

	//-------------------------------------------------------------------------
	// pseudolosowoœæ
	uint cfg_seed = cfg.GetUint("seed"), seed;
	if(cfg_seed == 0)
		seed = (uint)time(nullptr);
	else
	{
		seed = cfg_seed;
		game.force_seed = seed;
	}
	game.next_seed = cfg.GetUint("next_seed");
	game.force_seed_all = ToBool(cfg.GetBool3("force_seed", False));
	LOG(Format("Random seed: %u/%u/%d", seed, game.next_seed, (game.force_seed_all ? 1 : 0)));
	srand2(seed);

	// inne
	game.check_updates = ToBool(cfg.GetBool3("check_updates", True));
	game.skip_tutorial = ToBool(cfg.GetBool3("skip_tutorial", False));
	const string& wersja_str = cfg.GetString("skip_version", "");
	if(wersja_str.empty())
		game.skip_version = 0;
	else
	{
		game.skip_version = StringToVersion(wersja_str.c_str());
		if(game.skip_version == -1)
		{
			WARN("Settings: Invalid value for 'skip_version'.");
			game.skip_version = 0;
		}
	}

	// zapisz konfiguracjê
	game.SaveCfg();

	//-------------------------------------------------------------------------
	// rozpocznij grê 
	LOG("Starting game engine.");
	bool b = game.Start0(windowed != True, w, h);

	//-------------------------------------------------------------------------
	// sprz¹tanie
	delete[] cmd_line;
	delete[] argv;
	delete logger;
	
	return (b ? 0 : 1);
}
