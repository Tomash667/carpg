#pragma once

#include <VertexDeclaration.h>

class MeshBuilder
{
public:
	MeshBuilder();
	~MeshBuilder();
	void Clear();
	void Build(Level* level);
	void Draw(Camera& camera);

private:
	void BuildLinks(Level* level);
	Dir GetDirection(Room* a, Room* b);

	SuperShader* shader;
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	vector<VDefault> vertices;
	vector<word> indices;
	vector<Int2> floorParts, ceilingParts, wallParts;
	TexturePtr texFloor, texCeiling, texWall;
};
