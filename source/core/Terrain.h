#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
struct Tri
{
	int a, b, c;

	Tri() {}
	Tri(int a, int b, int c) : a(a), b(b), c(c) {}
};

//-----------------------------------------------------------------------------
void CalculateNormal(VTerrain& v1,VTerrain& v2,VTerrain& v3);

//-----------------------------------------------------------------------------
struct TerrainOptions
{
	float tile_size;
	uint n_parts;
	uint tiles_per_part;
	uint tex_size;
};

//-----------------------------------------------------------------------------
struct Terrain
{
public:
	//---------------------------
	class Part
	{
		friend struct Terrain;
	public:
		const BOX& GetBox() const
		{
			return box;
		}

	private:
		BOX box;
	};

	static const int DEFAULT_UV_MOD = 2;
	int uv_mod;

	//---------------------------
	Terrain();
	~Terrain();

	//---------------------------
	void Init(IDirect3DDevice9* dev, const TerrainOptions& options);
	void Build(bool smooth=true);
	void Rebuild(bool smooth=true);
	void RebuildUv();
	void Make(bool smooth=true);
	void ApplyTextures(ID3DXEffect* effect); // to delete
	void ApplyStreamSource(); // to delete, create reference leaks
	void Draw(uint i)
	{
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, n_verts, part_tris*i*3, part_tris) );
	}
	void SetHeight(float height);
	void ClearHeight()
	{
		SetHeight(0.f);
	}
	void RandomizeHeight(float hmin,float hmax);
	void RoundHeight();
	void Randomize();
	void CalculateBox();
	void SmoothNormals();
	void SmoothNormals(VTerrain* v);
	float Raytest(const VEC3& from, const VEC3& to) const;
	void FillGeometry(vector<Tri>& tris, vector<VEC3>& verts);
	void FillGeometryPart(vector<Tri>& tris, vector<VEC3>& verts, int px, int pz, const VEC3& offset=VEC3(0,0,0)) const;

	//---------------------------
	Part* GetPart(uint idx)
	{
		assert(idx < n_parts2);
		return &parts[idx];
	}
	float GetH(float x, float z) const;
	float GetH(const VEC3& _v) const
	{
		return GetH(_v.x, _v.z);
	}
	float GetH(const VEC2& v) const
	{
		return GetH(v.x, v.y);
	}
	void SetH(VEC3& _v) const
	{
		_v.y = GetH(_v.x, _v.z);
	}
	void GetAngle(float x, float z, VEC3& angle) const;
	uint GetPartsCount() const
	{
		return n_parts2;
	}
	TEX GetSplatTexture()
	{
		return texSplat;
	}
	TEX* GetTextures()
	{
		return tex;
	}
	const BOX& GetBox() const
	{
		return box;
	}
	const VEC3& GetPos() const
	{
		return pos;
	}
	LPD3DXMESH GetMesh()
	{
		return mesh;
	}
	float* GetHeightMap()
	{
		return h;
	}
	uint GetTerrainWidth() const
	{
		return hszer;
	}
	uint GetTilesCount() const
	{
		return n_tiles;
	}
	uint GetSplatSize() const
	{
		return tex_size;
	}
	void GetDrawOptions(uint& verts, uint& tris)
	{
		verts = n_verts;
		tris = part_tris;
	}
	float GetPartSize() const
	{
		return tiles_size/n_parts;
	}
	float GetTileSize() const
	{
		return tile_size;
	}

	//---------------------------
	void SetTextures(TEX* textures)
	{
		assert(textures);
		for(uint i=0; i<5; ++i)
			tex[i] = textures[i];
	}

	void RemoveHeightMap(bool _delete=false);
	void SetHeightMap(float* h);
	bool IsInside(float x, float z) const
	{
		return x >= 0.f && z >= 0.f && x < tiles_size && z < tiles_size;
	}
	bool IsInside(const VEC3& v) const
	{
		return IsInside(v.x, v.z);
	}

private:
	void CreateSplatTexture();
	
	Part* parts;
	float* h;
	float tile_size; // rozmiar jednego ma³ego kwadraciku terenu
	float tiles_size;
	uint n_tiles, n_tiles2; // liczba kwadracików na boku, wszystkich
	uint n_parts, n_parts2; // liczba sektorów na boku, wszystkich
	uint tiles_per_part;
	uint hszer, hszer2; // n_tiles+1
	uint n_tris, n_verts, part_tris, part_verts, tex_size;
	BOX box;
	LPD3DXMESH mesh;
	IDirect3DDevice9* device;
	TEX texSplat;
	TEX tex[5];
	VEC3 pos;
	int state;
};
