#include "Pch.h"
#include "Base.h"
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

	if(LOAD_VERSION >= V_0_10)
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
				b.type = content::FindBuilding(f.ReadString1C());
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
				b.type = content::FindBuilding(f.ReadString1C());
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
				b->ctx.mine = INT2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + INT2(256, 256);
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
			BOX2D spawn_area;
			BOX2D exit_area;
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
				b.type = content::FindOldBuilding(type);
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
				b->ctx.mine = INT2(b->level_shift.x * 256, b->level_shift.y * 256);
				b->ctx.maxe = b->ctx.mine + INT2(256, 256);
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
						VEC2 p(float(size) + 1.f, 0.8f*size * 2);
						exit_area = BOX2D(p.x - es, p.y + aa, p.x + es, p.y + bb);
						gates = GATE_NORTH;
						road_type = tiles[size / 2 + int(0.85f*size - 2)*size].t;
					}
					break;
				case 1: // from left
					{
						VEC2 p(0.2f*size * 2, float(size) + 1.f);
						exit_area = BOX2D(p.x - bb, p.y - es, p.x - aa, p.y + es);
						gates = GATE_WEST;
						road_type = tiles[int(0.15f*size) + 2 + (size / 2)*size].t;
					}
					break;
				case 2: // from bottom
					{
						VEC2 p(float(size) + 1.f, 0.2f*size * 2);
						exit_area = BOX2D(p.x - es, p.y - bb, p.x + es, p.y - aa);
						gates = GATE_SOUTH;
						road_type = tiles[size / 2 + int(0.15f*size + 2)*size].t;
					}
					break;
				case 3: // from right
					{
						VEC2 p(0.8f*size * 2, float(size) + 1.f);
						exit_area = BOX2D(p.x + aa, p.y - es, p.x + bb, p.y + es);
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
				Obj* to_remove = FindObject("to_remove");
				vector<Object>::iterator it = objects.begin();
				while(it != objects.end())
				{
					if(it->base == to_remove)
						it = objects.erase(it);
					else
						++it;
				}

				// add new buildings
				Obj* oWall = FindObject("wall");
				Obj* oTower = FindObject("tower");

				const int mid = int(0.5f*size);

				// walls
				for(int i = mur1; i < mur2; i += 3)
				{
					// top
					if(side != 2 || i < mid - 1 || i > mid)
					{
						Object& o = Add1(objects);
						o.pos = VEC3(float(i) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
						o.rot = VEC3(0, PI, 0);
						o.scale = 1.f;
						o.base = oWall;
						o.mesh = oWall->mesh;
					}

					// bottom
					if(side != 0 || i < mid - 1 || i > mid)
					{
						Object& o = Add1(objects);
						o.pos = VEC3(float(i) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
						o.rot = VEC3(0, 0, 0);
						o.scale = 1.f;
						o.base = oWall;
						o.mesh = oWall->mesh;
					}

					// left
					if(side != 1 || i < mid - 1 || i > mid)
					{
						Object& o = Add1(objects);
						o.pos = VEC3(int(0.15f*size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
						o.rot = VEC3(0, PI * 3 / 2, 0);
						o.scale = 1.f;
						o.base = oWall;
						o.mesh = oWall->mesh;
					}

					// right
					if(side != 3 || i < mid - 1 || i > mid)
					{
						Object& o = Add1(objects);
						o.pos = VEC3(int(0.85f*size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
						o.rot = VEC3(0, PI / 2, 0);
						o.scale = 1.f;
						o.base = oWall;
						o.mesh = oWall->mesh;
					}
				}

				// towers
				{
					// right top
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.85f*size) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
					o.rot = VEC3(0, 0, 0);
					o.scale = 1.f;
					o.base = oTower;
					o.mesh = oTower->mesh;
				}
				{
					// right bottom
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.85f*size) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
					o.rot = VEC3(0, PI / 2, 0);
					o.scale = 1.f;
					o.base = oTower;
					o.mesh = oTower->mesh;
				}
				{
					// left bottom
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.15f*size) * 2 + 1.f, 1.f, int(0.15f*size) * 2 + 1.f);
					o.rot = VEC3(0, PI, 0);
					o.scale = 1.f;
					o.base = oTower;
					o.mesh = oTower->mesh;
				}
				{
					// left top
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.15f*size) * 2 + 1.f, 1.f, int(0.85f*size) * 2 + 1.f);
					o.rot = VEC3(0, PI * 3 / 2, 0);
					o.scale = 1.f;
					o.base = oTower;
					o.mesh = oTower->mesh;
				}

				// gate
				Object& o = Add1(objects);
				o.rot.x = o.rot.z = 0.f;
				o.scale = 1.f;
				o.base = FindObject("gate");
				o.mesh = o.base->mesh;
				switch(side)
				{
				case 0:
					o.rot.y = 0;
					o.pos = VEC3(0.5f*size * 2 + 1.f, 1.f, 0.85f*size * 2);
					break;
				case 1:
					o.rot.y = PI * 3 / 2;
					o.pos = VEC3(0.15f*size * 2, 1.f, 0.5f*size * 2 + 1.f);
					break;
				case 2:
					o.rot.y = PI;
					o.pos = VEC3(0.5f*size * 2 + 1.f, 1.f, 0.15f*size * 2);
					break;
				case 3:
					o.rot.y = PI / 2;
					o.pos = VEC3(0.85f*size * 2, 1.f, 0.5f*size * 2 + 1.f);
					break;
				}

				// grate
				Object& o2 = Add1(objects);
				o2.pos = o.pos;
				o2.rot = o.rot;
				o2.scale = 1.f;
				o2.base = FindObject("grate");
				o2.mesh = o2.base->mesh;

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
				Building* village_hall = content::FindOldBuilding(OLD_BUILDING::B_VILLAGE_HALL);

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
					Building* village_hall_old = content::FindOldBuilding(OLD_BUILDING::B_VILLAGE_HALL_OLD);
					FindBuilding(village_hall)->type = village_hall_old;
					for(Object& o : objects)
					{
						if(o.mesh->res->filename == "soltys.qmsh")
							o.mesh = ResourceManager::Get().GetLoadedMesh("soltys_old.qmsh")->data;
					}
					InsideBuilding* b = FindInsideBuilding(village_hall);
					b->type = village_hall_old;
					for(Object& o : b->objects)
					{
						if(o.mesh->res->filename == "soltys_srodek.qmsh")
							o.mesh = ResourceManager::Get().GetLoadedMesh("soltys_srodek_old.qmsh")->data;
					}
				}
			}

			if(state == LS_KNOWN)
			{
				buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_VILLAGE_HALL)));
				buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_MERCHANT)));
				buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_FOOD_SELLER)));
				buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_VILLAGE_INN)));
				if(v_buildings[0] != OLD_BUILDING::B_NONE)
					buildings.push_back(CityBuilding(content::FindOldBuilding(v_buildings[0])));
				if(v_buildings[1] != OLD_BUILDING::B_NONE)
					buildings.push_back(CityBuilding(content::FindOldBuilding(v_buildings[1])));
				std::random_shuffle(buildings.begin() + 1, buildings.end(), myrand);
				buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_BARRACKS)));
				
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
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_CITY_HALL)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_ARENA)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_MERCHANT)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_FOOD_SELLER)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_BLACKSMITH)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_ALCHEMIST)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_INN)));
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_TRAINING_GROUNDS)));
			std::random_shuffle(buildings.begin() + 2, buildings.end(), myrand);
			buildings.push_back(CityBuilding(content::FindOldBuilding(OLD_BUILDING::B_BARRACKS)));
			
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

		for(vector<Useable*>::iterator it = (*it2)->useables.begin(), end = (*it2)->useables.end(); it != end; ++it)
		{
			(*it)->refid = (int)Useable::refid_table.size();
			Useable::refid_table.push_back(*it);
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
bool City::IsInsideCity(const VEC3& _pos)
{
	INT2 tile(int(_pos.x / 2), int(_pos.z / 2));
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
	assert(group >= 0);
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
	assert(group >= 0);
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
	assert(group >= 0);
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
