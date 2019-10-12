#include "Pch.h"
#include "GameCore.h"
#include "UnitLoader.h"
#include "UnitData.h"
#include "UnitGroup.h"
#include "GameDialog.h"
#include "ItemScript.h"
#include "Item.h"
#include "Spell.h"
#include "ResourceManager.h"
#include "Stock.h"
#include "Content.h"
#include "QuestScheme.h"
#include "GameResources.h"
#include <Mesh.h>

enum Group
{
	G_TYPE,
	G_PROPERTY,
	G_MATERIAL,
	G_FLAGS,
	G_FLAGS2,
	G_FLAGS3,
	G_GROUP,
	G_DMG_TYPE,
	G_BLOOD,
	G_ARMOR_TYPE,
	G_ATTRIBUTE,
	G_SKILL,
	G_SOUND_TYPE,
	G_FRAME_KEYWORD,
	G_WEAPON_FLAG,
	G_NULL,
	G_SPELL_KEYWORD,
	G_ITEM_KEYWORD,
	G_GROUP_KEYWORD,
	G_TRADER_KEYWORD,
	G_ITEM_GROUP,
	G_CONSUMABLE_GROUP,
	G_SUBPROFILE_GROUP,
	G_TAG,
	G_GROUP_OPTIONS
};

enum UnitDataType
{
	T_UNIT,
	T_PROFILE,
	T_ITEMS,
	T_SPELLS,
	T_SOUNDS,
	T_FRAMES,
	T_TEX,
	T_IDLES,
	T_ALIAS,
	T_GROUP
};

enum Property
{
	P_MESH,
	P_MAT,
	P_LEVEL,
	P_PROFILE,
	P_FLAGS,
	P_HP,
	P_STAMINA,
	P_ATTACK,
	P_DEF,
	P_ITEMS,
	P_SPELLS,
	P_GOLD,
	P_DIALOG,
	P_GROUP,
	P_DMG_TYPE,
	P_WALK_SPEED,
	P_RUN_SPEED,
	P_ROT_SPEED,
	P_BLOOD,
	P_BLOOD_SIZE,
	P_SOUNDS,
	P_FRAMES,
	P_TEX,
	P_IDLES,
	P_WIDTH,
	P_ATTACK_RANGE,
	P_ARMOR_TYPE,
	P_CLASS,
	P_TRADER,
	P_UPGRADE
};

enum FrameKeyword
{
	FK_ATTACKS,
	FK_SIMPLE_ATTACKS,
	FK_CAST,
	FK_TAKE_WEAPON,
	FK_BASH
};

enum ItemKeyword
{
	IK_CHANCE,
	IK_RANDOM,
	IK_IF,
	IK_ELSE,
	IK_LEVEL
};

enum SpellKeyword
{
	SK_NON_COMBAT,
	SK_NULL
};

enum GroupKeyword
{
	GK_LEADER,
	GK_GROUP,
	GK_FOOD_MOD,
	GK_ORC_FOOD,
	GK_HAVE_CAMPS,
	GK_ENCOUNTER_CHANCE,
	GK_SPECIAL,
	GK_LIST,
	GK_GENDER
};

enum TraderKeyword
{
	TK_STOCK,
	TK_GROUPS,
	TK_INCLUDE
};

enum IfState
{
	IFS_START,
	IFS_START_INLINE,
	IFS_ELSE,
	IFS_ELSE_INLINE
};

enum SubprofileKeyword
{
	SPK_WEAPON,
	SPK_BOW,
	SPK_SHIELD,
	SPK_ARMOR,
	SPK_AMULET,
	SPK_RING,
	SPK_TAG,
	SPK_PRIORITY,
	SPK_PERK,
	SPK_ITEMS,
	SPK_TAGS
};

//=================================================================================================
void UnitLoader::DoLoading()
{
	Load("units.txt", G_TYPE);
}

//=================================================================================================
void UnitLoader::Cleanup()
{
	DeleteElements(StatProfile::profiles);
	DeleteElements(ItemScript::scripts);
	DeleteElements(SpellList::lists);
	DeleteElements(SoundPack::packs);
	DeleteElements(IdlePack::packs);
	DeleteElements(TexPack::packs);
	DeleteElements(FrameInfo::frames);
	DeleteElements(UnitData::units);
	DeleteElements(UnitGroup::groups);
	DeleteElements(UnitStats::shared_stats);
}

