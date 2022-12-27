#include "Pch.h"
#include "BuildingLoader.h"

#include "Building.h"
#include "BuildingGroup.h"
#include "BuildingScript.h"
#include "UnitData.h"

#include <Mesh.h>
#include <ResourceManager.h>

enum Group
{
	G_TOP,
	G_BUILDING,
	G_BUILDING_FLAGS,
	G_SIDE,
	G_SCRIPT,
	G_SCRIPT2
};

enum Top
{
	T_BUILDING,
	T_BUILDING_GROUP,
	T_BUILDING_SCRIPT
};

enum BuildingKeyword
{
	BK_MESH,
	BK_INSIDE_MESH,
	BK_FLAGS,
	BK_SCHEME,
	BK_GROUP,
	BK_UNIT,
	BK_SHIFT
};

enum Side
{
	S_BOTTOM,
	S_RIGHT,
	S_TOP,
	S_LEFT
};

enum ScriptKeyword
{
	SK_VARIANT,
	SK_SHUFFLE,
	SK_REQUIRED,
	SK_RAND,
	SK_RANDOM,
	SK_VAR,
	SK_SET,
	SK_INC,
	SK_DEC,
	SK_IF,
	SK_ELSE,
	SK_ENDIF,
	SK_GROUP
};

enum ScriptKeyword2
{
	SK2_OFF,
	SK2_START,
	SK2_END
};

struct Var
{
	string name;
	int index;
	bool isConst;
};

//=================================================================================================
BuildingLoader::BuildingLoader() : ContentLoader(Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_HIDE_ID)
{
}

//=================================================================================================
void BuildingLoader::DoLoading()
{
	Load("buildings.txt", G_TOP);
}

//=================================================================================================
void BuildingLoader::Cleanup()
{
	DeleteElements(BuildingScript::scripts);
	DeleteElements(BuildingGroup::groups);
	DeleteElements(Building::buildings);
}

//=================================================================================================
void BuildingLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "building", T_BUILDING },
		{ "buildingGroup", T_BUILDING_GROUP },
		{ "buildingScript", T_BUILDING_SCRIPT }
		});

	t.AddKeywords(G_BUILDING, {
		{ "mesh", BK_MESH },
		{ "insideMesh", BK_INSIDE_MESH },
		{ "flags", BK_FLAGS },
		{ "scheme", BK_SCHEME },
		{ "group", BK_GROUP },
		{ "unit", BK_UNIT },
		{ "shift", BK_SHIFT }
		});

	t.AddKeywords(G_BUILDING_FLAGS, {
		{ "favorCenter", Building::FAVOR_CENTER },
		{ "favorRoad", Building::FAVOR_ROAD },
		{ "haveName", Building::HAVE_NAME },
		{ "list", Building::LIST },
		{ "favorDist", Building::FAVOR_DIST },
		{ "noPath", Building::NO_PATH }
		});

	t.AddKeywords(G_SIDE, {
		{ "bottom", S_BOTTOM },
		{ "top", S_TOP },
		{ "left", S_LEFT },
		{ "right", S_RIGHT }
		});

	t.AddKeywords(G_SCRIPT, {
		{ "variant", SK_VARIANT },
		{ "shuffle", SK_SHUFFLE },
		{ "required", SK_REQUIRED },
		{ "rand", SK_RAND },
		{ "random", SK_RANDOM },
		{ "var", SK_VAR },
		{ "set", SK_SET },
		{ "inc", SK_INC },
		{ "dec", SK_DEC },
		{ "if", SK_IF },
		{ "else", SK_ELSE },
		{ "endif", SK_ENDIF },
		{ "group", SK_GROUP }
		});

	t.AddKeywords(G_SCRIPT2, {
		{ "off", SK2_OFF },
		{ "start", SK2_START },
		{ "end", SK2_END }
		});
}

//=================================================================================================
void BuildingLoader::LoadEntity(int top, const string& id)
{
	switch(top)
	{
	case T_BUILDING:
		ParseBuilding(id);
		break;
	case T_BUILDING_GROUP:
		ParseBuildingGroup(id);
		break;
	case T_BUILDING_SCRIPT:
		ParseBuildingScript(id);
		break;
	}
}

//=================================================================================================
void BuildingLoader::Finalize()
{
	SetupBuildingGroups();
	CalculateCrc();

	Info("Loaded buildings (%u), groups (%u), scripts (%u) - crc %p.", Building::buildings.size(), BuildingGroup::groups.size(),
		BuildingScript::scripts.size(), content.crc[(int)Content::Id::Buildings]);
}

