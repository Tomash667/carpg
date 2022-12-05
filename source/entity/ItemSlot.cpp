#include "Pch.h"
#include "ItemSlot.h"

#include "Game.h"
#include "Item.h"
#include "Language.h"
#include "Unit.h"

cstring txAttack, txDefense, txBlock, txMobility, txRequiredStrength, txDTBlunt, txDTPierce, txDTSlash, txDTBluntPierce, txDTBluntSlash, txDTSlashPierce, txDTMagical,
txWeight, txValue, txInvalidArmor;

ItemSlotInfo ItemSlotInfo::slots[SLOT_MAX] = {
	"weapon",
	"bow",
	"shield",
	"armor",
	"amulet",
	"ring1",
	"ring2"
};

//=================================================================================================
ITEM_SLOT ItemSlotInfo::Find(const string& id)
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(id == slots[i].id)
			return (ITEM_SLOT)i;
	}
	return SLOT_INVALID;
}

//=================================================================================================
void SetItemStatsText()
{
	auto section = Language::GetSection("ItemStats");
	txAttack = section.Get("attack");
	txDefense = section.Get("defense");
	txBlock = section.Get("block");
	txMobility = section.Get("mobility");
	txRequiredStrength = section.Get("requiredStrength");
	txDTBlunt = section.Get("dtBlunt");
	txDTPierce = section.Get("dtPierce");
	txDTSlash = section.Get("dtSlash");
	txDTBluntPierce = section.Get("dtBluntPierce");
	txDTBluntSlash = section.Get("dtBluntSlash");
	txDTSlashPierce = section.Get("dtSlashPierce");
	txDTMagical = section.Get("dtMagical");
	txWeight = section.Get("weight");
	txValue = section.Get("value");
	txInvalidArmor = section.Get("invalidArmor");
}

//=================================================================================================
// Function used to sort items
// true - s1 need to be before s2
static bool SortItemsPred(const ItemSlot& s1, const ItemSlot& s2)
{
	if(!s1.item)
		return false;
	if(!s2.item)
		return true;
	if(s1.item != s2.item)
		return ItemCmp(s1.item, s2.item);
	else
	{
		if(s1.item->IsStackable())
			return false;
		else if(s1.teamCount < s2.teamCount)
			return true;
		else
			return false;
	}
}

//=================================================================================================
void SortItems(vector<ItemSlot>& items)
{
	// sort items
	std::sort(items.begin(), items.end(), SortItemsPred);

	// remove empty items
	while(!items.empty() && !items.back().item)
		items.pop_back();
}

char IsBetterColor(int old_value, int new_value)
{
	if(old_value >= new_value)
		return 'r';
	else
		return 'g';
}

