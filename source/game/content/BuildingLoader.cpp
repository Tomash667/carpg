#include "Pch.h"
#include "Base.h"
#include "BuildingLoader.h"
#include "Content.h"
#include "Building.h"

enum TopKeyword
{
	TOP_BUILDING_GROUPS,
	TOP_BUILDING
};

enum Keyword
{
	K_MESH,
	K_INSIDE_MESH,
	K_SCHEME,
	K_SHIFT,
	K_FLAGS,
	K_GROUP
};

enum Side
{
	S_BOTTOM,
	S_RIGHT,
	S_TOP,
	S_LEFT
};

enum KeywordGroup
{
	G_TOP,
	G_BUILDING,
	G_SIDE,
	G_FLAGS
};

BuildingLoader::BuildingLoader() : ContentLoader(ContentType::Building, "building")
{

}

BuildingLoader::~BuildingLoader()
{

}

void BuildingLoader::Init()
{
	t.AddKeywords(G_TOP, {
		{ "buildings_group", TOP_BUILDING_GROUPS },
		{ "building", TOP_BUILDING }
	});

	t.AddKeywords(G_BUILDING, {
		{ "mesh", K_MESH },
		{ "inside_mesh", K_INSIDE_MESH },
		{ "scheme", K_SCHEME },
		{ "shift", K_SHIFT },
		{ "flags", K_FLAGS },
		{ "group", K_GROUP }
	});

	t.AddKeywords(G_SIDE, {
		{ "bottom", S_BOTTOM },
		{ "right", S_RIGHT },
		{ "top", S_TOP },
		{ "left", S_LEFT }
	});

	t.AddKeywords(G_FLAGS, {
		{ "favor_center", Building::FAVOR_CENTER },
		{ "favor_road", Building::FAVOR_ROAD },
		{ "have_name", Building::HAVE_NAME }
	});
}

int BuildingLoader::Load()
{
	return t.ParseTop<TopKeyword>(G_TOP, [this] (TopKeyword top) {
		if(top == TOP_BUILDING_GROUPS)
			return LoadBuildingGroups();
		else
			return LoadBuilding();
	});
}

bool BuildingLoader::LoadBuilding()
{
	Building* b = new Building;

	try
	{
		// id
		const string& id = t.MustGetString();
		if(content::FindBuilding(id))
			t.Throw("Id already used.");
		b->id = id;
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			Keyword key = (Keyword)t.MustGetKeywordId(G_BUILDING);
			t.Next();

			switch(key)
			{
			case K_MESH:
				b->mesh_id = t.MustGetStringTrim();
				t.Next();
				break;
			case K_INSIDE_MESH:
				b->inside_mesh_id = t.MustGetStringTrim();
				t.Next();
				break;
			case K_SCHEME:
				{
					b->scheme.clear();
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
							TileScheme s;
							switch(line[i])
							{
							case ' ':
								s = SCHEME_GRASS;
								break;
							case '.':
								s = SCHEME_SAND;
								break;
							case '+':
								s = SCHEME_PATH;
								break;
							case '@':
								s = SCHEME_UNIT;
								break;
							case '|':
								s = SCHEME_BUILDING_PART;
								break;
							case 'O':
								s = SCHEME_BUILDING_NO_PHY;
								break;
							case '#':
								s = SCHEME_BUILDING;
								break;
							default:
								t.Throw("Unknown scheme tile '%c'.", line[i]);
							}
							b->scheme.push_back(s);
						}
						++size.y;
						t.Next();
					}
					t.Next();
					b->size = size;
				}
				break;
			case K_SHIFT:
				t.AssertSymbol('{');
				t.Next();
				if(t.IsKeywordGroup(G_SIDE))
				{
					do
					{
						Side side = (Side)t.MustGetKeywordId(G_SIDE);
						t.Next();
						t.Parse(b->shift[(int)side]);
					} while(t.IsKeywordGroup(G_SIDE));
				}
				else if(t.IsInt())
				{
					int shift_x = t.GetInt();
					t.Next();
					int shift_y = t.MustGetInt();
					t.Next();
					for(int i = 0; i < 4; ++i)
					{
						b->shift[i].x = shift_x;
						b->shift[i].y = shift_y;
					}
				}
				else
				{
					int group = G_SIDE;
					t.StartUnexpected().Add(Tokenizer::T_KEYWORD_GROUP, &group).Add(Tokenizer::T_INT).Throw();
				}
				t.AssertSymbol('}');
				t.Next();
				break;
			case K_FLAGS:
				b->flags = (Building::Flags)ReadFlags(t, G_FLAGS);
				t.Next();
				break;
			case K_GROUP:
				{
					const string& group = t.MustGetItemKeyword();
					b->group = content::FindBuildingGroup(group);
					if(b->group == -1)
						t.Throw("Missing building group '%s'.", group.c_str());
					t.Next();
				}
				break;
			}
		}

		content::buildings.push_back(b);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load %s: %s", GetItemString("building", b->id), e.ToString()));
		delete b;
		return false;
	}
}

bool BuildingLoader::LoadBuildingGroups()
{
	if(!content::building_groups.empty())
	{
		ERROR("Building groups already declared.");
		return false;
	}

	try
	{
		t.Next();
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			const string& id = t.MustGetItemKeyword();
			if(content::FindBuildingGroup(id) != -1)
				t.Throw("Id already used.");

			content::building_groups.push_back(id);
			t.Next();
		}

		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load building groups: %s", e.ToString()));
		return false;
	}
}

void BuildingLoader::Cleanup()
{
	DeleteElements(content::buildings);
	content::building_groups.clear();
}

Building* content::FindBuilding(AnyString id)
{
	for(Building* b : buildings)
	{
		if(b->id == id.s)
			return b;
	}
	return nullptr;
}

int content::FindBuildingGroup(AnyString id)
{
	int index = 0;
	for(string& group : building_groups)
	{
		if(group == id.s)
			return index;
		++index;
	}
	return -1;
}