//=================================================================================================
void BuildingLoader::ParseBuilding(const string& id)
{
	Building* existingBuilding = Building::TryGet(id);
	if(existingBuilding)
		t.Throw("Id must be unique.");

	Ptr<Building> building;
	building->id = id;
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		auto prop = (BuildingKeyword)t.MustGetKeywordId(G_BUILDING);
		t.Next();

		switch(prop)
		{
		case BK_MESH:
			{
				const string& meshId = t.MustGetString();
				building->mesh = resMgr->TryGet<Mesh>(meshId);
				if(!building->mesh)
					LoadError("Missing mesh '%s'.", meshId.c_str());
				t.Next();
			}
			break;
		case BK_INSIDE_MESH:
			{
				const string& meshId = t.MustGetString();
				building->insideMesh = resMgr->TryGet<Mesh>(meshId);
				if(!building->insideMesh)
					LoadError("Missing mesh '%s'.", meshId.c_str());
				t.Next();
			}
			break;
		case BK_FLAGS:
			t.ParseFlags(G_BUILDING_FLAGS, building->flags);
			t.Next();
			break;
		case BK_SCHEME:
			{
				building->scheme.clear();
				t.AssertSymbol('{');
				t.Next();
				Int2 size(0, 0);
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
						building->scheme.push_back(s);
					}
					++size.y;
					t.Next();
				}
				t.Next();
				building->size = size;
			}
			break;
		case BK_GROUP:
			{
				const string& groupId = t.MustGetItem();
				auto group = BuildingGroup::TryGet(groupId);
				if(!group)
				{
					Warn("Missing group '%s' for building '%s'.", groupId.c_str(), building->id.c_str());
					content.warnings++;
				}
				else
					building->group = group;
				t.Next();
			}
			break;
		case BK_UNIT:
			{
				const string& unitId = t.MustGetItem();
				auto unit = UnitData::TryGet(unitId.c_str());
				if(!unit)
				{
					Warn("Missing unit '%s' for building '%s'.", unitId.c_str(), building->id.c_str());
					content.warnings++;
				}
				else
					building->unit = unit;
				t.Next();
			}
			break;
		case BK_SHIFT:
			{
				t.AssertSymbol('{');
				t.Next();
				if(t.IsKeywordGroup(G_SIDE))
				{
					do
					{
						Side side = (Side)t.MustGetKeywordId(G_SIDE);
						t.Next();
						t.Parse(building->shift[(int)side]);
					}
					while(t.IsKeywordGroup(G_SIDE));
				}
				else if(t.IsInt())
				{
					int shiftX = t.GetInt();
					t.Next();
					int shiftY = t.MustGetInt();
					t.Next();
					for(int i = 0; i < 4; ++i)
					{
						building->shift[i].x = shiftX;
						building->shift[i].y = shiftY;
					}
				}
				else
				{
					int group = G_SIDE;
					t.StartUnexpected().Add(tokenizer::T_KEYWORD_GROUP, &group).Add(tokenizer::T_INT).Throw();
				}
				t.AssertSymbol('}');
				t.Next();
			}
			break;
		}
	}

	if(!building->mesh)
		t.Throw("Building '%s' is missing mesh.", building->id.c_str());
	if(building->scheme.empty())
		t.Throw("Building '%s' is missing scheme.", building->id.c_str());

	if(building->group)
		building->group->buildings.push_back(building.Get());
	Building::buildings.push_back(building.Pin());
}

//=================================================================================================
void BuildingLoader::ParseBuildingGroup(const string& id)
{
	BuildingGroup* existingBuildingGroup = BuildingGroup::TryGet(id);
	if(existingBuildingGroup)
		t.Throw("Id must be unique.");

	BuildingGroup* buildingGroup = new BuildingGroup;
	buildingGroup->id = id;
	BuildingGroup::groups.push_back(buildingGroup);
}

//=================================================================================================
void BuildingLoader::ParseBuildingScript(const string& id)
{
	BuildingScript* existingBuildingScript = BuildingScript::TryGet(id);
	if(existingBuildingScript)
		t.Throw("Id must be unique.");
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	Ptr<BuildingScript> script;
	script->id = id;
	DeleteElements(script->variants);
	script->requiredOffset = (uint)-1;
	variant = nullptr;
	code = nullptr;

	ParseCode(*script.Get());

	BuildingScript::scripts.push_back(script.Pin());
}

