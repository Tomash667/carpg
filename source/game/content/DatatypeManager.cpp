#include "Pch.h"
#include "Base.h"
#include "DatatypeManager.h"

/*
TODO:
+ null reference override
+ check if string for reference is not empty or whitespace
*/

const uint MAX_ERRORS = 100;

enum Group
{
	G_DATATYPE,
	G_STRING_KEYWORD,
	G_FREE
};

DatatypeManager::DatatypeManager(cstring _system_path, cstring _lang_path) : free_group(G_FREE), errors(0), need_calculate_crc(true)
{
	t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE | Tokenizer::F_FILE_INFO);
	t.AddKeywordGroup("datatype", G_DATATYPE);
	t.AddKeywords(G_STRING_KEYWORD, { { "alias", 0 } }, "string keyword");
	datatypes.resize(DT_Max);
	system_path = Format("%s/datatypes/", _system_path);
	lang_path = Format("%s/datatypes/", _lang_path);
}

DatatypeManager::~DatatypeManager()
{
	DeleteElements(datatypes);
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
	datatype.group_name = Format("%s field", datatype.id.c_str());
	assert(!datatype.fields.empty());
	datatypes[(int)datatype.datatype_id] = _datatype;
	t.AddKeyword(datatype.id.c_str(), datatype.datatype_id, G_DATATYPE);

	// set required fields
	assert(datatype.fields.size() < 32u);
	uint r = 0;
	datatype.alias = -1;
	for(uint i = 0, count = datatype.fields.size(); i < count; ++i)
	{
		Datatype::Field& field = *datatype.fields[i];
		if(field.required)
			r |= (1 << i);
		if(!field.name.empty())
			t.AddKeyword(field.name.c_str(), i, free_group);
		if(field.alias)
		{
			assert(datatype.alias == -1);
			datatype.alias = i;
		}
	}
	datatype.required_fields = r;
	t.AddKeywordGroup(datatype.group_name.c_str(), free_group);
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
	{
		t.AddKeywordGroup(datatype.group_name.c_str(), free_group);
		datatype.localized_group = free_group++;
	}
}

void DatatypeManager::LoadDatatypesFromText(uint& _errors)
{
	for(File& f : system_files)
		LoadDatatypesFromText(f.path.c_str());
	_errors += errors;

	for(Datatype* datatype : datatypes)
		datatype->handler->AfterLoad();
}

void DatatypeManager::LoadDatatypesFromText(cstring filename)
{
	INFO(Format("DatatypeManager: Parsing file '%s'.", filename));

	uint file_errors = 0;

	if(!t.FromFile(filename))
	{
		ERROR(Format("DatatypeManager: Failed to open file '%s'.", filename));
		++errors;
		return;
	}

	try
	{
		t.Next();

		LoadDatatypesFromTextImpl(file_errors);

		if(file_errors >= MAX_ERRORS)
		{
			ERROR(Format("DatatypeManager: Errors limit exceeded in file '%s'.", filename));
			++errors;
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("DatatypeManager: Failed to parse datatypes: %s", e.ToString()));
		++errors;
	}
}

