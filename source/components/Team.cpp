#include "Pch.h"
#include "Team.h"

#include "AIController.h"
#include "AITeam.h"
#include "BitStreamFunc.h"
#include "EntityInterpolator.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "ItemHelper.h"
#include "Level.h"
#include "Net.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Evil.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "TeamPanel.h"
#include "Unit.h"
#include "UnitHelper.h"

Team* team;

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
	unit->hero->teamMember = true;
	unit->hero->type = type;
	unit->OrderFollow(DialogContext::current ? DialogContext::current->pc->unit : leader);
	if(unit->hero->otherTeam)
		unit->hero->otherTeam->Remove(unit);

	// add to team list
	if(type == HeroType::Normal)
	{
		if(GetActiveTeamSize() == 1u)
			activeMembers[0].MakeItemsTeam(false);
		activeMembers.push_back(unit);
	}
	members.push_back(unit);

	// set items as not team
	unit->MakeItemsTeam(false);

	// update TeamPanel if open
	if(gameGui->team->visible)
		gameGui->team->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::RECRUIT_NPC);
		c.unit = unit;
	}

	if(unit->eventHandler)
		unit->eventHandler->HandleUnitEvent(UnitEventHandler::RECRUIT, unit);
}

void Team::RemoveMember(Unit* unit)
{
	assert(unit && unit->hero);

	if(!unit->IsTeamMember())
		return;

	// set as not team member
	unit->hero->teamMember = false;
	unit->OrderClear();

	// remove from team list
	if(unit->hero->type == HeroType::Normal)
	{
		RemoveElementOrder(activeMembers.ptrs, unit);
		if(unit->hero->investment)
		{
			unit->gold += unit->hero->investment;
			DialogContext::current->pc->unit->ModGold(-unit->hero->investment);
			unit->hero->investment = 0;
		}
	}
	RemoveElementOrder(members.ptrs, unit);

	// set items as team
	unit->MakeItemsTeam(true);

	// update TeamPanel if open
	if(gameGui->team->visible)
		gameGui->team->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::KICK_NPC);
		c.id = unit->id;
	}

	if(unit->eventHandler)
		unit->eventHandler->HandleUnitEvent(UnitEventHandler::KICK, unit);
}

Unit* Team::FindActiveTeamMember(int id)
{
	for(Unit& unit : activeMembers)
	{
		if(unit.id == id)
			return &unit;
	}

	return nullptr;
}

bool Team::FindItemInTeam(const Item* item, int questId, Unit** unitResult, int* outIndex, bool checkNpc)
{
	assert(item);

	for(Unit& unit : members)
	{
		if(unit.IsPlayer() || checkNpc)
		{
			int index = unit.FindItem(item, questId);
			if(index != Unit::INVALID_IINDEX)
			{
				if(unitResult)
					*unitResult = &unit;
				if(outIndex)
					*outIndex = index;
				return true;
			}
		}
	}

	return false;
}

Unit* Team::FindTeamMember(cstring id)
{
	UnitData* unitData = UnitData::Get(id);
	assert(unitData);

	for(Unit& unit : members)
	{
		if(unit.data == unitData)
			return &unit;
	}

	assert(0);
	return nullptr;
}

uint Team::GetActiveNpcCount()
{
	uint count = 0;
	for(Unit& unit : activeMembers)
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
	for(Unit& unit : activeMembers)
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

	for(Unit& unit : activeMembers)
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
	info.saneHeroes = 0;
	info.insaneHeroes = 0;
	info.freeMembers = 0;

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
					++info.freeMembers;
					++info.summons;
				}
				else
				{
					if(unit.hero->type != HeroType::Normal)
						++info.freeMembers;
					else
					{
						++info.heroes;
						if(IsSet(unit.data->flags, F_CRAZY))
							++info.insaneHeroes;
						else
							++info.saneHeroes;
					}
				}
			}
			else
				++info.freeMembers;
		}
	}
}

uint Team::GetPlayersCount()
{
	uint count = 0;
	for(Unit& unit : activeMembers)
	{
		if(unit.player)
			++count;
	}
	return count;
}

