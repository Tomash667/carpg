#include "Pch.h"
#include "Base.h"
#include "DatatypeManager.h"

/*
TODO:
+ null reference override
+ check if string for reference is not empty or whitespace
*/

static cstring CATEGORY = "DatatypeManager";

enum Group
{
	G_DATATYPE,
	G_FREE
};

DatatypeManager::DatatypeManager(cstring _system_path, cstring _lang_path) : free_group(G_FREE), errors(0), need_calculate_crc(true)
{
	t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE | Tokenizer::F_FILE_INFO);
	datatypes.resize(DT_Max);
	system_path = Format("%s/datatypes/", _system_path);
	lang_path = Format("%s/datatypes/", _lang_path);
}

DatatypeManager::~DatatypeManager()
{

}

void DatatypeManager::Init()
{
	int index = 0;
	for(Datatype* datatype : datatypes)
	{
		assert(datatype);
		assert(datatype->datatype_id == (DatatypeId)index);

		for(Datatype::Field* field : datatype->fields)
		{
			if(field->type == Datatype::Field::REFERENCE)
			{
				Datatype* ref_datatype = datatypes[(int)field->datatype_id];
				assert(ref_datatype);
				field->id_offset = ref_datatype->fields[0]->offset;
			}
		}

		++index;
	}
}

void DatatypeManager::Add(Datatype* _datatype, DatatypeHandler* handler)
{
	assert(_datatype);
	assert(handler);

	Datatype& datatype = *_datatype;
	datatype.handler = handler;
	assert(!datatype.fields.empty());
	datatypes[(int)datatype.datatype_id] = _datatype;

	// set required fields
	assert(datatype.fields.size() < 32u);
	uint r = 0;
	for(uint i = 0, count = datatype.fields.size(); i < count; ++i)
	{
		if(datatype.fields[i]->required)
			r |= (1 << i);
		t.AddKeyword(datatype.fields[i]->name.c_str(), i, free_group);
	}
	datatype.required_fields = r;
	datatype.group = free_group++;

	// set required localized fields
	assert(datatype.localized_fields.size() < 32u);
	r = 0;
	for(uint i = 0, count = datatype.localized_fields.size(); i < count; ++i)
	{
		if(datatype.localized_fields[i]->required)
			r |= (1 << i);
		t.AddKeyword(datatype.localized_fields[i]->name.c_str(), i, free_group);
	}
	datatype.required_localized_fields = r;
	if(!datatype.localized_fields.empty())
		datatype.localized_group = free_group++;
}

void DatatypeManager::LoadDatatypesFromText()
{
	for(File& f : system_files)
		LoadDatatypesFromText(f.path.c_str());
}

