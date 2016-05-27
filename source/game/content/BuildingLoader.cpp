#include "Pch.h"
#include "Base.h"
#include "BuildingLoader.h"
#include "Content.h"
#include "Building.h"

//-----------------------------------------------------------------------------
enum KeywordGroup
{
	G_TOP,
	G_BUILDING,
	G_SIDE,
	G_FLAGS,
	G_SCRIPT,
	G_SCRIPT2
};

//-----------------------------------------------------------------------------
enum TopKeyword
{
	TOP_BUILDING_GROUPS,
	TOP_BUILDING,
	TOP_BUILDING_SCRIPT
};

//-----------------------------------------------------------------------------
enum Keyword
{
	K_MESH,
	K_INSIDE_MESH,
	K_SCHEME,
	K_SHIFT,
	K_FLAGS,
	K_GROUP,
	K_NAME,
	K_UNIT
};

//-----------------------------------------------------------------------------
enum Side
{
	S_BOTTOM,
	S_RIGHT,
	S_TOP,
	S_LEFT
};

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
enum ScriptKeyword2
{
	SK2_OFF,
	SK2_START,
	SK2_END
};

//=================================================================================================
BuildingLoader::BuildingLoader() : ContentLoader(ContentType::Building, "buildings", ContentLoader::HaveDatafile | ContentLoader::HaveTextfile)
{

}

//=================================================================================================
BuildingLoader::~BuildingLoader()
{

}

