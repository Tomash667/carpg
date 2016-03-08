// configuration reader/writer
#include "Pch.h"
#include "Base.h"
#include "Config.h"

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

	Tokenizer t(Tokenizer::F_JOIN_DOT);
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
					t.SetFlags(0);
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
				value = t.GetTokenString();

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

	DWORD tmp;
	HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
		return CANT_SAVE;

	cstring s = Format("#version %d\n", CONFIG_VERSION);
	WriteFile(file, s, strlen(s), &tmp, nullptr);

	std::sort(entries.begin(), entries.end(), [](const Config::Entry& e1, const Config::Entry& e2) -> bool { return e1.name < e2.name; });

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		cstring fmt;
		if(it->value.find_first_of(" \t,./;'[]-=<>?:\"{}!@#$%^&*()_+") != string::npos)
			fmt = "%s = \"%s\"\n";
		else
			fmt = "%s = %s\n";
		s = Format(fmt, it->name.c_str(), it->value.c_str());
		WriteFile(file, s, strlen(s), &tmp, nullptr);
	}

	CloseHandle(file);
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
