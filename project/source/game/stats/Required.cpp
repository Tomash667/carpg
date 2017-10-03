#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "Spell.h"
#include "BuildingGroup.h"
#include "Building.h"
#include "BuildingScript.h"
#include "BaseUsable.h"

extern string g_system_dir;

enum RequiredType
{
	R_ITEM,
	R_LIST,
	R_STOCK,
	R_UNIT,
	R_GROUP,
	R_SPELL,
	R_DIALOG,
	R_BUILDING_GROUP,
	R_BUILDING,
	R_BUILDING_SCRIPT,
	R_OBJECT,
	R_USABLE
};

//=================================================================================================
void CheckStartItems(Skill skill, bool required, uint& errors)
{
	bool have_0 = !required, have_heirloom = false;

	for(StartItem& si : StartItem::start_items)
	{
		if(si.skill == skill)
		{
			if(si.value == 0)
				have_0 = true;
			else if(si.value == HEIRLOOM)
				have_heirloom = true;
			if(have_0 && have_heirloom)
				return;
		}
	}

	if(!have_0)
	{
		Error("Missing starting item for skill %s.", g_skills[(int)skill].id);
		++errors;
	}
	if(!have_heirloom)
	{
		Error("Missing heirloom item for skill %s.", g_skills[(int)skill].id);
		++errors;
	}
}

//=================================================================================================
void CheckBaseItem(cstring name, int num, uint& errors)
{
	if(num == 0)
	{
		Error("Missing base %s.", name);
		++errors;
	}
	else if(num > 1)
	{
		Error("Multiple base %ss (%d).", name, num);
		++errors;
	}
}

//=================================================================================================
void CheckBaseItems(uint& errors)
{
	int have_short_blade = 0,
		have_long_blade = 0,
		have_axe = 0,
		have_blunt = 0,
		have_wand = 0,
		have_bow = 0,
		have_shield = 0,
		have_light_armor = 0,
		have_medium_armor = 0,
		have_heavy_armor = 0,
		have_mage_armor = 0;
	const ItemList* lis = FindItemList("base_items").lis;

	for(const Item* item : lis->items)
	{
		if(item->type == IT_WEAPON)
		{
			if(IS_SET(item->flags, ITEM_MAGE))
				++have_wand;
			else
			{
				switch(item->ToWeapon().weapon_type)
				{
				case WT_SHORT:
					++have_short_blade;
					break;
				case WT_LONG:
					++have_long_blade;
					break;
				case WT_AXE:
					++have_axe;
					break;
				case WT_MACE:
					++have_blunt;
					break;
				}
			}
		}
		else if(item->type == IT_BOW)
			++have_bow;
		else if(item->type == IT_SHIELD)
			++have_shield;
		else if(item->type == IT_ARMOR)
		{
			if(IS_SET(item->flags, ITEM_MAGE))
				++have_mage_armor;
			else
			{
				switch(item->ToArmor().skill)
				{
				case Skill::LIGHT_ARMOR:
					++have_light_armor;
					break;
				case Skill::MEDIUM_ARMOR:
					++have_medium_armor;
					break;
				case Skill::HEAVY_ARMOR:
					++have_heavy_armor;
					break;
				}
			}
		}
	}

	CheckBaseItem("short blade weapon", have_short_blade, errors);
	CheckBaseItem("long blade weapon", have_long_blade, errors);
	CheckBaseItem("axe weapon", have_axe, errors);
	CheckBaseItem("blunt weapon", have_blunt, errors);
	CheckBaseItem("mage weapon", have_wand, errors);
	CheckBaseItem("bow", have_bow, errors);
	CheckBaseItem("shield", have_shield, errors);
	CheckBaseItem("light armor", have_light_armor, errors);
	CheckBaseItem("medium armor", have_medium_armor, errors);
	CheckBaseItem("heavy armor", have_heavy_armor, errors);
	CheckBaseItem("mage armor", have_mage_armor, errors);
}

