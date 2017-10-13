#include "Pch.h"
#include "Core.h"
#include "UnitData.h"
#include "ContentLoader.h"

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
		G_PROFILE_KEYWORD,
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
		PK_FIXED
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

public:
	//=================================================================================================
	void Load()
	{
		InitTokenizer();

		ContentLoader loader;
		bool ok = loader.Load(t, "units.txt", G_TYPE, [&, this](int top, const string& id)
		{
			switch(top)
			{
			case T_UNIT:
				ParseUnit(id);
				break;
			case T_PROFILE:
				ParseProfile(id);
				break;
			case T_ITEMS:
				ParseItems(id);
				break;
			case T_SPELLS:
				ParseSpells(id);
				break;
			case T_SOUNDS:
				ParseSounds(id);
				break;
			case T_FRAMES:
				ParseFrames(id);
				break;
			case T_TEX:
				ParseTextures(id);
				break;
			case T_IDLES:
				ParseIdles(id);
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
		});
		if(!ok)
			return;

		CalculateCrc();

		//Info("Loaded objects (%u), usables (%u) - crc %p.",
		//	BaseObject::objs.size() - BaseUsable::usables.size(), BaseUsable::usables.size(), content::crc[(int)content::Id::Objects]);
	}

private:
	//=================================================================================================
	void InitTokenizer()
	{
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
			{ "strength", (int)Attribute::STR },
			{ "endurance", (int)Attribute::END },
			{ "dexterity", (int)Attribute::DEX },
			{ "inteligence", (int)Attribute::INT },
			{ "wisdom", (int)Attribute::WIS },
			{ "charisma", (int)Attribute::CHA }
		});

		t.AddKeywords(G_SKILL, {
			{ "one_handed", (int)Skill::ONE_HANDED_WEAPON },
			{ "short_blade", (int)Skill::SHORT_BLADE },
			{ "long_blade", (int)Skill::LONG_BLADE },
			{ "blunt", (int)Skill::BLUNT },
			{ "axe", (int)Skill::AXE },
			{ "bow", (int)Skill::BOW },
			{ "unarmed", (int)Skill::UNARMED },
			{ "shield", (int)Skill::SHIELD },
			{ "light_armor", (int)Skill::LIGHT_ARMOR },
			{ "medium_armor", (int)Skill::MEDIUM_ARMOR },
			{ "heavy_armor", (int)Skill::HEAVY_ARMOR },
			{ "nature_magic", (int)Skill::NATURE_MAGIC },
			{ "gods_magic", (int)Skill::GODS_MAGIC },
			{ "mystic_magic", (int)Skill::MYSTIC_MAGIC },
			{ "spellcraft", (int)Skill::SPELLCRAFT },
			{ "concentration", (int)Skill::CONCENTRATION },
			{ "identification", (int)Skill::IDENTIFICATION },
			{ "lockpick", (int)Skill::LOCKPICK },
			{ "sneak", (int)Skill::SNEAK },
			{ "traps", (int)Skill::TRAPS },
			{ "steal", (int)Skill::STEAL },
			{ "animal_empathy", (int)Skill::ANIMAL_EMPATHY },
			{ "survival", (int)Skill::SURVIVAL },
			{ "persuasion", (int)Skill::PERSUASION },
			{ "alchemy", (int)Skill::ALCHEMY },
			{ "crafting", (int)Skill::CRAFTING },
			{ "healing", (int)Skill::HEALING },
			{ "athletics", (int)Skill::ATHLETICS },
			{ "rage", (int)Skill::RAGE }
		});

		t.AddKeyword("fixed", PK_FIXED, G_PROFILE_KEYWORD);

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
			unit->CopyFrom(*parent);
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
				break;
			case P_MAT:
				unit->mat = (MATERIAL_TYPE)t.MustGetKeywordId(G_MATERIAL);
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
				break;
			case P_PROFILE:
				if(t.IsSymbol('{'))
				{
					if(!ParseProfile(empty_id, &unit->stat_profile))
						t.Throw("Failed to load inline profile.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->stat_profile = nullptr;
					for(StatProfile* p : stat_profiles)
					{
						if(id == p->id)
						{
							unit->stat_profile = p;
							break;
						}
					}
					if(!unit->stat_profile)
						t.Throw("Missing stat profile '%s'.", id.c_str());
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
				}
				break;
			case P_HP:
				unit->hp_bonus = t.MustGetInt();
				if(unit->hp_bonus < 0)
					t.Throw("Invalid hp bonus %d.", unit->hp_bonus);
				break;
			case P_STAMINA:
				unit->stamina_bonus = t.MustGetInt();
				break;
			case P_DEF:
				unit->def_bonus = t.MustGetInt();
				if(unit->def_bonus < 0)
					t.Throw("Invalid def bonus %d.", unit->def_bonus);
				break;
			case P_ITEMS:
				if(t.IsSymbol('{'))
				{
					if(LoadItems(t, crc, &unit->item_script))
						unit->items = &unit->item_script->code[0];
					else
						t.Throw("Failed to load inline item script.");
				}
				else
				{
					unit->item_script = nullptr;
					unit->items = nullptr;
					if(!t.IsKeywordGroup(G_NULL))
					{
						const string& id = t.MustGetItemKeyword();
						for(ItemScript* s : item_scripts)
						{
							if(s->id == id)
							{
								unit->item_script = s;
								break;
							}
						}
						if(unit->item_script)
							unit->items = &unit->item_script->code[0];
						else
							t.Throw("Missing item script '%s'.", id.c_str());
					}
				}
				break;
			case P_SPELLS:
				if(t.IsSymbol('{'))
				{
					if(!LoadSpells(t, crc, &unit->spells))
						t.Throw("Failed to load inline spell list.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->spells = nullptr;
					for(SpellList* s : spell_lists)
					{
						if(s->id == id)
						{
							unit->spells = s;
							break;
						}
					}
					if(!unit->spells)
						t.Throw("Missing spell list '%s'.", id.c_str());
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
				break;
			case P_DIALOG:
				{
					const string& id = t.MustGetItemKeyword();
					unit->dialog = FindDialog(id.c_str());
					if(!unit->dialog)
						t.Throw("Missing dialog '%s'.", id.c_str());
				}
				break;
			case P_GROUP:
				unit->group = (UNIT_GROUP)t.MustGetKeywordId(G_GROUP);
				break;
			case P_DMG_TYPE:
				unit->dmg_type = ReadFlags(t, G_DMG_TYPE);
				break;
			case P_WALK_SPEED:
				unit->walk_speed = t.MustGetNumberFloat();
				if(unit->walk_speed < 0.5f)
					t.Throw("Invalid walk speed %g.", unit->walk_speed);
				break;
			case P_RUN_SPEED:
				unit->run_speed = t.MustGetNumberFloat();
				if(unit->run_speed < 0)
					t.Throw("Invalid run speed %g.", unit->run_speed);
				break;
			case P_ROT_SPEED:
				unit->rot_speed = t.MustGetNumberFloat();
				if(unit->rot_speed < 0.5f)
					t.Throw("Invalid rot speed %g.", unit->rot_speed);
				break;
			case P_BLOOD:
				unit->blood = (BLOOD)t.MustGetKeywordId(G_BLOOD);
				break;
			case P_SOUNDS:
				if(t.IsSymbol('{'))
				{
					if(!LoadSounds(t, crc, &unit->sounds))
						t.Throw("Failed to load inline sound pack.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->sounds = nullptr;
					for(SoundPack* s : sound_packs)
					{
						if(s->id == id)
						{
							unit->sounds = s;
							break;
						}
					}
					if(!unit->sounds)
						t.Throw("Missing sound pack '%s'.", id.c_str());
				}
				break;
			case P_FRAMES:
				if(t.IsSymbol('{'))
				{
					if(!LoadFrames(t, crc, &unit->frames))
						t.Throw("Failed to load inline frame infos.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->frames = nullptr;
					for(FrameInfo* fi : frame_infos)
					{
						if(fi->id == id)
						{
							unit->frames = fi;
							break;
						}
					}
					if(!unit->frames)
						t.Throw("Missing frame infos '%s'.", id.c_str());
				}
				break;
			case P_TEX:
				if(t.IsSymbol('{'))
				{
					TexPack* tex;
					if(!LoadTex(t, crc, &tex))
						t.Throw("Failed to load inline tex pack.");
					else
						unit->tex = tex;
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					TexPack* tex = nullptr;
					for(TexPack* tp : tex_packs)
					{
						if(tp->id == id)
						{
							tex = tp;
							break;
						}
					}
					if(tex)
						unit->tex = tex;
					else
						t.Throw("Missing tex pack '%s'.", id.c_str());
				}
				break;
			case P_IDLES:
				if(t.IsSymbol('{'))
				{
					IdlePack* idles;
					if(!LoadIdles(t, crc, &idles))
						t.Throw("Failed to load inline idles pack.");
					else
						unit->idles = &idles->anims;
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					IdlePack* idles = nullptr;
					for(IdlePack* ip : idle_packs)
					{
						if(ip->id == id)
						{
							idles = ip;
							break;
						}
					}
					if(idles)
						unit->idles = &idles->anims;
					else
						t.Throw("Missing idles pack '%s'.", id.c_str());
				}
				break;
			case P_WIDTH:
				unit->width = t.MustGetNumberFloat();
				if(unit->width < 0.1f)
					t.Throw("Invalid width %g.", unit->width);
				break;
			case P_ATTACK_RANGE:
				unit->attack_range = t.MustGetNumberFloat();
				if(unit->attack_range < 0.1f)
					t.Throw("Invalid attack range %g.", unit->attack_range);
				break;
			case P_ARMOR_TYPE:
				unit->armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_TYPE);
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

		unit_datas.insert(unit);
	}

	//=================================================================================================
	void ParseProfile(const string& id, StatProfile* in_profile = nullptr)
	{

	}

	//=================================================================================================
	void ParseItems(const string& id)
	{

	}

	//=================================================================================================
	void ParseSpells(const string& id)
	{

	}

	//=================================================================================================
	void ParseSounds(const string& id)
	{

	}

	//=================================================================================================
	void ParseFrames(const string& id)
	{

	}

	//=================================================================================================
	void ParseTextures(const string& id)
	{

	}

	//=================================================================================================
	void ParseIdles(const string& id)
	{

	}

	//=================================================================================================
	void ParseAlias(const string& id)
	{

	}

	//=================================================================================================
	void ParseGroup(const string& id)
	{

	}

	//=================================================================================================
	void CalculateCrc()
	{

	}

	Tokenizer t;
	string empty_id;
};
