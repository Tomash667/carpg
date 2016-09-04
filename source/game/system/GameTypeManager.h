#pragma once

#include "GameType.h"

class Tokenizer;

template<typename T>
class SimpleGameTypeHandler : public GameTypeHandler
{
	typedef typename vector<T*>::iterator iterator;
public:
	inline SimpleGameTypeHandler(vector<T*>& container, uint id_offset) : container(container), id_offset(id_offset) {}
	inline ~SimpleGameTypeHandler()
	{
		DeleteElements(container);
	}
	inline GameTypeItem Find(const string& id, bool hint) override
	{
		for(T* item : container)
		{
			string& item_id = offset_cast<string>(item, id_offset);
			if(item_id == id)
				return item;
		}
		return nullptr;
	}
	inline GameTypeItem Create() override
	{
		return new T;
	}
	inline void Insert(GameTypeItem item) override
	{
		container.push_back((T*)item);
	}
	inline void Destroy(GameTypeItem item) override
	{
		T* t = (T*)item;
		delete t;
	}
	inline GameTypeItem GetFirstItem() override
	{
		it = container.begin();
		end = container.end();
		if(it == end)
			return nullptr;
		else
			return *it++;
	}
	inline GameTypeItem GetNextItem() override
	{
		if(it == end)
			return nullptr;
		else
			return *it++;
	}

private:
	vector<T*>& container;
	uint id_offset;
	iterator it, end;
};

class GameTypeManager
{
public:
	GameTypeManager(cstring system_path, cstring lang_path);
	~GameTypeManager();

	void Init();

	void Add(GameType* gametype, GameTypeHandler* handler);
	template<typename T>
	void Add(GameType* gametype, vector<T*>& container)
	{
		assert(gametype);
		assert(!gametype->fields.empty());
		Add(gametype, new SimpleGameTypeHandler<T>(container, gametype->fields[0]->offset));
	}
	inline GameType& GetGameType(GameTypeId gametype_id)
	{
		int i = (int)gametype_id;
		assert(i >= 0 && i < GT_Max);
		GameType* gametype = gametypes[i];
		assert(gametype->gametype_id == gametype_id);
		return *gametype;
	}
	int AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name = nullptr);
	
	// Load list of files containg gametypes/strings.
	bool LoadFilelist();
	// Load gametypes from text files.
	void LoadGameTypesFromText(uint& errors);
	// Load strings from text files.
	void LoadStringsFromText();

	void LogLoadedGameTypes();

	// crc functions
	void CalculateCrc();
	void WriteCrc(BitStream& stream);
	bool ReadCrc(BitStream& stream);
	bool GetCrc(GameTypeId gametype_id, uint& my_crc, cstring& name);
	bool ValidateCrc(GameTypeId& gametype_id, uint& my_crc, uint& other_crc, cstring& name);

private:
	struct File
	{
		string path;
		uint size;
	};

	void LoadGameTypesFromText(cstring filename);
	void LoadGameTypesFromTextImpl(uint& file_errors);
	bool LoadGameTypesFromTextSingle(GameType& gametype);
	void LoadStringsFromText(cstring filename);
	bool LoadStringsFromTextSingle(GameType& gametype);
	bool LoadGameTypeFilelist(Tokenizer& t);
	bool LoadStringsFilelist(Tokenizer& t);
	void VerifyStrings();
	GameTypeItem GetByAlias(GameType& gametype, const string* id = nullptr);

	vector<GameType*> gametypes;
	vector<File> system_files, lang_files;
	Tokenizer t;
	int free_group, errors;
	uint last_alias_offset;
	string system_path, lang_path, last_alias;
	bool need_calculate_crc;
};
