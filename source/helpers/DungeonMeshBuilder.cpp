#include "Pch.h"
#include "DungeonMeshBuilder.h"

#include <Algorithm.h>
#include <DirectX.h>

#include "BaseLocation.h"
#include "DrawBatch.h"
#include "GameResources.h"
#include "InsideLocation.h"
#include "Level.h"
#include "Room.h"

//-----------------------------------------------------------------------------
struct IBOX
{
	int x, y, s;
	float l, t;

	IBOX(int _x, int _y, int _s, float _l, float _t) : x(_x), y(_y), s(_s), l(_l), t(_t)
	{
	}

	bool IsTop() const
	{
		return (s == 1);
	}
	bool IsVisible(const FrustumPlanes& f) const
	{
		Box box(2.f * x, l, 2.f * y, 2.f * (x + s), t, 2.f * (y + s));
		return f.BoxToFrustum(box);
	}
	IBOX GetLeftTop() const
	{
		return IBOX(x, y, s / 2, l, t);
	}
	IBOX GetRightTop() const
	{
		return IBOX(x + s / 2, y, s / 2, l, t);
	}
	IBOX GetLeftBottom() const
	{
		return IBOX(x, y + s / 2, s / 2, l, t);
	}
	IBOX GetRightBottom() const
	{
		return IBOX(x + s / 2, y + s / 2, s / 2, l, t);
	}
	void PushTop(vector<Int2>& top, int size) const
	{
		top.push_back(Int2(x, y));
		if(x < size - 1)
		{
			top.push_back(Int2(x + 1, y));
			if(y < size - 1)
				top.push_back(Int2(x + 1, y + 1));
		}
		if(y < size - 1)
			top.push_back(Int2(x, y + 1));
	}
};

//=================================================================================================
DungeonMeshBuilder::DungeonMeshBuilder() : vb(nullptr), ib(nullptr), dungeon_tex_wrap(true)
{
}

//=================================================================================================
DungeonMeshBuilder::~DungeonMeshBuilder()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

