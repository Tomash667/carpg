#pragma once

//-----------------------------------------------------------------------------
#include "QuestHandler.h"
#include "Chest.h"
#include "Location.h"
#include "Item.h"
#include "Unit.h"
#include "Room.h"
#include "QuestConsts.h"

//-----------------------------------------------------------------------------
enum QuestDialogType
{
	QUEST_DIALOG_START,
	QUEST_DIALOG_FAIL,
	QUEST_DIALOG_NEXT
};

//-----------------------------------------------------------------------------
enum TimeoutType
{
	TIMEOUT_NORMAL,
	TIMEOUT_CAMP,
	TIMEOUT2,
	TIMEOUT_ENCOUNTER
};

//-----------------------------------------------------------------------------
struct Quest : public QuestHandler
{
	enum State
	{
		Hidden,
		Started,
		Completed,
		Failed
	};

	enum class LoadResult
	{
		Ok,
		Remove,
		Convert
	};

	struct ConversionData
	{
		cstring id;
		Vars* vars;

		void Add(cstring key, bool value);
		void Add(cstring key, int value);
		void Add(cstring key, Location* location);
		void Add(cstring key, const Item* item);
		void Add(cstring key, UnitGroup* group);
		void Add(cstring key, void*) = delete;
	};

	int id;
	QUEST_TYPE type;
	State state;
	string name;
	int prog, start_time, start_loc;
	uint quest_index;
	QuestCategory category;
	vector<string> msgs;
	bool timeout;

	Quest();
	virtual ~Quest() {}

	virtual void Start() = 0;
	virtual GameDialog* GetDialog(int type2) = 0;
	virtual void SetProgress(int prog2) = 0;
	// called on quest timeout, return true if timeout handled (if false it will be called on next time update)
	virtual bool OnTimeout(TimeoutType ttype) { return true; }

	virtual bool IsTimedout() const { return false; }
	virtual bool IfHaveQuestItem() const { return false; }
	virtual bool IfHaveQuestItem2(cstring id) const { return false; }
	virtual bool IfNeedTalk(cstring topic) const { return false; }
	virtual bool IfQuestEvent() const { return false; }
	virtual const Item* GetQuestItem() { return nullptr; }

	virtual void Save(GameWriter& f);
	virtual LoadResult Load(GameReader& f);
	virtual void GetConversionData(ConversionData& data) {}

	void OnStart(cstring name);
	void OnUpdate(const std::initializer_list<cstring>& msgs);
	template<typename ...Args>
	void OnUpdate(Args... args)
	{
		OnUpdate({ args... });
	}
	Location& GetStartLocation();
	const Location& GetStartLocation() const;
	cstring GetStartLocationName() const;
	bool IsActive() const { return state == Hidden || state == Started; }
};

//-----------------------------------------------------------------------------
// Used by MP clients (store only journal entries and status)
struct PlaceholderQuest : public Quest
{
	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	cstring FormatString(const string& str) override { return nullptr; }
};

//-----------------------------------------------------------------------------
struct Quest_Encounter : public Quest
{
	int enc;

	Quest_Encounter() : enc(-1)
	{
	}

	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void RemoveEncounter();
};

typedef void(*VoidFunc)();

//-----------------------------------------------------------------------------
struct Quest_Event
{
	enum SpawnItem
	{
		Item_DontSpawn,
		Item_GiveStrongest,
		Item_GiveSpawned,
		Item_InTreasure, // only crypt or labyrinth
		Item_OnGround,
		Item_InChest, // only dungeon
		Item_GiveSpawned2 // only dungeon
	};

	static const int MAX_ITEMS = 4;

	SpawnItem spawn_item;
	const Item* item_to_give[MAX_ITEMS]; // multiple items works only in Item_InChest
	int target_loc, at_level;
	bool done;
	LocationEventHandler* location_event_handler;
	Quest_Event* next_event; // only works in dungeon
	ChestEventHandler* chest_event_handler;
	bool whole_location_event_handler; // is location_event_handler is for all dungeon levels or only selected
	VoidDelegate callback;

	// unit
	UnitData* unit_to_spawn, *unit_to_spawn2; // second unit only in inside location
	bool unit_dont_attack, unit_auto_talk, send_spawn_event;
	bool spawn_2_guard_1; // only works in inside location
	UnitEventHandler* unit_event_handler;
	int unit_spawn_level, unit_spawn_level2; // spawned unit levels - >=0-value, -1-minimum, -2-Random(min,max), -3-Random(dungeon units level)
	RoomTarget spawn_unit_room; // room to spawn unit, only works in inside location
	RoomTarget spawn_unit_room2; // room to spawn second unit, only works in inside location

	Quest_Event() : done(false), item_to_give(), at_level(-1), spawn_item(Item_DontSpawn), unit_to_spawn(nullptr), unit_dont_attack(false),
		location_event_handler(nullptr), target_loc(-1), next_event(nullptr), chest_event_handler(nullptr), unit_event_handler(nullptr), unit_auto_talk(false),
		whole_location_event_handler(false), spawn_unit_room(RoomTarget::None), callback(nullptr), unit_to_spawn2(nullptr), send_spawn_event(false),
		unit_spawn_level(-2), unit_spawn_level2(-2), spawn_2_guard_1(false), spawn_unit_room2(RoomTarget::None)
	{
	}
};

//-----------------------------------------------------------------------------
struct Quest_Dungeon : public Quest, public Quest_Event
{
	virtual void Save(GameWriter& f) override;
	virtual LoadResult Load(GameReader& f) override;

	Location& GetTargetLocation();
	const Location& GetTargetLocation() const;
	cstring GetTargetLocationName() const;
	cstring GetTargetLocationDir() const;

	Quest_Event* GetEvent(int current_loc);
};
