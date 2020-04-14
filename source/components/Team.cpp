#include "Pch.h"
#include "Team.h"
#include "Unit.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Quest_Evil.h"
#include "Net.h"
#include "GameGui.h"
#include "TeamPanel.h"
#include "UnitHelper.h"
#include "AIController.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Game.h"
#include "GameMessages.h"
#include "ItemHelper.h"
#include "PlayerInfo.h"
#include "Level.h"
#include "EntityInterpolator.h"

Team* global::team;

//-----------------------------------------------------------------------------
// Team shares only work for equippable items, that have only 1 count in slot!
// NPCs first asks about best to worst weapons, then bows, then armor then shields
// if there are two same items it will check priority below
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// lower priority is better
enum TeamItemPriority
{
	PRIO_MY_ITEM,
	PRIO_MY_TEAM_ITEM,
	PRIO_NPC_TEAM_ITEM,
	PRIO_PC_TEAM_ITEM,
	PRIO_NPC_ITEM,
	PRIO_PC_ITEM
};

//-----------------------------------------------------------------------------
struct SortTeamShares
{
	bool operator () (const Team::TeamShareItem& t1, const Team::TeamShareItem& t2) const
	{
		if(t1.value == t2.value)
			return t1.priority < t2.priority;
		else
			return t1.value > t2.value;
	}
};

//-----------------------------------------------------------------------------
bool UniqueTeamShares(const Team::TeamShareItem& t1, const Team::TeamShareItem& t2)
{
	return t1.to == t2.to && t1.from == t2.from && t1.item == t2.item && t1.priority == t2.priority;
}

void Team::AddMember(Unit* unit, HeroType type)
{
	assert(unit && unit->hero);

	if(unit->IsTeamMember())
		return;

	// set as team member
	unit->hero->team_member = true;
	unit->hero->type = type;
	unit->OrderFollow(DialogContext::current ? DialogContext::current->pc->unit : leader);

	// add to team list
	if(type == HeroType::Normal)
	{
		if(GetActiveTeamSize() == 1u)
			active_members[0].MakeItemsTeam(false);
		active_members.push_back(unit);
	}
	members.push_back(unit);

	// set items as not team
	unit->MakeItemsTeam(false);

	// update TeamPanel if open
	if(game_gui->team->visible)
		game_gui->team->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::RECRUIT_NPC;
		c.unit = unit;
	}

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::RECRUIT, unit);
}

void Team::RemoveMember(Unit* unit)
{
	assert(unit && unit->hero);

	if(!unit->IsTeamMember())
		return;

	// set as not team member
	unit->hero->team_member = false;
	unit->OrderClear();

	// remove from team list
	if(unit->hero->type == HeroType::Normal)
		RemoveElementOrder(active_members.ptrs, unit);
	RemoveElementOrder(members.ptrs, unit);

	// set items as team
	unit->MakeItemsTeam(true);

	// update TeamPanel if open
	if(game_gui->team->visible)
		game_gui->team->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->id;
	}

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::KICK, unit);
}

Unit* Team::FindActiveTeamMember(int id)
{
	for(Unit& unit : active_members)
	{
		if(unit.id == id)
			return &unit;
	}

	return nullptr;
}

bool Team::FindItemInTeam(const Item* item, int quest_id, Unit** unit_result, int* i_index, bool check_npc)
{
	assert(item);

	for(Unit& unit : members)
	{
		if(unit.IsPlayer() || check_npc)
		{
			int index = unit.FindItem(item, quest_id);
			if(index != Unit::INVALID_IINDEX)
			{
				if(unit_result)
					*unit_result = &unit;
				if(i_index)
					*i_index = index;
				return true;
			}
		}
	}

	return false;
}

Unit* Team::FindTeamMember(cstring id)
{
	UnitData* unit_data = UnitData::Get(id);
	assert(unit_data);

	for(Unit& unit : members)
	{
		if(unit.data == unit_data)
			return &unit;
	}

	assert(0);
	return nullptr;
}

uint Team::GetActiveNpcCount()
{
	uint count = 0;
	for(Unit& unit : active_members)
	{
		if(!unit.player)
			++count;
	}
	return count;
}

