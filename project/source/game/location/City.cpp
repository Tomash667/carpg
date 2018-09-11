#include "Pch.h"
#include "GameCore.h"
#include "City.h"
#include "SaveState.h"
#include "Content.h"
#include "ResourceManager.h"
#include "Object.h"
#include "Unit.h"
#include "GameFile.h"
#include "BuildingScript.h"
#include "World.h"
#include "Level.h"
#include "BitStreamFunc.h"

//=================================================================================================
City::~City()
{
	DeleteElements(inside_buildings);
}

//=================================================================================================
void City::Save(GameWriter& f, bool local)
{
	OutsideLocation::Save(f, local);

	f << citizens;
	f << citizens_world;
	f << settlement_type;
	f << flags;
	f << variant;

	if(last_visit == -1)
	{
		// list of buildings in this location is generated
		f << buildings.size();
		for(CityBuilding& b : buildings)
			f << b.type->id;
	}
	else
	{
		f << entry_points;
		f << gates;

		f << buildings.size();
		for(CityBuilding& b : buildings)
		{
			f << b.type->id;
			f << b.pt;
			f << b.unit_pt;
			f << b.rot;
			f << b.walk_pt;
		}

		f << inside_offset;
		f << inside_buildings.size();
		for(InsideBuilding* b : inside_buildings)
			b->Save(f, local);

		f << quest_mayor;
		f << quest_mayor_time;
		f << quest_captain;
		f << quest_captain_time;
		f << arena_time;
		f << arena_pos;
	}
}

