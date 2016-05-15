#pragma once

class ContentLoader;

class ContentManager
{
public:
	void Load();
	void Reload(bool force = false);
	void Cleanup();

	void SortLoaders();
	void MarkAll();
	void DoLoading();
	
	vector<ContentLoader*> loaders;
};
