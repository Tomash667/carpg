#include "Pch.h"
#include "Core.h"
#include "City.h"
#include "SaveState.h"
#include "Content.h"
#include "ResourceManager.h"
#include "Object.h"
#include "Unit.h"

//=================================================================================================
City::~City()
{
	DeleteElements(inside_buildings);
}

//=================================================================================================
void City::Save(HANDLE file, bool local)
{
	OutsideLocation::Save(file, local);

	StreamWriter f(file);

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
			b->Save(file, local);

		f << quest_mayor;
		f << quest_mayor_time;
		f << quest_captain;
		f << quest_captain_time;
		f << arena_time;
		f << arena_pos;
	}
}

//=================================================================================================
void City::Load(HANDLE file, bool local, LOCATION_TOKEN token)
{
	OutsideLocation::Load(file, local, token);

	StreamReader f(file);

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
				b.type = Building::Get(f.ReadString1C());
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
				b.type = Building::Get(f.ReadString1C());
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
				b->Load(file, local);
				b->ctx.building_id = index;
				b->ctx.mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + Int2(256, 256);
				++index;
			}

			f.Refresh();
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
				b->Load(file, local);
				b->ctx.building_id = index;
				b->ctx.mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + Int2(256, 256);
				++index;
			}

			f.Refresh();
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
void City::BuildRefidTable()
{
	OutsideLocation::BuildRefidTable();

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
/*Unit* City::FindUnitInsideBuilding(const UnitData* ud, BUILDING building_type) const
{
	assert(ud);

	for(vector<InsideBuilding*>::const_iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it)
	{
		if((*it)->type == building_type)
		{
			return (*it)->FindUnit(ud);
		}
	}

	assert(0);
	return nullptr;
}*/

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
