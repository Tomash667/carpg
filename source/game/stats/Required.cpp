#include "Pch.h"
#include "Base.h"
#include "Game.h"

extern string g_system_dir;

enum RequiredType
{
	R_ITEM,
	R_LIST,
	R_STOCK,
	R_UNIT,
	R_GROUP,
	R_SPELL,
	R_DIALOG
};

//=================================================================================================
void CheckStartItems(Skill skill, bool required, int& errors)
{
	bool have_0 = !required, have_heirloom = false;

	for(StartItem& si : start_items)
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
		ERROR(Format("Missing starting item for skill %s.", g_skills[(int)skill].id));
		++errors;
	}
	if(!have_heirloom)
	{
		ERROR(Format("Missing heirloom item for skill %s.", g_skills[(int)skill].id));
		++errors;
	}
}

//=================================================================================================
void CheckBaseItem(cstring name, int num, int& errors)
{
	if(num == 0)
	{
		ERROR(Format("Missing base %s.", name));
		++errors;
	}
	else if(num > 1)
	{
		ERROR(Format("Multiple base %ss (%d).", name, num));
		++errors;
	}
}

//=================================================================================================
void CheckBaseItems(int& errors)
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
void Game::LoadRequiredStats()
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/required.txt", g_system_dir.c_str())))
		throw "Failed to open required.txt.";

	t.AddKeywords(0, {
		{ "item", R_ITEM },
		{ "list", R_LIST },
		{ "stock", R_STOCK },
		{ "unit", R_UNIT },
		{ "group", R_GROUP },
		{ "spell", R_SPELL },
		{ "dialog", R_DIALOG }
	});

	int errors = 0;

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
							ERROR(Format("Missing required item '%s'.", str.c_str()));
							++errors;
						}
						else if(result.lis)
						{
							ERROR(Format("Required item '%s' is list.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_LIST:
					{
						ItemListResult result = FindItemList(str.c_str(), false);
						if(!result.lis)
						{
							ERROR(Format("Missing required item list '%s'.", str.c_str()));
							++errors;
						}
						else if(result.is_leveled)
						{
							ERROR(Format("Required list '%s' is leveled.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_STOCK:
					{
						Stock* stock = FindStockScript(str.c_str());
						if(!stock)
						{
							ERROR(Format("Missing required item stock '%s'.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_UNIT:
					{
						UnitData* ud = FindUnitData(str.c_str(), false);
						if(!ud)
						{
							ERROR(Format("Missing required unit '%s'.", str.c_str()));
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
							ERROR(Format("Missing required unit group '%s'.", group_id.c_str()));
							++errors;
						}
						else if(!group->leader && need_leader)
						{
							ERROR(Format("Required unit group '%s' is missing leader.", group_id.c_str()));
							++errors;
						}
					}
					break;
				case R_SPELL:
					{
						Spell* spell = FindSpell(str.c_str());
						if(!spell)
						{
							ERROR(Format("Missing required spell '%s'.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_DIALOG:
					{
						GameDialog* dialog = FindDialog(str.c_str());
						if(!dialog)
						{
							ERROR(Format("Missing required dialog '%s'.", str.c_str()));
							++errors;
						}
					}
					break;
				}

				t.Next();
			}
			catch(const Tokenizer::Exception& e)
			{
				ERROR(Format("Parse error: %s", e.ToString()));
				++errors;
				t.SkipToKeywordGroup(0);
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load required entities: %s", e.ToString()));
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

	if(errors > 0)
		throw Format("Failed to load required entities (%d errors), check log for details.", errors);
}
