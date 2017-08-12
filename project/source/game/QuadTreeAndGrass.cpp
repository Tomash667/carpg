#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "Terrain.h"
#include "LocationHelper.h"

void QuadTree::Init(QuadNode* node, const Box2d& box, const Rect& grid_box, int splits, float margin)
{
	if(node)
	{
		node->rect = QuadRect(box);
		node->box = Box2d(box, margin);
		node->grid_box = grid_box;
		if(splits > 0)
		{
			node->leaf = false;
			--splits;
			//margin /= 2;
			node->childs[0] = get();
			Init(node->childs[0], box.LeftBottomPart(), grid_box.LeftBottomPart(), splits, margin);
			node->childs[1] = get();
			Init(node->childs[1], box.RightBottomPart(), grid_box.RightBottomPart(), splits, margin);
			node->childs[2] = get();
			Init(node->childs[2], box.LeftTopPart(), grid_box.LeftTopPart(), splits, margin);
			node->childs[3] = get();
			Init(node->childs[3], box.RightTopPart(), grid_box.RightTopPart(), splits, margin);
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
		Init(top, box, grid_box, splits, margin);
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
	const float radius_x2 = radius * 2;

	if(!CircleToRectangle(pos.x, pos.y, radius, node->rect.x, node->rect.y, node->rect.w, node->rect.h))
		return nullptr;

	while(true)
	{
		if(radius_x2 < node->rect.w && !node->leaf)
		{
			for(int i = 0; i < 4; ++i)
			{
				if(CircleToRectangle(pos.x, pos.y, radius, node->childs[i]->rect.x, node->childs[i]->rect.y, node->childs[i]->rect.w, node->childs[i]->rect.h))
				{
					node = node->childs[i];
					break;
				}
			}
		}
		else
			return node;
	}
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
	quadtree.Init(nullptr, Box2d(0, 0, 256, 256), Rect(0, 0, 128, 128), 5, 2.f);
}

void Game::DrawGrass()
{
	if(grass_patches[0].empty() && grass_patches[1].empty())
		return;

	PROFILER_BLOCK("DrawGrass");

	SetAlphaBlend(false);
	SetAlphaTest(true);
	SetNoCulling(true);
	SetNoZWrite(false);

	const uint count = max(grass_count[0], grass_count[1]);

	// create vertex buffer if existing is too small
	if(!vbInstancing || vb_instancing_max < count)
	{
		SafeRelease(vbInstancing);
		if(vb_instancing_max < count)
			vb_instancing_max = count;
		V(device->CreateVertexBuffer(sizeof(Matrix)*vb_instancing_max, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbInstancing, nullptr));
	}

	// setup stream source for instancing
	V(device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1));
	V(device->SetStreamSource(1, vbInstancing, 0, sizeof(Matrix)));

	// set effect
	Vec4 fogColor = GetFogColor();
	Vec4 fogParams = GetFogParams();
	Vec4 ambientColor(0.8f, 0.8f, 0.8f, 1.f);
	UINT passes;
	V(eGrass->SetTechnique(techGrass));
	V(eGrass->SetVector(hGrassFogColor, (D3DXVECTOR4*)&fogColor));
	V(eGrass->SetVector(hGrassFogParams, (D3DXVECTOR4*)&fogParams));
	V(eGrass->SetVector(hGrassAmbientColor, (D3DXVECTOR4*)&ambientColor));
	V(eGrass->SetMatrix(hGrassViewProj, (D3DXMATRIX*)&cam.matViewProj));
	V(eGrass->Begin(&passes, 0));
	V(eGrass->BeginPass(0));

	// normal grass
	for(int j = 0; j < 2; ++j)
	{
		if(!grass_patches[j].empty())
		{
			Mesh* mesh = FindObject(j == 0 ? "grass" : "corn")->mesh;

			// setup instancing data
			Matrix* m;
			V(vbInstancing->Lock(0, 0, (void**)&m, D3DLOCK_DISCARD));
			int index = 0;
			for(vector< const vector<Matrix>* >::const_iterator it = grass_patches[j].begin(), end = grass_patches[j].end(); it != end; ++it)
			{
				const vector<Matrix>& vm = **it;
				memcpy(&m[index], &vm[0], sizeof(Matrix)*vm.size());
				index += vm.size();
			}
			V(vbInstancing->Unlock());

			// setup stream source for mesh
			V(device->SetVertexDeclaration(vertex_decl[VDI_GRASS]));
			V(device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | grass_count[j]));
			V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
			V(device->SetIndices(mesh->ib));

			// draw
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				V(eGrass->SetTexture(hGrassTex, mesh->subs[i].tex->tex));
				V(eGrass->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
			}
		}
	}

	// end draw
	V(eGrass->EndPass());
	V(eGrass->End());

	// restore vertex stream frequency
	V(device->SetStreamSourceFreq(0, 1));
	V(device->SetStreamSourceFreq(1, 1));
}

