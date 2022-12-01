#include "Pch.h"
#include "Game.h"

#include "GameResources.h"
#include "Level.h"
#include "LevelPart.h"
#include "LocationHelper.h"
#include "Object.h"

#include <DirectX.h>
#include <GrassShader.h>
#include <ResourceManager.h>
#include <Terrain.h>

enum QuadPartType
{
	Q_LEFT_BOTTOM,
	Q_RIGHT_BOTTOM,
	Q_LEFT_TOP,
	Q_RIGHT_TOP
};

void QuadTree::Init(QuadNode* node, const Box2d& box, const Rect& gridBox, int splits)
{
	if(node)
	{
		node->box = box;
		node->gridBox = gridBox;
		if(splits > 0)
		{
			node->leaf = false;
			--splits;
			node->childs[Q_LEFT_BOTTOM] = get();
			Init(node->childs[Q_LEFT_BOTTOM], box.LeftBottomPart(), gridBox.LeftBottomPart(), splits);
			node->childs[Q_RIGHT_BOTTOM] = get();
			Init(node->childs[Q_RIGHT_BOTTOM], box.RightBottomPart(), gridBox.RightBottomPart(), splits);
			node->childs[Q_LEFT_TOP] = get();
			Init(node->childs[Q_LEFT_TOP], box.LeftTopPart(), gridBox.LeftTopPart(), splits);
			node->childs[Q_RIGHT_TOP] = get();
			Init(node->childs[Q_RIGHT_TOP], box.RightTopPart(), gridBox.RightTopPart(), splits);
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
		Init(top, box, gridBox, splits);
	}
}

void QuadTree::List(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmpNodes.clear();
	tmpNodes.push_back(top);

	while(!tmpNodes.empty())
	{
		QuadNode* node = tmpNodes.back();
		tmpNodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			nodes.push_back(node);
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmpNodes.push_back(node->childs[i]);
			}
		}
	}
}

