#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "City.h"
#include "InsideLocation.h"
#include "Action.h"
#include "Profiler.h"
#include "Portal.h"
#include "Level.h"
#include "SuperShader.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "ParticleSystem.h"
#include "Render.h"
#include "TerrainShader.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "PhysicCallbacks.h"
#include "GameResources.h"
#include "DirectX.h"

//-----------------------------------------------------------------------------
ObjectPool<SceneNode> node_pool;
ObjectPool<DebugSceneNode> debug_node_pool;
ObjectPool<Area2> area2_pool;

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
		Box box(2.f*x, l, 2.f*y, 2.f*(x + s), t, 2.f*(y + s));
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
void DrawBatch::Clear()
{
	node_pool.Free(nodes);
	debug_node_pool.Free(debug_nodes);
	glow_nodes.clear();
	terrain_parts.clear();
	bloods.clear();
	billboards.clear();
	explos.clear();
	pes.clear();
	areas.clear();
	area2_pool.Free(areas2);
	portals.clear();
	lights.clear();
	dungeon_parts.clear();
	matrices.clear();
	stuns.clear();

	// empty lights
	Lights& l = Add1(lights);
	for(int i = 0; i < 3; ++i)
	{
		l.ld[i].range = 1.f;
		l.ld[i].pos = Vec3(0, -1000, 0);
		l.ld[i].color = Vec3(0, 0, 0);
	}
}

//=================================================================================================
void Game::InitScene()
{
	blood_v[0].tex = Vec2(0, 0);
	blood_v[1].tex = Vec2(0, 1);
	blood_v[2].tex = Vec2(1, 0);
	blood_v[3].tex = Vec2(1, 1);
	for(int i = 0; i < 4; ++i)
		blood_v[i].pos.y = 0.f;

	billboard_v[0].pos = Vec3(-1, -1, 0);
	billboard_v[0].tex = Vec2(0, 0);
	billboard_v[0].color = Vec4(1.f, 1.f, 1.f, 1.f);
	billboard_v[1].pos = Vec3(-1, 1, 0);
	billboard_v[1].tex = Vec2(0, 1);
	billboard_v[1].color = Vec4(1.f, 1.f, 1.f, 1.f);
	billboard_v[2].pos = Vec3(1, -1, 0);
	billboard_v[2].tex = Vec2(1, 0);
	billboard_v[2].color = Vec4(1.f, 1.f, 1.f, 1.f);
	billboard_v[3].pos = Vec3(1, 1, 0);
	billboard_v[3].tex = Vec2(1, 1);
	billboard_v[3].color = Vec4(1.f, 1.f, 1.f, 1.f);

	billboard_ext[0] = Vec3(-1, -1, 0);
	billboard_ext[1] = Vec3(-1, 1, 0);
	billboard_ext[2] = Vec3(1, -1, 0);
	billboard_ext[3] = Vec3(1, 1, 0);

	portal_v[0].pos = Vec3(-0.67f, -0.67f, 0);
	portal_v[1].pos = Vec3(-0.67f, 0.67f, 0);
	portal_v[2].pos = Vec3(0.67f, -0.67f, 0);
	portal_v[3].pos = Vec3(0.67f, 0.67f, 0);
	portal_v[0].tex = Vec2(0, 0);
	portal_v[1].tex = Vec2(0, 1);
	portal_v[2].tex = Vec2(1, 0);
	portal_v[3].tex = Vec2(1, 1);
	portal_v[0].color = Vec4(1, 1, 1, 0.5f);
	portal_v[1].color = Vec4(1, 1, 1, 0.5f);
	portal_v[2].color = Vec4(1, 1, 1, 0.5f);
	portal_v[3].color = Vec4(1, 1, 1, 0.5f);

	if(!vbDungeon)
		BuildDungeon();
}

//=================================================================================================
void Game::BuildDungeon()
{
	// ile wierzcho³ków
	// 19*4, pod³oga, sufit, 4 œciany, niski sufit, 4 kawa³ki niskiego sufitu, 4 œciany w dziurze górnej, 4 œciany w dziurze dolnej
	IDirect3DDevice9* device = render->GetDevice();

	uint size = sizeof(VTangent) * 19 * 4;
	V(device->CreateVertexBuffer(size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbDungeon, nullptr));

	VTangent* v;
	V(vbDungeon->Lock(0, size, (void**)&v, 0));

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

	V(vbDungeon->Unlock());

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

	// index buffer
	V(device->CreateIndexBuffer(size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibDungeon, nullptr));

	word* id;
	V(ibDungeon->Lock(0, size, (void**)&id, 0));

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

	FillDungeonPart(dungeon_part, id, index, 8);
	FillDungeonPart(dungeon_part2, id, index, 28);
	FillDungeonPart(dungeon_part3, id, index, 44);
	FillDungeonPart(dungeon_part4, id, index, 60);

	V(ibDungeon->Unlock());
}

//=================================================================================================
void Game::ChangeDungeonTexWrap()
{
	VTangent* v;
	V(vbDungeon->Lock(0, 0, (void**)&v, 0));

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

	V(vbDungeon->Unlock());
}