//=================================================================================================
void BuildingLoader::ParseCode(BuildingScript& script)
{
	vector<bool> ifState; // false - if block, true - else block
	bool inlineVariant = false,
		inShuffle = false;

	while(!t.IsEof())
	{
		if(t.IsSymbol('}'))
		{
			if(variant && !inlineVariant)
			{
				if(code->empty())
					t.Throw("Empty code for variant %d.", variant->index + 1);
				variant->vars = vars.size();
				variant = nullptr;
				code = nullptr;
				t.Next();
			}
			else
				break;
		}

		if(t.IsKeywordGroup(G_SCRIPT))
		{
			ScriptKeyword k = (ScriptKeyword)t.GetKeywordId(G_SCRIPT);
			t.Next();

			if(k == SK_VARIANT)
			{
				if(variant)
					t.Throw("Already in variant block.");
				StartVariant(script);

				t.AssertSymbol('{');
				t.Next();
				continue;
			}

			if(!variant)
			{
				if(script.variants.empty())
				{
					StartVariant(script);
					inlineVariant = true;
				}
				else
					t.Throw("Code outside variant block.");
			}

			switch(k)
			{
			case SK_SHUFFLE:
				if(t.IsKeyword(SK2_START, G_SCRIPT2))
				{
					if(inShuffle)
						t.Throw("Shuffle already started.");
					code->push_back(BuildingScript::BS_SHUFFLE_START);
					inShuffle = true;
					t.Next();
				}
				else if(t.IsKeyword(SK2_END, G_SCRIPT2))
				{
					if(!inShuffle)
						t.Throw("Shuffle not started.");
					code->push_back(BuildingScript::BS_SHUFFLE_END);
					inShuffle = false;
					t.Next();
				}
				else
					t.Unexpected();
				break;
			case SK_REQUIRED:
				if(!ifState.empty())
					t.Throw("Required off can't be used inside if else section.");
				if(script.requiredOffset != (uint)-1)
					t.Throw("Required already turned off.");
				t.AssertKeyword(SK2_OFF, G_SCRIPT2);
				code->push_back(BuildingScript::BS_REQUIRED_OFF);
				script.requiredOffset = code->size();
				t.Next();
				break;
			case SK_RANDOM:
				{
					t.AssertSymbol('{');
					t.Next();
					code->push_back(BuildingScript::BS_RANDOM);
					uint pos = code->size(), items = 0;
					code->push_back(0);
					while(!t.IsSymbol('}'))
					{
						if(t.IsKeyword(SK_GROUP, G_SCRIPT2))
						{
							t.Next();
							const string& id = t.MustGetItem();
							BuildingGroup* group = BuildingGroup::TryGet(id);
							if(!group)
								t.Throw("Missing building group '%s'.", id.c_str());
							if(group->buildings.empty())
								t.Throw("Group with no buildings '%s'.", id.c_str());
							t.Next();
							code->push_back(BuildingScript::BS_GROUP);
							code->push_back((int)group);
							++items;
						}
						else
						{
							const string& id = t.MustGetItem();
							Building* b = Building::TryGet(id);
							if(!b)
								t.Throw("Missing building '%s'.", id.c_str());
							t.Next();
							code->push_back(BuildingScript::BS_BUILDING);
							code->push_back((int)b);
							++items;
						}
					}
					t.Next();
					if(items < 2)
						t.Throw("Random with %u items.", items);
					code->at(pos) = items;
				}
				break;
			case SK_VAR:
				{
					const string& id = t.MustGetItem();
					Var* v = FindVar(id);
					if(v)
						t.Throw("Variable '%s' already declared.", id.c_str());
					if(vars.size() >= BuildingScript::MAX_VARS)
						t.Throw("Can't declare more variables.");
					AddVar(id);
					t.Next();
				}
				break;
			case SK_SET:
				{
					// var
					Var& v = GetVar();

					// = or += -= *= /=
					char op = t.MustGetSymbol("+-*/=");
					t.Next();
					if(op != '=')
					{
						t.AssertSymbol('=');
						t.Next();
						code->push_back(BuildingScript::BS_PUSH_VAR);
						code->push_back(v.index);
					}

					// value
					GetExpr();

					if(op != '=')
						code->push_back(CharToOp(op));
					code->push_back(BuildingScript::BS_SET_VAR);
					code->push_back(v.index);
				}
				break;
			case SK_INC:
			case SK_DEC:
				{
					Var& v = GetVar();
					code->push_back(k == SK_INC ? BuildingScript::BS_INC : BuildingScript::BS_DEC);
					code->push_back(v.index);
				}
				break;
			case SK_IF:
				if(t.IsKeyword(SK_RAND, G_SCRIPT))
				{
					t.Next();
					GetExpr();
					code->push_back(BuildingScript::BS_IF_RAND);
				}
				else
				{
					GetExpr();
					char c;
					if(!t.IsSymbol("><!=", &c))
						t.Unexpected();
					BuildingScript::Code symbol;
					switch(c)
					{
					case '>':
						if(t.PeekSymbol('='))
							symbol = BuildingScript::BS_GREATER_EQUAL;
						else
							symbol = BuildingScript::BS_GREATER;
						break;
					case '<':
						if(t.PeekSymbol('='))
							symbol = BuildingScript::BS_LESS_EQUAL;
						else
							symbol = BuildingScript::BS_LESS;
						break;
					case '!':
						if(!t.PeekSymbol('='))
							t.Unexpected();
						symbol = BuildingScript::BS_NOT_EQUAL;
						break;
					case '=':
						if(!t.PeekSymbol('='))
							t.Unexpected();
						symbol = BuildingScript::BS_EQUAL;
						break;
					}
					t.Next();
					GetExpr();
					code->push_back(BuildingScript::BS_IF);
					code->push_back(symbol);
				}
				ifState.push_back(false);
				break;
			case SK_ELSE:
				if(ifState.empty() || ifState.back() == true)
					t.Unexpected();
				code->push_back(BuildingScript::BS_ELSE);
				ifState.back() = true;
				break;
			case SK_ENDIF:
				if(ifState.empty())
					t.Unexpected();
				code->push_back(BuildingScript::BS_ENDIF);
				ifState.pop_back();
				break;
			case SK_GROUP:
				{
					const string& id = t.MustGetItem();
					BuildingGroup* group = BuildingGroup::TryGet(id);
					if(!group)
						t.Throw("Missing building group '%s'.", id.c_str());
					if(group->buildings.empty())
						t.Throw("Group with no buildings '%s'.", id.c_str());
					t.Next();
					code->push_back(BuildingScript::BS_ADD_BUILDING);
					code->push_back(BuildingScript::BS_GROUP);
					code->push_back((int)group);
				}
				break;
			default:
				t.Unexpected();
				break;
			}
		}
		else if(t.IsItem())
		{
			if(!variant)
			{
				if(script.variants.empty())
				{
					StartVariant(script);
					inlineVariant = true;
				}
				else
					t.Throw("Code outside variant block.");
			}

			const string& id = t.MustGetItem();
			Building* b = Building::TryGet(id);
			if(!b)
				t.Throw("Missing building '%s'.", id.c_str());
			t.Next();
			code->push_back(BuildingScript::BS_ADD_BUILDING);
			code->push_back(BuildingScript::BS_BUILDING);
			code->push_back((int)b);
		}
		else
			t.Unexpected();
	}

	if(inShuffle)
		t.Throw("Missing end shuffle.");
	else if(!ifState.empty())
		t.Throw("Unclosed if/else block.");
	else if(script.variants.empty())
		t.Throw("Empty building script.");
	else if(script.requiredOffset == (uint)-1)
		t.Throw("Missing not required section.");
}

