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

//-----------------------------------------------------------------------------
struct Quest_DostarczList : public Quest
{
	int end_loc;
	OtherItem letter;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem();
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_DostarczPaczke : public Quest_Encounter
{
	int end_loc;
	OtherItem parcel;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem();
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_RozniesWiesci : public Quest
{
	struct Entry
	{
		int location;
		float dist;
		bool given;
	};
	vector<Entry> entries;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_OdzyskajPaczke : public Quest_Dungeon
{
	int from_loc;
	OtherItem parcel;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem();
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_UratujPorwanaOsobe : public Quest_Dungeon, public UnitEventHandler
{
	SPAWN_GROUP group;
	Unit* captive;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
struct Quest_BandyciPobierajaOplate : public Quest_Encounter, public LocationEventHandler
{
	int other_loc;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
struct Quest_ObozKoloMiasta : public Quest_Dungeon, public LocationEventHandler
{
	SPAWN_GROUP group;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
struct Quest_ZabijZwierzeta : public Quest_Dungeon, public LocationEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
struct Quest_ZnajdzArtefakt : public Quest_Dungeon
{
	int co;
	const Item* item;
	OtherItem quest_item;
	string item_id;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem2(cstring id);
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_ZgubionyPrzedmiot : public Quest_Dungeon
{
	int co;
	const Item* item;
	OtherItem quest_item;
	string item_id;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem2(cstring id);
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_UkradzionyPrzedmiot : public Quest_Dungeon
{
	int co;
	const Item* item;
	OtherItem quest_item;
	string item_id;
	SPAWN_GROUP group;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	bool IfHaveQuestItem2(cstring id);
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Quest_Tartak : public Quest_Dungeon, public LocationEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
struct Quest_Kopalnia : public Quest_Dungeon, public LocationEventHandler, public ChestEventHandler
{
	Quest_Event sub;
	int dungeon_loc;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleChestEvent(ChestEventHandler::Event event);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetChestEventHandlerQuestRefid()
	{
		return refid;
	}

	void InitSub();
};

//-----------------------------------------------------------------------------
struct Quest_Bandyci : public Quest_Dungeon, public LocationEventHandler, public UnitEventHandler
{
	int enc, other_loc, camp_loc;
	bool pewny_list;

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
// jakiœ facet chce ¿eby odnaleŸæ dla niego magiczny artefakt
// idziemy do krypty i go przynosimy
// p³aci i znika jak pos³aniec
// po jakimœ czasie natrafiamy na golema na drodze który ka¿e nam oddaæ ca³e z³oto, dostajemy questa "Golem na drodze"
// mo¿emy porozmawiaæ z dowolnym kapitanem stra¿y który mówi o tym ¿e ma wiêcej informacji o golemach bududowanych przez magów
// wysy³a nas do jakiegoœ maga w wiosce
// jest pijany i chce piwo, potem wódkê, potem prowadzi nas do jakichœ podziemi w których nic nie ma
// wracamy do kapitana który nam to zleci³
// ka¿e nam przynieœæ miksturkê oczyszczenia umys³u (mo¿na kupiæ u alchemika za 100 z³ota)
// dajemy j¹ magowie [mówi ¿e sobie wszystko przypomnia³ i nienawidzi piwa]
// informuje nas o tym ¿e jakiœ poszukiwacz przygód pomóg³ jednemu z magów znaleŸæ kule wiêzi potrzebn¹ do budowy golemów
// mówi nam gdzie trzeba iœæ zabiæ maga
// wracamy do kapitana, daje nagrodê
struct Quest_Magowie : public Quest_Dungeon
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void Load(HANDLE file);
};

struct Quest_Magowie2 : public Quest_Dungeon, public UnitEventHandler
{
	int mage_loc, talked; //0-nie powiedzia³, 1-pogada³ w jego wie¿y, 2-pogada³ po wejœciu, 3-pogada³ przed bossem

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
};

//-----------------------------------------------------------------------------
// orkowie napadaj¹ na karawany, trzeba iœæ do podziemi i ich pozabijaæ
// w podziemiach jest zamkniêty pokój który klucz ma najsilniejsza jednostka
// w zamkniêtym pokoju jest ork który siê do nas przy³¹cza [Gorush]
// po jakimœ czasie mówi nam o obozie orków którzy go porwali gdy by³ m³ody
// po zniszczeniu awansuje na szamana/³owce/wojownika
// po jakimœ czasie mówi o swoim klanie który zosta³ podbity przez silnego orka
// trzeba iœæ i ich pobiæ a wtedy ork zostaje nowym wodzem, mo¿na tam spotkaæ orkowego kowala który sprzedaje orkowe przedmioty
struct Quest_Orkowie : public Quest_Dungeon, public LocationEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

	int dungeon_levels, levels_cleared;
};

struct Quest_Orkowie2 : public Quest_Dungeon, public LocationEventHandler, public UnitEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	bool IfQuestEvent();
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

	void ChangeClass(int klasa);

	int near_loc, talked; //0-nie powiedzia³, 1-powiedzia³ w obozie, 2-powiedzia³ w podziemiach, 3-powiedzia³ o bosie
};

//-----------------------------------------------------------------------------
// quest gobliny
// "Odzyskaj antyczny ³uk" - jest w lesie, szlachcic uciek³ przed wilkami
// po zdobyciu go na drodze napadaj¹ gobliny i go kradn¹
// wraca siê do szlachcica i on to komentuje
// po jakimœ czasie przychodzi pos³aniec i mówi w których podziemiach s¹ gobliny
// jest tam ³uk, zanosimy go a szlachcic mówi ¿e to nie ten ale kupuje go za 500 sz
// potem spotykamy wêdrownego maga i pyta czy mamy ³uk, bo jest na nim jakaœ iluzja
// gdy wracamy to szlachcica nie ma, mo¿na zapytaæ karczmarza
// mówi ¿e poszed³ œmiej¹c siê z naiwnego gracza, do swojego fortu
// w forcie s¹ gobliny i na koñcu szlachcic i dwóch stra¿ników
// zabija siê go i koniec
struct Quest_Gobliny : public Quest_Dungeon, public UnitEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}

	int enc;
};

//-----------------------------------------------------------------------------
// zleca kap³an w losowym miejscu
// trzeba iœæ do podziemi zabiæ nekromantê ale go tam nie ma, jest zakrwawiony o³tarz, gdy siê podejdzie pojawiaj¹ siê nieumarli;
// potem trzeba zameldowaæ o postêpach, wysy³a nas do jakiegoœ maga po ksiêgê, mag mówi ¿e stra¿nicy j¹ zarewirowali, kapitan odsy³a do burmistrza
// potem do kapitana i j¹ daje, wracamy do kap³ana, wysy³a nas do podziemi na koñcu których jest portal do innych podziemi, na koñcu jest boss
struct Quest_Zlo : public Quest_Dungeon, public UnitEventHandler
{
	struct Loc : public Quest_Event
	{
		int near_loc;
		VEC3 pos;
		int state; // 0-brak, 1-powiedzia³ tekst po wejœciu, 2-powiedzia³ tekst po wejœciu na poziom, 3-zamkniêto portal
	};

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	bool IfQuestEvent();
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}

	inline int GetLocId(int location_id)
	{
		for(int i=0; i<3; ++i)
		{
			if(loc[i].target_loc == location_id)
				return i;
		}
		return -1;
	}

	int mage_loc;
	Loc loc[3];
	int closed;
	bool changed, told_about_boss;
};

//-----------------------------------------------------------------------------
struct Quest_Szaleni : public Quest_Dungeon
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
};

//-----------------------------------------------------------------------------
struct Quest_ListGonczy : public Quest_Dungeon, public UnitEventHandler
{
	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	bool IfHaveQuestItem();
	const Item* GetQuestItem()
	{
		return &letter;
	}
	bool IfNeedTalk(cstring topic);

	int level;
	bool crazy;
	Class clas;
	string unit_name;
	OtherItem letter;
	Unit* target_unit;
};