//=================================================================================================
void Game::FillDungeonPart(Int2* _part, word* faces, int& index, word offset)
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
void Game::ListDrawObjects(LevelArea& area, FrustumPlanes& frustum, bool outside)
{
	PROFILER_BLOCK("ListDrawObjects");

	TmpLevelArea& tmp_area = *area.tmp;

	draw_batch.Clear();
	ClearGrass();
	if(area.area_type == LevelArea::Type::Outside)
	{
		ListQuadtreeNodes();
		ListGrass();
	}

	// terrain
	if(area.area_type == LevelArea::Type::Outside && IsSet(draw_flags, DF_TERRAIN))
	{
		PROFILER_BLOCK("Terrain");
		uint parts = game_level->terrain->GetPartsCount();
		for(uint i = 0; i < parts; ++i)
		{
			if(frustum.BoxToFrustum(game_level->terrain->GetPart(i)->GetBox()))
				draw_batch.terrain_parts.push_back(i);
		}
	}

	// dungeon
	if(area.area_type == LevelArea::Type::Inside && IsSet(draw_flags, DF_TERRAIN))
	{
		PROFILER_BLOCK("Dungeon");
		FillDrawBatchDungeonParts(frustum);
	}

	// units
	if(IsSet(draw_flags, DF_UNITS))
	{
		PROFILER_BLOCK("Units");
		for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
		{
			Unit& u = **it;
			ListDrawObjectsUnit(frustum, outside, u);
		}
	}

	// objects
	if(IsSet(draw_flags, DF_OBJECTS))
	{
		PROFILER_BLOCK("Objects");
		if(area.area_type == LevelArea::Type::Outside)
		{
			for(LevelPart* part : level_parts)
			{
				for(QuadObj& obj : part->objects)
				{
					const Object& o = *obj.obj;
					if(frustum.SphereToFrustum(o.pos, o.GetRadius()))
						AddObjectToDrawBatch(area, o, frustum);
				}
			}
		}
		else
		{
			for(vector<Object*>::iterator it = area.objects.begin(), end = area.objects.end(); it != end; ++it)
			{
				const Object& o = **it;
				if(frustum.SphereToFrustum(o.pos, o.GetRadius()))
					AddObjectToDrawBatch(area, o, frustum);
			}
		}
	}

	// items
	if(IsSet(draw_flags, DF_ITEMS))
	{
		PROFILER_BLOCK("Ground items");
		Vec3 pos;
		for(vector<GroundItem*>::iterator it = area.items.begin(), end = area.items.end(); it != end; ++it)
		{
			GroundItem& item = **it;
			if(!item.item)
			{
				ReportError(7, Format("GroundItem with null item at %g;%g;%g (count %d, team count %d).",
					item.pos.x, item.pos.y, item.pos.z, item.count, item.team_count));
				area.items.erase(it);
				break;
			}
			Mesh* mesh;
			pos = item.pos;
			if(IsSet(item.item->flags, ITEM_GROUND_MESH))
			{
				mesh = item.item->mesh;
				pos.y -= mesh->head.bbox.v1.y;
			}
			else
				mesh = game_res->aBag;
			if(frustum.SphereToFrustum(item.pos, mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::RotationY(item.rot) * Matrix::Translation(pos);
				node->mesh = mesh;
				node->flags = IsSet(item.item->flags, ITEM_ALPHA) ? SceneNode::F_ALPHA_TEST : 0;
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, item.pos.x, item.pos.z, mesh->head.radius);
				if(pc->data.before_player == BP_ITEM && pc->data.before_player_ptr.item == &item)
				{
					if(cl_glow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.type = GlowNode::Item;
						glow.ptr = &item;
						glow.alpha = IsSet(item.item->flags, ITEM_ALPHA);
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				AddOrSplitSceneNode(node);
			}
		}
	}

	// usable objects
	if(IsSet(draw_flags, DF_USABLES))
	{
		PROFILER_BLOCK("Usables");
		for(vector<Usable*>::iterator it = area.usables.begin(), end = area.usables.end(); it != end; ++it)
		{
			Usable& use = **it;
			Mesh* mesh = use.GetMesh();
			if(frustum.SphereToFrustum(use.pos, mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::RotationY(use.rot) * Matrix::Translation(use.pos);
				node->mesh = mesh;
				node->flags = 0;
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, use.pos.x, use.pos.z, mesh->head.radius);
				if(pc->data.before_player == BP_USABLE && pc->data.before_player_ptr.usable == &use)
				{
					if(cl_glow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.type = GlowNode::Usable;
						glow.ptr = &use;
						glow.alpha = false;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				AddOrSplitSceneNode(node);
			}
		}
	}

	// chests
	if(IsSet(draw_flags, DF_USABLES))
	{
		PROFILER_BLOCK("Chests");
		for(vector<Chest*>::iterator it = area.chests.begin(), end = area.chests.end(); it != end; ++it)
		{
			Chest& chest = **it;
			if(frustum.SphereToFrustum(chest.pos, chest.mesh_inst->mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::RotationY(chest.rot) * Matrix::Translation(chest.pos);
				if(!chest.mesh_inst->groups[0].anim || chest.mesh_inst->groups[0].time == 0.f)
				{
					node->mesh = chest.mesh_inst->mesh;
					node->flags = 0;
				}
				else
				{
					chest.mesh_inst->SetupBones();
					node->mesh_inst = chest.mesh_inst;
					node->flags = SceneNode::F_ANIMATED;
					node->parent_mesh_inst = nullptr;
				}
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, chest.pos.x, chest.pos.z, chest.mesh_inst->mesh->head.radius);
				if(pc->data.before_player == BP_CHEST && pc->data.before_player_ptr.chest == &chest)
				{
					if(cl_glow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.type = GlowNode::Chest;
						glow.ptr = &chest;
						glow.alpha = false;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				AddOrSplitSceneNode(node);
			}
		}
	}

	// doors
	if(IsSet(draw_flags, DF_USABLES))
	{
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(frustum.SphereToFrustum(door.pos, door.mesh_inst->mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::RotationY(door.rot) * Matrix::Translation(door.pos);
				if(!door.mesh_inst->groups[0].anim || door.mesh_inst->groups[0].time == 0.f)
				{
					node->mesh = door.mesh_inst->mesh;
					node->flags = 0;
				}
				else
				{
					door.mesh_inst->SetupBones();
					node->mesh_inst = door.mesh_inst;
					node->flags = SceneNode::F_ANIMATED;
					node->parent_mesh_inst = nullptr;
				}
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, door.pos.x, door.pos.z, door.mesh_inst->mesh->head.radius);
				if(pc->data.before_player == BP_DOOR && pc->data.before_player_ptr.door == &door)
				{
					if(cl_glow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.type = GlowNode::Door;
						glow.ptr = &door;
						glow.alpha = false;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				AddOrSplitSceneNode(node);
			}
		}
	}

	// bloods
	if(IsSet(draw_flags, DF_BLOOD))
	{
		for(vector<Blood>::iterator it = area.bloods.begin(), end = area.bloods.end(); it != end; ++it)
		{
			if(it->size > 0.f && frustum.SphereToFrustum(it->pos, it->size * it->scale))
			{
				if(!outside)
					it->lights = GatherDrawBatchLights(area, nullptr, it->pos.x, it->pos.z, it->size);
				draw_batch.bloods.push_back(&*it);
			}
		}
	}

	// bullets
	if(IsSet(draw_flags, DF_BULLETS))
	{
		for(vector<Bullet>::iterator it = tmp_area.bullets.begin(), end = tmp_area.bullets.end(); it != end; ++it)
		{
			Bullet& bullet = *it;
			if(bullet.mesh)
			{
				if(frustum.SphereToFrustum(bullet.pos, bullet.mesh->head.radius))
				{
					SceneNode* node = node_pool.Get();
					node->billboard = false;
					node->mat = Matrix::Rotation(bullet.rot) * Matrix::Translation(bullet.pos);
					node->mesh = bullet.mesh;
					node->flags = 0;
					node->tint = Vec4(1, 1, 1, 1);
					node->tex_override = nullptr;
					if(!outside)
						node->lights = GatherDrawBatchLights(area, node, bullet.pos.x, bullet.pos.z, bullet.mesh->head.radius);
					AddOrSplitSceneNode(node);
				}
			}
			else
			{
				if(frustum.SphereToFrustum(bullet.pos, bullet.tex_size))
				{
					Billboard& bb = Add1(draw_batch.billboards);
					bb.pos = it->pos;
					bb.size = it->tex_size;
					bb.tex = it->tex->tex;
				}
			}
		}
	}

	// traps
	if(IsSet(draw_flags, DF_TRAPS))
	{
		for(vector<Trap*>::iterator it = area.traps.begin(), end = area.traps.end(); it != end; ++it)
		{
			Trap& trap = **it;
			if((trap.state == 0 || (trap.base->type != TRAP_ARROW && trap.base->type != TRAP_POISON)) && frustum.SphereToFrustum(trap.obj.pos, trap.obj.mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::Transform(trap.obj.pos, trap.obj.rot, trap.obj.scale);
				node->mesh = trap.obj.mesh;
				int alpha = trap.obj.RequireAlphaTest();
				if(alpha == -1)
					node->flags = 0;
				else if(alpha == 0)
					node->flags = SceneNode::F_ALPHA_TEST;
				else
					node->flags = SceneNode::F_ALPHA_TEST | SceneNode::F_NO_CULLING;
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, trap.obj.pos.x, trap.obj.pos.z, trap.obj.mesh->head.radius);
				AddOrSplitSceneNode(node);
			}
			if(trap.base->type == TRAP_SPEAR && InRange(trap.state, 2, 4) && frustum.SphereToFrustum(trap.obj2.pos, trap.obj2.mesh->head.radius))
			{
				SceneNode* node = node_pool.Get();
				node->billboard = false;
				node->mat = Matrix::Transform(trap.obj2.pos, trap.obj2.rot, trap.obj2.scale);
				node->mesh = trap.obj2.mesh;
				int alpha = trap.obj2.RequireAlphaTest();
				if(alpha == -1)
					node->flags = 0;
				else if(alpha == 0)
					node->flags = SceneNode::F_ALPHA_TEST;
				else
					node->flags = SceneNode::F_ALPHA_TEST | SceneNode::F_NO_CULLING;
				node->tex_override = nullptr;
				node->tint = Vec4(1, 1, 1, 1);
				if(!outside)
					node->lights = GatherDrawBatchLights(area, node, trap.obj2.pos.x, trap.obj2.pos.z, trap.obj2.mesh->head.radius);
				AddOrSplitSceneNode(node);
			}
		}
	}

	// explosions
	if(IsSet(draw_flags, DF_EXPLOS))
	{
		for(vector<Explo*>::iterator it = tmp_area.explos.begin(), end = tmp_area.explos.end(); it != end; ++it)
		{
			Explo& explo = **it;
			if(frustum.SphereToFrustum(explo.pos, explo.size))
				draw_batch.explos.push_back(&explo);
		}
	}

	// particles
	if(IsSet(draw_flags, DF_PARTICLES))
	{
		PROFILER_BLOCK("Particles");
		for(vector<ParticleEmitter*>::iterator it = tmp_area.pes.begin(), end = tmp_area.pes.end(); it != end; ++it)
		{
			ParticleEmitter& pe = **it;
			if(pe.alive && frustum.SphereToFrustum(pe.pos, pe.radius))
			{
				draw_batch.pes.push_back(&pe);
				if(draw_particle_sphere)
				{
					DebugSceneNode* debug_node = debug_node_pool.Get();
					debug_node->mat = Matrix::Scale(pe.radius * 2) * Matrix::Translation(pe.pos) * game_level->camera.matViewProj;
					debug_node->type = DebugSceneNode::Sphere;
					debug_node->group = DebugSceneNode::ParticleRadius;
					draw_batch.debug_nodes.push_back(debug_node);
				}
			}
		}

		if(tmp_area.tpes.empty())
			draw_batch.tpes = nullptr;
		else
			draw_batch.tpes = &tmp_area.tpes;
	}
	else
		draw_batch.tpes = nullptr;

	// electros
	if(IsSet(draw_flags, DF_LIGHTINGS) && !tmp_area.electros.empty())
		draw_batch.electros = &tmp_area.electros;
	else
		draw_batch.electros = nullptr;

	// portals
	if(IsSet(draw_flags, DF_PORTALS) && area.area_type != LevelArea::Type::Building)
	{
		Portal* portal = game_level->location->portal;
		while(portal)
		{
			if(game_level->location->outside || game_level->dungeon_level == portal->at_level)
				draw_batch.portals.push_back(portal);
			portal = portal->next_portal;
		}
	}

	// areas
	if(IsSet(draw_flags, DF_AREA))
		ListAreas(area);

	// colliders
	if(draw_col)
	{
		for(vector<CollisionObject>::iterator it = tmp_area.colliders.begin(), end = tmp_area.colliders.end(); it != end; ++it)
		{
			DebugSceneNode::Type type = DebugSceneNode::MaxType;
			Vec3 scale;
			float rot = 0.f;

			switch(it->type)
			{
			case CollisionObject::RECTANGLE:
				{
					scale = Vec3(it->w, 1, it->h);
					type = DebugSceneNode::Box;
				}
				break;
			case CollisionObject::RECTANGLE_ROT:
				{
					scale = Vec3(it->w, 1, it->h);
					type = DebugSceneNode::Box;
					rot = it->rot;
				}
				break;
			case CollisionObject::SPHERE:
				{
					scale = Vec3(it->radius, 1, it->radius);
					type = DebugSceneNode::Cylinder;
				}
				break;
			default:
				break;
			}

			if(type != DebugSceneNode::MaxType)
			{
				DebugSceneNode* node = debug_node_pool.Get();
				node->type = type;
				node->group = DebugSceneNode::Collider;
				node->mat = Matrix::Scale(scale) * Matrix::RotationY(rot) * Matrix::Translation(it->pt.x, 1.f, it->pt.y) * game_level->camera.matViewProj;
				draw_batch.debug_nodes.push_back(node);
			}
		}
	}

	// physics
	if(draw_phy)
	{
		const btCollisionObjectArray& cobjs = phy_world->getCollisionObjectArray();
		const int count = cobjs.size();

		for(int i = 0; i < count; ++i)
		{
			const btCollisionObject* cobj = cobjs[i];
			const btCollisionShape* shape = cobj->getCollisionShape();
			Matrix m_world;
			cobj->getWorldTransform().getOpenGLMatrix(&m_world._11);

			switch(shape->getShapeType())
			{
			case BOX_SHAPE_PROXYTYPE:
				{
					const btBoxShape* box = (const btBoxShape*)shape;
					DebugSceneNode* node = debug_node_pool.Get();
					node->type = DebugSceneNode::Box;
					node->group = DebugSceneNode::Physic;
					node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_world * game_level->camera.matViewProj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case CAPSULE_SHAPE_PROXYTYPE:
				{
					const btCapsuleShape* capsule = (const btCapsuleShape*)shape;
					float r = capsule->getRadius();
					float h = capsule->getHalfHeight();
					DebugSceneNode* node = debug_node_pool.Get();
					node->type = DebugSceneNode::Capsule;
					node->group = DebugSceneNode::Physic;
					node->mat = Matrix::Scale(r, h + r, r) * m_world * game_level->camera.matViewProj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case CYLINDER_SHAPE_PROXYTYPE:
				{
					const btCylinderShape* cylinder = (const btCylinderShape*)shape;
					DebugSceneNode* node = debug_node_pool.Get();
					node->type = DebugSceneNode::Cylinder;
					node->group = DebugSceneNode::Physic;
					Vec3 v = ToVec3(cylinder->getHalfExtentsWithoutMargin());
					node->mat = Matrix::Scale(v.x, v.y / 2, v.z) * m_world * game_level->camera.matViewProj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case TERRAIN_SHAPE_PROXYTYPE:
				break;
			case COMPOUND_SHAPE_PROXYTYPE:
				{
					const btCompoundShape* compound = (const btCompoundShape*)shape;
					int count = compound->getNumChildShapes();

					for(int i = 0; i < count; ++i)
					{
						const btCollisionShape* child = compound->getChildShape(i);
						if(child->getShapeType() == BOX_SHAPE_PROXYTYPE)
						{
							btBoxShape* box = (btBoxShape*)child;
							DebugSceneNode* node = debug_node_pool.Get();
							node->type = DebugSceneNode::Box;
							node->group = DebugSceneNode::Physic;
							Matrix m_child;
							compound->getChildTransform(i).getOpenGLMatrix(&m_child._11);
							node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_child * m_world * game_level->camera.matViewProj;
							draw_batch.debug_nodes.push_back(node);
						}
						else
						{
							// TODO
							assert(0);
						}
					}
				}
				break;
			case TRIANGLE_MESH_SHAPE_PROXYTYPE:
				{
					DebugSceneNode* node = debug_node_pool.Get();
					const btBvhTriangleMeshShape* trimesh = (const btBvhTriangleMeshShape*)shape;
					node->type = DebugSceneNode::TriMesh;
					node->group = DebugSceneNode::Physic;
					node->mat = m_world * game_level->camera.matViewProj;
					node->mesh_ptr = (void*)trimesh->getMeshInterface();
					draw_batch.debug_nodes.push_back(node);
				}
			default:
				break;
			}
		}
	}

	std::sort(draw_batch.nodes.begin(), draw_batch.nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		if(node1->flags == node2->flags)
			return node1->mesh_inst > node2->mesh_inst;
		else
			return node1->flags < node2->flags;
	});
}

//=================================================================================================
void Game::ListDrawObjectsUnit(FrustumPlanes& frustum, bool outside, Unit& u)
{
	if(!frustum.SphereToFrustum(u.visual_pos, u.GetSphereRadius()))
		return;

	// add stun effect
	if(u.IsAlive())
	{
		Effect* effect = u.FindEffect(EffectId::Stun);
		if(effect)
		{
			StunEffect& stun = Add1(draw_batch.stuns);
			stun.pos = u.GetHeadPoint();
			stun.time = effect->time;
		}
	}

	// ustaw koœci
	if(u.data->type == UNIT_TYPE::HUMAN)
		u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.mesh_inst->SetupBones();

	bool selected = (pc->data.before_player == BP_UNIT && pc->data.before_player_ptr.unit == &u)
		|| (game_state == GS_LEVEL && ((pc->data.action_ready && pc->data.action_ok && pc->data.action_target == u)
		|| (pc->unit->action == A_CAST && pc->unit->action_unit == u)));

	// dodaj scene node
	SceneNode* node = node_pool.Get();
	node->billboard = false;
	node->mat = Matrix::RotationY(u.rot) * Matrix::Translation(u.visual_pos);
	node->mesh_inst = u.mesh_inst;
	node->flags = SceneNode::F_ANIMATED;
	node->tex_override = u.data->GetTextureOverride();
	node->parent_mesh_inst = nullptr;
	node->tint = Vec4(1, 1, 1, 1);

	// ustawienia œwiat³a
	int lights = -1;
	if(!outside)
	{
		assert(u.area);
		lights = GatherDrawBatchLights(*u.area, node, u.pos.x, u.pos.z, u.GetSphereRadius());
	}
	node->lights = lights;
	if(selected)
	{
		if(cl_glow)
		{
			GlowNode& glow = Add1(draw_batch.glow_nodes);
			glow.node = node;
			glow.type = GlowNode::Unit;
			glow.ptr = &u;
			glow.alpha = false;
		}
		else
			node->tint = Vec4(2, 2, 2, 1);
	}
	AddOrSplitSceneNode(node, (u.HaveArmor() && u.GetArmor().armor_unit_type == ArmorUnitType::HUMAN && u.GetArmor().mesh) ? 1 : 0);

	// pancerz
	if(u.HaveArmor() && u.GetArmor().mesh)
	{
		const Armor& armor = u.GetArmor();
		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;
		node2->mesh = armor.mesh;
		node2->parent_mesh_inst = u.mesh_inst;
		node2->mat = node->mat;
		node2->flags = SceneNode::F_ANIMATED;
		node2->tex_override = armor.GetTextureOverride();
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = false;
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		AddOrSplitSceneNode(node2);
	}

	// przedmiot w d³oni
	Mesh* right_hand_item = nullptr;
	int right_hand_item_flags = 0;
	bool w_dloni = false;

	switch(u.weapon_state)
	{
	case WS_HIDDEN:
		break;
	case WS_TAKEN:
		if(u.weapon_taken == W_BOW)
		{
			if(u.action == A_SHOOT)
			{
				if(u.animation_state != 2)
					right_hand_item = game_res->aArrow;
			}
			else
				right_hand_item = game_res->aArrow;
		}
		else if(u.weapon_taken == W_ONE_HANDED)
			w_dloni = true;
		break;
	case WS_TAKING:
		if(u.animation_state == 1)
		{
			if(u.weapon_taken == W_BOW)
				right_hand_item = game_res->aArrow;
			else
				w_dloni = true;
		}
		break;
	case WS_HIDING:
		if(u.animation_state == 0)
		{
			if(u.weapon_hiding == W_BOW)
				right_hand_item = game_res->aArrow;
			else
				w_dloni = true;
		}
		break;
	}

	if(u.used_item && u.action != A_USE_ITEM)
	{
		right_hand_item = u.used_item->mesh;
		if(IsSet(u.used_item->flags, ITEM_ALPHA))
			right_hand_item_flags = SceneNode::F_ALPHA_TEST;
	}

	Matrix mat_scale;
	if(u.human_data)
	{
		Vec2 scale = u.human_data->GetScale();
		scale.x = 1.f / scale.x;
		scale.y = 1.f / scale.y;
		mat_scale = Matrix::Scale(scale.x, scale.y, scale.x);
	}
	else
		mat_scale = Matrix::IdentityMatrix;

	// broñ
	Mesh* mesh;
	if(u.HaveWeapon() && right_hand_item != (mesh = u.GetWeapon().mesh))
	{
		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(w_dloni ? NAMES::point_weapon : NAMES::point_hidden_weapon);
		assert(point);

		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = mesh;
		node2->flags = 0;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = false;
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		AddOrSplitSceneNode(node2);

		// hitbox broni
		if(draw_hitbox && u.weapon_state == WS_TAKEN && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = mesh->FindPoint("hit");
			assert(box && box->IsBox());

			DebugSceneNode* debug_node = debug_node_pool.Get();
			debug_node->mat = box->mat * node2->mat * game_level->camera.matViewProj;
			debug_node->type = DebugSceneNode::Box;
			debug_node->group = DebugSceneNode::Hitbox;
			draw_batch.debug_nodes.push_back(debug_node);
		}
	}

	// tarcza
	if(u.HaveShield() && u.GetShield().mesh)
	{
		Mesh* shield = u.GetShield().mesh;
		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(w_dloni ? NAMES::point_shield : NAMES::point_shield_hidden);
		assert(point);

		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = shield;
		node2->flags = 0;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = false;
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		AddOrSplitSceneNode(node2);

		// hitbox tarczy
		if(draw_hitbox && u.weapon_state == WS_TAKEN && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = shield->FindPoint("hit");
			assert(box && box->IsBox());

			DebugSceneNode* debug_node = debug_node_pool.Get();
			node->mat = box->mat * node2->mat * game_level->camera.matViewProj;
			debug_node->type = DebugSceneNode::Box;
			debug_node->group = DebugSceneNode::Hitbox;
			draw_batch.debug_nodes.push_back(debug_node);
		}
	}

	// jakiœ przedmiot
	if(right_hand_item)
	{
		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);

		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = right_hand_item;
		node2->flags = right_hand_item_flags;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = IsSet(right_hand_item_flags, SceneNode::F_ALPHA_TEST);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		AddOrSplitSceneNode(node2);
	}

	// ³uk
	if(u.HaveBow())
	{
		bool w_dloni;

		switch(u.weapon_state)
		{
		case WS_HIDING:
			w_dloni = (u.weapon_hiding == W_BOW && u.animation_state == 0);
			break;
		case WS_HIDDEN:
			w_dloni = false;
			break;
		case WS_TAKING:
			w_dloni = (u.weapon_taken == W_BOW && u.animation_state == 1);
			break;
		case WS_TAKEN:
			w_dloni = (u.weapon_taken == W_BOW);
			break;
		}

		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;

		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(w_dloni ? NAMES::point_bow : NAMES::point_shield_hidden);
		assert(point);

		if(u.action == A_SHOOT)
		{
			u.bow_instance->SetupBones();
			node2->mesh_inst = u.bow_instance;
			node2->parent_mesh_inst = nullptr;
			node2->flags = SceneNode::F_ANIMATED;
		}
		else
		{
			node2->mesh = u.GetBow().mesh;
			node2->flags = 0;
		}

		Matrix m1;
		if(w_dloni)
			m1 = Matrix::RotationZ(-PI / 2) * point->mat * u.mesh_inst->mat_bones[point->bone];
		else
			m1 = point->mat * u.mesh_inst->mat_bones[point->bone];
		node2->mat = mat_scale * m1 * node->mat;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = false;
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		AddOrSplitSceneNode(node2);
	}

	// w³osy/broda/brwi u ludzi
	if(u.data->type == UNIT_TYPE::HUMAN)
	{
		Human& h = *u.human_data;

		// brwi
		SceneNode* node2 = node_pool.Get();
		node2->billboard = false;
		node2->mesh = game_res->aEyebrows;
		node2->parent_mesh_inst = node->mesh_inst;
		node2->flags = SceneNode::F_ANIMATED;
		node2->mat = node->mat;
		node2->tex_override = nullptr;
		node2->tint = h.hair_color;
		node2->lights = lights;
		if(selected)
		{
			if(cl_glow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.type = GlowNode::Unit;
				glow.ptr = &u;
				glow.alpha = false;
			}
			else
			{
				node2->tint.x *= 2;
				node2->tint.y *= 2;
				node2->tint.z *= 2;
			}
		}
		AddOrSplitSceneNode(node2);

		// w³osy
		if(h.hair != -1)
		{
			SceneNode* node3 = node_pool.Get();
			node3->billboard = false;
			node3->mesh = game_res->aHair[h.hair];
			node3->parent_mesh_inst = node->mesh_inst;
			node3->flags = SceneNode::F_ANIMATED;
			node3->mat = node->mat;
			node3->tex_override = nullptr;
			node3->tint = h.hair_color;
			node3->lights = lights;
			if(selected)
			{
				if(cl_glow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
					glow.node = node3;
					glow.type = GlowNode::Unit;
					glow.ptr = &u;
					glow.alpha = false;
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			AddOrSplitSceneNode(node3);
		}

		// broda
		if(h.beard != -1)
		{
			SceneNode* node3 = node_pool.Get();
			node3->billboard = false;
			node3->mesh = game_res->aBeard[h.beard];
			node3->parent_mesh_inst = node->mesh_inst;
			node3->flags = SceneNode::F_ANIMATED;
			node3->mat = node->mat;
			node3->tex_override = nullptr;
			node3->tint = h.hair_color;
			node3->lights = lights;
			if(selected)
			{
				if(cl_glow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
					glow.node = node3;
					glow.type = GlowNode::Unit;
					glow.ptr = &u;
					glow.alpha = false;
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			AddOrSplitSceneNode(node3);
		}

		// w¹sy
		if(h.mustache != -1 && (h.beard == -1 || !g_beard_and_mustache[h.beard]))
		{
			SceneNode* node3 = node_pool.Get();
			node3->billboard = false;
			node3->mesh = game_res->aMustache[h.mustache];
			node3->parent_mesh_inst = node->mesh_inst;
			node3->flags = SceneNode::F_ANIMATED;
			node3->mat = node->mat;
			node3->tex_override = nullptr;
			node3->tint = h.hair_color;
			node3->lights = lights;
			if(selected)
			{
				if(cl_glow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
					glow.node = node3;
					glow.type = GlowNode::Unit;
					glow.ptr = &u;
					glow.alpha = false;
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			AddOrSplitSceneNode(node3);
		}
	}

	// pseudo hitbox postaci
	if(draw_unit_radius)
	{
		float h = u.GetUnitHeight() / 2;
		DebugSceneNode* debug_node = debug_node_pool.Get();
		debug_node->mat = Matrix::Scale(u.GetUnitRadius(), h, u.GetUnitRadius()) * Matrix::Translation(u.GetColliderPos() + Vec3(0, h, 0)) * game_level->camera.matViewProj;
		debug_node->type = DebugSceneNode::Cylinder;
		debug_node->group = DebugSceneNode::UnitRadius;
		draw_batch.debug_nodes.push_back(debug_node);
	}
	if(draw_hitbox)
	{
		float h = u.GetUnitHeight() / 2;
		Box box;
		u.GetBox(box);
		DebugSceneNode* debug_node = debug_node_pool.Get();
		debug_node->mat = Matrix::Scale(box.SizeX() / 2, h, box.SizeZ() / 2) * Matrix::Translation(u.pos + Vec3(0, h, 0)) * game_level->camera.matViewProj;
		debug_node->type = DebugSceneNode::Box;
		debug_node->group = DebugSceneNode::Hitbox;
		draw_batch.debug_nodes.push_back(debug_node);
	}
}

//=================================================================================================
void Game::AddObjectToDrawBatch(LevelArea& area, const Object& o, FrustumPlanes& frustum)
{
	SceneNode* node = node_pool.Get();
	if(!o.IsBillboard())
	{
		node->billboard = false;
		node->mat = Matrix::Transform(o.pos, o.rot, o.scale);
	}
	else
	{
		node->billboard = true;
		node->mat = Matrix::CreateLookAt(o.pos, game_level->camera.center);
	}

	node->mesh = o.mesh;
	int alpha = o.RequireAlphaTest();
	if(alpha == -1)
		node->flags = 0;
	else if(alpha == 0)
		node->flags = SceneNode::F_ALPHA_TEST;
	else
		node->flags = SceneNode::F_ALPHA_TEST | SceneNode::F_NO_CULLING;
	node->tex_override = nullptr;
	node->tint = Vec4(1, 1, 1, 1);
	if(!IsSet(node->mesh->head.flags, Mesh::F_SPLIT))
	{
		if(area.area_type != LevelArea::Type::Outside)
			node->lights = GatherDrawBatchLights(area, node, o.pos.x, o.pos.z, o.GetRadius());
		AddOrSplitSceneNode(node);
	}
	else
	{
		const Mesh& mesh = node->GetMesh();
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_BINORMALS;

		//#define DEBUG_SPLITS
#ifdef DEBUG_SPLITS
		static int req = 33;
		if(input->PressedRelease('8'))
		{
			++req;
			if(req == mesh.head.n_subs)
				req = 0;
		}
#endif

		// for simplicity original node in unused and freed at end
#ifndef DEBUG_SPLITS
		for(int i = 0; i < mesh.head.n_subs; ++i)
#else
		int i = req;
#endif
		{
			Vec3 pos = Vec3::Transform(mesh.splits[i].pos, node->mat);
			const float radius = mesh.splits[i].radius*o.scale;
			if(frustum.SphereToFrustum(pos, radius))
			{
				SceneNode* node2 = node_pool.Get();
				node2->billboard = false;
				node2->mesh_inst = node->mesh_inst;
				node2->mat = node->mat;
				node2->flags = node->flags;
				node2->parent_mesh_inst = nullptr;
				node2->subs = SPLIT_INDEX | i;
				node2->tint = node->tint;
				node2->lights = node->lights;
				node2->tex_override = node->tex_override;
				if(cl_normalmap && mesh.subs[i].tex_normal)
					node2->flags |= SceneNode::F_NORMAL_MAP;
				if(cl_specularmap && mesh.subs[i].tex_specular)
					node2->flags |= SceneNode::F_SPECULAR_MAP;
				if(area.area_type != LevelArea::Type::Outside)
					node2->lights = GatherDrawBatchLights(area, node2, pos.x, pos.z, radius, i);
				draw_batch.nodes.push_back(node2);
			}

#ifdef DEBUG_SPLITS
			DebugSceneNode* debug = debug_node_pool.Get();
			debug->type = DebugSceneNode::Sphere;
			debug->group = DebugSceneNode::Physic;
			D3DXMatrixScaling(&m1, radius * 2);
			D3DXMatrixTranslation(&m2, pos);
			D3DXMatrixMultiply(&m3, &m1, &m2);
			D3DXMatrixMultiply(&debug->mat, &m3, &game_level->camera.matViewProj);
			draw_batch.debug_nodes.push_back(debug);
#endif
		}

		node_pool.Free(node);
	}
}

//=================================================================================================
void Game::ListAreas(LevelArea& area)
{
	if(area.area_type == LevelArea::Type::Outside)
	{
		if(game_level->city_ctx)
		{
			if(IsSet(game_level->city_ctx->flags, City::HaveExit))
			{
				for(vector<EntryPoint>::const_iterator entry_it = game_level->city_ctx->entry_points.begin(), entry_end = game_level->city_ctx->entry_points.end();
					entry_it != entry_end; ++entry_it)
				{
					const EntryPoint& e = *entry_it;
					Area& a = Add1(draw_batch.areas);
					a.v[0] = Vec3(e.exit_region.v1.x, e.exit_y, e.exit_region.v2.y);
					a.v[1] = Vec3(e.exit_region.v2.x, e.exit_y, e.exit_region.v2.y);
					a.v[2] = Vec3(e.exit_region.v1.x, e.exit_y, e.exit_region.v1.y);
					a.v[3] = Vec3(e.exit_region.v2.x, e.exit_y, e.exit_region.v1.y);
				}
			}

			for(vector<InsideBuilding*>::const_iterator it = game_level->city_ctx->inside_buildings.begin(), end = game_level->city_ctx->inside_buildings.end(); it != end; ++it)
			{
				const InsideBuilding& ib = **it;
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(ib.enter_region.v1.x, ib.enter_y, ib.enter_region.v2.y);
				a.v[1] = Vec3(ib.enter_region.v2.x, ib.enter_y, ib.enter_region.v2.y);
				a.v[2] = Vec3(ib.enter_region.v1.x, ib.enter_y, ib.enter_region.v1.y);
				a.v[3] = Vec3(ib.enter_region.v2.x, ib.enter_y, ib.enter_region.v1.y);
			}
		}

		if(!game_level->city_ctx || !IsSet(game_level->city_ctx->flags, City::HaveExit))
		{
			const float H1 = -10.f;
			const float H2 = 30.f;

			// góra
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
			}

			// dó³
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[2] = Vec3(33.f, H2, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}

			// lewa
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(33.f, H2, 33.f);
				a.v[2] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(33.f, H2, 256.f - 33.f);
			}

			// prawa
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}
		}
		draw_batch.area_range = 10.f;
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HaveUpStairs())
		{
			Area& a = Add1(draw_batch.areas);
			a.v[0] = a.v[1] = a.v[2] = a.v[3] = PtToPos(lvl.staircase_up);
			switch(lvl.staircase_up_dir)
			{
			case GDIR_DOWN:
				a.v[0] += Vec3(-0.85f, 2.87f, 0.85f);
				a.v[1] += Vec3(0.85f, 2.87f, 0.85f);
				a.v[2] += Vec3(-0.85f, 0.83f, 0.85f);
				a.v[3] += Vec3(0.85f, 0.83f, 0.85f);
				break;
			case GDIR_UP:
				a.v[0] += Vec3(0.85f, 2.87f, -0.85f);
				a.v[1] += Vec3(-0.85f, 2.87f, -0.85f);
				a.v[2] += Vec3(0.85f, 0.83f, -0.85f);
				a.v[3] += Vec3(-0.85f, 0.83f, -0.85f);
				break;
			case GDIR_RIGHT:
				a.v[0] += Vec3(-0.85f, 2.87f, -0.85f);
				a.v[1] += Vec3(-0.85f, 2.87f, 0.85f);
				a.v[2] += Vec3(-0.85f, 0.83f, -0.85f);
				a.v[3] += Vec3(-0.85f, 0.83f, 0.85f);
				break;
			case GDIR_LEFT:
				a.v[0] += Vec3(0.85f, 2.87f, 0.85f);
				a.v[1] += Vec3(0.85f, 2.87f, -0.85f);
				a.v[2] += Vec3(0.85f, 0.83f, 0.85f);
				a.v[3] += Vec3(0.85f, 0.83f, -0.85f);
				break;
			}
		}
		if(inside->HaveDownStairs())
		{
			Area& a = Add1(draw_batch.areas);
			a.v[0] = a.v[1] = a.v[2] = a.v[3] = PtToPos(lvl.staircase_down);
			switch(lvl.staircase_down_dir)
			{
			case GDIR_DOWN:
				a.v[0] += Vec3(-0.85f, 0.45f, 0.85f);
				a.v[1] += Vec3(0.85f, 0.45f, 0.85f);
				a.v[2] += Vec3(-0.85f, -1.55f, 0.85f);
				a.v[3] += Vec3(0.85f, -1.55f, 0.85f);
				break;
			case GDIR_UP:
				a.v[0] += Vec3(0.85f, 0.45f, -0.85f);
				a.v[1] += Vec3(-0.85f, 0.45f, -0.85f);
				a.v[2] += Vec3(0.85f, -1.55f, -0.85f);
				a.v[3] += Vec3(-0.85f, -1.55f, -0.85f);
				break;
			case GDIR_RIGHT:
				a.v[0] += Vec3(-0.85f, 0.45f, -0.85f);
				a.v[1] += Vec3(-0.85f, 0.45f, 0.85f);
				a.v[2] += Vec3(-0.85f, -1.55f, -0.85f);
				a.v[3] += Vec3(-0.85f, -1.55f, 0.85f);
				break;
			case GDIR_LEFT:
				a.v[0] += Vec3(0.85f, 0.45f, 0.85f);
				a.v[1] += Vec3(0.85f, 0.45f, -0.85f);
				a.v[2] += Vec3(0.85f, -1.55f, 0.85f);
				a.v[3] += Vec3(0.85f, -1.55f, -0.85f);
				break;
			}
		}
		draw_batch.area_range = 5.f;
	}
	else
	{
		// exit from building
		Area& a = Add1(draw_batch.areas);
		const Box2d& region = static_cast<InsideBuilding&>(area).exit_region;
		a.v[0] = Vec3(region.v1.x, 0.1f, region.v2.y);
		a.v[1] = Vec3(region.v2.x, 0.1f, region.v2.y);
		a.v[2] = Vec3(region.v1.x, 0.1f, region.v1.y);
		a.v[3] = Vec3(region.v2.x, 0.1f, region.v1.y);
		draw_batch.area_range = 5.f;
	}

	// action area2
	if(pc->data.action_ready)
		PrepareAreaPath();
}

//=================================================================================================
void Game::PrepareAreaPath()
{
	if(!pc->CanUseAction())
	{
		pc->data.action_ready = false;
		sound_mgr->PlaySound2d(game_res->sCancel);
		return;
	}

	Action& action = pc->GetAction();
	Area2* area_ptr = area2_pool.Get();
	Area2& area = *area_ptr;
	area.ok = 2;
	draw_batch.areas2.push_back(area_ptr);

	const float h = 0.06f;
	const Vec3& pos = pc->unit->pos;
	Vec3 from = pc->unit->GetPhysicsPos();

	switch(action.area)
	{
	case Action::LINE:
		{
			float rot = Clip(pc->unit->rot + PI + pc->data.action_rot);
			const int steps = 10;

			// find max line
			float t;
			Vec3 dir(sin(rot)*action.area_size.x, 0, cos(rot)*action.area_size.x);
			bool ignore_units = IsSet(action.flags, Action::F_IGNORE_UNITS);
			game_level->LineTest(pc->unit->cobj->getCollisionShape(), from, dir, [this, ignore_units](btCollisionObject* obj, bool)
			{
				int flags = obj->getCollisionFlags();
				if(IsSet(flags, CG_TERRAIN))
					return LT_IGNORE;
				if(IsSet(flags, CG_UNIT) && (obj->getUserPointer() == pc->unit || ignore_units))
					return LT_IGNORE;
				return LT_COLLIDE;
			}, t);

			float len = action.area_size.x * t;

			if(game_level->location->outside && pc->unit->area->area_type == LevelArea::Type::Outside)
			{
				// build line on terrain
				area.points.clear();
				area.faces.clear();

				Vec3 active_pos = pos;
				Vec3 step = dir * t / (float)steps;
				Vec3 unit_offset(pc->unit->pos.x, 0, pc->unit->pos.z);
				float len_step = len / steps;
				float active_step = 0;
				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < steps; ++i)
				{
					float current_h = game_level->terrain->GetH(active_pos) + h;
					area.points.push_back(Vec3::Transform(Vec3(-action.area_size.y, current_h, active_step), mat) + unit_offset);
					area.points.push_back(Vec3::Transform(Vec3(+action.area_size.y, current_h, active_step), mat) + unit_offset);

					active_pos += step;
					active_step += len_step;
				}

				for(int i = 0; i < steps - 1; ++i)
				{
					area.faces.push_back(i * 2);
					area.faces.push_back((i + 1) * 2);
					area.faces.push_back(i * 2 + 1);
					area.faces.push_back((i + 1) * 2);
					area.faces.push_back(i * 2 + 1);
					area.faces.push_back((i + 1) * 2 + 1);
				}
			}
			else
			{
				// build line on flat terrain
				area.points.resize(4);
				area.points[0] = Vec3(-action.area_size.y, h, 0);
				area.points[1] = Vec3(-action.area_size.y, h, len);
				area.points[2] = Vec3(action.area_size.y, h, 0);
				area.points[3] = Vec3(action.area_size.y, h, len);

				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < 4; ++i)
					area.points[i] = Vec3::Transform(area.points[i], mat) + pc->unit->pos;

				area.faces.clear();
				area.faces.push_back(0);
				area.faces.push_back(1);
				area.faces.push_back(2);
				area.faces.push_back(1);
				area.faces.push_back(2);
				area.faces.push_back(3);
			}

			pc->data.action_ok = true;
		}
		break;

	case Action::POINT:
		{
			const float cam_max = 4.63034153f;
			const float cam_min = 4.08159288f;
			const float radius = action.area_size.x / 2;
			const float unit_r = pc->unit->GetUnitRadius();

			float range = (Clamp(game_level->camera.rot.y, cam_min, cam_max) - cam_min) / (cam_max - cam_min) * action.area_size.y;
			if(range < radius + unit_r)
				range = radius + unit_r;
			const float min_t = action.area_size.x / range / 2;
			float rot = Clip(pc->unit->rot + PI);
			static vector<float> t_forward;
			static vector<float> t_backward;

			// what are we doing here you may ask :) first do line test from player to max point, then from max point to player
			// this will allow to find free spot event behind small rock on ground
			t_forward.clear();
			t_backward.clear();

			auto clbk = [this](btCollisionObject* obj, bool second)
			{
				int flags = obj->getCollisionFlags();
				if(!second)
				{
					if(IsSet(flags, CG_TERRAIN))
						return LT_IGNORE;
					if(IsSet(flags, CG_UNIT) && obj->getUserPointer() == pc->unit)
						return LT_IGNORE;
					return LT_COLLIDE;
				}
				else
				{
					if(IsSet(flags, CG_BUILDING | CG_DOOR))
						return LT_END;
					else
						return LT_COLLIDE;
				}
			};

			float t, end_t;
			Vec3 dir_normal(sin(rot), 0, cos(rot));
			Vec3 dir = dir_normal * range;
			game_level->LineTest(game_level->shape_summon, from, dir, clbk, t, &t_forward, true, &end_t);
			game_level->LineTest(game_level->shape_summon, from + dir, -dir, clbk, t, &t_backward);

			// merge t's to find free spots, we only use last one
			static vector<pair<float, bool>> t_merged;
			const bool OPEN = true;
			const bool CLOSE = false;
			t_merged.clear();
			float unit_t = pc->unit->GetUnitRadius() / range;
			if(unit_t < end_t)
			{
				t_merged.push_back(pair<float, bool>(0.f, OPEN));
				t_merged.push_back(pair<float, bool>(unit_t, CLOSE));
			}
			for(float f : t_forward)
			{
				if(f >= end_t)
					break;
				t_merged.push_back(pair<float, bool>(f, OPEN));
			}
			t_merged.push_back(pair<float, bool>(end_t, OPEN));
			for(float f : t_backward)
			{
				if(f == 0.f)
					f = 1.f;
				else
					f = 1.f - (f + radius / range);
				if(f >= end_t)
					continue;
				t_merged.push_back(pair<float, bool>(f, CLOSE));
			}
			std::sort(t_merged.begin(), t_merged.end(), [](const pair<float, bool>& a, const pair<float, bool>& b) { return a.first < b.first; });

			// get list of free ranges
			//const float extra_t = range / (range + min_t);
			int open = 0;
			float start = 0.f;
			static vector<Vec2> results;
			results.clear();
			for(auto& point : t_merged)
			{
				if(point.second == CLOSE)
				{
					--open;
					start = point.first;
				}
				else
				{
					if(open == 0)
					{
						float len = point.first - start;
						if(len >= min_t)
							results.push_back(Vec2(start, point.first));
					}
					++open;
				}
			}
			if(open == 0)
			{
				float len = 1.f - start;
				if(len >= min_t)
					results.push_back(Vec2(start, 1.f));
			}

			// get best T, actualy last one
			if(results.empty())
				t = -1.f;
			else
				t = results.back().y;

			if(t < 0)
			{
				// no free space
				t = end_t;
				area.ok = 0;
				pc->data.action_ok = false;
			}
			else
			{
				pc->data.action_ok = true;
			}

			// build circle
			PrepareAreaPathCircle(area, radius, t * range, rot);

			// build yellow circle
			if(t != 1.f)
			{
				Area2* y_area = area2_pool.Get();
				y_area->ok = 1;
				PrepareAreaPathCircle(*y_area, radius, range, rot);
				draw_batch.areas2.push_back(y_area);
			}

			// set action
			if(pc->data.action_ok)
				pc->data.action_point = area.points[0];
		}
		break;

	case Action::TARGET:
		{
			Unit* target;
			if(GKey.KeyDownAllowed(GK_MOVE_BACK))
			{
				// cast on self
				target = pc->unit;
			}
			else
			{
				// raytest
				RaytestClosestUnitDeadOrAliveCallback clbk(pc->unit);
				Vec3 from = game_level->camera.from;
				Vec3 dir = (game_level->camera.to - from).Normalized();
				Vec3 to = from + dir * action.area_size.x;
				phy_world->rayTest(ToVector3(from), ToVector3(to), clbk);
				target = clbk.hit;
			}

			if(target)
			{
				pc->data.action_ok = true;
				pc->data.action_point = target->pos;
				pc->data.action_target = target;
			}
			else
			{
				pc->data.action_ok = false;
				pc->data.action_target = nullptr;
			}

			area2_pool.Free(area_ptr);
			draw_batch.areas2.clear();
		}
		break;
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, float radius, float range, float rot)
{
	const float h = 0.06f;
	const int circle_points = 10;

	area.points.resize(circle_points + 1);
	area.points[0] = Vec3(0, h, 0 + range);
	float angle = 0;
	for(int i = 0; i < circle_points; ++i)
	{
		area.points[i + 1] = Vec3(sin(angle)*radius, h, cos(angle)*radius + range);
		angle += (PI * 2) / circle_points;
	}

	bool outside = (game_level->location->outside && pc->unit->area->area_type == LevelArea::Type::Outside);
	Matrix mat = Matrix::RotationY(rot);
	for(int i = 0; i < circle_points + 1; ++i)
	{
		area.points[i] = Vec3::Transform(area.points[i], mat) + pc->unit->pos;
		if(outside)
			area.points[i].y = game_level->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circle_points; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circle_points + 1) ? 1 : (i + 2));
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, const Vec3& pos, float radius)
{
	const float h = 0.06f;
	const int circle_points = 10;

	area.points.resize(circle_points + 1);
	area.points[0] = pos + Vec3(0, h, 0);
	float angle = 0;
	for(int i = 0; i < circle_points; ++i)
	{
		area.points[i + 1] = pos + Vec3(sin(angle)*radius, h, cos(angle)*radius);
		angle += (PI * 2) / circle_points;
	}

	bool outside = (game_level->location->outside && pc->unit->area->area_type == LevelArea::Type::Outside);
	if(outside)
	{
		for(int i = 0; i < circle_points + 1; ++i)
			area.points[i].y = game_level->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circle_points; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circle_points + 1) ? 1 : (i + 2));
	}
}

//=================================================================================================
void Game::FillDrawBatchDungeonParts(FrustumPlanes& frustum)
{
	InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	Box box;
	static vector<Light*> lights;
	Light* light[3];
	float range[3], dist;

	if(!IsSet(base.options, BLO_LABYRINTH))
	{
		for(Room* room : lvl.rooms)
		{
			box.v1 = Vec3(float(room->pos.x * 2), 0, float(room->pos.y * 2));
			box.v2 = box.v1;
			box.v2 += Vec3(float(room->size.x * 2), 4, float(room->size.y * 2));

			if(!frustum.BoxToFrustum(box))
				continue;

			// zbierz listê œwiate³ oœwietlaj¹ce ten pokój
			Vec2 v1(box.v1.x, box.v1.z);
			Vec2 v2(box.v2.x, box.v2.z);
			Vec2 ext = (v2 - v1) / 2;
			Vec2 mid = v1 + ext;

			lights.clear();
			for(vector<Light>::iterator it3 = lvl.lights.begin(), end3 = lvl.lights.end(); it3 != end3; ++it3)
			{
				if(CircleToRectangle(it3->pos.x, it3->pos.z, it3->range, mid.x, mid.y, ext.x, ext.y))
					lights.push_back(&*it3);
			}

			// dla ka¿dego pola
			for(int y = 0; y < room->size.y; ++y)
			{
				for(int x = 0; x < room->size.x; ++x)
				{
					// czy coœ jest na tym polu
					Tile& tile = lvl.map[(x + room->pos.x) + (y + room->pos.y)*lvl.w];
					if(tile.room != room->index || tile.flags == 0 || tile.flags == Tile::F_REVEALED)
						continue;

					// ustaw œwiat³a
					range[0] = range[1] = range[2] = game_level->camera.draw_range;
					light[0] = light[1] = light[2] = nullptr;

					float dx = 2.f*(room->pos.x + x) + 1.f;
					float dz = 2.f*(room->pos.y + y) + 1.f;

					for(vector<Light*>::iterator it2 = lights.begin(), end2 = lights.end(); it2 != end2; ++it2)
					{
						dist = Distance(dx, dz, (*it2)->pos.x, (*it2)->pos.z);
						if(dist < 1.414213562373095f + (*it2)->range && dist < range[2])
						{
							if(dist < range[1])
							{
								if(dist < range[0])
								{
									// wstaw jako 0, 0 i 1 przesuñ
									range[2] = range[1];
									range[1] = range[0];
									range[0] = dist;
									light[2] = light[1];
									light[1] = light[0];
									light[0] = *it2;
								}
								else
								{
									// wstaw jako 1, 1 przesuñ na 2
									range[2] = range[1];
									range[1] = dist;
									light[2] = light[1];
									light[1] = *it2;
								}
							}
							else
							{
								// wstaw jako 2
								range[2] = dist;
								light[2] = *it2;
							}
						}
					}

					// kopiuj w³aœciwoœci œwiat³a
					int lights_id = draw_batch.lights.size();
					Lights& l = Add1(draw_batch.lights);
					for(int i = 0; i < 3; ++i)
					{
						if(!light[i])
						{
							l.ld[i].range = 1.f;
							l.ld[i].pos = Vec3(0, -1000, 0);
							l.ld[i].color = Vec3(0, 0, 0);
						}
						else
						{
							l.ld[i].pos = light[i]->t_pos;
							l.ld[i].range = light[i]->range;
							l.ld[i].color = light[i]->t_color;
						}
					}

					// ustaw macierze
					int matrix_id = draw_batch.matrices.size();
					NodeMatrix& m = Add1(draw_batch.matrices);
					m.matWorld = Matrix::Translation(2.f*(room->pos.x + x), 0, 2.f*(room->pos.y + y));
					m.matCombined = m.matWorld * game_level->camera.matViewProj;

					int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

					// pod³oga
					if(IsSet(tile.flags, Tile::F_FLOOR))
					{
						DungeonPart& dp = Add1(draw_batch.dungeon_parts);
						dp.tex_o = &game_res->tFloor[tex_id];
						dp.start_index = 0;
						dp.primitive_count = 2;
						dp.matrix = matrix_id;
						dp.lights = lights_id;
					}

					// sufit
					if(IsSet(tile.flags, Tile::F_CEILING | Tile::F_LOW_CEILING))
					{
						DungeonPart& dp = Add1(draw_batch.dungeon_parts);
						dp.tex_o = &game_res->tCeil[tex_id];
						dp.start_index = IsSet(tile.flags, Tile::F_LOW_CEILING) ? 12 : 6;
						dp.primitive_count = 2;
						dp.matrix = matrix_id;
						dp.lights = lights_id;
					}

					// œciany
					int d = (tile.flags & 0xFFFF00) >> 8;
					if(d != 0)
					{
						// normalne
						int d2 = (d & 0xF);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(draw_batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part[d2].x;
							dp.primitive_count = dungeon_part[d2].y;
							dp.matrix = matrix_id;
							dp.lights = lights_id;
						}

						// niskie
						/*d2 = ((d & 0xF0)>>4);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(draw_batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part2[d2].x;
							dp.primitive_count = dungeon_part2[d2].y;
							dp.matrix = matrix_id;
							dp.lights = lights_id;
						}*/

						// góra
						d2 = ((d & 0xF00) >> 8);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(draw_batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part3[d2].x;
							dp.primitive_count = dungeon_part3[d2].y;
							dp.matrix = matrix_id;
							dp.lights = lights_id;
						}

						// dó³
						d2 = ((d & 0xF000) >> 12);
						if(d2 != 0)
						{
							DungeonPart& dp = Add1(draw_batch.dungeon_parts);
							dp.tex_o = &game_res->tWall[tex_id];
							dp.start_index = dungeon_part4[d2].x;
							dp.primitive_count = dungeon_part4[d2].y;
							dp.matrix = matrix_id;
							dp.lights = lights_id;
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

		// podziel na kawa³ki u¿ywaj¹c pseudo quad-tree i frustum culling
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

		// dla ka¿dego pola
		for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			Tile& tile = lvl.map[it->x + it->y*lvl.w];
			if(tile.flags == 0 || tile.flags == Tile::F_REVEALED)
				continue;

			Box box(2.f*it->x, -4.f, 2.f*it->y, 2.f*(it->x + 1), 8.f, 2.f*(it->y + 1));
			if(!frustum.BoxToFrustum(box))
				continue;

			range[0] = range[1] = range[2] = game_level->camera.draw_range;
			light[0] = light[1] = light[2] = nullptr;

			float dx = 2.f*it->x + 1.f;
			float dz = 2.f*it->y + 1.f;

			for(vector<Light>::iterator it2 = lvl.lights.begin(), end2 = lvl.lights.end(); it2 != end2; ++it2)
			{
				dist = Distance(dx, dz, it2->pos.x, it2->pos.z);
				if(dist < 1.414213562373095f + it2->range && dist < range[2])
				{
					if(dist < range[1])
					{
						if(dist < range[0])
						{
							// wstaw jako 0, 0 i 1 przesuñ
							range[2] = range[1];
							range[1] = range[0];
							range[0] = dist;
							light[2] = light[1];
							light[1] = light[0];
							light[0] = &*it2;
						}
						else
						{
							// wstaw jako 1, 1 przesuñ na 2
							range[2] = range[1];
							range[1] = dist;
							light[2] = light[1];
							light[1] = &*it2;
						}
					}
					else
					{
						// wstaw jako 2
						range[2] = dist;
						light[2] = &*it2;
					}
				}
			}

			// kopiuj w³aœciwoœci œwiate³
			int lights_id = draw_batch.lights.size();
			Lights& l = Add1(draw_batch.lights);
			for(int i = 0; i < 3; ++i)
			{
				if(!light[i])
				{
					l.ld[i].range = 1.f;
					l.ld[i].pos = Vec3(0, -1000, 0);
					l.ld[i].color = Vec3(0, 0, 0);
				}
				else
				{
					l.ld[i].pos = light[i]->t_pos;
					l.ld[i].range = light[i]->range;
					l.ld[i].color = light[i]->t_color;
				}
			}

			// ustaw macierze
			int matrix_id = draw_batch.matrices.size();
			NodeMatrix& m = Add1(draw_batch.matrices);
			m.matWorld = Matrix::Translation(2.f*it->x, 0, 2.f*it->y);
			m.matCombined = m.matWorld * game_level->camera.matViewProj;

			int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

			// pod³oga
			if(IsSet(tile.flags, Tile::F_FLOOR))
			{
				DungeonPart& dp = Add1(draw_batch.dungeon_parts);
				dp.tex_o = &game_res->tFloor[tex_id];
				dp.start_index = 0;
				dp.primitive_count = 2;
				dp.matrix = matrix_id;
				dp.lights = lights_id;
			}

			// sufit
			if(IsSet(tile.flags, Tile::F_CEILING | Tile::F_LOW_CEILING))
			{
				DungeonPart& dp = Add1(draw_batch.dungeon_parts);
				dp.tex_o = &game_res->tCeil[tex_id];
				dp.start_index = IsSet(tile.flags, Tile::F_LOW_CEILING) ? 12 : 6;
				dp.primitive_count = 2;
				dp.matrix = matrix_id;
				dp.lights = lights_id;
			}

			// œciany
			int d = (tile.flags & 0xFFFF00) >> 8;
			if(d != 0)
			{
				// normalne
				int d2 = (d & 0xF);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(draw_batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part[d2].x;
					dp.primitive_count = dungeon_part[d2].y;
					dp.matrix = matrix_id;
					dp.lights = lights_id;
				}

				// niskie
				d2 = ((d & 0xF0) >> 4);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(draw_batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part2[d2].x;
					dp.primitive_count = dungeon_part2[d2].y;
					dp.matrix = matrix_id;
					dp.lights = lights_id;
				}

				// góra
				d2 = ((d & 0xF00) >> 8);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(draw_batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part3[d2].x;
					dp.primitive_count = dungeon_part3[d2].y;
					dp.matrix = matrix_id;
					dp.lights = lights_id;
				}

				// dó³
				d2 = ((d & 0xF000) >> 12);
				if(d2 != 0)
				{
					DungeonPart& dp = Add1(draw_batch.dungeon_parts);
					dp.tex_o = &game_res->tWall[tex_id];
					dp.start_index = dungeon_part4[d2].x;
					dp.primitive_count = dungeon_part4[d2].y;
					dp.matrix = matrix_id;
					dp.lights = lights_id;
				}
			}
		}

		tiles.clear();
	}

	std::sort(draw_batch.dungeon_parts.begin(), draw_batch.dungeon_parts.end(), [](const DungeonPart& p1, const DungeonPart& p2)
	{
		if(p1.tex_o != p2.tex_o)
			return p1.tex_o->GetIndex() < p2.tex_o->GetIndex();
		else
			return false;
	});
}

//=================================================================================================
void Game::AddOrSplitSceneNode(SceneNode* node, int exclude_subs)
{
	assert(node && node->GetMesh().head.n_subs < 31);

	const Mesh& mesh = node->GetMesh();
	assert(mesh.state == ResourceState::Loaded);
	if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
		node->flags |= SceneNode::F_BINORMALS;
	if(mesh.head.n_subs == 1)
	{
		node->subs = 0x7FFFFFFF;
		if(cl_normalmap && mesh.subs[0].tex_normal)
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(cl_specularmap && mesh.subs[0].tex_specular)
			node->flags |= SceneNode::F_SPECULAR_MAP;
		draw_batch.nodes.push_back(node);
	}
	else
	{
		enum Split
		{
			SplitDefault,
			SplitNormal,
			SplitSpecular,
			SplitNormalSpecular
		};

		int splits[4] = { 0 };
		for(word i = 0, count = mesh.head.n_subs; i < count; ++i)
		{
			const int shift = 1 << i;
			if(!IsSet(exclude_subs, shift))
				splits[(cl_normalmap && mesh.subs[i].tex_normal) + (cl_specularmap && mesh.subs[i].tex_specular) * 2] |= shift;
		}

		int split_count = 0, first = -1;
		for(int i = 0; i < 4; ++i)
		{
			if(splits[i])
			{
				++split_count;
				if(first == -1)
					first = i;
			}
		}

		if(split_count != 1)
		{
			for(int i = first + 1; i < 4; ++i)
			{
				if(splits[i])
				{
					SceneNode* node2 = node_pool.Get();
					node2->billboard = node->billboard;
					node2->mesh_inst = node->mesh_inst;
					node2->mat = node->mat;
					node2->flags = node->flags;
					node2->parent_mesh_inst = nullptr;
					node2->subs = splits[i];
					node2->tint = node->tint;
					node2->lights = node->lights;
					node2->tex_override = node->tex_override;
					if(IsSet(i, SplitNormal))
						node2->flags |= SceneNode::F_NORMAL_MAP;
					if(IsSet(i, SplitSpecular))
						node2->flags |= SceneNode::F_SPECULAR_MAP;
					draw_batch.nodes.push_back(node2);
				}
			}
		}

		if(IsSet(first, SplitNormal))
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(IsSet(first, SplitSpecular))
			node->flags |= SceneNode::F_SPECULAR_MAP;
		node->subs = splits[first];
		draw_batch.nodes.push_back(node);
	}
}

//=================================================================================================
int Game::GatherDrawBatchLights(LevelArea& area, SceneNode* node, float x, float z, float radius, int sub)
{
	assert(radius > 0);

	Light* light[3];
	float range[3], dist;

	light[0] = light[1] = light[2] = nullptr;
	range[0] = range[1] = range[2] = game_level->camera.draw_range;

	if(area.masks.empty())
	{
		for(vector<Light>::iterator it3 = area.lights.begin(), end3 = area.lights.end(); it3 != end3; ++it3)
		{
			dist = Distance(x, z, it3->t_pos.x, it3->t_pos.z);
			if(dist < it3->range + radius && dist < range[2])
			{
				if(dist < range[1])
				{
					if(dist < range[0])
					{
						// wstaw jako 0, 0 i 1 przesuñ
						range[2] = range[1];
						range[1] = range[0];
						range[0] = dist;
						light[2] = light[1];
						light[1] = light[0];
						light[0] = &*it3;
					}
					else
					{
						// wstaw jako 1, 1 przesuñ na 2
						range[2] = range[1];
						range[1] = dist;
						light[2] = light[1];
						light[1] = &*it3;
					}
				}
				else
				{
					// wstaw jako 2
					range[2] = dist;
					light[2] = &*it3;
				}
			}
		}

		if(light[0])
		{
			Lights& lights = Add1(draw_batch.lights);

			for(int i = 0; i < 3; ++i)
			{
				if(!light[i])
				{
					lights.ld[i].range = 1.f;
					lights.ld[i].pos = Vec3(0, -1000, 0);
					lights.ld[i].color = Vec3(0, 0, 0);
				}
				else
				{
					lights.ld[i].pos = light[i]->t_pos;
					lights.ld[i].range = light[i]->range;
					lights.ld[i].color = light[i]->t_color;
				}
			}

			return draw_batch.lights.size() - 1;
		}
		else
			return 0;
	}
	else
	{
		Vec3 lights_pos[3];
		float lights_range[3] = { 0 };
		const Vec2 obj_pos(x, z);
		bool is_split = (node && IsSet(node->GetMesh().head.flags, Mesh::F_SPLIT));
		Vec2 light_pos;

		for(vector<Light>::iterator it3 = area.lights.begin(), end3 = area.lights.end(); it3 != end3; ++it3)
		{
			bool ok = false;
			if(!is_split)
			{
				dist = Distance(x, z, it3->pos.x, it3->pos.z);
				if(IsZero(dist))
					ok = true;
				else if(dist < it3->range + radius && dist < range[2])
				{
					light_pos = Vec2(it3->pos.x, it3->pos.z);
					float range_sum = 0.f;

					// are there any masks between object and light?
					for(vector<LightMask>::iterator it4 = area.masks.begin(), end4 = area.masks.end(); it4 != end4; ++it4)
					{
						if(LineToRectangleSize(obj_pos, light_pos, it4->pos, it4->size))
						{
							// move light to one side of mask
							Vec2 new_pos, new_pos2;
							float new_dist[2];
							if(it4->size.x > it4->size.y)
							{
								new_pos.x = it4->pos.x - it4->size.x;
								new_pos2.x = it4->pos.x + it4->size.x;
								new_pos.y = new_pos2.y = it4->pos.y;
							}
							else
							{
								new_pos.x = new_pos2.x = it4->pos.x;
								new_pos.y = it4->pos.y - it4->size.y;
								new_pos2.y = it4->pos.y + it4->size.y;
							}
							new_dist[0] = Vec2::Distance(light_pos, new_pos) + Vec2::Distance(new_pos, obj_pos);
							new_dist[1] = Vec2::Distance(light_pos, new_pos2) + Vec2::Distance(new_pos2, obj_pos);
							if(new_dist[1] < new_dist[0])
								new_pos = new_pos2;

							// recalculate distance
							range_sum += Vec2::Distance(light_pos, new_pos);
							dist = range_sum + Distance(x, z, new_pos.x, new_pos.y);
							if(dist >= range[2])
								goto next_light;
							light_pos = new_pos;
						}
					}

					ok = true;
				}
			}
			else
			{
				const Mesh& mesh = node->GetMesh();
				light_pos = Vec2(it3->pos.x, it3->pos.z);
				const Vec2 sub_size = mesh.splits[sub].box.SizeXZ();
				dist = DistanceRectangleToPoint(obj_pos, sub_size, light_pos);
				if(IsZero(dist))
					ok = true;
				else if(dist < it3->range + radius && dist < range[2])
				{
					float range_sum = 0.f;

					// are there any masks between object and light?
					for(vector<LightMask>::iterator it4 = area.masks.begin(), end4 = area.masks.end(); it4 != end4; ++it4)
					{
						if(LineToRectangleSize(obj_pos, light_pos, it4->pos, it4->size))
						{
							// move light to one side of mask
							Vec2 new_pos, new_pos2;
							float new_dist[2];
							if(it4->size.x > it4->size.y)
							{
								new_pos.x = it4->pos.x - it4->size.x;
								new_pos2.x = it4->pos.x + it4->size.x;
								new_pos.y = new_pos2.y = it4->pos.y;
							}
							else
							{
								new_pos.x = new_pos2.x = it4->pos.x;
								new_pos.y = it4->pos.y - it4->size.y;
								new_pos2.y = it4->pos.y + it4->size.y;
							}
							new_dist[0] = Vec2::Distance(light_pos, new_pos) + DistanceRectangleToPoint(obj_pos, sub_size, new_pos);
							new_dist[1] = Vec2::Distance(light_pos, new_pos2) + DistanceRectangleToPoint(obj_pos, sub_size, new_pos2);
							if(new_dist[1] < new_dist[0])
								new_pos = new_pos2;

							// recalculate distance
							range_sum += Vec2::Distance(light_pos, new_pos);
							dist = range_sum + DistanceRectangleToPoint(obj_pos, sub_size, new_pos);
							if(dist >= range[2])
								goto next_light;
							light_pos = new_pos;
						}
					}

					ok = true;
				}
			}

			if(ok)
			{
				float light_range = it3->range - Vec2::Distance(light_pos, Vec2(it3->pos.x, it3->pos.z));
				if(light_range > 0)
				{
					if(dist < range[1])
					{
						if(dist < range[0])
						{
							// wstaw jako 0, 0 i 1 przesuñ
							range[2] = range[1];
							range[1] = range[0];
							range[0] = dist;
							light[2] = light[1];
							light[1] = light[0];
							light[0] = &*it3;
							lights_pos[2] = lights_pos[1];
							lights_pos[1] = lights_pos[0];
							lights_pos[0] = Vec3(light_pos.x, it3->pos.y, light_pos.y);
							lights_range[2] = lights_range[1];
							lights_range[1] = lights_range[0];
							lights_range[0] = light_range;
						}
						else
						{
							// wstaw jako 1, 1 przesuñ na 2
							range[2] = range[1];
							range[1] = dist;
							light[2] = light[1];
							light[1] = &*it3;
							lights_pos[2] = lights_pos[1];
							lights_pos[1] = Vec3(light_pos.x, it3->pos.y, light_pos.y);
							lights_range[2] = lights_range[1];
							lights_range[1] = light_range;
						}
					}
					else
					{
						// wstaw jako 2
						range[2] = dist;
						light[2] = &*it3;
						lights_pos[2] = Vec3(light_pos.x, it3->pos.y, light_pos.y);
						lights_range[2] = light_range;
					}
				}
			}

		next_light:;
		}

		if(light[0])
		{
			Lights& lights = Add1(draw_batch.lights);

			for(int i = 0; i < 3; ++i)
			{
				if(!light[i])
				{
					lights.ld[i].range = 1.f;
					lights.ld[i].pos = Vec3(0, -1000, 0);
					lights.ld[i].color = Vec3(0, 0, 0);
				}
				else
				{
					lights.ld[i].pos = lights_pos[i];
					lights.ld[i].range = lights_range[i];
					lights.ld[i].color = light[i]->t_color;
				}
			}

			return draw_batch.lights.size() - 1;
		}
		else
			return 0;
	}
}

//=================================================================================================
void Game::DrawScene(bool outside)
{
	PROFILER_BLOCK("DrawScene");

	// niebo
	if(outside && IsSet(draw_flags, DF_SKYBOX))
		DrawSkybox();

	// teren
	if(!draw_batch.terrain_parts.empty())
	{
		PROFILER_BLOCK("DrawTerrain");
		DrawTerrain(draw_batch.terrain_parts);
	}

	// podziemia
	if(!draw_batch.dungeon_parts.empty())
	{
		PROFILER_BLOCK("DrawDugneon");
		DrawDungeon(draw_batch.dungeon_parts, draw_batch.lights, draw_batch.matrices);
	}

	// modele
	if(!draw_batch.nodes.empty())
	{
		PROFILER_BLOCK("DrawSceneNodes");
		DrawSceneNodes(draw_batch.nodes, draw_batch.lights, outside);
	}

	// trawa
	DrawGrass();

	// debug nodes
	if(!draw_batch.debug_nodes.empty())
		DrawDebugNodes(draw_batch.debug_nodes);

	// krew
	if(!draw_batch.bloods.empty())
		DrawBloods(outside, draw_batch.bloods, draw_batch.lights);

	// billboardy
	if(!draw_batch.billboards.empty())
		DrawBillboards(draw_batch.billboards);

	// eksplozje
	if(!draw_batch.explos.empty())
		DrawExplosions(draw_batch.explos);

	// trail particles
	if(draw_batch.tpes)
		DrawTrailParticles(*draw_batch.tpes);

	// cz¹steczki
	if(!draw_batch.pes.empty())
		DrawParticles(draw_batch.pes);

	// electros
	if(draw_batch.electros)
		DrawLightings(*draw_batch.electros);

	// stun effects
	if(!draw_batch.stuns.empty())
		DrawStunEffects(draw_batch.stuns);

	// portale
	if(!draw_batch.portals.empty())
		DrawPortals(draw_batch.portals);

	// obszary
	if(!draw_batch.areas.empty() || !draw_batch.areas2.empty())
		DrawAreas(draw_batch.areas, draw_batch.area_range, draw_batch.areas2);
}

//=================================================================================================
// nie zoptymalizowane, póki co wyœwietla jeden obiekt (lub kilka ale dobrze posortowanych w przypadku postaci z przedmiotami)
void Game::DrawGlowingNodes(bool use_postfx)
{
	PROFILER_BLOCK("DrawGlowingNodes");

	IDirect3DDevice9* device = render->GetDevice();

	// ustaw flagi renderowania
	render->SetAlphaBlend(false);
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(true);
	V(device->SetRenderState(D3DRS_STENCILENABLE, TRUE));
	V(device->SetRenderState(D3DRS_STENCILREF, 1));
	V(device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE));
	V(device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS));

	// ustaw render target
	SURFACE render_surface;
	if(!render->IsMultisamplingEnabled())
		V(tPostEffect[0]->GetSurfaceLevel(0, &render_surface));
	else
		render_surface = sPostEffect[0];
	V(device->SetRenderTarget(0, render_surface));
	V(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0));
	V(device->BeginScene());

	// renderuj wszystkie obiekty
	int prev_mode = -1;
	Vec4 glow_color;
	Mesh* mesh;
	uint passes;

	for(vector<GlowNode>::iterator it = draw_batch.glow_nodes.begin(), end = draw_batch.glow_nodes.end(); it != end; ++it)
	{
		GlowNode& glow = *it;
		render->SetAlphaTest(glow.alpha);

		// animowany czy nie?
		if(IsSet(glow.node->flags, SceneNode::F_ANIMATED))
		{
			if(prev_mode != 1)
			{
				if(prev_mode != -1)
				{
					V(eGlow->EndPass());
					V(eGlow->End());
				}
				prev_mode = 1;
				V(eGlow->SetTechnique(techGlowAni));
				V(eGlow->Begin(&passes, 0));
				V(eGlow->BeginPass(0));
			}
			if(!glow.node->parent_mesh_inst)
			{
				vector<Matrix>& mat_bones = glow.node->mesh_inst->mat_bones;
				V(eGlow->SetMatrixArray(hGlowBones, (D3DXMATRIX*)&mat_bones[0], mat_bones.size()));
				mesh = glow.node->mesh_inst->mesh;
			}
			else
			{
				vector<Matrix>& mat_bones = glow.node->parent_mesh_inst->mat_bones;
				V(eGlow->SetMatrixArray(hGlowBones, (D3DXMATRIX*)&mat_bones[0], mat_bones.size()));
				mesh = glow.node->mesh;
			}
		}
		else
		{
			if(prev_mode != 0)
			{
				if(prev_mode != -1)
				{
					V(eGlow->EndPass());
					V(eGlow->End());
				}
				prev_mode = 0;
				V(eGlow->SetTechnique(techGlowMesh));
				V(eGlow->Begin(&passes, 0));
				V(eGlow->BeginPass(0));
			}
			mesh = glow.node->mesh;
		}

		// wybierz kolor
		if(glow.type == GlowNode::Unit)
		{
			Unit& unit = *static_cast<Unit*>(glow.ptr);
			if(pc->unit->IsEnemy(unit))
				glow_color = Vec4(1, 0, 0, 1);
			else if(pc->unit->IsFriend(unit))
				glow_color = Vec4(0, 1, 0, 1);
			else
				glow_color = Vec4(1, 1, 0, 1);
		}
		else
			glow_color = Vec4(1, 1, 1, 1);

		// ustawienia shadera
		Matrix m1 = glow.node->mat * game_level->camera.matViewProj;
		V(eGlow->SetMatrix(hGlowCombined, (D3DXMATRIX*)&m1));
		V(eGlow->SetVector(hGlowColor, (D3DXVECTOR4*)&glow_color));
		V(eGlow->CommitChanges());

		// ustawienia modelu
		V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh->vertex_decl)));
		V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
		V(device->SetIndices(mesh->ib));

		// render mesh
		if(glow.alpha)
		{
			// for glow need to set texture per submesh
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(i == 0 || glow.type != GlowNode::Door)
				{
					V(eGlow->SetTexture(hGlowTex, mesh->subs[i].tex->tex));
					V(eGlow->CommitChanges());
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
				}
			}
		}
		else
		{
			V(eGlow->SetTexture(hGlowTex, game_res->tBlack->tex));
			V(eGlow->CommitChanges());
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(i == 0 || glow.type != GlowNode::Door)
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
			}
		}
	}

	V(eGlow->EndPass());
	V(eGlow->End());
	V(device->EndScene());

	V(device->SetRenderState(D3DRS_ZENABLE, FALSE));
	V(device->SetRenderState(D3DRS_STENCILENABLE, FALSE));

	//======================================================================
	// w teksturze s¹ teraz wyrenderowane obiekty z kolorem glow
	// trzeba rozmyæ teksturê, napierw po X
	TEX tex;
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		V(tPostEffect[1]->GetSurfaceLevel(0, &render_surface));
		tex = tPostEffect[0];
	}
	else
	{
		SURFACE tex_surface;
		V(tPostEffect[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = tPostEffect[0];
		render_surface = sPostEffect[1];
	}
	V(device->SetRenderTarget(0, render_surface));
	V(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0));

	// ustawienia shadera
	D3DXHANDLE tech = ePostFx->GetTechniqueByName("BlurX");
	V(ePostFx->SetTechnique(tech));
	V(ePostFx->SetTexture(hPostTex, tex));
	// chcê ¿eby rozmiar efektu by³ % taki sam w ka¿dej rozdzielczoœci (ju¿ tak nie jest)
	const float base_range = 2.5f;
	const float range_x = (base_range / 1024.f);// *(wnd_size.x/1024.f);
	const float range_y = (base_range / 768.f);// *(wnd_size.x/768.f);
	V(ePostFx->SetVector(hPostSkill, (D3DXVECTOR4*)&Vec4(range_x, range_y, 0, 0)));
	V(ePostFx->SetFloat(hPostPower, 1));

	// ustawienia modelu
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_TEX)));
	V(device->SetStreamSource(0, vbFullscreen, 0, sizeof(VTex)));

	// renderowanie
	V(device->BeginScene());
	V(ePostFx->Begin(&passes, 0));
	V(ePostFx->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(ePostFx->EndPass());
	V(ePostFx->End());
	V(device->EndScene());

	//======================================================================
	// rozmywanie po Y
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		V(tPostEffect[0]->GetSurfaceLevel(0, &render_surface));
		tex = tPostEffect[1];
	}
	else
	{
		SURFACE tex_surface;
		V(tPostEffect[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = tPostEffect[0];
		render_surface = sPostEffect[0];
	}
	V(device->SetRenderTarget(0, render_surface));

	// ustawienia shadera
	tech = ePostFx->GetTechniqueByName("BlurY");
	V(ePostFx->SetTechnique(tech));
	V(ePostFx->SetTexture(hPostTex, tex));

	// renderowanie
	V(device->BeginScene());
	V(ePostFx->Begin(&passes, 0));
	V(ePostFx->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(ePostFx->EndPass());
	V(ePostFx->End());
	V(device->EndScene());

	//======================================================================
	// Renderowanie tekstury z glow na ekran gry
	// ustaw normalny render target
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		tex = tPostEffect[1];
	}
	else
	{
		SURFACE tex_surface;
		V(tPostEffect[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = tPostEffect[0];
	}
	if(!use_postfx)
	{
		V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &render_surface));
		V(device->SetRenderTarget(0, render_surface));
		render_surface->Release();
	}
	else
	{
		if(!render->IsMultisamplingEnabled())
		{
			V(tPostEffect[2]->GetSurfaceLevel(0, &render_surface));
			render_surface->Release();
		}
		else
			render_surface = sPostEffect[2];
		V(device->SetRenderTarget(0, render_surface));
	}

	// ustaw potrzebny render state
	V(device->SetRenderState(D3DRS_STENCILENABLE, TRUE));
	V(device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP));
	V(device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL));
	V(device->SetRenderState(D3DRS_STENCILREF, 0));
	render->SetAlphaBlend(true);
	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

	// ustawienia shadera
	tech = ePostFx->GetTechniqueByName("Empty");
	V(ePostFx->SetTexture(hPostTex, tex));

	// renderowanie
	V(device->BeginScene());
	V(ePostFx->Begin(&passes, 0));
	V(ePostFx->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(ePostFx->EndPass());
	V(ePostFx->End());
	V(device->EndScene());

	// przywróæ ustawienia
	V(device->SetRenderState(D3DRS_ZENABLE, TRUE));
	V(device->SetRenderState(D3DRS_STENCILENABLE, FALSE));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void Game::DrawSkybox()
{
	IDirect3DDevice9* device = render->GetDevice();
	Mesh& mesh = *game_res->aSkybox;

	render->SetAlphaTest(false);
	render->SetAlphaBlend(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(true);

	uint passes;
	Matrix m1 = Matrix::Translation(game_level->camera.center) * game_level->camera.matViewProj;

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	V(eSkybox->SetTechnique(techSkybox));
	V(eSkybox->SetMatrix(hSkyboxCombined, (D3DXMATRIX*)&m1));
	V(eSkybox->Begin(&passes, 0));
	V(eSkybox->BeginPass(0));

	for(vector<Mesh::Submesh>::iterator it = mesh.subs.begin(), end = mesh.subs.end(); it != end; ++it)
	{
		V(eSkybox->SetTexture(hSkyboxTex, it->tex->tex));
		V(eSkybox->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, it->min_ind, it->n_ind, it->first * 3, it->tris));
	}

	V(eSkybox->EndPass());
	V(eSkybox->End());
}

//=================================================================================================
void Game::DrawTerrain(const vector<uint>& parts)
{
	terrain_shader->SetCamera(game_level->camera);
	terrain_shader->SetFog(game_level->GetFogColor(), game_level->GetFogParams());
	terrain_shader->SetLight(game_level->GetLightDir(), game_level->GetLightColor(), game_level->GetAmbientColor());
	terrain_shader->Draw(game_level->terrain, parts);
}

//=================================================================================================
void Game::DrawDungeon(const vector<DungeonPart>& parts, const vector<Lights>& lights, const vector<NodeMatrix>& matrices)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaBlend(false);
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(false);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_TANGENT)));
	V(device->SetStreamSource(0, vbDungeon, 0, sizeof(VTangent)));
	V(device->SetIndices(ibDungeon));

	int last_mode = -1;
	ID3DXEffect* e = nullptr;
	bool first = true;
	TexOverride* last_override = nullptr;
	uint passes;

	for(vector<DungeonPart>::const_iterator it = parts.begin(), end = parts.end(); it != end; ++it)
	{
		const DungeonPart& dp = *it;
		int mode = dp.tex_o->GetIndex();

		// change shader
		if(mode != last_mode)
		{
			last_mode = mode;
			if(!first)
			{
				V(e->EndPass());
				V(e->End());
			}
			e = super_shader->GetShader(super_shader->GetShaderId(false, true, game_level->cl_fog, cl_specularmap && dp.tex_o->specular != nullptr,
				cl_normalmap && dp.tex_o->normal != nullptr, game_level->cl_lighting, false));
			if(first)
			{
				first = false;
				V(e->SetVector(super_shader->hTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
				V(e->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&game_level->GetAmbientColor()));
				V(e->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&game_level->GetFogColor()));
				V(e->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&game_level->GetFogParams()));
				V(e->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&game_level->camera.center));
				V(e->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
				V(e->SetFloat(super_shader->hSpecularIntensity, 0.2f));
				V(e->SetFloat(super_shader->hSpecularHardness, 10));
			}
			V(e->Begin(&passes, 0));
			V(e->BeginPass(0));
		}

		// set textures
		if(last_override != dp.tex_o)
		{
			last_override = dp.tex_o;
			V(e->SetTexture(super_shader->hTexDiffuse, last_override->diffuse->tex));
			if(cl_normalmap && last_override->normal)
				V(e->SetTexture(super_shader->hTexNormal, last_override->normal->tex));
			if(cl_specularmap && last_override->specular)
				V(e->SetTexture(super_shader->hTexSpecular, last_override->specular->tex));
		}

		// set matrices
		const NodeMatrix& m = matrices[dp.matrix];
		V(e->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m.matCombined));
		V(e->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&m.matWorld));

		// set lights
		V(e->SetRawValue(super_shader->hLights, &lights[dp.lights].ld[0], 0, sizeof(LightData) * 3));

		// draw
		V(e->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 76, dp.start_index, dp.primitive_count));
	}

	V(e->EndPass());
	V(e->End());
}