//=================================================================================================
void BuildingLoader::StartVariant(BuildingScript& script)
{
	variant = new BuildingScript::Variant;
	variant->index = script.variants.size();
	variant->vars = 0;
	script.variants.push_back(variant);
	code = &variant->code;

	vars.clear();
	AddVar("count");
	AddVar("counter");
	AddVar("citizens");
	AddVar("citizens_world");
	AddVar("first_city", true);
}

//=================================================================================================
void BuildingLoader::AddVar(Cstring id, bool isConst)
{
	Var v;
	v.name = id.s;
	v.isConst = isConst;
	v.index = vars.size();
	vars.push_back(v);
}

//=================================================================================================
BuildingLoader::Var* BuildingLoader::FindVar(const string& id)
{
	for(Var& v : vars)
	{
		if(v.name == id)
			return &v;
	}
	return nullptr;
}

//=================================================================================================
BuildingLoader::Var& BuildingLoader::GetVar(bool canBeConst)
{
	const string& id = t.MustGetItem();
	Var* v = FindVar(id);
	if(!v)
		t.Throw("Missing variable '%s'.", id.c_str());
	if(v->isConst && !canBeConst)
		t.Throw("Variable '%s' cannot be modified.", id.c_str());
	t.Next();
	return *v;
}

//=================================================================================================
void BuildingLoader::GetExpr()
{
	GetItem();

	while(true)
	{
		char c;
		if(!t.IsSymbol("+-*/", &c))
			return;
		t.Next();

		GetItem();

		code->push_back(CharToOp(c));
	}
}