//=================================================================================================
void DungeonMeshBuilder::Build()
{
	ID3D11Device* device = render->GetDevice();
	Buf buf;

	// fill vertex buffer
	//--------------------
	// ile wierzcho³ków
	// 19*4, pod³oga, sufit, 4 œciany, niski sufit, 4 kawa³ki niskiego sufitu, 4 œciany w dziurze górnej, 4 œciany w dziurze dolnej
	uint size = sizeof(VTangent) * 19 * 4;
	VTangent* v = buf.Get<VTangent>(size);

	// krawêdzie musz¹ na siebie lekko zachodziæ, inaczej widaæ dziury pomiêdzy kafelkami
	const float L = -0.001f; // pozycja lewej krawêdzi
	const float R = 2.001f; // pozycja prawej krawêdzi
	const float H = Room::HEIGHT; // wysokoœæ sufitu
	const float HS = Room::HEIGHT_LOW; // wysokoœæ niskiego sufitu
	const float Z = 0.f; // wysokoœæ pod³ogi
	const float U = H + 0.001f; // wysokoœæ œciany
	const float D = Z - 0.001f; // poziom pod³ogi œciany
	//const float DS = HS-0.001f; // pocz¹tek wysokoœæ niskiego sufitu
	const float H1D = 3.999f;
	const float H1U = 8.f;
	const float H2D = -4.f;
	const float H2U = 0.001f;
	const float V0 = (dungeon_tex_wrap ? 2.f : 1);

#define NTB_PX Vec3(1,0,0), Vec3(0,0,1), Vec3(0,-1,0)
#define NTB_MX Vec3(-1,0,0), Vec3(0,0,-1), Vec3(0,-1,0)
#define NTB_PY Vec3(0,1,0), Vec3(1,0,0), Vec3(0,0,-1)
#define NTB_MY Vec3(0,-1,0), Vec3(1,0,0), Vec3(0,0,1)
#define NTB_PZ Vec3(0,0,1), Vec3(-1,0,0), Vec3(0,-1,0)
#define NTB_MZ Vec3(0,0,-1), Vec3(1,0,0), Vec3(0,-1,0)

	// pod³oga
	// 1    3
	// |\   |
	// | \  |
	// |  \ |
	// |   \|
	// 0    2
	v[0] = VTangent(Vec3(L, Z, L), Vec2(0, 1), NTB_PY);
	v[1] = VTangent(Vec3(L, Z, R), Vec2(0, 0), NTB_PY);
	v[2] = VTangent(Vec3(R, Z, L), Vec2(1, 1), NTB_PY);
	v[3] = VTangent(Vec3(R, Z, R), Vec2(1, 0), NTB_PY);

	// sufit
	v[4] = VTangent(Vec3(L, H, R), Vec2(0, 1), NTB_MY);
	v[5] = VTangent(Vec3(L, H, L), Vec2(0, 0), NTB_MY);
	v[6] = VTangent(Vec3(R, H, R), Vec2(1, 1), NTB_MY);
	v[7] = VTangent(Vec3(R, H, L), Vec2(1, 0), NTB_MY);

	// lewa
	v[8] = VTangent(Vec3(R, D, R), Vec2(0, V0), NTB_MX);
	v[9] = VTangent(Vec3(R, U, R), Vec2(0, 0), NTB_MX);
	v[10] = VTangent(Vec3(R, D, L), Vec2(1, V0), NTB_MX);
	v[11] = VTangent(Vec3(R, U, L), Vec2(1, 0), NTB_MX);

	// prawa
	v[12] = VTangent(Vec3(L, D, L), Vec2(0, V0), NTB_PX);
	v[13] = VTangent(Vec3(L, U, L), Vec2(0, 0), NTB_PX);
	v[14] = VTangent(Vec3(L, D, R), Vec2(1, V0), NTB_PX);
	v[15] = VTangent(Vec3(L, U, R), Vec2(1, 0), NTB_PX);

	// przód
	v[16] = VTangent(Vec3(L, D, R), Vec2(0, V0), NTB_MZ);
	v[17] = VTangent(Vec3(L, U, R), Vec2(0, 0), NTB_MZ);
	v[18] = VTangent(Vec3(R, D, R), Vec2(1, V0), NTB_MZ);
	v[19] = VTangent(Vec3(R, U, R), Vec2(1, 0), NTB_MZ);

	// ty³
	v[20] = VTangent(Vec3(R, D, L), Vec2(0, V0), NTB_PZ);
	v[21] = VTangent(Vec3(R, U, L), Vec2(0, 0), NTB_PZ);
	v[22] = VTangent(Vec3(L, D, L), Vec2(1, V0), NTB_PZ);
	v[23] = VTangent(Vec3(L, U, L), Vec2(1, 0), NTB_PZ);

	// niski sufit
	v[24] = VTangent(Vec3(L, HS, R), Vec2(0, 1), NTB_MY);
	v[25] = VTangent(Vec3(L, HS, L), Vec2(0, 0), NTB_MY);
	v[26] = VTangent(Vec3(R, HS, R), Vec2(1, 1), NTB_MY);
	v[27] = VTangent(Vec3(R, HS, L), Vec2(1, 0), NTB_MY);

	/* niskie œciany nie s¹ u¿ywane, uv nie zaktualizowane
	// niski sufit lewa
	v[28] = VTangent(Vec3(R,DS,R), Vec2(0,1), NTB_MX);
	v[29] = VTangent(Vec3(R,U,R), Vec2(0,0.5f), NTB_MX);
	v[30] = VTangent(Vec3(R,DS,L), Vec2(1,1), NTB_MX);
	v[31] = VTangent(Vec3(R,U,L), Vec2(1,0.5f), NTB_MX);

	// niski sufit prawa
	v[32] = VTangent(Vec3(L,DS,L), Vec2(0,1), NTB_PX);
	v[33] = VTangent(Vec3(L,U,L), Vec2(0,0.5f), NTB_PX);
	v[34] = VTangent(Vec3(L,DS,R), Vec2(1,1), NTB_PX);
	v[35] = VTangent(Vec3(L,U,R), Vec2(1,0.5f), NTB_PX);

	// niski sufit przód
	v[36] = VTangent(Vec3(L,DS,R), Vec2(0,1), NTB_MZ);
	v[37] = VTangent(Vec3(L,U,R), Vec2(0,0.5f), NTB_MZ);
	v[38] = VTangent(Vec3(R,DS,R), Vec2(1,1), NTB_MZ);
	v[39] = VTangent(Vec3(R,U,R), Vec2(1,0.5f), NTB_MZ);

	// niski sufit ty³
	v[40] = VTangent(Vec3(R,DS,L), Vec2(0,1), NTB_PZ);
	v[41] = VTangent(Vec3(R,U,L), Vec2(0,0.5f), NTB_PZ);
	v[42] = VTangent(Vec3(L,DS,L), Vec2(1,1), NTB_PZ);
	v[43] = VTangent(Vec3(L,U,L), Vec2(1,0.5f), NTB_PZ);
	*/

	// dziura góra lewa
	v[44] = VTangent(Vec3(R, H1D, R), Vec2(0, V0), NTB_MX);
	v[45] = VTangent(Vec3(R, H1U, R), Vec2(0, 0), NTB_MX);
	v[46] = VTangent(Vec3(R, H1D, L), Vec2(1, V0), NTB_MX);
	v[47] = VTangent(Vec3(R, H1U, L), Vec2(1, 0), NTB_MX);

	// dziura góra prawa
	v[48] = VTangent(Vec3(L, H1D, L), Vec2(0, V0), NTB_PX);
	v[49] = VTangent(Vec3(L, H1U, L), Vec2(0, 0), NTB_PX);
	v[50] = VTangent(Vec3(L, H1D, R), Vec2(1, V0), NTB_PX);
	v[51] = VTangent(Vec3(L, H1U, R), Vec2(1, 0), NTB_PX);

	// dziura góra przód
	v[52] = VTangent(Vec3(L, H1D, R), Vec2(0, V0), NTB_MZ);
	v[53] = VTangent(Vec3(L, H1U, R), Vec2(0, 0), NTB_MZ);
	v[54] = VTangent(Vec3(R, H1D, R), Vec2(1, V0), NTB_MZ);
	v[55] = VTangent(Vec3(R, H1U, R), Vec2(1, 0), NTB_MZ);

	// dziura góra ty³
	v[56] = VTangent(Vec3(R, H1D, L), Vec2(0, V0), NTB_PZ);
	v[57] = VTangent(Vec3(R, H1U, L), Vec2(0, 0), NTB_PZ);
	v[58] = VTangent(Vec3(L, H1D, L), Vec2(1, V0), NTB_PZ);
	v[59] = VTangent(Vec3(L, H1U, L), Vec2(1, 0), NTB_PZ);

	// dziura dó³ lewa
	v[60] = VTangent(Vec3(R, H2D, R), Vec2(0, V0), NTB_MX);
	v[61] = VTangent(Vec3(R, H2U, R), Vec2(0, 0), NTB_MX);
	v[62] = VTangent(Vec3(R, H2D, L), Vec2(1, V0), NTB_MX);
	v[63] = VTangent(Vec3(R, H2U, L), Vec2(1, 0), NTB_MX);

	// dziura dó³ prawa
	v[64] = VTangent(Vec3(L, H2D, L), Vec2(0, V0), NTB_PX);
	v[65] = VTangent(Vec3(L, H2U, L), Vec2(0, 0), NTB_PX);
	v[66] = VTangent(Vec3(L, H2D, R), Vec2(1, V0), NTB_PX);
	v[67] = VTangent(Vec3(L, H2U, R), Vec2(1, 0), NTB_PX);

	// dziura dó³ przód
	v[68] = VTangent(Vec3(L, H2D, R), Vec2(0, V0), NTB_MZ);
	v[69] = VTangent(Vec3(L, H2U, R), Vec2(0, 0), NTB_MZ);
	v[70] = VTangent(Vec3(R, H2D, R), Vec2(1, V0), NTB_MZ);
	v[71] = VTangent(Vec3(R, H2U, R), Vec2(1, 0), NTB_MZ);

	// dziura dó³ ty³
	v[72] = VTangent(Vec3(R, H2D, L), Vec2(0, V0), NTB_PZ);
	v[73] = VTangent(Vec3(R, H2U, L), Vec2(0, 0), NTB_PZ);
	v[74] = VTangent(Vec3(L, H2D, L), Vec2(1, V0), NTB_PZ);
	v[75] = VTangent(Vec3(L, H2U, L), Vec2(1, 0), NTB_PZ);

	// create vertex buffer
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = buf.Get();

	V(device->CreateBuffer(&desc, &data, &vb));
	SetDebugName(vb, "DungeonMeshVb");

	// fill index buffer
	//----------
	// ile indeksów ?
	// pod³oga: 6
	// sufit: 6
	// niski sufit: 6
	//----------
	// opcja ¿e jest jedna œciania: 6*4  -\
	// opcja ¿e s¹ dwie œciany: 12*6       \  razy 4
	// opcja ¿e s¹ trzy œciany: 18*4       /
	// opcja ¿e s¹ wszystkie œciany: 24  -/
	size = sizeof(word) * (6 * 3 + (6 * 4 + 12 * 6 + 18 * 4 + 24) * 4);
	word* id = buf.Get<word>(size);

	// pod³oga
	id[0] = 0;
	id[1] = 1;
	id[2] = 2;
	id[3] = 2;
	id[4] = 1;
	id[5] = 3;

	// sufit
	id[6] = 4;
	id[7] = 5;
	id[8] = 6;
	id[9] = 6;
	id[10] = 5;
	id[11] = 7;

	// niski sufit
	id[12] = 24;
	id[13] = 25;
	id[14] = 26;
	id[15] = 26;
	id[16] = 25;
	id[17] = 27;

	int index = 18;

	FillPart(dungeon_part, id, index, 8);
	FillPart(dungeon_part2, id, index, 28);
	FillPart(dungeon_part3, id, index, 44);
	FillPart(dungeon_part4, id, index, 60);

	// create index buffer
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;

	data.pSysMem = buf.Get();

	V(device->CreateBuffer(&desc, &data, &ib));
#ifdef _DEBUG
	ib->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("DungeonMeshIb"), "DungeonMeshIb");
