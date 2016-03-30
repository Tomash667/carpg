#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "UnitStats.h"
#include "Blood.h"
#include "StatProfile.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "DamageTypes.h"

//-----------------------------------------------------------------------------
struct Spell;
struct DialogEntry;

//-----------------------------------------------------------------------------
struct ItemScript
{
	string id;
	vector<int> code;
};

//-----------------------------------------------------------------------------
// Lista zaklêæ postaci
struct SpellList
{
	string id;
	int level[3];
	cstring name[3];
	Spell* spell[3];
	bool have_non_combat;

	SpellList() : spell(), name(), level(), have_non_combat(false) {}
	SpellList(int _l1, cstring _n1, int _l2, cstring _n2, int _l3, cstring _n3, bool _have_non_combat) : spell(), have_non_combat(_have_non_combat)
	{
		level[0] = _l1;
		level[1] = _l2;
		level[2] = _l3;
		name[0] = _n1;
		name[1] = _n2;
		name[2] = _n3;
	}

	inline bool operator != (const SpellList& l) const
	{
		if(have_non_combat != l.have_non_combat)
			return true;
		for(int i = 0; i < 3; ++i)
		{
			if(spell[i] != l.spell[i] || level[i] != l.level[i])
				return true;
		}
		return false;
	}
};

//-----------------------------------------------------------------------------
// Unit groups
enum UNIT_GROUP
{
	G_PLAYER,
	G_TEAM,
	G_CITIZENS,
	G_GOBLINS,
	G_ORCS,
	G_UNDEAD,
	G_MAGES,
	G_ANIMALS,
	G_CRAZIES,
	G_BANDITS
};

//-----------------------------------------------------------------------------
// Unit flags
enum UNIT_FLAGS
{
	F_HUMAN = 1<<0, // u¿ywa przedmiotów, widaæ zbrojê, ma brodê i w³osy
	F_HUMANOID = 1<<1, // u¿ywa przedmiotów
	F_COWARD = 1<<2, // ucieka gdy ma ma³o hp albo ktoœ zginie
	F_DONT_ESCAPE = 1<<3, // nigdy nie ucieka
	F_ARCHER = 1<<4, // preferuje walkê broni¹ dystansow¹
	F_LEADER = 1<<5, // inni go chroni¹ ?
	F_PIERCE_RES25 = 1<<6, // odpornoœæ na k³ute 25%
	F_SLASH_RES25 = 1<<7, // odpornoœæ na ciête 25%
	F_BLUNT_RES25 = 1<<8, // odpornoœæ na obuchowe 25%
	F_PIERCE_WEAK25 = 1<<9, // podatnoœæ na k³ute 25%
	F_SLASH_WEAK25 = 1<<10, // podatnoœæ na ciête 25%
	F_BLUNT_WEAK25 = 1<<11, // podatnoœæ na obuchowe 25%
	F_UNDEAD = 1<<12, // mo¿na o¿ywiæ
	F_SLOW = 1<<13, // nie biega
	F_POISON_ATTACK = 1<<14, // atak zatruwa
	F_IMMORTAL = 1<<15, // nie mo¿na zabiæ tej postaci
	F_TOMASHU = 1<<16, // przy generowaniu postaci jest œciœle okreœlony kolor i w³osy
	F_CRAZY = 1<<17, // mo¿liwe kolorowe w³osy
	F_DONT_OPEN = 1<<18, // nie potrafi otwieraæ drzwi
	F_SLIGHT = 1<<19, // nie uruchamia pu³apek
	F_SECRET = 1<<20, // nie mo¿na zespawnowaæ
	F_DONT_SUFFER = 1<<21, // odpornoœæ na ból
	F_MAGE = 1<<22, // próbuje staæ jak najdalej od przeciwnika
	F_POISON_RES = 1<<23, // odpornoœæ na trucizny
	F_GRAY_HAIR = 1<<24, // dla nieumar³ych i nekromantów
	F_NO_POWER_ATTACK = 1<<25, // nie posiada potê¿nego ataku
	F_AI_CLERK = 1<<26, // jak stoi ale ma animacjê przegl¹dania dokumentów, chodzi niedaleko
	F_AI_GUARD = 1<<27, // stoi w miejscu i siê rozgl¹da
	F_AI_STAY = 1<<28, // stoi w miejscu i u¿ywa obiektów, chodzi niedaleko
	F_AI_WANDERS = 1<<29, // u¿ywa obiektów, ³azi po ca³ym mieœcie
	F_AI_DRUNKMAN = 1<<30, // pije piwo o ile jest w budynku
	F_HERO = 1<<31 // mo¿na go do³¹czyæ do dru¿yny, ma HeroData
};