//=================================================================================================
void BuildingLoader::GetItem()
{
	bool neg = false;
	if(t.IsSymbol('-'))
	{
		neg = true;
		t.Next();
	}

	if(t.IsKeyword(SK_RANDOM, G_SCRIPT))
	{
		// function
		t.Next();
		t.AssertSymbol('(');
		t.Next();
		GetExpr();
		t.AssertSymbol(',');
		t.Next();
		GetExpr();
		t.AssertSymbol(')');
		t.Next();
		code->push_back(BuildingScript::BS_CALL);
	}
	else if(t.IsInt())
	{
		// int
		code->push_back(BuildingScript::BS_PUSH_INT);
		int value = t.GetInt();
		t.Next();
		if(neg)
		{
			value = -value;
			neg = false;
		}
		code->push_back(value);
	}
	else if(t.IsItem())
	{
		// var ?
		Var& v = GetVar(true);
		code->push_back(BuildingScript::BS_PUSH_VAR);
		code->push_back(v.index);
	}
	else
		t.Unexpected();

	if(neg)
		code->push_back(BuildingScript::BS_NEG);
}

//=================================================================================================
int BuildingLoader::CharToOp(char c)
{
	switch(c)
	{
	default:
	case '+':
		return BuildingScript::BS_ADD;
	case '-':
		return BuildingScript::BS_SUB;
	case '*':
		return BuildingScript::BS_MUL;
	case '/':
		return BuildingScript::BS_DIV;
	}
}

//=================================================================================================
void BuildingLoader::SetupBuildingGroups()
{
	BuildingGroup::BG_INN = BuildingGroup::Get("inn");
	BuildingGroup::BG_HALL = BuildingGroup::Get("hall");
	BuildingGroup::BG_TRAINING_GROUNDS = BuildingGroup::Get("training_grounds");
	BuildingGroup::BG_ARENA = BuildingGroup::Get("arena");
	BuildingGroup::BG_FOOD_SELLER = BuildingGroup::Get("food_seller");
	BuildingGroup::BG_ALCHEMIST = BuildingGroup::Get("alchemist");
	BuildingGroup::BG_BLACKSMITH = BuildingGroup::Get("blacksmith");
	BuildingGroup::BG_MERCHANT = BuildingGroup::Get("merchant");
}

//=================================================================================================
void BuildingLoader::CalculateCrc()
{
	Crc crc;

	for(auto group : BuildingGroup::groups)
		crc.Update(group->id);

	for(auto building : Building::buildings)
	{
		crc.Update(building->id);
		crc.Update(building->mesh->filename);
		crc.Update(building->size);
		crc.Update(building->shift);
		crc.Update(building->scheme);
		crc.Update(building->flags);
		if(building->insideMesh)
			crc.Update(building->insideMesh->filename);
		if(building->group)
			crc.Update(building->group->id);
		if(building->unit)
			crc.Update(building->unit->id);
	}

	for(auto script : BuildingScript::scripts)
	{
		crc.Update(script->id);
		crc.Update(script->variants.size());
		for(auto variant : script->variants)
		{
			for(int* data = variant->code.data(), *end = variant->code.data() + variant->code.size(); data != end;)
			{
				int op = *data;
				++data;
				crc.Update(op);
				switch(op)
				{
				case BuildingScript::BS_ADD_BUILDING:
					{
						int type = *data;
						crc.Update(type);
						++data;
						int ptr = *data;
						if(type == BuildingScript::BS_BUILDING)
							crc.Update(reinterpret_cast<Building*>(ptr)->id);
						else
							crc.Update(reinterpret_cast<BuildingGroup*>(ptr)->id);
						++data;
					}
					break;
				case BuildingScript::BS_RANDOM:
					{
						uint count = (uint)*data;
						crc.Update(count);
						++data;
						for(uint i = 0; i < count; ++i)
						{
							int type = *data;
							crc.Update(type);
							++data;
							int ptr = *data;
							if(type == BuildingScript::BS_BUILDING)
								crc.Update(reinterpret_cast<Building*>(ptr)->id);
							else
								crc.Update(reinterpret_cast<BuildingGroup*>(ptr)->id);
							++data;
						}
					}
					break;
				case BuildingScript::BS_PUSH_INT:
				case BuildingScript::BS_PUSH_VAR:
				case BuildingScript::BS_SET_VAR:
				case BuildingScript::BS_INC:
				case BuildingScript::BS_DEC:
				case BuildingScript::BS_IF:
					crc.Update(*data);
					++data;
					break;
				}
			}
		}
	}

	content.crc[(int)Content::Id::Buildings] = crc.Get();
}
