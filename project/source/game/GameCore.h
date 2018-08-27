#pragma once

#include "EngineCore.h"
#include "WindowsIncludes.h"
#define far
#define IN
#define OUT
#include <slikenet/peerinterface.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/BitStream.h>
#undef far
#undef IN
#undef OUT
#undef NULL

using namespace SLNet;

//-----------------------------------------------------------------------------
class Camera;
class GameReader;
class GameWriter;
class ScriptManager;
struct AIController;
struct Blood;
struct Bullet;
struct Chest;
struct CreatedCharacter;
struct DialogContext;
struct Door;
struct Electro;
struct EntityInterpolator;
struct Explo;
struct GameDialog;
struct GroundItem;
struct HeroData;
struct Human;
struct HumanData;
struct Item;
struct Light;
struct Object;
struct PlayerController;
struct PlayerInfo;
struct Portal;
struct Room;
struct SpeechBubble;
struct Spell;
struct Trap;
struct Unit;
struct UnitEventHandler;
struct Usable;

// FIXME: remove
extern DWORD tmp;

//-----------------------------------------------------------------------------
// Funkcje zapisuj¹ce/wczytuj¹ce z pliku
//-----------------------------------------------------------------------------
template<typename T>
inline void WriteString(HANDLE file, const string& s)
{
	assert(s.length() <= std::numeric_limits<T>::max());
	T len = (T)s.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
		WriteFile(file, s.c_str(), len, &tmp, nullptr);
}

template<typename T>
inline void ReadString(HANDLE file, string& s)
{
	T len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
	{
		s.resize(len);
		ReadFile(file, (char*)s.c_str(), len, &tmp, nullptr);
	}
	else
		s.clear();
}

inline void WriteString1(HANDLE file, const string& s)
{
	WriteString<byte>(file, s);
}

inline void ReadString1(HANDLE file, string& s)
{
	ReadString<byte>(file, s);
}