uint Team::GetNpcCount()
{
	uint count = 0;
	for(Unit& unit : members)
	{
		if(!unit.player)
			++count;
	}
	return count;
}

Vec2 Team::GetShare()
{
	uint pc = 0, npc = 0;
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
			++pc;
		else
			++npc;
	}
	return GetShare(pc, npc);
}

Vec2 Team::GetShare(int pc, int npc)
{
	if(pc == 0)
	{
		if(npc == 0)
			return Vec2(1, 1);
		else
			return Vec2(0, 1.f / npc);
	}
	else
	{
		int total = pc * 2 + npc;
		float r = 1.f / total;
		return Vec2(r * 2, r);
	}
}

Unit* Team::GetRandomSaneHero()
{
	LocalVector<Unit*> v;

	for(Unit& unit : active_members)
	{
		if(unit.IsHero() && !IsSet(unit.data->flags, F_CRAZY))
			v->push_back(&unit);
	}

	return v->at(Rand() % v->size());
}

void Team::GetTeamInfo(TeamInfo& info)
{
	info.players = 0;
	info.npcs = 0;
	info.heroes = 0;
	info.sane_heroes = 0;
	info.insane_heroes = 0;
	info.free_members = 0;

	for(Unit& unit : members)
	{
		if(unit.IsPlayer())
			++info.players;
		else
		{
			++info.npcs;
			if(unit.IsHero())
			{
				if(unit.summoner)
				{
					++info.free_members;
					++info.summons;
				}
				else
				{
					if(unit.hero->type != HeroType::Normal)
						++info.free_members;
					else
					{
						++info.heroes;
						if(IsSet(unit.data->flags, F_CRAZY))
							++info.insane_heroes;
						else
							++info.sane_heroes;
					}
				}
			}
			else
				++info.free_members;
		}
	}
}

uint Team::GetPlayersCount()
{
	uint count = 0;
	for(Unit& unit : active_members)
	{
		if(unit.player)
			++count;
	}
	return count;
}

bool Team::HaveActiveNpc()
{
	for(Unit& unit : active_members)
	{
		if(!unit.player)
			return true;
	}
	return false;
}

bool Team::HaveNpc()
{
	for(Unit& unit : members)
	{
		if(!unit.player)
			return true;
	}
	return false;
}

bool Team::HaveOtherPlayer()
{
	bool first = true;
	for(Unit& unit : active_members)
	{
		if(unit.player)
		{
			if(!first)
				return true;
			first = false;
		}
	}
	return false;
}

bool Team::IsAnyoneAlive()
{
	for(Unit& unit : members)
	{
		if(unit.IsAlive() || unit.in_arena != -1)
			return true;
	}

	return false;
}

bool Team::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->team_member;
	else
		return false;
}

bool Team::IsTeamNotBusy()
{
	for(Unit& unit : members)
	{
		if(unit.busy)
			return false;
	}

	return true;
}

void Team::Load(GameReader& f)
{
	members.ptrs.resize(f.Read<uint>());
	for(Unit*& unit : members.ptrs)
		unit = Unit::GetById(f.Read<int>());

	active_members.ptrs.resize(f.Read<uint>());
	for(Unit*& unit : active_members.ptrs)
		unit = Unit::GetById(f.Read<int>());

	leader = Unit::GetById(f.Read<int>());
	if(LOAD_VERSION < V_0_7)
	{
		int team_gold;
		f >> team_gold;
		if(team_gold > 0)
		{
			Vec2 share = GetShare();
			for(Unit& unit : active_members)
			{
				float gold = (unit.IsPlayer() ? share.x : share.y) * team_gold;
				float gold_int, part = modf(gold, &gold_int);
				unit.gold += (int)gold_int;
				if(unit.IsPlayer())
					unit.player->split_gold = part;
				else
					unit.hero->split_gold = part;
			}
		}
	}
	f >> crazies_attack;
	f >> is_bandit;
	if(LOAD_VERSION >= V_0_8)
		f >> free_recruits;
	else
	{
		bool free_recruit;
		f >> free_recruit;
		if(free_recruit)
			free_recruits = 3 - team->GetActiveTeamSize();
		else
			free_recruits = 0;
	}

	CheckCredit(false, true);
	CalculatePlayersLevel();

	// apply leader requests
	for(Entity<Unit>* unit : leader_requests)
		*unit = leader;
	leader_requests.clear();
}

