#include "Pch.h"
#include "GameCore.h"
#include "ItemScript.h"
#include "StatProfile.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
vector<ItemScript*> ItemScript::scripts;
const LeveledItemList* ItemScript::weapon_list[WT_MAX], *ItemScript::armor_list[AT_MAX];

//=================================================================================================
void ItemScript::Init()
{
	weapon_list[WT_SHORT_BLADE] = ItemList::Get("short_blade").llis;
	weapon_list[WT_LONG_BLADE] = ItemList::Get("long_blade").llis;
	weapon_list[WT_AXE] = ItemList::Get("axe").llis;
	weapon_list[WT_BLUNT] = ItemList::Get("blunt").llis;
	armor_list[AT_LIGHT] = ItemList::Get("light_armor").llis;
	armor_list[AT_MEDIUM] = ItemList::Get("medium_armor").llis;
	armor_list[AT_HEAVY] = ItemList::Get("heavy_armor").llis;
}

//=================================================================================================
const LeveledItemList& ItemScript::GetSpecial(int special, int sub)
{
	SubprofileInfo s;
	s.value = sub;
	if(special == SPECIAL_WEAPON)
		return *weapon_list[s.weapon];
	else
	{
		assert(special == SPECIAL_ARMOR);
		return *armor_list[s.armor];
	}
}

//=================================================================================================
void ItemScript::CheckItem(const int*& ps, string& errors, uint& count)
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
void ItemScript::Test(string& errors, uint& count)
{
	const int* ps = code.data();
	int a, b, depth = 0, elsel = 0;

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ONE:
			CheckItem(ps, errors, count);
			break;
		case PS_ONE_OF_MANY:
			if((a = *ps) == 0)
			{
				errors += "\tOne from many: 0.\n";
				++count;
			}
			else
			{
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
			break;
		case PS_END_IF:
			if(depth == 0)
			{
				errors += "\tEnd if without if.\n";
				++count;
			}
			else
				--depth;
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
ItemScript* ItemScript::TryGet(Cstring id)
{
	for(auto script : scripts)
	{
		if(script->id == id)
			return script;
	}

	return nullptr;
}

//=================================================================================================
void ItemScript::Parse(Unit& unit) const
{
	const int* ps = code.data();
	int a, b, depth = 0, depth_if = 0;

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ONE:
			if(depth == depth_if)
				GiveItem(unit, ps, 1);
			else
				SkipItem(ps, 1);
			break;
		case PS_ONE_OF_MANY:
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				b = Rand() % a;
				for(int i = 0; i < a; ++i)
				{
					if(i == b)
						GiveItem(unit, ps, 1);
					else
						SkipItem(ps, 1);
				}
			}
			else
				SkipItem(ps, a);
			break;
		case PS_CHANCE:
			a = *ps;
			++ps;
			if(depth == depth_if && Rand() % 100 < a)
				GiveItem(unit, ps, 1);
			else
				SkipItem(ps, 1);
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				if(Rand() % 100 < a)
				{
					GiveItem(unit, ps, 1);
					SkipItem(ps, 1);
				}
				else
				{
					SkipItem(ps, 1);
					GiveItem(unit, ps, 1);
				}
			}
			else
				SkipItem(ps, 2);
			break;
		case PS_IF_CHANCE:
			a = *ps;
			if(depth == depth_if && Rand() % 100 < a)
				++depth_if;
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			if(depth == depth_if && unit.level >= *ps)
				++depth_if;
			++depth;
			++ps;
			break;
		case PS_ELSE:
			if(depth == depth_if)
				--depth_if;
			else if(depth == depth_if + 1)
				++depth_if;
			break;
		case PS_END_IF:
			if(depth == depth_if)
				--depth_if;
			--depth;
			break;
		case PS_MANY:
			a = *ps;
			++ps;
			if(depth == depth_if)
				GiveItem(unit, ps, a);
			else
				SkipItem(ps, 1);
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			a = Random(a, b);
			if(depth == depth_if && a > 0)
				GiveItem(unit, ps, a);
			else
				SkipItem(ps, 1);
			break;
		default:
			assert(0);
			break;
		}
	}
}

//=================================================================================================
void ItemScript::GiveItem(Unit& unit, const int*& ps, int count) const
{
	int type = *ps;
	++ps;
	switch(type)
	{
	case PS_ITEM:
		unit.AddItemAndEquipIfNone((const Item*)(*ps), count);
		break;
	case  PS_LIST:
		{
			const ItemList& lis = *(const ItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get());
		}
		break;
	case PS_LEVELED_LIST:
		{
			const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1)));
		}
		break;
	case PS_LEVELED_LIST_MOD:
		{
			int mod = *ps++;
			const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1) + mod));
		}
		break;
	case PS_SPECIAL_ITEM:
		{
			int special = *ps;
			const LeveledItemList& lis = ItemScript::GetSpecial(special, unit.stats->subprofile.value);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1)));
		}
		break;
	case PS_SPECIAL_ITEM_MOD:
		{
			int mod = *ps++;
			int special = *ps;
			const LeveledItemList& lis = ItemScript::GetSpecial(special, unit.stats->subprofile.value);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1) + mod));
		}
	}

	++ps;
}

//=================================================================================================
void ItemScript::SkipItem(const int*& ps, int count) const
{
	for(int i = 0; i < count; ++i)
	{
		int type = *ps;
		++ps;
		if(type == PS_LEVELED_LIST_MOD || type == PS_SPECIAL_ITEM_MOD)
			++ps;
		++ps;
	}
}