//=================================================================================================
void UnitLoader::InitTokenizer()
{
	t.SetFlags(Tokenizer::F_UNESCAPE | Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS);

	t.AddKeywords(G_TYPE, {
		{ "unit", T_UNIT },
		{ "items", T_ITEMS },
		{ "profile", T_PROFILE },
		{ "spells", T_SPELLS },
		{ "sounds", T_SOUNDS },
		{ "frames", T_FRAMES },
		{ "tex", T_TEX },
		{ "idles", T_IDLES },
		{ "alias", T_ALIAS },
		{ "group", T_GROUP }
		});

	t.AddKeywords(G_PROPERTY, {
		{ "mesh", P_MESH },
		{ "mat", P_MAT },
		{ "level", P_LEVEL },
		{ "profile", P_PROFILE },
		{ "flags", P_FLAGS },
		{ "hp", P_HP },
		{ "stamina", P_STAMINA },
		{ "attack", P_ATTACK },
		{ "def", P_DEF },
		{ "items", P_ITEMS },
		{ "spells", P_SPELLS },
		{ "gold", P_GOLD },
		{ "dialog", P_DIALOG },
		{ "group", P_GROUP },
		{ "dmg_type", P_DMG_TYPE },
		{ "walk_speed", P_WALK_SPEED },
		{ "run_speed", P_RUN_SPEED },
		{ "rot_speed", P_ROT_SPEED },
		{ "blood", P_BLOOD },
		{ "blood_size", P_BLOOD_SIZE },
		{ "sounds", P_SOUNDS },
		{ "frames", P_FRAMES },
		{ "tex", P_TEX },
		{ "idles", P_IDLES },
		{ "width", P_WIDTH },
		{ "attack_range", P_ATTACK_RANGE },
		{ "armor_type", P_ARMOR_TYPE },
		{ "class", P_CLASS },
		{ "trader", P_TRADER },
		{ "upgrade", P_UPGRADE }
		});

	t.AddKeywords(G_MATERIAL, {
		{ "wood", MAT_WOOD },
		{ "skin", MAT_SKIN },
		{ "metal", MAT_IRON },
		{ "crystal", MAT_CRYSTAL },
		{ "cloth", MAT_CLOTH },
		{ "rock", MAT_ROCK },
		{ "body", MAT_BODY },
		{ "bone", MAT_BONE }
		});

	t.AddKeywords(G_FLAGS, {
		{ "human", F_HUMAN },
		{ "humanoid", F_HUMANOID },
		{ "coward", F_COWARD },
		{ "dont_escape", F_DONT_ESCAPE },
		{ "archer", F_ARCHER },
		{ "peaceful", F_PEACEFUL },
		{ "pierce_res25", F_PIERCE_RES25 },
		{ "slash_res25", F_SLASH_RES25 },
		{ "blunt_res25", F_BLUNT_RES25 },
		{ "pierce_weak25", F_PIERCE_WEAK25 },
		{ "slash_weak25", F_SLASH_WEAK25 },
		{ "blunt_weak25", F_BLUNT_WEAK25 },
		{ "undead", F_UNDEAD },
		{ "slow", F_SLOW },
		{ "poison_attack", F_POISON_ATTACK },
		{ "immortal", F_IMMORTAL },
		{ "tomashu", F_TOMASHU },
		{ "crazy", F_CRAZY },
		{ "dont_open", F_DONT_OPEN },
		{ "slight", F_SLIGHT },
		{ "secret", F_SECRET },
		{ "dont_suffer", F_DONT_SUFFER },
		{ "mage", F_MAGE },
		{ "poison_res", F_POISON_RES },
		{ "gray_hair", F_GRAY_HAIR },
		{ "no_power_attack", F_NO_POWER_ATTACK },
		{ "ai_clerk", F_AI_CLERK },
		{ "ai_guard", F_AI_GUARD },
		{ "ai_stay", F_AI_STAY },
		{ "ai_wanders", F_AI_WANDERS },
		{ "ai_drunkman", F_AI_DRUNKMAN },
		{ "hero", F_HERO }
		});

	t.AddKeywords(G_FLAGS2, {
		{ "ai_train", F2_AI_TRAIN },
		{ "specific_name", F2_SPECIFIC_NAME },
		{ "fixed_stats", F2_FIXED_STATS },
		{ "contest", F2_CONTEST },
		{ "contest_50", F2_CONTEST_50 },
		{ "dont_talk", F2_DONT_TALK },
		{ "construct", F2_CONSTRUCT },
		{ "fast_learner", F2_FAST_LEARNER },
		{ "old", F2_OLD },
		{ "melee", F2_MELEE },
		{ "melee_50", F2_MELEE_50 },
		{ "boss", F2_BOSS },
		{ "bloodless", F2_BLOODLESS },
		{ "limited_rot", F2_LIMITED_ROT },
		{ "stun_res", F2_STUN_RESISTANCE },
		{ "sit_on_throne", F2_SIT_ON_THRONE },
		{ "xar", F2_XAR },
		{ "tournament", F2_TOURNAMENT },
		{ "yell", F2_YELL },
		{ "backstab", F2_BACKSTAB },
		{ "ignore_block", F2_IGNORE_BLOCK },
		{ "backstab_res", F2_BACKSTAB_RES },
		{ "magic_res50", F2_MAGIC_RES50 },
		{ "magic_res25", F2_MAGIC_RES25 },
		{ "guarded", F2_GUARDED },
		{ "not_goblin", F2_NOT_GOBLIN }
		});

	t.AddKeywords(G_FLAGS3, {
		{ "contest_25", F3_CONTEST_25 },
		{ "drunk_mage", F3_DRUNK_MAGE },
		{ "drunkman_after_contest", F3_DRUNKMAN_AFTER_CONTEST },
		{ "dont_eat", F3_DONT_EAT },
		{ "orc_food", F3_ORC_FOOD },
		{ "miner", F3_MINER },
		{ "talk_at_competition", F3_TALK_AT_COMPETITION }
		});

	t.AddKeywords(G_GROUP, {
		{ "player", G_PLAYER },
		{ "team", G_TEAM },
		{ "citizens", G_CITIZENS },
		{ "goblins", G_GOBLINS },
		{ "orcs", G_ORCS },
		{ "undead", G_UNDEAD },
		{ "mages", G_MAGES },
		{ "animals", G_ANIMALS },
		{ "crazies", G_CRAZIES },
		{ "bandits", G_BANDITS }
		});

	t.AddKeywords(G_DMG_TYPE, {
		{ "slash", DMG_SLASH },
		{ "pierce", DMG_PIERCE },
		{ "blunt", DMG_BLUNT }
		});

	t.AddKeywords(G_BLOOD, {
		{ "red", BLOOD_RED },
		{ "green", BLOOD_GREEN },
		{ "black", BLOOD_BLACK },
		{ "bone", BLOOD_BONE },
		{ "rock", BLOOD_ROCK },
		{ "iron", BLOOD_IRON }
		});

	t.AddKeywords(G_ARMOR_TYPE, {
		{ "human", (int)ArmorUnitType::HUMAN },
		{ "goblin", (int)ArmorUnitType::GOBLIN },
		{ "orc", (int)ArmorUnitType::ORC },
		{ "none", (int)ArmorUnitType::NONE }
		});

	t.AddKeywords(G_ATTRIBUTE, {
		{ "strength", (int)AttributeId::STR },
		{ "endurance", (int)AttributeId::END },
		{ "dexterity", (int)AttributeId::DEX },
		{ "inteligence", (int)AttributeId::INT },
		{ "wisdom", (int)AttributeId::WIS },
		{ "charisma", (int)AttributeId::CHA }
		});

	for(uint i = 0; i < (uint)SkillId::MAX; ++i)
		t.AddKeyword(Skill::skills[i].id, i, G_SKILL);

	t.AddKeywords(G_SOUND_TYPE, {
		{ "see_enemy", SOUND_SEE_ENEMY },
		{ "pain", SOUND_PAIN },
		{ "death", SOUND_DEATH },
		{ "attack", SOUND_ATTACK },
		{ "talk", SOUND_TALK }
		});

	t.AddKeywords(G_FRAME_KEYWORD, {
		{ "attacks", FK_ATTACKS },
		{ "simple_attacks", FK_SIMPLE_ATTACKS },
		{ "cast", FK_CAST },
		{ "take_weapon", FK_TAKE_WEAPON },
		{ "bash", FK_BASH }
		});

	t.AddKeywords(G_WEAPON_FLAG, {
		{ "short_blade", A_SHORT_BLADE },
		{ "long_blade", A_LONG_BLADE },
		{ "blunt", A_BLUNT },
		{ "axe", A_AXE }
		});

	t.AddKeyword("null", 0, G_NULL);

	t.AddKeywords(G_SPELL_KEYWORD, {
		{ "non_combat", SK_NON_COMBAT },
		{ "null", SK_NULL }
		});

	t.AddKeywords(G_ITEM_KEYWORD, {
		{ "chance", IK_CHANCE },
		{ "random", IK_RANDOM },
		{ "if", IK_IF },
		{ "else", IK_ELSE },
		{ "level", IK_LEVEL }
		});

	t.AddKeywords(G_GROUP_KEYWORD, {
		{ "leader", GK_LEADER },
		{ "group", GK_GROUP },
		{ "food_mod", GK_FOOD_MOD },
		{ "orc_food", GK_ORC_FOOD },
		{ "have_camps", GK_HAVE_CAMPS },
		{ "encounter_chance", GK_ENCOUNTER_CHANCE },
		{ "special", GK_SPECIAL },
		{ "list", GK_LIST },
		{ "gender", GK_GENDER }
		});

	t.AddKeywords(G_TRADER_KEYWORD, {
		{ "stock", TK_STOCK },
		{ "groups", TK_GROUPS },
		{ "include", TK_INCLUDE }
		});

	t.AddKeywords(G_ITEM_GROUP, {
		{ "weapon", IT_WEAPON },
		{ "bow", IT_BOW },
		{ "shield", IT_SHIELD },
		{ "armor", IT_ARMOR },
		{ "amulet", IT_AMULET },
		{ "ring", IT_RING },
		{ "other", IT_OTHER },
		{ "consumable", IT_CONSUMABLE },
		{ "book", IT_BOOK }
		});

	t.AddEnums<ConsumableType>(G_CONSUMABLE_GROUP, {
		{ "food", ConsumableType::Food },
		{ "drink", ConsumableType::Drink },
		{ "potion", ConsumableType::Potion },
		{ "herb", ConsumableType::Herb }
		});

	t.AddKeywords(G_SUBPROFILE_GROUP, {
		{ "weapon", SPK_WEAPON },
		{ "bow", SPK_BOW },
		{ "shield", SPK_SHIELD },
		{ "armor", SPK_ARMOR },
		{ "amulet", SPK_AMULET },
		{ "ring", SPK_RING },
		{ "tag", SPK_TAG },
		{ "priority", SPK_PRIORITY },
		{ "perk", SPK_PERK },
		{ "items", SPK_ITEMS },
		{ "tags", SPK_TAGS }
		});

	t.AddKeywords(G_TAG, {
		{ "str", TAG_STR },
		{ "dex", TAG_DEX },
		{ "melee", TAG_MELEE },
		{ "ranged", TAG_RANGED },
		{ "def", TAG_DEF },
		{ "stamina", TAG_STAMINA },
		{ "mage", TAG_MAGE },
		{ "mana", TAG_MANA },
		{ "cleric", TAG_CLERIC }
		});
}