void QuadTree::ListLeafs(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmpNodes.clear();
	tmpNodes.push_back(top);

	while(!tmpNodes.empty())
	{
		QuadNode* node = tmpNodes.back();
		tmpNodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmpNodes.push_back(node->childs[i]);
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
	tmpNodes.clear();
	tmpNodes.push_back(top);

	while(!tmpNodes.empty())
	{
		QuadNode* node = tmpNodes.back();
		tmpNodes.pop_back();
		nodes.push_back(node);
		for(int i = 0; i < 4; ++i)
		{
			if(node->childs[i])
				tmpNodes.push_back(node->childs[i]);
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

ObjectPool<LevelQuad> level_quads_pool;

QuadNode* GetLevelQuad()
{
	LevelQuad* quad = level_quads_pool.Get();
	quad->generated = false;
	return quad;
}

void Game::InitQuadTree()
{
	quadtree.get = GetLevelQuad;
	quadtree.Init(nullptr, Box2d(0, 0, 256, 256), Rect(0, 0, 128, 128), 5);
}

void Game::DrawGrass()
{
	if(grass_patches[0].empty() && grass_patches[1].empty())
		return;

	grass_shader->Prepare(game_level->localPart->lvlPart->scene, &game_level->camera);
	for(int i = 0; i < 2; ++i)
	{
		if(grass_count[i] > 0)
		{
			Mesh* mesh = BaseObject::Get(i == 0 ? "grass" : "corn")->mesh;
			grass_shader->Draw(mesh, grass_patches[i], grass_count[i]);
		}
	}
}

void Game::ListGrass()
{
	if(settings.grass_range < 0.5f)
		return;

	OutsideLocation* outside = static_cast<OutsideLocation*>(game_level->location);
	Vec3 pos, angle;
	Vec2 from = game_level->camera.from.XZ();
	float in_dist = settings.grass_range * settings.grass_range;

	for(LevelQuads::iterator it = level_quads.begin(), end = level_quads.end(); it != end; ++it)
	{
		LevelQuad& quad = **it;
		if(!quad.leaf || Vec2::DistanceSquared(quad.box.Midpoint(), from) > in_dist)
			continue;

		if(!quad.generated)
		{
			int minx = max(1, quad.gridBox.p1.x),
				miny = max(1, quad.gridBox.p1.y),
				maxx = min(OutsideLocation::size - 1, quad.gridBox.p2.x),
				maxy = min(OutsideLocation::size - 1, quad.gridBox.p2.y);
			quad.generated = true;
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
								&& outside->tiles[x + (y - 1) * OutsideLocation::size].t == TT_GRASS
								&& outside->tiles[x + (y + 1) * OutsideLocation::size].t == TT_GRASS)
							{
								for(int i = 0; i < 6; ++i)
								{
									pos = Vec3(2.f * x + Random(2.f), 0.f, 2.f * y + Random(2.f));
									game_level->terrain->GetAngle(pos.x, pos.z, angle);
									if(angle.y < 0.7f)
										continue;
									game_level->terrain->SetY(pos);
									quad.grass.push_back(Matrix::Scale(Random(3.f, 4.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
								}
							}
							else
							{
								for(int i = 0; i < 4; ++i)
								{
									pos = Vec3(2.f * x + 0.1f + Random(1.8f), 0, 2.f * y + 0.1f + Random(1.8f));
									game_level->terrain->SetY(pos);
									quad.grass.push_back(Matrix::Scale(Random(2.f, 3.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
								}
							}
						}
					}
					else if(t == TT_FIELD)
					{
						if(outside->tiles[x - 1 + y * OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + 1 + y * OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + (y - 1) * OutsideLocation::size].mode == TM_FIELD
							&& outside->tiles[x + (y + 1) * OutsideLocation::size].mode == TM_FIELD)
						{
							for(int i = 0; i < 1; ++i)
							{
								pos = Vec3(2.f * x + 0.5f + Random(1.f), 0, 2.f * y + 0.5f + Random(1.f));
								game_level->terrain->SetY(pos);
								quad.grass2.push_back(Matrix::Scale(Random(3.f, 4.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
							}
						}
					}
				}
			}
		}

		if(!quad.grass.empty())
		{
			grass_patches[0].push_back(&quad.grass);
			grass_count[0] += quad.grass.size();
		}
		if(!quad.grass2.empty())
		{
			grass_patches[1].push_back(&quad.grass2);
			grass_count[1] += quad.grass2.size();
		}
	}
}

void Game::SetTerrainTextures()
{
	TexturePtr tex[5] = { game_res->tGrass, game_res->tGrass2, game_res->tGrass3, game_res->tFootpath, game_res->tRoad };
	if(LocationHelper::IsVillage(game_level->location))
		tex[2] = game_res->tField;

	for(int i = 0; i < 5; ++i)
		resMgr->Load(tex[i]);

	game_level->terrain->SetTextures(tex);
}

void Game::ClearQuadtree()
{
	if(!quadtree.top)
		return;

	quadtree.Clear((QuadTree::Nodes&)level_quads);

	for(LevelQuads::iterator it = level_quads.begin(), end = level_quads.end(); it != end; ++it)
	{
		LevelQuad& quad = **it;
		quad.grass.clear();
		quad.grass2.clear();
		quad.objects.clear();
	}

	level_quads_pool.Free(level_quads);
	level_quads.clear();
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
	if(game_level->localPart->partType != LocationPart::Type::Outside)
		return;

	for(Object* obj : game_level->localPart->objects)
	{
		auto node = (LevelQuad*)quadtree.GetNode(obj->pos.XZ(), obj->GetRadius());
		node->objects.push_back(QuadObj(obj));
	}
}

void Game::ListQuadtreeNodes()
{
	quadtree.List(game_level->camera.frustum, (QuadTree::Nodes&)level_quads);
}