//=================================================================================================
void BuildingLoader::Init()
{
	// data tokenizer
	Tokenizer& t = data_tokenizer;
	t.SetFlags(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_JOIN_MINUS);

	t.AddKeywords(G_TOP, {
		{ "building_groups", TOP_BUILDING_GROUPS },
		{ "building", TOP_BUILDING },
		{ "building_script", TOP_BUILDING_SCRIPT }
	});

	t.AddKeywords(G_BUILDING, {
		{ "mesh", K_MESH },
		{ "inside_mesh", K_INSIDE_MESH },
		{ "scheme", K_SCHEME },
		{ "shift", K_SHIFT },
		{ "flags", K_FLAGS },
		{ "group", K_GROUP },
		{ "name", K_NAME },
		{ "unit", K_UNIT }
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
		{ "have_name", Building::HAVE_NAME },
		{ "list", Building::LIST }
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

	// text tokenizer
	Tokenizer& t2 = text_tokenizer;
	t2.AddKeyword("building", 0);
}

//=================================================================================================
void BuildingLoader::Cleanup()
{
	DeleteElements(content::buildings);
	content::building_groups.clear();
	DeleteElements(content::building_scripts);
}

//=================================================================================================
int BuildingLoader::Load()
{
	Tokenizer& t = data_tokenizer;

	int errors = t.ParseTop<TopKeyword>(G_TOP, [this, &t](TopKeyword top) {
		switch(top)
		{
		case TOP_BUILDING:
			return LoadBuilding(t);
		case TOP_BUILDING_GROUPS:
			return LoadBuildingGroups(t);
		case TOP_BUILDING_SCRIPT:
			return LoadBuildingScript(t);
		default:
			assert(0);
			return false;
		}
	});

	BG_INN = content::FindBuildingGroup("inn");
	BG_HALL = content::FindBuildingGroup("hall");
	BG_TRAINING_GROUNDS = content::FindBuildingGroup("training_grounds");
	BG_ARENA = content::FindBuildingGroup("arena");
	BG_FOOD_SELLER = content::FindBuildingGroup("food_seller");
	BG_ALCHEMIST = content::FindBuildingGroup("alchemist");
	BG_BLACKSMITH = content::FindBuildingGroup("blacksmith");
	BG_MERCHANT = content::FindBuildingGroup("merchant");

	return errors;
}

//=================================================================================================
bool BuildingLoader::LoadBuilding(Tokenizer& t)
{
	Building* b = new Building;

	try
	{
		// id
		const string& id = t.MustGetItemKeyword();
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
							case 'O':
								s = Building::SCHEME_BUILDING_NO_PHY;
								break;
							case '#':
								s = Building::SCHEME_BUILDING;
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
					if(!b->group)
						t.Throw("Missing building group '%s'.", group.c_str());
					t.Next();
				}
				break;
			case K_NAME:
				b->name = t.MustGetItem();
				t.Next();
				break;
			case K_UNIT:
				{
					const string& id = t.MustGetItem();
					b->unit = content::FindUnit(id);
					if(!b->unit)
						t.Throw("Missing unit '%s'.", id.c_str());
					t.Next();
				}
				break;
			}
		}

		if(b->group)
			b->group->buildings.push_back(b);
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

//=================================================================================================
bool BuildingLoader::LoadBuildingGroups(Tokenizer& t)
{
	if(!content::building_groups.empty())
	{
		ERROR("Building groups already declared.");
		return false;
	}

	try
	{
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			const string& id = t.MustGetItemKeyword();
			if(content::FindBuildingGroup(id))
				t.Throw("Id already used.");

			BuildingGroup& group = Add1(content::building_groups);
			group.id = id;
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

//=================================================================================================
bool BuildingLoader::LoadBuildingScript(Tokenizer& t)
{
	BuildingScript* script = new BuildingScript;
	vector<bool> if_state; // false - if block, true - else block

	try
	{
		// id
		const string& id = t.MustGetItemKeyword();
		if(content::FindBuildingScript(id))
			t.Throw("Id already used.");
		script->id = id;
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();
		script->variant = nullptr;
		code = nullptr;

		bool inline_variant = false, in_shuffle = false;

		while(true)
		{
			if(t.IsSymbol('}'))
			{
				if(in_shuffle)
					t.Throw("Missing end shuffle.");
				if(!if_state.empty())
					t.Throw("Unclosed if/else block.");
				if(script->variant)
				{
					if(code->empty())
						t.Throw(inline_variant ? "Empty building script." : Format("Empty code for variant %d.", script->variant->index + 1));
					script->variant->vars = vars.size();
					script->variant = nullptr;
					code = nullptr;
					if(inline_variant)
						break;
				}
				else if(script->variants.empty())
					t.Throw("Empty building script.");
				break;
			}

			if(t.IsKeywordGroup(G_SCRIPT))
			{
				ScriptKeyword k = (ScriptKeyword)t.GetKeywordId(G_SCRIPT);
				t.Next();

				if(k == SK_VARIANT)
				{
					if(script->variant)
						t.Throw("Already in variant block.");
					StartVariant(script);

					t.AssertSymbol('{');
					t.Next();
					continue;
				}
				else
				{
					if(!script->variant)
					{
						if(script->variants.empty())
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
						t.AssertKeyword(SK2_OFF, G_SCRIPT2);
						code->push_back(BuildingScript::BS_REQUIRED_OFF);
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
								if(t.IsKeyword(SK_GROUP, G_SCRIPT))
								{
									t.Next();
									const string& id = t.MustGetItem();
									BuildingGroup* group = content::FindBuildingGroup(id);
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
									Building* b = content::FindBuilding(id);
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
							Var& v = GetVar(t);

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
							GetExpr(t);

							if(op != '=')
								code->push_back(CharToOp(op));
							code->push_back(BuildingScript::BS_SET_VAR);
							code->push_back(v.index);
						}
						break;
					case SK_INC:
					case SK_DEC:
						{
							Var& v = GetVar(t);
							code->push_back(k == SK_INC ? BuildingScript::BS_INC : BuildingScript::BS_DEC);
							code->push_back(v.index);
						}
						break;
					case SK_IF:
						if(t.IsKeyword(SK_RAND, G_SCRIPT))
						{
							t.Next();
							GetExpr(t);
							code->push_back(BuildingScript::BS_IF_RAND);
						}
						else
						{
							GetExpr(t);
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
							GetExpr(t);
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
							BuildingGroup* group = content::FindBuildingGroup(id);
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
			}
			else if(t.IsItem())
			{
				if(!script->variant)
				{
					if(script->variants.empty())
					{
						StartVariant(script);
						inline_variant = true;
					}
					else
						t.Throw("Code outside variant block.");
				}

				const string& id = t.MustGetItem();
				Building* b = content::FindBuilding(id);
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

		content::building_scripts.push_back(script);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load %s: %s", GetItemString("building script", script->id), e.ToString()));
		delete script;
		return false;
	}
}

//=================================================================================================
void BuildingLoader::StartVariant(BuildingScript* script)
{
	script->variant = new BuildingScript::Variant;
	script->variant->index = script->variants.size();
	script->variant->vars = 0;
	script->variants.push_back(script->variant);
	code = &script->variant->code;

	vars.clear();
	AddVar("count");
	AddVar("counter");
	AddVar("citizens");
	AddVar("citizens_world");
}

//=================================================================================================
void BuildingLoader::AddVar(AnyString id, bool is_const)
{
	Var v;
	v.name = id.s;
	v.is_const = is_const;
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
BuildingLoader::Var& BuildingLoader::GetVar(Tokenizer& t, bool can_be_const)
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
void BuildingLoader::GetExpr(Tokenizer& t)
{
	GetItem(t);

	while(true)
	{
		char c;
		if(!t.IsSymbol("+-*/", &c))
			return;
		t.Next();

		GetItem(t);

		code->push_back(CharToOp(c));
	}
}

//=================================================================================================
void BuildingLoader::GetItem(Tokenizer& t)
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
		GetExpr(t);
		t.AssertSymbol(',');
		t.Next();
		GetExpr(t);
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
		Var& v = GetVar(t, true);
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
int BuildingLoader::LoadText()
{
	Tokenizer& t = text_tokenizer;

	int errors = t.ParseTop<int>(-1, [this, &t](int) {
		// name
		tmp_str = t.MustGetItem();
		t.Next();

		// =
		t.AssertSymbol('=');
		t.Next();

		// text
		const string& text = t.MustGetString();
		bool any = false;
		for(Building* b : content::buildings)
		{
			if(!b->name_set && IS_SET(b->flags, Building::HAVE_NAME))
			{
				if(b->name.empty())
				{
					if(b->id == tmp_str)
					{
						b->name = text;
						b->name_set = true;
						any = true;
					}
				}
				else
				{
					if(b->name == tmp_str)
					{
						b->name = text;
						b->name_set = true;
						any = true;
					}
				}
			}
		}

		if(!any)
			WARN(Format("Unused building text %s = \"%s\".", tmp_str.c_str(), text.c_str()));

		return true;
	});

	for(Building* b : content::buildings)
	{
		if(!b->name_set && IS_SET(b->flags, Building::HAVE_NAME))
		{
			if(b->name.empty())
				ERROR(Format("Missing text for building id '%s'.", b->id.c_str()));
			else
				ERROR(Format("Missing text for building name '%s'.", b->name.c_str()));
			++errors;
		}
	}

	return errors;
}
