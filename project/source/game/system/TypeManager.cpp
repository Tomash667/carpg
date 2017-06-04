#include "Pch.h"
#include "Base.h"
#include "TextWriter.h"
#include "TypeManager.h"
#include "TypeProxy.h"

/*
TODO:
+ null reference override
+ check if string for reference is not empty or whitespace
*/

const uint MAX_ERRORS = 100;
TypeManager* TypeManager::type_manager;

enum Group
{
	G_TYPE,
	G_TYPE_BASE,
	G_FREE
};

TypeManager::TypeManager(cstring _system_path, cstring _lang_path) : free_group(G_FREE), errors(0), need_calculate_crc(true)
{
	type_manager = this;
	t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE | Tokenizer::F_FILE_INFO);
	t.AddKeywordGroup("type", G_TYPE);
	t.AddKeywordGroup("base type keyword", G_TYPE_BASE);
	t.AddKeyword("toolset_path", 0, G_TYPE_BASE);
	types.resize((uint)TypeId::Max);
	system_path = Format("%s/types/", _system_path);
	lang_path = Format("%s/types/", _lang_path);
}

TypeManager::~TypeManager()
{
	for(auto type : types)
	{
		type->DeleteContainer();
		delete type;
	}
}

void TypeManager::Init()
{
	int index = 0;

	for(Type* type : types)
	{
		assert(type);
		assert(type->type_id == (TypeId)index);

		for(Type::Field* field : type->fields)
		{
			if(field->type == Type::Field::REFERENCE)
			{
				Type* ref_type = types[(int)field->type_id];
				assert(ref_type);
				field->id_offset = ref_type->fields[0]->offset;
			}
		}

		++index;
	}
}

void TypeManager::OrderDependencies()
{
	Graph g((int)TypeId::Max);

	for(Type& type : Deref(types))
	{
		for(TypeId dep : type.depends_on)
			g.AddEdge((int)dep, (int)type.type_id);
	}

	bool ok = g.Sort();
	assert(ok); // if failed types have cycles dependency

	auto& result = g.GetResult();
	load_order.reserve(result.size());
	for(int id : result)
		load_order.push_back((TypeId)id);
}

void TypeManager::Add(Type* _type)
{
	assert(_type);
	assert(_type->container);

	Type& type = *_type;
	type.changes = false;
	type.group_name = Format("%s field", type.token.c_str());
	assert(!type.fields.empty());
	types[(int)type.type_id] = _type;
	t.AddKeyword(type.token.c_str(), (int)type.type_id, G_TYPE);

	// set required fields
	assert(type.fields.size() < 32u);
	uint r = 0;
	for(uint i = 0, count = type.fields.size(); i < count; ++i)
	{
		Type::Field& field = *type.fields[i];
		if(field.required)
			r |= (1 << i);
		if(!field.name.empty())
			t.AddKeyword(field.name.c_str(), i, free_group);
	}
	type.required_fields = r;
	t.AddKeywordGroup(type.group_name.c_str(), free_group);
	type.group = free_group++;

	// set required localized fields
	assert(type.localized_fields.size() < 32u);
	r = 0;
	for(uint i = 0, count = type.localized_fields.size(); i < count; ++i)
	{
		if(type.localized_fields[i]->required)
			r |= (1 << i);
		t.AddKeyword(type.localized_fields[i]->name.c_str(), i, free_group);
	}
	type.required_localized_fields = r;
	if(!type.localized_fields.empty())
	{
		t.AddKeywordGroup(type.group_name.c_str(), free_group);
		type.localized_group = free_group++;
	}
}

Type& TypeManager::GetType(TypeId type_id)
{
	int index = (int)type_id;
	assert(index >= 0 && index < (int)TypeId::Max);
	Type* type = types[index];
	assert(type && type->type_id == type_id);
	return *type;
}

void TypeManager::LoadTypes(uint& _errors)
{
	for(File& f : system_files)
		LoadTypes(f.path.c_str());
	_errors += errors;

	for(Type* type : types)
		type->AfterLoad();
}

