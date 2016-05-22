#pragma once

class ContentLoader;
class ResourceManager;

class ContentManager
{
public:
	ContentManager(ResourceManager& resMgr) : resMgr(resMgr) {}
	~ContentManager();

	inline void AddLoader(ContentLoader* l) { loaders.push_back(l); }
	uint GetDatafileLoadersCount() const;
	uint GetTextfileLoadersCount() const;
	void Init();
	void LoadDatafiles();
	void LoadTextfiles();

private:
	void SetupTexts();
	void SortLoaders();
	
	ResourceManager& resMgr;
	vector<ContentLoader*> loaders;
	cstring txLoadingDatafiles, txLoadingTextfiles, txLoadingResources;
};
