#include "Pch.h"
#include "City.h"

#include "BitStreamFunc.h"
#include "BuildingScript.h"
#include "Content.h"
#include "GameCommon.h"
#include "GroundItem.h"
#include "Level.h"
#include "Object.h"
#include "Unit.h"
#include "World.h"

#include <ResourceManager.h>
#include <Terrain.h>

//=================================================================================================
City::~City()
{
	DeleteElements(insideBuildings);
}

//=================================================================================================
void City::Apply(vector<std::reference_wrapper<LocationPart>>& parts)
{
	parts.push_back(*this);
	for(InsideBuilding* ib : insideBuildings)
		parts.push_back(*ib);
}

//=================================================================================================
void City::Save(GameWriter& f)
{
	OutsideLocation::Save(f);

	f << citizens;
	f << citizensWorld;
	f << flags;
	f << variant;

	if(lastVisit == -1)
	{
		// list of buildings in this location is generated
		f << buildings.size();
		for(CityBuilding& b : buildings)
			f << b.building->id;
	}
	else
	{
		f << entryPoints;
		f << gates;

		f << buildings.size();
		for(CityBuilding& b : buildings)
		{
			f << b.building->id;
			f << b.pt;
			f << b.unitPt;
			f << b.dir;
			f << b.walkPt;
		}

		f << insideOffset;
		f << insideBuildings.size();
		for(InsideBuilding* b : insideBuildings)
			b->Save(f);

		f << questMayor;
		f << questMayorTime;
		f << questCaptain;
		f << questCaptainTime;
		f << arenaTime;
		f << arenaPos;
	}
}

//=================================================================================================
void City::Load(GameReader& f)
{
	OutsideLocation::Load(f);

	f >> citizens;
	f >> citizensWorld;
	if(LOAD_VERSION < V_0_12)
		f >> target;
	f >> flags;
	f >> variant;

	if(lastVisit == -1)
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
		f >> entryPoints;
		f >> gates;

		uint count;
		f >> count;
		buildings.resize(count);
		for(CityBuilding& b : buildings)
		{
			b.building = Building::Get(f.ReadString1());
			f >> b.pt;
			f >> b.unitPt;
			f >> b.dir;
			f >> b.walkPt;
			assert(b.building != nullptr);
			if(LOAD_VERSION < V_0_14 && b.building->id == "mages_tower")
			{
				const float rot = DirToRot(b.dir);
				Mesh::Point* point = b.building->mesh->FindPoint("o_s_point");
				Vec3 pos;
				if(b.dir != GDIR_DOWN)
				{
					const Matrix m = point->mat * Matrix::RotationY(rot);
					pos = Vec3::TransformZero(m);
				}
				else
					pos = Vec3::TransformZero(point->mat);
				pos += Vec3(float(b.pt.x + b.building->shift[b.dir].x) * 2, 0.f, float(b.pt.y + b.building->shift[b.dir].y) * 2);
				b.walkPt = pos;
				gameLevel->terrain->SetHeightMap(h);
				gameLevel->terrain->SetY(b.walkPt);
				gameLevel->terrain->RemoveHeightMap();
			}
		}

		f >> insideOffset;
		f >> count;
		insideBuildings.resize(count);
		int index = 0;
		for(InsideBuilding*& b : insideBuildings)
		{
			b = new InsideBuilding(index);
			b->Load(f);
			b->mine = Int2(b->levelShift.x * 256, b->levelShift.y * 256);
			b->maxe = b->mine + Int2(256, 256);
			++index;
		}

		f >> questMayor;
		f >> questMayorTime;
		f >> questCaptain;
		f >> questCaptainTime;
		f >> arenaTime;
		f >> arenaPos;
	}
}

