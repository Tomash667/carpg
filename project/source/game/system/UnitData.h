#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "UnitStats.h"
#include "Blood.h"
#include "StatProfile.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "DamageTypes.h"
#include "ItemScript.h"
#include "FrameInfo.h"
#include "Class.h"
#include "ItemSlot.h"

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

	static vector<SpellList*> lists;
	static SpellList* TryGet(Cstring id);
};

//-----------------------------------------------------------------------------
enum class UNIT_TYPE
{
	ANIMAL,
	HUMANOID,
	HUMAN
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
	F_HUMAN = 1 << 0, // use items, have armor/hair/beard
	F_HUMANOID = 1 << 1, // use items
	F_COWARD = 1 << 2, // escapes when allies die or have small hp amount
	F_DONT_ESCAPE = 1 << 3, // never escapes
	F_ARCHER = 1 << 4, // preferuje walk� broni� dystansow�
	F_LEADER = 1 << 5, // inni go chroni� ?
	F_PIERCE_RES25 = 1 << 6, // odporno�� na k�ute 25%
	F_SLASH_RES25 = 1 << 7, // odporno�� na ci�te 25%
	F_BLUNT_RES25 = 1 << 8, // odporno�� na obuchowe 25%
	F_PIERCE_WEAK25 = 1 << 9, // podatno�� na k�ute 25%
	F_SLASH_WEAK25 = 1 << 10, // podatno�� na ci�te 25%
	F_BLUNT_WEAK25 = 1 << 11, // podatno�� na obuchowe 25%
	F_UNDEAD = 1 << 12, // mo�na o�ywi�
	F_SLOW = 1 << 13, // nie biega
	F_POISON_ATTACK = 1 << 14, // atak zatruwa
	F_IMMORTAL = 1 << 15, // nie mo�na zabi� tej postaci
	F_TOMASHU = 1 << 16, // przy generowaniu postaci jest �ci�le okre�lony kolor i w�osy
	F_CRAZY = 1 << 17, // mo�liwe kolorowe w�osy
	F_DONT_OPEN = 1 << 18, // nie potrafi otwiera� drzwi
	F_SLIGHT = 1 << 19, // nie uruchamia pu�apek
	F_SECRET = 1 << 20, // nie mo�na zespawnowa�
	F_DONT_SUFFER = 1 << 21, // odporno�� na b�l
	F_MAGE = 1 << 22, // pr�buje sta� jak najdalej od przeciwnika
	F_POISON_RES = 1 << 23, // odporno�� na trucizny
	F_GRAY_HAIR = 1 << 24, // dla nieumar�ych i nekromant�w
	F_NO_POWER_ATTACK = 1 << 25, // nie posiada pot�nego ataku
	F_AI_CLERK = 1 << 26, // jak stoi ale ma animacj� przegl�dania dokument�w, chodzi niedaleko
	F_AI_GUARD = 1 << 27, // stoi w miejscu i si� rozgl�da
	F_AI_STAY = 1 << 28, // stoi w miejscu i u�ywa obiekt�w, chodzi niedaleko
	F_AI_WANDERS = 1 << 29, // u�ywa obiekt�w, �azi po ca�ym mie�cie
	F_AI_DRUNKMAN = 1 << 30, // pije piwo o ile jest w budynku
	F_HERO = 1 << 31 // mo�na go do��czy� do dru�yny, ma HeroData
};

//-----------------------------------------------------------------------------
// More unit flags
enum UNIT_FLAGS2
{
	F2_AI_TRAIN = 1 << 0, // trenuje walk� na manekinie/celu strzelniczym
	F2_SPECIFIC_NAME = 1 << 1, // nie generuje imienia
	// unused (1 << 2)
	F2_CONTEST = 1 << 3, // do��cza do zawod�w w piciu
	F2_CONTEST_50 = 1 << 4, // 50% �e do��czy do zawod�w w piciu
	// unused (1 << 5)
	// unused (1 << 6)
	// unused (1 << 7)
	// unused (1 << 8)
	F2_OLD = 1 << 9, // siwe w�osy
	F2_MELEE = 1 << 10, // walczy wr�cz nawet jak ma �uk a wr�g jest daleko
	F2_MELEE_50 = 1 << 11, // walczy wr�cz 50%
	F2_BOSS = 1 << 12, // muzyka bossa
	F2_BLOODLESS = 1 << 13, // nie mo�na rzuci� wyssania hp
	F2_LIMITED_ROT = 1 << 14, // stoi w miar� prosto - karczmarz za lad�
	// unused (1 << 15)
	F2_STUN_RESISTANCE = 1 << 16, // 50% resistance to stuns
	F2_SIT_ON_THRONE = 1 << 17, // siada na tronie
	F2_ORC_SOUNDS = 1 << 18, // d�wi�k gadania
	F2_GOBLIN_SOUNDS = 1 << 19, // d�wi�k gadania
	F2_XAR = 1 << 20, // d�wi�k gadania, stoi przed o�tarzem i si� modli
	F2_GOLEM_SOUNDS = 1 << 21, // d�wi�k gadania
	F2_TOURNAMENT = 1 << 22, // bierze udzia� w zawodach
	F2_YELL = 1 << 23, // okrzyk bojowy nawet gdy kto� inny pierwszy zauwa�y wroga
	F2_BACKSTAB = 1 << 24, // podw�jna premia za cios w plecy
	F2_IGNORE_BLOCK = 1 << 25, // blokowanie mniej daje przeciwko jego atakom
	F2_BACKSTAB_RES = 1 << 26, // 50% odporno�ci na ataki w plecy
	F2_MAGIC_RES50 = 1 << 27, // 50% odporno�ci na magi�
	F2_MAGIC_RES25 = 1 << 28, // 25% odporno�ci na magi�
	F2_MARK = 1 << 29, // rysuje trupa na minimapie
	F2_GUARDED = 1 << 30, // jednostki wygenerowane w tym samym pokoju chroni� go (dzia�a tylko w podziemiach na Event::unit_to_spawn)
	F2_NOT_GOBLIN = 1 << 31, // nie ma tekst�w goblina
};

