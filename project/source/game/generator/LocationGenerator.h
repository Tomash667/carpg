#pragma once

class LocationGenerator
{
	friend class LocationGeneratorFactory;
public:
	virtual ~LocationGenerator() {}
	void Init(bool first, bool reenter, int dungeon_level = 0)
	{
		this->first = first;
		this->reenter = reenter;
		this->dungeon_level = dungeon_level;
		Init();
	}
	virtual void Init() {}
	virtual int GetNumberOfSteps();
	virtual void Generate() = 0;
	virtual void OnEnter() = 0;
	virtual void GenerateObjects() {}
	virtual void GenerateUnits() {}
	virtual void GenerateItems() {}
	virtual void CreateMinimap() = 0;
	virtual void OnLoad() = 0;

	Location* loc;
	int dungeon_level;
	bool first, reenter;
};