//=================================================================================================
void City::Write(BitStreamWriter& f)
{
	OutsideLocation::Write(f);

	f.WriteCasted<byte>(flags);
	f.WriteCasted<byte>(entryPoints.size());
	for(EntryPoint& entryPoint : entryPoints)
	{
		f << entryPoint.exitRegion;
		f << entryPoint.exitY;
	}
	f.WriteCasted<byte>(buildings.size());
	for(CityBuilding& building : buildings)
	{
		f << building.building->id;
		f << building.pt;
		f.WriteCasted<byte>(building.dir);
	}
	f.WriteCasted<byte>(insideBuildings.size());
	for(InsideBuilding* insideBuilding : insideBuildings)
		insideBuilding->Write(f);
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
	entryPoints.resize(count);
	for(EntryPoint& entry : entryPoints)
	{
		f.Read(entry.exitRegion);
		f.Read(entry.exitY);
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
		const string& buildingId = f.ReadString1();
		f >> building.pt;
		f.ReadCasted<byte>(building.dir);
		if(!f)
		{
			Error("Read level: Broken packet for buildings.");
			return false;
		}
		building.building = Building::Get(buildingId);
		if(!building.building)
		{
			Error("Read level: Invalid building id '%s'.", buildingId.c_str());
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
	insideBuildings.resize(count);
	int index = 0;
	for(InsideBuilding*& ib : insideBuildings)
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
bool City::IsInsideCity(const Vec3& pos)
{
	Int2 tile(int(pos.x / 2), int(pos.z / 2));
	if(tile.x <= int(0.15f * size) || tile.y <= int(0.15f * size) || tile.x >= int(0.85f * size) || tile.y >= int(0.85f * size))
		return false;
	else
		return true;
}

//=================================================================================================
InsideBuilding* City::FindInsideBuilding(Building* building, int* outIndex)
{
	assert(building);
	int index = 0;
	for(InsideBuilding* i : insideBuildings)
	{
		if(i->building == building)
		{
			if(outIndex)
				*outIndex = index;
			return i;
		}
		++index;
	}
	if(outIndex)
		*outIndex = -1;
	return nullptr;
}

//=================================================================================================
InsideBuilding* City::FindInsideBuilding(BuildingGroup* group, int* outIndex)
{
	assert(group);
	int index = 0;
	for(InsideBuilding* i : insideBuildings)
	{
		if(i->building->group == group)
		{
			if(outIndex)
				*outIndex = index;
			return i;
		}
		++index;
	}
	if(outIndex)
		*outIndex = -1;
	return nullptr;
}

//=================================================================================================
CityBuilding* City::FindBuilding(BuildingGroup* group, int* outIndex)
{
	assert(group);
	int index = 0;
	for(CityBuilding& b : buildings)
	{
		if(b.building->group == group)
		{
			if(outIndex)
				*outIndex = index;
			return &b;
		}
		++index;
	}
	if(outIndex)
		*outIndex = -1;
	return nullptr;
}

//=================================================================================================
CityBuilding* City::FindBuilding(Building* building, int* outIndex)
{
	assert(building);
	int index = 0;
	for(CityBuilding& b : buildings)
	{
		if(b.building == building)
		{
			if(outIndex)
				*outIndex = index;
			return &b;
		}
		++index;
	}
	if(outIndex)
		*outIndex = -1;
	return nullptr;
}

//=================================================================================================
void City::GenerateCityBuildings(vector<Building*>& buildings, bool required)
{
	cstring scriptName;
	switch(target)
	{
	default:
	case CITY:
		scriptName = "city";
		break;
	case CAPITAL:
		scriptName = "capital";
		break;
	case VILLAGE:
		scriptName = "village";
		break;
	case VILLAGE_EMPTY:
		scriptName = "village_empty";
		break;
	case VILLAGE_DESTROYED:
		scriptName = "village_destroyed";
		break;
	case VILLAGE_DESTROYED2:
		scriptName = "village_destroyed2";
		break;
	}

	BuildingScript* script = BuildingScript::Get(scriptName);
	if(variant == -1)
		variant = Rand() % script->variants.size();

	BuildingScript::Variant* v = script->variants[variant];
	int* code = v->code.data();
	int* end = code + v->code.size();
	for(uint i = 0; i < BuildingScript::MAX_VARS; ++i)
		script->vars[i] = 0;
	script->vars[BuildingScript::V_COUNT] = 1;
	script->vars[BuildingScript::V_CITIZENS] = citizens;
	script->vars[BuildingScript::V_CITIZENS_WORLD] = citizensWorld;
	if(!required)
		code += script->requiredOffset;

	vector<int> stack;
	int ifLevel = 0, ifDepth = 0;
	int shuffleStart = -1;
	int& buildingCount = script->vars[BuildingScript::V_COUNT];

	while(code != end)
	{
		BuildingScript::Code c = (BuildingScript::Code)*code++;
		switch(c)
		{
		case BuildingScript::BS_ADD_BUILDING:
			if(ifLevel == ifDepth)
			{
				BuildingScript::Code type = (BuildingScript::Code)*code++;
				if(type == BuildingScript::BS_BUILDING)
				{
					Building* b = (Building*)*code++;
					for(int i = 0; i < buildingCount; ++i)
						buildings.push_back(b);
				}
				else
				{
					BuildingGroup* bg = (BuildingGroup*)*code++;
					for(int i = 0; i < buildingCount; ++i)
						buildings.push_back(RandomItem(bg->buildings));
				}
			}
			else
				code += 2;
			break;
		case BuildingScript::BS_RANDOM:
			{
				uint count = (uint)*code++;
				if(ifLevel != ifDepth)
				{
					code += count * 2;
					break;
				}

				for(int i = 0; i < buildingCount; ++i)
				{
					uint index = Rand() % count;
					BuildingScript::Code type = (BuildingScript::Code) * (code + index * 2);
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
			if(ifLevel == ifDepth)
				shuffleStart = (int)buildings.size();
			break;
		case BuildingScript::BS_SHUFFLE_END:
			if(ifLevel == ifDepth)
			{
				int new_pos = (int)buildings.size();
				if(new_pos - shuffleStart >= 2)
					Shuffle(buildings.begin() + shuffleStart, buildings.end());
				shuffleStart = -1;
			}
			break;
		case BuildingScript::BS_REQUIRED_OFF:
			if(required)
				goto cleanup;
			break;
		case BuildingScript::BS_PUSH_INT:
			if(ifLevel == ifDepth)
				stack.push_back(*code++);
			else
				++code;
			break;
		case BuildingScript::BS_PUSH_VAR:
			if(ifLevel == ifDepth)
				stack.push_back(script->vars[*code++]);
			else
				++code;
			break;
		case BuildingScript::BS_SET_VAR:
			if(ifLevel == ifDepth)
			{
				script->vars[*code++] = stack.back();
				stack.pop_back();
			}
			else
				++code;
			break;
		case BuildingScript::BS_INC:
			if(ifLevel == ifDepth)
				++script->vars[*code++];
			else
				++code;
			break;
		case BuildingScript::BS_DEC:
			if(ifLevel == ifDepth)
				--script->vars[*code++];
			else
				++code;
			break;
		case BuildingScript::BS_IF:
			if(ifLevel == ifDepth)
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
				++ifLevel;
				if(ok)
					++ifDepth;
			}
			else
			{
				++code;
				++ifLevel;
			}
			break;
		case BuildingScript::BS_IF_RAND:
			if(ifLevel == ifDepth)
			{
				int a = stack.back();
				stack.pop_back();
				if(a > 0 && Rand() % a == 0)
					++ifDepth;
			}
			++ifLevel;
			break;
		case BuildingScript::BS_ELSE:
			if(ifLevel == ifDepth)
				--ifDepth;
			break;
		case BuildingScript::BS_ENDIF:
			if(ifLevel == ifDepth)
				--ifDepth;
			--ifLevel;
			break;
		case BuildingScript::BS_CALL:
		case BuildingScript::BS_ADD:
		case BuildingScript::BS_SUB:
		case BuildingScript::BS_MUL:
		case BuildingScript::BS_DIV:
			if(ifLevel == ifDepth)
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
			if(ifLevel == ifDepth)
				stack.back() = -stack.back();
			break;
		}
	}

cleanup:
	citizens = script->vars[BuildingScript::V_CITIZENS];
	citizensWorld = script->vars[BuildingScript::V_CITIZENS_WORLD];
}

//=================================================================================================
void City::GetEntry(Vec3& pos, float& rot)
{
	if(entryPoints.size() == 1)
	{
		pos = entryPoints[0].spawnRegion.Midpoint().XZ();
		rot = entryPoints[0].spawnRot;
	}
	else
	{
		// check which spawn rot i closest to entry rot
		float bestDif = 999.f;
		int bestIndex = -1, index = 0;
		float dir = ConvertNewAngleToOld(world->GetTravelDir());
		for(vector<EntryPoint>::iterator it = entryPoints.begin(), end = entryPoints.end(); it != end; ++it, ++index)
		{
			float dif = AngleDiff(dir, it->spawnRot);
			if(dif < bestDif)
			{
				bestDif = dif;
				bestIndex = index;
			}
		}
		pos = entryPoints[bestIndex].spawnRegion.Midpoint().XZ();
		rot = entryPoints[bestIndex].spawnRot;
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
	LocalVector<Building*> buildings;
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

//=================================================================================================
Vec3 CityBuilding::GetUnitPos()
{
	return Vec3(float(unitPt.x) * 2 + 1, 0, float(unitPt.y) * 2 + 1);
}

//=================================================================================================
float CityBuilding::GetUnitRot()
{
	return DirToRot(dir);
}