void Team::Reset()
{
	crazies_attack = false;
	is_bandit = false;
	free_recruits = 2;
}

void Team::ClearOnNewGameOrLoad()
{
	team_shares.clear();
	team_share_id = -1;
	if(Net::IsSingleplayer())
	{
		my_id = 0;
		leader_id = 0;
	}
}

void Team::Save(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit& unit : members)
		f << unit.id;

	f << GetActiveTeamSize();
	for(Unit& unit : active_members)
		f << unit.id;

	f << leader->id;
	f << crazies_attack;
	f << is_bandit;
	f << free_recruits;
}

void Team::SaveOnWorldmap(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit& unit : members)
		unit.Save(f, false);
}

void Team::Update(int days, bool travel)
{
	if(!travel)
	{
		for(Unit& unit : members)
		{
			if(unit.IsHero())
				unit.hero->PassTime(days);
		}
	}
	else
	{
		bool autoheal = (quest_mgr->quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_mgr->quest_evil->evil_state == Quest_Evil::State::KillBoss);

		// regeneracja hp / trenowanie
		for(Unit& unit : members)
		{
			if(autoheal)
				unit.hp = unit.hpmax;
			if(unit.IsPlayer())
				unit.player->Rest(1, false, true);
			else
				unit.hero->PassTime(1, true);
		}

		// ubywanie wolnych dni
		if(Net::IsOnline())
		{
			int max_days = 0;
			for(Unit& unit : active_members)
			{
				if(unit.IsPlayer() && unit.player->free_days > max_days)
					max_days = unit.player->free_days;
			}

			if(max_days > 0)
			{
				for(Unit& unit : active_members)
				{
					if(unit.IsPlayer() && unit.player->free_days == max_days)
						--unit.player->free_days;
				}
			}
		}
	}
}

//=================================================================================================
void Team::CheckTeamItemShares()
{
	if(!HaveActiveNpc())
	{
		team_share_id = -1;
		return;
	}

	team_shares.clear();
	uint pos_a, pos_b;

	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
			continue;

		pos_a = team_shares.size();

		for(Unit& other_unit : active_members)
		{
			int index = 0;
			for(ItemSlot& slot : other_unit.items)
			{
				if(slot.item && unit.CanWear(slot.item))
				{
					// don't check if can't buy
					if(slot.team_count == 0 && slot.item->value / 2 > unit.gold && &unit != &other_unit)
					{
						++index;
						continue;
					}

					int value, prev_value;
					if(unit.IsBetterItem(slot.item, &value, &prev_value))
					{
						float real_value = 1000.f * value * unit.stats->priorities[slot.item->type] / slot.item->value;
						if(real_value > 0)
						{
							TeamShareItem& tsi = Add1(team_shares);
							tsi.from = &other_unit;
							tsi.to = &unit;
							tsi.item = slot.item;
							tsi.index = index;
							tsi.value = real_value;
							tsi.is_team = (slot.team_count != 0);
							if(&unit == &other_unit)
							{
								if(slot.team_count == 0)
									tsi.priority = PRIO_MY_ITEM;
								else
									tsi.priority = PRIO_MY_TEAM_ITEM;
							}
							else if(slot.team_count != 0)
							{
								if(other_unit.IsPlayer())
									tsi.priority = PRIO_PC_TEAM_ITEM;
								else
									tsi.priority = PRIO_NPC_TEAM_ITEM;
							}
							else
							{
								if(other_unit.IsPlayer())
									tsi.priority = PRIO_PC_ITEM;
								else
									tsi.priority = PRIO_NPC_ITEM;
							}
						}
					}
				}
				++index;
			}
		}

		// remove duplictes
		pos_b = team_shares.size();
		if(pos_b - pos_a > 1)
		{
			std::vector<TeamShareItem>::iterator it2 = std::unique(team_shares.begin() + pos_a, team_shares.end(), UniqueTeamShares);
			team_shares.resize(std::distance(team_shares.begin(), it2));
		}
	}

	if(team_shares.empty())
		team_share_id = -1;
	else
	{
		team_share_id = 0;
		std::sort(team_shares.begin(), team_shares.end(), SortTeamShares());
	}
}