void DatatypeManager::LoadDatatypesFromText(cstring filename)
{
	try
	{
		t.FromFile(filename);
		t.Next();

		while(!t.IsEof())
		{
			int id = t.MustGetKeywordId(G_DATATYPE);
			Datatype& datatype = GetDatatype((DatatypeId)id);
			t.Next();

			bool ok;

			if(t.IsSymbol('{'))
			{
				t.Next();
				while(!t.IsSymbol('}'))
				{
					ok = LoadDatatypesFromTextSingle(datatype);
					if(!ok)
						break;
				}
				if(ok)
					t.Next();
			}
			else
				ok = LoadDatatypesFromTextSingle(datatype);

			if(!ok)
			{
				t.SkipToKeywordGroup(G_DATATYPE);
			}
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse datatypes: %s", e.ToString()));
		++errors;
	}
}

struct DatatypeProxy
{
	inline explicit DatatypeProxy(Datatype& datatype, bool create = true) : datatype(datatype)
	{
		item = (create ? datatype.handler->Create() : nullptr);
	}
	inline ~DatatypeProxy()
	{
		if(item)
			datatype.handler->Destroy(item);
	}

	template<typename T>
	inline T& Get(uint offset)
	{
		return offset_cast<T>(item, offset);
	}

	inline string& GetId()
	{
		return Get<string>(datatype.fields[0]->offset);
	}

	inline cstring GetName()
	{
		if(item)
		{
			string& id = GetId();
			if(id.empty())
				return datatype.id.c_str();
			else
				return Format("%s %s", datatype.id.c_str(), id.c_str());
		}
		else
			return datatype.id.c_str();
	}

	DatatypeItem item;
	Datatype& datatype;
};

bool DatatypeManager::LoadDatatypesFromTextSingle(Datatype& datatype)
{
	DatatypeProxy proxy(datatype);
	DatatypeHandler& handler = *datatype.handler;

	try
	{
		// id
		const string& id = t.MustGetString();
		if(handler.Find(id, true) != nullptr)
			t.Throw("Id already used.");
		proxy.GetId() = id;
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		if(datatype.fields[0]->handler)
		{
			// custom handler for datatype
			datatype.fields[0]->handler->LoadText(t, proxy.item);
			handler.Insert(proxy.item);
			proxy.item = nullptr;
			return true;
		}

		uint set_fields = 1u;

		while(!t.IsSymbol('}'))
		{
			int keyword = t.MustGetKeywordId(datatype.group);
			Datatype::Field& field = *datatype.fields[keyword];
			t.Next();

			if(IS_SET(set_fields, 1 << keyword))
				t.Throw("Field '%s' already used.", field.name.c_str());

			switch(field.type)
			{
			case Datatype::Field::STRING:
			case Datatype::Field::MESH:
				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();
				break;
			case Datatype::Field::FLAGS:
				proxy.Get<int>(field.offset) = ReadFlags(t, field.keyword_group);
				t.Next();
				break;
			case Datatype::Field::REFERENCE:
				{
					const string& ref_id = t.MustGetString();
					DatatypeItem found = FindItem(field.datatype_id, ref_id);
					if(!found)
						t.Throw("Missing datatype %s '%s' for field '%s'.", GetDatatype(field.datatype_id).id.c_str(), ref_id.c_str(), field.name.c_str());
					proxy.Get<void*>(field.offset) = found;
				}
				break;
			case Datatype::Field::CUSTOM:
				field.handler->LoadText(t, proxy.item);
				break;
			}

			set_fields |= (1 << keyword);
		}
		t.Next();

		if(!IS_ALL_SET(datatype.required_fields, set_fields))
		{
			LocalString s = "Missing required fields: ";
			bool first = true;
			for(uint i = 1, count = datatype.fields.size(); i<count; ++i)
			{
				if(IS_SET(datatype.required_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
				{
					if(first)
						first = false;
					else
						s += ", ";
					s += datatype.fields[i]->name;
				}
			}
			s += ".";
			t.Throw(s.c_str());
		}

		handler.Insert(proxy.item);
		proxy.item = nullptr;
		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void DatatypeManager::LoadStringsFromText()
{
	for(File& f : lang_files)
		LoadStringsFromText(f.path.c_str());
}

void DatatypeManager::LoadStringsFromText(cstring filename)
{
	assert(filename);

	try
	{
		t.FromFile(filename);
		t.Next();

		while(true)
		{
			int id = t.MustGetKeywordId(G_DATATYPE);
			Datatype& datatype = GetDatatype((DatatypeId)id);

			bool ok;
			if(datatype.localized_fields.empty())
			{
				ERROR(Format("Type '%s' don't have any localized fields.", datatype.id.c_str()));
				t.Next();
				ok = false;
			}
			else
			{
				t.Next();
				if(t.IsSymbol('{'))
				{
					t.Next();
					while(!t.IsSymbol('}'))
					{
						ok = LoadStringsFromTextSingle(datatype);
						if(!ok)
							break;
					}
					if(ok)
						t.Next();
				}
				else
					ok = LoadStringsFromTextSingle(datatype);
			}

			if(!ok)
				t.SkipToKeywordGroup(G_DATATYPE);
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse datatypes: %s", e.ToString()));
		++errors;
	}
}

bool DatatypeManager::LoadStringsFromTextSingle(Datatype& datatype)
{
	DatatypeProxy proxy(datatype, false);

	try
	{
		const string& id = t.MustGetString();
		proxy.item = datatype.handler->Find(id, false);
		if(!proxy.item)
		{
			ERROR(Format("Missing '%s %s'.", datatype.id.c_str(), id.c_str()));
			++errors;
			return false;
		}
		t.Next();

		if(t.IsSymbol('='))
		{
			if(datatype.localized_fields.size() != 1u)
				t.Throw("Can't use simple string assigment when datatype have multiple fields.");
			t.Next();
			proxy.Get<string>(datatype.localized_fields[0]->offset) = t.MustGetString();
			t.Next();
		}
		else
		{
			t.AssertSymbol('{');
			t.Next();

			uint set_fields = 0;

			while(!t.IsSymbol('}'))
			{
				int keyword = t.MustGetKeywordId(datatype.localized_group);
				Datatype::LocalizedField& field = *datatype.localized_fields[keyword];
				t.Next();

				if(IS_SET(set_fields, 1 << keyword))
					t.Throw("Field '%s' already used.", field.name.c_str());

				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();

				set_fields |= (1 << keyword);
			}
			t.Next();

			if(!IS_ALL_SET(datatype.required_localized_fields, set_fields))
			{
				LocalString s = "Missing required fields: ";
				bool first = true;
				for(uint i = 1, count = datatype.localized_fields.size(); i<count; ++i)
				{
					if(IS_SET(datatype.required_localized_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
					{
						if(first)
							first = false;
						else
							s += ", ";
						s += datatype.localized_fields[i]->name;
					}
				}
				s += ".";
				t.Throw(s.c_str());
			}
		}

		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse localized '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

int DatatypeManager::AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords)
{
	t.AddKeywords(free_group, keywords);
	return free_group++;
}

void DatatypeManager::CalculateCrc()
{
	if(!need_calculate_crc)
		return;
	for(Datatype* datatype : datatypes)
		datatype->CalculateCrc();
	need_calculate_crc = false;
}

void DatatypeManager::WriteCrc(BitStream& stream)
{
	for(Datatype* datatype : datatypes)
		stream.Write(datatype->crc);
}

bool DatatypeManager::ReadCrc(BitStream& stream)
{
	bool ok = true;
	for(Datatype* datatype : datatypes)
		ok = stream.Read(datatype->other_crc) && ok;
	return ok;
}

bool DatatypeManager::GetCrc(DatatypeId datatype_id, uint& my_crc, cstring& name)
{
	if((int)datatype_id < 0 || datatype_id >= DT_Max)
		return false;
	Datatype* datatype = datatypes[(int)datatype_id];
	my_crc = datatype->crc;
	name = datatype->id.c_str();
	return true;
}

bool DatatypeManager::ValidateCrc(DatatypeId& datatype_id, uint& my_crc, uint& other_crc, cstring& name)
{
	for(Datatype* datatype : datatypes)
	{
		if(datatype->crc != datatype->other_crc)
		{
			datatype_id = datatype->datatype_id;
			my_crc = datatype->crc;
			other_crc = datatype->other_crc;
			name = datatype->id.c_str();
			return false;
		}
	}
	return true;
}

bool DatatypeManager::LoadFilelist()
{
	Tokenizer t;
	bool ok = LoadDatatypeFilelist(t);
	return LoadDatatypeLanguageFilelist(t) && ok;
}

bool DatatypeManager::LoadDatatypeFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", system_path.c_str());
	if(!core::io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(system_files);
		file.path = Format("%s%s", system_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		logger->Error(CATEGORY, "Failed to load datatype filelist.");
		return false;
	}
	else
		return true;
}

bool DatatypeManager::LoadDatatypeLanguageFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", lang_path.c_str());
	if(!core::io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(lang_files);
		file.path = Format("%s%s", lang_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		logger->Error(CATEGORY, "Failed to load language datatype filelist.");
		return false;
	}
	else
		return true;
}
