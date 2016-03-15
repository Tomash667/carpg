#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "UnitHelper.h"

// Team shares only work for equippable items, that have only 1 count in slot!

//-----------------------------------------------------------------------------
// powinno sortowaæ w takiej kolejnoœci:
// najlepsza broñ, gorsza broñ, najgorsza broñ, najlepszy ³uk, œreni ³uk, najgorszy ³uk, najlepszy pancerz, œredni pancerz, najgorszy pancerz, najlepsza tarcza,
//	œrednia tarcza, najgorsza tarcza
const int item_type_prio[4] = {
	0, // IT_WEAPON
	1, // IT_BOW
	3, // IT_SHIELD
	2  // IT_ARMOR
};

//-----------------------------------------------------------------------------
struct SortTeamShares
{
	Unit* unit;

	explicit SortTeamShares(Unit* unit) : unit(unit)
	{

	}

	bool operator () (const TeamShareItem& t1, const TeamShareItem& t2) const
	{
		if(t1.item->type == t2.item->type)
			return t1.value > t2.value;
		else
		{
			int p1 = item_type_prio[t1.item->type];
			int p2 = item_type_prio[t2.item->type];
			if(p1 != p2)
				return p1 < p2;
			else
			{
				if(t1.priority != t2.priority)
					return t1.priority < t2.priority;
				else
					return t1.value < t2.value;
			}
		}
	}
};

//-----------------------------------------------------------------------------
bool UniqueTeamShares(const TeamShareItem& t1, const TeamShareItem& t2)
{
	return t1.to == t2.to && t1.from == t2.from && t1.item == t2.item && t1.priority == t2.priority;
}

//=================================================================================================
void Game::CheckTeamItemShares()
{
	if(!HaveTeamMemberNPC())
	{
		team_share_id = -1;
		return;
	}

	team_shares.clear();
	uint pos_a, pos_b;

	for(Unit* unit : active_team)
	{
		if(unit->IsPlayer())
			continue;

		pos_a = team_shares.size();

		for(Unit* other_unit : active_team)
		{
			int index = 0;
			for(ItemSlot& slot : other_unit->items)
			{
				if(slot.item && slot.item->IsWearableByHuman())
				{
					// don't check if can't buy
					if(slot.team_count == 0 && slot.item->value / 2 > unit->gold && unit != other_unit)
						continue;

					int value;
					if(IsBetterItem(*unit, slot.item, &value))
					{
						TeamShareItem& tsi = Add1(team_shares);
						tsi.from = other_unit;
						tsi.to = unit;
						tsi.item = slot.item;
						tsi.index = index;
						tsi.value = value;
						if(unit == other_unit)
						{
							if(slot.team_count == 0)
								tsi.priority = 0; // my item
							else
								tsi.priority = 1; // team item i have
						}
						else
						{
							if(slot.team_count != 0)
							{
								if(other_unit->IsPlayer())
									tsi.priority = 3; // team item that player have
								else
									tsi.priority = 2; // team item that ai have
							}
							else
							{
								if(other_unit->IsPlayer())
									tsi.priority = 5; // item that player own
								else
									tsi.priority = 4; // item that ai own
							}
						}
					}
				}
				++index;
			}
		}

		// sort and remove duplictes
		pos_b = team_shares.size();
		if(pos_b - pos_a > 1)
		{
			std::vector<TeamShareItem>::iterator it2 = std::unique(team_shares.begin() + pos_a, team_shares.end(), UniqueTeamShares);
			team_shares.resize(std::distance(team_shares.begin(), it2));
			std::sort(team_shares.begin() + pos_a, team_shares.end(), SortTeamShares(unit));
		}
	}

	if(team_shares.empty())
		team_share_id = -1;
	else
		team_share_id = 0;
}

