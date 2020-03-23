#pragma once

//-----------------------------------------------------------------------------
#include "ManagedResource.h"

//-----------------------------------------------------------------------------
class DungeonMeshBuilder : public ManagedResource
{
public:
	DungeonMeshBuilder();
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Build();
	void ChangeTexWrap(bool use_tex_wrap);
	void FillPart(Int2* dungeon_part, word* faces, int& index, word offset);
	void ListVisibleParts(DrawBatch& batch, FrustumPlanes& frustum);

	VB vbDungeon;
	IB ibDungeon;
	Int2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	bool dungeon_tex_wrap;
};