extern Matrix m1, m2, m3, m4;

void Game::ListGrass()
{
	ClearGrass();

	if(grass_range < 0.5f)
		return;

	quadtree.ListLeafs(cam.frustum2, (QuadTree::Nodes&)level_parts);

	OutsideLocation* outside = (OutsideLocation*)location;
	Vec3 pos, angle;

	for(LevelParts::iterator it = level_parts.begin(), end = level_parts.end(); it != end; ++it)
	{
		LevelPart& part = **it;
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
					TERRAIN_TILE t = outside->tiles[x + y*OutsideLocation::size].t;
					if(t == TT_GRASS)
					{
						if(outside->tiles[x - 1 + y*OutsideLocation::size].t == TT_GRASS &&
							outside->tiles[x + 1 + y*OutsideLocation::size].t == TT_GRASS &&
							outside->tiles[x + (y - 1)*OutsideLocation::size].t == TT_GRASS &&
							outside->tiles[x + (y + 1)*OutsideLocation::size].t == TT_GRASS)
						{
							for(int i = 0; i < 6; ++i)
							{
								pos = Vec3(2.f*x + Random(2.f), 0.f, 2.f*y + Random(2.f));
								terrain->GetAngle(pos.x, pos.z, angle);
								if(angle.y < 0.7f)
									continue;
								part.grass.push_back(Matrix::Scale(Random(3.f, 4.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
							}
						}
						else
						{
							for(int i = 0; i < 4; ++i)
							{
								pos = Vec3(2.f*x + 0.1f + Random(1.8f), 0, 2.f*y + 0.1f + Random(1.8f));
								terrain->SetH(pos);
								part.grass.push_back(Matrix::Scale(Random(2.f, 3.f)) * Matrix::RotationY(Random(MAX_ANGLE)) * Matrix::Translation(pos));
							}
						}
					}
					else if(t == TT_FIELD)
					{
						if(outside->tiles[x - 1 + y*OutsideLocation::size].mode == TM_FIELD &&
							outside->tiles[x + 1 + y*OutsideLocation::size].mode == TM_FIELD &&
							outside->tiles[x + (y - 1)*OutsideLocation::size].mode == TM_FIELD &&
							outside->tiles[x + (y + 1)*OutsideLocation::size].mode == TM_FIELD)
						{
							for(int i = 0; i < 1; ++i)
							{
								pos = Vec3(2.f*x + 0.5f + Random(1.f), 0, 2.f*y + 0.5f + Random(1.f));
								terrain->SetH(pos);
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
	TexturePtr tex[5] = { tTrawa, tTrawa2, tTrawa3, tZiemia, tDroga };
	if(LocationHelper::IsVillage(location))
		tex[1] = tPole;

	auto& tex_mgr = ResourceManager::Get<Texture>();
	for(int i = 0; i < 5; ++i)
		tex_mgr.AddLoadTask(tex[i]);

	terrain->SetTextures(tex);
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
