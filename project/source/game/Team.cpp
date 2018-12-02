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
// items sort order:
// best to worst weapon, best to worst bow, best to worst armor, best to worst shield
const int item_type_prio[4] = {
	0, // IT_WEAPON
	1, // IT_BOW
	3, // IT_SHIELD
	2  // IT_ARMOR
};

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
	Unit* unit;

	explicit SortTeamShares(Unit* unit) : unit(unit)
	{
	}

	bool operator () (const TeamSingleton::TeamShareItem& t1, const TeamSingleton::TeamShareItem& t2) const
	{
		if(t1.item->type != t2.item->type)
		{
			int p1 = item_type_prio[t1.item->type];
			int p2 = item_type_prio[t2.item->type];
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
	unit->hero->mode = HeroData::Follow;

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
		float r = 1.f - npc * 0.1f;
		return Vec2(r / pc, 0.1f);
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
	f >> free_recruit;

	CheckCredit(false, true);
}

void TeamSingleton::Reset()
{
	crazies_attack = false;
	is_bandit = false;
	free_recruit = true;
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
	f << free_recruit;
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
				if(slot.item && slot.item->IsWearableByHuman())
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
		while(ile_hp2 < p2 && u.gold >= hp2->value / 2)
		{
			u.AddItem(hp2, 1, false);
			u.gold -= hp2->value / 2;
			++ile_hp2;
		}
		while(ile_hp < p1 && u.gold >= hp1->value / 2)
		{
			u.AddItem(hp1, 1, false);
			u.gold -= hp1->value / 2;
			++ile_hp;
		}

		// darmowe miksturki dla biedaków
		int count = p1 / 2 - ile_hp;
		if(count > 0)
			u.AddItem(hp1, (uint)count, false);
		count = p2 / 2 - ile_hp2;
		if(count > 0)
			u.AddItem(hp2, (uint)count, false);
		count = p3 / 2 - ile_hp3;
		if(count > 0)
			u.AddItem(hp3, (uint)count, false);

		// kup przedmioty
		const ItemList* lis = ItemList::Get("base_items").lis;
		// kup broñ
		if(!u.HaveWeapon())
			u.AddItem(UnitHelper::GetBaseWeapon(u, lis));
		else
		{
			const Item* weapon = u.slots[SLOT_WEAPON];
			while(true)
			{
				const Item* item = ItemHelper::GetBetterItem(weapon);
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
			item = ItemHelper::GetBetterItem(&u.GetBow());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup pancerz
		if(!u.HaveArmor())
			item = UnitHelper::GetBaseArmor(u, lis);
		else
			item = ItemHelper::GetBetterItem(&u.GetArmor());
		if(item && u.gold >= item->value && u.IsBetterArmor(item->ToArmor()))
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup tarcze
		if(!u.HaveShield())
			item = UnitHelper::GetBaseShield(lis);
		else
			item = ItemHelper::GetBetterItem(&u.GetShield());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// za³ó¿ nowe przedmioty
		u.UpdateInventory(false);
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

//=================================================================================================
void TeamSingleton::ValidateTeamItems()
{
	if(!Net::IsLocal())
		return;

	struct IVector
	{
		void* _Alval;
		void* _Myfirst;	// pointer to beginning of array
		void* _Mylast;	// pointer to current end of sequence
		void* _Myend;
	};

	int errors = 0;
	for(Unit* unit : active_members)
	{
		if(unit->items.empty())
			continue;

		IVector* iv = (IVector*)&unit->items;
		if(!iv->_Myfirst)
		{
			Error("Hero '%s' items._Myfirst = nullptr!", unit->GetName());
			++errors;
			continue;
		}

		int index = 0;
		for(vector<ItemSlot>::iterator it2 = unit->items.begin(), end2 = unit->items.end(); it2 != end2; ++it2, ++index)
		{
			if(!it2->item)
			{
				Error("Hero '%s' has nullptr item at index %d.", unit->GetName(), index);
				++errors;
			}
			else if(it2->item->IsStackable())
			{
				int index2 = index + 1;
				for(vector<ItemSlot>::iterator it3 = it2 + 1; it3 != end2; ++it3, ++index2)
				{
					if(it2->item == it3->item)
					{
						Error("Hero '%s' has multiple stacks of %s, index %d and %d.", unit->GetName(), it2->item->id.c_str(), index, index2);
						++errors;
						break;
					}
				}
			}
		}
	}

	if(errors)
		Game::Get().gui->messages->AddGameMsg(Format("%d hero inventory errors!", errors), 10.f);
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
	Game& game = Game::Get();

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
		game.AddGold(team_gold);

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
void TeamSingleton::AddExp(int exp, vector<Unit*>* units)
{
	if(!units)
		units = &active_members;

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

	for(Unit* unit : *units)
	{
		if(unit->IsPlayer())
			unit->player->AddExp(exp);
	}
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
