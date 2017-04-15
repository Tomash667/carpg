#include "Pch.h"
#include "Base.h"
#include "Building.h"
#include "BuildingGroup.h"
#include "BuildingScript.h"
#include "Content.h"
#include "Crc.h"
#include "TypeVectorContainer.h"

//-----------------------------------------------------------------------------
vector<BuildingScript*> content::building_scripts;

//=================================================================================================
// Checks if building script have building from selected building group
//=================================================================================================
bool BuildingScript::HaveBuilding(const string& group_id) const
{
	BuildingGroup* group = content::FindBuildingGroup(group_id);
	if(!group)
		return false;
	for(Variant* v : variants)
	{
		if(!HaveBuilding(group, v))
			return false;
	}
	return true;
}

//=================================================================================================
// Checks if building script have building from selected building group
//=================================================================================================
bool BuildingScript::HaveBuilding(BuildingGroup* group, Variant* variant) const
{
	assert(group && variant);

	enum IfStatus
	{
		IFS_IF, // if { ? }
		IFS_IF_OK, // if { OK }
		IFS_ELSE, // if { X } else { ? }
		IFS_ELSE_OK, // if { OK } else { ? }
		IFS_ELSE_OK2 // if { OK } else { OK }
	};

	const int* code = variant->code.data();
	const int* end = code + variant->code.size();
	vector<IfStatus> if_status;

	while(code != end)
	{
		Code c = (Code)*code++;

		switch(c)
		{
		case BS_ADD_BUILDING:
			if(IsEntryGroup(code, group))
			{
				if(if_status.empty())
					return true;
				else
				{
					IfStatus& s = if_status.back();
					if(s == IFS_IF)
						s = IFS_IF_OK;
					else if(s == IFS_ELSE_OK)
						s = IFS_ELSE_OK2;
				}
			}
			break;
		case BS_RANDOM:
			{
				int count = *code++;
				int ok = 0;
				for(int i = 0; i < count; ++i)
				{
					if(IsEntryGroup(code, group))
						++ok;
				}
				if(count == ok)
				{
					if(if_status.empty())
						return true;
					else
					{
						IfStatus& s = if_status.back();
						if(s == IFS_IF)
							s = IFS_IF_OK;
						else if(s == IFS_ELSE_OK)
							s = IFS_ELSE_OK2;
					}
				}
			}
			break;
		case BS_SHUFFLE_START:
		case BS_SHUFFLE_END:
		case BS_CALL:
		case BS_ADD:
		case BS_SUB:
		case BS_MUL:
		case BS_DIV:
		case BS_NEG:
			break;
		case BS_REQUIRED_OFF:
			// buildings that aren't required don't count for HaveBuilding
			return false;
		case BS_PUSH_INT:
		case BS_PUSH_VAR:
		case BS_SET_VAR:
		case BS_INC:
		case BS_DEC:
			++code;
			break;
		case BS_IF:
			if_status.push_back(IFS_IF);
			++code;
			break;
		case BS_IF_RAND:
			if_status.push_back(IFS_IF);
			break;
		case BS_ELSE:
			{
				IfStatus& s = if_status.back();
				if(s == IFS_IF)
					s = IFS_ELSE;
				else
					s = IFS_ELSE_OK;
			}
			break;
		case BS_ENDIF:
			{
				IfStatus s = if_status.back();
				if_status.pop_back();
				if(if_status.empty())
				{
					if(s == IFS_IF_OK || IFS_ELSE_OK2)
						return true;
				}
				else
				{
					IfStatus& s2 = if_status.back();
					if(s == IFS_IF_OK || IFS_ELSE_OK2)
					{
						if(s2 == IFS_IF)
							s2 = IFS_IF_OK;
						else if(s2 == IFS_ELSE_OK)
							s2 = IFS_ELSE_OK2;
					}
				}
			}
			break;
		default:
			assert(0);
			return false;
		}
	}

	return false;
}

//=================================================================================================
// Check if building or group belongs to building group in code
//=================================================================================================
bool BuildingScript::IsEntryGroup(const int*& code, BuildingGroup* group) const
{
	Code type = (Code)*code++;
	if(type == BS_BUILDING)
	{
		Building* b = (Building*)*code++;
		return b->group == group;
	}
	else // type == BS_GROUP
	{
		BuildingGroup* g = (BuildingGroup*)*code++;
		return g == group;
	}
}

