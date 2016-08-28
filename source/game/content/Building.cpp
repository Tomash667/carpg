#include "Pch.h"
#include "Base.h"
#include "Building.h"
#include "BuildingGroup.h"
#include "Content.h"
#include "DatatypeManager.h"

//-----------------------------------------------------------------------------
vector<Building*> content::buildings;

//=================================================================================================
// Find building by id
//=================================================================================================
Building* content::FindBuilding(AnyString id)
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
struct BuildingSchemeHandler : public CustomFieldHandler
{
	//=================================================================================================
	// Load scheme from text file
	void LoadText(Tokenizer& t, DatatypeItem item) override
	{
		Building& b = *(Building*)item;

		b.scheme.clear();
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
				b.scheme.push_back(s);
			}
			++size.y;
			t.Next();
		}
		t.Next();
		b.size = size;
	}

	//=================================================================================================
	// Update crc using item
	void UpdateCrc(CRC32& crc, DatatypeItem item) override
	{
		Building& b = *(Building*)item;
		crc.UpdateVector(b.scheme);
	}
};

//=================================================================================================
// Building shift type handler
//=================================================================================================
struct BuildingShiftHandler : public CustomFieldHandler
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
	BuildingShiftHandler(DatatypeManager& dt_mgr)
	{
		group = dt_mgr.AddKeywords({
			{ "bottom",S_BOTTOM },
			{ "top",S_TOP },
			{ "left",S_LEFT },
			{ "right",S_RIGHT }
		}, "building side");
	}

	//=================================================================================================
	// Load shift from text file
	void LoadText(Tokenizer& t, DatatypeItem item) override
	{
		Building& b = *(Building*)item;

		t.AssertSymbol('{');
		t.Next();
		if(t.IsKeywordGroup(group))
		{
			do
			{
				Side side = (Side)t.MustGetKeywordId(group);
				t.Next();
				t.Parse(b.shift[(int)side]);
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
				b.shift[i].x = shift_x;
				b.shift[i].y = shift_y;
			}
		}
		else
			t.StartUnexpected().Add(tokenizer::T_KEYWORD_GROUP, &group).Add(tokenizer::T_INT).Throw();
		t.AssertSymbol('}');
		t.Next();
	}

	//=================================================================================================
	// Update crc using item
	void UpdateCrc(CRC32& crc, DatatypeItem item) override
	{
		Building& b = *(Building*)item;
		crc.Update(b.shift);
	}
};

class BuildingHandler : public SimpleDatatypeHandler<Building>
{
public:
	BuildingHandler() : SimpleDatatypeHandler(content::buildings, offsetof(Building, id))
	{

	}

	void Callback(DatatypeItem item, DatatypeItem ref_item, int type) override
	{
		Building* building = (Building*)item;
		BuildingGroup* group = (BuildingGroup*)ref_item;
		group->buildings.push_back(building);
	}

	void Insert(DatatypeItem item) override
	{
		// set 1 as name to disable missing text warning
		Building* building = (Building*)item;
		building->name = "1";
		SimpleDatatypeHandler::Insert(item);
	}
};

//=================================================================================================
// Register building datatype
//=================================================================================================
void Building::Register(DatatypeManager& dt_mgr)
{
	Datatype* dt = new Datatype(DT_Building, "building");
	dt->AddId(offsetof(Building, id));
	dt->AddMesh("mesh", offsetof(Building, mesh_id), offsetof(Building, mesh));
	dt->AddMesh("inside_mesh", offsetof(Building, inside_mesh_id), offsetof(Building, inside_mesh))
		.NotRequired();
	dt->AddFlags("flags", offsetof(Building, flags), dt_mgr.AddKeywords({
		{"favor_center", Building::FAVOR_CENTER},
		{"favor_road", Building::FAVOR_ROAD},
		{"have_name", Building::HAVE_NAME},
		{"list", Building::LIST}
	}, "building flags")).NotRequired();
	dt->AddCustomField("scheme", new BuildingSchemeHandler);
	dt->AddReference("group", DT_BuildingGroup, offsetof(Building, group))
		.NotRequired()
		.Callback(0);
	dt->AddReference("unit", DT_Unit, offsetof(Building, unit))
		.NotRequired();
	dt->AddCustomField("shift", new BuildingShiftHandler(dt_mgr))
		.NotRequired();
	dt->AddString("alias", offsetof(Building, alias))
		.NotRequired()
		.Alias();

	dt->AddLocalizedString("name", offsetof(Building, name));

	dt_mgr.Add(dt, new BuildingHandler);
}
