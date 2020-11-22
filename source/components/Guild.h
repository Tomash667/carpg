#pragma once

class Guild
{
	friend class GuildPanel;
public:
	void Clear();
	void Create();
	void Save(GameWriter& f);
	void Load(GameReader& f);

	bool IsCreated() const { return created; }
	const string& GetName() const { return name; }

	static const uint MAX_SIZE = 20;

private:
	string name, tmpName;
	int reputation;
	Entity<Unit> master;
	vector<Entity<Unit>> members;
	bool created;
};