//=================================================================================================
bool Game::CheckTeamShareItem(TeamShareItem& tsi)
{
	int search_next;

	if((int)tsi.from->items.size() <= tsi.index)
	{
		// index jest za du¿y, sprawdŸ czy przedmiotu nie ma wczeœniej
		search_next = false;
	}
	else if(tsi.from->items[tsi.index].item == tsi.item)
	{
		// wszystko ok
		return true;
	}
	else
	{
		// na tym miejscu jest inny przedmiot
		if(tsi.index + 1 == (int)tsi.from->items.size())
		{
			// to jest ostatni przedmiot, szukaj wczeœniej
			search_next = false;
		}
		else if(ItemCmp(tsi.item, tsi.from->items[tsi.index].item))
		{
			// poszukiwany przedmiot powinien byæ wczeœniej
			search_next = false;
		}
		else
		{
			// poszukiwany przedmiot powinien byæ póŸniej
			search_next = true;
		}
	}

	// szukaj
	int index = tsi.index;
	if(search_next)
	{
		while(true)
		{
			++index;
			if(index == (int)tsi.from->items.size())
				return false;
			const Item* item = tsi.from->items[index].item;
			if(item == tsi.item)
			{
				// znaleziono przedmiot
				tsi.index = index;
				return true;
			}
			else if(ItemCmp(tsi.item, item))
			{
				// przedmiot powinien byæ przed tym przedmiotem ale go nie by³o
				return false;
			}
		}
	}
	else
	{
		while(index > 0 && index >= (int)tsi.from->items.size())
			--index;
		while(true)
		{
			--index;
			if(index == -1)
				return false;
			const Item* item = tsi.from->items[index].item;
			if(item == tsi.item)
			{
				// znaleziono przedmiot
				tsi.index = index;
				return true;
			}
			else if(!ItemCmp(tsi.item, item))
			{
				// przedmiot powinien byæ po tym przedmiocie ale go nie by³o
				return false;
			}
		}
	}
}

