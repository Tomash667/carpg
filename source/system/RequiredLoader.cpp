#include "Pch.h"
#include "RequiredLoader.h"
#include "Ability.h"
#include "BaseUsable.h"
#include "Building.h"
#include "BuildingGroup.h"
#include "BuildingScript.h"
#include "GameDialog.h"
#include "Item.h"
#include "QuestList.h"
#include "Stock.h"
#include "UnitData.h"
#include "UnitGroup.h"

enum RequiredType
{
	R_ITEM,
	R_LIST,
	R_STOCK,
	R_UNIT,
	R_GROUP,
	R_ABILITY,
	R_DIALOG,
	R_BUILDING_GROUP,
	R_BUILDING,
	R_BUILDING_SCRIPT,
	R_OBJECT,
	R_USABLE,
	R_QUEST_LIST
};

//=================================================================================================
void RequiredLoader::DoLoading()
{
	Load("required.txt", 0);
}

//=================================================================================================
void RequiredLoader::InitTokenizer()
{
	t.AddKeywords(0, {
		{ "item", R_ITEM },
		{ "list", R_LIST },
		{ "stock", R_STOCK },
		{ "unit", R_UNIT },
		{ "group", R_GROUP },
		{ "ability", R_ABILITY },
		{ "dialog", R_DIALOG },
		{ "building_group", R_BUILDING_GROUP },
		{ "building", R_BUILDING },
		{ "building_script", R_BUILDING_SCRIPT },
		{ "object", R_OBJECT },
		{ "usable", R_USABLE },
		{ "quest_list", R_QUEST_LIST }
		});
}