#endif
}

//=================================================================================================
void DungeonMeshBuilder::ChangeTexWrap(bool use_tex_wrap)
{
	if(use_tex_wrap == dungeon_tex_wrap)
		return;

	dungeon_tex_wrap = use_tex_wrap;

	ResourceLock lock(vb);
	VTangent* v = lock.Get<VTangent>();

	const float V0 = (dungeon_tex_wrap ? 2.f : 1);

	// lewa
	v[8].tex.y = V0;
	v[10].tex.y = V0;

	// prawa
	v[12].tex.y = V0;
	v[14].tex.y = V0;

	// przód
	v[16].tex.y = V0;
	v[18].tex.y = V0;

	// ty³
	v[20].tex.y = V0;
	v[22].tex.y = V0;

	// dziura góra lewa
	v[44].tex.y = V0;
	v[46].tex.y = V0;

	// dziura góra prawa
	v[48].tex.y = V0;
	v[50].tex.y = V0;

	// dziura góra przód
	v[52].tex.y = V0;
	v[54].tex.y = V0;

	// dziura góra ty³
	v[56].tex.y = V0;
	v[58].tex.y = V0;

	// dziura dó³ lewa
	v[60].tex.y = V0;
	v[62].tex.y = V0;

	// dziura dó³ prawa
	v[64].tex.y = V0;
	v[66].tex.y = V0;

	// dziura dó³ przód
	v[68].tex.y = V0;
	v[70].tex.y = V0;

	// dziura dó³ ty³
	v[72].tex.y = V0;
	v[74].tex.y = V0;
}

