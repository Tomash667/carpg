#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "LocationHelper.h"
#include "Profiler.h"
#include "Level.h"
#include "ResourceManager.h"
#include "Render.h"
#include "GrassShader.h"
#include "Object.h"
#include "GameResources.h"
#include "DirectX.h"

enum QuadPartType
{
	Q_LEFT_BOTTOM,
	Q_RIGHT_BOTTOM,
	Q_LEFT_TOP,
	Q_RIGHT_TOP
};

void QuadTree::Init(QuadNode* node, const Box2d& box, const Rect& grid_box, int splits)
{
	if(node)
	{
		node->box = box;
		node->grid_box = grid_box;
		if(splits > 0)
		{
			node->leaf = false;
			--splits;
			node->childs[Q_LEFT_BOTTOM] = get();
			Init(node->childs[Q_LEFT_BOTTOM], box.LeftBottomPart(), grid_box.LeftBottomPart(), splits);
			node->childs[Q_RIGHT_BOTTOM] = get();
			Init(node->childs[Q_RIGHT_BOTTOM], box.RightBottomPart(), grid_box.RightBottomPart(), splits);
			node->childs[Q_LEFT_TOP] = get();
			Init(node->childs[Q_LEFT_TOP], box.LeftTopPart(), grid_box.LeftTopPart(), splits);
			node->childs[Q_RIGHT_TOP] = get();
			Init(node->childs[Q_RIGHT_TOP], box.RightTopPart(), grid_box.RightTopPart(), splits);
		}
		else
		{
			node->leaf = true;
			for(int i = 0; i < 4; ++i)
				node->childs[i] = nullptr;
		}
	}
	else
	{
		top = get();
		Init(top, box, grid_box, splits);
	}
}

void QuadTree::List(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			nodes.push_back(node);
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmp_nodes.push_back(node->childs[i]);
			}
		}
	}
}

void QuadTree::ListLeafs(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmp_nodes.push_back(node->childs[i]);
			}
			else
				nodes.push_back(node);
		}
	}
}

void QuadTree::Clear(Nodes& nodes)
{
	if(!top)
		return;

	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		nodes.push_back(node);
		for(int i = 0; i < 4; ++i)
		{
			if(node->childs[i])
				tmp_nodes.push_back(node->childs[i]);
		}
	}

	top = nullptr;
}

QuadNode* QuadTree::GetNode(const Vec2& pos, float radius)
{
	QuadNode* node = top;
	if(!node->box.IsFullyInside(pos, radius))
		return top;

	while(!node->leaf)
	{
		Vec2 midpoint = node->box.Midpoint();
		int index;
		if(pos.x >= midpoint.x)
		{
			if(pos.y >= midpoint.y)
				index = Q_RIGHT_BOTTOM;
			else
				index = Q_RIGHT_TOP;
		}
		else
		{
			if(pos.y >= midpoint.y)
				index = Q_LEFT_BOTTOM;
			else
				index = Q_LEFT_TOP;
		}

		if(node->childs[index]->box.IsFullyInside(pos, radius))
			node = node->childs[index];
		else
			break;
	}

	return node;
}

ObjectPool<LevelPart> level_parts_pool;

QuadNode* GetLevelPart()
{
	LevelPart* part = level_parts_pool.Get();
	part->generated = false;
	return part;
}

void Game::InitQuadTree()
{
	quadtree.get = GetLevelPart;
	quadtree.Init(nullptr, Box2d(0, 0, 256, 256), Rect(0, 0, 128, 128), 5);
}

void Game::DrawGrass()
{
	if(grass_patches[0].empty() && grass_patches[1].empty())
		return;

	PROFILER_BLOCK("DrawGrass");

	grass_shader->SetCamera(game_level->camera);
	grass_shader->SetFog(game_level->GetFogColor(), game_level->GetFogParams());
	grass_shader->Begin(max(grass_count[0], grass_count[1]));
	for(int i = 0; i < 2; ++i)
	{
		if(grass_count[i] > 0)
		{
			Mesh* mesh = BaseObject::Get(i == 0 ? "grass" : "corn")->mesh;
			grass_shader->Draw(mesh, grass_patches[i], grass_count[i]);
		}
	}
	grass_shader->End();
}