void DatatypeManager::LoadDatatypesFromTextImpl(uint& file_errors)
{
	while(!t.IsEof())
	{
		int id = t.MustGetKeywordId(G_DATATYPE);
		Datatype& datatype = GetDatatype((DatatypeId)id);
		t.Next();

		if(t.IsSymbol('{'))
		{
			tokenizer::Pos block_start_pos = t.GetPos();
			t.Next();
			bool ok = true;
			while(!t.IsSymbol('}'))
			{
				tokenizer::Pos entry_start_pos = t.GetPos();
				if(!LoadDatatypesFromTextSingle(datatype))
				{
					++file_errors;
					if(file_errors >= MAX_ERRORS)
						return;
					// skip to end of broken entry
					t.MoveTo(entry_start_pos);
					if(t.IsString())
					{
						t.Next();
						if(!t.IsSymbol('{'))
						{
							// unexpected item
							ok = false;
							break;
						}
						else
						{
							if(!t.MoveToClosingSymbol('{', '}'))
								t.Throw("Missing closing bracket started at (%u,%u).", entry_start_pos.line, entry_start_pos.charpos);
							t.Next();
						}
					}
					else
					{
						// unexpected item
						ok = false;
						break;
					}
				}
			}

			if(ok)
				t.Next();
			else
			{
				t.MoveTo(block_start_pos);
				if(!t.MoveToClosingSymbol('{', '}'))
					t.Throw("Missing closing bracket started at (%u,%u).", block_start_pos.line, block_start_pos.charpos);
				t.Next();
			}
		}
		else
		{
			tokenizer::Pos entry_start_pos = t.GetPos();
			if(!LoadDatatypesFromTextSingle(datatype))
			{
				++file_errors;
				if(file_errors >= MAX_ERRORS)
					return;

				// skip to end of broken entry
				t.MoveTo(entry_start_pos);
				if(t.IsString())
				{
					t.Next();
					if(!t.IsSymbol('{'))
					{
						// unexpected item
						t.SkipToKeywordGroup(G_DATATYPE);
					}
					else
					{
						if(!t.MoveToClosingSymbol('{', '}'))
							t.Throw("Missing closing bracket started at (%u,%u).", entry_start_pos.line, entry_start_pos.charpos);
						t.Next();
					}
				}
				else
				{
					// unexpected item
					t.SkipToKeywordGroup(G_DATATYPE);
				}
			}
		}
	}
}

