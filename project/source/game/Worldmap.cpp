#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "CityGenerator.h"
#include "Inventory.h"
#include "Quest_Sawmill.h"
#include "Quest_Bandits.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Perlin.h"
#include "LocationHelper.h"
#include "Content.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "Cave.h"
#include "Camp.h"
#include "MultiInsideLocation.h"
#include "WorldMapGui.h"
#include "GameGui.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "MultiplayerPanel.h"
#include "AIController.h"
#include "Team.h"
#include "ItemContainer.h"
#include "BuildingScript.h"
#include "BuildingGroup.h"
#include "Stock.h"
#include "UnitGroup.h"
#include "Portal.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"
#include "ScriptManager.h"
#include "Var.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "LocationGeneratorFactory.h"
#include "OutsideObject.h"

extern Matrix m1, m2, m3, m4;

void Game::GenerateWorld()
{
	if(next_seed != 0)
	{
		Srand(next_seed);
		next_seed = 0;
	}
	else if(force_seed != 0 && force_seed_all)
		Srand(force_seed);

	Info("Generating world, seed %u.", RandVal());

	W.GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

bool Game::EnterLocation(int level, int from_portal, bool close_portal)
{
	Location& l = *L.location;
	if(l.type == L_ACADEMY)
	{
		ShowAcademyText();
		return false;
	}

	world_map->visible = false;
	game_gui->visible = true;

	const bool reenter = L.is_open;
	L.is_open = true;
	if(W.GetState() != World::State::INSIDE_ENCOUNTER)
		W.SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		L.enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		L.enter_from = ENTER_FROM_OUTSIDE;
	if(!reenter)
		L.light_angle = Random(PI * 2);

	dungeon_level = level;
	L.event_handler = nullptr;
	pc_data.before_player = BP_NONE;
	arena_free = true; //zabezpieczenie :3
	unit_views.clear();
	Inventory::lock = nullptr;

	bool first = false;

	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		first = true;
		level_generated = true;
	}
	else
		level_generated = false;

	if(!reenter)
		InitQuadTree();

	if(Net::IsOnline() && players > 1)
	{
		packet_data.resize(4);
		packet_data[0] = ID_CHANGE_LEVEL;
		packet_data[1] = (byte)L.location_index;
		packet_data[2] = 0;
		packet_data[3] = (W.GetState() == World::State::INSIDE_ENCOUNTER ? 1 : 0);
		int ack = peer->Send((cstring)&packet_data[0], 4, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		StreamWrite(packet_data, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
		for(auto info : game_players)
		{
			if(info->id == my_id)
				info->state = PlayerInfo::IN_GAME;
			else
			{
				info->state = PlayerInfo::WAITING_FOR_RESPONSE;
				info->ack = ack;
				info->timer = 5.f;
			}
		}
		Net_FilterServerChanges();
	}

	LocationGenerator* loc_gen = loc_gen_factory->Get(&l);
	loc_gen->Init(first, reenter);

	// calculate number of loading steps for drawing progress bar
	int steps = loc_gen->GetNumberOfSteps();
	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		if(next_seed != 0)
		{
			Srand(next_seed);
			next_seed = 0;
		}
		else if(force_seed != 0 && force_seed_all)
			Srand(force_seed);

		// log what is required to generate location for testing
		if(l.type == L_CITY)
		{
			City* city = (City*)&l;
			Info("Generating location '%s', seed %u [%d].", l.name.c_str(), RandVal(), city->citizens);
		}
		else
			Info("Generating location '%s', seed %u.", l.name.c_str(), RandVal());
		l.seed = RandVal();
		if(!l.outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				multi->infos[dungeon_level].seed = l.seed;
			}
		}

		// nie odwiedzono, trzeba wygenerowaæ
		l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		loc_gen->Generate();
	}
	else if(l.type != L_DUNGEON && l.type != L_CRYPT)
		Info("Entering location '%s'.", l.name.c_str());

	city_ctx = nullptr;

	if(L.location->outside)
	{
		loc_gen->OnEnter();
		SetTerrainTextures();
		CalculateQuadtree();
	}
	else
		EnterLevel(loc_gen);

	bool loaded_resources = RequireLoadingResources(L.location, nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = W.GetWorldtime();
	CheckIfLocationCleared();
	local_ctx_valid = true;
	cam.Reset();
	pc_data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete L.location->portal;
		L.location->portal = nullptr;
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(players > 1)
		{
			net_stream.Reset();
			PrepareLevelData(net_stream, loaded_resources);
			Info("Generated location packet: %d.", net_stream.GetNumberOfBytesUsed());
		}
		else
			game_players[0]->state = PlayerInfo::IN_GAME;

		info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui->visible = true;
	}

	if(L.location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");

	return true;
}

void Game::ApplyTiles(float* _h, TerrainTile* _tiles)
{
	assert(_h && _tiles);

	TEX splat = terrain->GetSplatTexture();
	D3DLOCKED_RECT lock;
	V(splat->LockRect(0, &lock, nullptr, 0));
	byte* bits = (byte*)lock.pBits;
	for(uint y = 0; y < 256; ++y)
	{
		DWORD* row = (DWORD*)(bits + lock.Pitch * y);
		for(uint x = 0; x < 256; ++x, ++row)
		{
			TerrainTile& t = _tiles[x / 2 + y / 2 * OutsideLocation::size];
			if(t.alpha == 0)
				*row = terrain_tile_info[t.t].mask;
			else
			{
				const TerrainTileInfo& tti1 = terrain_tile_info[t.t];
				const TerrainTileInfo& tti2 = terrain_tile_info[t.t2];
				if(tti1.shift > tti2.shift)
					*row = tti2.mask + ((255 - t.alpha) << tti1.shift);
				else
					*row = tti1.mask + (t.alpha << tti2.shift);
			}
		}
	}
	V(splat->UnlockRect(0));
	splat->GenerateMipSubLevels();

	terrain->SetHeightMap(_h);
	terrain->Rebuild();
	terrain->CalculateBox();
}

Game::ObjectEntity Game::SpawnObjectEntity(LevelContext& ctx, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* out_point,
	int variant)
{
	if(IS_SET(base->flags, OBJ_TABLE_SPAWNER))
	{
		// table & stools
		BaseObject* table = BaseObject::Get(Rand() % 2 == 0 ? "table" : "table2");
		BaseUsable* stool = BaseUsable::Get("stool");

		// table
		Object* o = new Object;
		o->mesh = table->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = 1;
		o->base = table;
		ctx.objects->push_back(o);
		SpawnObjectExtras(ctx, table, pos, rot, o);

		// stools
		int count = Random(2, 4);
		int d[4] = { 0,1,2,3 };
		for(int i = 0; i < 4; ++i)
			std::swap(d[Rand() % 4], d[Rand() % 4]);

		for(int i = 0; i < count; ++i)
		{
			float sdir, slen;
			switch(d[i])
			{
			case 0:
				sdir = 0.f;
				slen = table->size.y + 0.3f;
				break;
			case 1:
				sdir = PI / 2;
				slen = table->size.x + 0.3f;
				break;
			case 2:
				sdir = PI;
				slen = table->size.y + 0.3f;
				break;
			case 3:
				sdir = PI * 3 / 2;
				slen = table->size.x + 0.3f;
				break;
			default:
				assert(0);
				break;
			}

			sdir += rot;

			Usable* u = new Usable;
			ctx.usables->push_back(u);
			u->base = stool;
			u->pos = pos + Vec3(sin(sdir)*slen, 0, cos(sdir)*slen);
			u->rot = sdir;
			u->user = nullptr;
			if(Net::IsOnline())
				u->netid = Usable::netid_counter++;

			SpawnObjectExtras(ctx, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IS_SET(base->flags, OBJ_BUILDING))
	{
		// building
		int roti;
		if(Equal(rot, 0))
			roti = 0;
		else if(Equal(rot, PI / 2))
			roti = 1;
		else if(Equal(rot, PI))
			roti = 2;
		else if(Equal(rot, PI * 3 / 2))
			roti = 3;
		else
		{
			assert(0);
			roti = 0;
			rot = 0.f;
		}

		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		ctx.objects->push_back(o);

		ProcessBuildingObjects(ctx, nullptr, nullptr, o->mesh, nullptr, rot, roti, pos, nullptr, nullptr, false, out_point);

		return o;
	}
	else if(IS_SET(base->flags, OBJ_USABLE))
	{
		// usable object
		BaseUsable* base_use = (BaseUsable*)base;

		Usable* u = new Usable;
		u->base = base_use;
		u->pos = pos;
		u->rot = rot;
		u->user = nullptr;

		if(IS_SET(base_use->use_flags, BaseUsable::CONTAINER))
		{
			u->container = new ItemContainer;
			auto item = GetRandomBook();
			if(item)
				u->container->items.push_back({ item, 1, 1 });
		}

		if(variant == -1)
		{
			if(base->variants)
			{
				// extra code for bench
				if(IS_SET(base_use->use_flags, BaseUsable::IS_BENCH))
				{
					switch(L.location->type)
					{
					case L_CITY:
						variant = 0;
						break;
					case L_DUNGEON:
					case L_CRYPT:
						variant = Rand() % 2;
						break;
					default:
						variant = Rand() % 2 + 2;
						break;
					}
				}
				else
					variant = Random<int>(base->variants->entries.size() - 1);
			}
		}
		u->variant = variant;

		if(Net::IsOnline())
			u->netid = Usable::netid_counter++;
		ctx.usables->push_back(u);

		SpawnObjectExtras(ctx, base, pos, rot, u, scale, flags);

		return u;
	}
	else if(IS_SET(base->flags, OBJ_IS_CHEST))
	{
		// chest
		Chest* chest = new Chest;
		chest->mesh_inst = new MeshInstance(base->mesh);
		chest->rot = rot;
		chest->pos = pos;
		chest->handler = nullptr;
		chest->looted = false;
		ctx.chests->push_back(chest);
		if(Net::IsOnline())
			chest->netid = Chest::netid_counter++;

		SpawnObjectExtras(ctx, base, pos, rot, nullptr, scale, flags);

		return chest;
	}
	else
	{
		// normal object
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		ctx.objects->push_back(o);

		SpawnObjectExtras(ctx, base, pos, rot, o, scale, flags);

		return o;
	}
}

void Game::ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
	const Vec3& shift, Building* type, CityBuilding* building, bool recreate, Vec3* out_point)
{
	if(mesh->attach_points.empty())
	{
		building->walk_pt = shift;
		assert(!inside);
		return;
	}

	// https://github.com/Tomash667/carpg/wiki/%5BPL%5D-Buildings-export
	// o_x_[!N!]nazwa_???
	// x (o - obiekt, r - obrócony obiekt, p - fizyka, s - strefa, c - postaæ, m - maska œwiat³a, d - detal wokó³ obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	string token;
	Matrix m1, m2;
	bool have_exit = false, have_spawn = false;
	bool is_inside = (inside != nullptr);
	Vec3 spawn_point;
	static vector<const Mesh::Point*> details;

	for(vector<Mesh::Point>::const_iterator it2 = mesh->attach_points.begin(), end2 = mesh->attach_points.end(); it2 != end2; ++it2)
	{
		const Mesh::Point& pt = *it2;
		if(pt.name.length() < 5 || pt.name[0] != 'o')
			continue;

		char c = pt.name[2];
		if(c == 'o' || c == 'r' || c == 'p' || c == 's' || c == 'c' || c == 'l')
		{
			uint poss = pt.name.find_first_of('_', 4);
			if(poss == string::npos)
			{
				assert(0);
				continue;
			}
			token = pt.name.substr(4, poss - 4);
			for(uint k = 0, len = token.length(); k < len; ++k)
			{
				if(token[k] == '#')
					token[k] = '_';
			}
		}
		else if(c == 'm')
		{
			// light mask
			// nothing to do here
		}
		else if(c == 'd')
		{
			if (!recreate)
			{
				assert(!is_inside);
				details.push_back(&pt);
			}
			continue;
		}
		else
			continue;

		Vec3 pos;
		if(roti != 0)
		{
			m2 = pt.mat * Matrix::RotationY(rot);
			pos = Vec3::TransformZero(m2);
		}
		else
			pos = Vec3::TransformZero(pt.mat);
		pos += shift;

		if(c == 'o' || c == 'r' || c == 'l')
		{
			// obiekt / obrócony obiekt
			if(!recreate)
			{
				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* base = BaseObject::TryGet(name);
				assert(base);

				if(base)
				{
					if(ctx.type == LevelContext::Outside)
						terrain->SetH(pos);
					float r;
					switch(c)
					{
					case 'o':
					default:
						r = Random(MAX_ANGLE);
						break;
					case 'r':
						r = Clip(pt.rot.y + rot);
						break;
					case 'l':
						r = PI / 2 * (Rand() % 4);
						break;
					}
					SpawnObjectEntity(ctx, base, pos, r, 1.f, 0, nullptr, variant);
				}
			}
		}
		else if(c == 'p')
		{
			// fizyka
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.ptr = (is_wall ? CAM_COLLIDER : nullptr);

				if(ctx.type == LevelContext::Outside)
				{
					terrain->SetH(pos);
					pos.y += 2.f;
				}

				btCylinderShape* shape = new btCylinderShape(btVector3(pt.size.x, 4.f, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);
			}
			else if(token == "square" || token == "squarev" || token == "squarevn" || token == "squarevp")
			{
				bool is_wall = (token == "square" || token == "squarevn");
				bool block_camera = (token == "square");

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::RECTANGLE;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.w = pt.size.x;
				cobj.h = pt.size.z;
				cobj.ptr = (block_camera ? CAM_COLLIDER : nullptr);

				btBoxShape* shape;
				if(token != "squarevp")
				{
					shape = new btBoxShape(btVector3(pt.size.x, 16.f, pt.size.z));
					if(ctx.type == LevelContext::Outside)
					{
						terrain->SetH(pos);
						pos.y += 8.f;
					}
					else
						pos.y = 0.f;
				}
				else
				{
					shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
					if(ctx.type == LevelContext::Outside)
						pos.y += terrain->GetH(pos);
				}
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(roti != 0)
				{
					cobj.type = CollisionObject::RECTANGLE_ROT;
					cobj.rot = rot;
					cobj.radius = sqrt(cobj.w*cobj.w + cobj.h*cobj.h);
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
				}
			}
			else if(token == "squarevpa")
			{
				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				if(ctx.type == LevelContext::Outside)
					pos.y += terrain->GetH(pos);
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = CG_COLLIDER;
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(roti != 0)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
			}
			else if(token == "squarecam")
			{
				if(ctx.type == LevelContext::Outside)
					pos.y += terrain->GetH(pos);

				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
				if(roti != 0)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

				float w = pt.size.x, h = pt.size.z;
				if(roti == 1 || roti == 3)
					std::swap(w, h);

				CameraCollider& cc = Add1(cam_colliders);
				cc.box.v1.x = pos.x - w;
				cc.box.v2.x = pos.x + w;
				cc.box.v1.z = pos.z - h;
				cc.box.v2.z = pos.z + h;
				cc.box.v1.y = pos.y - pt.size.y;
				cc.box.v2.y = pos.y + pt.size.y;
			}
			else if(token == "xsphere")
			{
				inside->xsphere_pos = pos;
				inside->xsphere_radius = pt.size.x;
			}
			else
				assert(0);
		}
		else if(c == 's')
		{
			// strefa
			if(!recreate)
			{
				if(token == "enter")
				{
					assert(!inside);

					inside = new InsideBuilding;
					inside->level_shift = city->inside_offset;
					inside->offset = Vec2(512.f*city->inside_offset.x + 256.f, 512.f*city->inside_offset.y + 256.f);
					if(city->inside_offset.x > city->inside_offset.y)
					{
						--city->inside_offset.x;
						++city->inside_offset.y;
					}
					else
					{
						city->inside_offset.x += 2;
						city->inside_offset.y = 0;
					}
					float w, h;
					if(roti == 0 || roti == 2)
					{
						w = pt.size.x;
						h = pt.size.z;
					}
					else
					{
						w = pt.size.z;
						h = pt.size.x;
					}
					inside->enter_area.v1.x = pos.x - w;
					inside->enter_area.v1.y = pos.z - h;
					inside->enter_area.v2.x = pos.x + w;
					inside->enter_area.v2.y = pos.z + h;
					Vec2 mid = inside->enter_area.Midpoint();
					inside->enter_y = terrain->GetH(mid.x, mid.y) + 0.1f;
					inside->type = type;
					inside->outside_rot = rot;
					inside->top = -1.f;
					inside->xsphere_radius = -1.f;
					ApplyContext(inside, inside->ctx);
					inside->ctx.building_id = (int)city->inside_buildings.size();

					city->inside_buildings.push_back(inside);

					assert(inside_mesh);

					if(inside_mesh)
					{
						Vec3 o_pos = Vec3(inside->offset.x, 0.f, inside->offset.y);

						Object* o = new Object;
						o->base = nullptr;
						o->mesh = inside_mesh;
						o->pos = o_pos;
						o->rot = Vec3(0, 0, 0);
						o->scale = 1.f;
						o->require_split = true;
						inside->ctx.objects->push_back(o);

						ProcessBuildingObjects(inside->ctx, city, inside, inside_mesh, nullptr, 0.f, 0, o->pos, nullptr, nullptr);
					}

					have_exit = true;
				}
				else if(token == "exit")
				{
					assert(inside);

					inside->exit_area.v1.x = pos.x - pt.size.x;
					inside->exit_area.v1.y = pos.z - pt.size.z;
					inside->exit_area.v2.x = pos.x + pt.size.x;
					inside->exit_area.v2.y = pos.z + pt.size.z;

					have_exit = true;
				}
				else if(token == "spawn")
				{
					if(is_inside)
						inside->inside_spawn = pos;
					else
					{
						spawn_point = pos;
						terrain->SetH(spawn_point);
					}

					have_spawn = true;
				}
				else if(token == "top")
				{
					assert(is_inside);
					inside->top = pos.y;
				}
				else if(token == "door" || token == "doorc" || token == "doorl" || token == "door2")
				{
					Door* door = new Door;
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->state = Door::Open;
					door->door2 = (token == "door2");
					door->mesh_inst = new MeshInstance(door->door2 ? aDoor2 : aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					door->netid = Door::netid_counter++;

					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += 1.319f;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(token != "door") // door2 are closed now, this is intended
					{
						door->state = Door::Closed;
						if(token == "doorl")
							door->locked = 1;
					}
					else
					{
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}

					ctx.doors->push_back(door);
				}
				else if(token == "arena")
				{
					assert(inside);

					inside->arena1.v1.x = pos.x - pt.size.x;
					inside->arena1.v1.y = pos.z - pt.size.z;
					inside->arena1.v2.x = pos.x + pt.size.x;
					inside->arena1.v2.y = pos.z + pt.size.z;
				}
				else if(token == "arena2")
				{
					assert(inside);

					inside->arena2.v1.x = pos.x - pt.size.x;
					inside->arena2.v1.y = pos.z - pt.size.z;
					inside->arena2.v2.x = pos.x + pt.size.x;
					inside->arena2.v2.y = pos.z + pt.size.z;
				}
				else if(token == "viewer")
				{
					// ten punkt jest u¿ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(building)
					{
						building->walk_pt = pos;
						terrain->SetH(building->walk_pt);
					}
					else if(out_point)
						*out_point = pos;
				}
				else
					assert(0);
			}
		}
		else if(c == 'c')
		{
			if(!recreate)
			{
				UnitData* ud = UnitData::TryGet(token.c_str());
				assert(ud);
				if(ud)
				{
					Unit* u = SpawnUnitNearLocation(ctx, pos, *ud, nullptr, -2);
					u->rot = Clip(pt.rot.y + rot);
					u->ai->start_rot = u->rot;
				}
			}
		}
		else if(c == 'm')
		{
			LightMask& mask = Add1(inside->masks);
			mask.size = Vec2(pt.size.x, pt.size.z);
			mask.pos = Vec2(pos.x, pos.z);
		}
	}

	if(!details.empty() && !is_inside)
	{
		int c = Rand() % 80 + details.size() * 2, count;
		if(c < 10)
			count = 0;
		else if(c < 30)
			count = 1;
		else if(c < 60)
			count = 2;
		else if(c < 90)
			count = 3;
		else
			count = 4;
		if(count > 0)
		{
			std::random_shuffle(details.begin(), details.end(), MyRand);
			while(count && !details.empty())
			{
				const Mesh::Point& pt = *details.back();
				details.pop_back();
				--count;

				uint poss = pt.name.find_first_of('_', 4);
				if(poss == string::npos)
				{
					assert(0);
					continue;
				}
				token = pt.name.substr(4, poss - 4);
				for(uint k = 0, len = token.length(); k < len; ++k)
				{
					if(token[k] == '#')
						token[k] = '_';
				}

				Vec3 pos;
				if(roti != 0)
				{
					m1 = Matrix::RotationY(rot);
					m2 = pt.mat * m1;
					pos = Vec3::TransformZero(m2);
				}
				else
					pos = Vec3::TransformZero(pt.mat);
				pos += shift;

				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* obj = BaseObject::TryGet(name);
				assert(obj);

				if(obj)
				{
					if(ctx.type == LevelContext::Outside)
						terrain->SetH(pos);
					SpawnObjectEntity(ctx, obj, pos, Clip(pt.rot.y + rot), 1.f, 0, nullptr, variant);
				}
			}
		}
		details.clear();
	}

	if(!recreate)
	{
		if(is_inside || inside)
			assert(have_exit && have_spawn);

		if(!is_inside && inside)
			inside->outside_spawn = spawn_point;
	}

	if(inside)
		inside->ctx.masks = (!inside->masks.empty() ? &inside->masks : nullptr);
}

void Game::RespawnUnits()
{
	RespawnUnits(local_ctx);
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			RespawnUnits((*it)->ctx);
	}
}

void Game::RespawnUnits(LevelContext& ctx)
{
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit* u = *it;
		if(u->player)
			continue;

		// model
		u->action = A_NONE;
		u->talking = false;
		u->CreateMesh(Unit::CREATE_MESH::NORMAL);

		// fizyka
		CreateUnitPhysics(*u, true);

		// ai
		AIController* ai = new AIController;
		ai->Init(u);
		ais.push_back(ai);
	}
}

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int price_limit, int exclude_flags, uint count = 1)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > price_limit || IS_SET(item->flags, exclude_flags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

void Game::GenerateStockItems()
{
	Location& loc = *L.location;

	assert(loc.type == L_CITY);

	City& city = (City&)loc;
	int price_limit, price_limit2, count_mod;
	bool is_city;

	if(!city.IsVillage())
	{
		price_limit = Random(2000, 2500);
		price_limit2 = 99999;
		count_mod = 0;
		is_city = true;
	}
	else
	{
		price_limit = Random(500, 1000);
		price_limit2 = Random(1250, 2500);
		count_mod = -Random(1, 3);
		is_city = false;
	}

	// merchant
	if(IS_SET(city.flags, City::HaveMerchant))
		GenerateMerchantItems(chest_merchant, price_limit);

	// blacksmith
	if(IS_SET(city.flags, City::HaveBlacksmith))
	{
		chest_blacksmith.clear();
		for(int i = 0, count = Random(12, 18) + count_mod; i < count; ++i)
		{
			switch(Rand() % 4)
			{
			case IT_WEAPON:
				InsertRandomItem(chest_blacksmith, Weapon::weapons, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_BOW:
				InsertRandomItem(chest_blacksmith, Bow::bows, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_SHIELD:
				InsertRandomItem(chest_blacksmith, Shield::shields, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_ARMOR:
				InsertRandomItem(chest_blacksmith, Armor::armors, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			}
		}
		// basic equipment
		Stock::Get("blacksmith")->Parse(5, is_city, chest_blacksmith);
		SortItems(chest_blacksmith);
	}

	// alchemist
	if(IS_SET(city.flags, City::HaveAlchemist))
	{
		chest_alchemist.clear();
		for(int i = 0, count = Random(8, 12) + count_mod; i < count; ++i)
			InsertRandomItem(chest_alchemist, Consumable::consumables, 99999, ITEM_NOT_SHOP | ITEM_NOT_ALCHEMIST, Random(3, 6));
		SortItems(chest_alchemist);
	}

	// innkeeper
	if(IS_SET(city.flags, City::HaveInn))
	{
		chest_innkeeper.clear();
		Stock::Get("innkeeper")->Parse(5, is_city, chest_innkeeper);
		const ItemList* lis2 = ItemList::Get("normal_food").lis;
		for(uint i = 0, count = Random(10, 20) + count_mod; i < count; ++i)
			InsertItemBare(chest_innkeeper, lis2->Get());
		SortItems(chest_innkeeper);
	}

	// food seller
	if(IS_SET(city.flags, City::HaveFoodSeller))
	{
		chest_food_seller.clear();
		const ItemList* lis = ItemList::Get("food_and_drink").lis;
		for(const Item* item : lis->items)
		{
			uint value = Random(50, 100);
			if(!is_city)
				value /= 2;
			InsertItemBare(chest_food_seller, item, value / item->value);
		}
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("frying_pan"));
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("ladle"));
		SortItems(chest_food_seller);
	}
}

void Game::GenerateMerchantItems(vector<ItemSlot>& items, int price_limit)
{
	items.clear();
	InsertItemBare(items, Item::Get("p_nreg"), Random(5, 10));
	InsertItemBare(items, Item::Get("p_hp"), Random(5, 10));
	for(int i = 0, count = Random(15, 20); i < count; ++i)
	{
		switch(Rand() % 6)
		{
		case IT_WEAPON:
			InsertRandomItem(items, Weapon::weapons, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_BOW:
			InsertRandomItem(items, Bow::bows, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_SHIELD:
			InsertRandomItem(items, Shield::shields, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_ARMOR:
			InsertRandomItem(items, Armor::armors, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_CONSUMABLE:
			InsertRandomItem(items, Consumable::consumables, price_limit / 5, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT, Random(2, 5));
			break;
		case IT_OTHER:
			InsertRandomItem(items, OtherItem::others, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		}
	}
	SortItems(items);
}

// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(Net::IsLocal())
	{
		// zawody
		if(QM.quest_tournament->state != Quest_Tournament::TOURNAMENT_NOT_DONE)
			CleanTournament();
		// arena
		if(!arena_free)
			CleanArena();
	}

	if(clear)
	{
		if(L.is_open)
			LeaveLevel(true);
		return;
	}

	if(!L.is_open)
		return;

	Info("Leaving location.");

	pvp_response.ok = false;
	QM.quest_tournament->generated = false;

	if(Net::IsLocal() && (QM.quest_crazies->check_stone
		|| (QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && QM.quest_crazies->crazies_state < Quest_Crazies::State::End)))
		CheckCraziesStone();

	// drinking contest
	Quest_Contest* contest = QM.quest_contest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
	{
		for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.busy = Unit::Busy_No;
			u.look_target = nullptr;
			u.event_handler = nullptr;
		}

		InsideBuilding* inn = city_ctx->FindInn();
		Unit* innkeeper = inn->FindUnit(UnitData::Get("innkeeper"));

		innkeeper->talking = false;
		innkeeper->mesh_inst->need_update = true;
		innkeeper->busy = Unit::Busy_No;
		contest->state = Quest_Contest::CONTEST_DONE;
		contest->units.clear();
		W.AddNews(txContestNoWinner);
	}

	// clear blood & bodies from orc base
	if(Net::IsLocal() && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && L.location_index == QM.quest_orcs2->target_loc)
	{
		QM.quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		UpdateLocation(31, 100, false);
	}

	if(city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// opuszczanie miasta
		BuyTeamItems();
	}

	LeaveLevel();

	if(L.is_open)
	{
		if(Net::IsLocal())
		{
			// usuñ questowe postacie
			RemoveQuestUnits(true);
		}

		L.ProcessRemoveUnits(true);

		if(L.location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = (OutsideLocation*)L.location;
			outside->bloods.clear();
			DeleteElements(outside->objects);
			DeleteElements(outside->chests);
			DeleteElements(outside->items);
			outside->objects.clear();
			DeleteElements(outside->usables);
			for(vector<Unit*>::iterator it = outside->units.begin(), end = outside->units.end(); it != end; ++it)
				delete *it;
			outside->units.clear();
		}
	}

	if(Team.crazies_attack)
	{
		Team.crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	if(!Net::IsLocal())
		pc = nullptr;
	else if(end_buffs)
	{
		// usuñ tymczasowe bufy
		for(Unit* unit : Team.members)
			unit->EndEffects();
	}

	L.is_open = false;
	city_ctx = nullptr;
}

void Game::GenerateDungeonObjects2()
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	// schody w górê
	if(inside->HaveUpStairs())
	{
		Object* o = new Object;
		o->mesh = aStairsUp;
		o->pos = pt_to_pos(lvl.staircase_up);
		o->rot = Vec3(0, dir_to_rot(lvl.staircase_up_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		local_ctx.objects->push_back(o);
	}
	else
		SpawnObjectEntity(local_ctx, BaseObject::Get("portal"), inside->portal->pos, inside->portal->rot);

	// schody w dó³
	if(inside->HaveDownStairs())
	{
		Object* o = new Object;
		o->mesh = (lvl.staircase_down_in_wall ? aStairsDown2 : aStairsDown);
		o->pos = pt_to_pos(lvl.staircase_down);
		o->rot = Vec3(0, dir_to_rot(lvl.staircase_down_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		local_ctx.objects->push_back(o);
	}

	// kratki, drzwi
	for(int y = 0; y < lvl.h; ++y)
	{
		for(int x = 0; x < lvl.w; ++x)
		{
			POLE p = lvl.map[x + y*lvl.w].type;
			if(p == KRATKA || p == KRATKA_PODLOGA)
			{
				Object* o = new Object;
				o->mesh = aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 0, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);
			}
			if(p == KRATKA || p == KRATKA_SUFIT)
			{
				Object* o = new Object;
				o->mesh = aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 4, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);
			}
			if(p == DRZWI)
			{
				Object* o = new Object;
				o->mesh = aDoorWall;
				if(IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_DRUGA_TEKSTURA))
					o->mesh = aDoorWall2;
				o->pos = Vec3(float(x * 2) + 1, 0, float(y * 2) + 1);
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);

				if(czy_blokuje2(lvl.map[x - 1 + y*lvl.w].type))
				{
					o->rot = Vec3(0, 0, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x + (y - 1)*lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + (y + 1)*lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.z += 0.8229f;
					else if(mov == -1)
						o->pos.z -= 0.8229f;
				}
				else
				{
					o->rot = Vec3(0, PI / 2, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x - 1 + y*lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + 1 + y*lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.x += 0.8229f;
					else if(mov == -1)
						o->pos.x -= 0.8229f;
				}

				if(Rand() % 100 < base.door_chance || IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_SPECJALNE))
				{
					Door* door = new Door;
					local_ctx.doors->push_back(door);
					door->pt = Int2(x, y);
					door->pos = o->pos;
					door->rot = o->rot.y;
					door->state = Door::Closed;
					door->mesh_inst = new MeshInstance(aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					door->netid = Door::netid_counter++;
					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += 1.319f;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_SPECJALNE))
						door->locked = LOCK_ORCS;
					else if(Rand() % 100 < base.door_open)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}
				}
				else
					lvl.map[x + y*lvl.w].type = OTWOR_NA_DRZWI;
			}
		}
	}
}

void Game::SpawnCityPhysics()
{
	City* city = (City*)L.location;
	TerrainTile* tiles = city->tiles;

	for(int z = 0; z < City::size; ++z)
	{
		for(int x = 0; x < City::size; ++x)
		{
			if(tiles[x + z*OutsideLocation::size].mode == TM_BUILDING_BLOCK)
			{
				btCollisionObject* cobj = new btCollisionObject;
				cobj->setCollisionShape(shape_block);
				cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
				cobj->getWorldTransform().setOrigin(btVector3(2.f*x + 1.f, terrain->GetH(2.f*x + 1.f, 2.f*x + 1), 2.f*z + 1.f));
				phy_world->addCollisionObject(cobj, CG_BUILDING);
			}
		}
	}
}

void Game::RespawnBuildingPhysics()
{
	for(vector<CityBuilding>::iterator it = city_ctx->buildings.begin(), end = city_ctx->buildings.end(); it != end; ++it)
	{
		Building* b = it->type;

		int r = it->rot;
		if(r == 1)
			r = 3;
		else if(r == 3)
			r = 1;

		ProcessBuildingObjects(local_ctx, city_ctx, nullptr, b->mesh, nullptr, dir_to_rot(r), r,
			Vec3(float(it->pt.x + b->shift[it->rot].x) * 2, 1.f, float(it->pt.y + b->shift[it->rot].y) * 2), nullptr, &*it, true);
	}

	for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
	{
		ProcessBuildingObjects((*it)->ctx, city_ctx, *it, (*it)->type->inside_mesh, nullptr, 0.f, 0, Vec3((*it)->offset.x, 0.f, (*it)->offset.y), nullptr,
			nullptr, true);
	}
}

void Game::RepositionCityUnits()
{
	const int a = int(0.15f*OutsideLocation::size) + 3;
	const int b = int(0.85f*OutsideLocation::size) - 3;

	UnitData* citizen;
	if(city_ctx->IsVillage())
		citizen = UnitData::Get("villager");
	else
		citizen = UnitData::Get("citizen");
	UnitData* guard = UnitData::Get("guard_move");
	InsideBuilding* inn = city_ctx->FindInn();

	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsAlive() && u.IsAI())
		{
			if(u.ai->goto_inn)
				WarpToArea(inn->ctx, (Rand() % 5 == 0 ? inn->arena2 : inn->arena1), u.GetUnitRadius(), u.pos);
			else if(u.data == citizen || u.data == guard)
			{
				for(int j = 0; j < 50; ++j)
				{
					Int2 pt(Random(a, b), Random(a, b));
					if(city_ctx->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
					{
						WarpUnit(u, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1));
						break;
					}
				}
			}
		}
	}
}

void Game::Event_RandomEncounter(int)
{
	dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	W.StartEncounter();
	EnterLocation();
}

void Game::SpawnUnitsGroup(LevelContext& ctx, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback)
{
	static TmpUnitGroup tgroup;
	tgroup.group = group;
	tgroup.Fill(level);

	for(uint i = 0; i < count; ++i)
	{
		int x = Rand() % tgroup.total,
			y = 0;
		for(auto& entry : tgroup.entries)
		{
			y += entry.count;
			if(x < y)
			{
				Unit* u = SpawnUnitNearLocation(ctx, pos, *entry.ud, look_at, Clamp(entry.ud->level.Random(), level / 2, level), 4.f);
				if(u && callback)
					callback(u);
				break;
			}
		}
	}
}

// up³yw czasu
// 1-10 dni (usuñ zw³oki)
// 5-30 dni (usuñ krew)
// 1+ zamknij/otwórz drzwi
template<typename T>
struct RemoveRandomPred
{
	int chance, a, b;

	RemoveRandomPred(int chance, int a, int b)
	{
	}

	bool operator () (const T&)
	{
		return Random(a, b) < chance;
	}
};

void Game::UpdateLocation(LevelContext& ctx, int days, int open_chance, bool reset)
{
	if(days <= 10)
	{
		// usuñ niektóre zw³oki i przedmioty
		for(Unit*& u : *ctx.units)
		{
			if(!u->IsAlive() && Random(4, 10) < days)
			{
				delete u;
				u = nullptr;
			}
		}
		RemoveNullElements(ctx.units);
		auto from = std::remove_if(ctx.items->begin(), ctx.items->end(), RemoveRandomPred<GroundItem*>(days, 0, 10));
		auto end = ctx.items->end();
		if(from != end)
		{
			for(vector<GroundItem*>::iterator it = from; it != end; ++it)
				delete *it;
			ctx.items->erase(from, end);
		}
	}
	else
	{
		// usuñ wszystkie zw³oki i przedmioty
		if(reset)
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
				delete *it;
			ctx.units->clear();
		}
		else
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if(!(*it)->IsAlive())
				{
					delete *it;
					*it = nullptr;
				}
			}
			RemoveNullElements(ctx.units);
		}
		DeleteElements(ctx.items);
	}

	// wylecz jednostki
	if(!reset)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive())
				(*it)->NaturalHealing(days);
		}
	}

	if(days > 30)
	{
		// usuñ krew
		ctx.bloods->clear();
	}
	else if(days >= 5)
	{
		// usuñ czêœciowo krew
		RemoveElements(ctx.bloods, RemoveRandomPred<Blood>(days, 4, 30));
	}

	if(ctx.traps)
	{
		if(days > 30)
		{
			// usuñ wszystkie jednorazowe pu³apki
			for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL)
				{
					delete *it;
					if(it + 1 == end)
					{
						ctx.traps->pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						ctx.traps->pop_back();
						end = ctx.traps->end();
					}
				}
				else
					++it;
			}
		}
		else if(days >= 5)
		{
			// usuñ czêœæ 1razowych pu³apek
			for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL && Rand() % 30 < days)
				{
					delete *it;
					if(it + 1 == end)
					{
						ctx.traps->pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						ctx.traps->pop_back();
						end = ctx.traps->end();
					}
				}
				else
					++it;
			}
		}
	}

	// losowo otwórz/zamknij drzwi
	if(ctx.doors)
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.locked == 0)
			{
				if(Rand() % 100 < open_chance)
					door.state = Door::Open;
				else
					door.state = Door::Closed;
			}
		}
	}
}

