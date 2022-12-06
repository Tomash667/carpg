#pragma once

//-----------------------------------------------------------------------------
class DungeonMeshBuilder
{
public:
	DungeonMeshBuilder();
	~DungeonMeshBuilder();
	void Build();
	void FillPart(Int2* dungeonPart, word* faces, int& index, word offset);
	void ListVisibleParts(DrawBatch& batch, FrustumPlanes& frustum);

	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	Int2 dungeonPart[16], dungeonPart2[16], dungeonPart3[16], dungeonPart4[16];
};