struct DatatypeProxy
{
	inline explicit DatatypeProxy(Datatype& datatype, bool create = true, bool own = true) : datatype(datatype), own(own)
	{
		item = (create ? datatype.handler->Create() : nullptr);
	}
	inline ~DatatypeProxy()
	{
		if(item && own)
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
	bool own;
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
			datatype.loaded++;
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
					DatatypeItem found = GetDatatype(field.datatype_id).handler->Find(ref_id, false);
					if(!found)
						t.Throw("Missing datatype %s '%s' for field '%s'.", GetDatatype(field.datatype_id).id.c_str(), ref_id.c_str(), field.name.c_str());
					proxy.Get<void*>(field.offset) = found;
					t.Next();
					if(field.callback >= 0)
						datatype.handler->Callback(proxy.item, found, field.callback);
				}
				break;
			case Datatype::Field::CUSTOM:
				field.handler->LoadText(t, proxy.item);
				break;
			}

			set_fields |= (1 << keyword);
		}
		t.Next();

		if(!IS_ALL_SET(set_fields, datatype.required_fields))
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
		datatype.loaded++;
		proxy.item = nullptr;
		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("DatatypeManager: Failed to parse '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void DatatypeManager::LoadStringsFromText()
{
	for(File& f : lang_files)
		LoadStringsFromText(f.path.c_str());

	VerifyStrings();
}

void DatatypeManager::LoadStringsFromText(cstring filename)
{
	assert(filename);

	try
	{
		t.FromFile(filename);
		t.Next();

		while(!t.IsEof())
		{
			int id = t.MustGetKeywordId(G_DATATYPE);
			Datatype& datatype = GetDatatype((DatatypeId)id);

			bool ok;
			if(datatype.localized_fields.empty())
			{
				ERROR(Format("DatatypeManager: Type '%s' don't have any localized fields.", datatype.id.c_str()));
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
		ERROR(Format("DatatypeManager: Failed to parse datatypes: %s", e.ToString()));
		++errors;
	}
}

bool DatatypeManager::LoadStringsFromTextSingle(Datatype& datatype)
{
	DatatypeProxy proxy(datatype, false, false);

	try
	{
		bool alias = false;

		if(t.IsString())
		{
			const string& id = t.MustGetString();
			proxy.item = datatype.handler->Find(id, false);
			if(!proxy.item)
			{
				ERROR(Format("DatatypeManager: Missing '%s %s'.", datatype.id.c_str(), id.c_str()));
				++errors;
				return false;
			}
			t.Next();
		}
		else if(t.IsKeyword(0, G_STRING_KEYWORD))
		{
			if(datatype.alias == -1)
				t.Throw("Datatype '%s' don't have alias.", datatype.id.c_str());
			t.Next();
			const string& id = t.MustGetString();
			DatatypeItem item = GetByAlias(datatype, &id);
			if(!item)
			{
				WARN(Format("DatatypeManager: Unused '%s' alias '%s'.", datatype.id.c_str(), id.c_str()));
				++errors;
				return false;
			}
			proxy.item = item;
			alias = true;
			t.Next();
		}
		else
		{
			int keyword = 0;
			int group = G_STRING_KEYWORD;
			t.StartUnexpected().Add(tokenizer::T_STRING).Add(tokenizer::T_KEYWORD, &keyword, &group).Throw();
		}
		
		uint set_fields = 0;
		if(t.IsSymbol('='))
		{
			if(datatype.localized_fields.size() != 1u)
				t.Throw("Can't use simple string assigment when datatype have multiple fields.");
			t.Next();
			proxy.Get<string>(datatype.localized_fields[0]->offset) = t.MustGetString();
			t.Next();
			set_fields = 1u;
		}
		else
		{
			t.AssertSymbol('{');
			t.Next();
			
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

			if(!IS_ALL_SET(set_fields, datatype.required_localized_fields))
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

		if(alias)
		{
			DatatypeItem parent = proxy.item;
			DatatypeItem child;
			while((child = GetByAlias(datatype)) != nullptr)
			{
				for(uint i = 0, count = datatype.localized_fields.size(); i < count; ++i)
				{
					if(IS_SET(set_fields, 1 << i))
					{
						uint offset = datatype.localized_fields[i]->offset;
						offset_cast<string>(child, offset) = offset_cast<string>(parent, offset);
					}
				}
			}
		}

		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("DatatypeManager: Failed to parse localized '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void DatatypeManager::VerifyStrings()
{
	for(Datatype* dt_ptr : datatypes)
	{
		Datatype& datatype = *dt_ptr;
		if(datatype.required_localized_fields != 0u)
		{
			uint id_offset = datatype.GetIdOffset();
			DatatypeItem item = datatype.handler->GetFirstItem();
			while(item)
			{
				for(Datatype::LocalizedField* field : datatype.localized_fields)
				{
					if(field->required)
					{
						if(offset_cast<string>(item, field->offset).empty())
						{
							++errors;
							WARN(Format("DatatypeManager: Missing localized string '%s' for '%s %s'.", field->name.c_str(), datatype.id.c_str(),
								offset_cast<string>(item, id_offset).c_str()));
						}
					}
				}
				item = datatype.handler->GetNextItem();
			}
		}
	}
}

DatatypeItem DatatypeManager::GetByAlias(Datatype& datatype, const string* alias)
{
	DatatypeItem item = nullptr;
	if(alias)
	{
		last_alias_offset = datatype.fields[datatype.alias]->offset;
		last_alias = *alias;
		item = datatype.handler->GetFirstItem();
	}
	else
		item = datatype.handler->GetNextItem();

	while(item && offset_cast<string>(item, last_alias_offset) != last_alias)
		item = datatype.handler->GetNextItem();

	return item;
}

int DatatypeManager::AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name)
{
	t.AddKeywords(free_group, keywords, group_name);
	return free_group++;
}

void DatatypeManager::CalculateCrc()
{
	if(!need_calculate_crc)
		return;
	for(Datatype* datatype : datatypes)
	{
		datatype->CalculateCrc();
		INFO(Format("DatatypeManager: Crc for %s (%p).", datatype->id.c_str(), datatype->crc));
	}
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
		ERROR("DatatypeManager: Failed to load datatype filelist.");
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
		ERROR("DatatypeManager: Failed to load language datatype filelist.");
		return false;
	}
	else
		return true;
}

void DatatypeManager::LogLoadedDatatypes()
{
	for(Datatype* datatype : datatypes)
		INFO(Format("DatatypeManager: Loaded %s(s): %u.", datatype->id.c_str(), datatype->loaded));
}
