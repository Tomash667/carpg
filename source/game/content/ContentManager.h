#pragma once

//-----------------------------------------------------------------------------
class ContentLoader;
class ResourceManager;

//-----------------------------------------------------------------------------
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
	void WriteCrc(BitStream& stream);
	bool ReadCrc(BitStream& stream);
	bool ValidateCrc(uint& my_crc, uint& other_crc, int& type, cstring& type_str);
	bool GetCrc(int type, uint& my_crc, cstring& type_str);

private:
	void SetupTexts();
	void SortLoaders();
	
	ResourceManager& resMgr;
	vector<ContentLoader*> loaders;
	cstring txLoadingDatafiles, txLoadingTextfiles, txLoadingResources;
};
