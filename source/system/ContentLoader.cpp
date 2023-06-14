#include "Pch.h"
#include "ContentLoader.h"

#include "Language.h"

//=================================================================================================
void ContentLoader::Load(cstring filename, int topGroup, bool* requireId)
{
	InitTokenizer();
	if(DoLoad(filename, topGroup, requireId))
	{
		localId.clear();
		LoadTexts();
		Finalize();
	}
}

//=================================================================================================
bool ContentLoader::DoLoad(cstring filename, int topGroup, bool* requireId)
{
	LocalString path = Format("%s/%s", content.systemDir.c_str(), filename);
	this->topGroup = topGroup;

	if(!t.FromFile((const string&)path))
	{
		Error("Failed to open file '%s'.", path.c_str());
		++content.errors;
		return false;
	}

	try
	{
		content.errors += t.ParseTop(topGroup, [&](int top)
		{
			currentEntity = top;
			if(requireId && !requireId[top])
			{
				localId.clear();
				try
				{
					LoadEntity(top, localId);
					return true;
				}
				catch(Tokenizer::Exception& e)
				{
					Error("Failed to parse %s: %s", t.FindKeyword(top, topGroup)->name, e.ToString());
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
				localId = t.GetText();

				try
				{
					LoadEntity(top, localId);
					return true;
				}
				catch(Tokenizer::Exception& e)
				{
					Error("Failed to parse %s '%s': %s", t.FindKeyword(top, topGroup)->name, localId.c_str(), e.ToString());
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
	if(localId == prefix)
	{
		t.Next();
		localId = t.MustGetText();
		return true;
	}
	return false;
}

//=================================================================================================
cstring ContentLoader::GetEntityName()
{
	cstring typeName = t.FindKeyword(currentEntity, topGroup)->name;
	if(localId.empty())
		return typeName;
	else
		return Format("%s '%s'", typeName, localId.c_str());
}