//=================================================================================================
void City::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	OutsideLocation::Load(f, local, token);

	f >> citizens;
	f >> citizens_world;

	if(LOAD_VERSION >= V_0_5)
	{
		f >> settlement_type;
		f >> flags;
		f >> variant;

		if(last_visit == -1)
		{
			// list of buildings in this location is generated
			uint count;
			f >> count;
			buildings.resize(count);
			for(CityBuilding& b : buildings)
			{
				b.type = Building::Get(f.ReadString1());
				assert(b.type != nullptr);
			}
		}
		else
		{
			f >> entry_points;
			f >> gates;

			uint count;
			f >> count;
			buildings.resize(count);
			for(CityBuilding& b : buildings)
			{
				b.type = Building::Get(f.ReadString1());
				f >> b.pt;
				f >> b.unit_pt;
				f >> b.rot;
				f >> b.walk_pt;
				assert(b.type != nullptr);
			}

			f >> inside_offset;
			f >> count;
			inside_buildings.resize(count);
			int index = 0;
			for(InsideBuilding*& b : inside_buildings)
			{
				b = new InsideBuilding;
				b->Load(f, local);
				b->ctx.building_id = index;
				b->ctx.mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + Int2(256, 256);
				++index;
			}

			f >> quest_mayor;
			f >> quest_mayor_time;
			f >> quest_captain;
			f >> quest_captain_time;
			f >> arena_time;
			f >> arena_pos;
		}
	}
	else
	{
		if(token == LT_CITY)
			settlement_type = SettlementType::City;
		else
		{
			settlement_type = SettlementType::Village;
			image = LI_VILLAGE;
		}
		flags = 0;
		variant = 0;

		if(last_visit != -1)
		{
			int side;
			Box2d spawn_area;
			Box2d exit_area;
			float spawn_dir;

			if(LOAD_VERSION < V_0_3)
			{
				f >> side;
				f >> spawn_area;
				f >> exit_area;
				f >> spawn_dir;
			}
			else
			{
				f.ReadVector<byte>(entry_points);
				bool have_exit;
				f >> have_exit;
				gates = f.Read<byte>();

				if(have_exit)
					flags |= HaveExit;
			}

			uint count;
			f >> count;
			buildings.resize(count);
			for(CityBuilding& b : buildings)
			{
				OLD_BUILDING type;
				f >> type;
				f >> b.pt;
				f >> b.unit_pt;
				f >> b.rot;
				f >> b.walk_pt;
				b.type = Building::GetOld(type);
				assert(b.type != nullptr);
			}

			f >> inside_offset;
			f >> count;
			inside_buildings.resize(count);
			int index = 0;
			for(InsideBuilding*& b : inside_buildings)
			{
				b = new InsideBuilding;
				b->Load(f, local);
				b->ctx.building_id = index;
				b->ctx.mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + Int2(256, 256);
				++index;
			}

			f >> quest_mayor;
			f >> quest_mayor_time;
			f >> quest_captain;
			f >> quest_captain_time;
			f >> arena_time;
			f >> arena_pos;

			if(LOAD_VERSION < V_0_3)
			{
				const float aa = 11.1f;
				const float bb = 12.6f;
				const float es = 1.3f;
				const int mur1 = int(0.15f*size);
				const int mur2 = int(0.85f*size);
				TERRAIN_TILE road_type;

				// setup entrance
				switch(side)
				{
				case 0: // from top
					{
						Vec2 p(float(size) + 1.f, 0.8f*size * 2);
						exit_area = Box2d(p.x - es, p.y + aa, p.x + es, p.y + bb);
						gates = GATE_NORTH;
						road_type = tiles[size / 2 + int(0.85f*size - 2)*size].t;
					}
					break;
				case 1: // from left
					{
						Vec2 p(0.2f*size * 2, float(size) + 1.f);
						exit_area = Box2d(p.x - bb, p.y - es, p.x - aa, p.y + es);
						gates = GATE_WEST;
						road_type = tiles[int(0.15f*size) + 2 + (size / 2)*size].t;
					}
					break;
				case 2: // from bottom
					{
						Vec2 p(float(size) + 1.f, 0.2f*size * 2);
						exit_area = Box2d(p.x - es, p.y - bb, p.x + es, p.y - aa);
						gates = GATE_SOUTH;
						road_type = tiles[size / 2 + int(0.15f*size + 2)*size].t;
					}
					break;
				case 3: // from right
					{
						Vec2 p(0.8f*size * 2, float(size) + 1.f);
						exit_area = Box2d(p.x + aa, p.y - es, p.x + bb, p.y + es);
						gates = GATE_EAST;
						road_type = tiles[int(0.85f*size) - 2 + (size / 2)*size].t;
					}
					break;
				}

				// update terrain tiles
				// tiles under walls
				for(int i = mur1; i <= mur2; ++i)
				{
					// north
					tiles[i + mur1*size].Set(TT_SAND, TM_BUILDING);
					if(tiles[i + (mur1 + 1)*size].t == TT_GRASS)
						tiles[i + (mur1 + 1)*size].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
					// south
					tiles[i + mur2*size].Set(TT_SAND, TM_BUILDING);
					if(tiles[i + (mur2 - 1)*size].t == TT_GRASS)
						tiles[i + (mur2 - 1)*size].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
					// west
					tiles[mur1 + i*size].Set(TT_SAND, TM_BUILDING);
					if(tiles[mur1 + 1 + i*size].t == TT_GRASS)
						tiles[mur1 + 1 + i*size].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
					// east
					tiles[mur2 + i*size].Set(TT_SAND, TM_BUILDING);
					if(tiles[mur2 - 1 + i*size].t == TT_GRASS)
						tiles[mur2 - 1 + i*size].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
				}

				// tiles under gates
				if(gates == GATE_SOUTH)
				{
					tiles[size / 2 - 1 + int(0.15f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + int(0.15f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + 1 + int(0.15f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 - 1 + (int(0.15f*size) + 1)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + (int(0.15f*size) + 1)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + 1 + (int(0.15f*size) + 1)*size].Set(road_type, TM_ROAD);
				}
				if(gates == GATE_WEST)
				{
					tiles[int(0.15f*size) + (size / 2 - 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.15f*size) + (size / 2)*size].Set(road_type, TM_ROAD);
					tiles[int(0.15f*size) + (size / 2 + 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.15f*size) + 1 + (size / 2 - 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.15f*size) + 1 + (size / 2)*size].Set(road_type, TM_ROAD);
					tiles[int(0.15f*size) + 1 + (size / 2 + 1)*size].Set(road_type, TM_ROAD);
				}
				if(gates == GATE_NORTH)
				{
					tiles[size / 2 - 1 + int(0.85f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + int(0.85f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + 1 + int(0.85f*size)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 - 1 + (int(0.85f*size) - 1)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + (int(0.85f*size) - 1)*size].Set(road_type, TM_ROAD);
					tiles[size / 2 + 1 + (int(0.85f*size) - 1)*size].Set(road_type, TM_ROAD);
				}
				if(gates == GATE_EAST)
				{
					tiles[int(0.85f*size) + (size / 2 - 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.85f*size) + (size / 2)*size].Set(road_type, TM_ROAD);
					tiles[int(0.85f*size) + (size / 2 + 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.85f*size) - 1 + (size / 2 - 1)*size].Set(road_type, TM_ROAD);
					tiles[int(0.85f*size) - 1 + (size / 2)*size].Set(road_type, TM_ROAD);
					tiles[int(0.85f*size) - 1 + (size / 2 + 1)*size].Set(road_type, TM_ROAD);
				}

				// delete old walls
				BaseObject* to_remove = BaseObject::Get("to_remove");
				LoopAndRemove(objects, [to_remove](const Object* obj)
				{
					if(obj->base == to_remove)
					{
						delete obj;
						return true;
					}
					return false;
				});

				// add new buildings
				BaseObject* oWall = BaseObject::Get("wall"),
					*oTower = BaseObject::Get("tower");

				const int mid = int(0.5f*size);

				// walls
				for(int i = mur1; i < mur2; i += 3)
				{
					// top
					if(side != 2 || i < mid - 1 || i > mid)
					{
						Object* o = new Object;
						o->pos = Vec3(float(i) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
						o->rot = Vec3(0, PI, 0);
						o->scale = 1.f;
						o->base = oWall;
						o->mesh = oWall->mesh;
						objects.push_back(o);
					}

					// bottom
					if(side != 0 || i < mid - 1 || i > mid)
					{
						Object* o = new Object;
						o->pos = Vec3(float(i) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
						o->rot = Vec3(0, 0, 0);
						o->scale = 1.f;
						o->base = oWall;
						o->mesh = oWall->mesh;
						objects.push_back(o);
					}

					// left
					if(side != 1 || i < mid - 1 || i > mid)
					{
						Object* o = new Object;
						o->pos = Vec3(int(0.15f*size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
						o->rot = Vec3(0, PI * 3 / 2, 0);
						o->scale = 1.f;
						o->base = oWall;
						o->mesh = oWall->mesh;
						objects.push_back(o);
					}

					// right
					if(side != 3 || i < mid - 1 || i > mid)
					{
						Object* o = new Object;
						o->pos = Vec3(int(0.85f*size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
						o->rot = Vec3(0, PI / 2, 0);
						o->scale = 1.f;
						o->base = oWall;
						o->mesh = oWall->mesh;
						objects.push_back(o);
					}
				}

				// towers
				{
					// right top
					Object* o = new Object;
					o->pos = Vec3(int(0.85f*size) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
					o->rot = Vec3(0, 0, 0);
					o->scale = 1.f;
					o->base = oTower;
					o->mesh = oTower->mesh;
					objects.push_back(o);
				}
				{
					// right bottom
					Object* o = new Object;
					o->pos = Vec3(int(0.85f*size) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
					o->rot = Vec3(0, PI / 2, 0);
					o->scale = 1.f;
					o->base = oTower;
					o->mesh = oTower->mesh;
					objects.push_back(o);
				}
				{
					// left bottom
					Object* o = new Object;
					o->pos = Vec3(int(0.15f*size) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
					o->rot = Vec3(0, PI, 0);
					o->scale = 1.f;
					o->base = oTower;
					o->mesh = oTower->mesh;
					objects.push_back(o);
				}
				{
					// left top
					Object* o = new Object;
					o->pos = Vec3(int(0.15f*size) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
					o->rot = Vec3(0, PI * 3 / 2, 0);
					o->scale = 1.f;
					o->base = oTower;
					o->mesh = oTower->mesh;
					objects.push_back(o);
				}

				// gate
				Object* o = new Object;
				o->rot.x = o->rot.z = 0.f;
				o->scale = 1.f;
				o->base = BaseObject::Get("gate");
				o->mesh = o->base->mesh;
				switch(side)
				{
				case 0:
					o->rot.y = 0;
					o->pos = Vec3(0.5f*size * 2 + 1.f, 1.f, 0.85f*size * 2);
					break;
				case 1:
					o->rot.y = PI * 3 / 2;
					o->pos = Vec3(0.15f*size * 2, 1.f, 0.5f*size * 2 + 1.f);
					break;
				case 2:
					o->rot.y = PI;
					o->pos = Vec3(0.5f*size * 2 + 1.f, 1.f, 0.15f*size * 2);
					break;
				case 3:
					o->rot.y = PI / 2;
					o->pos = Vec3(0.85f*size * 2, 1.f, 0.5f*size * 2 + 1.f);
					break;
				}
				objects.push_back(o);

				// grate
				Object* o2 = new Object;
				o2->pos = o->pos;
				o2->rot = o->rot;
				o2->scale = 1.f;
				o2->base = BaseObject::Get("grate");
				o2->mesh = o2->base->mesh;
				objects.push_back(o2);

				// exit
				EntryPoint& entry = Add1(entry_points);
				entry.spawn_area = spawn_area;
				entry.spawn_rot = spawn_dir;
				entry.exit_area = exit_area;
				entry.exit_y = 1.1f;
			}
		}

		if(token == LT_VILLAGE_OLD)
		{
			OLD_BUILDING v_buildings[2];
			f >> v_buildings;

			if(LOAD_VERSION <= V_0_3 && v_buildings[1] == OLD_BUILDING::B_COTTAGE)
				v_buildings[1] = OLD_BUILDING::B_NONE;

			// fix wrong village house building
			if(last_visit != -1 && LOAD_VERSION < V_0_4)
			{
				bool need_fix = false;
				Building* village_hall = Building::GetOld(OLD_BUILDING::B_VILLAGE_HALL);

				if(LOAD_VERSION < V_0_3)
					need_fix = true;
				else if(LOAD_VERSION == V_0_3)
				{
					InsideBuilding* b = FindInsideBuilding(village_hall);
					// easiest way to find out if it uses old mesh
					if(b->top > 3.5f)
						need_fix = true;
				}

				if(need_fix)
				{
					Building* village_hall_old = Building::GetOld(OLD_BUILDING::B_VILLAGE_HALL_OLD);
					FindBuilding(village_hall)->type = village_hall_old;
					for(Object* obj : objects)
					{
						if(strcmp(obj->mesh->filename, "soltys.qmsh") == 0)
						{
							obj->mesh = ResourceManager::Get<Mesh>().GetLoaded("soltys_old.qmsh");
							break;
						}
					}
					InsideBuilding* b = FindInsideBuilding(village_hall);
					b->type = village_hall_old;
					for(Object* obj : b->objects)
					{
						if(strcmp(obj->mesh->filename, "soltys_srodek.qmsh") == 0)
						{
							obj->mesh = ResourceManager::Get<Mesh>().GetLoaded("soltys_srodek_old.qmsh");
							break;
						}
					}
				}
			}

			if(state == LS_KNOWN)
			{
				buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_VILLAGE_HALL)));
				buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_MERCHANT)));
				buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_FOOD_SELLER)));
				buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_VILLAGE_INN)));
				if(v_buildings[0] != OLD_BUILDING::B_NONE)
					buildings.push_back(CityBuilding(Building::GetOld(v_buildings[0])));
				if(v_buildings[1] != OLD_BUILDING::B_NONE)
					buildings.push_back(CityBuilding(Building::GetOld(v_buildings[1])));
				std::random_shuffle(buildings.begin() + 1, buildings.end(), MyRand);
				buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_BARRACKS)));

				flags |= HaveInn | HaveMerchant | HaveFoodSeller;
				if(v_buildings[0] == OLD_BUILDING::B_TRAINING_GROUNDS || v_buildings[1] == OLD_BUILDING::B_TRAINING_GROUNDS)
					flags |= HaveTrainingGrounds;
				if(v_buildings[0] == OLD_BUILDING::B_BLACKSMITH || v_buildings[1] == OLD_BUILDING::B_BLACKSMITH)
					flags |= HaveBlacksmith;
				if(v_buildings[0] == OLD_BUILDING::B_ALCHEMIST || v_buildings[1] == OLD_BUILDING::B_ALCHEMIST)
					flags |= HaveAlchemist;
			}
		}
		else if(state == LS_KNOWN)
		{
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_CITY_HALL)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_ARENA)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_MERCHANT)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_FOOD_SELLER)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_BLACKSMITH)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_ALCHEMIST)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_INN)));
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_TRAINING_GROUNDS)));
			std::random_shuffle(buildings.begin() + 2, buildings.end(), MyRand);
			buildings.push_back(CityBuilding(Building::GetOld(OLD_BUILDING::B_BARRACKS)));

			flags |= HaveTrainingGrounds | HaveArena | HaveMerchant | HaveFoodSeller | HaveBlacksmith | HaveAlchemist | HaveInn;
		}
	}
}

