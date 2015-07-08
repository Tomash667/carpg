#pragma once

#include "Chest.h"
#include "Location.h"
#include "Item.h"
#include "Unit.h"
#include "Mapa2.h"
#include "QuestId.h"

//-----------------------------------------------------------------------------
#define QUEST_DIALOG_START 0
#define QUEST_DIALOG_FAIL 1
#define QUEST_DIALOG_NEXT 2

//-----------------------------------------------------------------------------
struct Game;
struct DialogEntry;
struct Item;

//-----------------------------------------------------------------------------
struct Quest
{
	enum State
	{
		Hidden,
		Started,
		Completed,
		Failed
	};

	enum class Type
	{
		Mayor,
		Captain,
		Random,
		Unique
	};

	QUEST quest_id;
	State state;
	string name;
	int prog, refid, start_time, start_loc;
	uint quest_index;
	Type type;
	vector<string> msgs;
	static Game* game;

	Quest() : state(Hidden), prog(0)
	{

	}

	virtual ~Quest()
	{

	}

	virtual void Start() = 0;
	virtual DialogEntry* GetDialog(int type2) = 0;
	virtual void SetProgress(int prog2) = 0;
	virtual cstring FormatString(const string& str) = 0;
	virtual bool IsTimedout()
	{
		return false;
	}
	virtual bool IfHaveQuestItem()
	{
		return false;
	}
	virtual bool IfHaveQuestItem2(cstring id)
	{
		return false;
	}
	virtual bool IfNeedTalk(cstring topic)
	{
		return false;
	}
	virtual const Item* GetQuestItem()
	{
		return NULL;
	}
	virtual void Save(HANDLE file);
	virtual void Load(HANDLE file);

	// to powinno byæ inline ale nie wysz³o :/
	/*inline*/ Location& GetStartLocation();
	/*{
		return *game->locations[start_loc];
	}*/
	/*inline*/ const Location& GetStartLocation() const;
	/*{
		return *game->locations[start_loc];
	}*/
	/*inline*/ cstring GetStartLocationName() const;
	/*{
		return GetStartLocation().name.c_str();
	}*/
	virtual bool IfQuestEvent()
	{
		return false;
	}

	inline bool IsActive() const
	{
		return state == Hidden || state == Started;
	}
};

//-----------------------------------------------------------------------------
#define QUEST_ITEM_PLACEHOLDER ((const Item*)-1)

//-----------------------------------------------------------------------------
struct QuestItemClient
{
	string str_id;
	Item* item;

	QuestItemClient() : item(NULL) {}
	~QuestItemClient() { delete item; }
};

//-----------------------------------------------------------------------------
// u¿ywane w MP u klienta
struct PlaceholderQuest : public Quest
{
	void Start()
	{

	}
	DialogEntry* GetDialog(int type2)
	{
		return NULL;
	}
	virtual void SetProgress(int prog2)
	{

	}
	virtual cstring FormatString(const string& str)
	{
		return NULL;
	}
};

//-----------------------------------------------------------------------------
struct Quest_Encounter : public Quest
{
	int enc;

	Quest_Encounter() : enc(-1)
	{

	}

	void RemoveEncounter();
	virtual void Save(HANDLE file);
	virtual void Load(HANDLE file);
};

typedef void(*VoidFunc)();

//-----------------------------------------------------------------------------
// aktualnie next_event i item_to_give2 dzia³a tylko w podziemiach
// w mieœcie dzia³a tylko unit_to_spawn, unit_dont_attack, unit_auto_talk, unit_spawn_level, unit_event_handler i send_spawn_event (spawnuje w karczmie)
struct Quest_Event
{
	enum SpawnItem
	{
		Item_DontSpawn,
		Item_GiveStrongest,
		Item_GiveSpawned,
		Item_InTreasure, // tylko labirynt i krypta!
		Item_OnGround,
		Item_InChest, // tylko podziemia
		Item_GiveSpawned2 // tylko podziemia
	};

	static const int MAX_ITEMS = 4;

	SpawnItem spawn_item;
	const Item* item_to_give[MAX_ITEMS]; // wiele przedmiotów obs³ugiwane tylko w Item_InChest
	UnitData* unit_to_spawn, *unit_to_spawn2; // druga jednostka tylko w podziemiach
	int target_loc, at_level;
	bool done, unit_dont_attack, unit_auto_talk, spawn_2_guard_1; // obs³ugiwane tylko w podziemiach
	LocationEventHandler* location_event_handler;
	Quest_Event* next_event;
	ChestEventHandler* chest_event_handler;
	UnitEventHandler* unit_event_handler; // obs³ugiwane tylko w podziemiach
	bool whole_location_event_handler; // czy location_event_handler jest dla wszystkich poziomów czy tylko okreœlonego
	int spawn_unit_room; // gdzie stworzyæ jednostkê (np POKOJ_CEL_WIEZIENIE, POKOJ_CEL_BRAK = gdziekolwiek), tylko w podziemiach ma to efekt
	VoidFunc callback;
	bool send_spawn_event; // tylko w mieœcie
	int unit_spawn_level, unit_spawn_level2; // poziom jednostki (liczba-okreœlony, -1-minimalny, -2-losowy(min,max), -3-losowy(dostosowany do poziomu podziemi)

	Quest_Event() : done(false), item_to_give(), at_level(0), spawn_item(Item_DontSpawn), unit_to_spawn(NULL), unit_dont_attack(false), location_event_handler(NULL), target_loc(-1),
		next_event(NULL), chest_event_handler(NULL), unit_event_handler(NULL), unit_auto_talk(false), whole_location_event_handler(false), spawn_unit_room(POKOJ_CEL_BRAK),
		callback(NULL), unit_to_spawn2(NULL), send_spawn_event(false), unit_spawn_level(-2), unit_spawn_level2(-2), spawn_2_guard_1(false)
	{

	}
};

//-----------------------------------------------------------------------------
struct Quest_Dungeon : public Quest, public Quest_Event
{
	virtual void Save(HANDLE file);
	virtual void Load(HANDLE file);

	// to powinno byæ inline ale nie wysz³o :/
	/*inline*/ Location& GetTargetLocation();
	/*{
		return *game->locations[target_loc];
	}*/
	/*inline*/ const Location& GetTargetLocation() const;
	/*{
		return *game->locations[target_loc];
	}*/
	/*inline*/ cstring GetTargetLocationName() const;
	/*{
		return GetTargetLocation().name.c_str();
	}*/
	/*inline*/ cstring GetTargetLocationDir() const;
	/*{
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	}*/

	Quest_Event* GetEvent(int current_loc);
};