//=================================================================================================
void Game::DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<Lights>& lights, bool outside)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaBlend(false);

	Vec4 fogColor = game_level->GetFogColor();
	Vec4 fogParams = game_level->GetFogParams();
	Vec4 lightDir = game_level->GetLightDir();
	Vec4 lightColor = game_level->GetLightColor();
	Vec4 ambientColor = game_level->GetAmbientColor();

	// setup effect
	ID3DXEffect* e = super_shader->GetEffect();
	V(e->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&ambientColor));
	V(e->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&fogColor));
	V(e->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&fogParams));
	V(e->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&game_level->camera.center));
	if(outside)
	{
		V(e->SetVector(super_shader->hLightDir, (D3DXVECTOR4*)&lightDir));
		V(e->SetVector(super_shader->hLightColor, (D3DXVECTOR4*)&lightColor));
	}

	// modele
	uint passes;
	int current_flags = -1;
	bool inside_begin = false;
	const Mesh* prev_mesh = nullptr;

	for(vector<SceneNode*>::const_iterator it = nodes.begin(), end = nodes.end(); it != end; ++it)
	{
		const SceneNode* node = *it;
		const Mesh& mesh = node->GetMesh();
		if(!mesh.IsLoaded())
		{
			ReportError(10, Format("Drawing not loaded mesh '%s'.", mesh.filename));
			res_mgr->Load(const_cast<Mesh*>(&mesh));
			break;
		}

		// pobierz nowy efekt jeœli trzeba
		if(node->flags != current_flags)
		{
			if(inside_begin)
			{
				e->EndPass();
				e->End();
				inside_begin = false;
			}
			current_flags = node->flags;
			e = super_shader->GetShader(super_shader->GetShaderId(
				IsSet(node->flags, SceneNode::F_ANIMATED),
				IsSet(node->flags, SceneNode::F_BINORMALS),
				game_level->cl_fog,
				cl_specularmap && IsSet(node->flags, SceneNode::F_SPECULAR_MAP),
				cl_normalmap && IsSet(node->flags, SceneNode::F_NORMAL_MAP),
				game_level->cl_lighting && !outside,
				game_level->cl_lighting && outside));
			D3DXHANDLE tech;
			V(e->FindNextValidTechnique(nullptr, &tech));
			V(e->SetTechnique(tech));

			render->SetNoZWrite(IsSet(current_flags, SceneNode::F_NO_ZWRITE));
			render->SetNoCulling(IsSet(current_flags, SceneNode::F_NO_CULLING));
			render->SetAlphaTest(IsSet(current_flags, SceneNode::F_ALPHA_TEST));
		}

		// ustaw parametry shadera
		Matrix m1;
		if(!node->billboard)
			m1 = node->mat * game_level->camera.matViewProj;
		else
			m1 = node->mat.Inverse() * game_level->camera.matViewProj;
		V(e->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m1));
		V(e->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&node->mat));
		V(e->SetVector(super_shader->hTint, (D3DXVECTOR4*)&node->tint));
		if(IsSet(node->flags, SceneNode::F_ANIMATED))
		{
			const MeshInstance& mesh_inst = node->GetMeshInstance();
			V(e->SetMatrixArray(super_shader->hMatBones, (D3DXMATRIX*)&mesh_inst.mat_bones[0], mesh_inst.mat_bones.size()));
		}

		// ustaw model
		if(prev_mesh != &mesh)
		{
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
			V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
			V(device->SetIndices(mesh.ib));
			prev_mesh = &mesh;
		}

		// œwiat³a
		if(!outside)
			V(e->SetRawValue(super_shader->hLights, &lights[node->lights].ld[0], 0, sizeof(LightData) * 3));

		// renderowanie
		if(!IsSet(node->subs, SPLIT_INDEX))
		{
			for(int i = 0; i < mesh.head.n_subs; ++i)
			{
				if(!IsSet(node->subs, 1 << i))
					continue;

				const Mesh::Submesh& sub = mesh.subs[i];

				// tekstura
				V(e->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(i, node->tex_override)));
				if(cl_normalmap && IsSet(current_flags, SceneNode::F_NORMAL_MAP))
					V(e->SetTexture(super_shader->hTexNormal, sub.tex_normal->tex));
				if(cl_specularmap && IsSet(current_flags, SceneNode::F_SPECULAR_MAP))
					V(e->SetTexture(super_shader->hTexSpecular, sub.tex_specular->tex));

				// ustawienia œwiat³a
				V(e->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
				V(e->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
				V(e->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

				if(!inside_begin)
				{
					V(e->Begin(&passes, 0));
					V(e->BeginPass(0));
					inside_begin = true;
				}
				else
					V(e->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}
		else
		{
			int index = (node->subs & ~SPLIT_INDEX);
			const Mesh::Submesh& sub = mesh.subs[index];

			// tekstura
			V(e->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(index, node->tex_override)));
			if(cl_normalmap && IsSet(current_flags, SceneNode::F_NORMAL_MAP))
				V(e->SetTexture(super_shader->hTexNormal, sub.tex_normal->tex));
			if(cl_specularmap && IsSet(current_flags, SceneNode::F_SPECULAR_MAP))
				V(e->SetTexture(super_shader->hTexSpecular, sub.tex_specular->tex));

			// ustawienia œwiat³a
			V(e->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
			V(e->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
			V(e->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

			if(!inside_begin)
			{
				V(e->Begin(&passes, 0));
				V(e->BeginPass(0));
				inside_begin = true;
			}
			else
				V(e->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	if(inside_begin)
	{
		V(e->EndPass());
		V(e->End());
	}
}

//=================================================================================================
void Game::DrawDebugNodes(const vector<DebugSceneNode*>& nodes)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(false);
	render->SetNoCulling(true);
	render->SetNoZWrite(false);

	V(device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
	V(device->SetRenderState(D3DRS_ZENABLE, FALSE));

	uint passes;
	V(eMesh->SetTechnique(techMeshSimple2));
	V(eMesh->Begin(&passes, 0));
	V(eMesh->BeginPass(0));

	static Mesh* meshes[DebugSceneNode::MaxType] = {
		game_res->aBox,
		game_res->aCylinder,
		game_res->aSphere,
		game_res->aCapsule
	};

	static Vec4 colors[DebugSceneNode::MaxGroup] = {
		Vec4(0,0,0,1),
		Vec4(1,1,1,1),
		Vec4(0,1,0,1),
		Vec4(153.f / 255,217.f / 255,164.f / 234,1.f),
		Vec4(163.f / 255,73.f / 255,164.f / 255,1.f)
	};

	for(vector<DebugSceneNode*>::const_iterator it = nodes.begin(), end = nodes.end(); it != end; ++it)
	{
		const DebugSceneNode& node = **it;

		V(eMesh->SetVector(hMeshTint, (D3DXVECTOR4*)&colors[node.group]));
		V(eMesh->SetMatrix(hMeshCombined, (D3DXMATRIX*)&node.mat));

		if(node.type == DebugSceneNode::TriMesh)
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)node.mesh_ptr;
			// currently only dungeon mesh is supported here
			assert(mesh == game_level->dungeon_shape_data);
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_POS)));
			V(eMesh->CommitChanges());

			V(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, game_level->dungeon_shape_pos.size(), game_level->dungeon_shape_index.size() / 3, game_level->dungeon_shape_index.data(),
				D3DFMT_INDEX32, game_level->dungeon_shape_pos.data(), sizeof(Vec3)));
		}
		else
		{
			Mesh* mesh = meshes[node.type];
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh->vertex_decl)));
			V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
			V(device->SetIndices(mesh->ib));
			V(eMesh->CommitChanges());

			for(int i = 0; i < mesh->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
		}
	}

	V(eMesh->EndPass());
	V(eMesh->End());

	V(device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));
	V(device->SetRenderState(D3DRS_ZENABLE, TRUE));
}

