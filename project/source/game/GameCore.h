#pragma once

//-----------------------------------------------------------------------------
#include "EngineCore.h"
#include "File.h"
#include "Physics.h"
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
class Arena;
class BitStreamReader;
class BitStreamWriter;
class BookPanel;
class Console;
class Controls;
class CreateCharacterPanel;
class CreateServerPanel;
class Game;
class GameComponent;
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
class InventoryPanel;
class Journal;
class Level;
class LevelAreaContext;
class LoadScreen;
class MainMenu;
class Minimap;
class MpBox;
class MultiplayerPanel;
class Options;
class Pathfinding;
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
struct ItemScript;
struct ItemSlot;
struct LevelContext;
struct LeveledItemList;
struct Light;
struct LoadingHandler;
struct LocationEventHandler;
struct LocationTexturePack;
struct NetChangePlayer;
struct News;
struct Object;
struct OutsideObject;
struct OtherItem;
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
struct SpellList;
struct TerrainTile;
struct Tile;
struct Trap;
struct Unit;
struct UnitData;
struct UnitEventHandler;
struct UnitGroup;
struct Usable;
struct Var;
struct VarsContainer;
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
struct OutsideLocation;
struct SingleInsideLocation;

//-----------------------------------------------------------------------------
// Location generators
class CampGenerator;
class CaveGenerator;
class CityGenerator;
class DungeonGenerator;
class EncounterGenerator;
class ForestGenerator;
class LabyrinthGenerator;
class LocationGenerator;
class LocationGeneratorFactory;
class MoonwellGenerator;
class SecretLocationGenerator;
class TutorialLocationGenerator;

//-----------------------------------------------------------------------------
enum Direction;
enum EncounterMode;
enum GameDirection;
enum MATERIAL_TYPE;
enum TRAP_TYPE;
enum SPAWN_GROUP;
enum SpecialEncounter;
enum class Class;
enum class OLD_BUILDING;
