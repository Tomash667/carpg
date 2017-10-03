#include "Pch.h"
#include "Core.h"
#include "ContentLoader.h"
#include "BuildingGroup.h"
#include "Building.h"
#include "BuildingScript.h"
#include "UnitData.h"
#include "Crc.h"

//-----------------------------------------------------------------------------
uint content::buildings_crc;

//=================================================================================================
class BuildingLoader
{
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

public:
	//=================================================================================================
	BuildingLoader() : t(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE | Tokenizer::F_HIDE_ID)
	{
	}

	//=================================================================================================
	void Load()
	{
		InitTokenizer();

		ContentLoader loader;
		bool ok = loader.Load(t, "buildings.txt", G_TOP, [this](int top, const string& id)
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
		});
		if(!ok)
			return;

		SetupBuildingGroups();
		CalculateCrc();

		Info("Loaded buildings (%u), groups (%u), scripts (%u) - crc %p.", Building::buildings.size(), BuildingGroup::groups.size(),
			BuildingScript::scripts.size(), content::buildings_crc);
	}

private:
	struct Var
	{
		string name;
		int index;
		bool is_const;
	};

	//=================================================================================================
	void InitTokenizer()
	{
		t.AddKeywords(G_TOP, {
			{ "building", T_BUILDING },
			{ "building_group", T_BUILDING_GROUP },
			{ "building_script", T_BUILDING_SCRIPT }
		});

		t.AddKeywords(G_BUILDING, {
			{ "mesh", BK_MESH },
			{ "inside_mesh", BK_INSIDE_MESH },
			{ "flags", BK_FLAGS },
			{ "scheme", BK_SCHEME },
			{ "group", BK_GROUP },
			{ "unit", BK_UNIT },
			{ "shift", BK_SHIFT }
		});

		t.AddKeywords(G_BUILDING_FLAGS, {
			{ "favor_center", Building::FAVOR_CENTER },
			{ "favor_road", Building::FAVOR_ROAD },
			{ "have_name", Building::HAVE_NAME },
			{ "list", Building::LIST }
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
	void ParseBuilding(const string& id)
	{
		auto existing_building = Building::TryGet(id);
		if(existing_building)
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
				building->mesh_id = t.MustGetString();
				t.Next();
				break;
			case BK_INSIDE_MESH:
				building->inside_mesh_id = t.MustGetString();
				t.Next();
				break;
			case BK_FLAGS:
				building->flags = ReadFlags(t, G_BUILDING_FLAGS);
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
					const string& group_id = t.MustGetItem();
					auto group = BuildingGroup::TryGet(group_id);
					if(!group)
					{
						Warn("Missing group '%s' for building '%s'.", group_id.c_str(), building->id.c_str());
						content::warnings++;
					}
					else
						building->group = group;
					t.Next();
				}
				break;
			case BK_UNIT:
				{
					const string& unit_id = t.MustGetItem();
					auto unit = FindUnitData(unit_id.c_str(), false);
					if(!unit)
					{
						Warn("Missing unit '%s' for building '%s'.", unit_id.c_str(), building->id.c_str());
						content::warnings++;
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
							building->shift[i].x = shift_x;
							building->shift[i].y = shift_y;
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

		if(building->mesh_id.empty())
			t.Throw("Building '%s' is missing mesh.", building->id.c_str());
		if(building->scheme.empty())
			t.Throw("Building '%s' is missing scheme.", building->id.c_str());

		if(building->group)
			building->group->buildings.push_back(building.Get());
		Building::buildings.push_back(building.Pin());
	}

	//=================================================================================================
	void ParseBuildingGroup(const string& id)
	{
		auto existing_building_group = BuildingGroup::TryGet(id);
		if(existing_building_group)
			t.Throw("Id must be unique.");

		auto building_group = new BuildingGroup;
		building_group->id = id;
		BuildingGroup::groups.push_back(building_group);
	}

	//=================================================================================================
	void ParseBuildingScript(const string& id)
	{
		auto existing_building_script = BuildingScript::TryGet(id);
		if(existing_building_script)
			t.Throw("Id must be unique.");
		t.Next();

		t.AssertSymbol('{');
		t.Next();

		Ptr<BuildingScript> script;
		script->id = id;
		DeleteElements(script->variants);
		script->required_offset = (uint)-1;
		variant = nullptr;
		code = nullptr;

		ParseCode(*script.Get());

		BuildingScript::scripts.push_back(script.Pin());
	}

	//=================================================================================================
	void ParseCode(BuildingScript& script)
	{
		vector<bool> if_state; // false - if block, true - else block
		bool inline_variant = false,
			in_shuffle = false;

		while(!t.IsEof())
		{
			if(t.IsSymbol('}'))
			{
				if(variant && !inline_variant)
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
						inline_variant = true;
					}
					else
						t.Throw("Code outside variant block.");
				}

				switch(k)
				{
				case SK_SHUFFLE:
					if(t.IsKeyword(SK2_START, G_SCRIPT2))
					{
						if(in_shuffle)
							t.Throw("Shuffle already started.");
						code->push_back(BuildingScript::BS_SHUFFLE_START);
						in_shuffle = true;
						t.Next();
					}
					else if(t.IsKeyword(SK2_END, G_SCRIPT2))
					{
						if(!in_shuffle)
							t.Throw("Shuffle not started.");
						code->push_back(BuildingScript::BS_SHUFFLE_END);
						in_shuffle = false;
						t.Next();
					}
					else
						t.Unexpected();
					break;
				case SK_REQUIRED:
					if(!if_state.empty())
						t.Throw("Required off can't be used inside if else section.");
					if(script.required_offset != (uint)-1)
						t.Throw("Required already turned off.");
					t.AssertKeyword(SK2_OFF, G_SCRIPT2);
					code->push_back(BuildingScript::BS_REQUIRED_OFF);
					script.required_offset = code->size();
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
					if_state.push_back(false);
					break;
				case SK_ELSE:
					if(if_state.empty() || if_state.back() == true)
						t.Unexpected();
					code->push_back(BuildingScript::BS_ELSE);
					if_state.back() = true;
					break;
				case SK_ENDIF:
					if(if_state.empty())
						t.Unexpected();
					code->push_back(BuildingScript::BS_ENDIF);
					if_state.pop_back();
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
				}
			}
			else if(t.IsItem())
			{
				if(!variant)
				{
					if(script.variants.empty())
					{
						StartVariant(script);
						inline_variant = true;
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

		if(in_shuffle)
			t.Throw("Missing end shuffle.");
		else if(!if_state.empty())
			t.Throw("Unclosed if/else block.");
		else if(script.variants.empty())
			t.Throw("Empty building script.");
		else if(script.required_offset == (uint)-1)
			t.Throw("Missing not required section.");
	}

	//=================================================================================================
	void StartVariant(BuildingScript& script)
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
	}

	//=================================================================================================
	void AddVar(const AnyString& id, bool is_const = false)
	{
		Var v;
		v.name = id.s;
		v.is_const = is_const;
		v.index = vars.size();
		vars.push_back(v);
	}

	//=================================================================================================
	Var* FindVar(const string& id)
	{
		for(Var& v : vars)
		{
			if(v.name == id)
				return &v;
		}
		return nullptr;
	}

	//=================================================================================================
	Var& GetVar(bool can_be_const = false)
	{
		const string& id = t.MustGetItem();
		Var* v = FindVar(id);
		if(!v)
			t.Throw("Missing variable '%s'.", id.c_str());
		if(v->is_const && !can_be_const)
			t.Throw("Variable '%s' cannot be modified.", id.c_str());
		t.Next();
		return *v;
	}

	//=================================================================================================
	void GetExpr()
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
	void GetItem()
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
	int CharToOp(char c)
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
	void SetupBuildingGroups()
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
	void CalculateCrc()
	{
		Crc crc;

		for(auto group : BuildingGroup::groups)
			crc.Update(group->id);

		for(auto building : Building::buildings)
		{
			crc.Update(building->id);
			crc.Update(building->mesh_id);
			crc.Update(building->inside_mesh_id);
			crc.Update(building->size);
			crc.Update(building->shift);
			crc.Update(building->scheme);
			crc.Update(building->flags);
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
								crc.Update(((Building*)ptr)->id);
							else
								crc.Update(((BuildingGroup*)ptr)->id);
							++data;
						}
						break;
					case BuildingScript::BS_RANDOM:
						{
							uint count = (uint)*data;
							crc.Update(count);
							++data;
							for(uint i = 0; i<count; ++i)
							{
								int type = *data;
								crc.Update(type);
								++data;
								int ptr = *data;
								if(type == BuildingScript::BS_BUILDING)
									crc.Update(((Building*)ptr)->id);
								else
									crc.Update(((BuildingGroup*)ptr)->id);
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

		content::buildings_crc = crc.Get();
	}

	BuildingScript::Variant* variant;
	vector<int>* code;
	vector<Var> vars;
	Tokenizer t;
};

//=================================================================================================
void content::LoadBuildings()
{
	BuildingLoader loader;
	loader.Load();
}

//=================================================================================================
void content::CleanupBuildings()
{
	DeleteElements(BuildingScript::scripts);
	DeleteElements(BuildingGroup::groups);
	DeleteElements(Building::buildings);
}
