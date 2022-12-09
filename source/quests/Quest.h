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
	Location* startLoc;
	int prog, startTime;
	uint questIndex;
	QuestCategory category;
	vector<string> msgs;
	bool timeout, isNew;

	Quest();
	virtual ~Quest() {}

	virtual void Init() {}
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
	cstring GetStartLocationName() const { return startLoc->name.c_str(); }
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

	SpawnItem spawnItem;
	const Item* itemToGive[MAX_ITEMS]; // multiple items works only in Item_InChest
	Location* targetLoc;
	int atLevel;
	bool done;
	LocationEventHandler* locationEventHandler;
	Quest_Event* nextEvent; // only works in dungeon
	ChestEventHandler* chestEventHandler;
	bool wholeLocationEventHandler; // is locationEventHandler is for all dungeon levels or only selected
	VoidDelegate callback;

	// unit
	UnitData* unitToSpawn, *unitToSpawn2; // second unit only in inside location
	bool unitDontAttack, unitAutoTalk, sendSpawnEvent;
	bool spawnGuards; // only works in inside location
	UnitEventHandler* unitEventHandler;
	int unitSpawnLevel, unitSpawnLevel2; // spawned unit levels - >=0-value, -1-minimum, -2-Random(min,max), -3-Random(dungeon units level)
	RoomTarget unitSpawnRoom; // room to spawn unit, only works in inside location
	RoomTarget unitSpawnRoom2; // room to spawn second unit, only works in inside location

	Quest_Event() : done(false), itemToGive(), atLevel(-1), spawnItem(Item_DontSpawn), unitToSpawn(nullptr), unitDontAttack(false),
		locationEventHandler(nullptr), targetLoc(nullptr), nextEvent(nullptr), chestEventHandler(nullptr), unitEventHandler(nullptr), unitAutoTalk(false),
		wholeLocationEventHandler(false), unitSpawnRoom(RoomTarget::None), callback(nullptr), unitToSpawn2(nullptr), sendSpawnEvent(false),
		unitSpawnLevel(-2), unitSpawnLevel2(-2), spawnGuards(false), unitSpawnRoom2(RoomTarget::None)
	{
	}
};

//-----------------------------------------------------------------------------
struct Quest_Dungeon : public Quest, public Quest_Event
{
	virtual void Save(GameWriter& f) override;
	virtual LoadResult Load(GameReader& f) override;

	cstring GetTargetLocationName() const;
	cstring GetTargetLocationDir() const;

	Quest_Event* GetEvent(Location* currentLoc);
};
