#include "Pch.h"
#include "AIManager.h"

#include "AITeam.h"
#include "GameFile.h"

AIManager* aiMgr;

AIManager::~AIManager()
{
	DeleteElements(teams);
}

AITeam* AIManager::GetTeam(int id)
{
	for(AITeam* team : teams)
	{
		if(team->id == id)
			return team;
	}
	return nullptr;
}

AITeam* AIManager::CreateTeam()
{
	AITeam* team = new AITeam;
	team->id = teamsCounter++;
	teams.push_back(team);
	return team;
}

void AIManager::RemoveTeam(AITeam* team)
{
	assert(team);
	RemoveElement(teams, team);
	delete team;
}

void AIManager::Reset()
{
	DeleteElements(teams);
	teamsCounter = 0;
}

void AIManager::Save(GameWriter& f)
{
	f << teamsCounter;
	f << teams.size();
	for(AITeam* team : teams)
		team->Save(f);
}

void AIManager::Load(GameReader& f)
{
	f >> teamsCounter;
	teams.resize(f.Read<uint>());
	for(AITeam*& team : teams)
	{
		team = new AITeam;
		team->Load(f);
	}
}