//=================================================================================================
void Game::UpdateTeamItemShares()
{
	if(fallback_co != -1 || team_share_id == -1)
		return;

	if(team_share_id >= (int)team_shares.size())
	{
		team_share_id = -1;
		return;
	}

	TeamShareItem& tsi = team_shares[team_share_id];
	int state = 1; // 0-no need to talk, 1-ask about share, 2-wait for time to talk
	DialogEntry* dialog = nullptr;
	if(tsi.to->busy != Unit::Busy_No)
		state = 2;
	else if(!CheckTeamShareItem(tsi))
		state = 0;
	else
	{
		ItemSlot& slot = tsi.from->items[tsi.index];
		if(!IsBetterItem(*tsi.to, tsi.item))
			state = 0;
		else
		{
			// new item weight - if it's already in inventory then it don't add weight
			int item_weight = (tsi.from != tsi.to ? slot.item->weight : 0); 

			// old item, can be sold if overweight
			int prev_item_weight;
			ITEM_SLOT slot_type = ItemTypeToSlot(slot.item->type);
			if(tsi.to->slots[slot_type])
				prev_item_weight = tsi.to->slots[slot_type]->weight;
			else
				prev_item_weight = 0;

			if(tsi.to->weight + item_weight - prev_item_weight > tsi.to->weight_max)
			{
				// unit will be overweighted, maybe sell some trash?
				int items_to_sell_weight = tsi.to->ItemsToSellWeight();
				if(tsi.to->weight + item_weight - prev_item_weight - items_to_sell_weight > tsi.to->weight_max)
				{
					// don't try to get, will get overweight
					state = 0;
				}
			}

			if(state == 1)
			{
				if(slot.team_count == 0)
				{
					if(tsi.from == tsi.to)
					{
						// NPC own better item, just equip it
						state = 0;
						UpdateUnitInventory(*tsi.to);
					}
					else if(tsi.from->IsHero())
					{
						// NPC owns item and sells it to other NPC
						state = 0;
						if(tsi.to->gold >= tsi.item->value / 2)
						{
							tsi.to->AddItem(tsi.item, 1, false);
							int value = tsi.item->value / 2;
							tsi.to->gold -= value;
							tsi.from->gold += value;
							tsi.from->items.erase(tsi.from->items.begin() + tsi.index);
							UpdateUnitInventory(*tsi.to);
							CheckUnitOverload(*tsi.to);
						}
					}
					else
					{
						// PC owns item that NPC wants to buy
						if(tsi.to->gold >= tsi.item->value / 2)
						{
							if(distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
								state = 0;
							else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
							{
								if(IS_SET(tsi.to->data->flags, F_CRAZY))
									dialog = dialog_szalony_przedmiot_kup;
								else
									dialog = dialog_hero_przedmiot_kup;
							}
							else
								state = 2;
						}
						else
							state = 0;
					}
				}
				else
				{
					if(tsi.from->IsHero())
					{
						// NPC owns item that other NPC wants to take for credit, ask leader
						if(distance2d(tsi.to->pos, leader->pos) > 8.f)
							state = 0;
						else if(leader->busy == Unit::Busy_No && leader->player->action == PlayerController::Action_None)
						{
							if(IS_SET(tsi.to->data->flags, F_CRAZY))
								dialog = dialog_szalony_przedmiot;
							else
								dialog = dialog_hero_przedmiot;
						}
						else
							state = 2;
					}
					else
					{
						// PC owns item that other NPC wants to take for credit, ask him
						if(distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
							state = 0;
						else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
						{
							if(IS_SET(tsi.to->data->flags, F_CRAZY))
								dialog = dialog_szalony_przedmiot;
							else
								dialog = dialog_hero_przedmiot;
						}
						else
							state = 2;
					}
				}
			}
		}
	}

	if(state < 2)
	{
		if(state == 1)
		{
			// start dialog
			DialogContext& ctx = *(tsi.from->IsPlayer() ? tsi.from : leader)->player->dialog_ctx;
			ctx.team_share_id = team_share_id;
			ctx.team_share_item = tsi.from->items[tsi.index].item;
			StartDialog2(tsi.from->player, tsi.to, dialog);
		}

		++team_share_id;
		if(team_share_id == (int)team_shares.size())
			team_share_id = -1;
	}
}

//=================================================================================================
void Game::TeamShareGiveItemCredit(DialogContext& ctx)
{
	TeamShareItem& tsi = team_shares[ctx.team_share_id];
	if(CheckTeamShareItem(tsi))
	{
		if(tsi.from != tsi.to)
		{
			tsi.to->AddItem(tsi.item, 1, false);
			if(tsi.from->IsPlayer())
				tsi.from->weight -= tsi.item->weight;
			tsi.to->hero->credit += tsi.item->value / 2;
			CheckCredit(true);
			tsi.from->items.erase(tsi.from->items.begin() + tsi.index);
			if(!ctx.is_local && tsi.from == ctx.pc->unit)
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::REMOVE_ITEMS;
				c.pc = tsi.from->player;
				c.id = tsi.index;
				c.ile = 1;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
			UpdateUnitInventory(*tsi.to);
			CheckUnitOverload(*tsi.to);
		}
		else
		{
			tsi.to->hero->credit += tsi.item->value / 2;
			CheckCredit(true);
			UpdateUnitInventory(*tsi.to);
		}
	}
}

//=================================================================================================
void Game::TeamShareSellItem(DialogContext& ctx)
{
	TeamShareItem& tsi = team_shares[ctx.team_share_id];
	if(CheckTeamShareItem(tsi))
	{
		tsi.to->AddItem(tsi.item, 1, false);
		tsi.from->weight -= tsi.item->weight;
		int value = tsi.item->value / 2;
		tsi.to->gold += value;
		tsi.from->gold += value;
		tsi.from->items.erase(tsi.from->items.begin() + tsi.index);
		if(!ctx.is_local)
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::REMOVE_ITEMS;
			c.pc = tsi.from->player;
			c.id = tsi.index;
			c.ile = 1;
			GetPlayerInfo(c.pc).NeedUpdateAndGold();
		}
		UpdateUnitInventory(*tsi.to);
		CheckUnitOverload(*tsi.to);
	}
}

//=================================================================================================
void Game::TeamShareDecline(DialogContext& ctx)
{
	int share_id = ctx.team_share_id;
	TeamShareItem tsi = team_shares[share_id];
	if(CheckTeamShareItem(tsi))
	{
		ItemSlot& slot = tsi.from->items[tsi.index];
		if(slot.team_count == 0 || (tsi.from->IsPlayer() && tsi.from != leader))
		{
			// player don't want to sell/share this, remove other questions about this item from him
			for(vector<TeamShareItem>::iterator it = team_shares.begin()+share_id+1, end = team_shares.end(); it != end;)
			{
				if(tsi.from == it->from && tsi.item == it->item && CheckTeamShareItem(*it))
				{
					it = team_shares.erase(it);
					end = team_shares.end();
				}
				else
					++it;
			}
		}
		else
		{
			// leader don't want to share this item, remove other questions about this item from all npc (can only ask other pc's)
			for(vector<TeamShareItem>::iterator it = team_shares.begin()+share_id+1, end = team_shares.end(); it != end;)
			{
				if((tsi.from == leader || !tsi.from->IsPlayer()) && tsi.item == it->item && CheckTeamShareItem(*it))
				{
					it = team_shares.erase(it);
					end = team_shares.end();
				}
				else
					++it;
			}
		}
	}
}

//=================================================================================================
void Game::BuyTeamItems()
{
	const Item* hp1 = FindItem("p_hp");
	const Item* hp2 = FindItem("p_hp2");
	const Item* hp3 = FindItem("p_hp3");

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
			continue;

		// sprzedaj stare przedmioty, policz miksturki
		int ile_hp = 0, ile_hp2 = 0, ile_hp3 = 0;
		for(vector<ItemSlot>::iterator it2 = u.items.begin(), end2 = u.items.end(); it2 != end2;)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				if(it2->item == hp1)
					ile_hp += it2->count;
				else if(it2->item == hp2)
					ile_hp2 += it2->count;
				else if(it2->item == hp3)
					ile_hp3 += it2->count;
				++it2;
			}
			else if(it2->item->IsWearable() && it2->team_count == 0)
			{
				u.weight -= it2->item->weight;
				u.gold += it2->item->value / 2;
				if(it2 + 1 == end2)
				{
					u.items.pop_back();
					break;
				}
				else
				{
					it2 = u.items.erase(it2);
					end2 = u.items.end();
				}
			}
			else
				++it2;
		}

		// kup miksturki
		int p1, p2, p3;
		if(u.level < 4)
		{
			p1 = 5;
			p2 = 0;
			p3 = 0;
		}
		else if(u.level < 8)
		{
			p1 = 5;
			p2 = 2;
			p3 = 0;
		}
		else if(u.level < 12)
		{
			p1 = 6;
			p2 = 3;
			p3 = 1;
		}
		else if(u.level < 16)
		{
			p1 = 6;
			p2 = 4;
			p3 = 2;
		}
		else
		{
			p1 = 6;
			p2 = 5;
			p3 = 4;
		}

		while(ile_hp3 < p3 && u.gold >= hp3->value / 2)
		{
			u.AddItem(hp3, 1, false);
			u.gold -= hp3->value / 2;
			++ile_hp3;
		}
		while(ile_hp2 < p2 && (*it)->gold >= hp2->value / 2)
		{
			u.AddItem(hp2, 1, false);
			u.gold -= hp2->value / 2;
			++ile_hp2;
		}
		while(ile_hp < p1 && (*it)->gold >= hp1->value / 2)
		{
			u.AddItem(hp1, 1, false);
			u.gold -= hp1->value / 2;
			++ile_hp;
		}

		// darmowe miksturki dla biedaków
		int ile = p1 / 2 - ile_hp;
		if(ile > 0)
			u.AddItem(hp1, (uint)ile, false);
		ile = p2 / 2 - ile_hp2;
		if(ile > 0)
			u.AddItem(hp2, (uint)ile, false);
		ile = p3 / 2 - ile_hp3;
		if(ile > 0)
			u.AddItem(hp3, (uint)ile, false);

		// kup przedmioty
		const ItemList* lis = FindItemList("base_items").lis;
		// kup broñ
		if(!u.HaveWeapon())
			u.AddItem(UnitHelper::GetBaseWeapon(u, lis));
		else
		{
			const Item* weapon = u.slots[SLOT_WEAPON];
			while(true)
			{
				const Item* item = GetBetterItem(weapon);
				if(item && u.gold >= item->value)
				{
					if(u.IsBetterWeapon(item->ToWeapon()))
					{
						u.AddItem(item, 1, false);
						u.gold -= item->value;
						break;
					}
					else
						weapon = item;
				}
				else
					break;
			}
		}

		// kup ³uk
		const Item* item;
		if(!u.HaveBow())
			item = UnitHelper::GetBaseBow(lis);
		else
			item = GetBetterItem(&u.GetBow());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup pancerz
		if(!u.HaveArmor())
			item = UnitHelper::GetBaseArmor(u, lis);
		else
			item = GetBetterItem(&u.GetArmor());
		if(item && u.gold >= item->value && u.IsBetterArmor(item->ToArmor()))
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup tarcze
		if(!u.HaveShield())
			item = UnitHelper::GetBaseShield(lis);
		else
			item = GetBetterItem(&u.GetShield());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// za³ó¿ nowe przedmioty
		UpdateUnitInventory(u);
		u.ai->have_potion = 2;

		// sprzedaj stare przedmioty
		for(vector<ItemSlot>::iterator it2 = u.items.begin(), end2 = u.items.end(); it2 != end2;)
		{
			if(it2->item && it2->item->type != IT_CONSUMABLE && it2->item->IsWearable() && it2->team_count == 0)
			{
				u.weight -= it2->item->weight;
				u.gold += it2->item->value / 2;
				if(it2 + 1 == end2)
				{
					u.items.pop_back();
					break;
				}
				else
				{
					it2 = u.items.erase(it2);
					end2 = u.items.end();
				}
			}
			else
				++it2;
		}
	}

	// buying potions by old mage
	if(quest_mages2->scholar
		&& In(quest_mages2->mages_state, { Quest_Mages2::State::MageRecruited, Quest_Mages2::State::OldMageJoined, Quest_Mages2::State::OldMageRemembers, Quest_Mages2::State::BuyPotion }))
	{
		int ile = max(0, 3 - quest_mages2->scholar->CountItem(hp2));
		if(ile)
		{
			quest_mages2->scholar->AddItem(hp2, ile, false);
			quest_mages2->scholar->ai->have_potion = 2;
		}
	}

	// buying potions by orc
	if(quest_orcs2->orc && (quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined || quest_orcs2->orcs_state >= Quest_Orcs2::State::CompletedJoined))
	{
		int ile1, ile2;
		switch(quest_orcs2->orc_class)
		{
		case Quest_Orcs2::OrcClass::None:
			ile1 = 6;
			ile2 = 0;
			break;
		case Quest_Orcs2::OrcClass::Shaman:
			ile1 = 6;
			ile2 = 1;
			break;
		case Quest_Orcs2::OrcClass::Hunter:
			ile1 = 6;
			ile2 = 2;
			break;
		case Quest_Orcs2::OrcClass::Warrior:
			ile1 = 7;
			ile2 = 3;
			break;
		}

		int ile = max(0, ile1 - quest_orcs2->orc->CountItem(hp2));
		if(ile)
			quest_orcs2->orc->AddItem(hp2, ile, false);

		if(ile2)
		{
			ile = max(0, ile2 - quest_orcs2->orc->CountItem(hp3));
			if(ile)
				quest_orcs2->orc->AddItem(hp3, ile, false);
		}

		quest_orcs2->orc->ai->have_potion = 2;
	}

	// buying points for cleric
	if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss)
	{
		Unit* u = nullptr;
		UnitData* ud = FindUnitData("q_zlo_kaplan");
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			if((*it)->data == ud)
			{
				u = *it;
				break;
			}
		}

		if(u)
		{
			int ile = max(0, 5 - u->CountItem(hp2));
			if(ile)
			{
				u->AddItem(hp2, ile, false);
				u->ai->have_potion = 2;
			}
		}
	}
}

