#pragma once

#include "Type.h"

class TextWriter;
class Tokenizer;

//-----------------------------------------------------------------------------
class TypeManager
{
public:
	TypeManager(cstring system_path, cstring lang_path);
	~TypeManager();

	void Init();
	void Add(Type* type);
	Type& GetType(TypeId type_id);
	int AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name = nullptr);
	void OrderDependencies();	
	bool LoadFilelist();
	void LoadTypes(uint& errors);
	void LoadStrings();
	void LogLoadedTypes();

	// crc functions
	void CalculateCrc();
	void WriteCrc(BitStream& stream);
	bool ReadCrc(BitStream& stream);
	bool GetCrc(TypeId type_id, uint& my_crc, cstring& name);
	bool ValidateCrc(TypeId& type_id, uint& my_crc, uint& other_crc, cstring& name);

	bool Save();
	//void Merge(GameTypeId type_id, vector<Game)
	static TypeManager& Get() { return *type_manager; }

private:
	struct File
	{
		string path;
		uint size;
	};

	void LoadTypes(cstring filename);
	void LoadTypesImpl(uint& file_errors);
	bool LoadType(Type& type);
	void LoadStrings(cstring filename);
	bool LoadStringsImpl(Type& type);
	bool LoadTypeFilelist(Tokenizer& t);
	bool LoadStringsFilelist(Tokenizer& t);
	void VerifyStrings();
	bool SaveGroup(const string& file_group);
	void SaveType(Type& type, TextWriter& f);

	static TypeManager* type_manager;
	vector<Type*> types;
	vector<File> system_files, lang_files;
	vector<TypeId> load_order;
	Tokenizer t;
	int free_group, errors;
	string system_path, lang_path, last_alias;
	bool need_calculate_crc;
};
