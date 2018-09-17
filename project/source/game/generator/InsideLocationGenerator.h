#pragma once

//-----------------------------------------------------------------------------
#include "LocationGenerator.h"
#include "ObjectEntity.h"

//-----------------------------------------------------------------------------
class InsideLocationGenerator : public LocationGenerator
{
public:
	void Init() override;
	int GetNumberOfSteps() override;
	void OnEnter() override;
	virtual bool HandleUpdate(int days) { return true; }
	void CreateMinimap() override;
	void OnLoad() override;

protected:
	InsideLocationLevel& GetLevelData();
	void GenerateDungeonObjects();
	ObjectEntity GenerateDungeonObject(InsideLocationLevel& lvl, Room& room, BaseObject* base, vector<Vec3>& on_wall, vector<Int2>& blocks, int flags);
	void GenerateTraps();
	void RegenerateTraps();
	void RespawnTraps();
	void GenerateDungeonTreasure(vector<Chest*>& chests, int level, bool extra = false);
	void SpawnHeroesInsideDungeon();

	InsideLocation* inside;
};