//=================================================================================================
void Game::ValidateTeamItems()
{
	if(!IsLocal())
		return;

	struct IVector
	{
		void* _Alval;
		void* _Myfirst;	// pointer to beginning of array
		void* _Mylast;	// pointer to current end of sequence
		void* _Myend;
	};

	int errors = 0;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->items.empty())
			continue;

		IVector* iv = (IVector*)&(*it)->items;
		if(!iv->_Myfirst)
		{
			ERROR(Format("Hero '%s' items._Myfirst = nullptr!", (*it)->GetName()));
			++errors;
			continue;
		}

		int index = 0;
		for(vector<ItemSlot>::iterator it2 = (*it)->items.begin(), end2 = (*it)->items.end(); it2 != end2; ++it2, ++index)
		{
			if(!it2->item)
			{
				ERROR(Format("Hero '%s' has nullptr item at index %d.", (*it)->GetName(), index));
				++errors;
			}
			else if(it2->item->IsStackable())
			{
				int index2 = index + 1;
				for(vector<ItemSlot>::iterator it3 = it2 + 1; it3 != end2; ++it3, ++index2)
				{
					if(it2->item == it3->item)
					{
						ERROR(Format("Hero '%s' has multiple stacks of %s, index %d and %d.", (*it)->GetName(), it2->item->id.c_str(), index, index2));
						++errors;
						break;
					}
				}
			}
		}
	}

	if(errors)
		AddGameMsg(Format("%d hero inventory errors!", errors), 10.f);
}