//=================================================================================================
bool Team::CheckTeamShareItem(TeamShareItem& tsi)
{
	int index = FindItemIndex(tsi.from->items, tsi.index, tsi.item, tsi.is_team);
	if(index == -1)
		return false;
	tsi.index = index;
	return true;
}

//=================================================================================================
void Team::UpdateTeamItemShares()
{
	if(game->fallback_type != FALLBACK::NO || team_share_id == -1)
		return;

	if(team_share_id >= (int)team_shares.size())
	{
		team_share_id = -1;
		return;
	}

	TeamShareItem& tsi = team_shares[team_share_id];
	int state = 1; // 0-no need to talk, 1-ask about share, 2-wait for time to talk
	GameDialog* dialog = nullptr;
	if(tsi.to->busy != Unit::Busy_No)
		state = 2;
	else if(!CheckTeamShareItem(tsi))
		state = 0;
	else
	{
		ITEM_SLOT target_slot;
		if(!tsi.to->IsBetterItem(tsi.item, nullptr, nullptr, &target_slot))
			state = 0;
		else
		{
			ItemSlot& slot = tsi.from->items[tsi.index];

			// new item weight - if it's already in inventory then it don't add weight
			int item_weight = (tsi.from != tsi.to ? slot.item->weight : 0);

			// old item, can be sold if overweight
			int prev_item_weight;
			if(tsi.to->slots[target_slot])
				prev_item_weight = tsi.to->slots[target_slot]->weight;
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
						tsi.to->UpdateInventory();
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
							tsi.to->UpdateInventory();
							CheckUnitOverload(*tsi.to);
						}
					}
					else
					{
						// PC owns item that NPC wants to buy
						if(tsi.to->gold >= tsi.item->value / 2)
						{
							if(Vec3::Distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
								state = 0;
							else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerAction::None)
								dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazy_buy_item" : "hero_buy_item");
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
						if(Vec3::Distance2d(tsi.to->pos, leader->pos) > 8.f)
							state = 0;
						else if(leader->busy == Unit::Busy_No && leader->player->action == PlayerAction::None)
							dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazy_get_item" : "hero_get_item");
						else
							state = 2;
					}
					else
					{
						// PC owns item that other NPC wants to take for credit, ask him
						if(Vec3::Distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
							state = 0;
						else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerAction::None)
							dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazy_get_item" : "hero_get_item");
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
			PlayerController* player_to_ask = (tsi.from->IsPlayer() ? tsi.from : leader)->player;
			DialogContext& ctx = *player_to_ask->dialog_ctx;
			ctx.team_share_id = team_share_id;
			ctx.team_share_item = tsi.from->items[tsi.index].item;
			player_to_ask->StartDialog(tsi.to, dialog);
		}

		++team_share_id;
		if(team_share_id == (int)team_shares.size())
			team_share_id = -1;
	}
}

//=================================================================================================
void Team::TeamShareGiveItemCredit(DialogContext& ctx)
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
				NetChangePlayer& c = Add1(tsi.from->player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS;
				c.id = tsi.index;
				c.count = 1;
			}
			tsi.to->UpdateInventory();
			CheckUnitOverload(*tsi.to);
		}
		else
		{
			tsi.to->hero->credit += tsi.item->value / 2;
			tsi.to->items[tsi.index].team_count = 0;
			CheckCredit(true);
			tsi.to->UpdateInventory();
		}
	}
}

//=================================================================================================
void Team::TeamShareSellItem(DialogContext& ctx)
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
			NetChangePlayer& c = Add1(tsi.from->player->player_info->changes);
			c.type = NetChangePlayer::REMOVE_ITEMS;
			c.id = tsi.index;
			c.count = 1;
			tsi.from->player->player_info->UpdateGold();
		}
		tsi.to->UpdateInventory();
		CheckUnitOverload(*tsi.to);
	}
}