void Game::UpdateLocation(int days, int open_chance, bool reset)
{
	UpdateLocation(local_ctx, days, open_chance, reset);

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			UpdateLocation((*it)->ctx, days, open_chance, reset);
	}
}

Game::ObjectEntity Game::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range, float margin, float scale)
{
	bool ok = false;
	if(obj->type == OBJ_CYLINDER)
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, obj->r + margin + range);
		float extra_radius = range / 20;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, pt, obj->r + margin))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

Game::ObjectEntity Game::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range, float margin,
	float scale)
{
	if(obj->type == OBJ_CYLINDER)
		return SpawnObjectNearLocation(ctx, obj, pos, Vec2::LookAtAngle(pos, rot_target), range, margin, scale);
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20, rot;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		bool ok = false;
		for(int i = 0; i < 20; ++i)
		{
			rot = Vec2::LookAtAngle(Vec2(pt.x, pt.z), rot_target);
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

void Game::SpawnTmpUnits(City* city)
{
	InsideBuilding* inn = city->FindInn();
	CityBuilding* training_grounds = city->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS);

	// heroes
	uint count;
	Int2 level;
	if(W.CheckFirstCity())
	{
		count = 4;
		level = Int2(2, 5);
	}
	else
	{
		count = Random(1u, 4u);
		level = Int2(2, 15);
	}

	for(uint i = 0; i < count; ++i)
	{
		UnitData& ud = GetHero(ClassInfo::GetRandom());

		if(Rand() % 2 == 0 || !training_grounds)
		{
			// inside inn
			SpawnUnitInsideInn(ud, level.Random(), inn, true);
		}
		else
		{
			// on training grounds
			Unit* u = SpawnUnitNearLocation(local_ctx, Vec3(2.f*training_grounds->unit_pt.x + 1, 0, 2.f*training_grounds->unit_pt.y + 1), ud, nullptr,
				level.Random(), 8.f);
			if(u)
				u->temporary = true;
		}
	}

	// quest traveler (100% chance in city, 50% in village)
	if(!city_ctx->IsVillage() || Rand() % 2 == 0)
		SpawnUnitInsideInn(*UnitData::Get("traveler"), -2, inn, SU_TEMPORARY);
}

void Game::RemoveTmpUnits(City* city)
{
	RemoveTmpUnits(local_ctx);

	for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it)
		RemoveTmpUnits((*it)->ctx);
}