//-----------------------------------------------------------------------------
// More unit flags
enum UNIT_FLAGS2
{
	F2_AI_TRAIN = 1<<0, // trenuje walkê na manekinie/celu strzelniczym
	F2_SPECIFIC_NAME = 1<<1, // nie generuje imienia
	F2_NO_CLASS = 1<<2, // ta postaæ tak na prawde nie jest bohaterem ale ma imie/mo¿e pod¹¿aæ za graczem
	F2_CONTEST = 1<<3, // do³¹cza do zawodów w piciu
	F2_CONTEST_50 = 1<<4, // 50% ¿e do³¹czy do zawodów w piciu
	F2_CLASS_FLAG = 1<<5, // ma flagê klasy
	F2_WARRIOR = 1<<6, // okreœlona klasa - wojownik
	F2_HUNTER = 1<<7, // okreœlona klasa - ³owca
	F2_ROGUE = 1<<8, // okreœlona klasa - ³otrzyk
	F2_OLD = 1<<9, // siwe w³osy
	F2_MELEE = 1<<10, // walczy wrêcz nawet jak ma ³uk a wróg jest daleko
	F2_MELEE_50 = 1<<11, // walczy wrêcz 50%
	F2_BOSS = 1<<12, // muzyka bossa
	F2_BLOODLESS = 1<<13, // nie mo¿na rzuciæ wyssania hp
	F2_LIMITED_ROT = 1<<14, // stoi w miarê prosto - karczmarz za lad¹
	F2_CLERIC = 1<<15, // okreœlona klasa - kap³an
	F2_UPDATE_V0_ITEMS = 1<<16, // aktualizuje ekwipunek jeœli zapisano w V0
	F2_SIT_ON_THRONE = 1<<17, // siada na tronie
	F2_ORC_SOUNDS = 1<<18, // dŸwiêk gadania
	F2_GOBLIN_SOUNDS = 1<<19, // dŸwiêk gadania
	F2_XAR = 1<<20, // dŸwiêk gadania, stoi przed o³tarzem i siê modli
	F2_GOLEM_SOUNDS = 1<<21, // dŸwiêk gadania
	F2_TOURNAMENT = 1<<22, // bierze udzia³ w zawodach
	F2_YELL = 1<<23, // okrzyk bojowy nawet gdy ktoœ inny pierwszy zauwa¿y wroga
	F2_BACKSTAB = 1<<24, // podwójna premia za cios w plecy
	F2_IGNORE_BLOCK = 1<<25, // blokowanie mniej daje przeciwko jego atakom
	F2_BACKSTAB_RES = 1<<26, // 50% odpornoœci na ataki w plecy
	F2_MAGIC_RES50 = 1<<27, // 50% odpornoœci na magiê
	F2_MAGIC_RES25 = 1<<28, // 25% odpornoœci na magiê
	F2_MARK = 1<<29, // rysuje trupa na minimapie
	F2_GUARDED = 1<<30, // jednostki wygenerowane w tym samym pokoju chroni¹ go (dzia³a tylko w podziemiach na Event::unit_to_spawn)
	F2_NOT_GOBLIN = 1<<31, // nie ma tekstów goblina
};

