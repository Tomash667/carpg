// console commands
#pragma once

//-----------------------------------------------------------------------------
enum CMD_FLAGS
{
	F_CHEAT = 1<<0,
	F_SINGLEPLAYER = 1<<1,
	F_MULTIPLAYER = 1<<2,
	F_LOBBY = 1<<3,
	F_SERVER = 1<<4,
	F_MENU = 1<<5,
	F_GAME = F_SINGLEPLAYER|F_MULTIPLAYER,
	F_NOT_MENU = F_SINGLEPLAYER|F_MULTIPLAYER|F_LOBBY,
	F_ANYWHERE = F_NOT_MENU|F_MENU,
	F_WORLD_MAP = 1<<6,
	F_NO_ECHO = 1<<7,
	F_MP_VAR = 1<<8
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
struct Game;
typedef void(Game::* GameVoidPtr)();

//-----------------------------------------------------------------------------
enum class TargetCmd
{
	Hurt,
	BreakAction,
	Fall
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
		GameVoidPtr func_ptr;
	};
	union
	{
		VEC2P _float;
		INT2P _int;
		UINT2P _uint;
	};
	int flags;
	VoidF changed;

	ConsoleCommand() {}
	ConsoleCommand(GameVoidPtr func_ptr, cstring name, cstring desc, int flags) : func_ptr(func_ptr), name(name), desc(desc), flags(flags), type(VAR_NONE)
	{
		assert(name && desc);
	}
	ConsoleCommand(bool* var, cstring name, cstring desc, int flags, VoidF changed=nullptr) : var(var), name(name), desc(desc), flags(flags), type(VAR_BOOL),
		changed(changed)
	{
		assert(name && desc && var);
	}
	ConsoleCommand(int* var, cstring name, cstring desc, int flags, int _min=INT_MIN, int _max=INT_MAX, VoidF changed=nullptr) : var(var), name(name),
		desc(desc), flags(flags), type(VAR_INT), changed(changed)
	{
		assert(name && desc && var);
		_int.x = _min;
		_int.y = _max;
	}
	ConsoleCommand(uint* var, cstring name, cstring desc, int flags, uint _min=0, uint _max=UINT_MAX) : var(var), name(name), desc(desc), flags(flags),
		type(VAR_UINT)
	{
		assert(name && desc && var);
		_uint.x = _min;
		_uint.y = _max;
	}
	ConsoleCommand(float* var, cstring name, cstring desc, int flags, float _min=-inf(), float _max=inf()) : var(var), name(name), desc(desc), flags(flags),
		type(VAR_FLOAT)
	{
		assert(name && desc && var);
		_float.x = _min;
		_float.y = _max;
	}

	template<typename T>
	inline T& Get()
	{
		return *(T*)var;
	}

	static inline bool SortByName(const ConsoleCommand* cmd1, const ConsoleCommand* cmd2)
	{
		return strcmp(cmd1->name, cmd2->name) < 0;
	}
};
