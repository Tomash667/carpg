#include "Pch.h"
#include "GameCore.h"
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
	FIXME;
	LocalString path = Format("XXX/%s", filename);
	this->top_group = top_group;

	if(!t.FromFile(path))
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
	cstring type_name = t.FindKeyword(current_entity, top_group)->name;
	if(local_id.empty())
		Warn("Warning loading %s: %s", type_name, msg);
	else
		Warn("Warning loading %s '%s': %s", type_name, local_id.c_str(), msg);
	++content.warnings;
}

//=================================================================================================
void ContentLoader::LoadError(cstring msg)
{
	assert(msg);
	cstring type_name = t.FindKeyword(current_entity, top_group)->name;
	if(local_id.empty())
		Error("Error loading %s: %s", type_name, msg);
	else
		Error("Error loading %s '%s': %s", type_name, local_id.c_str(), msg);
	++content.errors;
}

//=================================================================================================
cstring ContentLoader::FormatLanguagePath(cstring filename)
{
	FIXME;
	return Format("XXX/lang/%s/%s", Language::prefix.c_str(), filename);
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