bool Team::HaveActiveNpc()
{
	for(Unit& unit : activeMembers)
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
	for(Unit& unit : activeMembers)
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

bool Team::HaveClass(Class* clas) const
{
	assert(clas);
	for(const Unit& unit : activeMembers)
	{
		if(unit.GetClass() == clas)
			return true;
	}
	return false;
}

bool Team::IsAnyoneAlive()
{
	for(Unit& unit : members)
	{
		if(unit.IsAlive() || unit.inArena != -1)
			return true;
	}

	return false;
}

bool Team::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->teamMember;
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

void Team::Clear(bool newGame)
{
	teamShares.clear();
	teamShareId = -1;
	if(Net::IsSingleplayer())
	{
		myId = 0;
		leaderId = 0;
	}

	if(newGame)
	{
		craziesAttack = false;
		isBandit = false;
		freeRecruits = 2;
		checkResults.clear();
		investments.clear();
	}
}

void Team::Save(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit& unit : members)
		f << unit.id;

	f << GetActiveTeamSize();
	for(Unit& unit : activeMembers)
		f << unit.id;

	f << leader->id;
	f << craziesAttack;
	f << isBandit;
	f << freeRecruits;
	f << checkResults;

	f << investments.size();
	for(Investment& investment : investments)
	{
		f << investment.questId;
		f << investment.gold;
		f << investment.days;
		f << investment.name;
	}
}

void Team::SaveOnWorldmap(GameWriter& f)
{
	f << GetTeamSize();
	for(Unit& unit : members)
		unit.Save(f);
}

void Team::Load(GameReader& f)
{
	members.ptrs.resize(f.Read<uint>());
	for(Unit*& unit : members.ptrs)
		unit = Unit::GetById(f.Read<int>());

	activeMembers.ptrs.resize(f.Read<uint>());
	for(Unit*& unit : activeMembers.ptrs)
		unit = Unit::GetById(f.Read<int>());

	leader = Unit::GetById(f.Read<int>());
	f >> craziesAttack;
	f >> isBandit;
	f >> freeRecruits;

	if(LOAD_VERSION >= V_0_17)
		f >> checkResults;
	else
		checkResults.clear();

	if(LOAD_VERSION >= V_0_18)
	{
		investments.resize(f.Read<uint>());
		for(Investment& investment : investments)
		{
			f >> investment.questId;
			f >> investment.gold;
			f >> investment.days;
			f >> investment.name;
		}
	}
	else
		investments.clear();

	CheckCredit(false, true);
	CalculatePlayersLevel();
}

void Team::Update(int days, UpdateMode mode)
{
	if(mode == UM_NORMAL)
	{
		for(Unit& unit : members)
		{
			if(unit.IsHero())
				unit.hero->PassTime(days);
		}
	}
	else if(mode == UM_TRAVEL)
	{
		bool autoheal = false;
		for(Unit& unit : members)
		{
			Class* clas = unit.GetClass();
			if(clas && IsSet(clas->flags, Class::F_AUTOHEAL))
			{
				autoheal = true;
				break;
			}
		}

		// healing, training
		for(Unit& unit : members)
		{
			if(autoheal)
				unit.hp = unit.hpmax;
			if(unit.IsPlayer())
				unit.player->Rest(1, false, true);
			else
				unit.hero->PassTime(1, true);
		}

		// remove free days
		if(Net::IsOnline())
		{
			int maxDays = 0;
			for(Unit& unit : activeMembers)
			{
				if(unit.IsPlayer() && unit.player->freeDays > maxDays)
					maxDays = unit.player->freeDays;
			}

			if(maxDays > 0)
			{
				for(Unit& unit : activeMembers)
				{
					if(unit.IsPlayer() && unit.player->freeDays == maxDays)
						--unit.player->freeDays;
				}
			}
		}
	}

	// investments
	if(!isBandit)
	{
		int income = 0;
		for(Investment& investment : investments)
		{
			investment.days += days;
			int count = investment.days / 30;
			if(count)
			{
				investment.days -= count * 30;
				income += count * investment.gold;
			}
		}

		if(income)
		{
			if(HaveActiveNpc())
			{
				const int investment = int(GetShare().y * income);
				for(Unit& unit : activeMembers)
				{
					if(!unit.IsPlayer())
						unit.hero->investment = Max(unit.hero->investment - investment, 0);
				}
			}

			AddGold(income, nullptr, true);
		}
	}

	checkResults.clear();
}

