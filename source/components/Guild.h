#pragma once

class Guild
{
public:
	void Clear();
	void Create();
	void Save(GameWriter& f);
	void Load(GameReader& f);

	bool IsCreated() const { return created; }
	const string& GetName() const { return name; }

private:
	string name, tmpName;
	bool created;
};