//=================================================================================================
void RequiredLoader::LoadEntity(int type, const string& id)
{
	switch(type)
	{
	case R_ITEM:
		{
			ItemListResult result;
			const Item* item = Item::TryGet(id);
			if(!item)
			{
				Error("Missing required item '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_LIST:
		{
			const bool leveled = IsPrefix("leveled");
			ItemListResult result = ItemList::TryGet(id.c_str());
			if(!result.lis)
			{
				Error("Missing required item list '%s'.", id.c_str());
				++content.errors;
			}
			else if(result.is_leveled != leveled)
			{
				if(leveled)
					Error("Required list '%s' must be leveled.", id.c_str());
				else
					Error("Required list '%s' is leveled.", id.c_str());
				++content.errors;
			}
			else if(result.lis->items.empty())
			{
				Error("Required list '%s' is empty.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_STOCK:
		{
			Stock* stock = Stock::TryGet(id);
			if(!stock)
			{
				Error("Missing required item stock '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_UNIT:
		{
			UnitData* ud = UnitData::TryGet(id.c_str());
			if(!ud)
			{
				Error("Missing required unit '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_GROUP:
		if(id == "list")
		{
			t.Next();
			const string& group_id = t.MustGetItemKeyword();
			UnitGroup* group = UnitGroup::TryGet(group_id);
			if(!group)
			{
				Error("Missing required unit group list '%s'.", group_id.c_str());
				++content.errors;
			}
			else if(!group->is_list)
			{
				Error("Required unit group '%s' is not list.", group_id.c_str());
				++content.errors;
			}
		}
		else
		{
			bool need_leader = false;
			if(id == "with_leader")
			{
				need_leader = true;
				t.Next();
			}
			const string& group_id = t.MustGetItemKeyword();
			UnitGroup* group = UnitGroup::TryGet(group_id);
			if(!group)
			{
				Error("Missing required unit group '%s'.", group_id.c_str());
				++content.errors;
			}
			else if(group->is_list)
			{
				Error("Required unit group '%s' is list.", group_id.c_str());
				++content.errors;
			}
			else if(need_leader && !group->HaveLeader())
			{
				Error("Required unit group '%s' is missing leader.", group_id.c_str());
				++content.errors;
			}
		}
		break;
	case R_ABILITY:
		{
			Ability* ability = Ability::Get(id);
			if(!ability)
			{
				Error("Missing required ability '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_DIALOG:
		{
			GameDialog* dialog = GameDialog::TryGet(id.c_str());
			if(!dialog)
			{
				Error("Missing required dialog '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_BUILDING_GROUP:
		{
			BuildingGroup* group = BuildingGroup::TryGet(id);
			if(!group)
			{
				Error("Missing required building group '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_BUILDING:
		{
			Building* building = Building::TryGet(id);
			if(!building)
			{
				Error("Missing required building '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_BUILDING_SCRIPT:
		{
			BuildingScript* script = BuildingScript::TryGet(id);
			if(!script)
			{
				Error("Missing required building script '%s'.", id.c_str());
				++content.errors;
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
					++content.errors;
					break;
				}
				t.Next();
			}
		}
		break;
	case R_OBJECT:
		{
			auto obj = BaseObject::TryGet(id);
			if(!obj)
			{
				Error("Missing required object '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_USABLE:
		{
			auto use = BaseUsable::TryGet(id);
			if(!use)
			{
				Error("Missing required usable object '%s'.", id.c_str());
				++content.errors;
			}
		}
		break;
	case R_QUEST_LIST:
		{
			const bool not_none = IsPrefix("not_none");
			QuestList* list = QuestList::TryGet(id);
			if(!list)
			{
				Error("Missing required quest list '%s'.", id.c_str());
				++content.errors;
			}
			else if(not_none)
			{
				for(QuestList::Entry& e : list->entries)
				{
					if(!e.info)
					{
						Error("Required quest list '%s' can't contain 'none'.", list->id.c_str());
						++content.errors;
						break;
					}
				}
			}
		}
		break;
	}
}

//=================================================================================================
void RequiredLoader::Finalize()
{
	CheckStartItems(SkillId::SHORT_BLADE, true);
	CheckStartItems(SkillId::LONG_BLADE, true);
	CheckStartItems(SkillId::AXE, true);
	CheckStartItems(SkillId::BLUNT, true);
	CheckStartItems(SkillId::BOW, false);
	CheckStartItems(SkillId::SHIELD, false);
	CheckStartItems(SkillId::LIGHT_ARMOR, true);
	CheckStartItems(SkillId::MEDIUM_ARMOR, true);
	CheckStartItems(SkillId::HEAVY_ARMOR, true);

	CheckBaseItems();
}

//=================================================================================================
void RequiredLoader::CheckStartItems(SkillId skill, bool required)
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
		Error("Missing starting item for skill %s.", Skill::skills[(int)skill].id);
		++content.errors;
	}
	if(!have_heirloom)
	{
		Error("Missing heirloom item for skill %s.", Skill::skills[(int)skill].id);
		++content.errors;
	}
}

//=================================================================================================
void RequiredLoader::CheckBaseItems()
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
	const ItemList* lis = ItemList::Get("base_items").lis;

	for(const Item* item : lis->items)
	{
		if(item->type == IT_WEAPON)
		{
			if(IsSet(item->flags, ITEM_MAGE))
				++have_wand;
			else
			{
				switch(item->ToWeapon().weapon_type)
				{
				case WT_SHORT_BLADE:
					++have_short_blade;
					break;
				case WT_LONG_BLADE:
					++have_long_blade;
					break;
				case WT_AXE:
					++have_axe;
					break;
				case WT_BLUNT:
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
			if(IsSet(item->flags, ITEM_MAGE))
				++have_mage_armor;
			else
			{
				switch(item->ToArmor().armor_type)
				{
				case AT_LIGHT:
					++have_light_armor;
					break;
				case AT_MEDIUM:
					++have_medium_armor;
					break;
				case AT_HEAVY:
					++have_heavy_armor;
					break;
				}
			}
		}
	}

	CheckBaseItem("short blade weapon", have_short_blade);
	CheckBaseItem("long blade weapon", have_long_blade);
	CheckBaseItem("axe weapon", have_axe);
	CheckBaseItem("blunt weapon", have_blunt);
	CheckBaseItem("mage weapon", have_wand);
	CheckBaseItem("bow", have_bow);
	CheckBaseItem("shield", have_shield);
	CheckBaseItem("light armor", have_light_armor);
	CheckBaseItem("medium armor", have_medium_armor);
	CheckBaseItem("heavy armor", have_heavy_armor);
	CheckBaseItem("mage armor", have_mage_armor);
}

//=================================================================================================
void RequiredLoader::CheckBaseItem(cstring name, int num)
{
	if(num == 0)
	{
		Error("Missing base %s.", name);
		++content.errors;
	}
	else if(num > 1)
	{
		Error("Multiple base %ss (%d).", name, num);
		++content.errors;
	}
}
