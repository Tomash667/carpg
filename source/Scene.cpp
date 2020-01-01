#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "City.h"
#include "InsideLocation.h"
#include "Ability.h"
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
#include "ParticleShader.h"
#include "GlowShader.h"
#include "PostfxShader.h"
#include "BasicShader.h"
#include "SkyboxShader.h"
#include "Pathfinding.h"
#include "DirectX.h"

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
void Game::InitScene()
{
	blood_v[0].tex = Vec2(0, 0);
	blood_v[1].tex = Vec2(0, 1);
	blood_v[2].tex = Vec2(1, 0);
	blood_v[3].tex = Vec2(1, 1);
	for(int i = 0; i < 4; ++i)
		blood_v[i].pos.y = 0.f;

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
	// ile wierzcho�k�w
	// 19*4, pod�oga, sufit, 4 �ciany, niski sufit, 4 kawa�ki niskiego sufitu, 4 �ciany w dziurze g�rnej, 4 �ciany w dziurze dolnej
	IDirect3DDevice9* device = render->GetDevice();

	uint size = sizeof(VTangent) * 19 * 4;
	V(device->CreateVertexBuffer(size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbDungeon, nullptr));

	VTangent* v;
	V(vbDungeon->Lock(0, size, (void**)&v, 0));

	// kraw�dzie musz� na siebie lekko zachodzi�, inaczej wida� dziury pomi�dzy kafelkami
	const float L = -0.001f; // pozycja lewej kraw�dzi
	const float R = 2.001f; // pozycja prawej kraw�dzi
	const float H = Room::HEIGHT; // wysoko�� sufitu
	const float HS = Room::HEIGHT_LOW; // wysoko�� niskiego sufitu
	const float Z = 0.f; // wysoko�� pod�ogi
	const float U = H + 0.001f; // wysoko�� �ciany
	const float D = Z - 0.001f; // poziom pod�ogi �ciany
	//const float DS = HS-0.001f; // pocz�tek wysoko�� niskiego sufitu
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

	// pod�oga
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

	// prz�d
	v[16] = VTangent(Vec3(L, D, R), Vec2(0, V0), NTB_MZ);
	v[17] = VTangent(Vec3(L, U, R), Vec2(0, 0), NTB_MZ);
	v[18] = VTangent(Vec3(R, D, R), Vec2(1, V0), NTB_MZ);
	v[19] = VTangent(Vec3(R, U, R), Vec2(1, 0), NTB_MZ);

	// ty�
	v[20] = VTangent(Vec3(R, D, L), Vec2(0, V0), NTB_PZ);
	v[21] = VTangent(Vec3(R, U, L), Vec2(0, 0), NTB_PZ);
	v[22] = VTangent(Vec3(L, D, L), Vec2(1, V0), NTB_PZ);
	v[23] = VTangent(Vec3(L, U, L), Vec2(1, 0), NTB_PZ);

	// niski sufit
	v[24] = VTangent(Vec3(L, HS, R), Vec2(0, 1), NTB_MY);
	v[25] = VTangent(Vec3(L, HS, L), Vec2(0, 0), NTB_MY);
	v[26] = VTangent(Vec3(R, HS, R), Vec2(1, 1), NTB_MY);
	v[27] = VTangent(Vec3(R, HS, L), Vec2(1, 0), NTB_MY);

	/* niskie �ciany nie s� u�ywane, uv nie zaktualizowane
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

	// niski sufit prz�d
	v[36] = VTangent(Vec3(L,DS,R), Vec2(0,1), NTB_MZ);
	v[37] = VTangent(Vec3(L,U,R), Vec2(0,0.5f), NTB_MZ);
	v[38] = VTangent(Vec3(R,DS,R), Vec2(1,1), NTB_MZ);
	v[39] = VTangent(Vec3(R,U,R), Vec2(1,0.5f), NTB_MZ);

	// niski sufit ty�
	v[40] = VTangent(Vec3(R,DS,L), Vec2(0,1), NTB_PZ);
	v[41] = VTangent(Vec3(R,U,L), Vec2(0,0.5f), NTB_PZ);
	v[42] = VTangent(Vec3(L,DS,L), Vec2(1,1), NTB_PZ);
	v[43] = VTangent(Vec3(L,U,L), Vec2(1,0.5f), NTB_PZ);
	*/

	// dziura g�ra lewa
	v[44] = VTangent(Vec3(R, H1D, R), Vec2(0, V0), NTB_MX);
	v[45] = VTangent(Vec3(R, H1U, R), Vec2(0, 0), NTB_MX);
	v[46] = VTangent(Vec3(R, H1D, L), Vec2(1, V0), NTB_MX);
	v[47] = VTangent(Vec3(R, H1U, L), Vec2(1, 0), NTB_MX);

	// dziura g�ra prawa
	v[48] = VTangent(Vec3(L, H1D, L), Vec2(0, V0), NTB_PX);
	v[49] = VTangent(Vec3(L, H1U, L), Vec2(0, 0), NTB_PX);
	v[50] = VTangent(Vec3(L, H1D, R), Vec2(1, V0), NTB_PX);
	v[51] = VTangent(Vec3(L, H1U, R), Vec2(1, 0), NTB_PX);

	// dziura g�ra prz�d
	v[52] = VTangent(Vec3(L, H1D, R), Vec2(0, V0), NTB_MZ);
	v[53] = VTangent(Vec3(L, H1U, R), Vec2(0, 0), NTB_MZ);
	v[54] = VTangent(Vec3(R, H1D, R), Vec2(1, V0), NTB_MZ);
	v[55] = VTangent(Vec3(R, H1U, R), Vec2(1, 0), NTB_MZ);

	// dziura g�ra ty�
	v[56] = VTangent(Vec3(R, H1D, L), Vec2(0, V0), NTB_PZ);
	v[57] = VTangent(Vec3(R, H1U, L), Vec2(0, 0), NTB_PZ);
	v[58] = VTangent(Vec3(L, H1D, L), Vec2(1, V0), NTB_PZ);
	v[59] = VTangent(Vec3(L, H1U, L), Vec2(1, 0), NTB_PZ);

	// dziura d� lewa
	v[60] = VTangent(Vec3(R, H2D, R), Vec2(0, V0), NTB_MX);
	v[61] = VTangent(Vec3(R, H2U, R), Vec2(0, 0), NTB_MX);
	v[62] = VTangent(Vec3(R, H2D, L), Vec2(1, V0), NTB_MX);
	v[63] = VTangent(Vec3(R, H2U, L), Vec2(1, 0), NTB_MX);

	// dziura d� prawa
	v[64] = VTangent(Vec3(L, H2D, L), Vec2(0, V0), NTB_PX);
	v[65] = VTangent(Vec3(L, H2U, L), Vec2(0, 0), NTB_PX);
	v[66] = VTangent(Vec3(L, H2D, R), Vec2(1, V0), NTB_PX);
	v[67] = VTangent(Vec3(L, H2U, R), Vec2(1, 0), NTB_PX);

	// dziura d� prz�d
	v[68] = VTangent(Vec3(L, H2D, R), Vec2(0, V0), NTB_MZ);
	v[69] = VTangent(Vec3(L, H2U, R), Vec2(0, 0), NTB_MZ);
	v[70] = VTangent(Vec3(R, H2D, R), Vec2(1, V0), NTB_MZ);
	v[71] = VTangent(Vec3(R, H2U, R), Vec2(1, 0), NTB_MZ);

	// dziura d� ty�
	v[72] = VTangent(Vec3(R, H2D, L), Vec2(0, V0), NTB_PZ);
	v[73] = VTangent(Vec3(R, H2U, L), Vec2(0, 0), NTB_PZ);
	v[74] = VTangent(Vec3(L, H2D, L), Vec2(1, V0), NTB_PZ);
	v[75] = VTangent(Vec3(L, H2U, L), Vec2(1, 0), NTB_PZ);

	V(vbDungeon->Unlock());

	// ile indeks�w ?
	// pod�oga: 6
	// sufit: 6
	// niski sufit: 6
	//----------
	// opcja �e jest jedna �ciania: 6*4  -\
	// opcja �e s� dwie �ciany: 12*6       \  razy 4
	// opcja �e s� trzy �ciany: 18*4       /
	// opcja �e s� wszystkie �ciany: 24  -/
	size = sizeof(word) * (6 * 3 + (6 * 4 + 12 * 6 + 18 * 4 + 24) * 4);

	// index buffer
	V(device->CreateIndexBuffer(size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibDungeon, nullptr));

	word* id;
	V(ibDungeon->Lock(0, size, (void**)&id, 0));

	// pod�oga
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

	// prz�d
	v[16].tex.y = V0;
	v[18].tex.y = V0;

	// ty�
	v[20].tex.y = V0;
	v[22].tex.y = V0;

	// dziura g�ra lewa
	v[44].tex.y = V0;
	v[46].tex.y = V0;

	// dziura g�ra prawa
	v[48].tex.y = V0;
	v[50].tex.y = V0;

	// dziura g�ra prz�d
	v[52].tex.y = V0;
	v[54].tex.y = V0;

	// dziura g�ra ty�
	v[56].tex.y = V0;
	v[58].tex.y = V0;

	// dziura d� lewa
	v[60].tex.y = V0;
	v[62].tex.y = V0;

	// dziura d� prawa
	v[64].tex.y = V0;
	v[66].tex.y = V0;

	// dziura d� prz�d
	v[68].tex.y = V0;
	v[70].tex.y = V0;

	// dziura d� ty�
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
	draw_batch.camera = &game_level->camera;
	draw_batch.use_normalmap = use_normalmap;
	draw_batch.use_specularmap = use_specularmap;
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
				SceneNode* node = SceneNode::Get();
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
					if(use_glow)
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
				draw_batch.Add(node);
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
				SceneNode* node = SceneNode::Get();
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
					if(use_glow)
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
				draw_batch.Add(node);
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
				SceneNode* node = SceneNode::Get();
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
					if(use_glow)
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
				draw_batch.Add(node);
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
				SceneNode* node = SceneNode::Get();
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
					if(use_glow)
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
				draw_batch.Add(node);
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
					SceneNode* node = SceneNode::Get();
					node->billboard = false;
					node->mat = Matrix::Rotation(bullet.rot) * Matrix::Translation(bullet.pos);
					node->mesh = bullet.mesh;
					node->flags = 0;
					node->tint = Vec4(1, 1, 1, 1);
					node->tex_override = nullptr;
					if(!outside)
						node->lights = GatherDrawBatchLights(area, node, bullet.pos.x, bullet.pos.z, bullet.mesh->head.radius);
					draw_batch.Add(node);
				}
			}
			else
			{
				if(frustum.SphereToFrustum(bullet.pos, bullet.tex_size))
				{
					Billboard& bb = Add1(draw_batch.billboards);
					bb.pos = it->pos;
					bb.size = it->tex_size;
					bb.tex = it->tex;
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
				SceneNode* node = SceneNode::Get();
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
				draw_batch.Add(node);
			}
			if(trap.base->type == TRAP_SPEAR && InRange(trap.state, 2, 4) && frustum.SphereToFrustum(trap.obj2.pos, trap.obj2.mesh->head.radius))
			{
				SceneNode* node = SceneNode::Get();
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
				draw_batch.Add(node);
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
			{
				SceneNode* node = SceneNode::Get();
				node->pos = explo.pos;
				node->mat = Matrix::Scale(explo.size) * Matrix::Translation(explo.pos);
				node->mesh = game_res->aSpellball;
				node->parent_mesh_inst = nullptr;
				node->flags = SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
				node->tex_override = &explo.ability->tex_explode;
				node->tint = Vec4(1, 1, 1, 1.f - explo.size / explo.sizemax);
				draw_batch.Add(node);
			}
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
					DebugSceneNode* debug_node = DebugSceneNode::Get();
					debug_node->mat = Matrix::Scale(pe.radius * 2) * Matrix::Translation(pe.pos) * game_level->camera.mat_view_proj;
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

	// portals
	if(IsSet(draw_flags, DF_PORTALS) && area.area_type != LevelArea::Type::Building)
	{
		Portal* portal = game_level->location->portal;
		while(portal)
		{
			if(game_level->location->outside || game_level->dungeon_level == portal->at_level)
			{
				SceneNode* node = SceneNode::Get();
				node->billboard = false;
				node->pos = portal->pos + Vec3(0, 0.67f + 0.305f, 0);
				node->mat = Matrix::Rotation(0, portal->rot, -portal_anim * PI * 2) * Matrix::Translation(node->pos);
				node->mesh = game_res->aPortal;
				node->flags = SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING;
				node->tex_override = nullptr;
				node->parent_mesh_inst = nullptr;
				node->tint = Vec4::One;
				draw_batch.Add(node);
			}
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
				DebugSceneNode* node = DebugSceneNode::Get();
				node->type = type;
				node->group = DebugSceneNode::Collider;
				node->mat = Matrix::Scale(scale) * Matrix::RotationY(rot) * Matrix::Translation(it->pt.x, 1.f, it->pt.y) * game_level->camera.mat_view_proj;
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
					DebugSceneNode* node = DebugSceneNode::Get();
					node->type = DebugSceneNode::Box;
					node->group = DebugSceneNode::Physic;
					node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_world * game_level->camera.mat_view_proj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case CAPSULE_SHAPE_PROXYTYPE:
				{
					const btCapsuleShape* capsule = (const btCapsuleShape*)shape;
					float r = capsule->getRadius();
					float h = capsule->getHalfHeight();
					DebugSceneNode* node = DebugSceneNode::Get();
					node->type = DebugSceneNode::Capsule;
					node->group = DebugSceneNode::Physic;
					node->mat = Matrix::Scale(r, h + r, r) * m_world * game_level->camera.mat_view_proj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case CYLINDER_SHAPE_PROXYTYPE:
				{
					const btCylinderShape* cylinder = (const btCylinderShape*)shape;
					DebugSceneNode* node = DebugSceneNode::Get();
					node->type = DebugSceneNode::Cylinder;
					node->group = DebugSceneNode::Physic;
					Vec3 v = ToVec3(cylinder->getHalfExtentsWithoutMargin());
					node->mat = Matrix::Scale(v.x, v.y / 2, v.z) * m_world * game_level->camera.mat_view_proj;
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
							DebugSceneNode* node = DebugSceneNode::Get();
							node->type = DebugSceneNode::Box;
							node->group = DebugSceneNode::Physic;
							Matrix m_child;
							compound->getChildTransform(i).getOpenGLMatrix(&m_child._11);
							node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_child * m_world * game_level->camera.mat_view_proj;
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
					DebugSceneNode* node = DebugSceneNode::Get();
					const btBvhTriangleMeshShape* trimesh = (const btBvhTriangleMeshShape*)shape;
					node->type = DebugSceneNode::TriMesh;
					node->group = DebugSceneNode::Physic;
					node->mat = m_world * game_level->camera.mat_view_proj;
					node->mesh_ptr = (void*)trimesh->getMeshInterface();
					draw_batch.debug_nodes.push_back(node);
				}
			default:
				break;
			}
		}
	}

	draw_batch.Process();
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
			SceneNode* node = SceneNode::Get();
			node->billboard = false;
			node->pos = u.GetHeadPoint();
			node->mat = Matrix::RotationY(effect->time * 3) * Matrix::Translation(node->pos);
			node->mesh = game_res->aStun;
			node->flags = SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING | SceneNode::F_NO_ZWRITE;
			node->tex_override = nullptr;
			node->parent_mesh_inst = nullptr;
			node->tint = Vec4::One;
			draw_batch.Add(node);
		}
	}

	// ustaw ko�ci
	u.mesh_inst->SetupBones();

	bool selected = (pc->data.before_player == BP_UNIT && pc->data.before_player_ptr.unit == &u)
		|| (game_state == GS_LEVEL && ((pc->data.ability_ready && pc->data.ability_ok && pc->data.ability_target == u)
			|| (pc->unit->action == A_CAST && pc->unit->act.cast.target == u)));

	// dodaj scene node
	SceneNode* node = SceneNode::Get();
	node->billboard = false;
	node->mat = Matrix::RotationY(u.rot) * Matrix::Translation(u.visual_pos);
	node->mesh_inst = u.mesh_inst;
	node->flags = SceneNode::F_ANIMATED;
	node->tex_override = u.data->GetTextureOverride();
	node->parent_mesh_inst = nullptr;
	node->tint = Vec4(1, 1, 1, 1);

	// ustawienia �wiat�a
	int lights = -1;
	if(!outside)
	{
		assert(u.area);
		lights = GatherDrawBatchLights(*u.area, node, u.pos.x, u.pos.z, u.GetSphereRadius());
	}
	node->lights = lights;
	if(selected)
	{
		if(use_glow)
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
	draw_batch.Add(node);
	if(u.HaveArmor() && u.GetArmor().armor_unit_type == ArmorUnitType::HUMAN && u.GetArmor().mesh)
		node->subs = Bit(1) | Bit(2);

	// pancerz
	if(u.HaveArmor() && u.GetArmor().mesh)
	{
		const Armor& armor = u.GetArmor();
		SceneNode* node2 = SceneNode::Get();
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
			if(use_glow)
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
		draw_batch.Add(node2);
	}

	// przedmiot w d�oni
	Mesh* right_hand_item = nullptr;
	int right_hand_item_flags = 0;
	bool w_dloni = false;

	switch(u.weapon_state)
	{
	case WeaponState::Hidden:
		break;
	case WeaponState::Taken:
		if(u.weapon_taken == W_BOW)
		{
			if(u.action == A_SHOOT)
			{
				if(u.animation_state != AS_SHOOT_SHOT)
					right_hand_item = game_res->aArrow;
			}
			else
				right_hand_item = game_res->aArrow;
		}
		else if(u.weapon_taken == W_ONE_HANDED)
			w_dloni = true;
		break;
	case WeaponState::Taking:
		if(u.animation_state == AS_TAKE_WEAPON_MOVED)
		{
			if(u.weapon_taken == W_BOW)
				right_hand_item = game_res->aArrow;
			else
				w_dloni = true;
		}
		break;
	case WeaponState::Hiding:
		if(u.animation_state == AS_TAKE_WEAPON_START)
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

	// bro�
	Mesh* mesh;
	if(u.HaveWeapon() && right_hand_item != (mesh = u.GetWeapon().mesh))
	{
		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(w_dloni ? NAMES::point_weapon : NAMES::point_hidden_weapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = mesh;
		node2->flags = 0;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(use_glow)
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
		draw_batch.Add(node2);

		// hitbox broni
		if(draw_hitbox && u.weapon_state == WeaponState::Taken && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = mesh->FindPoint("hit");
			assert(box && box->IsBox());

			DebugSceneNode* debug_node = DebugSceneNode::Get();
			debug_node->mat = box->mat * node2->mat * game_level->camera.mat_view_proj;
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

		SceneNode* node2 = SceneNode::Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = shield;
		node2->flags = 0;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(use_glow)
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
		draw_batch.Add(node2);

		// hitbox tarczy
		if(draw_hitbox && u.weapon_state == WeaponState::Taken && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = shield->FindPoint("hit");
			assert(box && box->IsBox());

			DebugSceneNode* debug_node = DebugSceneNode::Get();
			node->mat = box->mat * node2->mat * game_level->camera.mat_view_proj;
			debug_node->type = DebugSceneNode::Box;
			debug_node->group = DebugSceneNode::Hitbox;
			draw_batch.debug_nodes.push_back(debug_node);
		}
	}

	// jaki� przedmiot
	if(right_hand_item)
	{
		Mesh::Point* point = u.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->billboard = false;
		node2->mat = mat_scale * point->mat * u.mesh_inst->mat_bones[point->bone] * node->mat;
		node2->mesh = right_hand_item;
		node2->flags = right_hand_item_flags;
		node2->tex_override = nullptr;
		node2->tint = Vec4(1, 1, 1, 1);
		node2->lights = lights;
		if(selected)
		{
			if(use_glow)
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
		draw_batch.Add(node2);
	}

	// �uk
	if(u.HaveBow())
	{
		bool w_dloni;

		switch(u.weapon_state)
		{
		case WeaponState::Hiding:
			w_dloni = (u.weapon_hiding == W_BOW && u.animation_state == AS_TAKE_WEAPON_START);
			break;
		case WeaponState::Hidden:
			w_dloni = false;
			break;
		case WeaponState::Taking:
			w_dloni = (u.weapon_taken == W_BOW && u.animation_state == AS_TAKE_WEAPON_MOVED);
			break;
		case WeaponState::Taken:
			w_dloni = (u.weapon_taken == W_BOW);
			break;
		}

		SceneNode* node2 = SceneNode::Get();
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
			if(use_glow)
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
		draw_batch.Add(node2);
	}

	// w�osy/broda/brwi u ludzi
	if(u.data->type == UNIT_TYPE::HUMAN)
	{
		Human& h = *u.human_data;

		// brwi
		SceneNode* node2 = SceneNode::Get();
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
			if(use_glow)
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
		draw_batch.Add(node2);

		// w�osy
		if(h.hair != -1)
		{
			SceneNode* node3 = SceneNode::Get();
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
				if(use_glow)
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
			draw_batch.Add(node3);
		}

		// broda
		if(h.beard != -1)
		{
			SceneNode* node3 = SceneNode::Get();
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
				if(use_glow)
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
			draw_batch.Add(node3);
		}

		// w�sy
		if(h.mustache != -1 && (h.beard == -1 || !g_beard_and_mustache[h.beard]))
		{
			SceneNode* node3 = SceneNode::Get();
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
				if(use_glow)
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
			draw_batch.Add(node3);
		}
	}

	// pseudo hitbox postaci
	if(draw_unit_radius)
	{
		float h = u.GetUnitHeight() / 2;
		DebugSceneNode* debug_node = DebugSceneNode::Get();
		debug_node->mat = Matrix::Scale(u.GetUnitRadius(), h, u.GetUnitRadius()) * Matrix::Translation(u.GetColliderPos() + Vec3(0, h, 0)) * game_level->camera.mat_view_proj;
		debug_node->type = DebugSceneNode::Cylinder;
		debug_node->group = DebugSceneNode::UnitRadius;
		draw_batch.debug_nodes.push_back(debug_node);
	}
	if(draw_hitbox)
	{
		float h = u.GetUnitHeight() / 2;
		Box box;
		u.GetBox(box);
		DebugSceneNode* debug_node = DebugSceneNode::Get();
		debug_node->mat = Matrix::Scale(box.SizeX() / 2, h, box.SizeZ() / 2) * Matrix::Translation(u.pos + Vec3(0, h, 0)) * game_level->camera.mat_view_proj;
		debug_node->type = DebugSceneNode::Box;
		debug_node->group = DebugSceneNode::Hitbox;
		draw_batch.debug_nodes.push_back(debug_node);
	}
}

//=================================================================================================
void Game::AddObjectToDrawBatch(LevelArea& area, const Object& o, FrustumPlanes& frustum)
{
	SceneNode* node = SceneNode::Get();
	if(!o.IsBillboard())
	{
		node->billboard = false;
		node->mat = Matrix::Transform(o.pos, o.rot, o.scale);
	}
	else
	{
		node->billboard = true;
		node->mat = Matrix::CreateLookAt(o.pos, game_level->camera.from);
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
		draw_batch.Add(node);
	}
	else
	{
		const Mesh& mesh = node->GetMesh();
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_TANGENTS;

		// for simplicity original node in unused and freed at end
		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			Vec3 pos = Vec3::Transform(mesh.splits[i].pos, node->mat);
			const float radius = mesh.splits[i].radius * o.scale;
			if(frustum.SphereToFrustum(pos, radius))
			{
				SceneNode* node2 = SceneNode::Get();
				node2->billboard = false;
				node2->mesh_inst = node->mesh_inst;
				node2->mat = node->mat;
				node2->flags = node->flags;
				node2->parent_mesh_inst = nullptr;
				node2->tint = node->tint;
				node2->lights = node->lights;
				node2->tex_override = node->tex_override;
				if(area.area_type != LevelArea::Type::Outside)
					node2->lights = GatherDrawBatchLights(area, node2, pos.x, pos.z, radius, i);
				draw_batch.Add(node2, i);
			}
		}

		node->Free();
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

			// g�ra
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
			}

			// d�
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
	if(pc->data.ability_ready)
		PrepareAreaPath();
	else if(pc->unit->action == A_CAST && Any(pc->unit->act.cast.ability->type, Ability::Point, Ability::Ray))
		pc->unit->target_pos = pc->RaytestTarget(pc->unit->act.cast.ability->range);
	else if(pc->unit->weapon_state == WeaponState::Taken
		&& ((pc->unit->weapon_taken == W_BOW && Any(pc->unit->action, A_NONE, A_SHOOT))
			|| (pc->unit->weapon_taken == W_ONE_HANDED && IsSet(pc->unit->GetWeapon().flags, ITEM_WAND)
				&& Any(pc->unit->action, A_NONE, A_ATTACK, A_CAST, A_BLOCK, A_BASH))))
		pc->unit->target_pos = pc->RaytestTarget(50.f);

	if(draw_hitbox && pc->ShouldUseRaytest())
	{
		Vec3 pos;
		if(pc->data.ability_ready)
			pos = pc->data.ability_point;
		else
			pos = pc->unit->target_pos;
		DebugSceneNode* node = DebugSceneNode::Get();
		node->mat = Matrix::Scale(0.25f) * Matrix::Translation(pos) * game_level->camera.mat_view_proj;
		node->type = DebugSceneNode::Sphere;
		node->group = DebugSceneNode::ParticleRadius;
		draw_batch.debug_nodes.push_back(node);
	}
}

//=================================================================================================
void Game::PrepareAreaPath()
{
	PlayerController::CanUseAbilityResult result = pc->CanUseAbility(pc->data.ability_ready);
	if(result != PlayerController::CanUseAbilityResult::Yes)
	{
		if(!(result == PlayerController::CanUseAbilityResult::TakeWand && pc->unit->action == A_TAKE_WEAPON && pc->unit->weapon_taken == W_ONE_HANDED))
		{
			pc->data.ability_ready = nullptr;
			sound_mgr->PlaySound2d(game_res->sCancel);
		}
		return;
	}

	if(pc->unit->action == A_CAST && pc->unit->animation_state == AS_CAST_ANIMATION)
		return;

	Ability& ability = *pc->data.ability_ready;
	switch(ability.type)
	{
	case Ability::Charge:
		{
			Area2* area_ptr = Area2::Get();
			Area2& area = *area_ptr;
			area.ok = 2;
			draw_batch.areas2.push_back(area_ptr);

			const Vec3 from = pc->unit->GetPhysicsPos();
			const float h = 0.06f;
			const float rot = Clip(pc->unit->rot + PI + pc->data.ability_rot);
			const int steps = 10;

			// find max line
			float t;
			Vec3 dir(sin(rot) * ability.range, 0, cos(rot) * ability.range);
			bool ignore_units = IsSet(ability.flags, Ability::IgnoreUnits);
			game_level->LineTest(pc->unit->cobj->getCollisionShape(), from, dir, [this, ignore_units](btCollisionObject* obj, bool)
			{
				int flags = obj->getCollisionFlags();
				if(IsSet(flags, CG_TERRAIN))
					return LT_IGNORE;
				if(IsSet(flags, CG_UNIT) && (obj->getUserPointer() == pc->unit || ignore_units))
					return LT_IGNORE;
				return LT_COLLIDE;
			}, t);

			float len = ability.range * t;

			if(game_level->location->outside && pc->unit->area->area_type == LevelArea::Type::Outside)
			{
				// build line on terrain
				area.points.clear();
				area.faces.clear();

				Vec3 active_pos = pc->unit->pos;
				Vec3 step = dir * t / (float)steps;
				Vec3 unit_offset(pc->unit->pos.x, 0, pc->unit->pos.z);
				float len_step = len / steps;
				float active_step = 0;
				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < steps; ++i)
				{
					float current_h = game_level->terrain->GetH(active_pos) + h;
					area.points.push_back(Vec3::Transform(Vec3(-ability.width, current_h, active_step), mat) + unit_offset);
					area.points.push_back(Vec3::Transform(Vec3(+ability.width, current_h, active_step), mat) + unit_offset);

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
				area.points[0] = Vec3(-ability.width, h, 0);
				area.points[1] = Vec3(-ability.width, h, len);
				area.points[2] = Vec3(ability.width, h, 0);
				area.points[3] = Vec3(ability.width, h, len);

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

			pc->data.ability_ok = true;
		}
		break;

	case Ability::Summon:
		{
			Area2* area_ptr = Area2::Get();
			Area2& area = *area_ptr;
			area.ok = 2;
			draw_batch.areas2.push_back(area_ptr);

			const Vec3 from = pc->unit->GetPhysicsPos();
			const float cam_max = 4.63034153f;
			const float cam_min = 4.08159288f;
			const float radius = ability.width / 2;
			const float unit_r = pc->unit->GetUnitRadius();

			float range = (Clamp(game_level->camera.rot.y, cam_min, cam_max) - cam_min) / (cam_max - cam_min) * ability.range;
			if(range < radius + unit_r)
				range = radius + unit_r;
			const float min_t = ability.width / range / 2;
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
				pc->data.ability_ok = false;
			}
			else
			{
				pc->data.ability_ok = true;
			}

			// build circle
			PrepareAreaPathCircle(area, radius, t * range, rot);

			// build yellow circle
			if(t != 1.f)
			{
				Area2* y_area = Area2::Get();
				y_area->ok = 1;
				PrepareAreaPathCircle(*y_area, radius, range, rot);
				draw_batch.areas2.push_back(y_area);
			}

			// set ability
			if(pc->data.ability_ok)
				pc->data.ability_point = area.points[0];
		}
		break;

	case Ability::Target:
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
				const float range = ability.range + game_level->camera.dist;
				RaytestClosestUnitDeadOrAliveCallback clbk(pc->unit);
				Vec3 from = game_level->camera.from;
				Vec3 dir = (game_level->camera.to - from).Normalized();
				from += dir * game_level->camera.dist;
				Vec3 to = from + dir * range;
				phy_world->rayTest(ToVector3(from), ToVector3(to), clbk);
				target = clbk.hit;
			}

			if(target)
			{
				pc->data.ability_ok = true;
				pc->data.ability_point = target->pos;
				pc->data.ability_target = target;
			}
			else
			{
				pc->data.ability_ok = false;
				pc->data.ability_target = nullptr;
			}
		}
		break;

	case Ability::Ray:
	case Ability::Point:
		pc->data.ability_point = pc->RaytestTarget(pc->data.ability_ready->range);
		pc->data.ability_ok = true;
		pc->data.ability_target = nullptr;
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
		area.points[i + 1] = Vec3(sin(angle) * radius, h, cos(angle) * radius + range);
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
		area.points[i + 1] = pos + Vec3(sin(angle) * radius, h, cos(angle) * radius);
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

			// zbierz list� �wiate� o�wietlaj�ce ten pok�j
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

			// dla ka�dego pola
			for(int y = 0; y < room->size.y; ++y)
			{
				for(int x = 0; x < room->size.x; ++x)
				{
					// czy co� jest na tym polu
					Tile& tile = lvl.map[(x + room->pos.x) + (y + room->pos.y) * lvl.w];
					if(tile.room != room->index || tile.flags == 0 || tile.flags == Tile::F_REVEALED)
						continue;

					// ustaw �wiat�a
					range[0] = range[1] = range[2] = game_level->camera.draw_range;
					light[0] = light[1] = light[2] = nullptr;

					float dx = 2.f * (room->pos.x + x) + 1.f;
					float dz = 2.f * (room->pos.y + y) + 1.f;

					for(vector<Light*>::iterator it2 = lights.begin(), end2 = lights.end(); it2 != end2; ++it2)
					{
						dist = Distance(dx, dz, (*it2)->pos.x, (*it2)->pos.z);
						if(dist < 1.414213562373095f + (*it2)->range && dist < range[2])
						{
							if(dist < range[1])
							{
								if(dist < range[0])
								{
									// wstaw jako 0, 0 i 1 przesu�
									range[2] = range[1];
									range[1] = range[0];
									range[0] = dist;
									light[2] = light[1];
									light[1] = light[0];
									light[0] = *it2;
								}
								else
								{
									// wstaw jako 1, 1 przesu� na 2
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

					// kopiuj w�a�ciwo�ci �wiat�a
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
					m.matWorld = Matrix::Translation(2.f * (room->pos.x + x), 0, 2.f * (room->pos.y + y));
					m.matCombined = m.matWorld * game_level->camera.mat_view_proj;

					int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

					// pod�oga
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

					// �ciany
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

						// g�ra
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

						// d�
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

		// podziel na kawa�ki u�ywaj�c pseudo quad-tree i frustum culling
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

		// dla ka�dego pola
		for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			Tile& tile = lvl.map[it->x + it->y * lvl.w];
			if(tile.flags == 0 || tile.flags == Tile::F_REVEALED)
				continue;

			Box box(2.f * it->x, -4.f, 2.f * it->y, 2.f * (it->x + 1), 8.f, 2.f * (it->y + 1));
			if(!frustum.BoxToFrustum(box))
				continue;

			range[0] = range[1] = range[2] = game_level->camera.draw_range;
			light[0] = light[1] = light[2] = nullptr;

			float dx = 2.f * it->x + 1.f;
			float dz = 2.f * it->y + 1.f;

			for(vector<Light>::iterator it2 = lvl.lights.begin(), end2 = lvl.lights.end(); it2 != end2; ++it2)
			{
				dist = Distance(dx, dz, it2->pos.x, it2->pos.z);
				if(dist < 1.414213562373095f + it2->range && dist < range[2])
				{
					if(dist < range[1])
					{
						if(dist < range[0])
						{
							// wstaw jako 0, 0 i 1 przesu�
							range[2] = range[1];
							range[1] = range[0];
							range[0] = dist;
							light[2] = light[1];
							light[1] = light[0];
							light[0] = &*it2;
						}
						else
						{
							// wstaw jako 1, 1 przesu� na 2
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

			// kopiuj w�a�ciwo�ci �wiate�
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
			m.matWorld = Matrix::Translation(2.f * it->x, 0, 2.f * it->y);
			m.matCombined = m.matWorld * game_level->camera.mat_view_proj;

			int tex_id = (IsSet(tile.flags, Tile::F_SECOND_TEXTURE) ? 1 : 0);

			// pod�oga
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

			// �ciany
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

				// g�ra
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

				// d�
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
						// wstaw jako 0, 0 i 1 przesu�
						range[2] = range[1];
						range[1] = range[0];
						range[0] = dist;
						light[2] = light[1];
						light[1] = light[0];
						light[0] = &*it3;
					}
					else
					{
						// wstaw jako 1, 1 przesu� na 2
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
							// wstaw jako 0, 0 i 1 przesu�
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
							// wstaw jako 1, 1 przesu� na 2
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
		skybox_shader->Draw(*game_res->aSkybox, game_level->camera);

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
	if(pathfinding->IsDebugDraw())
	{
		basic_shader->Prepare(game_level->camera);
		pathfinding->Draw(basic_shader);
	}

	// krew
	if(!draw_batch.bloods.empty())
		DrawBloods(outside, draw_batch.bloods, draw_batch.lights);

	// particles
	if(!draw_batch.billboards.empty() || !draw_batch.pes.empty() || draw_batch.tpes)
	{
		particle_shader->Begin(game_level->camera);
		if(!draw_batch.billboards.empty())
			particle_shader->DrawBillboards(draw_batch.billboards);
		if(draw_batch.tpes)
			particle_shader->DrawTrailParticles(*draw_batch.tpes);
		if(!draw_batch.pes.empty())
			particle_shader->DrawParticles(draw_batch.pes);
		particle_shader->End();
	}

	// alpha nodes
	if(!draw_batch.alpha_nodes.empty())
		DrawAlphaSceneNodes(draw_batch.alpha_nodes, draw_batch.lights, outside);

	// obszary
	if(!draw_batch.areas.empty() || !draw_batch.areas2.empty())
		DrawAreas(draw_batch.areas, draw_batch.area_range, draw_batch.areas2);
}

//=================================================================================================
// nie zoptymalizowane, p�ki co wy�wietla jeden obiekt (lub kilka ale dobrze posortowanych w przypadku postaci z przedmiotami)
void Game::DrawGlowingNodes(const vector<GlowNode>& glow_nodes, bool use_postfx)
{
	PROFILER_BLOCK("DrawGlowingNodes");

	IDirect3DDevice9* device = render->GetDevice();
	ID3DXEffect* effect = glow_shader->effect;

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
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &render_surface));
	else
		render_surface = postfx_shader->surf[0];
	V(device->SetRenderTarget(0, render_surface));
	V(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0));
	V(device->BeginScene());

	// renderuj wszystkie obiekty
	int prev_mode = -1;
	Vec4 glow_color;
	Mesh* mesh;
	uint passes;

	for(const GlowNode& glow : glow_nodes)
	{
		render->SetAlphaTest(glow.alpha);

		// animowany czy nie?
		if(IsSet(glow.node->flags, SceneNode::F_ANIMATED))
		{
			if(prev_mode != 1)
			{
				if(prev_mode != -1)
				{
					V(effect->EndPass());
					V(effect->End());
				}
				prev_mode = 1;
				V(effect->SetTechnique(glow_shader->techGlowAni));
				V(effect->Begin(&passes, 0));
				V(effect->BeginPass(0));
			}
			if(!glow.node->parent_mesh_inst)
			{
				vector<Matrix>& mat_bones = glow.node->mesh_inst->mat_bones;
				V(effect->SetMatrixArray(glow_shader->hMatBones, (D3DXMATRIX*)&mat_bones[0], mat_bones.size()));
				mesh = glow.node->mesh_inst->mesh;
			}
			else
			{
				vector<Matrix>& mat_bones = glow.node->parent_mesh_inst->mat_bones;
				V(effect->SetMatrixArray(glow_shader->hMatBones, (D3DXMATRIX*)&mat_bones[0], mat_bones.size()));
				mesh = glow.node->mesh;
			}
		}
		else
		{
			if(prev_mode != 0)
			{
				if(prev_mode != -1)
				{
					V(effect->EndPass());
					V(effect->End());
				}
				prev_mode = 0;
				V(effect->SetTechnique(glow_shader->techGlowMesh));
				V(effect->Begin(&passes, 0));
				V(effect->BeginPass(0));
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
		Matrix m1 = glow.node->mat * game_level->camera.mat_view_proj;
		V(effect->SetMatrix(glow_shader->hMatCombined, (D3DXMATRIX*)&m1));
		V(effect->SetVector(glow_shader->hColor, (D3DXVECTOR4*)&glow_color));
		V(effect->CommitChanges());

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
					V(effect->SetTexture(glow_shader->hTex, mesh->subs[i].tex->tex));
					V(effect->CommitChanges());
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
				}
			}
		}
		else
		{
			V(effect->SetTexture(glow_shader->hTex, game_res->tBlack->tex));
			V(effect->CommitChanges());
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(i == 0 || glow.type != GlowNode::Door)
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
			}
		}
	}

	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	V(device->SetRenderState(D3DRS_ZENABLE, FALSE));
	V(device->SetRenderState(D3DRS_STENCILENABLE, FALSE));

	//======================================================================
	// w teksturze s� teraz wyrenderowane obiekty z kolorem glow
	// trzeba rozmy� tekstur�, napierw po X
	effect = postfx_shader->effect;

	TEX tex;
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		V(postfx_shader->tex[1]->GetSurfaceLevel(0, &render_surface));
		tex = postfx_shader->tex[0];
	}
	else
	{
		SURFACE tex_surface;
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = postfx_shader->tex[0];
		render_surface = postfx_shader->surf[1];
	}
	V(device->SetRenderTarget(0, render_surface));
	V(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0));

	// ustawienia shadera
	V(effect->SetTechnique(postfx_shader->techBlurX));
	V(effect->SetTexture(postfx_shader->hTex, tex));
	// chc� �eby rozmiar efektu by� % taki sam w ka�dej rozdzielczo�ci (ju� tak nie jest)
	const float base_range = 2.5f;
	const float range_x = (base_range / 1024.f);// *(wnd_size.x/1024.f);
	const float range_y = (base_range / 768.f);// *(wnd_size.x/768.f);
	V(effect->SetVector(postfx_shader->hSkill, (D3DXVECTOR4*)&Vec4(range_x, range_y, 0, 0)));
	V(effect->SetFloat(postfx_shader->hPower, 1));

	// ustawienia modelu
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_TEX)));
	V(device->SetStreamSource(0, postfx_shader->vbFullscreen, 0, sizeof(VTex)));

	// renderowanie
	V(device->BeginScene());
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	//======================================================================
	// rozmywanie po Y
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &render_surface));
		tex = postfx_shader->tex[1];
	}
	else
	{
		SURFACE tex_surface;
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = postfx_shader->tex[0];
		render_surface = postfx_shader->surf[0];
	}
	V(device->SetRenderTarget(0, render_surface));

	// ustawienia shadera
	V(effect->SetTechnique(postfx_shader->techBlurY));
	V(effect->SetTexture(postfx_shader->hTex, tex));

	// renderowanie
	V(device->BeginScene());
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	//======================================================================
	// Renderowanie tekstury z glow na ekran gry
	// ustaw normalny render target
	if(!render->IsMultisamplingEnabled())
	{
		render_surface->Release();
		tex = postfx_shader->tex[1];
	}
	else
	{
		SURFACE tex_surface;
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &tex_surface));
		V(device->StretchRect(render_surface, nullptr, tex_surface, nullptr, D3DTEXF_NONE));
		tex_surface->Release();
		tex = postfx_shader->tex[0];
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
			V(postfx_shader->tex[2]->GetSurfaceLevel(0, &render_surface));
			render_surface->Release();
		}
		else
			render_surface = postfx_shader->surf[2];
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
	V(effect->SetTexture(postfx_shader->hTex, tex));

	// renderowanie
	V(device->BeginScene());
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));
	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	// przywr�� ustawienia
	V(device->SetRenderState(D3DRS_ZENABLE, TRUE));
	V(device->SetRenderState(D3DRS_STENCILENABLE, FALSE));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
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
			e = super_shader->GetShader(super_shader->GetShaderId(false, true, use_fog, use_specularmap && dp.tex_o->specular != nullptr,
				use_normalmap && dp.tex_o->normal != nullptr, use_lighting, false));
			if(first)
			{
				first = false;
				V(e->SetVector(super_shader->hTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
				V(e->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&game_level->GetAmbientColor()));
				V(e->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&game_level->GetFogColor()));
				V(e->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&game_level->GetFogParams()));
				V(e->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&game_level->camera.from));
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
			if(use_normalmap && last_override->normal)
				V(e->SetTexture(super_shader->hTexNormal, last_override->normal->tex));
			if(use_specularmap && last_override->specular)
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
	ID3DXEffect* effect = super_shader->GetEffect();
	V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&ambientColor));
	V(effect->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&fogColor));
	V(effect->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&fogParams));
	V(effect->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&game_level->camera.from));
	if(outside)
	{
		V(effect->SetVector(super_shader->hLightDir, (D3DXVECTOR4*)&lightDir));
		V(effect->SetVector(super_shader->hLightColor, (D3DXVECTOR4*)&lightColor));
	}

	// for each group
	const Mesh* prev_mesh = nullptr;
	for(const SceneNodeGroup& group : draw_batch.node_groups)
	{
		const bool animated = IsSet(group.flags, SceneNode::F_ANIMATED);
		const bool normal_map = IsSet(group.flags, SceneNode::F_NORMAL_MAP);
		const bool specular_map = IsSet(group.flags, SceneNode::F_SPECULAR_MAP);
		const bool use_lighting = this->use_lighting && !IsSet(group.flags, SceneNode::F_NO_LIGHTING);

		effect = super_shader->GetShader(super_shader->GetShaderId(
			animated,
			IsSet(group.flags, SceneNode::F_TANGENTS),
			use_fog && use_lighting,
			specular_map,
			normal_map,
			use_lighting && !outside,
			use_lighting && outside));
		D3DXHANDLE tech;
		uint passes;
		V(effect->FindNextValidTechnique(nullptr, &tech));
		V(effect->SetTechnique(tech));
		V(effect->Begin(&passes, 0));
		V(effect->BeginPass(0));

		render->SetNoZWrite(IsSet(group.flags, SceneNode::F_NO_ZWRITE));
		render->SetNoCulling(IsSet(group.flags, SceneNode::F_NO_CULLING));
		render->SetAlphaTest(IsSet(group.flags, SceneNode::F_ALPHA_TEST));

		// for each node in group
		for(auto it = draw_batch.nodes.begin() + group.start, end = draw_batch.nodes.begin() + group.end + 1; it != end; ++it)
		{
			const SceneNode* node = *it;
			const Mesh& mesh = node->GetMesh();
			if(!mesh.IsLoaded())
			{
				ReportError(10, Format("Drawing not loaded mesh '%s'.", mesh.filename));
				res_mgr->Load(const_cast<Mesh*>(&mesh));
				break;
			}

			// ustaw parametry shadera
			Matrix m1;
			if(node->billboard)
				m1 = node->mat.Inverse() * game_level->camera.mat_view_proj;
			else
				m1 = node->mat * game_level->camera.mat_view_proj;
			V(effect->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m1));
			V(effect->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&node->mat));
			V(effect->SetVector(super_shader->hTint, (D3DXVECTOR4*)&node->tint));
			if(animated)
			{
				const MeshInstance& mesh_inst = node->GetMeshInstance();
				V(effect->SetMatrixArray(super_shader->hMatBones, (D3DXMATRIX*)&mesh_inst.mat_bones[0], mesh_inst.mat_bones.size()));
			}

			// ustaw model
			if(prev_mesh != &mesh)
			{
				V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
				V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
				V(device->SetIndices(mesh.ib));
				prev_mesh = &mesh;
			}

			// �wiat�a
			if(!outside && use_lighting)
				V(effect->SetRawValue(super_shader->hLights, &lights[node->lights].ld[0], 0, sizeof(LightData) * 3));

			// renderowanie
			if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
			{
				for(int i = 0; i < mesh.head.n_subs; ++i)
				{
					if(!IsSet(node->subs, 1 << i))
						continue;

					const Mesh::Submesh& sub = mesh.subs[i];

					// tekstura
					V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(i, node->tex_override)));
					if(normal_map)
					{
						TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
						V(effect->SetTexture(super_shader->hTexNormal, tex));
					}
					if(specular_map)
					{
						TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
						V(effect->SetTexture(super_shader->hTexSpecular, tex));
					}

					// ustawienia �wiat�a
					V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
					V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
					V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

					V(effect->CommitChanges());
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
				}
			}
			else
			{
				int index = (node->subs & ~SceneNode::SPLIT_INDEX);
				const Mesh::Submesh& sub = mesh.subs[index];

				// tekstura
				V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(index, node->tex_override)));
				if(normal_map)
				{
					TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
					V(effect->SetTexture(super_shader->hTexNormal, tex));
				}
				if(specular_map)
				{
					TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
					V(effect->SetTexture(super_shader->hTexSpecular, tex));
				}

				// ustawienia �wiat�a
				V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
				V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
				V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

				V(effect->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}

		V(effect->EndPass());
		V(effect->End());
	}
}

