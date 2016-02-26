#include "Pch.h"
#include "Base.h"
#include "UnitData.h"
#include "ItemScript.h"
#include "Dialog.h"
#include "Spell.h"
#include "Item.h"
#include "Crc.h"

extern string g_system_dir;

vector<StatProfile*> stat_profiles;
vector<ItemScript*> item_scripts;
vector<SpellList*> spell_lists;
vector<SoundPack*> sound_packs;
vector<IdlePack*> idle_packs;
vector<TexPack*> tex_packs;
vector<FrameInfo*> frame_infos;
UnitDataContainer unit_datas;
std::map<string, UnitData*> unit_aliases;
std::map<string, DialogEntry*> dialogs_map;
UnitData unit_data_search;
vector<UnitGroup*> unit_groups;

void UnitData::CopyFrom(UnitData& ud)
{
	mesh_id = ud.mesh_id;
	mat = ud.mat;
	level = ud.level;
	stat_profile = ud.stat_profile;
	hp_bonus = ud.hp_bonus;
	def_bonus = ud.def_bonus;
	dmg_type = ud.dmg_type;
	flags = ud.flags;
	flags2 = ud.flags2;
	flags3 = ud.flags3;
	items = ud.items;
	spells = ud.spells;
	gold = ud.gold;
	gold2 = ud.gold2;
	dialog = ud.dialog;
	group = ud.group;
	walk_speed = ud.walk_speed;
	run_speed = ud.run_speed;
	rot_speed = ud.rot_speed;
	width = ud.width;
	attack_range = ud.attack_range;
	blood = ud.blood;
	sounds = ud.sounds;
	frames = ud.frames;
	tex = ud.tex;
	idles = ud.idles;
	armor_type = ud.armor_type;
	item_script = ud.item_script;
	new_items = ud.new_items;
}

enum KeywordGroup
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

enum Type
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

//=================================================================================================
bool LoadProfile(Tokenizer& t, CRC32& crc, StatProfile** result = nullptr)
{
	StatProfile* profile = new StatProfile;
	profile->fixed = false;
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		profile->attrib[i] = 10;
	for(int i = 0; i < (int)Skill::MAX; ++i)
		profile->skill[i] = -1;

	try
	{
		if(!result)
		{
			// not inline, id
			profile->id = t.MustGetItemKeyword();
			crc.Update(profile->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeyword(PK_FIXED, G_PROFILE_KEYWORD))
			{
				t.Next();
				profile->fixed = t.MustGetBool();
			}
			else if(t.IsKeywordGroup(G_ATTRIBUTE))
			{
				int a = t.MustGetKeywordId(G_ATTRIBUTE);
				t.Next();
				int val = t.MustGetInt();
				if(val < 1)
					t.Throw("Invalid attribute '%s' value %d.", g_attributes[a].id, val);
				crc.Update(a);
				crc.Update(val);
				profile->attrib[a] = val;
			}
			else if(t.IsKeywordGroup(G_SKILL))
			{
				int s = t.MustGetKeywordId(G_SKILL);
				t.Next();
				int val = t.MustGetInt();
				if(val < -1)
					t.Throw("Invalid skill '%s' value %d.", g_skills[s].id, val);
				crc.Update(s);
				crc.Update(val);
				profile->skill[s] = val;
			}
			else
			{
				int a = PK_FIXED, b = G_PROFILE_KEYWORD, c = G_ATTRIBUTE, d = G_SKILL;
				t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &b).Add(Tokenizer::T_KEYWORD_GROUP, &c).Add(Tokenizer::T_KEYWORD_GROUP, &d).Throw();
			}

			t.Next();
		}

		if(!profile->id.empty())
		{
			for(StatProfile* s : stat_profiles)
			{
				if(s->id == profile->id)
					t.Throw("Profile with that id already exists.");
			}
		}

		if(result)
			*result = profile;
		stat_profiles.push_back(profile);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!profile->id.empty())
			ERROR(Format("Failed to load profile '%s': %s", profile->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load profile: %s", e.ToString()));
		delete profile;
		return false;
	}
}

