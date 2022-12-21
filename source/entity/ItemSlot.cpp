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

char IsBetterColor(int oldValue, int newValue)
{
	if(oldValue >= newValue)
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
			str += Format(" (%s,%d)", id, item->questId);
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

			cstring dmgType;
			switch(weapon.dmgType)
			{
			case DMG_BLUNT:
				dmgType = txDTBlunt;
				break;
			case DMG_PIERCE:
				dmgType = txDTPierce;
				break;
			case DMG_SLASH:
				dmgType = txDTSlash;
				break;
			case DMG_BLUNT | DMG_PIERCE:
				dmgType = txDTBluntPierce;
				break;
			case DMG_BLUNT | DMG_SLASH:
				dmgType = txDTBluntSlash;
				break;
			case DMG_SLASH | DMG_PIERCE:
				dmgType = txDTSlashPierce;
				break;
			case DMG_BLUNT | DMG_PIERCE | DMG_SLASH:
				dmgType = txDTMagical;
				break;
			default:
				dmgType = "???";
				break;
			}

			int oldAttack = (unit->HaveWeapon() ? (int)unit->CalculateAttack() : 0);
			int newAttack = (int)unit->CalculateAttack(item);
			cstring atkDesc;
			if(oldAttack == newAttack)
				atkDesc = Format("%d", oldAttack);
			else
				atkDesc = Format("$c%c%d -> %d$c-", IsBetterColor(oldAttack, newAttack), oldAttack, newAttack);

			str += Format(" - %s\n%s: %d (%s) %s\n%s: $c%c%d$c-\n",
				WeaponTypeInfo::info[weapon.weaponType].name,
				txAttack,
				weapon.dmg,
				atkDesc,
				dmgType,
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

			int oldAttack = (unit->HaveBow() ? (int)unit->CalculateAttack(&unit->GetBow()) : 0);
			int newAttack = (int)unit->CalculateAttack(item);
			cstring atkDesc;
			if(oldAttack == newAttack)
				atkDesc = Format("%d", oldAttack);
			else
				atkDesc = Format("$c%c%d -> %d$c-", IsBetterColor(oldAttack, newAttack), oldAttack, newAttack);

			str += Format("\n%s: %d (%s) %s\n%s: $c%c%d$c-\n",
				txAttack,
				bow.dmg,
				atkDesc,
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
			cstring mobilityStr, armorTypeText;

			cstring skill = Skill::skills[(int)armor.GetSkill()].name.c_str();
			if(unit->data->armorType == armor.armorUnitType)
				armorTypeText = skill;
			else
				armorTypeText = Format("%s (%s)", skill, txInvalidArmor);

			int oldMobility = (int)unit->CalculateMobility();
			int newMobility = (int)unit->CalculateMobility(&armor);
			if(oldMobility == newMobility)
				mobilityStr = Format("%d", newMobility);
			else
				mobilityStr = Format("$c%c%d -> %d$c-", IsBetterColor(oldMobility, newMobility), oldMobility, newMobility);

			int oldDef = (int)unit->CalculateDefense();
			int newDef = (int)unit->CalculateDefense(item);
			cstring defDesc;
			if(oldDef == newDef)
				defDesc = Format("%d", oldDef);
			else
				defDesc = Format("$c%c%d -> %d$c-", IsBetterColor(oldDef, newDef), oldDef, newDef);

			str += Format(" - %s\n%s: %d (%s)\n%s: $c%c%d$c-\n%s: %d (%s)\n",
				armorTypeText,
				txDefense,
				armor.def,
				defDesc,
				txRequiredStrength,
				(unit->Get(AttributeId::STR) >= armor.reqStr ? '-' : 'r'),
				armor.reqStr,
				txMobility,
				armor.mobility,
				mobilityStr);
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

			cstring blockDesc;
			int newBlock = (int)unit->CalculateBlock(item);
			if(unit->HaveShield())
			{
				int oldBlock = (int)unit->CalculateBlock();
				if(oldBlock == newBlock)
					blockDesc = Format("%d", newBlock);
				else
					blockDesc = Format("$c%c%d -> %d$c-", IsBetterColor(oldBlock, newBlock), oldBlock, newBlock);
			}
			else
				blockDesc = Format("%d", newBlock);

			str += Format("\n%s: %d (%s)\n%s: $c%c%d$c-\n",
				txBlock,
				shield.block,
				blockDesc,
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
		str += Format(" (%g)\n", item->GetWeight() * count);
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
		vector<ItemSlot> vCopy(items.begin(), it);
		vCopy.reserve(items.size() + slot.count);

		// add new
		while(slot.count)
		{
			vCopy.push_back(slot2);
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
			vCopy.push_back(*it);

		items.swap(vCopy);
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
