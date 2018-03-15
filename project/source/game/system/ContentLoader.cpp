#include "Pch.h"
#include "GameCore.h"
#include "ContentLoader.h"

//=================================================================================================
void ContentLoader::Load(cstring filename, int top_group, bool* require_id)
{
	InitTokenizer();
	if(DoLoad(filename, top_group, require_id))
		Finalize();
}

//=================================================================================================
bool ContentLoader::DoLoad(cstring filename, int top_group, bool* require_id)
{
	LocalString path = Format("%s/%s", content::system_dir.c_str(), filename);
	this->top_group = top_group;

	if(!t.FromFile(path))
	{
		Error("Failed to open file '%s'.", path.c_str());
		++content::errors;
		return false;
	}

	try
	{
		content::errors += t.ParseTop<int>(top_group, [&](int top)
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
		++content::errors;
	}

	return true;
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
	++content::errors;
}
