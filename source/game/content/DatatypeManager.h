#pragma once

#include "Datatype.h"

class Tokenizer;

template<typename T>
class SimpleDatatypeHandler : public DatatypeHandler
{
	typedef typename vector<T*>::iterator iterator;
public:
	inline SimpleDatatypeHandler(vector<T*>& container, uint id_offset) : container(container), id_offset(id_offset) {}
	inline ~SimpleDatatypeHandler()
	{
		DeleteElements(container);
	}
	inline DatatypeItem Find(const string& id, bool hint) override
	{
		for(T* item : container)
		{
			string& item_id = offset_cast<string>(item, id_offset);
			if(item_id == id)
				return item;
		}
		return nullptr;
	}
	inline DatatypeItem Create() override
	{
		return new T;
	}
	inline void Insert(DatatypeItem item) override
	{
		container.push_back((T*)item);
	}
	inline void Destroy(DatatypeItem item) override
	{
		T* t = (T*)item;
		delete t;
	}
	inline DatatypeItem GetFirstItem() override
	{
		it = container.begin();
		end = container.end();
		if(it == end)
			return nullptr;
		else
			return *it++;
	}
	inline DatatypeItem GetNextItem() override
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

class DatatypeManager
{
public:
	DatatypeManager(cstring system_path, cstring lang_path);
	~DatatypeManager();

	void Init();

	void Add(Datatype* datatype, DatatypeHandler* handler);
	template<typename T>
	void Add(Datatype* datatype, vector<T*>& container)
	{
		assert(datatype);
		assert(!datatype->fields.empty());
		Add(datatype, new SimpleDatatypeHandler<T>(container, datatype->fields[0]->offset));
	}
	inline Datatype& GetDatatype(DatatypeId datatype_id)
	{
		int i = (int)datatype_id;
		assert(i >= 0 && i < DT_Max);
		Datatype* datatype = datatypes[i];
		assert(datatype->datatype_id == datatype_id);
		return *datatype;
	}
	int AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name = nullptr);
	
	// Load list of files containg datatypes/strings.
	bool LoadFilelist();
	// Load datatypes from text files.
	void LoadDatatypesFromText(uint& errors);
	// Load strings from text files.
	void LoadStringsFromText();

	void LogLoadedDatatypes();

	// crc functions
	void CalculateCrc();
	void WriteCrc(BitStream& stream);
	bool ReadCrc(BitStream& stream);
	bool GetCrc(DatatypeId datatype_id, uint& my_crc, cstring& name);
	bool ValidateCrc(DatatypeId& datatype_id, uint& my_crc, uint& other_crc, cstring& name);

private:
	struct File
	{
		string path;
		uint size;
	};

	void LoadDatatypesFromText(cstring filename);
	void LoadDatatypesFromTextImpl(uint& file_errors);
	bool LoadDatatypesFromTextSingle(Datatype& datatype);
	void LoadStringsFromText(cstring filename);
	bool LoadStringsFromTextSingle(Datatype& datatype);
	bool LoadDatatypeFilelist(Tokenizer& t);
	bool LoadDatatypeLanguageFilelist(Tokenizer& t);
	void VerifyStrings();
	DatatypeItem GetByAlias(Datatype& datatype, const string* id = nullptr);

	vector<Datatype*> datatypes;
	vector<File> system_files, lang_files;
	Tokenizer t;
	int free_group, errors;
	uint last_alias_offset;
	string system_path, lang_path, last_alias;
	bool need_calculate_crc;
};
