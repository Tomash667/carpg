#include "Pch.h"
#include "AITeam.h"

#include "AIManager.h"
#include "Unit.h"

void AITeam::Add(Unit* unit)
{
	assert(unit && unit->hero && !unit->hero->otherTeam);
	unit->hero->otherTeam = this;
	members.push_back(unit);
}

void AITeam::Remove(Unit* unit)
{
	assert(unit && unit->hero && unit->hero->otherTeam == this);
	RemoveElement(members, unit);
	unit->hero->otherTeam = nullptr;
	if(members.size() <= 1u)
	{
		for(Unit* u : members)
		{
			if(u)
				u->hero->otherTeam = nullptr;
		}
		aiMgr->RemoveTeam(this);
	}
	else if(unit == leader)
		SelectLeader();
}

void AITeam::Remove()
{
	for(Unit* u : members)
	{
		if(u)
			u->hero->otherTeam = nullptr;
	}
	aiMgr->RemoveTeam(this);
}

void AITeam::SelectLeader()
{
	leader = RandomItem(members);
}

void AITeam::Save(GameWriter& f)
{
	f << id;
	f << members;
	f << leader;
}

void AITeam::Load(GameReader& f)
{
	f >> id;
	f >> members;
	f >> leader;
}