//=================================================================================================
void Team::CheckTeamItemShares()
{
	if(!HaveActiveNpc())
	{
		teamShareId = -1;
		return;
	}

	teamShares.clear();
	uint posA, posB;

	for(Unit& unit : activeMembers)
	{
		if(unit.IsPlayer())
			continue;

		posA = teamShares.size();

		for(Unit& otherUnit : activeMembers)
		{
			int index = 0;
			for(ItemSlot& slot : otherUnit.items)
			{
				if(slot.item && unit.CanWear(slot.item))
				{
					// don't check if can't buy
					if(slot.teamCount == 0 && slot.item->value / 2 > unit.gold && &unit != &otherUnit)
					{
						++index;
						continue;
					}

					int value, prevValue;
					if(unit.IsBetterItem(slot.item, &value, &prevValue))
					{
						float realValue = 1000.f * value * unit.stats->priorities[slot.item->type];
						if(realValue > 0)
						{
							TeamShareItem& tsi = Add1(teamShares);
							tsi.from = &otherUnit;
							tsi.to = &unit;
							tsi.item = slot.item;
							tsi.index = index;
							tsi.value = realValue;
							tsi.isTeam = (slot.teamCount != 0);
							if(&unit == &otherUnit)
							{
								if(slot.teamCount == 0)
									tsi.priority = PRIO_MY_ITEM;
								else
									tsi.priority = PRIO_MY_TEAM_ITEM;
							}
							else if(slot.teamCount != 0)
							{
								if(otherUnit.IsPlayer())
									tsi.priority = PRIO_PC_TEAM_ITEM;
								else
									tsi.priority = PRIO_NPC_TEAM_ITEM;
							}
							else
							{
								if(otherUnit.IsPlayer())
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
		posB = teamShares.size();
		if(posB - posA > 1)
		{
			std::vector<TeamShareItem>::iterator it2 = std::unique(teamShares.begin() + posA, teamShares.end(), UniqueTeamShares);
			teamShares.resize(std::distance(teamShares.begin(), it2));
		}
	}

	if(teamShares.empty())
		teamShareId = -1;
	else
	{
		teamShareId = 0;
		std::sort(teamShares.begin(), teamShares.end(), SortTeamShares());
	}
}

//=================================================================================================
bool Team::CheckTeamShareItem(TeamShareItem& tsi)
{
	int index = FindItemIndex(tsi.from->items, tsi.index, tsi.item, tsi.isTeam);
	if(index == -1)
		return false;
	tsi.index = index;
	return true;
}

//=================================================================================================
void Team::UpdateTeamItemShares()
{
	if(game->fallbackType != FALLBACK::NO || teamShareId == -1)
		return;

	if(teamShareId >= (int)teamShares.size())
	{
		teamShareId = -1;
		return;
	}

	TeamShareItem& tsi = teamShares[teamShareId];
	int state = 1; // 0-no need to talk, 1-ask about share, 2-wait for time to talk
	GameDialog* dialog = nullptr;
	if(tsi.to->busy != Unit::Busy_No)
		state = 2;
	else if(!CheckTeamShareItem(tsi))
		state = 0;
	else
	{
		ITEM_SLOT targetSlot;
		if(!tsi.to->IsBetterItem(tsi.item, nullptr, nullptr, &targetSlot))
			state = 0;
		else
		{
			ItemSlot& slot = tsi.from->items[tsi.index];

			// new item weight - if it's already in inventory then it don't add weight
			int itemWeight = (tsi.from != tsi.to ? slot.item->weight : 0);

			// old item, can be sold if overweight
			int prevItemWeight;
			if(tsi.to->HaveEquippedItem(targetSlot))
				prevItemWeight = tsi.to->GetEquippedItem(targetSlot)->weight;
			else
				prevItemWeight = 0;

			if(tsi.to->weight + itemWeight - prevItemWeight > tsi.to->weightMax)
			{
				// unit will be overweighted, maybe sell some trash?
				int itemsToSellWeight = tsi.to->ItemsToSellWeight();
				if(tsi.to->weight + itemWeight - prevItemWeight - itemsToSellWeight > tsi.to->weightMax)
				{
					// don't try to get, will get overweight
					state = 0;
				}
			}

			if(state == 1)
			{
				if(slot.teamCount == 0)
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
								dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazyBuyItem" : "heroBuyItem");
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
							dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazyGetItem" : "heroGetItem");
						else
							state = 2;
					}
					else
					{
						// PC owns item that other NPC wants to take for credit, ask him
						if(Vec3::Distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
							state = 0;
						else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerAction::None)
							dialog = GameDialog::TryGet(IsSet(tsi.to->data->flags, F_CRAZY) ? "crazyGetItem" : "heroGetItem");
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
			PlayerController* playerToAsk = (tsi.from->IsPlayer() ? tsi.from : leader)->player;
			DialogContext& ctx = *playerToAsk->dialogCtx;
			ctx.teamShareId = teamShareId;
			ctx.teamShareItem = tsi.from->items[tsi.index].item;
			playerToAsk->StartDialog(tsi.to, dialog);
		}

		++teamShareId;
		if(teamShareId == (int)teamShares.size())
			teamShareId = -1;
	}
}

//=================================================================================================
void Team::TeamShareGiveItemCredit(DialogContext& ctx)
{
	TeamShareItem& tsi = teamShares[ctx.teamShareId];
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
			if(!ctx.isLocal && tsi.from == ctx.pc->unit)
			{
				NetChangePlayer& c = tsi.from->player->playerInfo->PushChange(NetChangePlayer::REMOVE_ITEMS);
				c.id = tsi.index;
				c.count = 1;
			}
			tsi.to->UpdateInventory();
			CheckUnitOverload(*tsi.to);
		}
		else
		{
			tsi.to->hero->credit += tsi.item->value / 2;
			tsi.to->items[tsi.index].teamCount = 0;
			CheckCredit(true);
			tsi.to->UpdateInventory();
		}
	}
}

//=================================================================================================
void Team::TeamShareSellItem(DialogContext& ctx)
{
	TeamShareItem& tsi = teamShares[ctx.teamShareId];
	if(CheckTeamShareItem(tsi))
	{
		tsi.to->AddItem(tsi.item, 1, false);
		tsi.from->weight -= tsi.item->weight;
		int value = tsi.item->value / 2;
		tsi.to->gold += value;
		tsi.from->gold += value;
		tsi.from->items.erase(tsi.from->items.begin() + tsi.index);
		if(!ctx.isLocal)
		{
			NetChangePlayer& c = tsi.from->player->playerInfo->PushChange(NetChangePlayer::REMOVE_ITEMS);
			c.id = tsi.index;
			c.count = 1;
			tsi.from->player->playerInfo->UpdateGold();
		}
		tsi.to->UpdateInventory();
		CheckUnitOverload(*tsi.to);
	}
}

//=================================================================================================
void Team::TeamShareDecline(DialogContext& ctx)
{
	int shareId = ctx.teamShareId;
	TeamShareItem tsi = teamShares[shareId];
	if(CheckTeamShareItem(tsi))
	{
		ItemSlot& slot = tsi.from->items[tsi.index];
		if(slot.teamCount == 0 || (tsi.from->IsPlayer() && tsi.from != leader))
		{
			// player don't want to sell/share this, remove other questions about this item from him
			for(vector<TeamShareItem>::iterator it = teamShares.begin() + shareId + 1, end = teamShares.end(); it != end;)
			{
				if(tsi.from == it->from && tsi.item == it->item && CheckTeamShareItem(*it))
				{
					it = teamShares.erase(it);
					end = teamShares.end();
				}
				else
					++it;
			}
		}
		else
		{
			// leader don't want to share this item, remove other questions about this item from all npc (can only ask other pc's)
			for(vector<TeamShareItem>::iterator it = teamShares.begin() + shareId + 1, end = teamShares.end(); it != end;)
			{
				if((tsi.from == leader || !tsi.from->IsPlayer()) && tsi.item == it->item && CheckTeamShareItem(*it))
				{
					it = teamShares.erase(it);
					end = teamShares.end();
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
	for(Unit& unit : activeMembers)
	{
		if(unit.IsPlayer())
			continue;

		const vector<pair<const Item*, int>>& potToBuy = unit.GetClass()->GetPotionEntry(unit.level).items;
		potHave.resize(potToBuy.size());
		memset(potHave.data(), 0, sizeof(int) * potToBuy.size());

		// sell old items, count potions
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2;)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				for(uint i = 0, count = potToBuy.size(); i < count; ++i)
				{
					if(potToBuy[i].first == it2->item)
					{
						potHave[i] += it2->count;
						break;
					}
				}
				++it2;
			}
			else if(it2->item->IsWearable() && it2->teamCount == 0)
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
		for(uint i = 0, count = potToBuy.size(); i < count; ++i)
		{
			int& have = potHave[i];
			int required = potToBuy[i].second;
			const Item* item = potToBuy[i].first;
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
		const ItemList& lis = ItemList::Get("baseItems");
		const float* priorities = unit.stats->priorities;
		toBuy.clear();
		for(int i = 0; i < IT_MAX_WEARABLE; ++i)
		{
			if(priorities[i] == 0)
				continue;

			ITEM_SLOT slotType = ItemTypeToSlot((ITEM_TYPE)i);
			if(slotType == SLOT_AMULET)
			{
				UnitHelper::BetterItem result = UnitHelper::GetBetterAmulet(unit);
				if(result.item)
				{
					float realValue = 1000.f * (result.value - result.prevValue) * priorities[IT_AMULET] / result.item->value;
					toBuy.push_back({ result.item, result.value, realValue });
				}
			}
			else if(slotType == SLOT_RING1)
			{
				array<UnitHelper::BetterItem, 2> result = UnitHelper::GetBetterRings(unit);
				for(int i = 0; i < 2; ++i)
				{
					if(result[i].item)
					{
						float realValue = 1000.f * (result[i].value - result[i].prevValue) * priorities[IT_AMULET] / result[i].item->value;
						toBuy.push_back({ result[i].item, result[i].value, realValue });
					}
				}
			}
			else
			{
				const Item* item;
				if(!unit.HaveEquippedItem(slotType))
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
					item = ItemHelper::GetBetterItem(unit.GetEquippedItem(slotType));

				while(item && unit.gold >= item->value)
				{
					int value, prevValue;
					if(unit.IsBetterItem(item, &value, &prevValue))
					{
						float realValue = 1000.f * (value - prevValue) * priorities[IT_BOW] / item->value;
						toBuy.push_back({ item, (float)value, realValue });
					}
					item = ItemHelper::GetBetterItem(item);
				}
			}
		}

		if(toBuy.empty())
			continue;

		std::sort(toBuy.begin(), toBuy.end(), [](const ItemToBuy& a, const ItemToBuy& b)
		{
			return a.priority > b.priority;
		});

		// get best items to buy by priority & gold left
		toBuy2.clear();
		int goldSpent = 0;
		for(const ItemToBuy& buy : toBuy)
		{
			if(unit.gold - goldSpent >= buy.item->value)
			{
				bool add = true;
				if(!Any(buy.item->type, IT_AMULET, IT_RING))
				{
					for(auto it = toBuy2.begin(), end = toBuy2.end(); it != end; ++it)
					{
						const ItemToBuy& buy2 = *it;
						if(buy2.item->type == buy.item->type)
						{
							if(buy.aiValue > buy2.aiValue)
							{
								goldSpent -= buy2.item->value;
								toBuy2.erase(it);
							}
							else
								add = false;
							break;
						}
					}
				}
				if(add)
				{
					goldSpent += buy.item->value;
					toBuy2.push_back(buy);
				}
			}
		}

		for(const ItemToBuy& buy : toBuy2)
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
		if(unit.ai->havePotion != HavePotion::No)
			unit.ai->havePotion = HavePotion::Check;
		if(unit.ai->haveMpPotion != HavePotion::No)
			unit.ai->haveMpPotion = HavePotion::Check;

		// sell old items
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2;)
		{
			if(it2->item && it2->item->type != IT_CONSUMABLE && it2->item->IsWearable() && it2->teamCount == 0)
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

		const vector<pair<const Item*, int>>& potToBuy = unit.GetClass()->GetPotionEntry(unit.level).items;
		potHave.resize(potToBuy.size());
		memset(potHave.data(), 0, sizeof(int) * potToBuy.size());

		// count potions
		for(vector<ItemSlot>::iterator it2 = unit.items.begin(), end2 = unit.items.end(); it2 != end2; ++it2)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMABLE)
			{
				for(uint i = 0, count = potToBuy.size(); i < count; ++i)
				{
					if(potToBuy[i].first == it2->item)
					{
						potHave[i] += it2->count;
						break;
					}
				}
			}
		}

		// give potions
		for(uint i = 0, count = potToBuy.size(); i < count; ++i)
		{
			int have = potHave[i];
			int required = potToBuy[i].second;
			if(have < required)
			{
				int count = required - have;
				unit.AddItem(potToBuy[i].first, count, false);
			}
		}

		if(unit.ai->havePotion != HavePotion::No)
			unit.ai->havePotion = HavePotion::Check;
		if(unit.ai->haveMpPotion != HavePotion::No)
			unit.ai->haveMpPotion = HavePotion::Check;
	}
}

//-----------------------------------------------------------------------------
struct ItemToSell
{
	int index;
	float value;
	bool isTeam;

	bool operator < (const ItemToSell& s)
	{
		if(isTeam != s.isTeam)
			return isTeam > s.isTeam;
		else
			return value > s.value;
	}
};
vector<ItemToSell> itemsToSell;

//=================================================================================================
// Only sell equippable items now
void Team::CheckUnitOverload(Unit& unit)
{
	if(unit.weight <= unit.weightMax)
		return;

	itemsToSell.clear();

	for(int i = 0, count = unit.items.size(); i < count; ++i)
	{
		ItemSlot& slot = unit.items[i];
		if(slot.item && slot.item->IsWearable() && !slot.item->IsQuest())
		{
			ItemToSell& toSell = Add1(itemsToSell);
			toSell.index = i;
			toSell.isTeam = (slot.teamCount != 0);
			toSell.value = slot.item->GetWeightValue();
		}
	}

	std::sort(itemsToSell.begin(), itemsToSell.end());

	int teamGold = 0;

	while(!itemsToSell.empty() && unit.weight > unit.weightMax)
	{
		ItemToSell& toSell = itemsToSell.back();
		ItemSlot& slot = unit.items[toSell.index];
		int price = ItemHelper::GetItemPrice(slot.item, unit, false);
		if(slot.teamCount == 0)
			unit.gold += price;
		else
			teamGold += price;
		unit.weight -= slot.item->weight;
		slot.item = nullptr;
		itemsToSell.pop_back();
	}

	RemoveNullItems(unit.items);
	SortItems(unit.items);
	if(teamGold > 0)
		AddGold(teamGold);

	assert(unit.weight <= unit.weightMax);
}

//=================================================================================================
void Team::CheckCredit(bool requireUpdate, bool ignore)
{
	if(GetActiveTeamSize() > 1)
	{
		int maxCredit = 0;
		for(Unit& unit : activeMembers)
		{
			int credit = unit.GetCredit();
			if(credit < maxCredit)
				maxCredit = credit;
		}

		if(maxCredit > 0)
		{
			requireUpdate = true;
			for(Unit& unit : activeMembers)
				unit.GetCredit() -= maxCredit;
		}
	}
	else
		activeMembers[0].player->credit = 0;

	if(!ignore && requireUpdate && Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

//=================================================================================================
bool Team::RemoveQuestItem(const Item* item, int questId)
{
	Unit* unit;
	int slotId;

	if(FindItemInTeam(item, questId, &unit, &slotId))
	{
		unit->RemoveItem(slotId, 1u);
		return true;
	}
	else
		return false;
}

//=================================================================================================
Unit* Team::FindPlayerTradingWithUnit(Unit& u)
{
	for(Unit& unit : activeMembers)
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
	for(Unit& unit : activeMembers)
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
void Team::AddGold(int count, rvector<Unit>* units, bool show, bool isQuest)
{
	GMS msg = (isQuest ? GMS_QUEST_COMPLETED_GOLD : GMS_GOLD_ADDED);

	if(!units)
		units = &activeMembers;

	if(units->size() == 1)
	{
		Unit& u = units->front();
		u.gold += count;
		if(u.IsPlayer())
		{
			if(!u.player->isLocal)
				u.player->playerInfo->UpdateGold();
			if(show)
				gameGui->messages->AddFormattedMessage(u.player, msg, -1, count);
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != game->pc->unit)
			{
				NetChangePlayer& c = trader->player->playerInfo->PushChange(NetChangePlayer::UPDATE_TRADER_GOLD);
				c.id = u.id;
				c.count = u.gold;
			}
		}
		return;
	}

	int pcCount = 0, npcCount = 0;
	bool creditInfo = false;

	for(Unit& unit : *units)
	{
		if(unit.IsPlayer())
		{
			++pcCount;
			unit.player->onCredit = false;
			unit.player->goldGet = 0;
		}
		else
		{
			++npcCount;
			unit.hero->onCredit = false;
			unit.hero->gainedGold = false;
		}
	}

	for(int i = 0; i < 2 && count > 0; ++i)
	{
		Vec2 share = GetShare(pcCount, npcCount);
		int goldLeft = 0;

		for(Unit& unit : *units)
		{
			HeroPlayerCommon& hpc = *(unit.IsPlayer() ? (HeroPlayerCommon*)unit.player : unit.hero);
			if(hpc.onCredit)
				continue;

			float gain = (unit.IsPlayer() ? share.x : share.y) * count + hpc.splitGold;
			float gainedFloat;
			hpc.splitGold = modf(gain, &gainedFloat);
			int gained = (int)gainedFloat;
			if(hpc.credit > gained)
			{
				creditInfo = true;
				hpc.credit -= gained;
				goldLeft += gained;
				hpc.onCredit = true;
				if(unit.IsPlayer())
					--pcCount;
				else
					--npcCount;
			}
			else if(hpc.credit)
			{
				creditInfo = true;
				gained -= hpc.credit;
				goldLeft += hpc.credit;
				hpc.credit = 0;
				unit.gold += gained;
				if(unit.IsPlayer())
					unit.player->goldGet += gained;
				else
					unit.hero->gainedGold = true;
			}
			else
			{
				unit.gold += gained;
				if(unit.IsPlayer())
					unit.player->goldGet += gained;
				else
					unit.hero->gainedGold = true;
			}
		}

		count = goldLeft;
	}

	for(Unit& unit : *units)
	{
		if(unit.IsPlayer())
		{
			if(unit.player->goldGet && !unit.player->isLocal)
				unit.player->playerInfo->UpdateGold();
			if(show && (unit.player->goldGet || isQuest))
				gameGui->messages->AddFormattedMessage(unit.player, msg, -1, unit.player->goldGet);
		}
		else if(unit.hero->gainedGold && unit.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(unit);
			if(trader != game->pc->unit)
			{
				NetChangePlayer& c = trader->player->playerInfo->PushChange(NetChangePlayer::UPDATE_TRADER_GOLD);
				c.id = unit.id;
				c.count = unit.gold;
			}
		}
	}

	if(Net::IsOnline() && creditInfo)
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
	for(Unit& unit : activeMembers)
	{
		if(unit.IsPlayer())
			unit.player->TrainMove(dist);
	}
}

//=================================================================================================
void Team::CalculatePlayersLevel()
{
	Perk* leaderPerk = Perk::Get("leader");
	bool haveLeaderPerk = false;
	playersLevel = -1;
	for(Unit& unit : activeMembers)
	{
		if(unit.IsPlayer())
		{
			if(unit.level > playersLevel)
				playersLevel = unit.level;
			if(unit.player->HavePerk(leaderPerk))
				haveLeaderPerk = true;
		}
	}
	if(haveLeaderPerk)
		--playersLevel;
}

//=================================================================================================
uint Team::RemoveItem(const Item* item, uint count)
{
	uint totalRemoved = 0;
	for(Unit& unit : members)
	{
		uint removed = unit.RemoveItem(item, count);
		totalRemoved += removed;
		if(count != 0)
		{
			count -= removed;
			if(count == 0)
				break;
		}
	}
	return totalRemoved;
}

//=================================================================================================
void Team::SetBandit(bool isBandit)
{
	if(this->isBandit == isBandit)
		return;
	this->isBandit = isBandit;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CHANGE_FLAGS);
}

//=================================================================================================
Unit* Team::GetNearestTeamMember(const Vec3& pos, float* outDist)
{
	Unit* best = nullptr;
	float bestDist;
	for(Unit& unit : activeMembers)
	{
		float dist = Vec3::DistanceSquared(unit.pos, pos);
		if(!best || dist < bestDist)
		{
			best = &unit;
			bestDist = dist;
		}
	}
	if(outDist)
		*outDist = sqrtf(bestDist);
	return best;
}

//=================================================================================================
// czy ktokolwiek w dru�ynie rozmawia
// b�dzie u�ywane w multiplayer
bool Team::IsAnyoneTalking() const
{
	if(Net::IsLocal())
	{
		if(Net::IsOnline())
		{
			for(Unit& unit : team->activeMembers)
			{
				if(unit.IsPlayer() && unit.player->dialogCtx->dialogMode)
					return true;
			}
			return false;
		}
		else
			return game->dialogContext.dialogMode;
	}
	else
		return anyoneTalking;
}

//=================================================================================================
void Team::Warp(const Vec3& pos, const Vec3& lookAt)
{
	// first warp leader to be in front
	gameLevel->WarpNearLocation(*gameLevel->localPart, *leader, pos, 2.f, true, 20);
	leader->visualPos = leader->pos;
	leader->rot = Vec3::LookAtAngle(leader->pos, lookAt);
	if(leader->interp)
		leader->interp->Reset(leader->pos, leader->rot);

	Vec3 dir = (pos - lookAt).Normalized();
	Vec3 targetPos = pos + dir * 2;
	for(Unit& unit : members)
	{
		if(&unit == leader)
			continue;
		gameLevel->WarpNearLocation(*gameLevel->localPart, unit, targetPos, 2.f, true, 20);
		unit.visualPos = unit.pos;
		unit.rot = Vec3::LookAtAngle(unit.pos, lookAt);
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
	}
}

//=================================================================================================
int Team::GetStPoints() const
{
	int pts = 0;
	for(const Unit& member : members)
		pts += member.level;
	return pts;
}

//=================================================================================================
bool Team::PersuasionCheck(int level)
{
	// if succeded - don't check
	// if failed - increase difficulty
	PlayerController* me = DialogContext::current->pc;
	Unit* talker = DialogContext::current->talker;
	CheckResult* existingResult = nullptr;
	for(CheckResult& check : checkResults)
	{
		if(check.unit == talker)
		{
			if(check.success)
			{
				gameGui->messages->AddGameMsg3(me, GMS_PERSUASION_SUCCESS);
				return true;
			}
			else
			{
				existingResult = &check;
				level += 25 * check.tries;
				break;
			}
		}
	}

	// find best skill value among nearby team members
	Unit* bestUnit = me->unit;
	int bestValue = me->unit->Get(SkillId::PERSUASION) + me->unit->Get(AttributeId::CHA) - 50;
	for(Unit& rUnit : members)
	{
		Unit* unit = &rUnit;
		if(unit == me->unit || unit->locPart != me->unit->locPart || Vec3::Distance(unit->pos, me->unit->pos) > 10.f)
			continue;
		int value = unit->Get(SkillId::PERSUASION) + unit->Get(AttributeId::CHA) - 50;
		if(value > bestValue)
		{
			bestValue = value;
			bestUnit = unit;
		}
	}

	// do skill check
	int chance = UnitHelper::CalculateChance(bestValue, level - 25, level + 25);
	const bool success = (Rand() % 100 < chance);

	// train player
	me->Train(TrainWhat::Persuade, 0.f, chance);
	if(me->unit != bestUnit && bestUnit->IsPlayer())
		bestUnit->player->Train(TrainWhat::Persuade, 0.f, chance);

	// add game msg
	gameGui->messages->AddGameMsg3(me, success ? GMS_PERSUASION_SUCCESS : GMS_PERSUASION_FAILED);

	// remember result
	if(existingResult)
	{
		existingResult->tries++;
		existingResult->success = success;
	}
	else
	{
		CheckResult check;
		check.unit = talker;
		check.tries = 1;
		check.success = success;
		checkResults.push_back(check);
	}

	return success;
}

//=================================================================================================
bool Team::HaveInvestment(int questId) const
{
	for(const Investment& investment : investments)
	{
		if(investment.questId == questId)
			return true;
	}
	return false;
}

//=================================================================================================
void Team::AddInvestment(cstring name, int questId, int gold, int days)
{
	Investment& investment = Add1(investments);
	investment.questId = questId;
	investment.gold = gold;
	investment.name = name;
	investment.days = days;

	if(HaveActiveNpc())
	{
		const int investment = int(gold * 5 * GetShare().y);
		for(Unit& unit : activeMembers)
		{
			if(!unit.IsPlayer())
				unit.hero->investment += investment;
		}
	}

	if(Net::IsServer())
		Net::PushChange(NetChange::ADD_INVESTMENT);
}

//=================================================================================================
void Team::UpdateInvestment(int questId, int gold)
{
	for(Investment& investment : investments)
	{
		if(investment.questId == questId)
		{
			if(HaveActiveNpc())
			{
				const int increase = int((gold - investment.gold) * 5 * GetShare().y);
				for(Unit& unit : activeMembers)
				{
					if(!unit.IsPlayer())
						unit.hero->investment += increase;
				}
			}

			investment.gold = gold;
			if(Net::IsServer())
			{
				NetChange& c = Net::PushChange(NetChange::UPDATE_INVESTMENT);
				c.id = questId;
				c.count = gold;
			}
			break;
		}
	}
}

//=================================================================================================
void Team::WriteInvestments(BitStreamWriter& f)
{
	f << investments.size();
	for(const Investment& investment : investments)
	{
		f << investment.questId;
		f << investment.gold;
		f << investment.name;
	}
}

//=================================================================================================
void Team::ReadInvestments(BitStreamReader& f)
{
	investments.resize(f.Read<uint>());
	for(Investment& investment : investments)
	{
		f >> investment.questId;
		f >> investment.gold;
		f >> investment.name;
	}
}

//=================================================================================================
void Team::ShortRest()
{
	for(Unit& unit : members)
	{
		unit.EndEffects();
		if(unit.IsPlayer())
			unit.player->RefreshCooldown();
	}
}
