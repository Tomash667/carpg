#include "Pch.h"
#include "GameCore.h"
#include "Team.h"
#include "Unit.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Quest_Evil.h"
#include "Net.h"
#include "GlobalGui.h"
#include "TeamPanel.h"
#include "UnitHelper.h"
#include "AIController.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Game.h"
#include "GameMessages.h"
#include "ItemHelper.h"
#include "PlayerInfo.h"


TeamSingleton Team;

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
	int priorities[IT_MAX_WEARABLE];

	explicit SortTeamShares(Unit* unit)
	{
		// convert of list of item priority to rank of item priority
		const ITEM_TYPE* p = unit->stats->priorities;
		for(int i = 0; i < IT_MAX_WEARABLE; ++i)
			priorities[i] = -1;
		for(int i = 0; i < IT_MAX_WEARABLE; ++i)
		{
			if(p[i] == IT_NONE)
				break;
			priorities[p[i]] = i;
		}
	}

	bool operator () (const TeamSingleton::TeamShareItem& t1, const TeamSingleton::TeamShareItem& t2) const
	{
		if(t1.item->type != t2.item->type)
		{
			int p1 = priorities[t1.item->type];
			int p2 = priorities[t2.item->type];
			return p1 < p2;
		}
		else if(t1.value != t2.value)
			return t1.value > t2.value;
		else
			return t1.priority < t2.priority;
	}
};

//-----------------------------------------------------------------------------
bool UniqueTeamShares(const TeamSingleton::TeamShareItem& t1, const TeamSingleton::TeamShareItem& t2)
{
	return t1.to == t2.to && t1.from == t2.from && t1.item == t2.item && t1.priority == t2.priority;
}


void TeamSingleton::AddTeamMember(Unit* unit, bool free)
{
	assert(unit && unit->hero);

	// set as team member
	unit->hero->team_member = true;
	unit->hero->free = free;
	unit->SetOrder(ORDER_FOLLOW);

	// add to team list
	if(!free)
	{
		if(GetActiveTeamSize() == 1u)
			active_members[0]->MakeItemsTeam(false);
		active_members.push_back(unit);
	}
	members.push_back(unit);

	// set items as not team
	unit->MakeItemsTeam(false);

	// update TeamPanel if open
	Game& game = Game::Get();
	if(game.gui->team->visible)
		game.gui->team->Changed();

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

void TeamSingleton::RemoveTeamMember(Unit* unit)
{
	assert(unit && unit->hero);

	// set as not team member
	unit->hero->team_member = false;

	// remove from team list
	if(!unit->hero->free)
		RemoveElementOrder(active_members, unit);
	RemoveElementOrder(members, unit);

	// set items as team
	unit->MakeItemsTeam(true);

	// update TeamPanel if open
	Game& game = Game::Get();
	if(game.gui->team->visible)
		game.gui->team->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->netid;
	}

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::KICK, unit);
}

Unit* TeamSingleton::FindActiveTeamMember(int netid)
{
	for(Unit* unit : active_members)
	{
		if(unit->netid == netid)
			return unit;
	}

	return nullptr;
}

bool TeamSingleton::FindItemInTeam(const Item* item, int refid, Unit** unit_result, int* i_index, bool check_npc)
{
	assert(item);

	for(Unit* unit : members)
	{
		if(unit->IsPlayer() || check_npc)
		{
			int index = unit->FindItem(item, refid);
			if(index != Unit::INVALID_IINDEX)
			{
				if(unit_result)
					*unit_result = unit;
				if(i_index)
					*i_index = index;
				return true;
			}
		}
	}

	return false;
}

Unit* TeamSingleton::FindTeamMember(cstring id)
{
	UnitData* unit_data = UnitData::Get(id);
	assert(unit_data);

	for(Unit* unit : members)
	{
		if(unit->data == unit_data)
			return unit;
	}

	assert(0);
	return nullptr;
}

uint TeamSingleton::GetActiveNpcCount()
{
	uint count = 0;
	for(Unit* unit : active_members)
	{
		if(!unit->player)
			++count;
	}
	return count;
}

uint TeamSingleton::GetNpcCount()
{
	uint count = 0;
	for(Unit* unit : members)
	{
		if(!unit->player)
			++count;
	}
	return count;
}

