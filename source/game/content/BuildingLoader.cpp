#include "Pch.h"
#include "Base.h"
#include "BuildingLoader.h"
#include "Content.h"
#include "Building.h"

enum KeywordGroup
{
	G_TOP,
	G_BUILDING,
	G_SIDE,
	G_FLAGS,
	G_SCRIPT,
	G_SCRIPT2
};

enum TopKeyword
{
	TOP_BUILDING_GROUPS,
	TOP_BUILDING,
	TOP_BUILDING_SCRIPT
};

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
	SK2_ON,
	SK2_OFF,
	SK2_START,
	SK2_END
};

BuildingLoader::BuildingLoader() : ContentLoader(ContentType::Building, "building", ContentLoader::HaveDatafile | ContentLoader::HaveText)
{

}

BuildingLoader::~BuildingLoader()
{

}

void BuildingLoader::Init()
{
	t.AddKeywords(G_TOP, {
		{ "buildings_group", TOP_BUILDING_GROUPS },
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
		{ "on", SK2_ON },
		{ "off", SK2_OFF },
		{ "start", SK2_START },
		{ "end", SK2_END }
	});

	t2.AddKeyword("building", 0);
}

void BuildingLoader::Cleanup()
{
	DeleteElements(content::buildings);
	content::building_groups.clear();
}

