#pragma once

//-----------------------------------------------------------------------------
class LocationGenerator
{
	friend class LocationGeneratorFactory;
public:
	enum UpdateFlags
	{
		PREVENT_RESET = 1 << 0,
		PREVENT_RESPAWN_UNITS = 1 << 1,
		PREVENT_RECREATE_OBJECTS = 1 << 2
	};

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
	virtual int HandleUpdate(int days) { return 0; }
	void RespawnUnits();
	virtual void SpawnUnits(UnitGroup* group, int level) { assert(0); }

	Location* loc;
	int dungeonLevel;
	bool first;
};