Vec2 TeamSingleton::GetShare()
{
	uint pc = 0, npc = 0;
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
			++pc;
		else if(!unit->hero->free)
			++npc;
	}
	return GetShare(pc, npc);
}

Vec2 TeamSingleton::GetShare(int pc, int npc)
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

Unit* TeamSingleton::GetRandomSaneHero()
{
	LocalVector<Unit*> v;

	for(Unit* unit : active_members)
	{
		if(unit->IsHero() && !IS_SET(unit->data->flags, F_CRAZY))
			v->push_back(unit);
	}

	return v->at(Rand() % v->size());
}

void TeamSingleton::GetTeamInfo(TeamInfo& info)
{
	info.players = 0;
	info.npcs = 0;
	info.heroes = 0;
	info.sane_heroes = 0;
	info.insane_heroes = 0;
	info.free_members = 0;

	for(Unit* unit : members)
	{
		if(unit->IsPlayer())
			++info.players;
		else
		{
			++info.npcs;
			if(unit->IsHero())
			{
				if(unit->summoner != nullptr)
				{
					++info.free_members;
					++info.summons;
				}
				else
				{
					if(unit->hero->free)
						++info.free_members;
					else
					{
						++info.heroes;
						if(IS_SET(unit->data->flags, F_CRAZY))
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

uint TeamSingleton::GetPlayersCount()
{
	uint count = 0;
	for(Unit* unit : active_members)
	{
		if(unit->player)
			++count;
	}
	return count;
}

bool TeamSingleton::HaveActiveNpc()
{
	for(Unit* unit : active_members)
	{
		if(!unit->player)
			return true;
	}
	return false;
}

bool TeamSingleton::HaveNpc()
{
	for(Unit* unit : members)
	{
		if(!unit->player)
			return true;
	}
	return false;
}

bool TeamSingleton::HaveOtherPlayer()
{
	bool first = true;
	for(Unit* unit : active_members)
	{
		if(unit->player)
		{
			if(!first)
				return true;
			first = false;
		}
	}
	return false;
}

bool TeamSingleton::IsAnyoneAlive()
{
	for(Unit* unit : members)
	{
		if(unit->IsAlive() || unit->in_arena != -1)
			return true;
	}

	return false;
}

bool TeamSingleton::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->team_member;
	else
		return false;
}

bool TeamSingleton::IsTeamNotBusy()
{
	for(Unit* unit : members)
	{
		if(unit->busy)
			return false;
	}

	return true;
}

void TeamSingleton::Load(GameReader& f)
{
	members.resize(f.Read<uint>());
	for(Unit*& unit : members)
		unit = Unit::GetByRefid(f.Read<int>());

	active_members.resize(f.Read<uint>());
	for(Unit*& unit : active_members)
		unit = Unit::GetByRefid(f.Read<int>());

	leader = Unit::GetByRefid(f.Read<int>());
	if(LOAD_VERSION < V_0_7)
	{
		int team_gold;
		f >> team_gold;
		if(team_gold > 0)
		{
			Vec2 share = GetShare();
			for(Unit* unit : active_members)
			{
				float gold = (unit->IsPlayer() ? share.x : share.y) * team_gold;
				float gold_int, part = modf(gold, &gold_int);
				unit->gold += (int)gold_int;
				if(unit->IsPlayer())
					unit->player->split_gold = part;
				else
					unit->hero->split_gold = part;
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
			free_recruits = 3 - Team.GetActiveTeamSize();
		else
			free_recruits = 0;
	}

	CheckCredit(false, true);
	CalculatePlayersLevel();
}

void TeamSingleton::Reset()
{
	crazies_attack = false;
	is_bandit = false;
	free_recruits = 2;
}

void TeamSingleton::ClearOnNewGameOrLoad()
{
	team_shares.clear();
	team_share_id = -1;
}

void TeamSingleton::Save(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit* unit : members)
		f << unit->refid;

	f << GetActiveTeamSize();
	for(Unit* unit : active_members)
		f << unit->refid;

	f << leader->refid;
	f << crazies_attack;
	f << is_bandit;
	f << free_recruits;
}

void TeamSingleton::SaveOnWorldmap(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit* unit : members)
	{
		unit->Save(f, false);
		unit->refid = (int)Unit::refid_table.size();
		Unit::refid_table.push_back(unit);
	}
}

void TeamSingleton::Update(int days, bool travel)
{
	if(!travel)
	{
		for(Unit* unit : members)
		{
			if(unit->IsHero())
				unit->hero->PassTime(days);
		}
	}
	else
	{
		bool autoheal = (QM.quest_evil->evil_state == Quest_Evil::State::ClosingPortals || QM.quest_evil->evil_state == Quest_Evil::State::KillBoss);

		// regeneracja hp / trenowanie
		for(Unit* unit : members)
		{
			if(autoheal)
				unit->hp = unit->hpmax;
			if(unit->IsPlayer())
				unit->player->Rest(1, false, true);
			else
				unit->hero->PassTime(1, true);
		}

		// ubywanie wolnych dni
		if(Net::IsOnline())
		{
			int maks = 0;
			for(Unit* unit : active_members)
			{
				if(unit->IsPlayer() && unit->player->free_days > maks)
					maks = unit->player->free_days;
			}

			if(maks > 0)
			{
				for(Unit* unit : active_members)
				{
					if(unit->IsPlayer() && unit->player->free_days == maks)
						--unit->player->free_days;
				}
			}
		}
	}
}

//=================================================================================================
void TeamSingleton::CheckTeamItemShares()
{
	if(!HaveActiveNpc())
	{
		team_share_id = -1;
		return;
	}

	team_shares.clear();
	uint pos_a, pos_b;

	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
			continue;

		pos_a = team_shares.size();

		for(Unit* other_unit : active_members)
		{
			int index = 0;
			for(ItemSlot& slot : other_unit->items)
			{
				if(slot.item && slot.item->IsWearableByHuman() && slot.item->type != IT_AMULET && slot.item->type != IT_RING)
				{
					// don't check if can't buy
					if(slot.team_count == 0 && slot.item->value / 2 > unit->gold && unit != other_unit)
					{
						++index;
						continue;
					}

					int value;
					if(unit->IsBetterItem(slot.item, &value))
					{
						TeamShareItem& tsi = Add1(team_shares);
						tsi.from = other_unit;
						tsi.to = unit;
						tsi.item = slot.item;
						tsi.index = index;
						tsi.value = value;
						tsi.is_team = (slot.team_count != 0);
						if(unit == other_unit)
						{
							if(slot.team_count == 0)
								tsi.priority = PRIO_MY_ITEM;
							else
								tsi.priority = PRIO_MY_TEAM_ITEM;
						}
						else
						{
							if(slot.team_count != 0)
							{
								if(other_unit->IsPlayer())
									tsi.priority = PRIO_PC_TEAM_ITEM;
								else
									tsi.priority = PRIO_NPC_TEAM_ITEM;
							}
							else
							{
								if(other_unit->IsPlayer())
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
bool TeamSingleton::CheckTeamShareItem(TeamShareItem& tsi)
{
	int index = FindItemIndex(tsi.from->items, tsi.index, tsi.item, tsi.is_team);
	if(index == -1)
		return false;
	tsi.index = index;
	return true;
}

//=================================================================================================
void TeamSingleton::UpdateTeamItemShares()
{
	Game& game = Game::Get();
	if(game.fallback_type != FALLBACK::NO || team_share_id == -1)
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
		ItemSlot& slot = tsi.from->items[tsi.index];
		if(!tsi.to->IsBetterItem(tsi.item))
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
							else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
								dialog = GameDialog::TryGet(IS_SET(tsi.to->data->flags, F_CRAZY) ? "crazy_buy_item" : "hero_buy_item");
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
						else if(leader->busy == Unit::Busy_No && leader->player->action == PlayerController::Action_None)
							dialog = GameDialog::TryGet(IS_SET(tsi.to->data->flags, F_CRAZY) ? "crazy_get_item" : "hero_get_item");
						else
							state = 2;
					}
					else
					{
						// PC owns item that other NPC wants to take for credit, ask him
						if(Vec3::Distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
							state = 0;
						else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
							dialog = GameDialog::TryGet(IS_SET(tsi.to->data->flags, F_CRAZY) ? "crazy_get_item" : "hero_get_item");
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
void TeamSingleton::TeamShareGiveItemCredit(DialogContext& ctx)
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
void TeamSingleton::TeamShareSellItem(DialogContext& ctx)
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
void TeamSingleton::TeamShareDecline(DialogContext& ctx)
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
void TeamSingleton::BuyTeamItems()
{
	const Item* hp1 = Item::Get("p_hp");
	const Item* hp2 = Item::Get("p_hp2");
	const Item* hp3 = Item::Get("p_hp3");

	for(Unit* unit : active_members)
	{
		Unit& u = *unit;
		if(u.IsPlayer())
			continue;

		// sell old items, count potions
		int hp1count = 0, hp2count = 0, hp3count = 0;
		for(vector<ItemSlot>::iterator it2 = u.items.begin(), end2 = u.items.end(); it2 != end2;)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				if(it2->item == hp1)
					hp1count += it2->count;
				else if(it2->item == hp2)
					hp2count += it2->count;
				else if(it2->item == hp3)
					hp3count += it2->count;
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

		// buy potions
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

		while(hp3count < p3 && u.gold >= hp3->value / 2)
		{
			u.AddItem(hp3, 1, false);
			u.gold -= hp3->value / 2;
			++hp3count;
		}
		while(hp2count < p2 && u.gold >= hp2->value / 2)
		{
			u.AddItem(hp2, 1, false);
			u.gold -= hp2->value / 2;
			++hp2count;
		}
		while(hp1count < p1 && u.gold >= hp1->value / 2)
		{
			u.AddItem(hp1, 1, false);
			u.gold -= hp1->value / 2;
			++hp1count;
		}

		// free potions for poor heroes
		int count = p1 / 2 - hp1count;
		if(count > 0)
			u.AddItem(hp1, (uint)count, false);
		count = p2 / 2 - hp2count;
		if(count > 0)
			u.AddItem(hp2, (uint)count, false);
		count = p3 / 2 - hp3count;
		if(count > 0)
			u.AddItem(hp3, (uint)count, false);

		// buy items
		const ItemList* lis = ItemList::Get("base_items").lis;
		const ITEM_TYPE* priorities = unit->stats->priorities;
		const Item* item;
		for(int i = 0; i < IT_MAX_WEARABLE; ++i)
		{
			switch(priorities[i])
			{
			case IT_WEAPON:
				if(!u.HaveWeapon())
					u.AddItem(UnitHelper::GetBaseWeapon(u, lis));
				else
				{
					const Item* weapon = u.slots[SLOT_WEAPON];
					while(true)
					{
						item = ItemHelper::GetBetterItem(weapon);
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
				break;
			case IT_BOW:
				if(!u.HaveBow())
					item = UnitHelper::GetBaseBow(lis);
				else
					item = ItemHelper::GetBetterItem(&u.GetBow());
				if(item && u.gold >= item->value)
				{
					u.AddItem(item, 1, false);
					u.gold -= item->value;
				}
				break;
			case IT_SHIELD:
				if(!u.HaveShield())
					item = UnitHelper::GetBaseShield(lis);
				else
					item = ItemHelper::GetBetterItem(&u.GetShield());
				if(item && u.gold >= item->value)
				{
					u.AddItem(item, 1, false);
					u.gold -= item->value;
				}
				break;
			case IT_ARMOR:
				if(!u.HaveArmor())
					item = UnitHelper::GetBaseArmor(u, lis);
				else
					item = ItemHelper::GetBetterItem(&u.GetArmor());
				if(item && u.gold >= item->value && u.IsBetterArmor(item->ToArmor()))
				{
					u.AddItem(item, 1, false);
					u.gold -= item->value;
				}
				break;
			case IT_NONE:
				// ai don't buy useless amulets/rings yet
				break;
			}
		}

		// equip new items
		u.UpdateInventory(false);
		u.ai->have_potion = 2;

		// sell old items
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
	if(QM.quest_mages2->scholar && Any(QM.quest_mages2->mages_state, Quest_Mages2::State::MageRecruited, Quest_Mages2::State::OldMageJoined,
		Quest_Mages2::State::OldMageRemembers, Quest_Mages2::State::BuyPotion))
	{
		int count = max(0, 3 - QM.quest_mages2->scholar->CountItem(hp2));
		if(count)
		{
			QM.quest_mages2->scholar->AddItem(hp2, count, false);
			QM.quest_mages2->scholar->ai->have_potion = 2;
		}
	}

	// buying potions by orc
	if(QM.quest_orcs2->orc
		&& (QM.quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined || QM.quest_orcs2->orcs_state >= Quest_Orcs2::State::CompletedJoined))
	{
		int ile1, ile2;
		switch(QM.quest_orcs2->GetOrcClass())
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

		int count = max(0, ile1 - QM.quest_orcs2->orc->CountItem(hp2));
		if(count)
			QM.quest_orcs2->orc->AddItem(hp2, count, false);

		if(ile2)
		{
			count = max(0, ile2 - QM.quest_orcs2->orc->CountItem(hp3));
			if(count)
				QM.quest_orcs2->orc->AddItem(hp3, count, false);
		}

		QM.quest_orcs2->orc->ai->have_potion = 2;
	}

	// buying points for cleric
	if(QM.quest_evil->evil_state == Quest_Evil::State::ClosingPortals || QM.quest_evil->evil_state == Quest_Evil::State::KillBoss)
	{
		Unit* u = FindTeamMember("q_zlo_kaplan");

		if(u)
		{
			int count = max(0, 5 - u->CountItem(hp2));
			if(count)
			{
				u->AddItem(hp2, count, false);
				u->ai->have_potion = 2;
			}
		}
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
void TeamSingleton::CheckUnitOverload(Unit& unit)
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
		__assume(slot.item != nullptr);
		int price = ItemHelper::GetItemPrice(slot.item, unit, false);
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

//=================================================================================================
void TeamSingleton::CheckCredit(bool require_update, bool ignore)
{
	if(GetActiveTeamSize() > 1)
	{
		int max_credit = active_members.front()->GetCredit();

		for(vector<Unit*>::iterator it = active_members.begin() + 1, end = active_members.end(); it != end; ++it)
		{
			int credit = (*it)->GetCredit();
			if(credit < max_credit)
				max_credit = credit;
		}

		if(max_credit > 0)
		{
			require_update = true;
			for(vector<Unit*>::iterator it = active_members.begin(), end = active_members.end(); it != end; ++it)
				(*it)->GetCredit() -= max_credit;
		}
	}
	else
		active_members[0]->player->credit = 0;

	if(!ignore && require_update && Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

//=================================================================================================
bool TeamSingleton::RemoveQuestItem(const Item* item, int refid)
{
	Unit* unit;
	int slot_id;

	if(FindItemInTeam(item, refid, &unit, &slot_id))
	{
		unit->RemoveItem(slot_id, 1u);
		return true;
	}
	else
		return false;
}

//=================================================================================================
Unit* TeamSingleton::FindPlayerTradingWithUnit(Unit& u)
{
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer() && unit->player->IsTradingWith(&u))
			return unit;
	}

	return nullptr;
}

//=================================================================================================
void TeamSingleton::AddLearningPoint(int count)
{
	assert(count >= 1);
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
			unit->player->AddLearningPoint(count);
	}
}

//=================================================================================================
// when exp is negative it doesn't depend of units count
void TeamSingleton::AddExp(int exp, vector<Unit*>* units)
{
	if(!units)
		units = &active_members;

	if(exp > 0)
	{
		switch(units->size())
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

	for(Unit* unit : *units)
	{
		if(unit->IsPlayer())
			unit->player->AddExp(exp);
		else
			unit->hero->AddExp(exp);
	}
}

//=================================================================================================
void TeamSingleton::AddGold(int count, vector<Unit*>* units, bool show, cstring msg, float time, bool defmsg)
{
	Game& game = Game::Get();

	if(!msg)
		msg = game.txGoldPlus;

	if(!units)
		units = &active_members;

	if(units->size() == 1)
	{
		Unit& u = *(*units)[0];
		u.gold += count;
		if(show && u.IsPlayer())
		{
			if(&u == game.pc->unit)
				game.gui->messages->AddGameMsg(Format(msg, count), time);
			else
			{
				NetChangePlayer& c = Add1(u.player->player_info->changes);
				c.type = NetChangePlayer::GOLD_MSG;
				c.id = (defmsg ? 1 : 0);
				c.count = count;
				u.player->player_info->UpdateGold();
			}
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != game.pc->unit)
			{
				NetChangePlayer& c = Add1(trader->player->player_info->changes);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.id = u.netid;
				c.count = u.gold;
			}
		}
		return;
	}

	int pc_count = 0, npc_count = 0;
	bool credit_info = false;

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
		{
			++pc_count;
			u.player->on_credit = false;
			u.player->gold_get = 0;
		}
		else
		{
			++npc_count;
			u.hero->on_credit = false;
			u.hero->gained_gold = false;
		}
	}

	for(int i = 0; i < 2 && count > 0; ++i)
	{
		Vec2 share = GetShare(pc_count, npc_count);
		int gold_left = 0;

		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			HeroPlayerCommon& hpc = *(u.IsPlayer() ? (HeroPlayerCommon*)u.player : u.hero);
			if(hpc.on_credit)
				continue;

			float gain = (u.IsPlayer() ? share.x : share.y) * count + hpc.split_gold;
			float gained_f;
			hpc.split_gold = modf(gain, &gained_f);
			int gained = (int)gained_f;
			if(hpc.credit > gained)
			{
				credit_info = true;
				hpc.credit -= gained;
				gold_left += gained;
				hpc.on_credit = true;
				if(u.IsPlayer())
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
				u.gold += gained;
				if(u.IsPlayer())
					u.player->gold_get += gained;
				else
					u.hero->gained_gold = true;
			}
			else
			{
				u.gold += gained;
				if(u.IsPlayer())
					u.player->gold_get += gained;
				else
					u.hero->gained_gold = true;
			}
		}

		count = gold_left;
	}

	if(Net::IsOnline())
	{
		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsPlayer())
			{
				if(u.player != game.pc)
				{
					if(u.player->gold_get)
					{
						u.player->player_info->update_flags |= PlayerInfo::UF_GOLD;
						if(show)
						{
							NetChangePlayer& c = Add1(u.player->player_info->changes);
							c.type = NetChangePlayer::GOLD_MSG;
							c.id = (defmsg ? 1 : 0);
							c.count = u.player->gold_get;
						}
					}
				}
				else
				{
					if(show)
						game.gui->messages->AddGameMsg(Format(msg, game.pc->gold_get), time);
				}
			}
			else if(u.hero->gained_gold && u.busy == Unit::Busy_Trading)
			{
				Unit* trader = FindPlayerTradingWithUnit(u);
				if(trader != game.pc->unit)
				{
					NetChangePlayer& c = Add1(trader->player->player_info->changes);
					c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
					c.id = u.netid;
					c.count = u.gold;
				}
			}
		}

		if(credit_info)
			Net::PushChange(NetChange::UPDATE_CREDIT);
	}
	else if(show)
		game.gui->messages->AddGameMsg(Format(msg, game.pc->gold_get), time);
}

//=================================================================================================
void TeamSingleton::AddReward(int gold, int exp)
{
	Game& game = Game::Get();
	AddGold(gold, nullptr, true, game.txQuestCompletedGold, 4.f, false);
	if(exp > 0)
		AddExp(exp);
}

//=================================================================================================
void TeamSingleton::OnTravel(float dist)
{
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
			unit->player->TrainMove(dist);
	}
}

//=================================================================================================
void TeamSingleton::CalculatePlayersLevel()
{
	bool have_leader_perk = false;
	players_level = -1;
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
		{
			if(unit->level > players_level)
				players_level = unit->level;
			if(unit->player->HavePerk(Perk::Leader))
				have_leader_perk = true;
		}
	}
	if(have_leader_perk)
		--players_level;
}

//=================================================================================================
uint TeamSingleton::RemoveItem(const Item* item, uint count)
{
	uint total_removed = 0;
	for(Unit* unit : members)
	{
		uint removed = unit->RemoveItem(item, count);
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
void TeamSingleton::SetBandit(bool is_bandit)
{
	if(this->is_bandit == is_bandit)
		return;
	this->is_bandit = is_bandit;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CHANGE_FLAGS);
}

//=================================================================================================
Unit* TeamSingleton::GetNearestTeamMember(const Vec3& pos, float* out_dist)
{
	Unit* best = nullptr;
	float best_dist;
	for(Unit* unit : active_members)
	{
		float dist = Vec3::DistanceSquared(unit->pos, pos);
		if(!best || dist < best_dist)
		{
			best = unit;
			best_dist = dist;
		}
	}
	if(out_dist)
		*out_dist = sqrtf(best_dist);
	return best;
}
