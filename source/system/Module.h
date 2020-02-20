#pragma once

struct Module
{
	string id, name, desc;
	float version;

	static vector<Module*> modules;
};