void TypeManager::LoadTypes(cstring filename)
{
	INFO(Format("TypeManager: Parsing file '%s'.", filename));

	if(!t.FromFile(filename))
	{
		ERROR(Format("TypeManager: Failed to open file '%s'.", filename));
		++errors;
		return;
	}

	try
	{
		t.Next();

		uint file_errors = 0;
		LoadTypesImpl(file_errors);

		if(file_errors >= MAX_ERRORS)
		{
			ERROR(Format("TypeManager: Errors limit exceeded in file '%s'.", filename));
			++errors;
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("TypeManager: Failed to parse types: %s", e.ToString()));
		++errors;
	}
}

void TypeManager::LoadTypesImpl(uint& file_errors)
{
	while(!t.IsEof())
	{
		int id = t.MustGetKeywordId(G_TYPE);
		Type& type = GetType((TypeId)id);
		t.Next();

		if(t.IsSymbol('{'))
		{
			tokenizer::Pos block_start_pos = t.GetPos();
			t.Next();
			bool ok = true;
			while(!t.IsSymbol('}'))
			{
				tokenizer::Pos entry_start_pos = t.GetPos();
				if(!LoadType(type))
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
			if(!LoadType(type))
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
						t.SkipToKeywordGroup(G_TYPE);
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
					t.SkipToKeywordGroup(G_TYPE);
				}
			}
		}
	}
}