//=================================================================================================
void Team::TeamShareDecline(DialogContext& ctx)
{
	int share_id = ctx.team_share_id;
	TeamShareItem tsi = team_shares[share_id];
	if(CheckTeamShareItem(tsi))
	{
		ItemSlot& slot = tsi.from->items[tsi.index];
		if(slot.team_count == 0 || (tsi.from->IsPlayer() && tsi.from != leader))
		{
			// player don't want to sell/share this, remove other questions about this item from him
			for(vector<TeamShareItem>::iterator it = team_shares.begin() + share_id + 1, end = team_shares.end(); it != end;)
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
			for(vector<TeamShareItem>::iterator it = team_shares.begin() + share_id + 1, end = team_shares.end(); it != end;)
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
void Team::BuyTeamItems()
{
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
			continue;

		const vector<pair<const Item*, int>>& pot_to_buy = unit.GetClass()->GetPotionEntry(unit.level).items;
		pot_have.resize(pot_to_buy.size());
		memset(pot_have.data(), 0, sizeof(int) * pot_to_buy.size());

		// sell old items, count potions
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2;)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				for(uint i = 0, count = pot_to_buy.size(); i < count; ++i)
				{
					if(pot_to_buy[i].first == it2->item)
					{
						pot_have[i] += it2->count;
						break;
					}
				}
				++it2;
			}
			else if(it2->item->IsWearable() && it2->team_count == 0)
			{
				unit.weight -= it2->item->weight;
				unit.gold += it2->item->value / 2;
				if(it2 + 1 == end2)
				{
					unit.items.pop_back();
					break;
				}
				else
				{
					it2 = unit.items.erase(it2);
					end2 = unit.items.end();
				}
			}
			else
				++it2;
		}

		// buy potions (half of potions if given for free to help poor heroes)
		for(uint i = 0, count = pot_to_buy.size(); i < count; ++i)
		{
			int& have = pot_have[i];
			int required = pot_to_buy[i].second;
			const Item* item = pot_to_buy[i].first;
			if(have < required / 2)
			{
				int count = required / 2 - have;
				unit.AddItem(item, count, false);
				have += count;
			}
			while(have < required && unit.gold >= item->value / 2)
			{
				unit.AddItem(item, 1, false);
				unit.gold -= item->value / 2;
				++have;
			}
		}

		// buy items
		const ItemList& lis = ItemList::Get("base_items");
		const float* priorities = unit.stats->priorities;
		to_buy.clear();
		for(int i = 0; i < IT_MAX_WEARABLE; ++i)
		{
			if(priorities[i] == 0)
				continue;

			ITEM_SLOT slot_type = ItemTypeToSlot((ITEM_TYPE)i);
			if(slot_type == SLOT_AMULET)
			{
				UnitHelper::BetterItem result = UnitHelper::GetBetterAmulet(unit);
				if(result.item)
				{
					float real_value = 1000.f * (result.value - result.prev_value) * priorities[IT_AMULET] / result.item->value;
					to_buy.push_back({ result.item, result.value, real_value });
				}
			}
			else if(slot_type == SLOT_RING1)
			{
				array<UnitHelper::BetterItem, 2> result = UnitHelper::GetBetterRings(unit);
				for(int i = 0; i < 2; ++i)
				{
					if(result[i].item)
					{
						float real_value = 1000.f * (result[i].value - result[i].prev_value) * priorities[IT_AMULET] / result[i].item->value;
						to_buy.push_back({ result[i].item, result[i].value, real_value });
					}
				}
			}
			else
			{
				const Item* item;
				if(!unit.slots[slot_type])
				{
					switch(i)
					{
					case IT_WEAPON:
						item = UnitHelper::GetBaseWeapon(unit, &lis);
						break;
					case IT_ARMOR:
						item = UnitHelper::GetBaseArmor(unit, &lis);
						break;
					default:
						item = UnitHelper::GetBaseItem((ITEM_TYPE)i, &lis);
						break;
					}
				}
				else
					item = ItemHelper::GetBetterItem(unit.slots[slot_type]);

				while(item && unit.gold >= item->value)
				{
					int value, prev_value;
					if(unit.IsBetterItem(item, &value, &prev_value))
					{
						float real_value = 1000.f * (value - prev_value) * priorities[IT_BOW] / item->value;
						to_buy.push_back({ item, (float)value, real_value });
					}
					item = ItemHelper::GetBetterItem(item);
				}
			}
		}

		if(to_buy.empty())
			continue;

		std::sort(to_buy.begin(), to_buy.end(), [](const ItemToBuy& a, const ItemToBuy& b)
		{
			return a.priority > b.priority;
		});

		// get best items to buy by priority & gold left
		to_buy2.clear();
		int gold_spent = 0;
		for(const ItemToBuy& buy : to_buy)
		{
			if(unit.gold - gold_spent >= buy.item->value)
			{
				bool add = true;
				if(!Any(buy.item->type, IT_AMULET, IT_RING))
				{
					for(auto it = to_buy2.begin(), end = to_buy2.end(); it != end; ++it)
					{
						const ItemToBuy& buy2 = *it;
						if(buy2.item->type == buy.item->type)
						{
							if(buy.ai_value > buy2.ai_value)
							{
								gold_spent -= buy2.item->value;
								to_buy2.erase(it);
							}
							else
								add = false;
							break;
						}
					}
				}
				if(add)
				{
					gold_spent += buy.item->value;
					to_buy2.push_back(buy);
				}
			}
		}

		for(const ItemToBuy& buy : to_buy2)
		{
			const Item* item = buy.item;
			if(unit.gold >= item->value)
			{
				unit.AddItem(item, 1, false);
				unit.gold -= item->value;
			}
		}

		// equip new items
		unit.UpdateInventory(false);
		if(unit.ai->have_potion != HavePotion::No)
			unit.ai->have_potion = HavePotion::Check;
		if(unit.ai->have_mp_potion != HavePotion::No)
			unit.ai->have_mp_potion = HavePotion::Check;

		// sell old items
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2;)
		{
			if(it2->item && it2->item->type != IT_CONSUMABLE && it2->item->IsWearable() && it2->team_count == 0)
			{
				unit.weight -= it2->item->weight;
				unit.gold += it2->item->value / 2;
				if(it2 + 1 == end2)
				{
					unit.items.pop_back();
					break;
				}
				else
				{
					it2 = unit.items.erase(it2);
					end2 = unit.items.end();
				}
			}
			else
				++it2;
		}
	}

	// free potions for some special team members
	for(Unit& unit : members)
	{
		if(unit.IsPlayer() || unit.summoner || unit.hero->type != HeroType::Free)
			continue;

		const vector<pair<const Item*, int>>& pot_to_buy = unit.GetClass()->GetPotionEntry(unit.level).items;
		pot_have.resize(pot_to_buy.size());
		memset(pot_have.data(), 0, sizeof(int) * pot_to_buy.size());

		// count potions
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2; ++it2)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				for(uint i = 0, count = pot_to_buy.size(); i < count; ++i)
				{
					if(pot_to_buy[i].first == it2->item)
					{
						pot_have[i] += it2->count;
						break;
					}
				}
			}
		}

		// give potions
		for(uint i = 0, count = pot_to_buy.size(); i < count; ++i)
		{
			int have = pot_have[i];
			int required = pot_to_buy[i].second;
			if(have < required)
			{
				int count = required - have;
				unit.AddItem(pot_to_buy[i].first, count, false);
			}
		}

		if(unit.ai->have_potion != HavePotion::No)
			unit.ai->have_potion = HavePotion::Check;
		if(unit.ai->have_mp_potion != HavePotion::No)
			unit.ai->have_mp_potion = HavePotion::Check;
	}
}

