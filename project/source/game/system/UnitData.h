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
// Lista zaklêæ postaci
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
	F_ARCHER = 1 << 4, // prefer ranged combat
	F_PEACEFUL = 1 << 5, // if have dont_attack flag ignore actions that cancel it
	F_PIERCE_RES25 = 1 << 6, // pierce damage resistance 25%
	F_SLASH_RES25 = 1 << 7, // slash damage resistance 25%
	F_BLUNT_RES25 = 1 << 8, // blunt damage resistance 25%
	F_PIERCE_WEAK25 = 1 << 9, // pierce damage weakness 25%
	F_SLASH_WEAK25 = 1 << 10, // slash damage weakness 25%
	F_BLUNT_WEAK25 = 1 << 11, // blunt damage weakness 25%
	F_UNDEAD = 1 << 12, // can be raised as undead, can't use potions
	F_SLOW = 1 << 13, // don't run
	F_POISON_ATTACK = 1 << 14, // attack apply poison
	F_IMMORTAL = 1 << 15, // immortal, can't have less then 1 hp
	F_TOMASHU = 1 << 16, // fixed Tomashu appearance
	F_CRAZY = 1 << 17, // random hair color, different dialogs
	F_DONT_OPEN = 1 << 18, // can't open doors
	F_SLIGHT = 1 << 19, // don't trigger traps
	F_SECRET = 1 << 20, // can't be spawned with command
	F_DONT_SUFFER = 1 << 21, // no suffer animation when taking damage
	F_MAGE = 1 << 22, // stay away from enemies, use mage weapon/armor
	F_POISON_RES = 1 << 23, // immune to poisons
	F_GRAY_HAIR = 1 << 24, // have gray/white/black hair
	F_NO_POWER_ATTACK = 1 << 25, // don't have power attack
	F_AI_CLERK = 1 << 26, // reading documents animation when using chair
	F_AI_GUARD = 1 << 27, // stays in place
	F_AI_STAY = 1 << 28, // stays near starting pos, can use objects
	F_AI_WANDERS = 1 << 29, // wanders around city
	F_AI_DRUNKMAN = 1 << 30, // drink beer when inside inn
	F_HERO = 1 << 31 // can join team, have name & class (sometimes used for non-hero units that reveal their real name)
};

//-----------------------------------------------------------------------------
// More unit flags
enum UNIT_FLAGS2
{
	F2_AI_TRAIN = 1 << 0, // when near manequin/arrow target trains
	F2_SPECIFIC_NAME = 1 << 1, // don't have random name
	F2_FIXED_STATS = 1 << 2, // don't calculate unit stats from skills/attributes, use fixed value
	F2_CONTEST = 1 << 3, // joins drinking contest
	F2_CONTEST_50 = 1 << 4, // 50% to join drinking contest
	F2_DONT_TALK = 1 << 5, // no idle talk
	// unused (1 << 6)
	// unused (1 << 7)
	// unused (1 << 8)
	F2_OLD = 1 << 9, // have old gray hair
	F2_MELEE = 1 << 10, // prefers melee combat
	F2_MELEE_50 = 1 << 11, // 50% prefers melee combat (randomly selected when spawned)
	F2_BOSS = 1 << 12, // when player is on same level, play boss music
	F2_BLOODLESS = 1 << 13, // no blood pool on death, can't cast drain hp
	F2_LIMITED_ROT = 1 << 14, // tries to keep starting rotation - innkeeper
	// unused (1 << 15)
	F2_STUN_RESISTANCE = 1 << 16, // 50% resistance to stuns
	F2_SIT_ON_THRONE = 1 << 17, // ai can sit on throne
	// unused (1 << 18)
	// unused (1 << 19)
	F2_XAR = 1 << 20, // pray in front of altar
	// unused (1 << 21)
	F2_TOURNAMENT = 1 << 22, // can join tournament
	F2_YELL = 1 << 23, // battle yell even when someone did that first
	F2_BACKSTAB = 1 << 24, // double backstab attack bonus
	F2_IGNORE_BLOCK = 1 << 25, // blocking is less effective against this enemy
	F2_BACKSTAB_RES = 1 << 26, // 50% backstab resistance
	F2_MAGIC_RES50 = 1 << 27, // 50% magic resistance
	F2_MAGIC_RES25 = 1 << 28, // 25% magic resistance
	// unused (1 << 29)
	F2_GUARDED = 1 << 30, // units spawned in same room protect and follow their leader (only works in dungeon Event::unit_to_spawn)
	F2_NOT_GOBLIN = 1 << 31, // unit is in goblin group but don't use goblin idle texts
};

//-----------------------------------------------------------------------------
// Even more unit flags...
enum UNIT_FLAGS3
{
	F3_CONTEST_25 = 1 << 0, // 25% to join drinking contest
	F3_DRUNK_MAGE = 1 << 1, // joins drinking contest when drunk, join tournament when healed, drink randomly until healed
	F3_DRUNKMAN_AFTER_CONTEST = 1 << 2, // drink beer when inside inn but only after contest is finished
	F3_DONT_EAT = 1 << 3, // don't randomly eat because can't or is in work (like guards)
	F3_ORC_FOOD = 1 << 4, // when randomly eating use food from orc list
	F3_MINER = 1 << 5, // 50% chance to start mining when idle
	F3_TALK_AT_COMPETITION = 1 << 6, // don't talk randomly when in competition (innkeeper/arena_master)
	F3_PARENT_DATA = 1 << 7, // unit data is inherited
};

//-----------------------------------------------------------------------------
// Id dŸwiêku
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
// DŸwiêki postaci
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
	string id, mesh_id, name, real_name;
	UnitData* parent;
	MeshPtr mesh;
	MATERIAL_TYPE mat;
	Int2 level;
	StatProfile* stat_profile;
	int hp, hp_lvl, stamina, attack, attack_lvl, def, def_lvl, dmg_type, flags, flags2, flags3;
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
	vector<UnitData*>* upgrade;

	UnitData() : mesh(nullptr), mat(MAT_BODY), level(0), stat_profile(nullptr), hp(0), hp_lvl(0), stamina(0), attack(0), attack_lvl(0), def(0), def_lvl(0),
		dmg_type(DMG_BLUNT), flags(0), flags2(0), flags3(0), spells(nullptr), gold(0), gold2(0), dialog(nullptr), group(G_CITIZENS), walk_speed(1.5f),
		run_speed(5.f), rot_speed(3.f), width(0.3f), attack_range(1.f), blood(BLOOD_RED), sounds(nullptr), frames(nullptr), tex(nullptr),
		armor_type(ArmorUnitType::NONE), item_script(nullptr), idles(nullptr), type(UNIT_TYPE::HUMAN), state(ResourceState::NotLoaded), clas(Class::INVALID),
		trader(nullptr), upgrade(nullptr), parent(nullptr)
	{
	}
	~UnitData()
	{
		delete trader;
		delete upgrade;
	}

	float GetRadius() const { return width; }
	StatProfile& GetStatProfile() const { return *stat_profile; }
	const TexId* GetTextureOverride() const
	{
		if(!tex)
			return nullptr;
		else
			return tex->textures.data();
	}
	UnitStats* GetStats(int level);
	UnitStats* GetStats(SubprofileInfo sub);
	void CopyFrom(UnitData& ud);
	int GetLevelDif(int level) const;

	static SetContainer<UnitData> units;
	static std::map<string, UnitData*> aliases;
	static UnitData* TryGet(Cstring id);
	static UnitData* Get(Cstring id);
	static UnitData* GetS(const string& id) { return Get(id); }
	static void Validate(uint& err);
};