//=================================================================================================
// Find building script by id
//=================================================================================================
BuildingScript* content::FindBuildingScript(const AnyString& id)
{
	for(BuildingScript* bs : building_scripts)
	{
		if(bs->id == id.s)
			return bs;
	}
	return nullptr;
}

//=================================================================================================
// Building script type handler
//=================================================================================================
struct BuildingScriptHandler : public TypeImpl<BuildingScript>
{
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

	enum KeywordGroup
	{
		G_KEYWORD,
		G_KEYWORD_TYPE
	};

	struct Var
	{
		string name;
		int index;
		bool is_const;
	};

	BuildingScript::Variant* variant;
	vector<int>* code;
	vector<Var> vars;
	Tokenizer t;

	//=================================================================================================
	// Register keywords
	BuildingScriptHandler() : TypeImpl(TypeId::BuildingScript, "building_script", "Building script", "buildings")
	{
		DependsOn(TypeId::Building);
		DependsOn(TypeId::BuildingGroup);
		HaveCustomCrc();

		AddId(offsetof(BuildingScript, id), true);
		AddString("code", offsetof(BuildingScript, code_str));

		t.AddKeywords(G_KEYWORD, {
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
		}, "building script keyword");

		t.AddKeywords(G_KEYWORD_TYPE, {
			{ "off", SK2_OFF },
			{ "start", SK2_START },
			{ "end", SK2_END }
		}, "building script keyword type");

		container = new TypeVectorContainer(this, content::building_scripts);
	}

	//=================================================================================================
	// Convert script into code
	cstring Prepare(TypeItem* item) override
	{
		BuildingScript& script = *(BuildingScript*)item;
		
		DeleteElements(script.variants);
		script.required_offset = (uint)-1;
		variant = nullptr;
		code = nullptr;

		

		try
		{
			ParseCode(script);
		}
		catch(const Tokenizer::Exception& ex)
		{
			return ex.ToString();
		}

		return nullptr;
	}

	void ParseCode(BuildingScript& script)
	{
		vector<bool> if_state; // false - if block, true - else block
		bool inline_variant = false,
			in_shuffle = false;

		t.FromString(script.code_str);

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
					t.Unexpected();
			}

			if(t.IsKeywordGroup(G_KEYWORD))
			{
				ScriptKeyword k = (ScriptKeyword)t.GetKeywordId(G_KEYWORD);
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
					if(t.IsKeyword(SK2_START, G_KEYWORD_TYPE))
					{
						if(in_shuffle)
							t.Throw("Shuffle already started.");
						code->push_back(BuildingScript::BS_SHUFFLE_START);
						in_shuffle = true;
						t.Next();
					}
					else if(t.IsKeyword(SK2_END, G_KEYWORD_TYPE))
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
					t.AssertKeyword(SK2_OFF, G_KEYWORD_TYPE);
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
							if(t.IsKeyword(SK_GROUP, G_KEYWORD_TYPE))
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
					if(t.IsKeyword(SK_RAND, G_KEYWORD))
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
	// Start variant block
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
	// Add new variable
	void AddVar(const AnyString& id, bool is_const = false)
	{
		Var v;
		v.name = id.s;
		v.is_const = is_const;
		v.index = vars.size();
		vars.push_back(v);
	}

	//=================================================================================================
	// Find existing variable
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
	// Try to get existing variable
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
	// Get code expression
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
	// Get code item
	void GetItem()
	{
		bool neg = false;
		if(t.IsSymbol('-'))
		{
			neg = true;
			t.Next();
		}

		if(t.IsKeyword(SK_RANDOM, G_KEYWORD))
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
	// Map character to operand
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
	// Update crc using item
	void UpdateCrc(CRC32& crc, TypeItem* item) override
	{
		BuildingScript& script = *(BuildingScript*)item;
		crc.Update(script.id);
		crc.Update(script.variants.size());
		for(BuildingScript::Variant* variant : script.variants)
			crc.UpdateVector(variant->code);
	}
};

Type* CreateBuildingScriptHandler()
{
	return new BuildingScriptHandler;
}