//=================================================================================================
void Game::DrawBloods(bool outside, const vector<Blood*>& bloods, const vector<Lights>& lights)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(false);
	render->SetNoZWrite(true);

	ID3DXEffect* e = super_shader->GetShader(
		super_shader->GetShaderId(false, false, game_level->cl_fog, false, false, !outside && game_level->cl_lighting, outside && game_level->cl_lighting));
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_DEFAULT)));
	V(e->SetVector(super_shader->hTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));

	uint passes;
	V(e->Begin(&passes, 0));
	V(e->BeginPass(0));

	for(vector<Blood*>::const_iterator it = draw_batch.bloods.begin(), end = draw_batch.bloods.end(); it != end; ++it)
	{
		const Blood& blood = **it;

		// set blood vertices
		for(int i = 0; i < 4; ++i)
			blood_v[i].normal = blood.normal;

		const float s = blood.size * blood.scale,
			r = blood.rot;

		if(blood.normal.Equal(Vec3(0, 1, 0)))
		{
			blood_v[0].pos.x = s * sin(r + 5.f / 4 * PI);
			blood_v[0].pos.z = s * cos(r + 5.f / 4 * PI);
			blood_v[1].pos.x = s * sin(r + 7.f / 4 * PI);
			blood_v[1].pos.z = s * cos(r + 7.f / 4 * PI);
			blood_v[2].pos.x = s * sin(r + 3.f / 4 * PI);
			blood_v[2].pos.z = s * cos(r + 3.f / 4 * PI);
			blood_v[3].pos.x = s * sin(r + 1.f / 4 * PI);
			blood_v[3].pos.z = s * cos(r + 1.f / 4 * PI);
		}
		else
		{
			const Vec3 front(sin(r), 0, cos(r)), right(sin(r + PI / 2), 0, cos(r + PI / 2));
			Vec3 v_x, v_z, v_lx, v_rx, v_lz, v_rz;
			v_x = blood.normal.Cross(front);
			v_z = blood.normal.Cross(right);
			if(v_x.x > 0.f)
			{
				v_rx = v_x * s;
				v_lx = -v_x * s;
			}
			else
			{
				v_rx = -v_x * s;
				v_lx = v_x * s;
			}
			if(v_z.z > 0.f)
			{
				v_rz = v_z * s;
				v_lz = -v_z * s;
			}
			else
			{
				v_rz = -v_z * s;
				v_lz = v_z * s;
			}

			blood_v[0].pos = v_lx + v_lz;
			blood_v[1].pos = v_lx + v_rz;
			blood_v[2].pos = v_rx + v_lz;
			blood_v[3].pos = v_rx + v_rz;
		}

		// setup shader
		Matrix m1 = Matrix::Translation(blood.pos);
		Matrix m2 = m1 * game_level->camera.matViewProj;
		V(e->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m2));
		V(e->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&m1));
		V(e->SetTexture(super_shader->hTexDiffuse, game_res->tBloodSplat[blood.type]->tex));

		// lights
		if(!outside)
			V(e->SetRawValue(super_shader->hLights, &lights[blood.lights].ld[0], 0, sizeof(LightData) * 3));

		// draw
		V(e->CommitChanges());
		V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blood_v, sizeof(VDefault)));
	}

	V(e->EndPass());
	V(e->End());
}