//-----------------------------------------------------------------------------
struct ItemToSell
{
	int index;
	float value;
	bool is_team;

	bool operator < (const ItemToSell& s)
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
void Team::CheckUnitOverload(Unit& unit)
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
		int price = ItemHelper::GetItemPrice(slot.item, unit, false);
		if(slot.team_count == 0)
			unit.gold += price;
		else
			team_gold += price;
		unit.weight -= slot.item->weight;
		slot.item = nullptr;
		items_to_sell.pop_back();
	}

	RemoveNullItems(unit.items);
	SortItems(unit.items);
	if(team_gold > 0)
		AddGold(team_gold);

	assert(unit.weight <= unit.weight_max);
}

//=================================================================================================
void Team::CheckCredit(bool require_update, bool ignore)
{
	if(GetActiveTeamSize() > 1)
	{
		int max_credit = 0;
		for(Unit& unit : active_members)
		{
			int credit = unit.GetCredit();
			if(credit < max_credit)
				max_credit = credit;
		}

		if(max_credit > 0)
		{
			require_update = true;
			for(Unit& unit : active_members)
				unit.GetCredit() -= max_credit;
		}
	}
	else
		active_members[0].player->credit = 0;

	if(!ignore && require_update && Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

//=================================================================================================
bool Team::RemoveQuestItem(const Item* item, int quest_id)
{
	Unit* unit;
	int slot_id;

	if(FindItemInTeam(item, quest_id, &unit, &slot_id))
	{
		unit->RemoveItem(slot_id, 1u);
		return true;
	}
	else
		return false;
}

//=================================================================================================
Unit* Team::FindPlayerTradingWithUnit(Unit& u)
{
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer() && unit.player->IsTradingWith(&u))
			return &unit;
	}

	return nullptr;
}

