#include "Pch.h"
#include "Base.h"
#include "Item.h"
#include "ItemSlot.h"
#include "Unit.h"
#include "Language.h"

cstring txDescWeapon, txRequiredStrength, txDTBlunt, txDTPierce, txDTSlash, txDTBluntPierce, txDTBluntSlash, txDTSlashPierce, txDTMagical, txDescBow, txDescArmor, txDescShield, txWeight, txValue;

//=================================================================================================
void LoadItemStatsText()
{
	txDescWeapon = Str("descWeapon");
	txRequiredStrength = Str("requiredStrength");
	txDTBlunt = Str("dtBlunt");
	txDTPierce = Str("dtPierce");
	txDTSlash = Str("dtSlash");
	txDTBluntPierce = Str("dtBluntPierce");
	txDTBluntSlash = Str("dtBluntSlash");
	txDTSlashPierce = Str("dtSlashPierce");
	txDTMagical = Str("dtMagical");
	txDescBow = Str("descBow");
	txDescArmor = Str("descArmor");
	txDescShield = Str("descShield");
	txWeight = Str("weight");
	txValue = Str("value");
}

//=================================================================================================
// Funkcja u¿ywana do sortowanie przedmiotów
// true - s1 ma byæ wczeœniej
//=================================================================================================
static bool SortItemsPred(const ItemSlot& s1, const ItemSlot& s2)
{
	if(!s1.item)
		return false;
	if(!s2.item)
		return true;
	return ItemCmp(s1.item, s2.item);
}

//=================================================================================================
// Sortowanie przedmiotów
//=================================================================================================
void SortItems(vector<ItemSlot>& items)
{
	// sortuj przedmioty
	std::sort(items.begin(), items.end(), SortItemsPred);

	// usuñ puste elementy
	while(!items.empty() && !items.back().item)
		items.pop_back();
}

extern cstring armor_type_string[4];

void GetItemString(string& str, const Item* item, Unit* unit, uint count)
{
	assert(item);

	str = item->name;

	switch(item->type)
	{
	case IT_WEAPON:
		{
			const Weapon& w = item->ToWeapon();
			str += Format(txDescWeapon, weapon_type_info[w.weapon_type].name, w.dmg, (int)unit->CalculateAttack(item));
			switch(w.dmg_type)
			{
			case DMG_BLUNT:
				str += txDTBlunt;
				break;
			case DMG_PIERCE:
				str += txDTPierce;
				break;
			case DMG_SLASH:
				str += txDTSlash;
				break;
			case DMG_BLUNT|DMG_PIERCE:
				str += txDTBluntPierce;
				break;
			case DMG_BLUNT|DMG_SLASH:
				str += txDTBluntSlash;
				break;
			case DMG_SLASH|DMG_PIERCE:
				str += txDTSlashPierce;
				break;
			case DMG_BLUNT|DMG_PIERCE|DMG_SLASH:
				str += txDTMagical;
				break;
			default:
				str += "???";
				break;
			}
			str += Format(txRequiredStrength, w.sila);
			if(unit->Get(Attribute::STR) < w.sila)
				str += " (!)";
			str += "\n";
		}
		break;
	case IT_BOW:
		{
			const Bow& b = item->ToBow();
			str += Format(txDescBow, b.dmg, (int)unit->CalculateAttack(item), b.sila);
			if(unit->Get(Attribute::STR) < b.sila)
				str += " (!)";
			str += "\n";
		}
		break;
	case IT_ARMOR:
		{
			const Armor& a = item->ToArmor();
			int without_armor;
			int dex = unit->CalculateDexterity(a, &without_armor);
			cstring s;
			if(dex == without_armor)
				s = "";
			else
				s = Format(" (%d->%d)", without_armor, dex);
			str += Format(txDescArmor, armor_type_string[a.armor_type], a.def, (int)unit->CalculateDefense(item), a.sila, (unit->Get(Attribute::STR) < a.sila ? " (!)" : ""),
				a.zrecznosc, s);
		}
		break;
	case IT_SHIELD:
		{
			const Shield& s = item->ToShield();
			str += Format(txDescShield, s.def, (int)unit->CalculateBlock(item), s.sila, (unit->Get(Attribute::STR) < s.sila ? " (!)" : ""));
		}
		break;
	default:
		str += "\n";
		break;
	}

	// waga
	str += Format(txWeight, item->GetWeight());
	if(count > 1)
		str += Format(" (%g)\n", item->GetWeight()*count);
	else
		str += "\n";

	// cena
	str += Format(txValue, item->value);
	if(count > 1)
		str += Format(" (%d)", item->value*count);
}

// dodaj stackuj¹cy siê przedmiot do wektora przedmiotów
bool InsertItemStackable(vector<ItemSlot>& items, ItemSlot& slot)
{
	vector<ItemSlot>::iterator it = std::lower_bound(items.begin(), items.end(), slot, SortItemsPred);
	if(it == items.end())
	{
		// dodaj na samym koñcu
		items.push_back(slot);
		return false;
	}
	else
	{
		if(it->item == slot.item)
		{
			// stackuj przedmiot
			it->count += slot.count;
			it->team_count += slot.team_count;
			return true;
		}
		else
		{
			// dodaj przedmiot
			items.insert(it, slot);
			return false;
		}
	}
}

// dodaj nie stackuj¹cy siê przedmiot do wektora przedmiotów
void InsertItemNotStackable(vector<ItemSlot>& items, ItemSlot& slot)
{
	vector<ItemSlot>::iterator it = std::lower_bound(items.begin(), items.end(), slot, SortItemsPred);
	uint size = items.size();
	ItemSlot slot2;
	slot2.item = slot.item;
	slot2.count = 1;
	if(slot.team_count)
	{
		slot2.team_count = 1;
		--slot.team_count;
	}
	else
		slot2.team_count = 0;

	if(it == items.end())
	{
		// powiêksz wektor
		items.reserve(size + slot.count);

		// dodaj na samym koñcu
		while(slot.count)
		{
			items.push_back(slot2);
			if(slot.team_count)
			{
				slot2.team_count = 1;
				--slot.team_count;
			}
			else
				slot2.team_count = 0;
			--slot.count;
		}
	}
	else
	{
		vector<ItemSlot> v_copy(items.begin(), it);
		v_copy.reserve(items.size()+slot.count);

		// dodaj nowe
		while(slot.count)
		{
			v_copy.push_back(slot2);
			if(slot.team_count)
			{
				slot2.team_count = 1;
				--slot.team_count;
			}
			else
				slot2.team_count = 0;
			--slot.count;
		}

		// dodaj stare z prawej
		for(vector<ItemSlot>::iterator end = items.end(); it != end; ++it)
			v_copy.push_back(*it);

		// zamieñ
		items.swap(v_copy);
	}
}

bool InsertItem(vector<ItemSlot>& items, ItemSlot& slot)
{
	assert(slot.item && slot.count && slot.team_count <= slot.count);

	if(slot.item->IsStackable())
		return InsertItemStackable(items, slot);
	else
	{
		InsertItemNotStackable(items, slot);
		return false;
	}
}

void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count, uint team_count)
{
	assert(item && count && count >= team_count);

	if(item->IsStackable())
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
			{
				it->count += count;
				it->team_count += team_count;
				return;
			}
		}
	}

	// nie stackuje siê lub nie ma takiego, dodaj
	ItemSlot& slot = Add1(items);
	slot.Set(item, count, team_count);
}