//=================================================================================================
void UnitLoader::LoadEntity(int top, const string& id)
{
	crc.Update(id);

	switch(top)
	{
	case T_UNIT:
		ParseUnit(id);
		break;
	case T_PROFILE:
		{
			if(StatProfile::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<StatProfile> profile;
			profile->id = id;
			t.Next();
			ParseProfile(profile);
		}
		break;
	case T_ITEMS:
		{
			if(ItemScript::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<ItemScript> script;
			script->is_subprofile = false;
			script->id = id;
			t.Next();
			ParseItems(script);
		}
		break;
	case T_SPELLS:
		{
			if(SpellList::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<SpellList> list;
			list->id = id;
			t.Next();
			ParseSpells(list);
		}
		break;
	case T_SOUNDS:
		{
			if(SoundPack::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<SoundPack> pack;
			pack->id = id;
			t.Next();
			ParseSounds(pack);
		}
		break;
	case T_FRAMES:
		{
			if(FrameInfo::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<FrameInfo> frames;
			frames->id = id;
			t.Next();
			ParseFrames(frames);
		}
		break;
	case T_TEX:
		{
			if(TexPack::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<TexPack> pack;
			pack->id = id;
			t.Next();
			ParseTextures(pack);
		}
		break;
	case T_IDLES:
		{
			if(IdlePack::TryGet(id))
				t.Throw("Id must be unique.");
			Ptr<IdlePack> pack;
			pack->id = id;
			t.Next();
			ParseIdles(pack);
		}
		break;
	case T_ALIAS:
		ParseAlias(id);
		break;
	case T_GROUP:
		ParseGroup(id);
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void UnitLoader::Finalize()
{
	UnitGroup::empty = UnitGroup::Get("empty");
	UnitGroup::random = UnitGroup::Get("random");

	uint crc_value = crc.Get();
	content.crc[(int)Content::Id::Units] = crc_value;
	Info("Loaded units (%u) - crc %p.", UnitData::units.size(), crc_value);
}

//=================================================================================================
void UnitLoader::ParseUnit(const string& id)
{
	if(UnitData::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<UnitData> unit;

	// id
	unit->id = id;
	t.Next();

	// parent unit
	if(t.IsSymbol(':'))
	{
		t.Next();
		auto& parent_id = t.MustGetItemKeyword();
		auto parent = UnitData::TryGet(parent_id);
		if(!parent)
			t.Throw("Missing parent unit '%s'.", parent_id.c_str());
		parent->flags3 |= F3_PARENT_DATA;
		unit->CopyFrom(*parent);
		crc.Update(parent_id);
		t.Next();
	}

	// {
	t.AssertSymbol('{');
	t.Next();

	// properties
	while(!t.IsSymbol('}'))
	{
		Property p = (Property)t.MustGetKeywordId(G_PROPERTY);
		t.Next();

		switch(p)
		{
		case P_MESH:
			{
				const string& mesh_id = t.MustGetString();
				unit->mesh = res_mgr->TryGet<Mesh>(mesh_id);
				if(unit->mesh)
					crc.Update(mesh_id);
				else
					LoadError("Missing mesh '%s'.", mesh_id.c_str());
			}
			break;
		case P_MAT:
			unit->mat = (MATERIAL_TYPE)t.MustGetKeywordId(G_MATERIAL);
			crc.Update(unit->mat);
			break;
		case P_LEVEL:
			if(t.IsSymbol('{'))
			{
				t.Next();
				unit->level.x = t.MustGetInt();
				t.Next();
				unit->level.y = t.MustGetInt();
				if(unit->level.x >= unit->level.y || unit->level.x < 0)
					t.Throw("Invalid level {%d %d}.", unit->level.x, unit->level.y);
				t.Next();
				t.AssertSymbol('}');
			}
			else
			{
				int lvl = t.MustGetInt();
				if(lvl < 0)
					t.Throw("Invalid level %d.", lvl);
				unit->level = Int2(lvl);
			}
			crc.Update(unit->level);
			break;
		case P_PROFILE:
			if(t.IsSymbol('{'))
			{
				Ptr<StatProfile> profile;
				unit->stat_profile = profile.Get();
				ParseProfile(profile);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->stat_profile = StatProfile::TryGet(id);
				if(!unit->stat_profile)
					t.Throw("Missing stat profile '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_FLAGS:
			{
				t.ParseFlags({
					{ &unit->flags, G_FLAGS },
					{ &unit->flags2, G_FLAGS2 },
					{ &unit->flags3, G_FLAGS3 }
					});
				crc.Update(unit->flags);
				crc.Update(unit->flags2);
				crc.Update(unit->flags3);
			}
			break;
		case P_HP:
			unit->hp = t.MustGetInt();
			crc.Update(unit->hp);
			if(t.QuerySymbol('+'))
			{
				t.Next();
				t.AssertSymbol('+');
				t.Next();
				unit->hp_lvl = t.MustGetInt();
				crc.Update(unit->hp_lvl);
			}
			break;
		case P_STAMINA:
			unit->stamina = t.MustGetInt();
			crc.Update(unit->stamina);
			break;
		case P_ATTACK:
			unit->attack = t.MustGetInt();
			crc.Update(unit->attack);
			if(t.QuerySymbol('+'))
			{
				t.Next();
				t.AssertSymbol('+');
				t.Next();
				unit->attack_lvl = t.MustGetInt();
				crc.Update(unit->attack_lvl);
			}
			break;
		case P_DEF:
			unit->def = t.MustGetInt();
			crc.Update(unit->def);
			if(t.QuerySymbol('+'))
			{
				t.Next();
				t.AssertSymbol('+');
				t.Next();
				unit->def_lvl = t.MustGetInt();
				crc.Update(unit->def_lvl);
			}
			break;
		case P_ITEMS:
			if(t.IsSymbol('{'))
			{
				Ptr<ItemScript> script;
				script->is_subprofile = false;
				unit->item_script = script.Get();
				ParseItems(script);
			}
			else
			{
				unit->item_script = nullptr;
				if(!t.IsKeywordGroup(G_NULL))
				{
					const string& id = t.MustGetItemKeyword();
					unit->item_script = ItemScript::TryGet(id);
					if(!unit->item_script)
						t.Throw("Missing item script '%s'.", id.c_str());
					crc.Update(id);
				}
				else
					crc.Update0();
			}
			break;
		case P_SPELLS:
			if(t.IsSymbol('{'))
			{
				Ptr<SpellList> spells;
				unit->spells = spells.Get();
				ParseSpells(spells);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->spells = SpellList::TryGet(id);
				if(!unit->spells)
					t.Throw("Missing spell list '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_GOLD:
			if(t.IsSymbol('{'))
			{
				t.Next();
				if(t.IsSymbol('{'))
				{
					t.Next();
					unit->gold.x = t.MustGetInt();
					t.Next();
					unit->gold.y = t.MustGetInt();
					t.Next();
					t.AssertSymbol('}');
					t.Next();
					t.AssertSymbol('{');
					t.Next();
					unit->gold2.x = t.MustGetInt();
					t.Next();
					unit->gold2.y = t.MustGetInt();
					t.Next();
					t.AssertSymbol('}');
					t.Next();
					t.AssertSymbol('}');
					if(unit->gold.x >= unit->gold.y || unit->gold.x < 0 || unit->gold2.x >= unit->gold2.y || unit->gold2.x < unit->gold.x || unit->gold2.y < unit->gold.y)
						t.Throw("Invalid gold count {{%d %d} {%d %d}}.", unit->gold.x, unit->gold.y, unit->gold2.x, unit->gold2.y);
				}
				else
				{
					unit->gold.x = t.MustGetInt();
					t.Next();
					unit->gold.y = t.MustGetInt();
					t.Next();
					t.AssertSymbol('}');
					if(unit->gold.x >= unit->gold.y || unit->gold.x < 0)
						t.Throw("Invalid gold count {%d %d}.", unit->gold.x, unit->gold.y);
					unit->gold2 = unit->gold;
				}
			}
			else
			{
				int gold = t.MustGetInt();
				if(gold < 0)
					t.Throw("Invalid gold count %d.", gold);
				unit->gold = unit->gold2 = Int2(gold);
			}
			crc.Update(unit->gold);
			crc.Update(unit->gold2);
			break;
		case P_DIALOG:
			{
				const string& id = t.MustGetText();
				if(id.find_first_of('/') != string::npos)
				{
					dialog_requests.push_back({ unit->id, id });
					crc.Update(id);
				}
				else
				{
					unit->dialog = GameDialog::TryGet(id.c_str());
					if(!unit->dialog)
						t.Throw("Missing dialog '%s'.", id.c_str());
					crc.Update(id);
				}
			}
			break;
		case P_GROUP:
			unit->group = (UNIT_GROUP)t.MustGetKeywordId(G_GROUP);
			crc.Update(unit->group);
			break;
		case P_DMG_TYPE:
			t.ParseFlags(G_DMG_TYPE, unit->dmg_type);
			crc.Update(unit->dmg_type);
			break;
		case P_WALK_SPEED:
			unit->walk_speed = t.MustGetFloat();
			if(unit->walk_speed < 0.5f)
				t.Throw("Invalid walk speed %g.", unit->walk_speed);
			crc.Update(unit->walk_speed);
			break;
		case P_RUN_SPEED:
			unit->run_speed = t.MustGetFloat();
			if(unit->run_speed < 0)
				t.Throw("Invalid run speed %g.", unit->run_speed);
			crc.Update(unit->run_speed);
			break;
		case P_ROT_SPEED:
			unit->rot_speed = t.MustGetFloat();
			if(unit->rot_speed < 0.5f)
				t.Throw("Invalid rot speed %g.", unit->rot_speed);
			crc.Update(unit->rot_speed);
			break;
		case P_BLOOD:
			unit->blood = (BLOOD)t.MustGetKeywordId(G_BLOOD);
			crc.Update(unit->blood);
			break;
		case P_BLOOD_SIZE:
			unit->blood_size = t.MustGetFloat();
			crc.Update(unit->blood_size);
			break;
		case P_SOUNDS:
			if(t.IsSymbol('{'))
			{
				Ptr<SoundPack> sounds;
				unit->sounds = sounds.Get();
				ParseSounds(sounds);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->sounds = SoundPack::TryGet(id);
				if(!unit->sounds)
					t.Throw("Missing sound pack '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_FRAMES:
			if(t.IsSymbol('{'))
			{
				Ptr<FrameInfo> frames;
				unit->frames = frames.Get();
				ParseFrames(frames);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->frames = FrameInfo::TryGet(id);
				if(!unit->frames)
					t.Throw("Missing frame infos '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_TEX:
			if(t.IsSymbol('{'))
			{
				Ptr<TexPack> tex;
				unit->tex = tex.Get();
				ParseTextures(tex);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->tex = TexPack::TryGet(id);
				if(!unit->tex)
					t.Throw("Missing tex pack '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_IDLES:
			if(t.IsSymbol('{'))
			{
				Ptr<IdlePack> idles;
				unit->idles = idles.Get();
				ParseIdles(idles);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				unit->idles = IdlePack::TryGet(id);
				if(!unit->idles)
					t.Throw("Missing idles pack '%s'.", id.c_str());
				crc.Update(id);
			}
			break;
		case P_WIDTH:
			unit->width = t.MustGetFloat();
			if(unit->width < 0.1f)
				t.Throw("Invalid width %g.", unit->width);
			crc.Update(unit->width);
			break;
		case P_ATTACK_RANGE:
			unit->attack_range = t.MustGetFloat();
			if(unit->attack_range < 0.1f)
				t.Throw("Invalid attack range %g.", unit->attack_range);
			crc.Update(unit->attack_range);
			break;
		case P_ARMOR_TYPE:
			unit->armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_TYPE);
			crc.Update(unit->armor_type);
			break;
		case P_CLASS:
			{
				const string& id = t.MustGetItemKeyword();
				unit->clas = Class::TryGet(id);
				if(!unit->clas)
					t.Throw("Invalid unit class '%s'.", id.c_str());
				crc.Update(unit->clas->id);
			}
			break;
		case P_TRADER:
			if(unit->trader)
				t.Throw("Unit is already marked as trader.");
			unit->trader = new TraderInfo;
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				int k = t.MustGetKeywordId(G_TRADER_KEYWORD);
				t.Next();
				switch(k)
				{
				case TK_STOCK:
					{
						if(unit->trader->stock)
							t.Throw("Unit trader stock already set.");
						const string& stock_id = t.MustGetItem();
						unit->trader->stock = Stock::TryGet(stock_id);
						if(!unit->trader->stock)
							LoadError("Missing trader stock '%s'.", stock_id.c_str());
					}
					break;
				case TK_GROUPS:
					t.AssertSymbol('{');
					t.Next();
					while(!t.IsSymbol('}'))
					{
						t.AssertKeywordGroup({ G_ITEM_GROUP, G_CONSUMABLE_GROUP });
						if(t.IsKeywordGroup(G_ITEM_GROUP))
						{
							int group = t.GetKeywordId(G_ITEM_GROUP);
							unit->trader->buy_flags |= (1 << group);
							if(group == IT_CONSUMABLE)
								unit->trader->buy_consumable_flags = 0xFF;
						}
						else
						{
							int group = t.GetKeywordId(G_CONSUMABLE_GROUP);
							unit->trader->buy_flags |= (1 << IT_CONSUMABLE);
							unit->trader->buy_consumable_flags |= (1 << group);
						}
						t.Next();
					}
					break;
				case TK_INCLUDE:
					t.AssertSymbol('{');
					t.Next();
					while(!t.IsSymbol('}'))
					{
						const string& item_id = t.MustGetItem();
						const Item* item = Item::TryGet(item_id);
						if(!item)
							LoadError("Missing trader item include '%s'.", item_id.c_str());
						else
							unit->trader->includes.push_back(item);
						t.Next();
					}
					break;
				}
				t.Next();
			}
			if(!unit->trader->stock)
				LoadError("Unit trader stock not set.");
			break;
		case P_UPGRADE:
			{
				const string& id = t.MustGetText();
				UnitData* u = UnitData::TryGet(id);
				if(!u)
					t.Throw("Missing unit '%s'.", id.c_str());
				if(!u->upgrade)
					u->upgrade = new vector<UnitData*>;
				u->upgrade->push_back(unit.Get());
			}
			break;
		default:
			t.Unexpected();
			break;
		}

		t.Next();
	}

	// configure
	if(IsSet(unit->flags, F_HUMAN))
		unit->type = UNIT_TYPE::HUMAN;
	else if(IsSet(unit->flags, F_HUMANOID))
		unit->type = UNIT_TYPE::HUMANOID;
	else
		unit->type = UNIT_TYPE::ANIMAL;

	if(!unit->mesh)
	{
		if(unit->type == UNIT_TYPE::HUMAN)
			unit->mesh = game_res->aHuman;
		else
			t.Throw("Unit without mesh.");
	}
	else
	{
		if(unit->type == UNIT_TYPE::HUMAN && unit->mesh != game_res->aHuman)
			t.Throw("Human unit with custom mesh.");
	}

	UnitData::units.insert(unit.Pin());
}

//=================================================================================================
void UnitLoader::ParseProfile(Ptr<StatProfile>& profile)
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		profile->attrib[i] = 10;
	for(int i = 0; i < (int)SkillId::MAX; ++i)
		profile->skill[i] = -1;

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		if(t.IsItem("subprofile"))
		{
			Ptr<StatProfile::Subprofile> subprofile;
			ParseSubprofile(subprofile);
			if(profile->GetSubprofile(subprofile->id))
				t.Throw("Subprofile '%s.%s' already exists.", profile->id.c_str(), subprofile->id.c_str());
			profile->subprofiles.push_back(subprofile.Pin());
		}
		else if(t.IsKeywordGroup(G_ATTRIBUTE))
		{
			int a = t.MustGetKeywordId(G_ATTRIBUTE);
			t.Next();
			int val = t.MustGetInt();
			if(val < 1)
				t.Throw("Invalid attribute '%s' value %d.", Attribute::attributes[a].id, val);
			else if(val % 5 != 0)
				t.Throw("Attribute '%s' value %d must be divisible by 5.", Attribute::attributes[a].id, val);
			profile->attrib[a] = val;
			crc.Update(1);
			crc.Update(a);
			crc.Update(val);
		}
		else if(t.IsKeywordGroup(G_SKILL))
		{
			int s = t.MustGetKeywordId(G_SKILL);
			t.Next();
			int val = t.MustGetInt();
			if(val < -1)
				t.Throw("Invalid skill '%s' value %d.", Skill::skills[s].id, val);
			else if(val % 5 != 0)
				t.Throw("Skill '%s' value %d must be divisible by 5.", Skill::skills[s].id, val);
			profile->skill[s] = val;
			crc.Update(2);
			crc.Update(s);
			crc.Update(val);
		}
		else
		{
			cstring a = "subprofile";
			int b = G_ATTRIBUTE, c = G_SKILL;
			t.StartUnexpected()
				.Add(tokenizer::T_ITEM, (int*)&a)
				.Add(tokenizer::T_KEYWORD_GROUP, &b)
				.Add(tokenizer::T_KEYWORD_GROUP, &c)
				.Throw();
		}

		t.Next();
	}

	StatProfile::profiles.push_back(profile.Pin());
}

//=================================================================================================
void UnitLoader::ParseSubprofile(Ptr<StatProfile::Subprofile>& subprofile)
{
	t.Next();
	subprofile->id = t.GetText();
	t.Next();
	t.AssertSymbol('{');
	t.Next();
	while(!t.IsSymbol('}'))
	{
		SubprofileKeyword key = (SubprofileKeyword)t.MustGetKeywordId(G_SUBPROFILE_GROUP);
		t.Next();
		switch(key)
		{
		case SPK_WEAPON:
			if(subprofile->weapon_total > 0)
				t.Throw("Weapon subprofile already set.");
			if(t.IsSymbol('{'))
			{
				t.Next();
				while(!t.IsSymbol('}'))
				{
					WEAPON_TYPE type = GetWeaponType();
					if(subprofile->weapon_chance[type] != 0)
						t.Throw("Weapon subprofile chance already set.");
					t.Next();
					int chance = t.MustGetInt();
					subprofile->weapon_chance[type] = chance;
					subprofile->weapon_total += chance;
					t.Next();
				}
			}
			else
			{
				WEAPON_TYPE type = GetWeaponType();
				subprofile->weapon_chance[type] = 1;
				subprofile->weapon_total = 1;
			}
			break;
		case SPK_ARMOR:
			if(subprofile->armor_total > 0)
				t.Throw("Armor subprofile already set.");
			if(t.IsSymbol('{'))
			{
				t.Next();
				while(!t.IsSymbol('}'))
				{
					ARMOR_TYPE type = GetArmorType();
					if(subprofile->armor_chance[type] != 0)
						t.Throw("Armor subprofile chance already set.");
					t.Next();
					int chance = t.MustGetInt();
					subprofile->armor_chance[type] = chance;
					subprofile->armor_total += chance;
					t.Next();
				}
			}
			else
			{
				ARMOR_TYPE type = GetArmorType();
				subprofile->armor_chance[type] = 1;
				subprofile->armor_total = 1;
			}
			break;
		case SPK_TAG:
			{
				int index = 0;
				for(; index < StatProfile::MAX_TAGS; ++index)
				{
					if(subprofile->tag_skills[index] == SkillId::NONE)
						break;
				}
				if(index == StatProfile::MAX_TAGS)
					t.Throw("Max %u tag skills.", StatProfile::MAX_TAGS);
				SkillId skill;
				if(t.IsKeyword(SPK_WEAPON, G_SUBPROFILE_GROUP))
					skill = SkillId::SPECIAL_WEAPON;
				else if(t.IsKeyword(SPK_ARMOR, G_SUBPROFILE_GROUP))
					skill = SkillId::SPECIAL_ARMOR;
				else
					skill = (SkillId)t.MustGetKeywordId(G_SKILL);
				subprofile->tag_skills[index] = skill;
			}
			break;
		case SPK_PRIORITY:
			{
				int set = 0;
				t.AssertSymbol('{');
				t.Next();
				while(!t.IsSymbol('}'))
				{
					SubprofileKeyword k = (SubprofileKeyword)t.MustGetKeywordId(G_SUBPROFILE_GROUP);
					ITEM_TYPE type;
					switch(k)
					{
					case SPK_WEAPON:
						type = IT_WEAPON;
						break;
					case SPK_SHIELD:
						type = IT_SHIELD;
						break;
					case SPK_BOW:
						type = IT_BOW;
						break;
					case SPK_ARMOR:
						type = IT_ARMOR;
						break;
					case SPK_AMULET:
						type = IT_AMULET;
						break;
					case SPK_RING:
						type = IT_RING;
						break;
					default:
						t.Unexpected();
						break;
					}
					if(IsSet(set, 1 << type))
						t.Throw("Subprofile priority already set.");
					set |= (1 << type);
					t.Next();
					float value = t.MustGetFloat();
					if(value < 0.f)
						t.Throw("Invalid subprofile priority %g.", value);
					subprofile->priorities[type] = value;
					t.Next();
				}
			}
			break;
		case SPK_PERK:
			{
				int index = 0;
				for(; index < StatProfile::MAX_PERKS; ++index)
				{
					if(subprofile->perks[index].perk == Perk::None)
						break;
				}
				if(index == StatProfile::MAX_PERKS)
					t.Throw("Max %u perks.", StatProfile::MAX_PERKS);
				const string& id = t.MustGetText();
				PerkInfo* info = PerkInfo::Find(id);
				if(!info)
					t.Throw("Missing perk '%s'.", id.c_str());
				subprofile->perks[index].perk = info->perk_id;
				int value = -1;
				if(info->value_type != PerkInfo::None)
				{
					t.Next();
					if(info->value_type == PerkInfo::Attribute)
						value = t.MustGetKeywordId(G_ATTRIBUTE);
					else if(info->value_type == PerkInfo::Skill)
					{
						if(t.IsKeyword(SPK_WEAPON, G_SUBPROFILE_GROUP))
							value = (int)SkillId::SPECIAL_WEAPON;
						else if(t.IsKeyword(SPK_ARMOR, G_SUBPROFILE_GROUP))
							value = (int)SkillId::SPECIAL_ARMOR;
						else
							value = t.MustGetKeywordId(G_SKILL);
					}
				}
				subprofile->perks[index].value = value;
			}
			break;
		case SPK_ITEMS:
			{
				t.AssertSymbol('{');
				Ptr<ItemScript> script;
				script->is_subprofile = true;
				subprofile->item_script = script.Get();
				ParseItems(script);
			}
			break;
		case SPK_TAGS:
			{
				int set = 0;
				t.AssertSymbol('{');
				t.Next();
				while(!t.IsSymbol('}'))
				{
					ItemTag tag = (ItemTag)t.MustGetKeywordId(G_TAG);
					if(IsSet(set, 1 << tag))
						t.Throw("Subprofile tag priority already set.");
					set |= (1 << tag);
					t.Next();
					float value = t.MustGetFloat();
					if(value < 0.f)
						t.Throw("Invalid subprofile tag priority %g.", value);
					subprofile->tag_priorities[tag] = value;
					t.Next();
				}
			}
			break;
		default:
			t.Unexpected();
			break;
		}
		t.Next();
	}
	if(subprofile->weapon_total == 0)
		t.Throw("Weapon subprofile not set.");
	if(subprofile->armor_total == 0)
		t.Throw("Armor subprofile not set.");
}

//=================================================================================================
WEAPON_TYPE UnitLoader::GetWeaponType()
{
	SkillId skill = (SkillId)t.MustGetKeywordId(G_SKILL);
	Skill& info = Skill::skills[(int)skill];
	if(info.type != SkillType::WEAPON)
		t.Throw("Skill '%s' is not valid weapon skill.", info.id);
	return ::GetWeaponType(skill);
}

//=================================================================================================
ARMOR_TYPE UnitLoader::GetArmorType()
{
	SkillId skill = (SkillId)t.MustGetKeywordId(G_SKILL);
	Skill& info = Skill::skills[(int)skill];
	if(info.type != SkillType::ARMOR)
		t.Throw("Skill '%s' is not valid armor skill.", info.id);
	return ::GetArmorType(skill);
}

//=================================================================================================
void UnitLoader::ParseItems(Ptr<ItemScript>& script)
{
	vector<IfState> if_state;
	bool done_if = false;

	// {
	t.AssertSymbol('{');
	t.Next();

	while(true)
	{
		if(t.IsSymbol('}'))
		{
			while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
			{
				script->code.push_back(PS_END_IF);
				crc.Update(PS_END_IF);
				if_state.pop_back();
			}
			if(if_state.empty())
				break;
			if(if_state.back() == IFS_START)
			{
				t.Next();
				if(t.IsKeyword(IK_ELSE, G_ITEM_KEYWORD))
				{
					script->code.push_back(PS_ELSE);
					crc.Update(PS_ELSE);
					t.Next();
					if(t.IsKeyword(IK_IF, G_ITEM_KEYWORD))
					{
						// else if
						t.Next();
						ItemKeyword iif = (ItemKeyword)t.MustGetKeywordId(G_ITEM_KEYWORD);
						if(iif == IK_LEVEL)
						{
							t.Next();
							int lvl = t.MustGetInt();
							if(lvl < 0)
								t.Throw("Invalid level %d.", lvl);
							script->code.push_back(PS_IF_LEVEL);
							script->code.push_back(lvl);
							crc.Update(PS_IF_LEVEL);
							crc.Update(lvl);
						}
						else if(iif == IK_CHANCE)
						{
							t.Next();
							int chance = t.MustGetInt();
							if(chance <= 0 || chance >= 100)
								t.Throw("Invalid chance %d.", chance);
							script->code.push_back(PS_IF_CHANCE);
							script->code.push_back(chance);
							crc.Update(PS_IF_CHANCE);
							crc.Update(chance);
						}
						else
						{
							int g = G_ITEM_KEYWORD,
								a = IK_CHANCE,
								b = IK_LEVEL;
							t.StartUnexpected().Add(tokenizer::T_KEYWORD, &a, &g).Add(tokenizer::T_KEYWORD, &b, &g).Throw();
						}
						t.Next();
						if_state.back() = IFS_ELSE_INLINE;
						if(t.IsSymbol('{'))
						{
							t.Next();
							if_state.push_back(IFS_START);
						}
						else
							if_state.push_back(IFS_START_INLINE);
					}
					else if(t.IsSymbol('{'))
					{
						// else { ... }
						t.Next();
						if_state.back() = IFS_ELSE;
					}
					else
					{
						// single expression else
						if_state.back() = IFS_ELSE_INLINE;
					}
				}
				else
				{
					// } end of if
					script->code.push_back(PS_END_IF);
					crc.Update(PS_END_IF);
					if_state.pop_back();
					while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
					{
						script->code.push_back(PS_END_IF);
						crc.Update(PS_END_IF);
						if_state.pop_back();
					}
				}
			}
			else if(if_state.back() == IFS_ELSE)
			{
				// } end of else
				script->code.push_back(PS_END_IF);
				crc.Update(PS_END_IF);
				if_state.pop_back();
				t.Next();
				while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
				{
					script->code.push_back(PS_END_IF);
					crc.Update(PS_END_IF);
					if_state.pop_back();
				}
			}
			else
				t.Unexpected();
			continue;
		}
		else if(t.IsInt())
		{
			// give multiple
			int count = t.MustGetInt();
			if(count <= 0)
				t.Throw("Invalid item count %d.", count);
			t.Next();

			// get item
			script->code.push_back(PS_MANY);
			script->code.push_back(count);
			crc.Update(PS_MANY);
			crc.Update(count);
			AddItem(script);
		}
		else if(t.IsKeywordGroup(G_ITEM_KEYWORD))
		{
			ItemKeyword k = (ItemKeyword)t.GetKeywordId(G_ITEM_KEYWORD);
			t.Next();

			switch(k)
			{
			case IK_CHANCE:
				{
					// get chance value
					int chance = t.MustGetInt();
					t.Next();
					if(chance <= 0 || chance >= 100)
						t.Throw("Invalid chance %d.", chance);

					if(t.IsSymbol('{'))
					{
						// two item chance
						script->code.push_back(PS_CHANCE2);
						script->code.push_back(chance);
						crc.Update(PS_CHANCE2);
						crc.Update(chance);
						t.Next();
						AddItem(script);
						t.Next();
						AddItem(script);
						t.Next();
						t.AssertSymbol('}');
					}
					else
					{
						// single item chance
						script->code.push_back(PS_CHANCE);
						script->code.push_back(chance);
						crc.Update(PS_CHANCE);
						crc.Update(chance);
						AddItem(script);
					}
				}
				break;
			case IK_RANDOM:
				if(t.IsSymbol('{'))
				{
					// one of many
					script->code.push_back(PS_ONE_OF_MANY);
					crc.Update(PS_ONE_OF_MANY);
					uint pos = script->code.size();
					int count = 0;
					script->code.push_back(0);
					t.Next();
					do
					{
						AddItem(script);
						t.Next();
						++count;
					}
					while(!t.IsSymbol('}'));

					if(count < 2)
						t.Throw("Invalid one of many count %d.", count);
					script->code[pos] = count;
					crc.Update(count);
				}
				else
				{
					// get count
					int from = t.MustGetInt();
					t.Next();
					int to = t.MustGetInt();
					t.Next();
					if(from < 0 || from > to)
						t.Throw("Invalid Random count (%d %d).", from, to);

					// get item
					script->code.push_back(PS_RANDOM);
					script->code.push_back(from);
					script->code.push_back(to);
					crc.Update(PS_RANDOM);
					crc.Update(from);
					crc.Update(to);
					AddItem(script);
				}
				break;
			case IK_IF:
				{
					ItemKeyword iif = (ItemKeyword)t.MustGetKeywordId(G_ITEM_KEYWORD);
					if(iif == IK_LEVEL)
					{
						t.Next();
						int lvl = t.MustGetInt();
						if(lvl < 0)
							t.Throw("Invalid level %d.", lvl);
						script->code.push_back(PS_IF_LEVEL);
						script->code.push_back(lvl);
						crc.Update(PS_IF_LEVEL);
						crc.Update(lvl);
					}
					else if(iif == IK_CHANCE)
					{
						t.Next();
						int chance = t.MustGetInt();
						if(chance <= 0 || chance >= 100)
							t.Throw("Invalid chance %d.", chance);
						script->code.push_back(PS_IF_CHANCE);
						script->code.push_back(chance);
						crc.Update(PS_IF_CHANCE);
						crc.Update(chance);
					}
					else
					{
						int g = G_ITEM_KEYWORD,
							a = IK_CHANCE,
							b = IK_LEVEL;
						t.StartUnexpected().Add(tokenizer::T_KEYWORD, &a, &g).Add(tokenizer::T_KEYWORD, &b, &g).Throw();
					}
					t.Next();
					if(t.IsSymbol('{'))
					{
						t.Next();
						if_state.push_back(IFS_START);
					}
					else
						if_state.push_back(IFS_START_INLINE);
					done_if = true;
				}
				break;
			default:
				t.Unexpected();
			}
		}
		else if(t.IsKeyword() || t.IsItem() || t.IsSymbol('!') || t.IsSymbol('$'))
		{
			// single item
			script->code.push_back(PS_ONE);
			crc.Update(PS_ONE);
			AddItem(script);
		}
		else
			t.Unexpected();

		if(done_if)
			done_if = false;
		else
		{
			t.Next();
			while(!if_state.empty())
			{
				if(if_state.back() == IFS_START_INLINE)
				{
					if_state.pop_back();
					if(t.IsKeyword(IK_ELSE, G_ITEM_KEYWORD))
					{
						script->code.push_back(PS_ELSE);
						crc.Update(PS_ELSE);
						t.Next();
						if(t.IsSymbol('{'))
						{
							t.Next();
							if_state.push_back(IFS_ELSE);
						}
						else
							if_state.push_back(IFS_ELSE_INLINE);
						break;
					}
					else
					{
						script->code.push_back(PS_END_IF);
						crc.Update(PS_END_IF);
					}
				}
				else if(if_state.back() == IFS_ELSE_INLINE)
				{
					script->code.push_back(PS_END_IF);
					crc.Update(PS_END_IF);
					if_state.pop_back();
				}
				else
					break;
			}
		}
	}

	while(!if_state.empty())
	{
		if(if_state.back() == IFS_START_INLINE || if_state.back() == IFS_ELSE_INLINE)
		{
			script->code.push_back(PS_END_IF);
			crc.Update(PS_END_IF);
			if_state.pop_back();
		}
		else
			t.Throw("Missing closing '}'.");
	}

	script->code.push_back(PS_END);
	crc.Update(PS_END);
	ItemScript::scripts.push_back(script.Pin());
}

//=================================================================================================
void UnitLoader::AddItem(ItemScript* script)
{
	if(!t.IsSymbol("!$"))
	{
		const string& s = t.MustGetItemKeyword();
		const Item* item = Item::TryGet(s.c_str());
		if(item)
		{
			script->code.push_back(PS_ITEM);
			script->code.push_back((int)item);
			crc.Update(PS_ITEM);
			crc.Update(item->id);
		}
		else
			t.Throw("Missing item '%s'.", s.c_str());
	}
	else
	{
		if(t.IsSymbol('$'))
		{
			if(!script->is_subprofile)
				t.Throw("Special item list can only be used in subprofiles.");
			t.Next();
			int mod = 0;
			if(t.IsSymbol('+') || t.IsSymbol('-'))
			{
				bool minus = t.IsSymbol('-');
				char c = t.PeekChar();
				if(c < '1' || c > '9')
					t.Throw("Invalid special list mod '%c'.", c);
				mod = c - '0';
				if(minus)
					mod = -mod;
				t.NextChar();
				t.Next();
			}
			int special;
			const string& text = t.MustGetItemKeyword();
			if(text == "weapon")
				special = SPECIAL_WEAPON;
			else if(text == "armor")
				special = SPECIAL_ARMOR;
			else
				t.Throw("Invalid special item list '%s'.", text.c_str());
			if(mod == 0)
			{
				script->code.push_back(PS_SPECIAL_ITEM);
				script->code.push_back(special);
				crc.Update(PS_SPECIAL_ITEM);
				crc.Update(special);
			}
			else
			{
				script->code.push_back(PS_SPECIAL_ITEM_MOD);
				script->code.push_back(mod);
				script->code.push_back(special);
				crc.Update(PS_SPECIAL_ITEM_MOD);
				crc.Update(mod);
				crc.Update(special);
			}
		}
		else
		{
			t.Next();
			if(t.IsSymbol('+') || t.IsSymbol('-'))
			{
				bool minus = t.IsSymbol('-');
				char c = t.PeekChar();
				if(c < '1' || c > '9')
					t.Throw("Invalid leveled list mod '%c'.", c);
				int mod = c - '0';
				if(minus)
					mod = -mod;
				t.NextChar();
				t.Next();
				const string& s = t.MustGetItemKeyword();
				ItemListResult lis = ItemList::TryGet(s);
				if(!lis.lis)
					t.Throw("Missing item list '%s'.", s.c_str());
				if(!lis.is_leveled)
					t.Throw("Can't use mod on non leveled list '%s'.", s.c_str());
				script->code.push_back(PS_LEVELED_LIST_MOD);
				script->code.push_back(mod);
				script->code.push_back((int)lis.llis);
				crc.Update(PS_LEVELED_LIST_MOD);
				crc.Update(mod);
				crc.Update(lis.llis->id);
			}
			else
			{
				const string& s = t.MustGetItemKeyword();
				ItemListResult lis = ItemList::TryGet(s);
				if(!lis.lis)
					t.Throw("Missing item list '%s'.", s.c_str());
				ParseScript type = (lis.is_leveled ? PS_LEVELED_LIST : PS_LIST);
				script->code.push_back(type);
				script->code.push_back((int)lis.lis);
				crc.Update(type);
				crc.Update(lis.GetIdString());
			}
		}
	}
}

//=================================================================================================
void UnitLoader::ParseSpells(Ptr<SpellList>& list)
{
	// {
	t.AssertSymbol('{');
	t.Next();

	int index = 0;

	do
	{
		if(t.IsKeywordGroup(G_SPELL_KEYWORD))
		{
			SpellKeyword k = (SpellKeyword)t.GetKeywordId(G_SPELL_KEYWORD);
			if(k == SK_NON_COMBAT)
			{
				// non_combat
				t.Next();
				list->have_non_combat = t.MustGetBool();
				crc.Update(list->have_non_combat ? 2 : 1);
			}
			else
			{
				// null
				if(index == MAX_SPELLS)
					t.Throw("Too many spells (max %d for now).", MAX_SPELLS);
				++index;
				crc.Update(0);
			}
		}
		else
		{
			if(index == MAX_SPELLS)
				t.Throw("Too many spells (max %d for now).", MAX_SPELLS);
			t.AssertSymbol('{');
			t.Next();
			const string& spell_id = t.MustGetItemKeyword();
			Spell* spell = Spell::TryGet(spell_id.c_str());
			if(!spell)
				t.Throw("Missing spell '%s'.", spell_id.c_str());
			t.Next();
			int lvl = t.MustGetInt();
			if(lvl < 0)
				t.Throw("Invalid spell level %d.", lvl);
			t.Next();
			t.AssertSymbol('}');
			list->spell[index] = spell;
			list->level[index] = lvl;
			crc.Update(spell->id);
			crc.Update(lvl);
			++index;
		}
		t.Next();
	}
	while(!t.IsSymbol('}'));

	if(list->spell[0] == nullptr && list->spell[1] == nullptr && list->spell[2] == nullptr)
		t.Throw("Empty spell list.");

	SpellList::lists.push_back(list.Pin());
}

//=================================================================================================
void UnitLoader::ParseSounds(Ptr<SoundPack>& pack)
{
	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		SOUND_ID type = (SOUND_ID)t.MustGetKeywordId(G_SOUND_TYPE);
		crc.Update(type);
		vector<SoundPtr>& entries = pack->sounds[type];
		t.Next();

		if(t.IsSymbol('{'))
		{
			t.Next();
			while(!t.IsSymbol('}'))
			{
				const string& filename = t.MustGetString();
				SoundPtr sound = res_mgr->TryGet<Sound>(filename);
				if(!sound)
					LoadError("Missing sound '%s'.", filename.c_str());
				else
				{
					entries.push_back(sound);
					crc.Update(filename);
				}
				t.Next();
			}
		}
		else
		{
			const string& filename = t.MustGetString();
			SoundPtr sound = res_mgr->TryGet<Sound>(filename);
			if(!sound)
				LoadError("Missing sound '%s'.", filename.c_str());
			else
			{
				entries.push_back(sound);
				crc.Update(filename);
			}
		}
		t.Next();
	}

	SoundPack::packs.push_back(pack.Pin());
}

//=================================================================================================
void UnitLoader::ParseFrames(Ptr<FrameInfo>& frames)
{
	// {
	t.AssertSymbol('{');
	t.Next();

	bool have_simple = false;

	while(!t.IsSymbol('}'))
	{
		FrameKeyword k = (FrameKeyword)t.MustGetKeywordId(G_FRAME_KEYWORD);
		t.Next();

		switch(k)
		{
		case FK_ATTACKS:
			if(have_simple || frames->extra)
				t.Throw("Frame info already have attacks information.");
			t.AssertSymbol('{');
			t.Next();
			frames->extra = new AttackFrameInfo;
			crc.Update(0);
			do
			{
				t.AssertSymbol('{');
				t.Next();
				float start = t.MustGetFloat();
				t.Next();
				float end = t.MustGetFloat();
				t.Next();
				int flags = 0;
				t.ParseFlags(G_WEAPON_FLAG, flags);
				t.Next();
				t.AssertSymbol('}');
				crc.Update(start);
				crc.Update(end);
				crc.Update(flags);

				if(start < 0.f || start >= end || end > 1.f)
					t.Throw("Invalid attack frame times (%g %g).", start, end);

				frames->extra->e.push_back({ start, end, flags });
				t.Next();
			}
			while(!t.IsSymbol('}'));
			frames->attacks = frames->extra->e.size();
			crc.Update(frames->attacks);
			break;
		case FK_SIMPLE_ATTACKS:
			if(have_simple || frames->extra)
				t.Throw("Frame info already have attacks information.");
			else
			{
				t.AssertSymbol('{');
				t.Next();
				crc.Update(1);
				int index = 0;
				frames->attacks = 0;
				do
				{
					if(index == FrameInfo::MAX_ATTACKS)
						t.Throw("To many simple attacks (max %d).", FrameInfo::MAX_ATTACKS);
					t.AssertSymbol('{');
					t.Next();
					float start = t.MustGetFloat();
					t.Next();
					float end = t.MustGetFloat();
					t.Next();
					float power = 1.f;
					if(t.IsFloat())
					{
						power = t.GetFloat();
						if(power <= 0.f)
							t.Throw("Invalid attack frame power %g.", power);
						t.Next();
					}
					t.AssertSymbol('}');
					crc.Update(start);
					crc.Update(end);

					if(start < 0.f || start >= end || end > 1.f)
						t.Throw("Invalid attack frame times (%g %g).", start, end);

					frames->t[F_ATTACK1_START + index * 2] = start;
					frames->t[F_ATTACK1_END + index * 2] = end;
					frames->attack_power[index] = power;
					++index;
					++frames->attacks;
					t.Next();
				}
				while(!t.IsSymbol('}'));
			}
			break;
		case FK_CAST:
			{
				float f = t.MustGetFloat();
				if(!InRange(f, 0.f, 1.f))
					t.Throw("Invalid cast frame time %g.", f);
				frames->t[F_CAST] = f;
				crc.Update(2);
				crc.Update(f);
			}
			break;
		case FK_TAKE_WEAPON:
			{
				float f = t.MustGetFloat();
				if(!InRange(f, 0.f, 1.f))
					t.Throw("Invalid take weapon frame time %g.", f);
				frames->t[F_TAKE_WEAPON] = f;
				crc.Update(3);
				crc.Update(f);
			}
			break;
		case FK_BASH:
			{
				float f = t.MustGetFloat();
				if(!InRange(f, 0.f, 1.f))
					t.Throw("Invalid bash frame time %g.", f);
				frames->t[F_BASH] = f;
				crc.Update(4);
				crc.Update(f);
			}
			break;
		}

		t.Next();
	}

	FrameInfo::frames.push_back(frames.Pin());
}

//=================================================================================================
void UnitLoader::ParseTextures(Ptr<TexPack>& pack)
{
	// {
	t.AssertSymbol('{');
	t.Next();

	bool any = false;
	do
	{
		if(t.IsKeywordGroup(G_NULL))
		{
			pack->textures.push_back(TexOverride(nullptr));
			crc.Update(0);
		}
		else
		{
			const string& tex_id = t.MustGetString();
			Texture* tex = res_mgr->TryGet<Texture>(tex_id);
			if(tex)
			{
				pack->textures.push_back(TexOverride(tex));
				crc.Update(tex_id);
			}
			else
				LoadError("Missing texture override '%s'.", tex_id.c_str());
			any = true;
		}
		t.Next();
	}
	while(!t.IsSymbol('}'));

	if(!any)
		t.Throw("Texture pack without textures.");

	TexPack::packs.push_back(pack.Pin());
}

//=================================================================================================
void UnitLoader::ParseIdles(Ptr<IdlePack>& pack)
{
	// {
	t.AssertSymbol('{');
	t.Next();

	do
	{
		const string& s = t.MustGetString();
		pack->anims.push_back(s);
		crc.Update(s);
		t.Next();
	}
	while(!t.IsSymbol('}'));

	IdlePack::packs.push_back(pack.Pin());
}

//=================================================================================================
void UnitLoader::ParseAlias(const string& id)
{
	UnitData* ud = UnitData::TryGet(id.c_str());
	if(!ud)
		t.Throw("Missing unit data '%s'.", id.c_str());
	t.Next();

	const string& alias = t.MustGetItemKeyword();
	UnitData* ud2 = UnitData::TryGet(alias.c_str());
	if(ud2)
		t.Throw("Can't create alias '%s', already exists.", alias.c_str());

	UnitData::aliases[alias] = ud;
	crc.Update(alias);
}

//=================================================================================================
void UnitLoader::ParseGroup(const string& id)
{
	Ptr<UnitGroup> group;
	group->id = id;
	if(UnitGroup::TryGet(group->id))
		t.Throw("Group with that id already exists.");
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		if(t.IsKeywordGroup(G_GROUP_KEYWORD))
		{
			GroupKeyword k = (GroupKeyword)t.GetKeywordId(G_GROUP_KEYWORD);
			t.Next();
			switch(k)
			{
			case GK_GROUP:
				{
					const string& id = t.MustGetItemKeyword();
					UnitGroup* other_group = UnitGroup::TryGet(id);
					if(!other_group)
						t.Throw("Missing group '%s'.", id.c_str());
					for(UnitGroup::Entry& e : other_group->entries)
						group->entries.push_back(e);
					group->max_weight += other_group->max_weight;
					crc.Update(id);
				}
				break;
			case GK_FOOD_MOD:
				group->food_mod = t.MustGetInt();
				if(!InRange(group->food_mod, -2, 0))
					t.Throw("Invalid food_mod %d.", group->food_mod);
				break;
			case GK_ORC_FOOD:
				group->orc_food = t.MustGetBool();
				break;
			case GK_HAVE_CAMPS:
				group->have_camps = t.MustGetBool();
				break;
			case GK_ENCOUNTER_CHANCE:
				group->encounter_chance = t.MustGetInt();
				if(group->encounter_chance <= 0)
					t.Throw("Invalid encounter_chance %d.", group->encounter_chance);
				break;
			case GK_SPECIAL:
				group->special = (UnitGroup::Special)t.MustGetInt();
				if(!InRange((int)group->special, 0, 2))
					t.Throw("Invalid special %d.", group->special);
				break;
			case GK_GENDER:
				group->gender = t.MustGetBool();
				break;
			case GK_LIST:
				if(!group->entries.empty())
					t.Throw("Group already have entries.");
				group->is_list = true;
				t.AssertSymbol('{');
				t.Next();
				while(!t.IsSymbol('}'))
				{
					int weight = 1;
					if(t.IsInt())
					{
						weight = t.GetInt();
						if(weight < 1)
							t.Throw("Invalid list item weight %d.", weight);
						t.Next();
					}
					const string& id = t.MustGetItemKeyword();
					UnitGroup* gr = UnitGroup::TryGet(id);
					if(!gr)
						t.Throw("Missing unit group '%s'.", id.c_str());
					group->entries.push_back(UnitGroup::Entry(gr, weight));
					group->max_weight += weight;
					crc.Update(id);
					crc.Update(weight);
					t.Next();
					if(t.IsItem("encounter"))
					{
						for(UnitGroup::Entry& entry : group->entries)
						{
							if(entry.is_leader)
								t.Throw("Group list already have entry marked as 'encounter'.");
						}
						group->entries.back().is_leader = true;
						crc.Update(true);
						t.Next();
					}
				}
				if(group->entries.empty())
					t.Throw("Empty list.");
				break;
			default:
				t.Unexpected();
				break;
			}
		}
		else
		{
			if(group->is_list)
				t.Throw("Can't mix units and group list.");

			const string& id = t.MustGetItemKeyword();
			UnitData* ud = UnitData::TryGet(id.c_str());
			if(!ud)
				t.Throw("Missing unit '%s'.", id.c_str());
			crc.Update(id);
			t.Next();

			bool is_leader = false;
			if(t.IsKeyword(GK_LEADER, G_GROUP_KEYWORD))
			{
				is_leader = true;
				t.Next();
			}

			int weight = t.MustGetInt();
			if(weight < 1)
				t.Throw("Invalid unit weight %d.", weight);
			group->entries.push_back(UnitGroup::Entry(ud, weight, is_leader));
			group->max_weight += weight;
			crc.Update(weight);
			crc.Update(is_leader);
		}
		t.Next();
	}

	if(group->entries.empty() && !group->special)
		t.Throw("Empty group.");
	if(group->is_list && group->encounter_chance > 0)
	{
		bool ok = false;
		for(UnitGroup::Entry& entry : group->entries)
		{
			if(entry.is_leader)
			{
				ok = true;
				break;
			}
		}
		if(!ok)
			t.Throw("Group list with encounter chance must have entry marked as 'encounter'.");
	}

	UnitGroup::groups.push_back(group.Pin());
}

//=================================================================================================
void UnitLoader::ProcessDialogRequests()
{
	for(DialogRequest& request : dialog_requests)
	{
		UnitData* unit = UnitData::TryGet(request.unit);
		if(!unit)
			continue;
		uint pos = request.dialog.find_first_of('/');
		string quest_name = request.dialog.substr(0, pos);
		string dialog_name = request.dialog.substr(pos + 1);
		QuestScheme* scheme = QuestScheme::TryGet(quest_name);
		GameDialog* dialog = (scheme ? scheme->GetDialog(dialog_name) : nullptr);
		if(!scheme || !dialog)
		{
			++content.errors;
			LoadError("Missing quest dialog '%s/%s' for unit %s.", quest_name.c_str(), dialog_name.c_str(), unit->id.c_str());
		}
		else
			unit->dialog = dialog;
	}
}