//=================================================================================================
void DungeonMeshBuilder::FillPart(Int2* _part, word* faces, int& index, word offset)
{
	assert(_part);

	_part[0] = Int2(0, 0);

	for(int i = 1; i < 16; ++i)
	{
		_part[i].x = index;

		int count = 0;
		if(i & 0x01)
		{
			faces[index++] = offset;
			faces[index++] = offset + 1;
			faces[index++] = offset + 2;
			faces[index++] = offset + 2;
			faces[index++] = offset + 1;
			faces[index++] = offset + 3;
			++count;
		}
		if(i & 0x02)
		{
			faces[index++] = offset + 4;
			faces[index++] = offset + 5;
			faces[index++] = offset + 6;
			faces[index++] = offset + 6;
			faces[index++] = offset + 5;
			faces[index++] = offset + 7;
			++count;
		}
		if(i & 0x04)
		{
			faces[index++] = offset + 8;
			faces[index++] = offset + 9;
			faces[index++] = offset + 10;
			faces[index++] = offset + 10;
			faces[index++] = offset + 9;
			faces[index++] = offset + 11;
			++count;
		}
		if(i & 0x08)
		{
			faces[index++] = offset + 12;
			faces[index++] = offset + 13;
			faces[index++] = offset + 14;
			faces[index++] = offset + 14;
			faces[index++] = offset + 13;
			faces[index++] = offset + 15;
			++count;
		}

		_part[i].y = count * 2;
	}
}

