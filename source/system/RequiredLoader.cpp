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
	R_QUEST_LIST,
	R_PERK
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
		{ "buildingGroup", R_BUILDING_GROUP },
		{ "building", R_BUILDING },
		{ "buildingScript", R_BUILDING_SCRIPT },
		{ "object", R_OBJECT },
		{ "usable", R_USABLE },
		{ "questList", R_QUEST_LIST },
		{ "perk", R_PERK }
		});
}

//=================================================================================================
void RequiredLoader::LoadEntity(int type, const string& id)
{
	switch(type)
	{
	case R_ITEM:
		{
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
			ItemList* lis = ItemList::TryGet(id.c_str());
			if(!lis)
			{
				Error("Missing required item list '%s'.", id.c_str());
				++content.errors;
			}
			else if(lis->isLeveled != leveled)
			{
				if(leveled)
					Error("Required list '%s' must be leveled.", id.c_str());
				else
					Error("Required list '%s' is leveled.", id.c_str());
				++content.errors;
			}
			else if(lis->items.empty())
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
			const string& groupId = t.MustGetItemKeyword();
			UnitGroup* group = UnitGroup::TryGet(groupId);
			if(!group)
			{
				Error("Missing required unit group list '%s'.", groupId.c_str());
				++content.errors;
			}
			else if(!group->isList)
			{
				Error("Required unit group '%s' is not list.", groupId.c_str());
				++content.errors;
			}
		}
		else
		{
			bool needLeader = false;
			if(id == "with_leader")
			{
				needLeader = true;
				t.Next();
			}
			const string& groupId = t.MustGetItemKeyword();
			UnitGroup* group = UnitGroup::TryGet(groupId);
			if(!group)
			{
				Error("Missing required unit group '%s'.", groupId.c_str());
				++content.errors;
			}
			else if(group->isList)
			{
				Error("Required unit group '%s' is list.", groupId.c_str());
				++content.errors;
			}
			else if(needLeader && !group->HaveLeader())
			{
				Error("Required unit group '%s' is missing leader.", groupId.c_str());
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
			const bool notNone = IsPrefix("notNone");
			QuestList* list = QuestList::TryGet(id);
			if(!list)
			{
				Error("Missing required quest list '%s'.", id.c_str());
				++content.errors;
			}
			else if(notNone)
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
	case R_PERK:
		{
			Perk* perk = Perk::Get(id);
			if(!perk)
			{
				Error("Missing required perk '%s'.", id.c_str());
				++content.errors;
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
	bool haveZero = !required, haveHeirloom = false;

	for(StartItem& si : StartItem::startItems)
	{
		if(si.skill == skill)
		{
			if(si.value == 0)
				haveZero = true;
			else if(si.value == HEIRLOOM)
				haveHeirloom = true;
			if(haveZero && haveHeirloom)
				return;
		}
	}

	if(!haveZero)
	{
		Error("Missing starting item for skill %s.", Skill::skills[(int)skill].id);
		++content.errors;
	}
	if(!haveHeirloom)
	{
		Error("Missing heirloom item for skill %s.", Skill::skills[(int)skill].id);
		++content.errors;
	}
}

//=================================================================================================
void RequiredLoader::CheckBaseItems()
{
	int haveShortBlade = 0,
		haveLongBlade = 0,
		haveAxe = 0,
		haveBlunt = 0,
		haveWand = 0,
		haveBow = 0,
		haveShield = 0,
		haveLightArmor = 0,
		haveMediumArmor = 0,
		haveHeavyArmor = 0,
		haveMageArmor = 0;
	const ItemList& lis = ItemList::Get("baseItems");

	for(const ItemList::Entry& e : lis.items)
	{
		const Item* item = e.item;
		if(item->type == IT_WEAPON)
		{
			if(IsSet(item->flags, ITEM_MAGE))
				++haveWand;
			else
			{
				switch(item->ToWeapon().weaponType)
				{
				case WT_SHORT_BLADE:
					++haveShortBlade;
					break;
				case WT_LONG_BLADE:
					++haveLongBlade;
					break;
				case WT_AXE:
					++haveAxe;
					break;
				case WT_BLUNT:
					++haveBlunt;
					break;
				}
			}
		}
		else if(item->type == IT_BOW)
			++haveBow;
		else if(item->type == IT_SHIELD)
			++haveShield;
		else if(item->type == IT_ARMOR)
		{
			if(IsSet(item->flags, ITEM_MAGE))
				++haveMageArmor;
			else
			{
				switch(item->ToArmor().armorType)
				{
				case AT_LIGHT:
					++haveLightArmor;
					break;
				case AT_MEDIUM:
					++haveMediumArmor;
					break;
				case AT_HEAVY:
					++haveHeavyArmor;
					break;
				}
			}
		}
	}

	CheckBaseItem("short blade weapon", haveShortBlade);
	CheckBaseItem("long blade weapon", haveLongBlade);
	CheckBaseItem("axe weapon", haveAxe);
	CheckBaseItem("blunt weapon", haveBlunt);
	CheckBaseItem("mage weapon", haveWand);
	CheckBaseItem("bow", haveBow);
	CheckBaseItem("shield", haveShield);
	CheckBaseItem("light armor", haveLightArmor);
	CheckBaseItem("medium armor", haveMediumArmor);
	CheckBaseItem("heavy armor", haveHeavyArmor);
	CheckBaseItem("mage armor", haveMageArmor);
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
