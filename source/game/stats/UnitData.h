#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "UnitStats.h"
#include "Blood.h"
#include "StatProfile.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "DamageTypes.h"
#include "TypeItem.h"

//-----------------------------------------------------------------------------
struct Spell;
struct DialogEntry;
struct GameDialog;
class GameTypeManager;

//-----------------------------------------------------------------------------
struct ItemScript
{
	string id;
	vector<int> code;
};

//-----------------------------------------------------------------------------
// Lista zakl�� postaci
struct SpellList
{
	string id;
	int level[3];
	cstring name[3];
	Spell* spell[3];
	bool have_non_combat;

	SpellList() : spell(), name(), level(), have_non_combat(false) {}
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
	F_HUMAN = 1<<0, // u�ywa przedmiot�w, wida� zbroj�, ma brod� i w�osy
	F_HUMANOID = 1<<1, // u�ywa przedmiot�w
	F_COWARD = 1<<2, // ucieka gdy ma ma�o hp albo kto� zginie
	F_DONT_ESCAPE = 1<<3, // nigdy nie ucieka
	F_ARCHER = 1<<4, // preferuje walk� broni� dystansow�
	F_LEADER = 1<<5, // inni go chroni� ?
	F_PIERCE_RES25 = 1<<6, // odporno�� na k�ute 25%
	F_SLASH_RES25 = 1<<7, // odporno�� na ci�te 25%
	F_BLUNT_RES25 = 1<<8, // odporno�� na obuchowe 25%
	F_PIERCE_WEAK25 = 1<<9, // podatno�� na k�ute 25%
	F_SLASH_WEAK25 = 1<<10, // podatno�� na ci�te 25%
	F_BLUNT_WEAK25 = 1<<11, // podatno�� na obuchowe 25%
	F_UNDEAD = 1<<12, // mo�na o�ywi�
	F_SLOW = 1<<13, // nie biega
	F_POISON_ATTACK = 1<<14, // atak zatruwa
	F_IMMORTAL = 1<<15, // nie mo�na zabi� tej postaci
	F_TOMASHU = 1<<16, // przy generowaniu postaci jest �ci�le okre�lony kolor i w�osy
	F_CRAZY = 1<<17, // mo�liwe kolorowe w�osy
	F_DONT_OPEN = 1<<18, // nie potrafi otwiera� drzwi
	F_SLIGHT = 1<<19, // nie uruchamia pu�apek
	F_SECRET = 1<<20, // nie mo�na zespawnowa�
	F_DONT_SUFFER = 1<<21, // odporno�� na b�l
	F_MAGE = 1<<22, // pr�buje sta� jak najdalej od przeciwnika
	F_POISON_RES = 1<<23, // odporno�� na trucizny
	F_GRAY_HAIR = 1<<24, // dla nieumar�ych i nekromant�w
	F_NO_POWER_ATTACK = 1<<25, // nie posiada pot�nego ataku
	F_AI_CLERK = 1<<26, // jak stoi ale ma animacj� przegl�dania dokument�w, chodzi niedaleko
	F_AI_GUARD = 1<<27, // stoi w miejscu i si� rozgl�da
	F_AI_STAY = 1<<28, // stoi w miejscu i u�ywa obiekt�w, chodzi niedaleko
	F_AI_WANDERS = 1<<29, // u�ywa obiekt�w, �azi po ca�ym mie�cie
	F_AI_DRUNKMAN = 1<<30, // pije piwo o ile jest w budynku
	F_HERO = 1<<31 // mo�na go do��czy� do dru�yny, ma HeroData
};

//-----------------------------------------------------------------------------
// More unit flags
enum UNIT_FLAGS2
{
	F2_AI_TRAIN = 1<<0, // trenuje walk� na manekinie/celu strzelniczym
	F2_SPECIFIC_NAME = 1<<1, // nie generuje imienia
	F2_NO_CLASS = 1<<2, // ta posta� tak na prawde nie jest bohaterem ale ma imie/mo�e pod��a� za graczem
	F2_CONTEST = 1<<3, // do��cza do zawod�w w piciu
	F2_CONTEST_50 = 1<<4, // 50% �e do��czy do zawod�w w piciu
	F2_CLASS_FLAG = 1<<5, // ma flag� klasy
	F2_WARRIOR = 1<<6, // okre�lona klasa - wojownik
	F2_HUNTER = 1<<7, // okre�lona klasa - �owca
	F2_ROGUE = 1<<8, // okre�lona klasa - �otrzyk
	F2_OLD = 1<<9, // siwe w�osy
	F2_MELEE = 1<<10, // walczy wr�cz nawet jak ma �uk a wr�g jest daleko
	F2_MELEE_50 = 1<<11, // walczy wr�cz 50%
	F2_BOSS = 1<<12, // muzyka bossa
	F2_BLOODLESS = 1<<13, // nie mo�na rzuci� wyssania hp
	F2_LIMITED_ROT = 1<<14, // stoi w miar� prosto - karczmarz za lad�
	F2_CLERIC = 1<<15, // okre�lona klasa - kap�an
	//F2_UPDATE_V0_ITEMS = 1<<16, // aktualizuje ekwipunek je�li zapisano w V0, flag removed
	F2_SIT_ON_THRONE = 1<<17, // siada na tronie
	F2_ORC_SOUNDS = 1<<18, // d�wi�k gadania
	F2_GOBLIN_SOUNDS = 1<<19, // d�wi�k gadania
	F2_XAR = 1<<20, // d�wi�k gadania, stoi przed o�tarzem i si� modli
	F2_GOLEM_SOUNDS = 1<<21, // d�wi�k gadania
	F2_TOURNAMENT = 1<<22, // bierze udzia� w zawodach
	F2_YELL = 1<<23, // okrzyk bojowy nawet gdy kto� inny pierwszy zauwa�y wroga
	F2_BACKSTAB = 1<<24, // podw�jna premia za cios w plecy
	F2_IGNORE_BLOCK = 1<<25, // blokowanie mniej daje przeciwko jego atakom
	F2_BACKSTAB_RES = 1<<26, // 50% odporno�ci na ataki w plecy
	F2_MAGIC_RES50 = 1<<27, // 50% odporno�ci na magi�
	F2_MAGIC_RES25 = 1<<28, // 25% odporno�ci na magi�
	F2_MARK = 1<<29, // rysuje trupa na minimapie
	F2_GUARDED = 1<<30, // jednostki wygenerowane w tym samym pokoju chroni� go (dzia�a tylko w podziemiach na Event::unit_to_spawn)
	F2_NOT_GOBLIN = 1<<31, // nie ma tekst�w goblina
};

