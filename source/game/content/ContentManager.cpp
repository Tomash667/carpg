#include "Pch.h"
#include "Base.h"
#include "ContentManager.h"
#include "ContentLoader.h"
#include "ResourceManager.h"
#include "Language.h"

//-----------------------------------------------------------------------------
extern string g_system_dir;
extern string g_lang_prefix;

//=================================================================================================
ContentManager::~ContentManager()
{
	for(ContentLoader* l : loaders)
	{
		l->Cleanup();
		delete l;
	}
}

//=================================================================================================
void ContentManager::Init()
{
	LOG("Content manager: Initialziation.");
	SetupTexts();
	SortLoaders();

	for(ContentLoader* l : loaders)
		l->Init();
}

//=================================================================================================
void ContentManager::SetupTexts()
{
	txLoadingDatafiles = Str("loadingDatafiles");
	txLoadingTextfiles = Str("loadingDatafiles");
	txLoadingResources = Str("loadingDatafiles");

	for(ContentLoader* l : loaders)
		l->name = Str(Format("%sGroup", l->id));
}

//=================================================================================================
// Kahn's algorithm from https://en.wikipedia.org/wiki/Topological_sorting
void ContentManager::SortLoaders()
{
	if(loaders.empty())
		return;

	// fill graph edges
	for(ContentLoader* l : loaders)
	{
		for(ContentType t : l->dependency)
		{
			if(t == l->type)
				throw Format("Content manager: Loader '%s' depends on itself!", l->id);
			bool found = false;
			for(ContentLoader* l2 : loaders)
			{
				if(l2->type == t)
				{
					l->edge_from.push_back(l2);
					l2->edge_to.push_back(l);
					found = true;
					break;
				}
			}
			if(!found)
				throw Format("Content manager: Missing content loader for type %d for loader '%s'!", t, l->id);
		}
	}

	vector<ContentLoader*> S, L;

	// take all loader that are not dependecies
	uint total_edges = 0;
	for(ContentLoader* l : loaders)
	{
		if(l->edge_to.empty())
			S.push_back(l);
		l->incoming_edges = l->edge_to.size();
		total_edges += l->incoming_edges;
	}
	if(S.empty())
		throw "Content manager: Cyclic dependency in loaders, no free nodes!";

	// do magic!
	while(!S.empty())
	{
		ContentLoader* n = S.back();
		S.pop_back();
		L.push_back(n);

		for(ContentLoader* m : n->edge_to)
		{
			--m->incoming_edges;
			--total_edges;
			if(m->incoming_edges == 0)
				S.push_back(m);
		}
	}

	if(total_edges == 0)
		std::swap(L, loaders);
	else
		throw "Content manager: Cyclic dependency between loaders!";
}

//=================================================================================================
uint ContentManager::GetDatafileLoadersCount() const
{
	uint count = 0;
	for(ContentLoader* l : loaders)
	{
		if(IS_SET(l->flags, ContentLoader::HaveDatafile))
			++count;
	}
	return count;
}

//=================================================================================================
uint ContentManager::GetTextfileLoadersCount() const
{
	uint count = 0;
	for(ContentLoader* l : loaders)
	{
		if(IS_SET(l->flags, ContentLoader::HaveTextfile))
			++count;
	}
	return count;
}

//=================================================================================================
void ContentManager::LoadDatafiles()
{
	int errors = 0;

	for(ContentLoader* l : loaders)
	{
		if(!IS_SET(l->flags, ContentLoader::HaveDatafile))
			continue;

		resMgr.NextTask(Format("%s [%s]", txLoadingDatafiles, l->name));

		cstring path = Format("%s/%s.txt", g_system_dir.c_str(), l->id);
		if(!l->t.FromFile(path))
		{
			++errors;
			ERROR(Format("Content manager: Failed to open file '%s'.", path));
		}
		else
		{
			int e = l->Load();
			if(e > 0)
			{
				errors += e;
				ERROR(Format("Content manager: %d errors for content '%s'.", errors, l->id));
			}
		}
	}

	if(errors > 0)
		throw Format("Content manager: %d errors for datafiles loading. Check log for details.", errors);
}

//=================================================================================================
void ContentManager::LoadTextfiles()
{
	int errors = 0;

	for(ContentLoader* l : loaders)
	{
		if(!IS_SET(l->flags, ContentLoader::HaveTextfile))
			continue;

		resMgr.NextTask(Format("%s [%s]", txLoadingTextfiles, l->name));

		cstring path = Format("%s/lang/%s/%s.txt", g_system_dir.c_str(), g_lang_prefix.c_str(), l->id);
		if(!l->t2.FromFile(path))
		{
			++errors;
			ERROR(Format("Content manager: Failed to open file '%s'.", path));
		}
		else
		{
			int e = l->LoadText();
			if(e > 0)
			{
				errors += e;
				ERROR(Format("Content manager: %d errors for content text '%s'.", errors, l->id));
			}
		}
	}

	if(errors > 0)
		throw Format("Content manager: %d errors for textfiles loading. Check log for details.", errors);
}
