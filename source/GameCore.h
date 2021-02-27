#pragma once

//-----------------------------------------------------------------------------
#include <File.h>
#include <Physics.h>
#include <Sound.h>
#include <Texture.h>
#include <Key.h>
#include <Entity.h>
#include <WindowsIncludes.h>
#define far
#define IN
#define OUT
#include <slikenet/peerinterface.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/BitStream.h>
#undef far
#undef IN
#undef OUT

//-----------------------------------------------------------------------------
// usings
using namespace SLNet;
using namespace app;

//-----------------------------------------------------------------------------
// angel script
class asIScriptContext;
class asIScriptEngine;
class asIScriptFunction;
class asIScriptObject;
class asIScriptModule;
class asITypeInfo;
class CScriptArray;
class CScriptDictionary;

//-----------------------------------------------------------------------------
class AbilityPanel;
class AIManager;
class Arena;
class BitStreamReader;
class BitStreamWriter;
class BookPanel;
class CommandParser;
class Console;
class Controls;
class CraftPanel;
class CreateCharacterPanel;
class CreateServerPanel;
class DungeonMeshBuilder;
class Game;
class GameComponent;
class GameGui;
class GameMenu;
class GameMessages;
class GamePanel;
class GamePanelContainer;
class GameReader;
class GameResources;
class GameStats;
class GameWriter;
class InfoBox;
class Inventory;
class InventoryPanel;
class Journal;
class Level;
class LevelAreaContext;
class LevelGui;
class LoadScreen;
class LobbyApi;
class MainMenu;
class Messenger;
class Minimap;
class MpBox;
class MultiplayerPanel;
class Net;
class Options;
class Pathfinding;
class PickServerPanel;
class QuestManager;
class Quest_Scripted;
class SaveLoad;
class ScriptManager;
class ServerPanel;
class StatsPanel;
class Team;
class TeamPanel;
class World;
class WorldMapGui;

//-----------------------------------------------------------------------------
struct Ability;
struct AbilityList;
struct AIController;
struct AITeam;
struct Amulet;
struct Armor;
struct BaseObject;
struct BaseTrap;
struct BaseUsable;
struct Blood;
struct Book;
struct Bow;
struct Building;
struct BuildingGroup;
struct BuildingScript;
struct Bullet;
struct CameraCollider;
struct Chest;
struct CityBuilding;
struct Class;
struct CollisionObject;
struct Consumable;
struct CreatedCharacter;
struct DialogContext;
struct DialogEntry;
struct DialogScripts;
struct Door;
struct Drain;
struct DrawBatch;
struct Electro;
struct Encounter;
struct EncounterSpawn;
struct EntityInterpolator;
struct Explo;
struct FrameInfo;
struct GameCamera;
struct GameDialog;
struct GameLight;
struct GlobalEncounter;
struct GroundItem;
struct Hero;
struct Human;
struct HumanData;
struct IdlePack;
struct Item;
struct ItemList;
struct ItemScript;
struct ItemSlot;
struct LevelArea;
struct LoadingHandler;
struct LocationEventHandler;
struct LocationTexturePack;
struct MapSettings;
struct Music;
struct NetChange;
struct NetChangePlayer;
struct News;
struct Object;
struct ObjectGroup;
struct OutsideObject;
struct OtherItem;
struct Perk;
struct PlayerController;
struct PlayerInfo;
struct Portal;
struct Quest;
struct Quest2;
struct Quest_Dungeon;
struct Quest_Encounter;
struct Quest_Event;
struct QuestDialog;
struct QuestHandler;
struct QuestInfo;
struct QuestList;
struct QuestScheme;
struct Recipe;
struct Ring;
struct Room;
struct RoomGroup;
struct RoomType;
struct SaveSlot;
struct ScriptEvent;
struct Shield;
struct SoundPack;
struct SpeechBubble;
struct StatProfile;
struct Stock;
struct TerrainTile;
struct TexPack;
struct Tile;
struct TmpLevelArea;
struct Trap;
struct Unit;
struct UnitData;
struct UnitEventHandler;
struct UnitGroup;
struct UnitList;
struct UnitStats;
struct Usable;
struct Var;
struct Vars;
struct Weapon;

//-----------------------------------------------------------------------------
// Locations
struct Camp;
struct Cave;
struct City;
struct InsideBuilding;
struct InsideLocation;
struct InsideLocationLevel;
struct Location;
struct MultiInsideLocation;
struct OffscreenLocation;
struct OutsideLocation;
struct SingleInsideLocation;

//-----------------------------------------------------------------------------
// Location generators
class AcademyGenerator;
class CampGenerator;
class CaveGenerator;
class CityGenerator;
class DungeonGenerator;
class EncounterGenerator;
class ForestGenerator;
class HillsGenerator;
class LabyrinthGenerator;
class LocationGenerator;
class LocationGeneratorFactory;
class MoonwellGenerator;
class SecretLocationGenerator;
class TutorialLocationGenerator;

//-----------------------------------------------------------------------------
enum CMD;
enum DialogOp : short;
enum Direction;
enum EncounterMode;
enum EntryType;
enum GameDirection;
enum ITEM_TYPE;
enum MATERIAL_TYPE;
enum TRAP_TYPE;
enum SpecialEncounter;
enum class AttributeId;
enum class EffectId;
enum class EffectSource;
enum class HeroType;
enum class Msg;
enum class MusicType;
enum class QuestCategory;
enum class RoomTarget;
enum class SkillId;

//-----------------------------------------------------------------------------
typedef pair<UnitData*, int> TmpSpawn;

//-----------------------------------------------------------------------------
extern AIManager* aiMgr;
extern LobbyApi* api;
extern CommandParser* cmdp;
extern Game* game;
extern GameGui* game_gui;
extern GameResources* game_res;
extern GameStats* game_stats;
extern Level* game_level;
extern Messenger* messenger;
extern Net* net;
extern Pathfinding* pathfinding;
extern CustomCollisionWorld* phy_world;
extern QuestManager* quest_mgr;
extern ScriptManager* script_mgr;
extern Team* team;
extern World* world;

//-----------------------------------------------------------------------------
#include "GameFile.h"
#include "SaveState.h"