void Game::RemoveTmpUnits(LevelContext& ctx)
{
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end;)
	{
		Unit* u = *it;
		if(u->temporary)
		{
			delete u;

			if(it + 1 == end)
			{
				ctx.units->pop_back();
				return;
			}
			else
			{
				it = ctx.units->erase(it);
				end = ctx.units->end();
			}
		}
		else
			++it;
	}
}

void Game::SpawnObjectExtras(LevelContext& ctx, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale, int flags)
{
	assert(obj);

	// ogieñ pochodni
	if(!IS_SET(flags, SOE_DONT_SPAWN_PARTICLES))
	{
		if(IS_SET(obj->flags, OBJ_LIGHT))
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 3, -1);
			pe->speed_max = Vec3(1, 4, 1);
			pe->mode = 1;
			pe->Init();
			ctx.pes->push_back(pe);

			pe->tex = tFlare;
			if(IS_SET(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					pe->tex = tFlare2;
			}

			// œwiat³o
			if(!IS_SET(flags, SOE_DONT_CREATE_LIGHT) && ctx.lights)
			{
				Light& s = Add1(ctx.lights);
				s.pos = pe->pos;
				s.range = 5;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					s.color = Vec3(0.8f, 0.8f, 1.f);
				else
					s.color = Vec3(1.f, 0.9f, 0.9f);
			}
		}
		else if(IS_SET(obj->flags, OBJ_BLOOD_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 4, -1);
			pe->speed_max = Vec3(1, 6, 1);
			pe->mode = 0;
			pe->tex = tKrew[BLOOD_RED];
			pe->size = 0.5f;
			pe->Init();
			ctx.pes->push_back(pe);
		}
		else if(IS_SET(obj->flags, OBJ_WATER_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 500;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 3.f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 4;
			pe->spawn_max = 8;
			pe->speed_min = Vec3(-0.6f, 4, -0.6f);
			pe->speed_max = Vec3(0.6f, 7, 0.6f);
			pe->mode = 0;
			pe->tex = tWoda;
			pe->size = 0.05f;
			pe->Init();
			ctx.pes->push_back(pe);
		}
	}

	// fizyka
	if(obj->shape)
	{
		CollisionObject& c = Add1(ctx.colliders);
		c.ptr = user_ptr;

		int group = CG_OBJECT;
		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
			group |= CG_CAMERA_COLLIDER;

		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(obj->shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);

		if(obj->type == OBJ_CYLINDER)
		{
			cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2, pos.z));
			c.type = CollisionObject::SPHERE;
			c.pt = Vec2(pos.x, pos.z);
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			btTransform& tr = cobj->getWorldTransform();
			m1 = Matrix::RotationY(rot);
			m2 = *obj->matrix * m1;
			Vec3 pos2 = Vec3::TransformZero(m2);
			pos2 += pos;
			tr.setOrigin(ToVector3(pos2));
			tr.setRotation(btQuaternion(rot, 0, 0));

			c.pt = Vec2(pos2.x, pos2.z);
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
		else
		{
			m1 = Matrix::RotationY(rot);
			m2 = Matrix::Translation(pos);
			// skalowanie jakimœ sposobem przechodzi do btWorldTransform i przy rysowaniu jest z³a skala (dwukrotnie u¿yta)
			m3 = Matrix::Scale(1.f / obj->size.x, 1.f, 1.f / obj->size.y);
			m3 = m3 * *obj->matrix * m1 * m2;
			cobj->getWorldTransform().setFromOpenGLMatrix(&m3._11);
			Vec3 out_pos = Vec3::TransformZero(m3);
			Quat q = Quat::CreateFromRotationMatrix(m3);

			float yaw = asin(-2 * (q.x*q.z - q.w*q.y));
			c.pt = Vec2(out_pos.x, out_pos.z);
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(yaw))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = yaw;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}

		phy_world->addCollisionObject(cobj, group);

		if(IS_SET(obj->flags, OBJ_PHYSICS_PTR))
		{
			assert(user_ptr);
			cobj->setUserPointer(user_ptr);
		}

		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
			c.ptr = CAM_COLLIDER;

		if(IS_SET(obj->flags, OBJ_DOUBLE_PHYSICS))
			SpawnObjectExtras(ctx, obj->next_obj, pos, rot, user_ptr, scale, flags);
		else if(IS_SET(obj->flags, OBJ_MULTI_PHYSICS))
		{
			for(int i = 0;; ++i)
			{
				if(obj->next_obj[i].shape)
					SpawnObjectExtras(ctx, &obj->next_obj[i], pos, rot, user_ptr, scale, flags);
				else
					break;
			}
		}
	}
	else if(IS_SET(obj->flags, OBJ_SCALEABLE))
	{
		CollisionObject& c = Add1(ctx.colliders);
		c.type = CollisionObject::SPHERE;
		c.pt = Vec2(pos.x, pos.z);
		c.radius = obj->r*scale;

		btCollisionObject* cobj = new btCollisionObject;
		btCylinderShape* shape = new btCylinderShape(btVector3(obj->r*scale, obj->h*scale, obj->r*scale));
		shapes.push_back(shape);
		cobj->setCollisionShape(shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);
		cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2 * scale, pos.z));
		phy_world->addCollisionObject(cobj, CG_OBJECT);
	}

	if(IS_SET(obj->flags, OBJ_CAM_COLLIDERS))
	{
		int roti = (int)round((rot / (PI / 2)));
		for(vector<Mesh::Point>::const_iterator it = obj->mesh->attach_points.begin(), end = obj->mesh->attach_points.end(); it != end; ++it)
		{
			const Mesh::Point& pt = *it;
			if(strncmp(pt.name.c_str(), "camcol", 6) != 0)
				continue;

			m2 = pt.mat * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(m2) + pos;

			btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
			shapes.push_back(shape);
			btCollisionObject* co = new btCollisionObject;
			co->setCollisionShape(shape);
			co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
			co->getWorldTransform().setOrigin(ToVector3(pos2));
			phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
			if(roti != 0)
				co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

			float w = pt.size.x, h = pt.size.z;
			if(roti == 1 || roti == 3)
				std::swap(w, h);

			CameraCollider& cc = Add1(cam_colliders);
			cc.box.v1.x = pos2.x - w;
			cc.box.v2.x = pos2.x + w;
			cc.box.v1.z = pos2.z - h;
			cc.box.v2.z = pos2.z + h;
			cc.box.v1.y = pos2.y - pt.size.y;
			cc.box.v2.y = pos2.y + pt.size.y;
		}
	}
}

