#include "Pch.h"
#include "ContentLoader.h"

#include "Language.h"

//=================================================================================================
void ContentLoader::Load(cstring filename, int top_group, bool* require_id)
{
	InitTokenizer();
	if(DoLoad(filename, top_group, require_id))
	{
		local_id.clear();
		LoadTexts();
		Finalize();
	}
}

//=================================================================================================
bool ContentLoader::DoLoad(cstring filename, int top_group, bool* require_id)
{
	LocalString path = Format("%s/%s", content.system_dir.c_str(), filename);
	this->top_group = top_group;

	if(!t.FromFile((const string&)path))
	{
		Error("Failed to open file '%s'.", path.c_str());
		++content.errors;
		return false;
	}

	try
	{
		content.errors += t.ParseTop<int>(top_group, [&](int top)
		{
			current_entity = top;
			if(require_id && !require_id[top])
			{
				local_id.clear();
				try
				{
					LoadEntity(top, local_id);
					return true;
				}
				catch(Tokenizer::Exception& e)
				{
					Error("Failed to parse %s: %s", t.FindKeyword(top, top_group)->name, e.ToString());
					return false;
				}
			}
			else
			{
				if(!t.IsText())
				{
					Error(t.Expecting("id"));
					return false;
				}
				local_id = t.GetText();

				try
				{
					LoadEntity(top, local_id);
					return true;
				}
				catch(Tokenizer::Exception& e)
				{
					Error("Failed to parse %s '%s': %s", t.FindKeyword(top, top_group)->name, local_id.c_str(), e.ToString());
					return false;
				}
			}
		});
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Failed to parse file '%s': %s", path.c_str(), e.ToString());
		++content.errors;
	}

	return true;
}

//=================================================================================================
void ContentLoader::LoadWarning(cstring msg)
{
	assert(msg);
	Warn("Warning loading %s: %s", GetEntityName(), msg);
	++content.warnings;
}

//=================================================================================================
void ContentLoader::LoadError(cstring msg)
{
	assert(msg);
	Error("Error loading %s: %s", GetEntityName(), msg);
	++content.errors;
}

//=================================================================================================
cstring ContentLoader::FormatLanguagePath(cstring filename)
{
	return Language::GetPath(filename);
}

//=================================================================================================
bool ContentLoader::IsPrefix(cstring prefix)
{
	if(local_id == prefix)
	{
		t.Next();
		local_id = t.MustGetText();
		return true;
	}
	return false;
}

//=================================================================================================
cstring ContentLoader::GetEntityName()
{
	cstring typeName = t.FindKeyword(current_entity, top_group)->name;
	if(local_id.empty())
		return typeName;
	else
		return Format("%s '%s'", typeName, local_id.c_str());
}
