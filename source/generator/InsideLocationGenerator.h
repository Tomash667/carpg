#pragma once

//-----------------------------------------------------------------------------
#include "LocationGenerator.h"
#include "BaseLocation.h"
#include "ObjectEntity.h"
#include "RoomType.h"

//-----------------------------------------------------------------------------
class InsideLocationGenerator : public LocationGenerator
{
public:
	void Init() override;
	int GetNumberOfSteps() override;
	void OnEnter() override;
	void CreateMinimap() override;
	void OnLoad() override;

	ObjectEntity GenerateDungeonObject(InsideLocationLevel& lvl, const Int2& tile, BaseObject* base);

protected:
	InsideLocationLevel& GetLevelData();
	void AddRoomColliders(InsideLocationLevel& lvl, Room& room, vector<Int2>& blocks);
	void GenerateDungeonObjects();
	void GenerateDungeonEntry(InsideLocationLevel& lvl, EntryType type, const Int2& pt, GameDirection dir);
	ObjectEntity GenerateDungeonObject(InsideLocationLevel& lvl, Room& room, BaseObject* base, RoomType::Obj* room_obj,
		vector<Vec3>& on_wall, vector<Int2>& blocks, int flags);
	void GenerateTraps();
	void RegenerateTraps();
	void RespawnTraps();
	void GenerateDungeonTreasure(vector<Chest*>& chests, int level, bool extra = false);
	void SpawnHeroesInsideDungeon();
	void FindPathFromStairsToStairs(vector<RoomGroup*>& groups);
	void OpenDoorsByTeam(const Int2& pt);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void ApplyLocationTextureOverride(TexOverride& floor, TexOverride& wall, TexOverride& ceil, LocationTexturePack& tex);
	void ApplyLocationTextureOverride(TexOverride& tex_o, LocationTexturePack::Entry& e, TexOverride& tex_o_def);

	InsideLocation* inside;
};