//=================================================================================================
void Game::DrawBillboards(const vector<Billboard>& billboards)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaBlend(true);
	render->SetAlphaTest(false);
	render->SetNoCulling(true);
	render->SetNoZWrite(false);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_PARTICLE)));

	uint passes;
	V(eParticle->SetTechnique(techParticle));
	V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eParticle->Begin(&passes, 0));
	V(eParticle->BeginPass(0));

	for(vector<Billboard>::const_iterator it = billboards.begin(), end = billboards.end(); it != end; ++it)
	{
		game_level->camera.matViewInv._41 = it->pos.x;
		game_level->camera.matViewInv._42 = it->pos.y;
		game_level->camera.matViewInv._43 = it->pos.z;
		Matrix m1 = Matrix::Scale(it->size) * game_level->camera.matViewInv;

		for(int i = 0; i < 4; ++i)
			billboard_v[i].pos = Vec3::Transform(billboard_ext[i], m1);

		V(eParticle->SetTexture(hParticleTex, it->tex));
		V(eParticle->CommitChanges());

		V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, billboard_v, sizeof(VParticle)));
	}

	V(eParticle->EndPass());
	V(eParticle->End());
}

//=================================================================================================
void Game::DrawExplosions(const vector<Explo*>& explos)
{
	IDirect3DDevice9* device = render->GetDevice();
	Mesh& mesh = *game_res->aSpellball;
	Mesh::Submesh& sub = mesh.subs[0];

	render->SetAlphaBlend(true);
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration((mesh.vertex_decl))));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	uint passes;
	V(eMesh->SetTechnique(techMeshExplo));
	V(eMesh->Begin(&passes, 0));
	V(eMesh->BeginPass(0));

	Vec4 tint(1, 1, 1, 1);
	TEX last_tex = nullptr;

	for(vector<Explo*>::const_iterator it = explos.begin(), end = explos.end(); it != end; ++it)
	{
		const Explo& e = **it;

		if(e.tex->tex != last_tex)
		{
			last_tex = e.tex->tex;
			V(eMesh->SetTexture(hMeshTex, last_tex));
		}

		Matrix m1 = Matrix::Scale(e.size) * Matrix::Translation(e.pos) * game_level->camera.matViewProj;
		tint.w = 1.f - e.size / e.sizemax;

		V(eMesh->SetMatrix(hMeshCombined, (D3DXMATRIX*)&m1));
		V(eMesh->SetVector(hMeshTint, (D3DXVECTOR4*)&tint));
		V(eMesh->CommitChanges());

		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
	}

	V(eMesh->EndPass());
	V(eMesh->End());
}