//-----------------------------------------------------------------------------
// Even more unit flags...
enum UNIT_FLAGS3
{
	F3_CONTEST_25 = 1 << 0, // 25% szansy �e we�mie udzia� w zawodach w piciu
	F3_DRUNK_MAGE = 1 << 1, // bierze udzia� w zawodach w piciu o ile jest pijakiem, bierze udzia� w walce na arenie o ile nie jest, pije w karczmie i po do��czeniu
	F3_DRUNKMAN_AFTER_CONTEST = 1 << 2, // jak pijak ale po zawodach
	F3_DONT_EAT = 1 << 3, // nie je bo nie mo�e albo jest w pracy
	F3_ORC_FOOD = 1 << 4, // je orkowe jedzenie a nie normalne
	F3_MINER = 1 << 5, // 50% szansy �e zajmie si� wydobywaniem
	F3_TALK_AT_COMPETITION = 1 << 6, // nie gada o pierdo�ach na zawodach
	F3_PARENT_DATA = 1 << 7, // unit data is inherited
};

//-----------------------------------------------------------------------------
// Id d�wi�ku
enum SOUND_ID
{
	SOUND_SEE_ENEMY,
	SOUND_PAIN,
	SOUND_DEATH,
	SOUND_ATTACK,
	SOUND_TALK,
	SOUND_MAX
};

//-----------------------------------------------------------------------------
// D�wi�ki postaci
struct SoundPack
{
	string id;
	vector<SoundPtr> sounds[SOUND_MAX];
	bool inited;

	SoundPack() : inited(false) {}
	bool Have(SOUND_ID sound_id) const { return !sounds[sound_id].empty(); }
	SoundPtr Random(SOUND_ID sound_id) const
	{
		auto& e = sounds[sound_id];
		if(e.empty())
			return nullptr;
		else
			return e[Rand() % e.size()];
	}

	static vector<SoundPack*> packs;
	static SoundPack* TryGet(Cstring id);
};

//-----------------------------------------------------------------------------
struct IdlePack
{
	string id;
	vector<string> anims;

	static vector<IdlePack*> packs;
	static IdlePack* TryGet(Cstring id);
};

//-----------------------------------------------------------------------------
struct TexPack
{
	string id;
	vector<TexId> textures;
	bool inited;

	TexPack() : inited(false) {}

	static vector<TexPack*> packs;
	static TexPack* TryGet(Cstring id);
};

//-----------------------------------------------------------------------------
struct TraderInfo
{
	Stock* stock;
	int buy_flags, buy_consumable_flags;
	vector<const Item*> includes;

	TraderInfo() : stock(nullptr), buy_flags(0), buy_consumable_flags(0) {}
	bool CanBuySell(const Item* item);
};

//-----------------------------------------------------------------------------
// Dane postaci
struct UnitData
{
	string id, mesh_id, name;
	MeshPtr mesh;
	MATERIAL_TYPE mat;
	Int2 level;
	StatProfile* stat_profile;
	int hp_bonus, stamina_bonus, def_bonus, dmg_type, flags, flags2, flags3;
	SpellList* spells;
	Int2 gold, gold2;
	GameDialog* dialog;
	UNIT_GROUP group;
	float walk_speed, run_speed, rot_speed, width, attack_range;
	BLOOD blood;
	SoundPack* sounds;
	FrameInfo* frames;
	TexPack* tex;
	IdlePack* idles;
	ArmorUnitType armor_type;
	ItemScript* item_script;
	UNIT_TYPE type;
	ResourceState state;
	Class clas;
	TraderInfo* trader;

	UnitData() : mesh(nullptr), mat(MAT_BODY), level(0), stat_profile(nullptr), hp_bonus(100), stamina_bonus(0), def_bonus(0), dmg_type(DMG_BLUNT), flags(0),
		flags2(0), flags3(0), spells(nullptr), gold(0), gold2(0), dialog(nullptr), group(G_CITIZENS), walk_speed(1.5f), run_speed(5.f), rot_speed(3.f),
		width(0.3f), attack_range(1.f), blood(BLOOD_RED), sounds(nullptr), frames(nullptr), tex(nullptr), armor_type(ArmorUnitType::NONE),
		item_script(nullptr), idles(nullptr), type(UNIT_TYPE::HUMAN), state(ResourceState::NotLoaded), clas(Class::INVALID), trader(nullptr)
	{
	}
	~UnitData()
	{
		delete trader;
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

	static SetContainer<UnitData> units;
	static std::map<string, UnitData*> aliases;
	static UnitData* TryGet(Cstring id);
	static UnitData* Get(Cstring id);
	static void Validate(uint& err);
};