bool TypeManager::LoadType(Type& type)
{
	TypeProxy proxy(type);

	try
	{
		// id
		const string& id = t.MustGetString();
		if(type.container->Find(id))
			t.Throw("Id already used.");
		proxy.GetId() = id;
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		Type::Field& id_field = *type.fields[0];
		if(id_field.handler)
		{
			// custom handler for type
			id_field.handler->LoadText(t, proxy.item, id_field.offset);
			type.container->Add(proxy.item);
			type.loaded++;
			proxy.item = nullptr;
			return true;
		}

		uint set_fields = 1u;

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeywordGroup(G_TYPE_BASE))
			{
				t.Next();
				proxy.item->toolset_path = t.MustGetString();

				t.Next();
				continue;
			}

			int keyword = t.MustGetKeywordId(type.group);
			Type::Field& field = *type.fields[keyword];
			t.Next();

			if(IS_SET(set_fields, 1 << keyword))
				t.Throw("Field '%s' already used.", field.name.c_str());

			switch(field.type)
			{
			case Type::Field::STRING:
			case Type::Field::MESH:
				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();
				break;
			case Type::Field::FLAGS:
				proxy.Get<int>(field.offset) = ReadFlags(t, field.keyword_group);
				t.Next();
				break;
			case Type::Field::REFERENCE:
				{
					const string& ref_id = t.MustGetString();
					TypeItem* found = GetType(field.type_id).container->Find(ref_id);
					if(!found)
						t.Throw("Missing type %s '%s' for field '%s'.", GetType(field.type_id).token.c_str(), ref_id.c_str(), field.name.c_str());
					proxy.Get<void*>(field.offset) = found;
					t.Next();
					if(field.callback >= 0)
						type.ReferenceCallback(proxy.item, found, field.callback);
				}
				break;
			case Type::Field::CUSTOM:
				field.handler->LoadText(t, proxy.item, field.offset);
				break;
			}

			set_fields |= (1 << keyword);
		}
		t.Next();

		if(!IS_ALL_SET(set_fields, type.required_fields))
		{
			LocalString s = "Missing required fields: ";
			bool first = true;
			for(uint i = 1, count = type.fields.size(); i<count; ++i)
			{
				if(IS_SET(type.required_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
				{
					if(first)
						first = false;
					else
						s += ", ";
					s += type.fields[i]->name;
				}
			}
			s += ".";
			t.Throw(s.c_str());
		}

		type.container->Add(proxy.item);
		type.loaded++;
		proxy.item = nullptr;
		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("TypeManager: Failed to parse '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void TypeManager::LoadStrings()
{
	for(File& f : lang_files)
		LoadStrings(f.path.c_str());

	VerifyStrings();
}

void TypeManager::LoadStrings(cstring filename)
{
	assert(filename);

	try
	{
		t.FromFile(filename);
		t.Next();

		while(!t.IsEof())
		{
			int id = t.MustGetKeywordId(G_TYPE);
			Type& type = GetType((TypeId)id);

			bool ok;
			if(type.localized_fields.empty())
			{
				ERROR(Format("TypeManager: Type '%s' don't have any localized fields.", type.token.c_str()));
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
						ok = LoadStringsImpl(type);
						if(!ok)
							break;
					}
					if(ok)
						t.Next();
				}
				else
					ok = LoadStringsImpl(type);
			}

			if(!ok)
				t.SkipToKeywordGroup(G_TYPE);
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("TypeManager: Failed to parse types: %s", e.ToString()));
		++errors;
	}
}

bool TypeManager::LoadStringsImpl(Type& type)
{
	TypeProxy proxy(type, false, false);

	try
	{
		const string& id = t.MustGetString();
		proxy.item = type.container->Find(id);
		if(!proxy.item)
		{
			ERROR(Format("TypeManager: Missing '%s %s'.", type.token.c_str(), id.c_str()));
			++errors;
			return false;
		}
		t.Next();
		
		uint set_fields = 0;
		if(t.IsSymbol('='))
		{
			if(type.localized_fields.size() != 1u)
				t.Throw("Can't use simple string assigment when type have multiple fields.");
			t.Next();
			proxy.Get<string>(type.localized_fields[0]->offset) = t.MustGetString();
			t.Next();
			set_fields = 1u;
		}
		else
		{
			t.AssertSymbol('{');
			t.Next();
			
			while(!t.IsSymbol('}'))
			{
				int keyword = t.MustGetKeywordId(type.localized_group);
				Type::LocalizedField& field = *type.localized_fields[keyword];
				t.Next();

				if(IS_SET(set_fields, 1 << keyword))
					t.Throw("Field '%s' already used.", field.name.c_str());

				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();

				set_fields |= (1 << keyword);
			}
			t.Next();

			if(!IS_ALL_SET(set_fields, type.required_localized_fields))
			{
				LocalString s = "Missing required fields: ";
				bool first = true;
				for(uint i = 1, count = type.localized_fields.size(); i<count; ++i)
				{
					if(IS_SET(type.required_localized_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
					{
						if(first)
							first = false;
						else
							s += ", ";
						s += type.localized_fields[i]->name;
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
		ERROR(Format("TypeManager: Failed to parse localized '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void TypeManager::VerifyStrings()
{
	for(Type& type : Deref(types))
	{
		if(type.required_localized_fields == 0u)
			continue;

		uint id_offset = type.GetIdOffset();
		for(TypeItem* item : type.container->ForEach())
		{
			for(Type::LocalizedField* field : type.localized_fields)
			{
				if(field->required)
				{
					if(offset_cast<string>(item, field->offset).empty())
					{
						++errors;
						WARN(Format("TypeManager: Missing localized string '%s' for '%s %s'.", field->name.c_str(), type.token.c_str(),
							offset_cast<string>(item, id_offset).c_str()));
					}
				}
			}
		}
	}
}

int TypeManager::AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const& keywords, cstring group_name)
{
	t.AddKeywords(free_group, keywords, group_name);
	return free_group++;
}

int TypeManager::AddKeywords(std::initializer_list<Type::FlagDTO> const& flags, cstring group_name)
{
	if(group_name)
		t.AddKeywordGroup(group_name, free_group);

	for(auto& flag : flags)
		t.AddKeyword(flag.id, flag.value, free_group);

	return free_group++;
}

void TypeManager::CalculateCrc()
{
	if(!need_calculate_crc)
		return;
	for(Type* type : types)
	{
		type->CalculateCrc();
		INFO(Format("TypeManager: Crc for %s (%p).", type->token.c_str(), type->crc));
	}
	need_calculate_crc = false;
}

void TypeManager::WriteCrc(BitStream& stream)
{
	for(Type* type : types)
		stream.Write(type->crc);
}

bool TypeManager::ReadCrc(BitStream& stream)
{
	bool ok = true;
	for(Type* type : types)
		ok = stream.Read(type->other_crc) && ok;
	return ok;
}

bool TypeManager::GetCrc(TypeId type_id, uint& my_crc, cstring& name)
{
	if((int)type_id < 0 || type_id >= TypeId::Max)
		return false;
	Type* type = types[(int)type_id];
	my_crc = type->crc;
	name = type->token.c_str();
	return true;
}

bool TypeManager::ValidateCrc(TypeId& type_id, uint& my_crc, uint& other_crc, cstring& name)
{
	for(Type* type : types)
	{
		if(type->crc != type->other_crc)
		{
			type_id = type->type_id;
			my_crc = type->crc;
			other_crc = type->other_crc;
			name = type->token.c_str();
			return false;
		}
	}
	return true;
}

bool TypeManager::LoadFilelist()
{
	Tokenizer t;
	bool ok = LoadTypeFilelist(t);
	return LoadStringsFilelist(t) && ok;
}

bool TypeManager::LoadTypeFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", system_path.c_str());
	if(!io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(system_files);
		file.path = Format("%s%s", system_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		ERROR("TypeManager: Failed to load type filelist.");
		return false;
	}
	else
		return true;
}

bool TypeManager::LoadStringsFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", lang_path.c_str());
	if(!io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(lang_files);
		file.path = Format("%s%s", lang_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		ERROR("TypeManager: Failed to load language type filelist.");
		return false;
	}
	else
		return true;
}

void TypeManager::LogLoadedTypes()
{
	for(Type* type : types)
		INFO(Format("TypeManager: Loaded %s(s): %u.", type->token.c_str(), type->loaded));
}

bool TypeManager::Save()
{
	INFO("TypeManager: Saving changes...");

	for(Type* type : types)
		type->processed = false;

	bool any = false;
	bool ok = true;
	for(Type* type : types)
	{
		if(type->changes && !type->processed)
		{
			ok = SaveGroup(type->file_group) && ok;
			any = true;
		}
	}

	INFO(any ? "TypeManager: Saved." : "TypeManager: No changes.");
	return ok;
}

bool TypeManager::SaveGroup(const string& file_group)
{
	cstring path = Format("%s%s.txt", system_path.c_str(), file_group.c_str());
	TextWriter f(path);
	if(!f)
	{
		ERROR(Format("TypeManager: Failed to open file '%s' for writing.", path));
		return false;
	}

	for(TypeId type_id : load_order)
	{
		Type& type = *types[(int)type_id];
		if(type.file_group == file_group)
		{
			SaveType(type, f);
			type.processed = true;
			type.changes = false;
		}
	}

	return true;
}

void TypeManager::SaveType(Type& type, TextWriter& f)
{
	f << "//================================================================================\n";
	f << "// ";
	f << type.name;
	f << '\n';
	f << "//================================================================================\n";

	for(auto e : type.container->ForEach())
	{
		TypeProxy proxy(type, e);
		f << Format("%s \"%s\" {\n", type.token.c_str(), proxy.GetId().c_str());
		if(!proxy.item->toolset_path.empty())
			f << Format("\ttoolset_path \"%s\"\n", proxy.item->toolset_path.c_str());

		for(uint i = 1, count = type.fields.size(); i < count; ++i)
		{
			auto field = type.fields[i];
			switch(field->type)
			{
			case Type::Field::STRING:
			case Type::Field::MESH:
				{
					string& str = offset_cast<string>(e, field->offset);
					if(!str.empty())
						f << Format("\t%s \"%s\"\n", field->name.c_str(), str.c_str());
				}
				break;
			case Type::Field::FLAGS:
				{
					int flags = offset_cast<int>(e, field->offset);
					if(flags != 0)
					{
						f << '\t';
						f << field->name;
						f << ' ';
						int flags_set = count_bits(flags);
						if(flags_set > 1)
							f << '{';
						bool first = true;
						for(int i = 0; i < 32 && flags != 0; ++i)
						{
							int bit = 1 << i;
							if(IS_SET(flags, bit))
							{
								if(!first)
									f << " ";
								else
									first = false;
								cstring flag_name = field->GetFlag(bit);
								if(flag_name)
									f << flag_name;
								CLEAR_BIT(flags, bit);
							}
						}
						if(flags_set > 1)
							f << "}";
						f << '\n';
					}
				}
				break;
			case Type::Field::REFERENCE:
				{
					TypeItem* ref = offset_cast<TypeItem*>(e, field->offset);
					if(ref != nullptr)
						f << Format("\t%s \"%s\"\n", field->name.c_str(), GetType(field->type_id).GetItemId(ref).c_str());
				}
				break;
			case Type::Field::CUSTOM:
				f << Format("\t%s ", field->name.c_str());
				field->handler->SaveText(f, e, field->offset);
				f << '\n';
				break;
			}
		}

		f << "}\n\n";
	}

	f << '\n';
}
