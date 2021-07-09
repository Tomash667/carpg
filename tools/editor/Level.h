#pragma once

struct Level
{
	Level() : filename("(new)") {}
	void Save(FileWriter& f);
	bool Load(FileReader& f);

	string path, filename;
};