//-----------------------------------------------------------------------------
// Even more unit flags...
enum UNIT_FLAGS3
{
	F3_CONTEST_25 = 1<<0, // 25% szansy ¿e weŸmie udzia³ w zawodach w piciu
	F3_DRUNK_MAGE = 1<<1, // bierze udzia³ w zawodach w piciu o ile jest pijakiem, bierze udzia³ w walce na arenie o ile nie jest, pije w karczmie i po do³¹czeniu
	F3_DRUNKMAN_AFTER_CONTEST = 1<<2, // jak pijak ale po zawodach
	F3_DONT_EAT = 1<<3, // nie je bo nie mo¿e albo jest w pracy
	F3_ORC_FOOD = 1<<4, // je orkowe jedzenie a nie normalne
	F3_MINER = 1<<5, // 50% szansy ¿e zajmie siê wydobywaniem
	F3_TALK_AT_COMPETITION = 1<<6, // nie gada o pierdo³ach na zawodach
};

//-----------------------------------------------------------------------------
// Id dŸwiêku
enum SOUND_ID
{
	SOUND_SEE_ENEMY,
	SOUND_PAIN,
	SOUND_DEATH,
	SOUND_ATTACK,
	SOUND_MAX
};

//-----------------------------------------------------------------------------
// DŸwiêki postaci
struct SoundPack
{
	string id;
	string filename[SOUND_MAX];
	SOUND sound[SOUND_MAX];
	bool inited;

	SoundPack() : inited(false) {}
	SoundPack(cstring see_enemy, cstring pain, cstring death, cstring attack) : inited(false)
	{
		if(see_enemy)
			filename[SOUND_SEE_ENEMY] = see_enemy;
		if(pain)
			filename[SOUND_PAIN] = pain;
		if(death)
			filename[SOUND_DEATH] = death;
		if(attack)
			filename[SOUND_ATTACK] = attack;
		for(int i=0; i<SOUND_MAX; ++i)
			sound[i] = nullptr;
	}

	inline bool operator != (const SoundPack& s) const
	{
		for(int i = 0; i < SOUND_MAX; ++i)
		{
			if(filename[i] != s.filename[i])
				return true;
		}
		return false;
	}
};

//-----------------------------------------------------------------------------
// Typy ramek
enum FRAME_INDEX
{
	F_CAST,
	F_TAKE_WEAPON,
	F_ATTACK1_START,
	F_ATTACK1_END,
	F_ATTACK2_START,
	F_ATTACK2_END,
	F_ATTACK3_START,
	F_ATTACK3_END,
	F_BASH,
	F_MAX
};

//-----------------------------------------------------------------------------
// Flags for which weapon use which attack animation
#define A_SHORT_BLADE (1<<0)
#define A_LONG_BLADE (1<<1)
#define A_BLUNT (1<<2)
#define A_AXE (1<<3)

//-----------------------------------------------------------------------------
// Informacje o animacjach ataku postaci
struct AttackFrameInfo
{
	struct Entry
	{
		float start, end;
		int flags;

		inline float lerp() const
		{
			return ::lerp(start, end, 2.f/3);
		}
	};
	vector<Entry> e;
};

//-----------------------------------------------------------------------------
// Informacje o ramce animacji
struct FrameInfo
{
	string id;
	AttackFrameInfo* extra;
	float t[F_MAX];
	int attacks;

	FrameInfo() : extra(nullptr), attacks(0), t() {}
	~FrameInfo()
	{
		delete extra;
	}

	inline float lerp(int frame) const
	{
		return ::lerp(t[frame], t[frame+1], 2.f/3);
	}

	inline bool operator != (const FrameInfo& f) const
	{
		if(!extra != !f.extra)
			return true;
		if(attacks != f.attacks)
			return true;
		if(extra)
		{
			for(int i = 0; i < attacks; ++i)
			{
				if(extra->e[i].flags != f.extra->e[i].flags ||
					abs(extra->e[i].start - f.extra->e[i].start) > 0.1f ||
					abs(extra->e[i].end - f.extra->e[i].end) > 0.1f)
					return true;
			}
		}
		for(int i = 0; i < F_MAX; ++i)
		{
			if(abs(t[i] - f.t[i]) > 0.1f)
				return true;
		}
		return false;
	}
};

