// configuration reader/writer
#include "EnginePch.h"
#include "Core.h"
#include "Config.h"
#include "File.h"

const int CONFIG_VERSION = 1;

//=================================================================================================
void Config::Add(cstring name, cstring value)
{
	assert(name && value);

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		if(it->name == name)
		{
			it->value = value;
			return;
		}
	}

	Entry& e = Add1(entries);
	e.name = name;
	e.value = value;
}

//=================================================================================================
Config::Entry* Config::GetEntry(cstring name)
{
	assert(name);
	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}
	return nullptr;
}

//=================================================================================================
bool Config::GetBool(cstring name, bool def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;

	if(OR3_EQ(e->value, "0", "false", "FALSE"))
		return false;
	else if(OR3_EQ(e->value, "1", "true", "TRUE"))
		return true;
	else
		return def;
}

//=================================================================================================
Bool3 Config::GetBool3(cstring name, Bool3 def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;

	if(OR3_EQ(e->value, "0", "false", "FALSE"))
		return False;
	else if(OR3_EQ(e->value, "1", "true", "TRUE"))
		return True;
	else
		return def;
}

//=================================================================================================
const string& Config::GetString(cstring name)
{
	Entry* e = GetEntry(name);
	if(!e)
	{
		tmpstr.clear();
		return tmpstr;
	}
	else
		return e->value;
}

//=================================================================================================
const string& Config::GetString(cstring name, const string& def)
{
	Entry* e = GetEntry(name);
	if(!e)
	{
		tmpstr = def;
		return tmpstr;
	}
	else
		return e->value;
}

//=================================================================================================
int Config::GetInt(cstring name, int def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;
	else
	{
		int value;
		if(TextHelper::ToInt(e->value.c_str(), value))
			return value;
		else
			return def;
	}
}

//=================================================================================================
uint Config::GetUint(cstring name, uint def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;
	else
	{
		uint value;
		if(TextHelper::ToUint(e->value.c_str(), value))
			return value;
		else
			return def;
	}
}

//=================================================================================================
__int64 Config::GetInt64(cstring name, int def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;
	else
	{
		__int64 value;
		float f;
		if(TextHelper::ToNumber(e->value.c_str(), value, f) != 0)
			return value;
		else
			return def;
	}
}

//=================================================================================================
float Config::GetFloat(cstring name, float def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;
	else
		return (float)atof(e->value.c_str());
}

//=================================================================================================
Int2 Config::GetInt2(cstring name, Int2 def)
{
	Entry* e = GetEntry(name);
	if(!e)
		return def;
	t.SetFlags(Tokenizer::F_JOIN_MINUS);
	t.FromString(e->value);
	try
	{
		Int2 result;
		t.Next();
		t.Parse(result);
		t.AssertEof();
		return result;
	}
	catch(const Tokenizer::Exception&)
	{
		return def;
	}
}

//=================================================================================================
Config::GetResult Config::TryGetInt(cstring name, int& value)
{
	Entry* e = GetEntry(name);
	if(!e)
		return GET_MISSING;
	else if(TextHelper::ToInt(e->value.c_str(), value))
		return GET_OK;
	else
		return GET_INVALID;
}

//=================================================================================================
Config::Result Config::Load(cstring filename)
{
	assert(filename);

	t.SetFlags(Tokenizer::F_JOIN_DOT | Tokenizer::F_JOIN_MINUS);
	if(!t.FromFile(filename))
		return NO_FILE;

	LocalString item, value;

	try
	{
		t.Next();

		// version info
		if(t.IsSymbol('#'))
		{
			t.Next();
			if(t.IsItem("version"))
			{
				t.Next();
				version = t.MustGetInt();
				if(version < 0 || version > CONFIG_VERSION)
					t.Throw("Invalid version %d.", version);
				if(version == 1)
					t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_UNESCAPE);
				t.Next();
			}
		}

		// configuration
		while(!t.IsEof())
		{
			// name
			item = t.MustGetItem();
			t.Next();

			// =
			t.AssertSymbol('=');
			t.Next(true);

			// value
			if(t.IsEol())
				value->clear();
			else
			{
				if(t.IsSymbol('{'))
					value = t.GetBlock();
				else
					value = t.GetTokenString();
			}

			// add if not exists
			bool exists = false;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(item == it->name)
				{
					exists = true;
					break;
				}
			}
			if(!exists)
			{
				Entry& e = Add1(entries);
				e.name = item;
				e.value = value;
			}

			t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		error = e.ToString();
		return PARSE_ERROR;
	}

	return OK;
}

//=================================================================================================
Config::Result Config::Save(cstring filename)
{
	assert(filename);

	TextWriter f(filename);
	if(!f)
		return CANT_SAVE;

	f << Format("#version %d\n", CONFIG_VERSION);

	std::sort(entries.begin(), entries.end(), [](const Config::Entry& e1, const Config::Entry& e2) -> bool { return e1.name < e2.name; });

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		cstring s;
		if(it->value.find_first_of(" \t,./;'[]-=<>?:\"{}!@#$%^&*()_+") != string::npos)
			s = Format("%s = \"%s\"\n", it->name.c_str(), Escape(it->value));
		else
			s = Format("%s = %s\n", it->name.c_str(), it->value.c_str());
		f << s;
	}

	return OK;
}

//=================================================================================================
void Config::Remove(cstring name)
{
	assert(name);
	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		if(it->name == name)
		{
			entries.erase(it);
			return;
		}
	}
	assert(0);
}

//=================================================================================================
void Config::ParseConfigVar(cstring arg)
{
	assert(arg);

	int index = StrCharIndex(arg, '=');
	if(index == -1 || index == 0)
	{
		Warn("Broken command line variable '%s'.", arg);
		return;
	}

	ConfigVar* var = nullptr;
	for(ConfigVar& v : config_vars)
	{
		if(strncmp(arg, v.name, index) == 0)
		{
			var = &v;
			break;
		}
	}
	if(!var)
	{
		Warn("Missing config variable '%.*s'.", index, arg);
		return;
	}

	cstring value = arg + index + 1;
	if(!*value)
	{
		Warn("Missing command line variable value '%s'.", arg);
		return;
	}

	switch(var->type)
	{
	case AnyVarType::Bool:
		{
			bool b;
			if(!TextHelper::ToBool(value, b))
			{
				Warn("Value for config variable '%s' must be bool, found '%s'.", var->name, value);
				return;
			}
			var->new_value._bool = b;
			var->have_new_value = true;
		}
		break;
	}
}

//=================================================================================================
void Config::LoadConfigVars()
{
	for(ConfigVar& v : config_vars)
	{
		Config::Entry* entry = GetEntry(v.name);
		if(!entry)
			continue;

		switch(v.type)
		{
		case AnyVarType::Bool:
			if(!TextHelper::ToBool(entry->value.c_str(), v.ptr->_bool))
			{
				Warn("Value for config variable '%s' must be bool, found '%s'.", v.name, entry->value.c_str());
				return;
			}
			break;
		}
	}

	for(ConfigVar& v : config_vars)
	{
		if(!v.have_new_value)
			continue;
		switch(v.type)
		{
		case AnyVarType::Bool:
			v.ptr->_bool = v.new_value._bool;
			break;
		}
	}
}
