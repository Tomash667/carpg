#pragma once

//-----------------------------------------------------------------------------
#include "EngineCore.h"
#include "File.h"
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

//-----------------------------------------------------------------------------
using namespace SLNet;

//-----------------------------------------------------------------------------
class ActionPanel;
class BitStreamReader;
class BitStreamWriter;
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
class GlobalGui;
class InfoBox;
class Inventory;
class Journal;
class LevelAreaContext;
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
struct Camp;
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
struct LoadingHandler;
struct Location;
struct LocationEventHandler;
struct NetChangePlayer;
struct News;
struct Object;
struct OtherItem;
struct Pole;
struct PlayerController;
struct PlayerInfo;
struct Portal;
struct Quest;
struct Quest_Dungeon;
struct Quest_Encounter;
struct Quest_Event;
struct QuestHandler;
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
struct Var;
struct VarsContainer;
struct Weapon;

//-----------------------------------------------------------------------------
// Location generators
class CaveGenerator;
class CityGenerator;
class DungeonGenerator;
class EncounterGenerator;
class ForestGenerator;
class LabyrinthGenerator;
class LocationGenerator;
class LocationGeneratorFactory;

//-----------------------------------------------------------------------------
enum EncounterMode;
enum TRAP_TYPE;
enum SPAWN_GROUP;
enum SpecialEncounter;
enum class OLD_BUILDING;
