#include "Pch.h"
#include "GameCore.h"
#include "ModuleLoader.h"
#include "Module.h"
#include "Language.h"

extern string g_modules_dir;

enum Keyword
{
	K_NAME,
	K_DESC,
	K_VERSION
};

void ModuleLoader::InitTokenizer()
{
	t.AddKeywords(0, {
		{ "name", K_NAME },
		{ "desc", K_DESC },
		{ "version", K_VERSION }
		});
}

void ModuleLoader::LoadModules()
{
	InitTokenizer();

	io::FindFiles(Format("%s/*", g_modules_dir.c_str()), [&](const io::FileInfo& info)
	{
		if(info.is_dir)
			LoadModule(info.filename);
		return true;
	});
}

void ModuleLoader::LoadModule(cstring id)
{
	cstring path = Format("%s/%s/module.txt", g_modules_dir.c_str(), id);
	if(!t.FromFile(path))
		return;

	Ptr<Module> module;
	module->id = id;
	module->name = name;
	module->version = 0;

	try
	{
		while(t.Next())
		{
			Keyword k = t.MustGetKeywordId<Keyword>(0);
			t.Next();

			switch(k)
			{
			case K_NAME:
				ParseLocalizedString(module->name);
				break;
			case K_DESC:
				ParseLocalizedString(module->desc);
				break;
			case K_VERSION:
				module->version = t.MustGetFloat();
				t.Next();
				break;
			}
		}

		Module::modules.push_back(module.Pin());
	}
	catch(const Tokenizer::Exception & ex)
	{

	}
}

void ModuleLoader::ParseLocalizedString(string& str)
{
	if(t.IsSymbol('{'))
	{
		t.Next();
		bool set = false;
		while(!t.IsSymbol('}'))
		{
			const string& id = t.MustGetItem();
			if(id == Language::prefix || (!set && id == "end"))
			{
				t.Next();
				str = t.MustGetString();
				t.Next();
				set = true;
			}
			else
			{
				t.Next();
				t.AssertString();
				t.Next();
			}
		}
		if(!set)
			t.Throw("Empty string group.");
		t.Next();
	}
	else if(t.IsString())
	{
		str = t.GetString();
		t.Next();
	}
	else
		t.Unexpected("String or string group required.");
}