//-----------------------------------------------------------------------------
struct ItemToSell
{
	int index;
	float value;
	bool is_team;

	inline bool operator < (const ItemToSell& s)
	{
		if(is_team != s.is_team)
			return is_team > s.is_team;
		else
			return value > s.value;
	}
};
vector<ItemToSell> items_to_sell;

//=================================================================================================
// Only sell equippable items now
void Game::CheckUnitOverload(Unit& unit)
{
	if(unit.weight <= unit.weight_max)
		return;

	items_to_sell.clear();

	for(int i = 0, count = unit.items.size(); i < count; ++i)
	{
		ItemSlot& slot = unit.items[i];
		if(slot.item && slot.item->IsWearable() && !slot.item->IsQuest())
		{
			ItemToSell& to_sell = Add1(items_to_sell);
			to_sell.index = i;
			to_sell.is_team = (slot.team_count != 0);
			to_sell.value = slot.item->GetWeightValue();
		}
	}

	std::sort(items_to_sell.begin(), items_to_sell.end());

	int team_gold = 0;

	while(!items_to_sell.empty() && unit.weight > unit.weight_max)
	{
		ItemToSell& to_sell = items_to_sell.back();
		ItemSlot& slot = unit.items[to_sell.index];
		int price = GetItemPrice(slot.item, unit, false);
		if(slot.team_count == 0)
			unit.gold += price;
		else
			team_gold += price;
		unit.weight -= slot.item->weight;
		slot.item = nullptr;
		items_to_sell.pop_back();
	}

	if(team_gold > 0)
		AddGold(team_gold);

	assert(unit.weight <= unit.weight_max);
}
