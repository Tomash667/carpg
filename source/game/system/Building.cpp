#include "Pch.h"
#include "Base.h"
#include "Building.h"
#include "BuildingGroup.h"
#include "Content.h"
#include "Crc.h"
#include "TypeVectorContainer.h"
#include "TextWriter.h"

//-----------------------------------------------------------------------------
vector<Building*> content::buildings;

//=================================================================================================
// Find building by id
//=================================================================================================
Building* content::FindBuilding(const AnyString& id)
{
	for(Building* b : buildings)
	{
		if(b->id == id.s)
			return b;
	}
	return nullptr;
}

//=================================================================================================
// Find old building using hardcoded identifier
// Required for pre 0.5 compability
//=================================================================================================
Building* content::FindOldBuilding(OLD_BUILDING type)
{
	cstring name;
	switch(type)
	{
	case OLD_BUILDING::B_MERCHANT:
		name = "merchant";
		break;
	case OLD_BUILDING::B_BLACKSMITH:
		name = "blacksmith";
		break;
	case OLD_BUILDING::B_ALCHEMIST:
		name = "alchemist";
		break;
	case OLD_BUILDING::B_TRAINING_GROUNDS:
		name = "training_grounds";
		break;
	case OLD_BUILDING::B_INN:
		name = "inn";
		break;
	case OLD_BUILDING::B_CITY_HALL:
		name = "city_hall";
		break;
	case OLD_BUILDING::B_VILLAGE_HALL:
		name = "village_hall";
		break;
	case OLD_BUILDING::B_BARRACKS:
		name = "barracks";
		break;
	case OLD_BUILDING::B_HOUSE:
		name = "house";
		break;
	case OLD_BUILDING::B_HOUSE2:
		name = "house2";
		break;
	case OLD_BUILDING::B_HOUSE3:
		name = "house3";
		break;
	case OLD_BUILDING::B_ARENA:
		name = "arena";
		break;
	case OLD_BUILDING::B_FOOD_SELLER:
		name = "food_seller";
		break;
	case OLD_BUILDING::B_COTTAGE:
		name = "cottage";
		break;
	case OLD_BUILDING::B_COTTAGE2:
		name = "cottage2";
		break;
	case OLD_BUILDING::B_COTTAGE3:
		name = "cottage3";
		break;
	case OLD_BUILDING::B_VILLAGE_INN:
		name = "village_inn";
		break;
	case OLD_BUILDING::B_VILLAGE_HALL_OLD:
		name = "village_hall_old";
		break;
	case OLD_BUILDING::B_NONE:
	case OLD_BUILDING::B_MAX:
	default:
		assert(0);
		return nullptr;
	}

	return FindBuilding(name);
}

//=================================================================================================
// Building scheme type handler
//=================================================================================================
struct BuildingSchemeHandler : public Type::CustomFieldHandler
{
	void SaveText(TextWriter& t, TypeItem* item, uint offset) override
	{
		Building& b = *(Building*)item;

		t << "{\n";
		for(int y = 0; y < b.size.y; ++y)
		{
			t << "\t\t\"";
			for(int x = 0; x < b.size.x; ++x)
			{
				switch(b.scheme[x + y*b.size.x])
				{
				case Building::SCHEME_GRASS:
					t << ' ';
					break;
				case Building::SCHEME_SAND:
					t << '.';
					break;
				case Building::SCHEME_PATH:
					t << '+';
					break;
				case Building::SCHEME_UNIT:
					t << '@';
					break;
				case Building::SCHEME_BUILDING_PART:
					t << '|';
					break;
				case Building::SCHEME_BUILDING:
					t << '#';
					break;
				}
			}
			t << "\"\n";
		}
		t << "\t}";
	}

	//=================================================================================================
	// Load scheme from text file
	void LoadText(Tokenizer& t, TypeItem* item, uint offset) override
	{
		Building& building = item->To<Building>();

		building.scheme.clear();
		t.AssertSymbol('{');
		t.Next();
		INT2 size(0, 0);
		while(!t.IsSymbol('}'))
		{
			const string& line = t.MustGetString();
			if(line.empty())
				t.Throw("Empty scheme line.");
			if(size.x == 0)
				size.x = line.length();
			else if(size.x != line.length())
				t.Throw("Mixed scheme line size.");
			for(uint i = 0; i < line.length(); ++i)
			{
				Building::TileScheme s;
				switch(line[i])
				{
				case ' ':
					s = Building::SCHEME_GRASS;
					break;
				case '.':
					s = Building::SCHEME_SAND;
					break;
				case '+':
					s = Building::SCHEME_PATH;
					break;
				case '@':
					s = Building::SCHEME_UNIT;
					break;
				case '|':
					s = Building::SCHEME_BUILDING_PART;
					break;
				case '#':
					s = Building::SCHEME_BUILDING;
					break;
				default:
					t.Throw("Unknown scheme tile '%c'.", line[i]);
				}
				building.scheme.push_back(s);
			}
			++size.y;
			t.Next();
		}
		t.Next();
		building.size = size;
	}

	//=================================================================================================
	// Update crc using item
	void UpdateCrc(CRC32& crc, TypeItem* item, uint offset) override
	{
		Building& building = item->To<Building>();
		crc.UpdateVector(building.scheme);
	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset) override
	{
		Building& building1 = item1->To<Building>();
		Building& building2 = item2->To<Building>();
		if(building1.size != building2.size)
			return false;
		else
			return building1.scheme == building2.scheme;
	}