//=================================================================================================
void AddItem(ItemScript* script, Tokenizer& t, CRC32& crc)
{
	if(!t.IsSymbol('!'))
	{
		const string& s = t.MustGetItemKeyword();
		const Item* item = FindItem(s.c_str(), false);
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
			if(c >= '1' && c <= '9')
			{
				int mod = c - '0';
				if(minus)
					mod = -mod;
				t.NextChar();
				t.Next();
				const string& s = t.MustGetItemKeyword();
				ItemListResult lis = FindItemList(s.c_str(), false);
				if(lis.lis)
				{
					if(lis.is_leveled)
					{
						script->code.push_back(PS_LEVELED_LIST_MOD);
						script->code.push_back(mod);
						script->code.push_back((int)lis.llis);
						crc.Update(PS_LEVELED_LIST_MOD);
						crc.Update(mod);
						crc.Update(lis.llis->id);
					}
					else
						t.Throw("Can't use mod on non leveled list '%s'.", s.c_str());
				}
				else
					t.Throw("Missing item list '%s'.", s.c_str());
			}
			else
				t.Throw("Invalid leveled list mod '%c'.", c);
		}
		else
		{
			const string& s = t.MustGetItemKeyword();
			ItemListResult lis = FindItemList(s.c_str(), false);
			if(lis.lis)
			{
				ParseScript type = (lis.is_leveled ? PS_LEVELED_LIST : PS_LIST);
				script->code.push_back(type);
				script->code.push_back((int)lis.lis);
				crc.Update(type);
				crc.Update(lis.GetIdString());
			}
			else
				t.Throw("Missing item list '%s'.", s.c_str());
		}
	}
}

enum IfState
{
	IFS_START,
	IFS_START_INLINE,
	IFS_ELSE,
	IFS_ELSE_INLINE
};