//=================================================================================================
void City::Write(BitStreamWriter& f)
{
	OutsideLocation::Write(f);

	f.WriteCasted<byte>(flags);
	f.WriteCasted<byte>(entry_points.size());
	for(EntryPoint& entry_point : entry_points)
	{
		f << entry_point.exit_area;
		f << entry_point.exit_y;
	}
	f.WriteCasted<byte>(buildings.size());
	for(CityBuilding& building : buildings)
	{
		f << building.type->id;
		f << building.pt;
		f.WriteCasted<byte>(building.rot);
	}
	f.WriteCasted<byte>(inside_buildings.size());
	for(InsideBuilding* inside_building : inside_buildings)
		inside_building->Write(f);
}

//=================================================================================================
bool City::Read(BitStreamReader& f)
{
	OutsideLocation::Read(f);

	// entry points
	const int ENTRY_POINT_MIN_SIZE = 20;
	byte count;
	f.ReadCasted<byte>(flags);
	f >> count;
	if(!f.Ensure(count * ENTRY_POINT_MIN_SIZE))
	{
		Error("Read level: Broken packet for city.");
		return false;
	}
	entry_points.resize(count);
	for(EntryPoint& entry : entry_points)
	{
		f.Read(entry.exit_area);
		f.Read(entry.exit_y);
	}
	if(!f)
	{
		Error("Read level: Broken packet for entry points.");
		return false;
	}

	// buildings
	const int BUILDING_MIN_SIZE = 10;
	f >> count;
	if(!f.Ensure(BUILDING_MIN_SIZE * count))
	{
		Error("Read level: Broken packet for buildings count.");
		return false;
	}
	buildings.resize(count);
	for(CityBuilding& building : buildings)
	{
		const string& building_id = f.ReadString1();
		f >> building.pt;
		f.ReadCasted<byte>(building.rot);
		if(!f)
		{
			Error("Read level: Broken packet for buildings.");
			return false;
		}
		building.type = Building::Get(building_id);
		if(!building.type)
		{
			Error("Read level: Invalid building id '%s'.", building_id.c_str());
			return false;
		}
	}

	// inside buildings
	const int INSIDE_BUILDING_MIN_SIZE = 73;
	f >> count;
	if(!f.Ensure(INSIDE_BUILDING_MIN_SIZE * count))
	{
		Error("Read level: Broken packet for inside buildings count.");
		return false;
	}
	inside_buildings.resize(count);
	int index = 0;
	for(InsideBuilding*& ib : inside_buildings)
	{
		ib = new InsideBuilding;
		L.ApplyContext(ib, ib->ctx);
		ib->ctx.building_id = index;
		if(!ib->Load(f))
		{
			Error("Read level: Failed to loading inside building %d.", index);
			return false;
		}
		++index;
	}

	return true;
}

