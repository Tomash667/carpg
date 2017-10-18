#include "Pch.h"
#include "Core.h"
#include "UnitData.h"
#include "ItemScript.h"
#include "Dialog.h"
#include "Spell.h"
#include "Item.h"
#include "Crc.h"
#include "Content.h"

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
}

//=================================================================================================
void CheckItem(const int*& ps, string& errors, uint& count)
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

	++ps;
}

//=================================================================================================
void TestItemScript(const int* script, string& errors, uint& count, uint& crc)
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
			CheckItem(ps, errors, count);
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
				for(int i = 0; i < a; ++i)
					CheckItem(ps, errors, count);
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
			CheckItem(ps, errors, count);
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
			CheckItem(ps, errors, count);
			CheckItem(ps, errors, count);
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
			CheckItem(ps, errors, count);
			crc += (a << 24);
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			if(a < 0 || b < 0 || a >= b)
			{
				errors += Format("\tGive Random %d, %d.", a, b);
				++count;
			}
			CheckItem(ps, errors, count);
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
void LogItem(string& s, const int*& ps)
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

	++ps;
}

//=================================================================================================
void LogItemScript(const int* script)
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
			LogItem(s, ps);
			s += "\n";
			break;
		case PS_ONE_OF_MANY:
			a = *ps;
			++ps;
			s += Format("one_of_many %d [", a);
			for(int i = 0; i < a; ++i)
			{
				LogItem(s, ps);
				s += "; ";
			}
			s += "]\n";
			break;
		case PS_CHANCE:
			a = *ps;
			++ps;
			s += Format("chance %d ", a);
			LogItem(s, ps);
			s += "\n";
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			s += Format("chance2 %d [", a);
			LogItem(s, ps);
			s += "; ";
			LogItem(s, ps);
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
			LogItem(s, ps);
			s += "\n";
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			s += Format("random %d %d ", a, b);
			LogItem(s, ps);
			s += "\n";
			break;
		}
	}

	Info(s.c_str());
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

//=================================================================================================
UnitGroup* FindUnitGroup(const AnyString& id)
{
	for(UnitGroup* group : unit_groups)
	{
		if(group->id == id.s)
			return group;
	}
	return nullptr;
}
