// miasto
#include "Pch.h"
#include "Base.h"
#include "City.h"
#include "SaveState.h"

//=================================================================================================
City::~City()
{
	DeleteElements(inside_buildings);
}

//=================================================================================================
void City::Save(HANDLE file, bool local)
{
	OutsideLocation::Save(file, local);

	WriteFile(file, &citizens, sizeof(citizens), &tmp, nullptr);
	WriteFile(file, &citizens_world, sizeof(citizens_world), &tmp, nullptr);

	if(last_visit != -1)
	{
		FileWriter f(file);
		f.WriteVector1(entry_points);
		f << have_exit;
		f.Write<byte>(gates);

		uint ile = buildings.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		WriteFile(file, &buildings[0], sizeof(CityBuilding)*ile, &tmp, nullptr);
		WriteFile(file, &inside_offset, sizeof(inside_offset), &tmp, nullptr);
		ile = inside_buildings.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it)
			(*it)->Save(file, local);

		WriteFile(file, &quest_mayor, sizeof(quest_mayor), &tmp, nullptr);
		WriteFile(file, &quest_mayor_time, sizeof(quest_mayor_time), &tmp, nullptr);
		WriteFile(file, &quest_captain, sizeof(quest_captain), &tmp, nullptr);
		WriteFile(file, &quest_captain_time, sizeof(quest_captain_time), &tmp, nullptr);
		WriteFile(file, &arena_time, sizeof(arena_time), &tmp, nullptr);
		WriteFile(file, &arena_pos, sizeof(arena_pos), &tmp, nullptr);
	}
}