//=================================================================================================
void DungeonMeshBuilder::ListVisibleParts(DrawBatch& batch, FrustumPlanes& frustum)
{
	InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	Box box;

	if(!IsSet(base.options, BLO_LABYRINTH))
	{
		for(Room* room : lvl.rooms)
		{
			box.v1 = Vec3(float(room->pos.x * 2), 0, float(room->pos.y * 2));
			box.v2 = box.v1;
			box.v2 += Vec3(float(room->size.x * 2), 4, float(room->size.y * 2));

			if(!frustum.BoxToFrustum(box))
				continue;

			// find all lights affecting this room
			Vec2 v1(box.v1.x, box.v1.z);
			Vec2 v2(box.v2.x, box.v2.z);
			Vec2 ext = (v2 - v1) / 2;
			Vec2 mid = v1 + ext;

			static vector<GameLight*> lights;
			lights.clear();
			for(GameLight& light : lvl.lights)
			{
				if(CircleToRectangle(light.pos.x, light.pos.z, light.range, mid.x, mid.y, ext.x, ext.y))
					lights.push_back(&light);
			}

			// for each room tile
			for(int y = 0; y < room->size.y; ++y)
			{
				for(int x = 0; x < room->size.x; ++x)
				{
					// anything to draw at this tile?
					Tile& tile = lvl.map[(x + room->pos.x) + (y + room->pos.y) * lvl.w];
					if(tile.room != room->index || tile.flags == 0 || tile.flags == Tile::F_REVEALED)
						continue;

					uint group = batch.dungeon_part_groups.size();
					DungeonPartGroup& dungeon_group = Add1(batch.dungeon_part_groups);

					// find best lights
					TopN<GameLight*, 3, float, std::less<>> best(nullptr, game_level->camera.zfar);

					float dx = 2.f * (room->pos.x + x) + 1.f;
					float dz = 2.f * (room->pos.y + y) + 1.f;

					for(GameLight* light : lights)
					{
						float dist = Distance(dx, dz, light->pos.x, light->pos.z);
						if(dist < 1.414213562373095f + light->range)
							best.Add(light, dist);
					}

					for(int i = 0; i < 3; ++i)
						dungeon_group.lights[i] = best[i];

					// set matrices
					dungeon_group.mat_world = Matrix::Translation(2.f * (room->pos.x + x), 0, 2.f * (room->pos.y + y));
					dungeon_group.mat_combined = dungeon_group.mat_world * game_level->camera.mat_view_proj;

					int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

					// floor
					if(IsSet(tile.flags, Tile::F_FLOOR))
					{
						DungeonPart& dp = Add1(batch.dungeon_parts);
						dp.tex_o = &game_res->tFloor[tex_id];
						dp.start_index = 0;
						dp.primitive_count = 2;
						dp.group = group;
					}

					// ceiling
					if(IsSet(tile.flags, Tile::F_CEILING | Tile::F_LOW_CEILING))
					{
						DungeonPart& dp = Add1(batch.dungeon_parts);
						dp.tex_o = &game_res->tCeil[tex_id];
						dp.start_index = IsSet(tile.flags, Tile::F_LOW_CEILING) ? 12 : 6;
						dp.primitive_count = 2;
						dp.group = group;
					}

					// walls
					int d = (tile.flags & 0xFFFF00) >> 8;
					if(d != 0)
					{
						// normal
						int d2 = (d & 0xF);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part[d2].x;
							dp.primitive_count = dungeon_part[d2].y;
							dp.group = group;
						}

						// upper
						d2 = ((d & 0xF00) >> 8);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part3[d2].x;
							dp.primitive_count = dungeon_part3[d2].y;
							dp.group = group;
						}

						// lower
						d2 = ((d & 0xF000) >> 12);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part4[d2].x;
							dp.primitive_count = dungeon_part4[d2].y;
							dp.group = group;
						}
					}
				}
			}
		}
	}
	else
	{
		static vector<IBOX> tocheck;
		static vector<Int2> tiles;
		assert(lvl.w == lvl.h);
		const int size = lvl.w;

		// divide level using pseudo quad-tree & apply frutum culling
		tocheck.push_back(IBOX(0, 0, 64, -4.f, 8.f));

		while(!tocheck.empty())
		{
			IBOX ibox = tocheck.back();
			tocheck.pop_back();

			if(ibox.x < size && ibox.y < size && ibox.IsVisible(frustum))
			{
				if(ibox.IsTop())
					ibox.PushTop(tiles, size);
				else
				{
					tocheck.push_back(ibox.GetLeftTop());
					tocheck.push_back(ibox.GetRightTop());
					tocheck.push_back(ibox.GetLeftBottom());
					tocheck.push_back(ibox.GetRightBottom());
				}
			}
		}

		// for each tile
		for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			Tile& tile = lvl.map[it->x + it->y * lvl.w];
			if(tile.flags == 0 || tile.flags == Tile::F_REVEALED)
				continue;

			Box box(2.f * it->x, -4.f, 2.f * it->y, 2.f * (it->x + 1), 8.f, 2.f * (it->y + 1));
			if(!frustum.BoxToFrustum(box))
				continue;

			uint group = batch.dungeon_part_groups.size();
			DungeonPartGroup& dungeon_group = Add1(batch.dungeon_part_groups);

			// find best lights
			TopN<GameLight*, 3, float, std::less<>> best(nullptr, game_level->camera.zfar);

			float dx = 2.f * it->x + 1.f;
			float dz = 2.f * it->y + 1.f;

			for(GameLight& light : lvl.lights)
			{
				float dist = Distance(dx, dz, light.pos.x, light.pos.z);
				if(dist < 1.414213562373095f + light.range)
					best.Add(&light, dist);
			}

			for(int i = 0; i < 3; ++i)
				dungeon_group.lights[i] = best[i];

			// set matrices
			dungeon_group.mat_world = Matrix::Translation(2.f * it->x, 0, 2.f * it->y);
			dungeon_group.mat_combined = dungeon_group.mat_world * game_level->camera.mat_view_proj;

			int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

			// floor
			if(IsSet(tile.flags, Tile::F_FLOOR))
			{
				DungeonPart& dp = Add1(batch.dungeon_parts);
				dp.tex_o = &game_res->tFloor[tex_id];
				dp.start_index = 0;
				dp.primitive_count = 2;
				dp.group = group;
			}

			// ceiling
			if(IsSet(tile.flags, Tile::F_CEILING | Tile::F_LOW_CEILING))
			{
				DungeonPart& dp = Add1(batch.dungeon_parts);
				dp.tex_o = &game_res->tCeil[tex_id];
				dp.start_index = IsSet(tile.flags, Tile::F_LOW_CEILING) ? 12 : 6;
				dp.primitive_count = 2;
				dp.group = group;
			}

			// walls
			int d = (tile.flags & 0xFFFF00) >> 8;
			if(d != 0)
			{
				// normal
				int d2 = (d & 0xF);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part[d2].x;
					dp.primitive_count = dungeon_part[d2].y;
					dp.group = group;
				}

				// lower
				d2 = ((d & 0xF0) >> 4);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part2[d2].x;
					dp.primitive_count = dungeon_part2[d2].y;
					dp.group = group;
				}

				// top
				d2 = ((d & 0xF00) >> 8);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part3[d2].x;
					dp.primitive_count = dungeon_part3[d2].y;
					dp.group = group;
				}

				// bottom
				d2 = ((d & 0xF000) >> 12);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part4[d2].x;
					dp.primitive_count = dungeon_part4[d2].y;
					dp.group = group;
				}
			}
		}

		tiles.clear();
	}

	std::sort(batch.dungeon_parts.begin(), batch.dungeon_parts.end(), [](const DungeonPart& p1, const DungeonPart& p2)
	{
		if(p1.tex_o != p2.tex_o)
			return p1.tex_o->GetIndex() < p2.tex_o->GetIndex();
		else
			return false;
	});
}
