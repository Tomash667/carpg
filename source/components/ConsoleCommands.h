#pragma once

//-----------------------------------------------------------------------------
enum CMD
{
	CMD_GOTO_MAP,
	CMD_VERSION,
	CMD_QUIT,
	CMD_REVEAL,
	CMD_MAP2CONSOLE,
	CMD_ADD_ITEM,
	CMD_ADD_TEAM_ITEM,
	CMD_ADD_GOLD,
	CMD_ADD_TEAM_GOLD,
	CMD_SET_STAT,
	CMD_MOD_STAT,
	CMD_CMDS,
	CMD_HELP,
	CMD_SPAWN_UNIT,
	CMD_HEAL,
	CMD_KILL,
	CMD_LIST,
	CMD_HEAL_UNIT,
	CMD_SUICIDE,
	CMD_CITIZEN,
	CMD_SCREENSHOT,
	CMD_WARP,
	CMD_SCARE,
	CMD_INVISIBLE,
	CMD_KILLALL,
	CMD_SAVE,
	CMD_LOAD,
	CMD_REVEAL_MINIMAP,
	CMD_SKIP_DAYS,
	CMD_WHISPER,
	CMD_SERVER_SAY,
	CMD_KICK,
	CMD_READY,
	CMD_LEADER,
	CMD_EXIT,
	CMD_RANDOM,
	CMD_START,
	CMD_SAY,
	CMD_GODMODE,
	CMD_NOCLIP,
	CMD_PLAYER_DEVMODE,
	CMD_NOAI,
	CMD_PAUSE,
	CMD_MULTISAMPLING,
	CMD_QUICKSAVE,
	CMD_QUICKLOAD,
	CMD_RESOLUTION,
	CMD_QS,
	CMD_CLEAR,
	CMD_HURT,
	CMD_BREAK_ACTION,
	CMD_FALL,
	CMD_RELOAD_SHADERS,
	CMD_TILE_INFO,
	CMD_SET_SEED,
	CMD_CRASH,
	CMD_FORCE_QUEST,
	CMD_STUN,
	CMD_REFRESH_COOLDOWN,
	CMD_DRAW_PATH,
	CMD_VERIFY,
	CMD_ADD_EFFECT,
	CMD_REMOVE_EFFECT,
	CMD_LIST_EFFECTS,
	CMD_ADD_PERK,
	CMD_REMOVE_PERK,
	CMD_LIST_PERKS,
	CMD_SELECT,
	CMD_LIST_STATS,
	CMD_ADD_LEARNING_POINTS,
	CMD_CLEAN_LEVEL,
	CMD_ARENA,
	CMD_REMOVE_UNIT,
	CMD_ADD_EXP,
	CMD_NOCD,

	CMD_MAX
};

//-----------------------------------------------------------------------------
enum CMD_FLAGS
{
	F_CHEAT = 1 << 0,
	F_SINGLEPLAYER = 1 << 1,
	F_MULTIPLAYER = 1 << 2,
	F_LOBBY = 1 << 3,
	F_SERVER = 1 << 4,
	F_MENU = 1 << 5,
	F_GAME = F_SINGLEPLAYER | F_MULTIPLAYER,
	F_NOT_MENU = F_SINGLEPLAYER | F_MULTIPLAYER | F_LOBBY,
	F_ANYWHERE = F_NOT_MENU | F_MENU,
	F_WORLD_MAP = 1 << 6,
	F_NO_ECHO = 1 << 7,
	F_MP_VAR = 1 << 8
};

//-----------------------------------------------------------------------------
enum PARSE_SOURCE
{
	PS_UNKNOWN,
	PS_CONSOLE,
	PS_LOBBY,
	PS_CHAT
};

//-----------------------------------------------------------------------------
struct ConsoleCommand
{
	enum VarType
	{
		VAR_NONE,
		VAR_BOOL,
		VAR_FLOAT,
		VAR_INT,
		VAR_UINT
	};

	cstring name, desc;
	VarType type;
	union
	{
		void* var;
		CMD cmd;
	};
	union
	{
		Vec2 _float;
		Int2 _int;
		POD::Uint2 _uint;
	};
	int flags;
	VoidF changed;

	ConsoleCommand(CMD cmd, cstring name, cstring desc, int flags) : cmd(cmd), name(name), desc(desc), flags(flags), type(VAR_NONE)
	{
		assert(name && desc);
	}
	ConsoleCommand(bool* var, cstring name, cstring desc, int flags, VoidF changed = nullptr) : var(var), name(name), desc(desc), flags(flags), type(VAR_BOOL), changed(changed)
	{
		assert(name && desc && var);
	}
	ConsoleCommand(int* var, cstring name, cstring desc, int flags, int _min = INT_MIN, int _max = INT_MAX, VoidF changed = nullptr) : var(var), name(name), desc(desc), flags(flags), type(VAR_INT),
		changed(changed)
	{
		assert(name && desc && var);
		_int.x = _min;
		_int.y = _max;
	}
	ConsoleCommand(uint* var, cstring name, cstring desc, int flags, uint _min = 0, uint _max = UINT_MAX) : var(var), name(name), desc(desc), flags(flags), type(VAR_UINT)
	{
		assert(name && desc && var);
		_uint.x = _min;
		_uint.y = _max;
	}
	ConsoleCommand(float* var, cstring name, cstring desc, int flags, float _min = -Inf(), float _max = Inf()) : var(var), name(name), desc(desc), flags(flags), type(VAR_FLOAT)
	{
		assert(name && desc && var);
		_float.x = _min;
		_float.y = _max;
	}
	ConsoleCommand(const ConsoleCommand& cmd) : name(cmd.name), desc(cmd.desc), type(cmd.type), var(cmd.var), flags(cmd.flags), changed(cmd.changed)
	{
		memcpy(&_float, &cmd._float, sizeof(cmd._float));
	}

	template<typename T>
	T& Get()
	{
		return *(T*)var;
	}
};
