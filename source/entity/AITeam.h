#pragma once

struct AITeam
{
	int id;
	vector<Entity<Unit>> members;
	Entity<Unit> leader;

	void Add(Unit* unit);
	void Remove(Unit* unit);
	void Remove();
	void SelectLeader();
	void Save(GameWriter& f);
	void Load(GameReader& f);
};