//-----------------------------------------------------------------------------
// Even more unit flags...
enum UNIT_FLAGS3
{
	F3_CONTEST_25 = 1<<0, // 25% szansy �e we�mie udzia� w zawodach w piciu
	F3_DRUNK_MAGE = 1<<1, // bierze udzia� w zawodach w piciu o ile jest pijakiem, bierze udzia� w walce na arenie o ile nie jest, pije w karczmie i po do��czeniu
	F3_DRUNKMAN_AFTER_CONTEST = 1<<2, // jak pijak ale po zawodach
	F3_DONT_EAT = 1<<3, // nie je bo nie mo�e albo jest w pracy
	F3_ORC_FOOD = 1<<4, // je orkowe jedzenie a nie normalne
	F3_MINER = 1<<5, // 50% szansy �e zajmie si� wydobywaniem
	F3_TALK_AT_COMPETITION = 1<<6, // nie gada o pierdo�ach na zawodach
};

//-----------------------------------------------------------------------------
// Id d�wi�ku
enum SOUND_ID
{
	SOUND_SEE_ENEMY,
	SOUND_PAIN,
	SOUND_DEATH,
	SOUND_ATTACK,
	SOUND_MAX
};

//-----------------------------------------------------------------------------
// D�wi�ki postaci
struct SoundPack
{
	string id;
	string filename[SOUND_MAX];
	SOUND sound[SOUND_MAX];
	bool inited;

	SoundPack() : inited(false), sound() {}
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

		float lerp() const
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

	float lerp(int frame) const
	{
		return ::lerp(t[frame], t[frame+1], 2.f/3);
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
struct UnitData : public TypeItem
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
	GameDialog* dialog;
	UNIT_GROUP group;
	float walk_speed, run_speed, rot_speed, width, attack_range;
	BLOOD blood;
	SoundPack* sounds;
	FrameInfo* frames;
	TexPack* tex;
	vector<string>* idles;
	ArmorUnitType armor_type;
	ItemScript* item_script;

	UnitData() : mesh(nullptr), mat(MAT_BODY), level(0), stat_profile(nullptr), hp_bonus(100), def_bonus(0),
		dmg_type(DMG_BLUNT), flags(0), flags2(0), flags3(0), items(nullptr), spells(nullptr), gold(0), gold2(0), dialog(nullptr), group(G_CITIZENS),
		walk_speed(1.5f), run_speed(5.f), rot_speed(3.f), width(0.3f), attack_range(1.f), blood(BLOOD_RED), sounds(nullptr), frames(nullptr), tex(nullptr),
		armor_type(ArmorUnitType::NONE), item_script(nullptr), idles(nullptr)
	{
	}

	float GetRadius() const
	{
		return width;
	}

	StatProfile& GetStatProfile() const
	{
		return *stat_profile;
	}
	
	const TexId* GetTextureOverride() const
	{
		if(!tex)
			return nullptr;
		else
			return tex->textures.data();
	}

	void CopyFrom(UnitData& ud);

	static void Register(GameTypeManager& gt_mgr);
};

//-----------------------------------------------------------------------------
struct UnitDataComparer
{
	bool operator() (const UnitData* ud1, const UnitData* ud2) const
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

		Entry() {}
		Entry(UnitData* ud, int count) : ud(ud), count(count) {}
	};

	string id;
	vector<Entry> entries;
	UnitData* leader;
	int total;
};
extern vector<UnitGroup*> unit_groups;

struct TmpUnitGroup
{
	UnitGroup* group;
	vector<UnitGroup::Entry> entries;
	int total, max_level;
};

//-----------------------------------------------------------------------------
UnitData* FindUnitData(cstring id, bool report = true);
UnitGroup* FindUnitGroup(const AnyString& id);
uint LoadUnits(uint& crc, uint& errors);
void CleanupUnits();
void TestItemScript(const int* script, string& errors, uint& count, uint& crc);