int BuildingLoader::Load()
{
	return t.ParseTop<TopKeyword>(G_TOP, [this](TopKeyword top) {
		switch(top)
		{
		case TOP_BUILDING:
			return LoadBuilding();
		case TOP_BUILDING_GROUPS:
			return LoadBuildingGroups();
		case TOP_BUILDING_SCRIPT:
			return LoadBuildingScript();
		default:
			assert(0);
			return false;
		}
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

bool BuildingLoader::LoadBuildingScript()
{
	BuildingScript* script = new BuildingScript;
	vector<bool> if_state; // false - if block, true - else block

	try
	{
		// id
		const string& id = t.MustGetString();
		if(content::FindBuildingScript(id))
			t.Throw("Id already used.");
		script->id = id;
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		script->variant = nullptr;
		vector<int>* code = nullptr;
		bool inline_variant = false, in_shuffle = false;

		while(true)
		{
			if(t.IsSymbol('}'))
			{
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
					StartVariant(script, code);

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
							StartVariant(script, code);
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
							code->push_back(BS_SHUFFLE_START);
							in_shuffle = true;
							t.Next();
						}
						else if(t.IsKeyword(SK2_END, G_SCRIPT2))
						{
							if(!in_shuffle)
								t.Throw("Shuffle not started.");
							code->push_back(BS_SHUFFLE_END);
							in_shuffle = false;
							t.Next();
						}
						else
							t.Unexpected();
						break;
					case SK_REQUIRED:
						if(t.IsKeyword(SK2_ON, G_SCRIPT2))
						{
							code->push_back(BS_REQUIRED_ON);
							t.Next();
						}
						else if(t.IsKeyword(SK2_OFF, G_SCRIPT2))
						{
							code->push_back(BS_REQUIRED_OFF);
							t.Next();
						}
						else
							t.Unexpected();
						break;
					case SK_RANDOM:
						{
							t.AssertSymbol('{');
							t.Next();
							code->push_back(BS_RANDOM);
							uint pos = code->size(), items = 0;
							code->push_back(0);
							while(!t.IsSymbol('}'))
							{
								if(t.IsKeyword(SK_GROUP, G_SCRIPT))
								{
									t.Next();
									const string& id = t.MustGetItem();
									int group = content::FindBuildingGroup(id);
									if(group == -1)
										t.Throw("Missing building group '%s'.", id.c_str());
									t.Next();
									code->push_back(BS_GROUP);
									code->push_back(group);
									++items;
								}
								else
								{
									const string& id = t.MustGetString();
									Building* b = content::FindBuilding(id);
									if(!b)
										t.Throw("Missing building '%s'.", id.c_str());
									t.Next();
									code->push_back(BS_BUILDING);
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
							Var& v = GetVar();
							t.AssertSymbol('=');
							t.Next();
							int left, left_value;
							GetVarOrInt(left, left_value);
							char c;
							if(t.IsSymbol("+-*/", &c))
							{
								t.Next();
								int right, right_value;
								GetVarOrInt(right, right_value);
								code->push_back(BS_SET_VAR_OP);
								code->push_back(v.index);
								code->push_back(c);
								code->push_back(left);
								code->push_back(left_value);
								code->push_back(right);
								code->push_back(right_value);
							}
							else
							{
								code->push_back(BS_SET_VAR);
								code->push_back(v.index);
								code->push_back(left);
								code->push_back(left_value);
							}
						}
						break;
					case SK_INC:
					case SK_DEC:
						{
							Var& v = GetVar();
							code->push_back(k == SK_INC ? BS_INC : BS_DEC);
							code->push_back(v.index);
						}
						break;
					case SK_IF:
						if(t.IsKeyword(SK_RANDOM, G_SCRIPT))
						{
							t.Next();
							int type, value;
							GetVarOrInt(type, value);
							code->push_back(BS_IF_RANDOM);
							code->push_back(type);
							code->push_back(value);
						}
						else
						{
							int left, left_value, right, right_value;
							GetVarOrInt(left, left_value);
							char c;
							if(!t.IsSymbol("><!=", &c))
								t.Unexpected();
							BsCode symbol;
							switch(c)
							{
							case '>':
								if(t.PeekSymbol('='))
									symbol = BS_GREATER_EQUAL;
								else
									symbol = BS_GREATER;
								break;
							case '<':
								if(t.PeekSymbol('='))
									symbol = BS_LESS_EQUAL;
								else
									symbol = BS_LESS;
								break;
							case '!':
								if(!t.PeekSymbol('='))
									t.Unexpected();
								symbol = BS_NOT_EQUAL;
								break;
							case '=':
								if(!t.PeekSymbol('='))
									t.Unexpected();
								symbol = BS_EQUAL;
								break;
							}
							t.Next();
							GetVarOrInt(right, right_value);
							code->push_back(BS_IF);
							code->push_back(symbol);
							code->push_back(left);
							code->push_back(left_value);
							code->push_back(right);
							code->push_back(right_value);
						}
						if_state.push_back(false);
						break;
					case SK_ELSE:
						if(if_state.empty() || if_state.back() == true)
							t.Unexpected();
						code->push_back(BS_ELSE);
						if_state.back() = true;
						break;
					case SK_ENDIF:
						if(if_state.empty())
							t.Unexpected();
						code->push_back(BS_ENDIF);
						if_state.pop_back();
						break;
					case SK_GROUP:
						{
							const string& id = t.MustGetItem();
							int group = content::FindBuildingGroup(id);
							if(group == -1)
								t.Throw("Missing building group '%s'.", id.c_str());
							t.Next();
							code->push_back(BS_ADD);
							code->push_back(BS_GROUP);
							code->push_back(group);
						}
						break;
					}
				}
			}
			else if(t.IsItem())
			{
				const string& id = t.MustGetString();
				Building* b = content::FindBuilding(id);
				if(!b)
					t.Throw("Missing building '%s'.", id.c_str());
				t.Next();
				code->push_back(BS_ADD);
				code->push_back(BS_BUILDING);
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

void BuildingLoader::StartVariant(BuildingScript* script, vector<int>*& code)
{
	script->variant = new BuildingScript::Variant;
	script->variant->index = script->variants.size();
	script->variant->vars = 0;
	script->variants.push_back(script->variant);
	code = &script->variant->code;

	vars.clear();
	AddVar("count");
	AddVar("counter");
	AddVar("citizens", true);
}

void BuildingLoader::AddVar(AnyString id, bool is_const)
{
	Var v;
	v.name = id.s;
	v.is_const = is_const;
	v.index = vars.size();
	vars.push_back(v);
}

BuildingLoader::Var* BuildingLoader::FindVar(const string& id)
{
	for(Var& v : vars)
	{
		if(v.name == id)
			return &v;
	}
	return nullptr;
}

BuildingLoader::Var& BuildingLoader::GetVar(bool can_be_const)
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

void BuildingLoader::GetVarOrInt(int& type, int& value)
{
	if(t.IsInt())
	{
		type = BS_INT;
		value = t.GetInt();
		t.Next();
	}
	else if(t.IsItem())
	{
		Var& v = GetVar(true);
		type = BS_VAR;
		value = v.index;
	}
	else
		t.StartUnexpected().Add(Tokenizer::T_ITEM).Add(Tokenizer::T_INT).Throw();
}

int BuildingLoader::LoadText()
{
	int errors = t2.ParseTop<int>(0, [this](int) {
		// name
		tmp_str = t2.MustGetItem();
		t.Next();

		// =
		t2.AssertSymbol('=');
		t2.Next();

		// text
		const string& text = t2.MustGetString();
		bool any = false;
		for(Building* b : content::buildings)
		{
			if(!b->name_set)
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
		t.Next();

		return true;
	});

	for(Building* b : content::buildings)
	{
		if(!b->name_set)
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

BuildingScript* content::FindBuildingScript(AnyString id)
{
	for(BuildingScript* bs : building_scripts)
	{
		if(bs->id == id.s)
			return bs;
	}
	return nullptr;
}
