#pragma once

struct Level
{
	Level() : filename("(new)") {}
	~Level();
	void Save(FileWriter& f);
	bool Load(FileReader& f);

	string path, filename;
	vector<Room*> rooms;
};