//=================================================================================================
void City::Load(HANDLE file, bool local)
{
	OutsideLocation::Load(file, local);

	ReadFile(file, &citizens, sizeof(citizens), &tmp, nullptr);
	ReadFile(file, &citizens_world, sizeof(citizens_world), &tmp, nullptr);

	if(last_visit != -1)
	{
		int side;
		BOX2D spawn_area;
		BOX2D exit_area;
		float spawn_dir;

		if(LOAD_VERSION < V_0_3)
		{
			ReadFile(file, &side, sizeof(side), &tmp, nullptr);
			ReadFile(file, &spawn_area, sizeof(spawn_area), &tmp, nullptr);
			ReadFile(file, &exit_area, sizeof(exit_area), &tmp, nullptr);
			ReadFile(file, &spawn_dir, sizeof(spawn_dir), &tmp, nullptr);
		}
		else
		{
			FileReader f(file);
			f.ReadVector1(entry_points);
			f >> have_exit;
			gates = f.Read<byte>();
		}

		uint ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		buildings.resize(ile);
		ReadFile(file, &buildings[0], sizeof(CityBuilding)*ile, &tmp, nullptr);
		ReadFile(file, &inside_offset, sizeof(inside_offset), &tmp, nullptr);
		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		inside_buildings.resize(ile);
		int index = 0;
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it, ++index)
		{
			*it = new InsideBuilding;
			(*it)->Load(file, local);
			(*it)->ctx.building_id = index;
			(*it)->ctx.mine = INT2((*it)->level_shift.x * 256, (*it)->level_shift.y * 256);
			(*it)->ctx.maxe = (*it)->ctx.mine + INT2(256, 256);
		}

		ReadFile(file, &quest_mayor, sizeof(quest_mayor), &tmp, nullptr);
		ReadFile(file, &quest_mayor_time, sizeof(quest_mayor_time), &tmp, nullptr);
		ReadFile(file, &quest_captain, sizeof(quest_captain), &tmp, nullptr);
		ReadFile(file, &quest_captain_time, sizeof(quest_captain_time), &tmp, nullptr);
		ReadFile(file, &arena_time, sizeof(arena_time), &tmp, nullptr);
		ReadFile(file, &arena_pos, sizeof(arena_pos), &tmp, nullptr);

		if(LOAD_VERSION < V_0_3)
		{
			const float aa = 11.1f;
			const float bb = 12.6f;
			const float es = 1.3f;
			const uint _s = 16 * 8;
			const int w = size;
			//const int w1 = w+1;
			const int mur1 = int(0.15f*w);
			const int mur2 = int(0.85f*w);
			TERRAIN_TILE road_type;

			// setup entrance
			switch(side)
			{
			case 0: // from top
				{
					VEC2 p(float(_s) + 1.f, 0.8f*_s * 2);
					exit_area = BOX2D(p.x - es, p.y + aa, p.x + es, p.y + bb);
					gates = GATE_NORTH;
					road_type = tiles[w / 2 + int(0.85f*w - 2)*w].t;
				}
				break;
			case 1: // from left
				{
					VEC2 p(0.2f*_s * 2, float(_s) + 1.f);
					exit_area = BOX2D(p.x - bb, p.y - es, p.x - aa, p.y + es);
					gates = GATE_WEST;
					road_type = tiles[int(0.15f*w) + 2 + (w / 2)*w].t;
				}
				break;
			case 2: // from bottom
				{
					VEC2 p(float(_s) + 1.f, 0.2f*_s * 2);
					exit_area = BOX2D(p.x - es, p.y - bb, p.x + es, p.y - aa);
					gates = GATE_SOUTH;
					road_type = tiles[w / 2 + int(0.15f*w + 2)*w].t;
				}
				break;
			case 3: // from right
				{
					VEC2 p(0.8f*_s * 2, float(_s) + 1.f);
					exit_area = BOX2D(p.x + aa, p.y - es, p.x + bb, p.y + es);
					gates = GATE_EAST;
					road_type = tiles[int(0.85f*w) - 2 + (w / 2)*w].t;
				}
				break;
			}

			// update terrain tiles
			// tiles under walls
			for(int i = mur1; i <= mur2; ++i)
			{
				// north
				tiles[i + mur1*w].Set(TT_SAND, TM_BUILDING);
				if(tiles[i + (mur1 + 1)*w].t == TT_GRASS)
					tiles[i + (mur1 + 1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
				// south
				tiles[i + mur2*w].Set(TT_SAND, TM_BUILDING);
				if(tiles[i + (mur2 - 1)*w].t == TT_GRASS)
					tiles[i + (mur2 - 1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
				// west
				tiles[mur1 + i*w].Set(TT_SAND, TM_BUILDING);
				if(tiles[mur1 + 1 + i*w].t == TT_GRASS)
					tiles[mur1 + 1 + i*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
				// east
				tiles[mur2 + i*w].Set(TT_SAND, TM_BUILDING);
				if(tiles[mur2 - 1 + i*w].t == TT_GRASS)
					tiles[mur2 - 1 + i*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
			}

			// tiles under gates
			if(gates == GATE_SOUTH)
			{
				tiles[w / 2 - 1 + int(0.15f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + int(0.15f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + 1 + int(0.15f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 - 1 + (int(0.15f*w) + 1)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + (int(0.15f*w) + 1)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + 1 + (int(0.15f*w) + 1)*w].Set(road_type, TM_ROAD);
			}
			if(gates == GATE_WEST)
			{
				tiles[int(0.15f*w) + (w / 2 - 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.15f*w) + (w / 2)*w].Set(road_type, TM_ROAD);
				tiles[int(0.15f*w) + (w / 2 + 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.15f*w) + 1 + (w / 2 - 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.15f*w) + 1 + (w / 2)*w].Set(road_type, TM_ROAD);
				tiles[int(0.15f*w) + 1 + (w / 2 + 1)*w].Set(road_type, TM_ROAD);
			}
			if(gates == GATE_NORTH)
			{
				tiles[w / 2 - 1 + int(0.85f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + int(0.85f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + 1 + int(0.85f*w)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 - 1 + (int(0.85f*w) - 1)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + (int(0.85f*w) - 1)*w].Set(road_type, TM_ROAD);
				tiles[w / 2 + 1 + (int(0.85f*w) - 1)*w].Set(road_type, TM_ROAD);
			}
			if(gates == GATE_EAST)
			{
				tiles[int(0.85f*w) + (w / 2 - 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.85f*w) + (w / 2)*w].Set(road_type, TM_ROAD);
				tiles[int(0.85f*w) + (w / 2 + 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.85f*w) - 1 + (w / 2 - 1)*w].Set(road_type, TM_ROAD);
				tiles[int(0.85f*w) - 1 + (w / 2)*w].Set(road_type, TM_ROAD);
				tiles[int(0.85f*w) - 1 + (w / 2 + 1)*w].Set(road_type, TM_ROAD);
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
			Obj* oMur = FindObject("wall");
			Obj* oWieza = FindObject("tower");

			const int mid = int(0.5f*_s);

			// walls
			for(int i = mur1; i < mur2; i += 3)
			{
				// top
				if(side != 2 || i < mid - 1 || i > mid)
				{
					Object& o = Add1(objects);
					o.pos = VEC3(float(i) * 2 + 1.f, 1.f, int(0.15f*_s) * 2 + 1.f);
					o.rot = VEC3(0, PI, 0);
					o.scale = 1.f;
					o.base = oMur;
					o.mesh = oMur->mesh;
				}

				// bottom
				if(side != 0 || i < mid - 1 || i > mid)
				{
					Object& o = Add1(objects);
					o.pos = VEC3(float(i) * 2 + 1.f, 1.f, int(0.85f*_s) * 2 + 1.f);
					o.rot = VEC3(0, 0, 0);
					o.scale = 1.f;
					o.base = oMur;
					o.mesh = oMur->mesh;
				}

				// left
				if(side != 1 || i < mid - 1 || i > mid)
				{
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.15f*_s) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
					o.rot = VEC3(0, PI * 3 / 2, 0);
					o.scale = 1.f;
					o.base = oMur;
					o.mesh = oMur->mesh;
				}

				// right
				if(side != 3 || i < mid - 1 || i > mid)
				{
					Object& o = Add1(objects);
					o.pos = VEC3(int(0.85f*_s) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
					o.rot = VEC3(0, PI / 2, 0);
					o.scale = 1.f;
					o.base = oMur;
					o.mesh = oMur->mesh;
				}
			}

			// towers
			{
				// right top
				Object& o = Add1(objects);
				o.pos = VEC3(int(0.85f*_s) * 2 + 1.f, 1.f, int(0.85f*_s) * 2 + 1.f);
				o.rot = VEC3(0, 0, 0);
				o.scale = 1.f;
				o.base = oWieza;
				o.mesh = oWieza->mesh;
			}
			{
				// right bottom
				Object& o = Add1(objects);
				o.pos = VEC3(int(0.85f*_s) * 2 + 1.f, 1.f, int(0.15f*_s) * 2 + 1.f);
				o.rot = VEC3(0, PI / 2, 0);
				o.scale = 1.f;
				o.base = oWieza;
				o.mesh = oWieza->mesh;
			}
			{
				// left bottom
				Object& o = Add1(objects);
				o.pos = VEC3(int(0.15f*_s) * 2 + 1.f, 1.f, int(0.15f*_s) * 2 + 1.f);
				o.rot = VEC3(0, PI, 0);
				o.scale = 1.f;
				o.base = oWieza;
				o.mesh = oWieza->mesh;
			}
			{
				// left top
				Object& o = Add1(objects);
				o.pos = VEC3(int(0.15f*_s) * 2 + 1.f, 1.f, int(0.85f*_s) * 2 + 1.f);
				o.rot = VEC3(0, PI * 3 / 2, 0);
				o.scale = 1.f;
				o.base = oWieza;
				o.mesh = oWieza->mesh;
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
				o.pos = VEC3(0.5f*_s * 2 + 1.f, 1.f, 0.85f*_s * 2);
				break;
			case 1:
				o.rot.y = PI * 3 / 2;
				o.pos = VEC3(0.15f*_s * 2, 1.f, 0.5f*_s * 2 + 1.f);
				break;
			case 2:
				o.rot.y = PI;
				o.pos = VEC3(0.5f*_s * 2 + 1.f, 1.f, 0.15f*_s * 2);
				break;
			case 3:
				o.rot.y = PI / 2;
				o.pos = VEC3(0.85f*_s * 2, 1.f, 0.5f*_s * 2 + 1.f);
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
Unit* City::FindUnitInsideBuilding(const UnitData* ud, BUILDING building_type) const
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