//=================================================================================================
void Team::AddLearningPoint(int count)
{
	assert(count >= 1);
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
			unit.player->AddLearningPoint(count);
	}
}

//=================================================================================================
// when exp is negative it doesn't depend of units count
void Team::AddExp(int exp, rvector<Unit>* units)
{
	if(!units)
		units = &members;

	if(exp > 0)
	{
		// count heroes - free heroes gain exp but not affect modifier
		uint count = 0;
		for(Unit& unit : *units)
		{
			if(unit.IsPlayer())
				++count;
			else if(unit.hero->type == HeroType::Normal)
				++count;
		}

		switch(count)
		{
		case 1:
			exp *= 2;
			break;
		case 2:
			exp = exp * 3 / 2;
			break;
		case 3:
			exp = exp * 5 / 4;
		case 4: // no change
			break;
		case 5:
			exp = int(0.75f * exp);
			break;
		case 6:
			exp = int(0.6f * exp);
			break;
		case 7:
			exp = exp / 2;
			break;
		case 8:
		default:
			exp = int(0.4f * exp);
			break;
		}
	}
	else
		exp = -exp;

	for(Unit& unit : *units)
	{
		if(unit.IsPlayer())
			unit.player->AddExp(exp);
		else if(unit.hero->type != HeroType::Visitor)
			unit.hero->AddExp(exp);
	}
}

//=================================================================================================
void Team::AddGold(int count, rvector<Unit>* units, bool show, bool is_quest)
{
	GMS msg = (is_quest ? GMS_QUEST_COMPLETED_GOLD : GMS_GOLD_ADDED);

	if(!units)
		units = &active_members;

	if(units->size() == 1)
	{
		Unit& u = units->front();
		u.gold += count;
		if(u.IsPlayer())
		{
			if(!u.player->is_local)
				u.player->player_info->UpdateGold();
			if(show)
				game_gui->messages->AddFormattedMessage(u.player, msg, -1, count);
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != game->pc->unit)
			{
				NetChangePlayer& c = Add1(trader->player->player_info->changes);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.id = u.id;
				c.count = u.gold;
			}
		}
		return;
	}

	int pc_count = 0, npc_count = 0;
	bool credit_info = false;

	for(Unit& unit : *units)
	{
		if(unit.IsPlayer())
		{
			++pc_count;
			unit.player->on_credit = false;
			unit.player->gold_get = 0;
		}
		else
		{
			++npc_count;
			unit.hero->on_credit = false;
			unit.hero->gained_gold = false;
		}
	}

	for(int i = 0; i < 2 && count > 0; ++i)
	{
		Vec2 share = GetShare(pc_count, npc_count);
		int gold_left = 0;

		for(Unit& unit : *units)
		{
			HeroPlayerCommon& hpc = *(unit.IsPlayer() ? (HeroPlayerCommon*)unit.player : unit.hero);
			if(hpc.on_credit)
				continue;

			float gain = (unit.IsPlayer() ? share.x : share.y) * count + hpc.split_gold;
			float gained_f;
			hpc.split_gold = modf(gain, &gained_f);
			int gained = (int)gained_f;
			if(hpc.credit > gained)
			{
				credit_info = true;
				hpc.credit -= gained;
				gold_left += gained;
				hpc.on_credit = true;
				if(unit.IsPlayer())
					--pc_count;
				else
					--npc_count;
			}
			else if(hpc.credit)
			{
				credit_info = true;
				gained -= hpc.credit;
				gold_left += hpc.credit;
				hpc.credit = 0;
				unit.gold += gained;
				if(unit.IsPlayer())
					unit.player->gold_get += gained;
				else
					unit.hero->gained_gold = true;
			}
			else
			{
				unit.gold += gained;
				if(unit.IsPlayer())
					unit.player->gold_get += gained;
				else
					unit.hero->gained_gold = true;
			}
		}

		count = gold_left;
	}

	for(Unit& unit : *units)
	{
		if(unit.IsPlayer())
		{
			if(unit.player->gold_get && !unit.player->is_local)
				unit.player->player_info->UpdateGold();
			if(show && (unit.player->gold_get || is_quest))
				game_gui->messages->AddFormattedMessage(unit.player, msg, -1, unit.player->gold_get);
		}
		else if(unit.hero->gained_gold && unit.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(unit);
			if(trader != game->pc->unit)
			{
				NetChangePlayer& c = Add1(trader->player->player_info->changes);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.id = unit.id;
				c.count = unit.gold;
			}
		}
	}

	if(Net::IsOnline() && credit_info)
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

