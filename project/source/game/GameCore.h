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
class ActionPanel;
class BookPanel;
class Console;
class Controls;
class CreateCharacterPanel;
class CreateServerPanel;
class Game;
class GameGui;
class GameMenu;
class GameMessages;
class GamePanel;
class GamePanelContainer;
class GameReader;
class GameWriter;
class InfoBox;
class Inventory;
class Journal;
class MainMenu;
class Minimap;
class MpBox;
class MultiplayerPanel;
class Options;
class PickServerPanel;
class QuestManager;
class SaveLoad;
class ScriptManager;
class ServerPanel;
class StatsPanel;
class TeamPanel;
class WorldMapGui;

//-----------------------------------------------------------------------------
struct Action;
struct AIController;
struct Armor;
struct BaseObject;
struct Blood;
struct Book;
struct Bow;
struct Building;
struct BuildingGroup;
struct BuildingScript;
struct Bullet;
struct Camera;
struct CameraCollider;
struct Chest;
struct City;
struct CityBuilding;
struct CollisionObject;
struct Consumable;
struct CreatedCharacter;
struct DialogContext;
struct DialogEntry;
struct Door;
struct Electro;
struct Encounter;
struct EntityInterpolator;
struct Explo;
struct GameDialog;
struct GroundItem;
struct HeroData;
struct Human;
struct HumanData;
struct Item;
struct ItemList;
struct ItemListResult;
struct ItemSlot;
struct InsideBuilding;
struct InsideLocationLevel;
struct LeveledItemList;
struct Light;
struct Location;
struct NetChangePlayer;
struct Object;
struct OtherItem;
struct PlayerController;
struct PlayerInfo;
struct Portal;
struct Quest_Dungeon;
struct Quest_Event;
struct Room;
struct Shield;
struct SpeechBubble;
struct Spell;
struct TerrainTile;
struct Trap;
struct Unit;
struct UnitData;
struct UnitEventHandler;
struct Usable;
struct Weapon;

//-----------------------------------------------------------------------------
enum TRAP_TYPE;
enum class OLD_BUILDING;

//-----------------------------------------------------------------------------
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
