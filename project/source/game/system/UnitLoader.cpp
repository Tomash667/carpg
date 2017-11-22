#include "Pch.h"
#include "Core.h"
#include "UnitData.h"
#include "UnitGroup.h"
#include "ContentLoader.h"
#include "Dialog.h"
#include "ItemScript.h"
#include "Item.h"
#include "Spell.h"
#include "Crc.h"

//-----------------------------------------------------------------------------
class UnitLoader
{
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
		G_PROFILE_KEYWORDS,
		G_PROFILE_FLAGS,
		G_SUBPROFILE_KEYWORDS,
		G_SOUND_TYPE,
		G_FRAME_KEYWORD,
		G_WEAPON_FLAG,
		G_NULL,
		G_SPELL_KEYWORD,
		G_ITEM_KEYWORD,
		G_GROUP_KEYWORD
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
		T_GROUP,
		T_GROUP_LIST
	};

	enum Property
	{
		P_MESH,
		P_MAT,
		P_LEVEL,
		P_PROFILE,
		P_FLAGS,
		P_HP,
		P_HP_BONUS,
		P_ATK,
		P_ATK_BONUS,
		P_DEF,
		P_DEF_BONUS,
		P_STAMINA,
		P_BLOCK,
		P_BLOCK_BONUS,
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
		P_SOUNDS,
		P_FRAMES,
		P_TEX,
		P_IDLES,
		P_WIDTH,
		P_ATTACK_RANGE,
		P_ARMOR_TYPE
	};

	enum ProfileKeyword
	{
		PK_FLAGS,
		PK_SUBPROFILE
	};

	enum SubprofileKeyword
	{
		SPK_WEAPON,
		SPK_ARMOR,
		SPK_SKILL,
		SPK_TAG,
		SPK_PERK
	};

	enum SoundType
	{
		ST_SEE_ENEMY,
		ST_PAIN,
		ST_DEATH,
		ST_ATTACK
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
		GK_GROUP
	};

	enum IfState
	{
		IFS_START,
		IFS_START_INLINE,
		IFS_ELSE,
		IFS_ELSE_INLINE
	};

public:
	//=================================================================================================
	void Load()
	{
		InitTokenizer();

		ContentLoader loader;
		bool ok = loader.Load(t, "units.txt", G_TYPE, [&, this](int top, const string& id)
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
					profile->unit_data = nullptr;
					t.Next();
					ParseProfile(profile);
				}
				break;
			case T_ITEMS:
				{
					if(ItemScript::TryGet(id))
						t.Throw("Id must be unique.");
					Ptr<ItemScript> script;
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
			case T_GROUP_LIST:
				ParseGroupList(id);
				break;
			default:
				assert(0);
				break;
			}
		});
		if(!ok)
			return;

		content::crc[(int)content::Id::Units] = crc.Get();

		Info("Loaded units (%u) - crc %p.", UnitData::units.size(), content::crc[(int)content::Id::Units]);
	}