//=================================================================================================
void City::BuildRefidTables()
{
	OutsideLocation::BuildRefidTables();

	for(vector<InsideBuilding*>::iterator it2 = inside_buildings.begin(), end2 = inside_buildings.end(); it2 != end2; ++it2)
	{
		for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
		{
			(*it)->refid = (int)Unit::refid_table.size();
			Unit::refid_table.push_back(*it);
		}

		for(vector<Usable*>::iterator it = (*it2)->usables.begin(), end = (*it2)->usables.end(); it != end; ++it)
		{
			(*it)->refid = (int)Usable::refid_table.size();
			Usable::refid_table.push_back(*it);
		}
	}
}

//=================================================================================================
bool City::FindUnit(Unit* unit, int* level)
{
	assert(unit);

	for(Unit* u : units)
	{
		if(u == unit)
		{
			if(level)
				*level = -1;
			return true;
		}
	}

	for(uint i = 0; i < inside_buildings.size(); ++i)
	{
		for(Unit* u : inside_buildings[i]->units)
		{
			if(u == unit)
			{
				if(level)
					*level = i;
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
Unit* City::FindUnit(UnitData* data, int& at_level)
{
	assert(data);

	for(Unit* u : units)
	{
		if(u->data == data)
		{
			at_level = -1;
			return u;
		}
	}

	for(uint i = 0; i < inside_buildings.size(); ++i)
	{
		for(Unit* u : inside_buildings[i]->units)
		{
			if(u->data == data)
			{
				at_level = i;
				return u;
			}
		}
	}

	return nullptr;
}

//=================================================================================================
bool City::IsInsideCity(const Vec3& _pos)
{
	Int2 tile(int(_pos.x / 2), int(_pos.z / 2));
	if(tile.x <= int(0.15f*size) || tile.y <= int(0.15f*size) || tile.x >= int(0.85f*size) || tile.y >= int(0.85f*size))
		return false;
	else
		return true;
}

//=================================================================================================
InsideBuilding* City::FindInsideBuilding(Building* type)
{
	assert(type);
	for(InsideBuilding* i : inside_buildings)
	{
		if(i->type == type)
			return i;
	}
	return nullptr;
}

//=================================================================================================
InsideBuilding* City::FindInsideBuilding(BuildingGroup* group)
{
	assert(group);
	for(InsideBuilding* i : inside_buildings)
	{
		if(i->type->group == group)
			return i;
	}
	return nullptr;
}

//=================================================================================================
InsideBuilding* City::FindInsideBuilding(BuildingGroup* group, int& index)
{
	assert(group);
	index = 0;
	for(InsideBuilding* i : inside_buildings)
	{
		if(i->type->group == group)
			return i;
		++index;
	}
	index = -1;
	return nullptr;
}

//=================================================================================================
CityBuilding* City::FindBuilding(BuildingGroup* group)
{
	assert(group);
	for(CityBuilding& b : buildings)
	{
		if(b.type->group == group)
			return &b;
	}
	return nullptr;
}

//=================================================================================================
CityBuilding* City::FindBuilding(Building* type)
{
	assert(type);
	for(CityBuilding& b : buildings)
	{
		if(b.type == type)
			return &b;
	}
	return nullptr;
}

//=================================================================================================
void City::GenerateCityBuildings(vector<Building*>& buildings, bool required)
{
	BuildingScript* script = BuildingScript::Get(IsVillage() ? "village" : "city");
	if(variant == -1)
		variant = Rand() % script->variants.size();

	BuildingScript::Variant* v = script->variants[variant];
	int* code = v->code.data();
	int* end = code + v->code.size();
	for(uint i = 0; i < BuildingScript::MAX_VARS; ++i)
		script->vars[i] = 0;
	script->vars[BuildingScript::V_COUNT] = 1;
	script->vars[BuildingScript::V_CITIZENS] = citizens;
	script->vars[BuildingScript::V_CITIZENS_WORLD] = citizens_world;
	if(!required)
		code += script->required_offset;

	vector<int> stack;
	int if_level = 0, if_depth = 0;
	int shuffle_start = -1;
	int& building_count = script->vars[BuildingScript::V_COUNT];

	while(code != end)
	{
		BuildingScript::Code c = (BuildingScript::Code)*code++;
		switch(c)
		{
		case BuildingScript::BS_ADD_BUILDING:
			if(if_level == if_depth)
			{
				BuildingScript::Code type = (BuildingScript::Code)*code++;
				if(type == BuildingScript::BS_BUILDING)
				{
					Building* b = (Building*)*code++;
					for(int i = 0; i < building_count; ++i)
						buildings.push_back(b);
				}
				else
				{
					BuildingGroup* bg = (BuildingGroup*)*code++;
					for(int i = 0; i < building_count; ++i)
						buildings.push_back(random_item(bg->buildings));
				}
			}
			else
				code += 2;
			break;
		case BuildingScript::BS_RANDOM:
			{
				uint count = (uint)*code++;
				if(if_level != if_depth)
				{
					code += count * 2;
					break;
				}

				for(int i = 0; i < building_count; ++i)
				{
					uint index = Rand() % count;
					BuildingScript::Code type = (BuildingScript::Code)*(code + index * 2);
					if(type == BuildingScript::BS_BUILDING)
					{
						Building* b = (Building*)*(code + index * 2 + 1);
						buildings.push_back(b);
					}
					else
					{
						BuildingGroup* bg = (BuildingGroup*)*(code + index * 2 + 1);
						buildings.push_back(random_item(bg->buildings));
					}
				}

				code += count * 2;
			}
			break;
		case BuildingScript::BS_SHUFFLE_START:
			if(if_level == if_depth)
				shuffle_start = (int)buildings.size();
			break;
		case BuildingScript::BS_SHUFFLE_END:
			if(if_level == if_depth)
			{
				int new_pos = (int)buildings.size();
				if(new_pos - shuffle_start >= 2)
					std::random_shuffle(buildings.begin() + shuffle_start, buildings.end(), MyRand);
				shuffle_start = -1;
			}
			break;
		case BuildingScript::BS_REQUIRED_OFF:
			if(required)
				goto cleanup;
			break;
		case BuildingScript::BS_PUSH_INT:
			if(if_level == if_depth)
				stack.push_back(*code++);
			else
				++code;
			break;
		case BuildingScript::BS_PUSH_VAR:
			if(if_level == if_depth)
				stack.push_back(script->vars[*code++]);
			else
				++code;
			break;
		case BuildingScript::BS_SET_VAR:
			if(if_level == if_depth)
			{
				script->vars[*code++] = stack.back();
				stack.pop_back();
			}
			else
				++code;
			break;
		case BuildingScript::BS_INC:
			if(if_level == if_depth)
				++script->vars[*code++];
			else
				++code;
			break;
		case BuildingScript::BS_DEC:
			if(if_level == if_depth)
				--script->vars[*code++];
			else
				++code;
			break;
		case BuildingScript::BS_IF:
			if(if_level == if_depth)
			{
				BuildingScript::Code op = (BuildingScript::Code)*code++;
				int right = stack.back();
				stack.pop_back();
				int left = stack.back();
				stack.pop_back();
				bool ok = false;
				switch(op)
				{
				case BuildingScript::BS_EQUAL:
					ok = (left == right);
					break;
				case BuildingScript::BS_NOT_EQUAL:
					ok = (left != right);
					break;
				case BuildingScript::BS_GREATER:
					ok = (left > right);
					break;
				case BuildingScript::BS_GREATER_EQUAL:
					ok = (left >= right);
					break;
				case BuildingScript::BS_LESS:
					ok = (left < right);
					break;
				case BuildingScript::BS_LESS_EQUAL:
					ok = (left <= right);
					break;
				}
				++if_level;
				if(ok)
					++if_depth;
			}
			else
			{
				++code;
				++if_level;
			}
			break;
		case BuildingScript::BS_IF_RAND:
			if(if_level == if_depth)
			{
				int a = stack.back();
				stack.pop_back();
				if(a > 0 && Rand() % a == 0)
					++if_depth;
			}
			++if_level;
			break;
		case BuildingScript::BS_ELSE:
			if(if_level == if_depth)
				--if_depth;
			break;
		case BuildingScript::BS_ENDIF:
			if(if_level == if_depth)
				--if_depth;
			--if_level;
			break;
		case BuildingScript::BS_CALL:
		case BuildingScript::BS_ADD:
		case BuildingScript::BS_SUB:
		case BuildingScript::BS_MUL:
		case BuildingScript::BS_DIV:
			if(if_level == if_depth)
			{
				int b = stack.back();
				stack.pop_back();
				int a = stack.back();
				stack.pop_back();
				int result = 0;
				switch(c)
				{
				case BuildingScript::BS_CALL:
					if(a == b)
						result = a;
					else if(b > a)
						result = Random(a, b);
					break;
				case BuildingScript::BS_ADD:
					result = a + b;
					break;
				case BuildingScript::BS_SUB:
					result = a - b;
					break;
				case BuildingScript::BS_MUL:
					result = a * b;
					break;
				case BuildingScript::BS_DIV:
					if(b != 0)
						result = a / b;
					break;
				}
				stack.push_back(result);
			}
			break;
		case BuildingScript::BS_NEG:
			if(if_level == if_depth)
				stack.back() = -stack.back();
			break;
		}
	}

cleanup:
	citizens = script->vars[BuildingScript::V_CITIZENS];
	citizens_world = script->vars[BuildingScript::V_CITIZENS_WORLD];
}

//=================================================================================================
void City::GetEntry(Vec3& pos, float& rot)
{
	if(entry_points.size() == 1)
	{
		pos = entry_points[0].spawn_area.Midpoint().XZ();
		rot = entry_points[0].spawn_rot;
	}
	else
	{
		// check which spawn rot i closest to entry rot
		float best_dif = 999.f;
		int best_index = -1, index = 0;
		float dir = Clip(-W.GetTravelDir() + PI / 2);
		for(vector<EntryPoint>::iterator it = entry_points.begin(), end = entry_points.end(); it != end; ++it, ++index)
		{
			float dif = AngleDiff(dir, it->spawn_rot);
			if(dif < best_dif)
			{
				best_dif = dif;
				best_index = index;
			}
		}
		pos = entry_points[best_index].spawn_area.Midpoint().XZ();
		rot = entry_points[best_index].spawn_rot;
	}
}

//=================================================================================================
void City::PrepareCityBuildings(vector<ToBuild>& tobuild)
{
	// required buildings
	tobuild.reserve(buildings.size());
	for(CityBuilding& cb : buildings)
		tobuild.push_back(ToBuild(cb.type, true));
	buildings.clear();

	// not required buildings
	LocalVector2<Building*> buildings;
	GenerateCityBuildings(buildings.Get(), false);
	tobuild.reserve(tobuild.size() + buildings.size());
	for(Building* b : buildings)
		tobuild.push_back(ToBuild(b, false));

	// set flags
	for(ToBuild& tb : tobuild)
	{
		if(tb.type->group == BuildingGroup::BG_TRAINING_GROUNDS)
			flags |= HaveTrainingGrounds;
		else if(tb.type->group == BuildingGroup::BG_BLACKSMITH)
			flags |= HaveBlacksmith;
		else if(tb.type->group == BuildingGroup::BG_MERCHANT)
			flags |= HaveMerchant;
		else if(tb.type->group == BuildingGroup::BG_ALCHEMIST)
			flags |= HaveAlchemist;
		else if(tb.type->group == BuildingGroup::BG_FOOD_SELLER)
			flags |= HaveFoodSeller;
		else if(tb.type->group == BuildingGroup::BG_INN)
			flags |= HaveInn;
		else if(tb.type->group == BuildingGroup::BG_ARENA)
			flags |= HaveArena;
	}
}