//=================================================================================================
bool Game::LoadRequiredStats(uint& errors)
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/required.txt", g_system_dir.c_str())))
	{
		Error("Failed to open required.txt.");
		++errors;
		return false;
	}

	t.AddKeywords(0, {
		{ "item", R_ITEM },
		{ "list", R_LIST },
		{ "stock", R_STOCK },
		{ "unit", R_UNIT },
		{ "group", R_GROUP },
		{ "spell", R_SPELL },
		{ "dialog", R_DIALOG },
		{ "building_group", R_BUILDING_GROUP },
		{ "building", R_BUILDING },
		{ "building_script", R_BUILDING_SCRIPT },
		{ "object", R_OBJECT },
		{ "usable", R_USABLE }
	});

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			try
			{
				RequiredType type = (RequiredType)t.MustGetKeywordId(0);
				t.Next();

				const string& str = t.MustGetItemKeyword();

				switch(type)
				{
				case R_ITEM:
					{
						ItemListResult result;
						const Item* item = FindItem(str.c_str(), false, &result);
						if(!item)
						{
							Error("Missing required item '%s'.", str.c_str());
							++errors;
						}
						else if(result.lis)
						{
							Error("Required item '%s' is list.", str.c_str());
							++errors;
						}
					}
					break;
				case R_LIST:
					{
						ItemListResult result = FindItemList(str.c_str(), false);
						if(!result.lis)
						{
							Error("Missing required item list '%s'.", str.c_str());
							++errors;
						}
						else if(result.is_leveled)
						{
							Error("Required list '%s' is leveled.", str.c_str());
							++errors;
						}
						else if(result.lis->items.empty())
						{
							Error("Required list '%s' is empty.", str.c_str());
							++errors;
						}
					}
					break;
				case R_STOCK:
					{
						Stock* stock = FindStockScript(str.c_str());
						if(!stock)
						{
							Error("Missing required item stock '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_UNIT:
					{
						UnitData* ud = FindUnitData(str.c_str(), false);
						if(!ud)
						{
							Error("Missing required unit '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_GROUP:
					{
						bool need_leader = false;
						if(str == "with_leader")
						{
							need_leader = true;
							t.Next();
						}
						const string& group_id = t.MustGetItemKeyword();
						UnitGroup* group = FindUnitGroup(group_id);
						if(!group)
						{
							Error("Missing required unit group '%s'.", group_id.c_str());
							++errors;
						}
						else if(!group->leader && need_leader)
						{
							Error("Required unit group '%s' is missing leader.", group_id.c_str());
							++errors;
						}
					}
					break;
				case R_SPELL:
					{
						Spell* spell = FindSpell(str.c_str());
						if(!spell)
						{
							Error("Missing required spell '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_DIALOG:
					{
						GameDialog* dialog = FindDialog(str.c_str());
						if(!dialog)
						{
							Error("Missing required dialog '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_BUILDING_GROUP:
					{
						BuildingGroup* group = BuildingGroup::TryGet(str);
						if(!group)
						{
							Error("Missing required building group '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_BUILDING:
					{
						Building* building = Building::TryGet(str);
						if(!building)
						{
							Error("Missing required building '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_BUILDING_SCRIPT:
					{
						BuildingScript* script = BuildingScript::TryGet(str);
						if(!script)
						{
							Error("Missing required building script '%s'.", str.c_str());
							++errors;
							break;
						}

						t.Next();
						t.AssertSymbol('{');
						t.Next();

						while(!t.IsSymbol('}'))
						{
							const string& id = t.MustGetItem();
							if(!script->HaveBuilding(id))
							{
								Error("Missing required building '%s' for building script '%s'.", script->id.c_str(), id.c_str());
								++errors;
								break;
							}
							t.Next();
						}
					}
					break;
				case R_OBJECT:
					{
						auto obj = BaseObject::TryGet(str.c_str());
						if(!obj)
						{
							Error("Missing required object '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				case R_USABLE:
					{
						auto use = BaseUsable::TryGet(str.c_str());
						if(!use)
						{
							Error("Missing required usable object '%s'.", str.c_str());
							++errors;
						}
					}
					break;
				}

				t.Next();
			}
			catch(const Tokenizer::Exception& e)
			{
				Error("Parse error: %s", e.ToString());
				++errors;
				t.SkipToKeywordGroup(0);
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load required entities: %s", e.ToString());
		++errors;
	}

	CheckStartItems(Skill::SHORT_BLADE, true, errors);
	CheckStartItems(Skill::LONG_BLADE, true, errors);
	CheckStartItems(Skill::AXE, true, errors);
	CheckStartItems(Skill::BLUNT, true, errors);
	CheckStartItems(Skill::BOW, false, errors);
	CheckStartItems(Skill::SHIELD, false, errors);
	CheckStartItems(Skill::LIGHT_ARMOR, true, errors);
	CheckStartItems(Skill::MEDIUM_ARMOR, true, errors);
	CheckStartItems(Skill::HEAVY_ARMOR, true, errors);

	CheckBaseItems(errors);

	return true;
}