//=================================================================================================
bool LoadItems(Tokenizer& t, CRC32& crc, ItemScript** result = nullptr)
{
	ItemScript* script = new ItemScript;
	vector<IfState> if_state;
	bool done_if = false;

	try
	{
		if(!result)
		{
			// not inline, id
			script->id = t.MustGetItemKeyword();
			crc.Update(script->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

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
								t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &g).Add(Tokenizer::T_KEYWORD, &b, &g).Throw();
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
					if_state.pop_back();
					t.Next();
					while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
					{
						script->code.push_back(PS_END_IF);
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
				AddItem(script, t, crc);
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
							AddItem(script, t, crc);
							t.Next();
							AddItem(script, t, crc);
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
							AddItem(script, t, crc);
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
							AddItem(script, t, crc);
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
							t.Throw("Invalid random count (%d %d).", from, to);

						// get item
						script->code.push_back(PS_RANDOM);
						script->code.push_back(from);
						script->code.push_back(to);
						crc.Update(PS_RANDOM);
						crc.Update(from);
						crc.Update(to);
						AddItem(script, t, crc);
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
							t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &g).Add(Tokenizer::T_KEYWORD, &b, &g).Throw();
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
				AddItem(script, t, crc);
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

		if(!script->id.empty())
		{
			for(ItemScript* is : item_scripts)
			{
				if(is->id == script->id)
					t.Throw("Item script with that id already exists.");
			}
		}

		item_scripts.push_back(script);
		if(result)
			*result = script;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!script->id.empty())
			ERROR(Format("Failed to load item script '%s': %s", script->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load item script: %s", e.ToString()));
		delete script;
		return false;
	}
}

//=================================================================================================
bool LoadSpells(Tokenizer& t, CRC32& crc, SpellList** result = nullptr)
{
	SpellList* spell_list = new SpellList;

	try
	{
		if(!result)
		{
			// not inline, id
			spell_list->id = t.MustGetItemKeyword();
			crc.Update(spell_list->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

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
					spell_list->have_non_combat = t.MustGetBool();
					crc.Update(spell_list->have_non_combat ? 2 : 1);
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
				spell_list->spell[index] = spell;
				spell_list->level[index] = lvl;
				crc.Update(spell->id);
				crc.Update(lvl);
				++index;
			}
			t.Next();
		} while(!t.IsSymbol('}'));

		if(spell_list->spell[0] == nullptr && spell_list->spell[1] == nullptr && spell_list->spell[2] == nullptr)
			t.Throw("Empty spell list.");

		if(!spell_list->id.empty())
		{
			for(SpellList* sl : spell_lists)
			{
				if(sl->id == spell_list->id)
					t.Throw("Spell list with that id already exists.");
			}
		}

		spell_lists.push_back(spell_list);
		if(result)
			*result = spell_list;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!spell_list->id.empty())
			ERROR(Format("Failed to load spell list '%s': %s", spell_list->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load spell list: %s", e.ToString()));
		delete spell_list;
		return false;
	}
}

//=================================================================================================
bool LoadSounds(Tokenizer& t, CRC32& crc, SoundPack** result = nullptr)
{
	SoundPack* sound = new SoundPack;

	try
	{
		if(!result)
		{
			// not inline, id
			sound->id = t.MustGetItemKeyword();
			crc.Update(sound->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		while(!t.IsSymbol('}'))
		{
			SoundType type = (SoundType)t.MustGetKeywordId(G_SOUND_TYPE);
			t.Next();

			sound->filename[(int)type] = t.MustGetString();
			crc.Update(type);
			crc.Update(sound->filename[(int)type]);
			t.Next();
		}

		// check if not exists
		if(!sound->id.empty())
		{
			for(SoundPack* s : sound_packs)
			{
				if(s->id == sound->id)
					t.Throw("Sound pack with that id already exists.");
			}
		}

		if(result)
			*result = sound;
		sound_packs.push_back(sound);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!sound->id.empty())
			ERROR(Format("Failed to load sound pack '%s': %s", sound->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load sound pack: %s", e.ToString()));
		delete sound;
		return false;
	}
}

//=================================================================================================
bool LoadFrames(Tokenizer& t, CRC32& crc, FrameInfo** result = nullptr)
{
	FrameInfo* frame = new FrameInfo;

	try
	{
		if(!result)
		{
			// not inline, id
			frame->id = t.MustGetItemKeyword();
			crc.Update(frame->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		bool have_simple = false;

		while(!t.IsSymbol('}'))
		{
			FrameKeyword k = (FrameKeyword)t.MustGetKeywordId(G_FRAME_KEYWORD);
			t.Next();

			switch(k)
			{
			case FK_ATTACKS:
				if(have_simple || frame->extra)
					t.Throw("Frame info already have attacks information.");
				t.AssertSymbol('{');
				t.Next();
				frame->extra = new AttackFrameInfo;
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

					frame->extra->e.push_back({ start, end, flags });
					t.Next();
				} while(!t.IsSymbol('}'));
				frame->attacks = frame->extra->e.size();
				crc.Update(frame->attacks);
				break;
			case FK_SIMPLE_ATTACKS:
				if(have_simple || frame->extra)
					t.Throw("Frame info already have attacks information.");
				else
				{
					t.AssertSymbol('{');
					t.Next();
					crc.Update(1);
					int index = 0;
					frame->attacks = 0;
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

						frame->t[F_ATTACK1_START + index * 2] = start;
						frame->t[F_ATTACK1_END + index * 2] = end;
						++index;
						++frame->attacks;
						t.Next();
					} while(!t.IsSymbol('}'));
				}
				break;
			case FK_CAST:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid cast frame time %g.", f);
					frame->t[F_CAST] = f;
					crc.Update(2);
					crc.Update(f);
				}
				break;
			case FK_TAKE_WEAPON:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid take weapon frame time %g.", f);
					frame->t[F_TAKE_WEAPON] = f;
					crc.Update(3);
					crc.Update(f);
				}
				break;
			case FK_BASH:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid bash frame time %g.", f);
					frame->t[F_BASH] = f;
					crc.Update(4);
					crc.Update(f);
				}
				break;
			}

			t.Next();
		}

		// check if not exists
		if(!frame->id.empty())
		{
			for(FrameInfo* s : frame_infos)
			{
				if(s->id == frame->id)
					t.Throw("Frame info with that id already exists.");
			}
		}

		frame_infos.push_back(frame);
		if(result)
			*result = frame;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!frame->id.empty())
			ERROR(Format("Failed to load frame info '%s': %s", frame->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load frame info: %s", frame->id.c_str(), e.ToString()));
		delete frame;
		return false;
	}
}

//=================================================================================================
bool LoadTex(Tokenizer& t, CRC32& crc, TexPack** result = nullptr)
{
	TexPack* tex = new TexPack;

	try
	{
		if(!result)
		{
			// not inline, id
			tex->id = t.MustGetItemKeyword();
			crc.Update(tex->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		do
		{
			if(t.IsKeywordGroup(G_NULL))
			{
				tex->textures.push_back(TexId(nullptr));
				crc.Update(0);
			}
			else
			{
				const string& s = t.MustGetString();
				tex->textures.push_back(TexId(s));
				crc.Update(s);
			}
			t.Next();
		} while(!t.IsSymbol('}'));

		bool any = false;
		for(TexId& ti : tex->textures)
		{
			if(!ti.id.empty())
			{
				any = true;
				break;
			}
		}
		if(!any)
			t.Throw("Texture pack without textures.");

		if(!tex->id.empty())
		{
			for(TexPack* tp : tex_packs)
			{
				if(tp->id == tex->id)
					t.Throw("Texture pack with that id already exists.");
			}
		}

		tex_packs.push_back(tex);
		if(result)
			*result = tex;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!tex->id.empty())
			ERROR(Format("Failed to load texture pack '%s': %s", tex->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load texture pack: %s", e.ToString()));
		delete tex;
		return false;
	}
}

//=================================================================================================
bool LoadIdles(Tokenizer& t, CRC32& crc, IdlePack** result = nullptr)
{
	IdlePack* idle = new IdlePack;

	try
	{
		if(!result)
		{
			// not inline, id
			idle->id = t.MustGetItemKeyword();
			crc.Update(idle->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();
		
		do
		{
			const string& s = t.MustGetString();
			idle->anims.push_back(s);
			crc.Update(s);
			t.Next();
		} while(!t.IsSymbol('}'));
		
		if(!idle->id.empty())
		{
			for(IdlePack* ip : idle_packs)
			{
				if(ip->id == idle->id)
					t.Throw("Idle pack with this id already exists.");
			}
		}

		idle_packs.push_back(idle);
		if(result)
			*result = idle;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!idle->id.empty())
			ERROR(Format("Failed to load idles '%s': %s", idle->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load idles: %s", e.ToString()));
		delete idle;
		return false;
	}
}

//=================================================================================================
bool LoadUnit(Tokenizer& t, CRC32& crc)
{
	UnitData* unit = new UnitData;

	try
	{
		// id
		unit->id = t.MustGetItemKeyword();
		crc.Update(unit->id);
		t.Next();

		// parent unit
		if(t.IsSymbol(':'))
		{
			t.Next();
			const string& s = t.MustGetItemKeyword();
			UnitData* parent = nullptr;
			for(UnitData* ud : unit_datas)
			{
				if(ud->id == s)
				{
					parent = ud;
					break;
				}
			}
			if(!parent)
				t.Throw("Missing parent unit '%s'.", s.c_str());
			crc.Update(s);
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
					crc.Update(unit->level);
				}
				else
				{
					int lvl = t.MustGetInt();
					if(lvl < 0)
						t.Throw("Invalid level %d.", lvl);
					unit->level = INT2(lvl);
					crc.Update(lvl);
				}
				break;
			case P_PROFILE:
				if(t.IsSymbol('{'))
				{
					if(!LoadProfile(t, crc, &unit->stat_profile))
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
				unit->hp_bonus = t.MustGetInt();
				if(unit->hp_bonus < 0)
					t.Throw("Invalid hp bonus %d.", unit->hp_bonus);
				crc.Update(unit->hp_bonus);
				break;
			case P_DEF:
				unit->def_bonus = t.MustGetInt();
				if(unit->def_bonus < 0)
					t.Throw("Invalid def bonus %d.", unit->def_bonus);
				crc.Update(unit->def_bonus);
				break;
			case P_ITEMS:
				if(t.IsSymbol('{'))
				{
					if(LoadItems(t, crc, &unit->item_script))
					{
						unit->items = &unit->item_script->code[0];
						unit->new_items = true;
					}
					else
						t.Throw("Failed to load inline item script.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->item_script = nullptr;
					for(ItemScript* s : item_scripts)
					{
						if(s->id == id)
						{
							unit->item_script = s;
							unit->new_items = true;
							break;
						}
					}
					if(unit->item_script)
						unit->items = &unit->item_script->code[0];
					else
						t.Throw("Missing item script '%s'.", id.c_str());
					crc.Update(id);
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
						crc.Update(unit->gold);
						crc.Update(unit->gold2);
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
						crc.Update(unit->gold);
					}
				}
				else
				{
					int gold = t.MustGetInt();
					if(gold < 0)
						t.Throw("Invalid gold count %d.", gold);
					unit->gold = unit->gold2 = INT2(gold);
					crc.Update(gold);
				}
				break;
			case P_DIALOG:
				{
					const string& s = t.MustGetItemKeyword();
					auto it = dialogs_map.find(s);
					if(it == dialogs_map.end())
						t.Throw("Missing dialog '%s'.", s.c_str());
					unit->dialog = it->second;
					crc.Update(s);
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
					crc.Update(id);
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
					crc.Update(id);
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
					crc.Update(id);
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

		// add
		std::pair<UnitDataIterator, bool>& result = unit_datas.insert(unit);
		if(!result.second)
			t.Throw("Unit with that id already exists.");
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load unit data '%s': %s", unit->id.c_str(), e.ToString()));
		delete unit;
		return false;
	}
}

//=================================================================================================
bool LoadAlias(Tokenizer& t, CRC32& crc)
{
	try
	{
		const string& id = t.MustGetItemKeyword();
		UnitData* ud = FindUnitData(id.c_str(), false);
		if(!ud)
			t.Throw("Missing unit data '%s'.", id.c_str());
		crc.Update(id);
		t.Next();

		const string& alias = t.MustGetItemKeyword();
		UnitData* ud2 = FindUnitData(alias.c_str(), false);
		if(ud2)
			t.Throw("Can't create alias '%s', already exists.", alias.c_str());
		crc.Update(alias);

		unit_aliases[alias] = ud;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load unit alias: %s", e.ToString()));
		return false;
	}
}

//=================================================================================================
bool LoadGroup(Tokenizer& t, CRC32& crc)
{
	UnitGroup* group = new UnitGroup;

	try
	{
		group->id = t.MustGetItemKeyword();
		group->total = 0;
		if(FindUnitGroup(group->id))
			t.Throw("Group with that id already exists.");
		crc.Update(group->id);
		t.Next();

		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeyword(GK_GROUP, G_GROUP_KEYWORD))
			{
				t.Next();
				const string& id = t.MustGetItemKeyword();
				UnitGroup* other_group = FindUnitGroup(id);
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
				UnitData* ud = FindUnitData(id.c_str(), false);
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

		unit_groups.push_back(group);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		cstring str;
		if(!group->id.empty())
			str = Format("Failed to load unit group '%s': %s", group->id.c_str(), e.ToString());
		else
			str = Format("Failed to load unit group: %s", group->id.c_str(), e.ToString());
		ERROR(str);
		delete group;
		return false;
	}
}

//=================================================================================================
void LoadUnits(uint& out_crc)
{
	Tokenizer t(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS);
	if(!t.FromFile(Format("%s/units.txt", g_system_dir.c_str())))
		throw "Failed to open units.txt.";

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
		{ "update_v0_items", F2_UPDATE_V0_ITEMS },
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

	dialogs_map["dialog_kowal"] = dialog_kowal;
	dialogs_map["dialog_kupiec"] = dialog_kupiec;
	dialogs_map["dialog_alchemik"] = dialog_alchemik;
	dialogs_map["dialog_burmistrz"] = dialog_burmistrz;
	dialogs_map["dialog_mieszkaniec"] = dialog_mieszkaniec;
	dialogs_map["dialog_widz"] = dialog_widz;
	dialogs_map["dialog_straznik"] = dialog_straznik;
	dialogs_map["dialog_trener"] = dialog_trener;
	dialogs_map["dialog_dowodca_strazy"] = dialog_dowodca_strazy;
	dialogs_map["dialog_karczmarz"] = dialog_karczmarz;
	dialogs_map["dialog_urzednik"] = dialog_urzednik;
	dialogs_map["dialog_mistrz_areny"] = dialog_mistrz_areny;
	dialogs_map["dialog_hero"] = dialog_hero;
	dialogs_map["dialog_hero_przedmiot"] = dialog_hero_przedmiot;
	dialogs_map["dialog_hero_przedmiot_kup"] = dialog_hero_przedmiot_kup;
	dialogs_map["dialog_hero_pvp"] = dialog_hero_pvp;
	dialogs_map["dialog_szalony"] = dialog_szalony;
	dialogs_map["dialog_szalony_przedmiot"] = dialog_szalony_przedmiot;
	dialogs_map["dialog_szalony_przedmiot_kup"] = dialog_szalony_przedmiot_kup;
	dialogs_map["dialog_szalony_pvp"] = dialog_szalony_pvp;
	dialogs_map["dialog_bandyci"] = dialog_bandyci;
	dialogs_map["dialog_bandyta"] = dialog_bandyta;
	dialogs_map["dialog_szalony_mag"] = dialog_szalony_mag;
	dialogs_map["dialog_porwany"] = dialog_porwany;
	dialogs_map["dialog_straznicy_nagroda"] = dialog_straznicy_nagroda;
	dialogs_map["dialog_zadanie"] = dialog_zadanie;
	dialogs_map["dialog_artur_drwal"] = dialog_artur_drwal;
	dialogs_map["dialog_drwal"] = dialog_drwal;
	dialogs_map["dialog_inwestor"] = dialog_inwestor;
	dialogs_map["dialog_gornik"] = dialog_gornik;
	dialogs_map["dialog_pijak"] = dialog_pijak;
	dialogs_map["dialog_q_bandyci"] = dialog_q_bandyci;
	dialogs_map["dialog_q_magowie"] = dialog_q_magowie;
	dialogs_map["dialog_q_magowie2"] = dialog_q_magowie2;
	dialogs_map["dialog_q_orkowie"] = dialog_q_orkowie;
	dialogs_map["dialog_q_orkowie2"] = dialog_q_orkowie2;
	dialogs_map["dialog_q_gobliny"] = dialog_q_gobliny;
	dialogs_map["dialog_q_zlo"] = dialog_q_zlo;
	dialogs_map["dialog_tut_czlowiek"] = dialog_tut_czlowiek;
	dialogs_map["dialog_q_szaleni"] = dialog_q_szaleni;
	dialogs_map["dialog_szaleni"] = dialog_szaleni;
	dialogs_map["dialog_tomashu"] = dialog_tomashu;
	dialogs_map["dialog_ochroniarz"] = dialog_ochroniarz;
	dialogs_map["dialog_mag_obstawa"] = dialog_mag_obstawa;
	dialogs_map["dialog_sprzedawca_jedzenia"] = dialog_sprzedawca_jedzenia;

	int errors = 0;
	CRC32 crc;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_TYPE))
			{
				Type type = (Type)t.MustGetKeywordId(G_TYPE);
				t.Next();

				bool ok = true;

				switch(type)
				{
				case T_UNIT:
					if(!LoadUnit(t, crc))
						ok = false;
					break;
				case T_PROFILE:
					if(!LoadProfile(t, crc))
						ok = false;
					break;
				case T_ITEMS:
					if(!LoadItems(t, crc))
						ok = false;
					break;
				case T_SPELLS:
					if(!LoadSpells(t, crc))
						ok = false;
					break;
				case T_SOUNDS:
					if(!LoadSounds(t, crc))
						ok = false;
					break;
				case T_FRAMES:
					if(!LoadFrames(t, crc))
						ok = false;
					break;
				case T_TEX:
					if(!LoadTex(t, crc))
						ok = false;
					break;
				case T_IDLES:
					if(!LoadIdles(t, crc))
						ok = false;
					break;
				case T_ALIAS:
					if(!LoadAlias(t, crc))
						ok = false;
					break;
				case T_GROUP:
					if(!LoadGroup(t, crc))
						ok = false;
					break;
				default:
					ERROR(Format("Invalid type %d.", type));
					ok = false;
					break;
				}

				if(!ok)
				{
					++errors;
					skip = true;
				}
			}
			else
			{
				int group = G_TYPE;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD_GROUP, &group));
				++errors;
				skip = true;
			}

			if(skip)
				t.SkipToKeywordGroup(G_TYPE);
			else
				t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load items: %s", e.ToString()));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load units (%d errors), check log for details.", errors);

	out_crc = crc.Get();
}

//=================================================================================================
void CleanupUnits()
{
	DeleteElements(stat_profiles);
	DeleteElements(item_scripts);
	DeleteElements(spell_lists);
	DeleteElements(sound_packs);
	DeleteElements(idle_packs);
	DeleteElements(tex_packs);
	DeleteElements(frame_infos);
	for(UnitData* ud : unit_datas)
		delete ud;
	DeleteElements(unit_groups);
}

//=================================================================================================
#define S(x) (*(cstring*)(x))
void CheckItem(const int*& ps, string& errors, uint& count, bool is_new)
{
	if(!is_new)
	{
		ItemListResult lis;
		const Item* item = FindItem(S(ps), false, &lis);
		if(!item && !lis.lis)
		{
			errors += Format("\tMissing item '%s'.\n", S(ps));
			++count;
		}
	}
	else
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		if(type == PS_ITEM || type == PS_LIST || type == PS_LEVELED_LIST)
		{
			if(*ps == 0)
			{
				errors += Format("\tMissing new item value %p.\n", *ps);
				++count;
			}
		}
		else if(type == PS_LEVELED_LIST_MOD)
		{
			int count = *ps;
			if(count == 0 || count > 9 || count < -9)
			{
				errors += Format("\tInvalid leveled list mod %d.\n", count);
				++count;
			}
			++ps;
			if(*ps == 0)
			{
				errors += Format("\tMissing leveled list value %p.\n", *ps);
				++count;
			}
		}
		else
		{
			errors += Format("\tMissing new item declaration.\n");
			++count;
		}
	}

	++ps;
}

//=================================================================================================
void TestItemScript(const int* script, string& errors, uint& count, bool is_new, uint& crc)
{
	assert(script);

	const int* ps = script;
	int a, b, depth = 0, elsel = 0;
	crc = 0;

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ONE:
			CheckItem(ps, errors, count, is_new);
			crc += 1;
			break;
		case PS_ONE_OF_MANY:
			if((a = *ps) == 0)
			{
				errors += "\tOne from many: 0.\n";
				++count;
			}
			else
			{
				crc += (a << 2);
				++ps;
				for(int i = 0; i<a; ++i)
					CheckItem(ps, errors, count, is_new);
			}
			break;
		case PS_CHANCE:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tChance %d.\n", a);
				++count;
			}
			crc += (a << 6);
			++ps;
			CheckItem(ps, errors, count, is_new);
			break;
		case PS_CHANCE2:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tChance2 %d.\n", a);
				++count;
			}
			++ps;
			crc += (a << 10);
			CheckItem(ps, errors, count, is_new);
			CheckItem(ps, errors, count, is_new);
			break;
		case PS_IF_CHANCE:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tIf chance %d.\n", a);
				++count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			crc += (a << 14);
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			a = *ps;
			if(a <= 1)
			{
				errors += Format("\tLevel %d.\n", a);
				++count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			crc += (a << 18);
			++depth;
			++ps;
			break;
		case PS_ELSE:
			if(depth == 0)
			{
				errors += "\tElse without if.\n";
				++count;
			}
			else
			{
				if(IS_SET(elsel, BIT(depth - 1)))
				{
					errors += "\tNext else.\n";
					++count;
				}
				else
					SET_BIT(elsel, BIT(depth - 1));
			}
			crc += (1 << 22);
			break;
		case PS_END_IF:
			if(depth == 0)
			{
				errors += "\tEnd if without if.\n";
				++count;
			}
			else
				--depth;
			crc += (1 << 23);
			break;
		case PS_MANY:
			a = *ps;
			if(a < 0)
			{
				errors += Format("\tGive some %d.\n", a);
				++count;
			}
			++ps;
			CheckItem(ps, errors, count, is_new);
			crc += (a << 24);
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			if(a < 0 || b < 0 || a >= b)
			{
				errors += Format("\tGive random %d, %d.", a, b);
				++count;
			}
			CheckItem(ps, errors, count, is_new);
			crc += (a << 28);
			crc += (b << 30);
			break;
		}
	}

	if(depth != 0)
	{
		errors += Format("\tUnclosed ifs %d.\n", depth);
		++count;
	}
}

//=================================================================================================
void LogItem(string& s, const int*& ps, bool is_new)
{
	if(!is_new)
		s += S(ps);
	else
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ITEM:
			s += ((const Item*)(*ps))->id;
			break;
		case PS_LIST:
			s += Format("!%s", ((ItemList*)(*ps))->id.c_str());
			break;
		case PS_LEVELED_LIST:
			s += Format("!%s", ((LeveledItemList*)(*ps))->id.c_str());
			break;
		case PS_LEVELED_LIST_MOD:
			{
				int mod = *ps;
				++ps;
				s += Format("!%+d%s", mod, ((LeveledItemList*)(*ps))->id.c_str());
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	++ps;
}

//=================================================================================================
void LogItemScript(const int* script, bool is_new)
{
	assert(script);

	const int* ps = script;
	int a, b, depth = 0, elsel = 0;
	string s = "Item script:\n";

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		for(int i = 0; i < depth; ++i)
			s += "  ";

		switch(type)
		{
		case PS_ONE:
			s += "one ";
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_ONE_OF_MANY:
			a = *ps;
			++ps;
			s += Format("one_of_many %d [", a);
			for(int i = 0; i<a; ++i)
			{
				LogItem(s, ps, is_new);
				s += "; ";
			}
			s += "]\n";
			break;
		case PS_CHANCE:
			a = *ps;
			++ps;
			s += Format("chance %d ", a);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			s += Format("chance2 %d [", a);
			LogItem(s, ps, is_new);
			s += "; ";
			LogItem(s, ps, is_new);
			s += "]\n";
			break;
		case PS_IF_CHANCE:
			a = *ps;
			s += Format("if chance %d\n", a);
			CLEAR_BIT(elsel, BIT(depth));
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			a = *ps;
			CLEAR_BIT(elsel, BIT(depth));
			s += Format("if level %d\n", a);
			++depth;
			++ps;
			break;
		case PS_ELSE:
			SET_BIT(elsel, BIT(depth - 1));
			s.pop_back();
			s.pop_back();
			s += "else\n";
			break;
		case PS_END_IF:
			s.pop_back();
			s.pop_back();
			s += "end_if\n";
			--depth;
			break;
		case PS_MANY:
			a = *ps;
			++ps;
			s += Format("many %d ", a);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			s += Format("random %d %d ", a, b);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		}
	}

	LOG(s.c_str());
}

//=================================================================================================
UnitData* FindUnitData(cstring id, bool report)
{
	assert(id);

	unit_data_search.id = id;
	auto it = unit_datas.find(&unit_data_search);
	if(it != unit_datas.end())
		return *it;

	auto it2 = unit_aliases.find(id);
	if(it2 != unit_aliases.end())
		return it2->second;

	if(report)
		throw Format("Can't find unit data '%s'!", id);

	return nullptr;
}