//=================================================================================================
void Game::DrawAlphaSceneNodes(const vector<SceneNode*>& nodes, const vector<Lights>& lights, bool outside)
{
	IDirect3DDevice9* device = render->GetDevice();

	render->SetAlphaBlend(true);
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

	Vec4 fogColor = game_level->GetFogColor();
	Vec4 fogParams = game_level->GetFogParams();
	Vec4 lightDir = game_level->GetLightDir();
	Vec4 lightColor = game_level->GetLightColor();
	Vec4 ambientColor = game_level->GetAmbientColor();

	// setup effect
	ID3DXEffect* effect = super_shader->GetEffect();
	V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&ambientColor));
	V(effect->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&fogColor));
	V(effect->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&fogParams));
	V(effect->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&game_level->camera.from));
	if(outside)
	{
		V(effect->SetVector(super_shader->hLightDir, (D3DXVECTOR4*)&lightDir));
		V(effect->SetVector(super_shader->hLightColor, (D3DXVECTOR4*)&lightColor));
	}

	// for each group
	const Mesh* prev_mesh = nullptr;
	uint last_id = -1;
	bool open = false;
	for(const SceneNode* node : nodes)
	{
		const bool animated = IsSet(node->flags, SceneNode::F_ANIMATED);
		const bool normal_map = IsSet(node->flags, SceneNode::F_NORMAL_MAP);
		const bool specular_map = IsSet(node->flags, SceneNode::F_SPECULAR_MAP);
		const bool use_lighting = this->use_lighting && !IsSet(node->flags, SceneNode::F_NO_LIGHTING);

		uint id = super_shader->GetShaderId(
			animated,
			IsSet(node->flags, SceneNode::F_TANGENTS),
			use_fog && use_lighting,
			specular_map,
			normal_map,
			use_lighting && !outside,
			use_lighting && outside);
		if(id != last_id)
		{
			if(open)
			{
				effect->EndPass();
				effect->End();
			}

			render->SetNoZWrite(IsSet(node->flags, SceneNode::F_NO_ZWRITE));
			render->SetNoCulling(IsSet(node->flags, SceneNode::F_NO_CULLING));
			render->SetAlphaTest(IsSet(node->flags, SceneNode::F_ALPHA_TEST));

			effect = super_shader->GetShader(id);
			D3DXHANDLE tech;
			uint passes;
			V(effect->FindNextValidTechnique(nullptr, &tech));
			V(effect->SetTechnique(tech));
			effect->Begin(&passes, 0);
			effect->BeginPass(0);

			open = true;
		}

		const Mesh& mesh = node->GetMesh();
		if(!mesh.IsLoaded())
		{
			ReportError(10, Format("Drawing not loaded mesh '%s'.", mesh.filename));
			res_mgr->Load(const_cast<Mesh*>(&mesh));
			break;
		}

		// ustaw parametry shadera
		Matrix m1;
		if(!node->billboard)
			m1 = node->mat * game_level->camera.mat_view_proj;
		else
			m1 = node->mat.Inverse() * game_level->camera.mat_view_proj;
		V(effect->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m1));
		V(effect->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&node->mat));
		V(effect->SetVector(super_shader->hTint, (D3DXVECTOR4*)&node->tint));
		if(animated)
		{
			const MeshInstance& mesh_inst = node->GetMeshInstance();
			V(effect->SetMatrixArray(super_shader->hMatBones, (D3DXMATRIX*)&mesh_inst.mat_bones[0], mesh_inst.mat_bones.size()));
		}

		// ustaw model
		if(prev_mesh != &mesh)
		{
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
			V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
			V(device->SetIndices(mesh.ib));
			prev_mesh = &mesh;
		}

		// �wiat�a
		if(use_lighting && !outside)
			V(effect->SetRawValue(super_shader->hLights, &lights[node->lights].ld[0], 0, sizeof(LightData) * 3));

		// renderowanie
		assert(!IsSet(node->subs, SceneNode::SPLIT_INDEX)); // yagni
		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			if(!IsSet(node->subs, 1 << i))
				continue;

			const Mesh::Submesh& sub = mesh.subs[i];

			// tekstura
			V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(i, node->tex_override)));
			if(normal_map)
			{
				TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
				V(effect->SetTexture(super_shader->hTexNormal, tex));
			}
			if(specular_map)
			{
				TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
				V(effect->SetTexture(super_shader->hTexSpecular, tex));
			}

			// ustawienia �wiat�a
			V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
			V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
			V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

			V(effect->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	if(open)
	{
		effect->EndPass();
		effect->End();
	}

	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void Game::DrawDebugNodes(const vector<DebugSceneNode*>& nodes)
{
	IDirect3DDevice9* device = render->GetDevice();
	ID3DXEffect* effect = basic_shader->effect;

	render->SetAlphaTest(false);
	render->SetAlphaBlend(false);
	render->SetNoCulling(true);
	render->SetNoZWrite(false);

	V(device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
	V(device->SetRenderState(D3DRS_ZENABLE, FALSE));

	uint passes;
	V(effect->SetTechnique(basic_shader->techSimple));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

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

		V(effect->SetVector(basic_shader->hColor, (D3DXVECTOR4*)&colors[node.group]));
		V(effect->SetMatrix(basic_shader->hMatCombined, (D3DXMATRIX*)&node.mat));

		if(node.type == DebugSceneNode::TriMesh)
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)node.mesh_ptr;
			// currently only dungeon mesh is supported here
			assert(mesh == game_level->dungeon_shape_data);
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_POS)));
			V(effect->CommitChanges());

			V(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, game_level->dungeon_shape_pos.size(), game_level->dungeon_shape_index.size() / 3, game_level->dungeon_shape_index.data(),
				D3DFMT_INDEX32, game_level->dungeon_shape_pos.data(), sizeof(Vec3)));
		}
		else
		{
			Mesh* mesh = meshes[node.type];
			V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh->vertex_decl)));
			V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
			V(device->SetIndices(mesh->ib));
			V(effect->CommitChanges());

			for(int i = 0; i < mesh->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
		}
	}

	V(effect->EndPass());
	V(effect->End());

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
		super_shader->GetShaderId(false, false, use_fog, false, false, !outside && use_lighting, outside && use_lighting));
	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_DEFAULT)));
	V(e->SetVector(super_shader->hTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));

	uint passes;
	V(e->Begin(&passes, 0));
	V(e->BeginPass(0));

	for(vector<Blood*>::const_iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
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
		Matrix m2 = m1 * game_level->camera.mat_view_proj;
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
void Game::DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2)
{
	IDirect3DDevice9* device = render->GetDevice();
	ID3DXEffect* effect = basic_shader->effect;

	render->SetAlphaTest(false);
	render->SetAlphaBlend(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_POS)));

	V(effect->SetTechnique(basic_shader->techArea));
	V(effect->SetMatrix(basic_shader->hMatCombined, (D3DXMATRIX*)&game_level->camera.mat_view_proj));
	V(effect->SetVector(basic_shader->hColor, (D3DXVECTOR4*)&Vec4(0, 1, 0, 0.5f)));
	Vec4 playerPos(pc->unit->pos, 1.f);
	playerPos.y += 0.75f;
	V(effect->SetVector(basic_shader->hPlayerPos, (D3DXVECTOR4*)&playerPos));
	V(effect->SetFloat(basic_shader->hRange, range));
	uint passes;
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(const Area& area : areas)
		V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (const void*)&area.v[0], sizeof(Vec3)));

	V(effect->EndPass());
	V(effect->End());

	if(!areas2.empty())
	{
		V(effect->Begin(&passes, 0));
		V(effect->BeginPass(0));
		V(effect->SetFloat(basic_shader->hRange, 100.f));

		static const Vec4 colors[3] = {
			Vec4(1, 0, 0, 0.5f),
			Vec4(1, 1, 0, 0.5f),
			Vec4(0, 0.58f, 1.f, 0.5f)
		};

		for(auto* area2 : areas2)
		{
			V(effect->SetVector(basic_shader->hColor, (D3DXVECTOR4*)&colors[area2->ok]));
			V(effect->CommitChanges());
			V(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, area2->points.size(), area2->faces.size() / 3, area2->faces.data(), D3DFMT_INDEX16,
				area2->points.data(), sizeof(Vec3)));
		}

		V(effect->EndPass());
		V(effect->End());
	}
}

//=================================================================================================
void Game::UvModChanged()
{
	game_level->terrain->uv_mod = uv_mod;
	game_level->terrain->RebuildUv();
}