void GetItemString(string& str, const Item* item, Unit* unit, uint count)
{
	assert(item);

	str = item->name;
	if(game->devmode)
	{
		cstring id = ReplaceAll(item->id.c_str(), "$", "$$");
		if(item->IsQuest())
			str += Format(" (%s,%d)", id, item->quest_id);
		else
			str += Format(" (%s)", id);
	}
	if(item->IsStackable() && count > 1)
		str += Format(" (%d)", count);

	switch(item->type)
	{
	case IT_WEAPON:
		{
			/*
			Rapier - Short blade
			Attack: 30 (40 -> 50) piercing
			Required strength: $50$
			*/
			const Weapon& weapon = item->ToWeapon();

			cstring dmg_type;
			switch(weapon.dmg_type)
			{
			case DMG_BLUNT:
				dmg_type = txDTBlunt;
				break;
			case DMG_PIERCE:
				dmg_type = txDTPierce;
				break;
			case DMG_SLASH:
				dmg_type = txDTSlash;
				break;
			case DMG_BLUNT | DMG_PIERCE:
				dmg_type = txDTBluntPierce;
				break;
			case DMG_BLUNT | DMG_SLASH:
				dmg_type = txDTBluntSlash;
				break;
			case DMG_SLASH | DMG_PIERCE:
				dmg_type = txDTSlashPierce;
				break;
			case DMG_BLUNT | DMG_PIERCE | DMG_SLASH:
				dmg_type = txDTMagical;
				break;
			default:
				dmg_type = "???";
				break;
			}

			int old_attack = (unit->HaveWeapon() ? (int)unit->CalculateAttack() : 0);
			int new_attack = (int)unit->CalculateAttack(item);
			cstring atk_desc;
			if(old_attack == new_attack)
				atk_desc = Format("%d", old_attack);
			else
				atk_desc = Format("$c%c%d -> %d$c-", IsBetterColor(old_attack, new_attack), old_attack, new_attack);

			str += Format(" - %s\n%s: %d (%s) %s\n%s: $c%c%d$c-\n",
				WeaponTypeInfo::info[weapon.weapon_type].name,
				txAttack,
				weapon.dmg,
				atk_desc,
				dmg_type,
				txRequiredStrength,
				(unit->Get(AttributeId::STR) >= weapon.reqStr ? '-' : 'r'),
				weapon.reqStr);
		}
		break;
	case IT_BOW:
		{
			/*
			Long bow
			Attack: 30 (40 -> 50) piercing
			Required strength: $40$
			*/
			const Bow& bow = item->ToBow();

			int old_attack = (unit->HaveBow() ? (int)unit->CalculateAttack(&unit->GetBow()) : 0);
			int new_attack = (int)unit->CalculateAttack(item);
			cstring atk_desc;
			if(old_attack == new_attack)
				atk_desc = Format("%d", old_attack);
			else
				atk_desc = Format("$c%c%d -> %d$c-", IsBetterColor(old_attack, new_attack), old_attack, new_attack);

			str += Format("\n%s: %d (%s) %s\n%s: $c%c%d$c-\n",
				txAttack,
				bow.dmg,
				atk_desc,
				txDTPierce,
				txRequiredStrength,
				(unit->Get(AttributeId::STR) >= bow.reqStr ? '-' : 'r'),
				bow.reqStr);
		}
		break;
	case IT_ARMOR:
		{
			/*
			Chainmail - Medium armor [(Does not fit)]
			Defense: 30 (40 -> 50)
			Required strength: $40$
			Mobility: 50 (40 -> 50)
			*/
			const Armor& armor = item->ToArmor();
			cstring mob_str, armor_type_text;

			cstring skill = Skill::skills[(int)armor.GetSkill()].name.c_str();
			if(unit->data->armor_type == armor.armor_unit_type)
				armor_type_text = skill;
			else
				armor_type_text = Format("%s (%s)", skill, txInvalidArmor);

			int old_mob = (int)unit->CalculateMobility();
			int new_mob = (int)unit->CalculateMobility(&armor);
			if(old_mob == new_mob)
				mob_str = Format("%d", new_mob);
			else
				mob_str = Format("$c%c%d -> %d$c-", IsBetterColor(old_mob, new_mob), old_mob, new_mob);

			int old_def = (int)unit->CalculateDefense();
			int new_def = (int)unit->CalculateDefense(item);
			cstring def_desc;
			if(old_def == new_def)
				def_desc = Format("%d", old_def);
			else
				def_desc = Format("$c%c%d -> %d$c-", IsBetterColor(old_def, new_def), old_def, new_def);

			str += Format(" - %s\n%s: %d (%s)\n%s: $c%c%d$c-\n%s: %d (%s)\n",
				armor_type_text,
				txDefense,
				armor.def,
				def_desc,
				txRequiredStrength,
				(unit->Get(AttributeId::STR) >= armor.reqStr ? '-' : 'r'),
				armor.reqStr,
				txMobility,
				armor.mobility,
				mob_str);
		}
		break;
	case IT_SHIELD:
		{
			/*
			Iron shield
			Block: 30 (40 -> 50)
			Required strength: $40$
			*/
			const Shield& shield = item->ToShield();

			cstring block_desc;
			int new_block = (int)unit->CalculateBlock(item);
			if(unit->HaveShield())
			{
				int old_block = (int)unit->CalculateBlock();
				if(old_block == new_block)
					block_desc = Format("%d", new_block);
				else
					block_desc = Format("$c%c%d -> %d$c-", IsBetterColor(old_block, new_block), old_block, new_block);
			}
			else
				block_desc = Format("%d", new_block);

			str += Format("\n%s: %d (%s)\n%s: $c%c%d$c-\n",
				txBlock,
				shield.block,
				block_desc,
				txRequiredStrength,
				(unit->Get(AttributeId::STR) >= shield.reqStr ? '-' : 'r'),
				shield.reqStr);
		}
		break;
	default:
		str += "\n";
		break;
	}

	// weight
	str += Format(txWeight, item->GetWeight());
	if(count > 1)
		str += Format(" (%g)\n", item->GetWeight()*count);
	else
		str += "\n";

	// value
	str += Format(txValue, item->value);
}

