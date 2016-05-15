#include "Pch.h"
#include "Base.h"
#include "ContentManager.h"
#include "ContentLoader.h"

extern string g_system_dir;

void ContentManager::Load()
{
	SortLoaders();
	MarkAll();
}

void ContentManager::Reload(bool force)
{

}

void ContentManager::Cleanup()
{
	for(ContentLoader* l : loaders)
		l->Cleanup();
}

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
				throw Format("Content loader: Loader %d depends on itself!", t);
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
				throw Format("Content loader: Missing content loader for dependent type %d!", t);
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
		throw "Content loader: Cyclic dependency, no free nodes!";

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
		throw "Content loader: Cyclic dependency between loaders!";
}

void ContentManager::MarkAll()
{
	for(ContentLoader* l : loaders)
		l->marked = true;
}

void ContentManager::DoLoading()
{
	int errors = 0;

	for(ContentLoader* l : loaders)
	{
		if(l->marked)
		{
			cstring path = Format("%s/%s.txt", g_system_dir.c_str(), l->name);
			if(!l->t.FromFile(path))
			{
				++errors;
				ERROR(Format("Content loader: Failed to open file '%s'.", path));
			}
			else
			{
				int e = l->Load();
				if(e > 0)
				{
					errors += e;
					ERROR(Format("Content loader: %d errors for content '%s'.", l->name));
				}
			}
		}
	}

	if(errors > 0)
		throw Format("Content loader: %d errors. Check log for details.", errors);
}
