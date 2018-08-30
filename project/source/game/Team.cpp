#include "Pch.h"
#include "GameCore.h"
#include "Team.h"
#include "Unit.h"
#include "SaveState.h"
#include "GameFile.h"

TeamSingleton Team;

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
}

void TeamSingleton::Reset()
{
	crazies_attack = false;
	is_bandit = false;
	free_recruit = true;
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