//=================================================================================================
void Team::AddReward(int gold, int exp)
{
	AddGold(gold, nullptr, true, true);
	if(exp > 0)
		AddExp(exp);
}

//=================================================================================================
void Team::OnTravel(float dist)
{
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
			unit.player->TrainMove(dist);
	}
}

//=================================================================================================
void Team::CalculatePlayersLevel()
{
	Perk* leader_perk = Perk::Get("leader");
	bool have_leader_perk = false;
	players_level = -1;
	for(Unit& unit : active_members)
	{
		if(unit.IsPlayer())
		{
			if(unit.level > players_level)
				players_level = unit.level;
			if(unit.player->HavePerk(leader_perk))
				have_leader_perk = true;
		}
	}
	if(have_leader_perk)
		--players_level;
}

//=================================================================================================
uint Team::RemoveItem(const Item* item, uint count)
{
	uint total_removed = 0;
	for(Unit& unit : members)
	{
		uint removed = unit.RemoveItem(item, count);
		total_removed += removed;
		if(count != 0)
		{
			count -= removed;
			if(count == 0)
				break;
		}
	}
	return total_removed;
}

//=================================================================================================
void Team::SetBandit(bool is_bandit)
{
	if(this->is_bandit == is_bandit)
		return;
	this->is_bandit = is_bandit;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CHANGE_FLAGS);
}

//=================================================================================================
Unit* Team::GetNearestTeamMember(const Vec3& pos, float* out_dist)
{
	Unit* best = nullptr;
	float best_dist;
	for(Unit& unit : active_members)
	{
		float dist = Vec3::DistanceSquared(unit.pos, pos);
		if(!best || dist < best_dist)
		{
			best = &unit;
			best_dist = dist;
		}
	}
	if(out_dist)
		*out_dist = sqrtf(best_dist);
	return best;
}

//=================================================================================================
// czy ktokolwiek w dru¿ynie rozmawia
// bêdzie u¿ywane w multiplayer
bool Team::IsAnyoneTalking() const
{
	if(Net::IsLocal())
	{
		if(Net::IsOnline())
		{
			for(Unit& unit : team->active_members)
			{
				if(unit.IsPlayer() && unit.player->dialog_ctx->dialog_mode)
					return true;
			}
			return false;
		}
		else
			return game->dialog_context.dialog_mode;
	}
	else
		return anyone_talking;
}

//=================================================================================================
void Team::Warp(const Vec3& pos, const Vec3& look_at)
{
	// first warp leader to be in front
	game_level->WarpNearLocation(*game_level->local_area, *leader, pos, 2.f, true, 20);
	leader->visual_pos = leader->pos;
	leader->rot = Vec3::LookAtAngle(leader->pos, look_at);
	if(leader->interp)
		leader->interp->Reset(leader->pos, leader->rot);

	Vec3 dir = (pos - look_at).Normalized();
	Vec3 target_pos = pos + dir * 2;
	for(Unit& unit : members)
	{
		if(&unit == leader)
			continue;
		game_level->WarpNearLocation(*game_level->local_area, unit, target_pos, 2.f, true, 20);
		unit.visual_pos = unit.pos;
		unit.rot = Vec3::LookAtAngle(unit.pos, look_at);
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
	}
}