//-----------------------------------------------------------------------------
struct IdlePack
{
	string id;
	vector<string> anims;
};

//-----------------------------------------------------------------------------
struct TexPack
{
	string id;
	vector<TexId> textures;
	bool inited;

	TexPack() : inited(false) {}
};

//-----------------------------------------------------------------------------
// Dane postaci
struct UnitData
{
	string id, mesh_id, name;
	Animesh* mesh;
	MATERIAL_TYPE mat;
	INT2 level;
	StatProfile* stat_profile;
	int hp_bonus, def_bonus, dmg_type, flags, flags2, flags3;
	const int* items;
	SpellList* spells;
	INT2 gold, gold2;
	DialogEntry* dialog;
	UNIT_GROUP group;
	float walk_speed, run_speed, rot_speed, width, attack_range;
	BLOOD blood;
	SoundPack* sounds;
	FrameInfo* frames;
	TexPack* tex;
	vector<string>* idles;
	ArmorUnitType armor_type;
	ItemScript* item_script;
	bool new_items, new_dialog;

	UnitData() : mesh(nullptr), mat(MAT_BODY), level(0), stat_profile(nullptr), hp_bonus(100), def_bonus(0),
		dmg_type(DMG_BLUNT), flags(0), flags2(0), flags3(0), items(nullptr), spells(nullptr), gold(0), gold2(0), dialog(nullptr), group(G_CITIZENS),
		walk_speed(1.5f), run_speed(5.f), rot_speed(3.f), width(0.3f), attack_range(1.f), blood(BLOOD_RED), sounds(nullptr), frames(nullptr), tex(nullptr),
		armor_type(ArmorUnitType::NONE), item_script(nullptr), idles(nullptr), new_items(false), new_dialog(false)
	{
	}

	inline float GetRadius() const
	{
		return width;
	}

	inline StatProfile& GetStatProfile() const
	{
		return *stat_profile;
	}
	
	inline const TexId* GetTextureOverride() const
	{
		if(!tex)
			return nullptr;
		else
			return tex->textures.data();
	}

	void CopyFrom(UnitData& ud);

	inline static bool SortById(const UnitData* unit1, const UnitData* unit2)
	{
		return unit1->id < unit2->id;
	}

	inline static bool SortByName(const UnitData* unit1, const UnitData* unit2)
	{
		return strcoll(unit1->name.c_str(), unit2->name.c_str()) < 0;
	}
};

//-----------------------------------------------------------------------------
struct UnitDataComparer
{
	inline bool operator() (const UnitData* ud1, const UnitData* ud2)
	{
		return _stricmp(ud1->id.c_str(), ud2->id.c_str()) > 0;
	}
};
typedef std::set<UnitData*, UnitDataComparer> UnitDataContainer;
typedef UnitDataContainer::iterator UnitDataIterator;
extern UnitDataContainer unit_datas;
extern std::map<string, UnitData*> unit_aliases;

//-----------------------------------------------------------------------------
struct UnitGroup
{
	struct Entry
	{
		UnitData* ud;
		int count;

		inline Entry() {}
		inline Entry(UnitData* ud, int count) : ud(ud), count(count) {}
	};

	string id;
	vector<Entry> entries;
	UnitData* leader;
	int total;
};
extern vector<UnitGroup*> unit_groups;
inline UnitGroup* FindUnitGroup(AnyString id)
{
	for(UnitGroup* group : unit_groups)
	{
		if(group->id == id.s)
			return group;
	}
	return nullptr;
}

struct TmpUnitGroup
{
	UnitGroup* group;
	vector<UnitGroup::Entry> entries;
	int total, max_level;
};

//-----------------------------------------------------------------------------
UnitData* FindUnitData(cstring id, bool report = true);
void LoadUnits(uint& crc);
void CleanupUnits();
void TestItemScript(const int* script, string& errors, uint& count, bool is_new, uint& crc);
void LogItemScript(const int* script, bool is_new);