//=================================================================================================
void Game::DrawParticles(const vector<ParticleEmitter*>& pes)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_PARTICLE)));

	uint passes;
	V(eParticle->SetTechnique(techParticle));
	V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eParticle->Begin(&passes, 0));
	V(eParticle->BeginPass(0));

	for(vector<ParticleEmitter*>::const_iterator it = pes.begin(), end = pes.end(); it != end; ++it)
	{
		const ParticleEmitter& pe = **it;

		// stwórz vertex buffer na cz¹steczki jeœli nie ma wystarczaj¹co du¿ego
		if(!vbParticle || particle_count < pe.alive)
		{
			if(vbParticle)
				vbParticle->Release();
			V(device->CreateVertexBuffer(sizeof(VParticle)*pe.alive * 6, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbParticle, nullptr));
			particle_count = pe.alive;
		}
		V(device->SetStreamSource(0, vbParticle, 0, sizeof(VParticle)));

		// wype³nij vertex buffer
		VParticle* v;
		V(vbParticle->Lock(0, sizeof(VParticle)*pe.alive * 6, (void**)&v, D3DLOCK_DISCARD));
		int idx = 0;
		for(vector<Particle>::const_iterator it2 = pe.particles.begin(), end2 = pe.particles.end(); it2 != end2; ++it2)
		{
			const Particle& p = *it2;
			if(!p.exists)
				continue;

			game_level->camera.matViewInv._41 = p.pos.x;
			game_level->camera.matViewInv._42 = p.pos.y;
			game_level->camera.matViewInv._43 = p.pos.z;
			Matrix m1 = Matrix::Scale(pe.GetScale(p)) * game_level->camera.matViewInv;

			const Vec4 color(1.f, 1.f, 1.f, pe.GetAlpha(p));

			v[idx].pos = Vec3::Transform(Vec3(-1, -1, 0), m1);
			v[idx].tex = Vec2(0, 0);
			v[idx].color = color;

			v[idx + 1].pos = Vec3::Transform(Vec3(-1, 1, 0), m1);
			v[idx + 1].tex = Vec2(0, 1);
			v[idx + 1].color = color;

			v[idx + 2].pos = Vec3::Transform(Vec3(1, -1, 0), m1);
			v[idx + 2].tex = Vec2(1, 0);
			v[idx + 2].color = color;

			v[idx + 3] = v[idx + 1];

			v[idx + 4].pos = Vec3::Transform(Vec3(1, 1, 0), m1);
			v[idx + 4].tex = Vec2(1, 1);
			v[idx + 4].color = color;

			v[idx + 5] = v[idx + 2];

			idx += 6;
		}

		V(vbParticle->Unlock());

		switch(pe.mode)
		{
		case 0:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
			break;
		case 1:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
			break;
		case 2:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
			break;
		default:
			assert(0);
			break;
		}

		V(eParticle->SetTexture(hParticleTex, pe.tex->tex));
		V(eParticle->CommitChanges());

		V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, pe.alive * 2));
	}

	V(eParticle->EndPass());
	V(eParticle->End());

	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void Game::DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_COLOR)));

	uint passes;
	V(eParticle->SetTechnique(techTrail));
	V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eParticle->Begin(&passes, 0));
	V(eParticle->BeginPass(0));

	VColor v[4];

	for(vector<TrailParticleEmitter*>::const_iterator it = tpes.begin(), end = tpes.end(); it != end; ++it)
	{
		const TrailParticleEmitter& tp = **it;

		if(tp.alive < 2)
			continue;

		int id = tp.first;
		const TrailParticle* prev = &tp.parts[id];
		id = prev->next;

		if(id < 0 || id >= (int)tp.parts.size() || !tp.parts[id].exists)
		{
			ReportError(6, Format("Trail particle emitter error, id = %d!", id));
			const_cast<TrailParticleEmitter&>(tp).alive = false;
			continue;
		}

		while(id != -1)
		{
			const TrailParticle& p = tp.parts[id];

			v[0].pos = prev->pt1;
			v[1].pos = prev->pt2;
			v[2].pos = p.pt1; // !!! tu siê czasami crashuje, p jest z³ym adresem, wiêc pewnie id te¿ jest z³e
			v[3].pos = p.pt2;

			v[0].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - prev->t / tp.fade);
			v[1].color = v[0].color;
			v[2].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - p.t / tp.fade);
			v[3].color = v[2].color;

			V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VColor)));

			prev = &p;
			id = prev->next;
		}
	}

	V(eParticle->EndPass());
	V(eParticle->End());
}

