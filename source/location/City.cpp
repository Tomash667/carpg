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
#include "GroundItem.h"
#include "GameCommon.h"

//=================================================================================================
City::~City()
{
	DeleteElements(inside_buildings);
}

//=================================================================================================
void City::Apply(vector<std::reference_wrapper<LevelArea>>& areas)
{
	areas.push_back(*this);
	for(InsideBuilding* ib : inside_buildings)
		areas.push_back(*ib);
}

//=================================================================================================
void City::Save(GameWriter& f, bool local)
{
	OutsideLocation::Save(f, local);

	f << citizens;
	f << citizens_world;
	f << flags;
	f << variant;

	if(last_visit == -1)
	{
		// list of buildings in this location is generated
		f << buildings.size();
		for(CityBuilding& b : buildings)
			f << b.building->id;
	}
	else
	{
		f << entry_points;
		f << gates;

		f << buildings.size();
		for(CityBuilding& b : buildings)
		{
			f << b.building->id;
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
void City::Load(GameReader& f, bool local)
{
	OutsideLocation::Load(f, local);

	f >> citizens;
	f >> citizens_world;
	if(LOAD_VERSION < V_0_12)
		f >> target;
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
			b.building = Building::Get(f.ReadString1());
			assert(b.building != nullptr);
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
			b.building = Building::Get(f.ReadString1());
			f >> b.pt;
			f >> b.unit_pt;
			f >> b.rot;
			f >> b.walk_pt;
			assert(b.building != nullptr);
		}

		f >> inside_offset;
		f >> count;
		inside_buildings.resize(count);
		int index = 0;
		for(InsideBuilding*& b : inside_buildings)
		{
			b = new InsideBuilding(index);
			b->Load(f, local);
			b->mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
			b->maxe = b->mine + Int2(256, 256);
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

//=================================================================================================
void City::Write(BitStreamWriter& f)
{
	OutsideLocation::Write(f);

	f.WriteCasted<byte>(flags);
	f.WriteCasted<byte>(entry_points.size());
	for(EntryPoint& entry_point : entry_points)
	{
		f << entry_point.exit_region;
		f << entry_point.exit_y;
	}
	f.WriteCasted<byte>(buildings.size());
	for(CityBuilding& building : buildings)
	{
		f << building.building->id;
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
	if(!OutsideLocation::Read(f))
		return false;

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
		f.Read(entry.exit_region);
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
		building.building = Building::Get(building_id);
		if(!building.building)
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
		ib = new InsideBuilding(index);
		if(!ib->Read(f))
		{
			Error("Read level: Failed to loading inside building %d.", index);
			return false;
		}
		++index;
	}

	return true;
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
InsideBuilding* City::FindInsideBuilding(Building* building)
{
	assert(building);
	for(InsideBuilding* i : inside_buildings)
	{
		if(i->building == building)
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
		if(i->building->group == group)
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
		if(i->building->group == group)
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
		if(b.building->group == group)
			return &b;
	}
	return nullptr;
}

//=================================================================================================
CityBuilding* City::FindBuilding(Building* building)
{
	assert(building);
	for(CityBuilding& b : buildings)
	{
		if(b.building == building)
			return &b;
	}
	return nullptr;
}

//=================================================================================================
void City::GenerateCityBuildings(vector<Building*>& buildings, bool required)
{
	cstring script_name;
	switch(target)
	{
	default:
	case CITY:
		script_name = "city";
		break;
	case CAPITAL:
		script_name = "capital";
		break;
	case VILLAGE:
		script_name = "village";
		break;
	}

	BuildingScript* script = BuildingScript::Get(script_name);
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
						buildings.push_back(RandomItem(bg->buildings));
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
						buildings.push_back(RandomItem(bg->buildings));
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
					Shuffle(buildings.begin() + shuffle_start, buildings.end());
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
		pos = entry_points[0].spawn_region.Midpoint().XZ();
		rot = entry_points[0].spawn_rot;
	}
	else
	{
		// check which spawn rot i closest to entry rot
		float best_dif = 999.f;
		int best_index = -1, index = 0;
		float dir = ConvertNewAngleToOld(world->GetTravelDir());
		for(vector<EntryPoint>::iterator it = entry_points.begin(), end = entry_points.end(); it != end; ++it, ++index)
		{
			float dif = AngleDiff(dir, it->spawn_rot);
			if(dif < best_dif)
			{
				best_dif = dif;
				best_index = index;
			}
		}
		pos = entry_points[best_index].spawn_region.Midpoint().XZ();
		rot = entry_points[best_index].spawn_rot;
	}
}

//=================================================================================================
void City::PrepareCityBuildings(vector<ToBuild>& tobuild)
{
	// required buildings
	tobuild.reserve(buildings.size());
	for(CityBuilding& cb : buildings)
		tobuild.push_back(ToBuild(cb.building, true));
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
		if(tb.building->group == BuildingGroup::BG_TRAINING_GROUNDS)
			flags |= HaveTrainingGrounds;
		else if(tb.building->group == BuildingGroup::BG_BLACKSMITH)
			flags |= HaveBlacksmith;
		else if(tb.building->group == BuildingGroup::BG_MERCHANT)
			flags |= HaveMerchant;
		else if(tb.building->group == BuildingGroup::BG_ALCHEMIST)
			flags |= HaveAlchemist;
		else if(tb.building->group == BuildingGroup::BG_FOOD_SELLER)
			flags |= HaveFoodSeller;
		else if(tb.building->group == BuildingGroup::BG_INN)
			flags |= HaveInn;
		else if(tb.building->group == BuildingGroup::BG_ARENA)
			flags |= HaveArena;
	}
}