bool InsertItemStackable(vector<ItemSlot>& items, ItemSlot& slot)
{
	vector<ItemSlot>::iterator it = std::lower_bound(items.begin(), items.end(), slot, SortItemsPred);
	if(it == items.end())
	{
		// add at end
		items.push_back(slot);
		return false;
	}
	else
	{
		if(it->item == slot.item)
		{
			// stack up item
			it->count += slot.count;
			it->teamCount += slot.teamCount;
			return true;
		}
		else
		{
			// add in middle
			items.insert(it, slot);
			return false;
		}
	}
}

void InsertItemNotStackable(vector<ItemSlot>& items, ItemSlot& slot)
{
	vector<ItemSlot>::iterator it = std::lower_bound(items.begin(), items.end(), slot, SortItemsPred);
	uint size = items.size();
	ItemSlot slot2;
	slot2.item = slot.item;
	slot2.count = 1;
	if(slot.teamCount)
	{
		slot2.teamCount = 1;
		--slot.teamCount;
	}
	else
		slot2.teamCount = 0;

	if(it == items.end())
	{
		// resize vector
		items.reserve(size + slot.count);

		// add at end
		while(slot.count)
		{
			items.push_back(slot2);
			if(slot.teamCount)
			{
				slot2.teamCount = 1;
				--slot.teamCount;
			}
			else
				slot2.teamCount = 0;
			--slot.count;
		}
	}
	else
	{
		vector<ItemSlot> v_copy(items.begin(), it);
		v_copy.reserve(items.size() + slot.count);

		// add new
		while(slot.count)
		{
			v_copy.push_back(slot2);
			if(slot.teamCount)
			{
				slot2.teamCount = 1;
				--slot.teamCount;
			}
			else
				slot2.teamCount = 0;
			--slot.count;
		}

		// add old at right
		for(vector<ItemSlot>::iterator end = items.end(); it != end; ++it)
			v_copy.push_back(*it);

		items.swap(v_copy);
	}
}

bool InsertItem(vector<ItemSlot>& items, ItemSlot& slot)
{
	assert(slot.item && slot.count && slot.teamCount <= slot.count);

	if(slot.item->IsStackable())
		return InsertItemStackable(items, slot);
	else
	{
		InsertItemNotStackable(items, slot);
		return false;
	}
}

void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count, uint teamCount)
{
	assert(item && count && count >= teamCount);

	if(item->IsStackable())
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
			{
				it->count += count;
				it->teamCount += teamCount;
				return;
			}
		}

		ItemSlot& slot = Add1(items);
		slot.Set(item, count, teamCount);
	}
	else
	{
		items.reserve(items.size() + count);
		ItemSlot slot;
		slot.Set(item, 1, 0);
		items.resize(items.size() + count - teamCount, slot);
		slot.teamCount = 1;
		items.resize(items.size() + teamCount, slot);
	}
}

bool CompareItemsForIndex(const ItemSlot& slot, const Item* item, bool isTeam)
{
	if(slot.item != item)
		return false;
	if(item->IsStackable())
		return true;
	return (slot.teamCount > 0) == isTeam;
}

int FindItemIndex(const vector<ItemSlot>& items, int index, const Item* item, bool isTeam)
{
	assert(index >= 0 && item);

	if((uint)index < items.size())
	{
		const ItemSlot& slot = items[index];
		if(CompareItemsForIndex(slot, item, isTeam))
			return index;
	}

	ItemSlot slot;
	slot.item = item;
	slot.teamCount = (isTeam && !item->IsStackable()) ? 1 : 0;
	auto it = std::lower_bound(items.begin(), items.end(), slot, SortItemsPred);
	if(it == items.end() || it->item != item || !CompareItemsForIndex(*it, item, isTeam))
		return -1;
	else
		return it - items.begin();
}