private:
	//=================================================================================================
	void InitTokenizer()
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
			{ "group", T_GROUP },
			{ "group_list", T_GROUP_LIST }
		});

		t.AddKeywords(G_PROPERTY, {
			{ "mesh", P_MESH },
			{ "mat", P_MAT },
			{ "level", P_LEVEL },
			{ "profile", P_PROFILE },
			{ "flags", P_FLAGS },
			{ "hp", P_HP },
			{ "hp_bonus", P_HP_BONUS },
			{ "atk", P_ATK },
			{ "atk_bonus", P_ATK_BONUS },
			{ "def", P_DEF },
			{ "def_bonus", P_DEF_BONUS },
			{ "stamina", P_STAMINA },
			{ "block", P_BLOCK },
			{ "block_bonus", P_BLOCK_BONUS },
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
			{ "sounds", P_SOUNDS },
			{ "frames", P_FRAMES },
			{ "tex", P_TEX },
			{ "idles", P_IDLES },
			{ "width", P_WIDTH },
			{ "attack_range", P_ATTACK_RANGE },
			{ "armor_type", P_ARMOR_TYPE }
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
			{ "leader", F_LEADER },
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
			{ "no_class", F2_NO_CLASS },
			{ "contest", F2_CONTEST },
			{ "contest_50", F2_CONTEST_50 },
			{ "class_flag", F2_CLASS_FLAG },
			{ "warrior", F2_WARRIOR },
			{ "hunter", F2_HUNTER },
			{ "rogue", F2_ROGUE },
			{ "old", F2_OLD },
			{ "melee", F2_MELEE },
			{ "melee_50", F2_MELEE_50 },
			{ "boss", F2_BOSS },
			{ "bloodless", F2_BLOODLESS },
			{ "limited_rot", F2_LIMITED_ROT },
			{ "cleric", F2_CLERIC },
			{ "stun_res", F2_STUN_RESISTANCE },
			{ "sit_on_throne", F2_SIT_ON_THRONE },
			{ "orc_sounds", F2_ORC_SOUNDS },
			{ "goblin_sounds", F2_GOBLIN_SOUNDS },
			{ "xar", F2_XAR },
			{ "golem_sounds", F2_GOLEM_SOUNDS },
			{ "tournament", F2_TOURNAMENT },
			{ "yell", F2_YELL },
			{ "backstab", F2_BACKSTAB },
			{ "ignore_block", F2_IGNORE_BLOCK },
			{ "backstab_res", F2_BACKSTAB_RES },
			{ "magic_res50", F2_MAGIC_RES50 },
			{ "magic_res25", F2_MAGIC_RES25 },
			{ "mark", F2_MARK },
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
			{ "talk_at_competition", F3_TALK_AT_COMPETITION },
			{ "fixed", F3_FIXED }
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
			{ "strength", (int)Attribute::STR },
			{ "endurance", (int)Attribute::END },
			{ "dexterity", (int)Attribute::DEX },
			{ "inteligence", (int)Attribute::INT },
			{ "wisdom", (int)Attribute::WIS },
			{ "charisma", (int)Attribute::CHA }
		});

		for(uint i = 0; i < (uint)Skill::MAX; ++i)
			t.AddKeyword(SkillInfo::skills[i].id, i, G_SKILL);

		t.AddKeywords(G_PROFILE_KEYWORDS, {
			{ "flags", PK_FLAGS },
			{ "subprofile", PK_SUBPROFILE }
		});

		t.AddKeywords(G_PROFILE_FLAGS, {
			{ "fixed", StatProfile::F_FIXED },
			{ "double_weapon_tag_skill", StatProfile::F_DOUBLE_WEAPON_TAG_SKILL }
		});

		t.AddKeywords(G_SUBPROFILE_KEYWORDS, {
			{ "weapon", SPK_WEAPON },
			{ "armor", SPK_ARMOR },
			{ "skill", SPK_SKILL },
			{ "tag", SPK_TAG },
			{ "perk", SPK_PERK }
		});

		t.AddKeywords(G_SOUND_TYPE, {
			{ "see_enemy", ST_SEE_ENEMY },
			{ "pain", ST_PAIN },
			{ "death", ST_DEATH },
			{ "attack", ST_ATTACK }
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
			{ "group", GK_GROUP }
		});
	}

	//=================================================================================================
	void ParseUnit(const string& id)
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
				unit->mesh_id = t.MustGetString();
				crc.Update(unit->mesh_id);
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
					unit->stat_profile->unit_data = unit.Get();
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
					bool clear = !t.IsSymbol('|');
					if(!clear)
						t.Next();
					ReadFlags(t, {
						{ &unit->flags, G_FLAGS },
						{ &unit->flags2, G_FLAGS2 },
						{ &unit->flags3, G_FLAGS3 }
					}, clear);
					crc.Update(unit->flags);
					crc.Update(unit->flags2);
					crc.Update(unit->flags3);
				}
				break;
			case P_HP:
				unit->hp = t.MustGetInt();
				if(unit->hp < 0)
					t.Throw("Invalid hp %d.", unit->hp);
				crc.Update(unit->hp);
				break;
			case P_HP_BONUS:
				unit->hp_bonus = t.MustGetInt();
				if(unit->hp_bonus < 0)
					t.Throw("Invalid hp bonus %d.", unit->hp_bonus);
				crc.Update(unit->hp_bonus);
				break;
			case P_ATK:
				unit->atk = t.MustGetInt();
				if(unit->atk < 0)
					t.Throw("Invalid attack %d.", unit->atk);
				crc.Update(unit->atk);
				break;
			case P_ATK_BONUS:
				unit->atk_bonus = t.MustGetInt();
				if(unit->atk_bonus < 0)
					t.Throw("Invalid attack bonus %d.", unit->atk_bonus);
				crc.Update(unit->atk_bonus);
				break;
			case P_DEF:
				unit->def = t.MustGetInt();
				if(unit->def < 0)
					t.Throw("Invalid defense %d.", unit->def);
				crc.Update(unit->def);
				break;
			case P_DEF_BONUS:
				unit->def_bonus = t.MustGetInt();
				if(unit->def_bonus < 0)
					t.Throw("Invalid defense bonus %d.", unit->def_bonus);
				crc.Update(unit->def_bonus);
				break;
			case P_STAMINA:
				unit->stamina = t.MustGetInt();
				crc.Update(unit->stamina);
				break;
			case P_BLOCK:
				unit->block = t.MustGetInt();
				if(unit->block < 0)
					t.Throw("Invalid block %d.", unit->block);
				crc.Update(unit->block);
				break;
			case P_BLOCK_BONUS:
				unit->block_bonus = t.MustGetInt();
				if(unit->block_bonus < 0)
					t.Throw("Invalid block bonus %d.", unit->block_bonus);
				crc.Update(unit->block_bonus);
				break;
			case P_ITEMS:
				if(t.IsSymbol('{'))
				{
					Ptr<ItemScript> script;
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
					const string& id = t.MustGetItemKeyword();
					unit->dialog = FindDialog(id.c_str());
					if(!unit->dialog)
						t.Throw("Missing dialog '%s'.", id.c_str());
					crc.Update(unit->id);
				}
				break;
			case P_GROUP:
				unit->group = (UNIT_GROUP)t.MustGetKeywordId(G_GROUP);
				crc.Update(unit->group);
				break;
			case P_DMG_TYPE:
				unit->dmg_type = ReadFlags(t, G_DMG_TYPE);
				crc.Update(unit->dmg_type);
				break;
			case P_WALK_SPEED:
				unit->walk_speed = t.MustGetNumberFloat();
				if(unit->walk_speed < 0.5f)
					t.Throw("Invalid walk speed %g.", unit->walk_speed);
				crc.Update(unit->walk_speed);
				break;
			case P_RUN_SPEED:
				unit->run_speed = t.MustGetNumberFloat();
				if(unit->run_speed < 0)
					t.Throw("Invalid run speed %g.", unit->run_speed);
				crc.Update(unit->run_speed);
				break;
			case P_ROT_SPEED:
				unit->rot_speed = t.MustGetNumberFloat();
				if(unit->rot_speed < 0.5f)
					t.Throw("Invalid rot speed %g.", unit->rot_speed);
				crc.Update(unit->rot_speed);
				break;
			case P_BLOOD:
				unit->blood = (BLOOD)t.MustGetKeywordId(G_BLOOD);
				crc.Update(unit->blood);
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
				unit->width = t.MustGetNumberFloat();
				if(unit->width < 0.1f)
					t.Throw("Invalid width %g.", unit->width);
				crc.Update(unit->width);
				break;
			case P_ATTACK_RANGE:
				unit->attack_range = t.MustGetNumberFloat();
				if(unit->attack_range < 0.1f)
					t.Throw("Invalid attack range %g.", unit->attack_range);
				crc.Update(unit->attack_range);
				break;
			case P_ARMOR_TYPE:
				unit->armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_TYPE);
				crc.Update(unit->armor_type);
				break;
			default:
				t.Unexpected();
				break;
			}

			t.Next();
		}

		// configure
		if(IS_SET(unit->flags, F_HUMAN))
			unit->type = UNIT_TYPE::HUMAN;
		else if(IS_SET(unit->flags, F_HUMANOID))
			unit->type = UNIT_TYPE::HUMANOID;
		else
			unit->type = UNIT_TYPE::ANIMAL;

		if(unit->mesh_id.empty())
		{
			if(unit->type != UNIT_TYPE::HUMAN)
				t.Throw("Unit without mesh.");
		}
		else
		{
			if(unit->type == UNIT_TYPE::HUMAN)
				t.Throw("Human unit with custom mesh.");
		}

		UnitData::units.insert(unit.Pin());
	}

	//=================================================================================================
	void ParseProfile(Ptr<StatProfile>& profile)
	{
		profile->flags = 0;
		for(int i = 0; i < (int)Attribute::MAX; ++i)
			profile->attrib[i] = 10;
		for(int i = 0; i < (int)Skill::MAX; ++i)
			profile->skill[i] = -1;

		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeywordGroup(G_PROFILE_KEYWORDS))
			{
				auto keyword = (ProfileKeyword)t.GetKeywordId(G_PROFILE_KEYWORDS);
				t.Next();
				switch(keyword)
				{
				case PK_FLAGS:
					profile->flags = ReadFlags(t, G_PROFILE_FLAGS);
					break;
				case PK_SUBPROFILE:
					ParseSubprofile(profile);
					break;
				}
			}
			else if(t.IsKeywordGroup(G_ATTRIBUTE))
			{
				int a = t.MustGetKeywordId(G_ATTRIBUTE);
				t.Next();
				int val = t.MustGetInt();
				if(val < 1)
					t.Throw("Invalid attribute '%s' value %d.", AttributeInfo::attributes[a].id, val);
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
					t.Throw("Invalid skill '%s' value %d.", SkillInfo::skills[s].id, val);
				profile->skill[s] = val;
				crc.Update(2);
				crc.Update(s);
				crc.Update(val);
			}
			else
			{
				t.StartUnexpected()
					.AddList(tokenizer::T_KEYWORD_GROUP, { G_SKILL, G_ATTRIBUTE, G_PROFILE_KEYWORDS })
					.Throw();
			}

			t.Next();
		}

		StatProfile::profiles.push_back(profile.Pin());
	}

	//=================================================================================================
	void ParseSubprofile(StatProfile* profile)
	{
		Ptr<SubProfile> sub;

		// id
		sub->id = t.MustGetItem();
		if(profile->TryGetSubprofile(sub->id))
			t.Throw("Id must be unique.");
		crc.Update(sub->id);
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			auto keyword = (SubprofileKeyword)t.MustGetKeywordId(G_SUBPROFILE_KEYWORDS);
			switch(keyword)
			{
			case SPK_WEAPON:
				t.Next();
				sub->weapons.clear();
				crc.Update(SPK_WEAPON);
				t.AssertSymbol('(');
				t.Next();
				while(!t.IsSymbol(')'))
				{
					auto skill = (Skill)t.MustGetKeywordId(G_SKILL);
					if(skill != Skill::SHORT_BLADE && skill != Skill::LONG_BLADE && skill != Skill::AXE && skill != Skill::BLUNT)
						t.Throw("Invalid weapon skill.");
					sub->weapons.push_back(skill);
					crc.Update(skill);
					t.Next();
				}
				t.Next();
				if(sub->weapons.size() < 2)
					t.Throw("Subprofile with %u weapons.", sub->weapons.size());
				break;
			case SPK_ARMOR:
				t.Next();
				sub->armors.clear();
				crc.Update(SPK_ARMOR);
				t.AssertSymbol('(');
				t.Next();
				while(!t.IsSymbol(')'))
				{
					auto skill = (Skill)t.MustGetKeywordId(G_SKILL);
					if(skill != Skill::LIGHT_ARMOR && skill != Skill::MEDIUM_ARMOR && skill != Skill::HEAVY_ARMOR)
						t.Throw("Invalid armor skill.");
					sub->armors.push_back(skill);
					sub->weapons.push_back(skill);
					t.Next();
				}
				t.Next();
				if(sub->armors.size() < 2)
					t.Throw("Subprofile with %u armors.", sub->armors.size());
				break;
			case SPK_SKILL:
				{
					bool is_tag;
					float value;
					t.Next();
					if(t.IsKeyword(SPK_TAG, G_SUBPROFILE_KEYWORDS))
					{
						is_tag = true;
						value = 1.f;
					}
					else if(t.IsNumber())
					{
						is_tag = false;
						value = t.GetFloat();
						if(value <= 0.f || value > 1.f)
							t.Throw("Invalid skill value %g.", value);
					}
					else
					{
						int group = G_SUBPROFILE_KEYWORDS;
						int keyword = SPK_TAG;
						t.StartUnexpected()
							.Add(tokenizer::T_FLOAT)
							.Add(tokenizer::T_KEYWORD, &keyword, &group)
							.Throw();
					}
					t.Next();

					Skill skill;
					if(t.IsKeywordGroup(G_SKILL))
						skill = (Skill)t.GetKeywordId(G_SKILL);
					else if(t.IsKeyword(SPK_WEAPON, G_SUBPROFILE_KEYWORDS))
					{
						skill = Skill::WEAPON_PROFILE;
						if(sub->weapons.empty())
							t.Throw("Weapon skill without weapon skills declared.");
					}
					else if(t.IsKeyword(SPK_ARMOR, G_SUBPROFILE_KEYWORDS))
					{
						skill = Skill::ARMOR_PROFILE;
						if(sub->armors.empty())
							t.Throw("Armor skill without armor skills declared.");
					}
					else
					{
						int group = G_SKILL;
						t.Unexpected(tokenizer::T_KEYWORD_GROUP, &group);
					}
					for(auto& s : sub->skills)
					{
						if(s.skill == skill)
							t.Throw("Skill %s already used.", t.GetTokenString().c_str());
					}
					t.Next();

					sub->skills.push_back({ skill, value, is_tag });
					crc.Update(SPK_SKILL);
					crc.Update(sub->skills.back());
				}
				break;
			case SPK_PERK:
				{
					t.Next();
					auto& id = t.MustGetItemKeyword();
					auto perk = PerkInfo::Find(id);
					if(!perk)
						t.Throw("Missing perk '%s'.", id.c_str());
					t.Next();

					int value = 0;
					switch(perk->required_value)
					{
					case PerkInfo::None:
						break;
					case PerkInfo::Attribute:
						value = t.MustGetKeywordId(G_ATTRIBUTE);
						t.Next();
						break;
					case PerkInfo::Skill:
						if(t.IsKeywordGroup(G_SKILL))
							value = t.MustGetKeywordId(G_SKILL);
						else if(t.IsKeyword(SPK_WEAPON, G_SUBPROFILE_KEYWORDS))
						{
							value = (int)Skill::WEAPON_PROFILE;
							if(sub->weapons.empty())
								t.Throw("Weapon skill without weapon skills declared.");
						}
						else if(t.IsKeyword(SPK_ARMOR, G_SUBPROFILE_KEYWORDS))
						{
							value = (int)Skill::ARMOR_PROFILE;
							if(sub->armors.empty())
								t.Throw("Armor skill without armor skills declared.");
						}
						else
							t.Unexpected();
						t.Next();
						break;
					}

					sub->perks.push_back({ perk->perk_id, value });
				}
				break;
			default:
				t.Unexpected();
				break;
			}
		}

		sub->variants = max(sub->weapons.size(), 1u) * max(sub->armors.size(), 1u);
		profile->subprofiles.push_back(sub.Pin());
	}

	//=================================================================================================
	void ParseItems(Ptr<ItemScript>& script)
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
						} while(!t.IsSymbol('}'));

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
			else if(t.IsKeyword() || t.IsItem() || t.IsSymbol('!'))
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
	void AddItem(ItemScript* script)
	{
		if(!t.IsSymbol('!'))
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

	//=================================================================================================
	void ParseSpells(Ptr<SpellList>& list)
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
					if(index == 3)
						t.Throw("Too many spells (max 3 for now).");
					++index;
					crc.Update(0);
				}
			}
			else
			{
				if(index == 3)
					t.Throw("Too many spells (max 3 for now).");
				t.AssertSymbol('{');
				t.Next();
				const string& spell_id = t.MustGetItemKeyword();
				Spell* spell = FindSpell(spell_id.c_str());
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
		} while(!t.IsSymbol('}'));

		if(list->spell[0] == nullptr && list->spell[1] == nullptr && list->spell[2] == nullptr)
			t.Throw("Empty spell list.");

		SpellList::lists.push_back(list.Pin());
	}

	//=================================================================================================
	void ParseSounds(Ptr<SoundPack>& pack)
	{
		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			SoundType type = (SoundType)t.MustGetKeywordId(G_SOUND_TYPE);
			t.Next();

			pack->filename[(int)type] = t.MustGetString();
			crc.Update(type);
			crc.Update(pack->filename[(int)type]);
			t.Next();
		}

		SoundPack::packs.push_back(pack.Pin());
	}

	//=================================================================================================
	void ParseFrames(Ptr<FrameInfo>& frames)
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
					float start = t.MustGetNumberFloat();
					t.Next();
					float end = t.MustGetNumberFloat();
					t.Next();
					int flags = ReadFlags(t, G_WEAPON_FLAG);
					t.Next();
					t.AssertSymbol('}');
					crc.Update(start);
					crc.Update(end);
					crc.Update(flags);

					if(start < 0.f || start >= end || end > 1.f)
						t.Throw("Invalid attack frame times (%g %g).", start, end);

					frames->extra->e.push_back({ start, end, flags });
					t.Next();
				} while(!t.IsSymbol('}'));
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
						if(index == 3)
							t.Throw("To many simple attacks (max 3 for now).");
						t.AssertSymbol('{');
						t.Next();
						float start = t.MustGetNumberFloat();
						t.Next();
						float end = t.MustGetNumberFloat();
						t.Next();
						t.AssertSymbol('}');
						crc.Update(start);
						crc.Update(end);

						if(start < 0.f || start >= end || end > 1.f)
							t.Throw("Invalid attack frame times (%g %g).", start, end);

						frames->t[F_ATTACK1_START + index * 2] = start;
						frames->t[F_ATTACK1_END + index * 2] = end;
						++index;
						++frames->attacks;
						t.Next();
					} while(!t.IsSymbol('}'));
				}
				break;
			case FK_CAST:
				{
					float f = t.MustGetNumberFloat();
					if(!InRange(f, 0.f, 1.f))
						t.Throw("Invalid cast frame time %g.", f);
					frames->t[F_CAST] = f;
					crc.Update(2);
					crc.Update(f);
				}
				break;
			case FK_TAKE_WEAPON:
				{
					float f = t.MustGetNumberFloat();
					if(!InRange(f, 0.f, 1.f))
						t.Throw("Invalid take weapon frame time %g.", f);
					frames->t[F_TAKE_WEAPON] = f;
					crc.Update(3);
					crc.Update(f);
				}
				break;
			case FK_BASH:
				{
					float f = t.MustGetNumberFloat();
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
	void ParseTextures(Ptr<TexPack>& pack)
	{
		// {
		t.AssertSymbol('{');
		t.Next();

		bool any = false;
		do
		{
			if(t.IsKeywordGroup(G_NULL))
			{
				pack->textures.push_back(TexId(nullptr));
				crc.Update(0);
			}
			else
			{
				const string& s = t.MustGetString();
				pack->textures.push_back(TexId(s));
				crc.Update(s);
				any = true;
			}
			t.Next();
		} while(!t.IsSymbol('}'));

		if(!any)
			t.Throw("Texture pack without textures.");

		TexPack::packs.push_back(pack.Pin());
	}

	//=================================================================================================
	void ParseIdles(Ptr<IdlePack>& pack)
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
		} while(!t.IsSymbol('}'));

		IdlePack::packs.push_back(pack.Pin());
	}

	//=================================================================================================
	void ParseAlias(const string& id)
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
	void ParseGroup(const string& id)
	{
		Ptr<UnitGroup> group;
		group->id = id;
		group->total = 0;
		if(UnitGroup::TryGet(group->id))
			t.Throw("Group with that id already exists.");
		t.Next();

		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeyword(GK_GROUP, G_GROUP_KEYWORD))
			{
				t.Next();
				const string& id = t.MustGetItemKeyword();
				UnitGroup* other_group = UnitGroup::TryGet(id);
				if(!other_group)
					t.Throw("Missing group '%s'.", id.c_str());
				for(UnitGroup::Entry& e : other_group->entries)
					group->entries.push_back(e);
				group->total += other_group->total;
				crc.Update(id);
			}
			else
			{
				const string& id = t.MustGetItemKeyword();
				UnitData* ud = UnitData::TryGet(id.c_str());
				if(!ud)
					t.Throw("Missing unit '%s'.", id.c_str());
				crc.Update(id);
				t.Next();

				if(t.IsKeyword(GK_LEADER, G_GROUP_KEYWORD))
				{
					group->leader = ud;
					crc.Update(0);
				}
				else
				{
					int count = t.MustGetInt();
					if(count < 1)
						t.Throw("Invalid unit count %d.", count);
					group->entries.push_back(UnitGroup::Entry(ud, count));
					group->total += count;
					crc.Update(count);
				}
			}
			t.Next();
		}

		if(group->entries.empty())
			t.Throw("Empty group.");

		UnitGroup::groups.push_back(group.Pin());
	}

	//=================================================================================================
	void ParseGroupList(const string& id)
	{
		Ptr<UnitGroupList> list;
		list->id = id;
		if(UnitGroupList::TryGet(list->id))
			t.Throw("Group list with that id already exists.");
		crc.Update(list->id);
		t.Next();

		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			auto& id = t.MustGetItemKeyword();
			auto group = UnitGroup::TryGet(id);
			if(!group)
				t.Throw("Missing unit group '%s'.", id.c_str());
			list->groups.push_back(group);
			crc.Update(id);
			t.Next();
		}

		if(list->groups.empty())
			t.Throw("Empty list.");

		UnitGroupList::lists.push_back(list.Pin());
	}

	Tokenizer t;
	Crc crc;
};

//=================================================================================================
void content::LoadUnits()
{
	UnitLoader loader;
	loader.Load();
}

//=================================================================================================
void content::CleanupUnits()
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
	DeleteElements(UnitGroupList::lists);
}