//=================================================================================================
void Game::DrawLightings(const vector<Electro*>& electros)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_PARTICLE)));

	uint passes;
	V(eParticle->SetTechnique(techParticle));
	V(eParticle->SetTexture(hParticleTex, game_res->tLightingLine->tex));
	V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eParticle->Begin(&passes, 0));
	V(eParticle->BeginPass(0));

	const Vec4 color(0.2f, 0.2f, 1.f, 1.f);

	Matrix matPart;
	VParticle v[4] = {
		Vec3(-1,-1,0), Vec2(0,0), color,
		Vec3(-1, 1,0), Vec2(0,1), color,
		Vec3(1,-1,0), Vec2(1,0), color,
		Vec3(1, 1,0), Vec2(1,1), color
	};

	const Vec3 pos[2] = {
		Vec3(0,-1,0),
		Vec3(0, 1,0)
	};

	Vec3 prev[2], next[2];

	for(vector<Electro*>::const_iterator it = electros.begin(), end = electros.end(); it != end; ++it)
	{
		for(vector<Electro::Line>::iterator it2 = (*it)->lines.begin(), end2 = (*it)->lines.end(); it2 != end2; ++it2)
		{
			if(it2->t >= 0.5f)
				continue;

			// startowy punkt
			game_level->camera.matViewInv._41 = it2->pts.front().x;
			game_level->camera.matViewInv._42 = it2->pts.front().y;
			game_level->camera.matViewInv._43 = it2->pts.front().z;
			const Matrix m_scale = Matrix::Scale(0.1f);
			Matrix m1 = m_scale * game_level->camera.matViewInv;

			for(int i = 0; i < 2; ++i)
				prev[i] = Vec3::Transform(pos[i], m1);

			const int count = int(it2->pts.size());

			for(int j = 1; j < count; ++j)
			{
				// nastêpny punkt
				game_level->camera.matViewInv._41 = it2->pts[j].x;
				game_level->camera.matViewInv._42 = it2->pts[j].y;
				game_level->camera.matViewInv._43 = it2->pts[j].z;
				m1 = m_scale * game_level->camera.matViewInv;

				for(int i = 0; i < 2; ++i)
					next[i] = Vec3::Transform(pos[i], m1);

				// œrodek
				v[0].pos = prev[0];
				v[1].pos = prev[1];
				v[2].pos = next[0];
				v[3].pos = next[1];

				float a = float(count - min(count, (int)abs(j - count * (it2->t / 0.25f)))) / count;
				float b = min(0.5f, a * a);

				for(int i = 0; i < 4; ++i)
					v[i].color.w = b;

				// rysuj
				V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VParticle)));

				// dalej
				prev[0] = next[0];
				prev[1] = next[1];
			}
		}
	}

	V(eParticle->EndPass());
	V(eParticle->End());

	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void Game::DrawStunEffects(const vector<StunEffect>& stuns)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	const Mesh& mesh = *game_res->aStun;

	V(eMesh->SetTechnique(techMeshDir));
	V(eMesh->SetVector(hMeshFogColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetVector(hMeshFogParam, (D3DXVECTOR4*)&Vec4(250.f, 500.f, 250.f, 0)));
	V(eMesh->SetVector(hMeshAmbientColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetVector(hMeshTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetVector(hMeshLightDir, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetVector(hMeshLightColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

	uint passes;
	V(eMesh->Begin(&passes, 0));
	V(eMesh->BeginPass(0));

	Matrix matWorld;
	for(auto& stun : stuns)
	{
		matWorld = Matrix::RotationY(stun.time * 3) * Matrix::Translation(stun.pos);
		V(eMesh->SetMatrix(hMeshCombined, (D3DXMATRIX*)&(matWorld * game_level->camera.matViewProj)));
		V(eMesh->SetMatrix(hMeshWorld, (D3DXMATRIX*)&matWorld));

		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			const Mesh::Submesh& sub = mesh.subs[i];
			V(eMesh->SetTexture(hMeshTex, sub.tex->tex));
			V(eMesh->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	V(eMesh->EndPass());
	V(eMesh->End());

	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void Game::DrawPortals(const vector<Portal*>& portals)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(false);

	uint passes;
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_PARTICLE)));
	V(eParticle->SetTechnique(techParticle));
	V(eParticle->SetTexture(hParticleTex, game_res->tPortal->tex));
	V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eParticle->Begin(&passes, 0));
	V(eParticle->BeginPass(0));

	for(vector<Portal*>::const_iterator it = portals.begin(), end = portals.end(); it != end; ++it)
	{
		const Portal& portal = **it;
		Matrix m1 = Matrix::Rotation(0, portal.rot, -portal_anim * PI * 2)
			* Matrix::Translation(portal.pos + Vec3(0, 0.67f + 0.305f, 0))
			* game_level->camera.matViewProj;
		V(eParticle->SetMatrix(hParticleCombined, (D3DXMATRIX*)&m1));
		V(eParticle->CommitChanges());
		V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, portal_v, sizeof(VParticle)));
	}

	V(eParticle->EndPass());
	V(eParticle->End());
}

//=================================================================================================
void Game::DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_POS)));

	V(eArea->SetTechnique(techArea));
	V(eArea->SetMatrix(hAreaCombined, (D3DXMATRIX*)&game_level->camera.matViewProj));
	V(eArea->SetVector(hAreaColor, (D3DXVECTOR4*)&Vec4(0, 1, 0, 0.5f)));
	Vec4 playerPos(pc->unit->pos, 1.f);
	playerPos.y += 0.75f;
	V(eArea->SetVector(hAreaPlayerPos, (D3DXVECTOR4*)&playerPos));
	V(eArea->SetFloat(hAreaRange, range));
	uint passes;
	V(eArea->Begin(&passes, 0));
	V(eArea->BeginPass(0));

	for(const Area& area : areas)
		V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (const void*)&area.v[0], sizeof(Vec3)));

	V(eArea->EndPass());
	V(eArea->End());

	if(!areas2.empty())
	{
		V(eArea->Begin(&passes, 0));
		V(eArea->BeginPass(0));
		V(eArea->SetFloat(hAreaRange, 100.f));

		static const Vec4 colors[3] = {
			Vec4(1, 0, 0, 0.5f),
			Vec4(1, 1, 0, 0.5f),
			Vec4(0, 0.58f, 1.f, 0.5f)
		};

		for(auto* area2 : areas2)
		{
			V(eArea->SetVector(hAreaColor, (D3DXVECTOR4*)&colors[area2->ok]));
			V(eArea->CommitChanges());
			V(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, area2->points.size(), area2->faces.size() / 3, area2->faces.data(), D3DFMT_INDEX16,
				area2->points.data(), sizeof(Vec3)));
		}

		V(eArea->EndPass());
		V(eArea->End());
	}
}

//=================================================================================================
void Game::UvModChanged()
{
	game_level->terrain->uv_mod = uv_mod;
	game_level->terrain->RebuildUv();
}