void Game::ListGrass()
{
	if(settings.grass_range < 0.5f)
		return;

	PROFILER_BLOCK("ListGrass");
	OutsideLocation* outside = static_cast<OutsideLocation*>(game_level->location);
	Vec3 pos, angle;
	Vec2 from = game_level->camera.from.XZ();
	float in_dist = settings.grass_range * settings.grass_range;

	for(LevelParts::iterator it = level_parts.begin(), end = level_parts.end(); it != end; ++it)
	{
		LevelPart& part = **it;
		if(!part.leaf || Vec2::DistanceSquared(part.box.Midpoint(), from) > in_dist)
			continue;

		if(!part.generated)
		{
			int minx = max(1, part.grid_box.p1.x),
				miny = max(1, part.grid_box.p1.y),
				maxx = min(OutsideLocation::size - 1, part.grid_box.p2.x),
				maxy = min(OutsideLocation::size - 1, part.grid_box.p2.y);
			part.generated = true;
			for(int y = miny; y < maxy; ++y)
			{
				for(int x = minx; x < maxx; ++x)
				{
					TERRAIN_TILE t = outside->tiles[x + y * OutsideLocation::size].t;
					if(t == TT_GRASS)
					{
						if(outside->tiles[x + y * OutsideLocation::size].mode != TM_NO_GRASS)
						{
							if(outside->tiles[x - 1 + y * OutsideLocation::size].t == TT_GRASS
								&& outside->tiles[x + 1 + y * OutsideLocation::size].t == TT_GRASS
								&& outside->tiles[x + (y - 1)*OutsideLocation::size].t == TT_GRASS
								&& outside->tiles[x + (y + 1)*OutsideLocation::size].t == TT_GRASS)
							{
								for(int i = 0; i < 6; ++i)
								{
									pos = Vec3(2.f*x + Random(2.f), 0.f, 2.f*y + Random(2.f));
									game_level->terrain->GetAngle(pos.x, pos.z, angle);
									if(angle.y < 0.7f)
										continue;
									game_level->terrain->SetH(pos);
									part.grass.push_back(Matrix::Scale(Random(3.f, 4.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
								}
							}
							else
							{
								for(int i = 0; i < 4; ++i)
								{
									pos = Vec3(2.f*x + 0.1f + Random(1.8f), 0, 2.f*y + 0.1f + Random(1.8f));
									game_level->terrain->SetH(pos);
									part.grass.push_back(Matrix::Scale(Random(2.f, 3.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
								}
							}
						}
					}
					else if(t == TT_FIELD)
					{
						if(outside->tiles[x - 1 + y * OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + 1 + y * OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + (y - 1)*OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + (y + 1)*OutsideLocation::size].mode == TM_FIELD)
						{
							for(int i = 0; i < 1; ++i)
							{
								pos = Vec3(2.f*x + 0.5f + Random(1.f), 0, 2.f*y + 0.5f + Random(1.f));
								game_level->terrain->SetH(pos);
								part.grass2.push_back(Matrix::Scale(Random(3.f, 4.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
							}
						}
					}
				}
			}
		}

		if(!part.grass.empty())
		{
			grass_patches[0].push_back(&part.grass);
			grass_count[0] += part.grass.size();
		}
		if(!part.grass2.empty())
		{
			grass_patches[1].push_back(&part.grass2);
			grass_count[1] += part.grass2.size();
		}
	}
}

void Game::SetTerrainTextures()
{
	TexturePtr tex[5] = { game_res->tGrass, game_res->tGrass2, game_res->tGrass3, game_res->tFootpath, game_res->tRoad };
	if(LocationHelper::IsVillage(game_level->location))
		tex[2] = game_res->tField;

	for(int i = 0; i < 5; ++i)
		res_mgr->Load(tex[i]);

	game_level->terrain->SetTextures(tex);
}

void Game::ClearQuadtree()
{
	if(!quadtree.top)
		return;

	quadtree.Clear((QuadTree::Nodes&)level_parts);

	for(LevelParts::iterator it = level_parts.begin(), end = level_parts.end(); it != end; ++it)
	{
		LevelPart& part = **it;
		part.grass.clear();
		part.grass2.clear();
		part.objects.clear();
	}

	level_parts_pool.Free(level_parts);
	level_parts.clear();
}

void Game::ClearGrass()
{
	grass_patches[0].clear();
	grass_patches[1].clear();
	grass_count[0] = 0;
	grass_count[1] = 0;
}

void Game::CalculateQuadtree()
{
	if(game_level->local_area->area_type != LevelArea::Type::Outside)
		return;

	for(Object* obj : game_level->local_area->objects)
	{
		auto node = (LevelPart*)quadtree.GetNode(obj->pos.XZ(), obj->GetRadius());
		node->objects.push_back(QuadObj(obj));
	}
}

void Game::ListQuadtreeNodes()
{
	PROFILER_BLOCK("ListQuadtreeNodes");
	quadtree.List(game_level->camera.frustum, (QuadTree::Nodes&)level_parts);
}
