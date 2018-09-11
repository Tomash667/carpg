#pragma once

class LocationGenerator
{
	friend class LocationGeneratorFactory;
public:
	virtual ~LocationGenerator() {}
	virtual void Init() {}
	virtual int GetNumberOfSteps();
	virtual void Generate() = 0;
	virtual void OnEnter() = 0;
	virtual void GenerateObjects() {}
	virtual void GenerateUnits() {}
	virtual void GenerateItems() {}
	virtual void CreateMinimap() = 0;
	virtual void OnLoad() = 0;
	void RespawnUnits();

	Location* loc;
	int dungeon_level;
	bool first, reenter;
};