	void Copy(TypeItem* _from, TypeItem* _to, uint offset) override
	{
		Building& from = _from->To<Building>();
		Building& to = _to->To<Building>();
		to.size = from.size;
		to.scheme = from.scheme;
	}
};

//=================================================================================================
// Building shift type handler
//=================================================================================================
struct BuildingShiftHandler : public Type::CustomFieldHandler
{
	enum Side
	{
		S_BOTTOM,
		S_RIGHT,
		S_TOP,
		S_LEFT
	};

	int group;

	//=================================================================================================
	// Register keywords
	BuildingShiftHandler(Type* type)
	{
		group = type->AddKeywords({
			{ "bottom",S_BOTTOM },
			{ "top",S_TOP },
			{ "left",S_LEFT },
			{ "right",S_RIGHT }
		}, "building side");
	}

	void SaveText(TextWriter& f, TypeItem* item, uint offset) override
	{
		auto& building = item->To<Building>();

		if(building.shift[0] == building.shift[1] && building.shift[0] == building.shift[2] && building.shift[0] == building.shift[3])
		{
			if(building.shift[0] == INT2(0, 0))
				return;

			f << Format("{%d %d}", building.shift[0].x, building.shift[0].y);
		}
		else
		{
			f << Format("{\n\t\tbottom {%d %d}\n\t\tright {%d %d}\n\t\ttop {%d %d}\n\t\tleft {%d %d}\n\t}", building.shift[0].x, building.shift[0].y,
				building.shift[1].x, building.shift[1].y, building.shift[2].x, building.shift[2].y, building.shift[3].x, building.shift[3].y);
		}
	}

	//=================================================================================================
	// Load shift from text file
	void LoadText(Tokenizer& t, TypeItem* item, uint offset) override
	{
		auto& building = item->To<Building>();

		t.AssertSymbol('{');
		t.Next();
		if(t.IsKeywordGroup(group))
		{
			do
			{
				Side side = (Side)t.MustGetKeywordId(group);
				t.Next();
				t.Parse(building.shift[(int)side]);
			} while(t.IsKeywordGroup(group));
		}
		else if(t.IsInt())
		{
			int shift_x = t.GetInt();
			t.Next();
			int shift_y = t.MustGetInt();
			t.Next();
			for(int i = 0; i < 4; ++i)
			{
				building.shift[i].x = shift_x;
				building.shift[i].y = shift_y;
			}
		}
		else
			t.StartUnexpected().Add(tokenizer::T_KEYWORD_GROUP, &group).Add(tokenizer::T_INT).Throw();
		t.AssertSymbol('}');
		t.Next();
	}

	//=================================================================================================
	// Update crc using item
	void UpdateCrc(CRC32& crc, TypeItem* item, uint offset) override
	{
		auto& building = item->To<Building>();
		crc.Update(building.shift);
	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset) override
	{
		auto& building1 = item1->To<Building>();
		auto& building2 = item2->To<Building>();

		return memcmp(&building1.shift, &building2.shift, sizeof(Building::shift)) == 0;
	}

	void Copy(TypeItem* _from, TypeItem* _to, uint offset) override
	{
		auto& from = _from->To<Building>();
		auto& to = _to->To<Building>();

		memcpy(&to.shift, &from.shift, sizeof(Building::shift));
	}
};

class BuildingHandler : public TypeImpl<Building>
{
public:
	BuildingHandler() : TypeImpl(TypeId::Building, "building", "Building", "buildings")
	{
		DependsOn(TypeId::BuildingGroup);
		DependsOn(TypeId::Unit);

		AddId(offsetof(Building, id));
		AddMesh("mesh", offsetof(Building, mesh_id), offsetof(Building, mesh));
		AddMesh("inside_mesh", offsetof(Building, inside_mesh_id), offsetof(Building, inside_mesh))
			.NotRequired()
			.FriendlyName("Inside mesh");
		AddFlags("flags", offsetof(Building, flags), {
			{ "favor_center", Building::FAVOR_CENTER, "Favor center" },
			{ "favor_road", Building::FAVOR_ROAD, "Favor road" },
			{ "have_name", Building::HAVE_NAME, "Have name" },
			{ "list", Building::LIST, "List" }
		}).NotRequired();
		AddCustomField("scheme", new BuildingSchemeHandler);
		AddReference("group", TypeId::BuildingGroup, offsetof(Building, group))
			.NotRequired()
			.Callback(0);
		AddReference("unit", TypeId::Unit, offsetof(Building, unit))
			.NotRequired();
		AddCustomField("shift", new BuildingShiftHandler(this))
			.NotRequired();

		AddLocalizedString("name", offsetof(Building, name));

		container = new TypeVectorContainer(this, content::buildings);
	}

	void ReferenceCallback(TypeItem* item, TypeItem* ref_item, int type) override
	{
		Building* building = (Building*)item;
		BuildingGroup* group = (BuildingGroup*)ref_item;
		group->buildings.push_back(building);
	}

	/*void Insert(GameTypeItem* item) override
	{
		// set 1 as name to disable missing text warning
		Building* building = (Building*)item;
		building->name = "1";
		SimpleGameTypeHandler::Insert(item);
	}*/
};

Type* CreateBuildingHandler()
{
	return new BuildingHandler;
}
