#pragma once

class AIManager
{
public:
	~AIManager();
	AITeam* GetTeam(int id);
	AITeam* CreateTeam();
	void RemoveTeam(AITeam* team);
	void Reset();
	void Save(GameWriter& f);
	void Load(GameReader& f);

private:
	vector<AITeam*> teams;
	int teamsCounter;
};