namespace PickableItem
{
	struct Item
	{
		uint spawn;
		Vec3 pos;
	};
	LevelContext* ctx;
	Object* o;
	vector<Box> spawns;
	vector<Item> items;
}

void Game::PickableItemBegin(LevelContext& ctx, Object& o)
{
	assert(o.base);

	PickableItem::ctx = &ctx;
	PickableItem::o = &o;
	PickableItem::spawns.clear();
	PickableItem::items.clear();

	for(vector<Mesh::Point>::iterator it = o.base->mesh->attach_points.begin(), end = o.base->mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			assert(it->type == Mesh::Point::Box);
			Box box(it->mat._41, it->mat._42, it->mat._43);
			box.v1.x -= it->size.x - 0.05f;
			box.v1.z -= it->size.z - 0.05f;
			box.v2.x += it->size.x - 0.05f;
			box.v2.z += it->size.z - 0.05f;
			box.v1.y = box.v2.y = box.v1.y - it->size.y;
			PickableItem::spawns.push_back(box);
		}
	}

	assert(!PickableItem::spawns.empty());
}

void Game::PickableItemAdd(const Item* item)
{
	assert(item);

	for(int i = 0; i < 20; ++i)
	{
		// pobierz punkt
		uint spawn = Rand() % PickableItem::spawns.size();
		Box& box = PickableItem::spawns[spawn];
		// ustal pozycjê
		Vec3 pos(Random(box.v1.x, box.v2.x), box.v1.y, Random(box.v1.z, box.v2.z));
		// sprawdŸ kolizjê
		bool ok = true;
		for(vector<PickableItem::Item>::iterator it = PickableItem::items.begin(), end = PickableItem::items.end(); it != end; ++it)
		{
			if(it->spawn == spawn)
			{
				if(CircleToCircle(pos.x, pos.z, 0.15f, it->pos.x, it->pos.z, 0.15f))
				{
					ok = false;
					break;
				}
			}
		}
		// dodaj
		if(ok)
		{
			PickableItem::Item& i = Add1(PickableItem::items);
			i.spawn = spawn;
			i.pos = pos;

			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			gi->rot = Random(MAX_ANGLE);
			float rot = PickableItem::o->rot.y,
				s = sin(rot),
				c = cos(rot);
			gi->pos = Vec3(pos.x*c + pos.z*s, pos.y, -pos.x*s + pos.z*c) + PickableItem::o->pos;
			PickableItem::ctx->items->push_back(gi);

			break;
		}
	}
}
