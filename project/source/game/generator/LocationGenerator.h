#pragma once

class LocationGenerator
{
public:
	virtual ~LocationGenerator() {}
	virtual void Init() {}
	virtual int GetNumberOfSteps();
	virtual void Generate() = 0;
	virtual void OnEnter() = 0;

	Location* loc;
	int dungeon_level;
	bool first, reenter;
};
