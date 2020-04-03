#pragma once

//-----------------------------------------------------------------------------
class DungeonMeshBuilder
{
public:
	DungeonMeshBuilder();
	~DungeonMeshBuilder();
	void Build();
	void ChangeTexWrap(bool use_tex_wrap);
	void FillPart(Int2* dungeon_part, word* faces, int& index, word offset);
	void ListVisibleParts(DrawBatch& batch, FrustumPlanes& frustum);

	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	Int2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	bool dungeon_tex_wrap;
};
