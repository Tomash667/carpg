#include "Pch.h"
#include "Base.h"
#include "GameTypeManager.h"
#include "GameTypeProxy.h"

/*
TODO:
+ null reference override
+ check if string for reference is not empty or whitespace
*/

const uint MAX_ERRORS = 100;

enum Group
{
	G_GAMETYPE,
	G_STRING_KEYWORD,
	G_FREE
};

GameTypeManager::GameTypeManager(cstring _system_path, cstring _lang_path) : free_group(G_FREE), errors(0), need_calculate_crc(true)
{
	t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE | Tokenizer::F_FILE_INFO);
	t.AddKeywordGroup("gametype", G_GAMETYPE);
	t.AddKeywords(G_STRING_KEYWORD, { { "alias", 0 } }, "string keyword");
	gametypes.resize(GT_Max);
	system_path = Format("%s/gametypes/", _system_path);
	lang_path = Format("%s/gametypes/", _lang_path);
}

GameTypeManager::~GameTypeManager()
{
	DeleteElements(gametypes);
}

void GameTypeManager::Init()
{
	int index = 0;
	for(GameType* gametype : gametypes)
	{
		assert(gametype);
		assert(gametype->gametype_id == (GameTypeId)index);

		for(GameType::Field* field : gametype->fields)
		{
			if(field->type == GameType::Field::REFERENCE)
			{
				GameType* ref_gametype = gametypes[(int)field->gametype_id];
				assert(ref_gametype);
				field->id_offset = ref_gametype->fields[0]->offset;
			}
		}

		++index;
	}
}

void GameTypeManager::Add(GameType* _gametype, GameTypeHandler* handler)
{
	assert(_gametype);
	assert(handler);

	GameType& gametype = *_gametype;
	gametype.handler = handler;
	gametype.group_name = Format("%s field", gametype.id.c_str());
	assert(!gametype.fields.empty());
	gametypes[(int)gametype.gametype_id] = _gametype;
	t.AddKeyword(gametype.id.c_str(), gametype.gametype_id, G_GAMETYPE);

	// set required fields
	assert(gametype.fields.size() < 32u);
	uint r = 0;
	gametype.alias = -1;
	for(uint i = 0, count = gametype.fields.size(); i < count; ++i)
	{
		GameType::Field& field = *gametype.fields[i];
		if(field.required)
			r |= (1 << i);
		if(!field.name.empty())
			t.AddKeyword(field.name.c_str(), i, free_group);
		if(field.alias)
		{
			assert(gametype.alias == -1);
			gametype.alias = i;
		}
	}
	gametype.required_fields = r;
	t.AddKeywordGroup(gametype.group_name.c_str(), free_group);
	gametype.group = free_group++;

	// set required localized fields
	assert(gametype.localized_fields.size() < 32u);
	r = 0;
	for(uint i = 0, count = gametype.localized_fields.size(); i < count; ++i)
	{
		if(gametype.localized_fields[i]->required)
			r |= (1 << i);
		t.AddKeyword(gametype.localized_fields[i]->name.c_str(), i, free_group);
	}
	gametype.required_localized_fields = r;
	if(!gametype.localized_fields.empty())
	{
		t.AddKeywordGroup(gametype.group_name.c_str(), free_group);
		gametype.localized_group = free_group++;
	}
}

void GameTypeManager::LoadGameTypesFromText(uint& _errors)
{
	for(File& f : system_files)
		LoadGameTypesFromText(f.path.c_str());
	_errors += errors;

	for(GameType* gametype : gametypes)
		gametype->handler->AfterLoad();
}

void GameTypeManager::LoadGameTypesFromText(cstring filename)
{
	INFO(Format("GameTypeManager: Parsing file '%s'.", filename));

	uint file_errors = 0;

	if(!t.FromFile(filename))
	{
		ERROR(Format("GameTypeManager: Failed to open file '%s'.", filename));
		++errors;
		return;
	}

	try
	{
		t.Next();

		LoadGameTypesFromTextImpl(file_errors);

		if(file_errors >= MAX_ERRORS)
		{
			ERROR(Format("GameTypeManager: Errors limit exceeded in file '%s'.", filename));
			++errors;
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("GameTypeManager: Failed to parse gametypes: %s", e.ToString()));
		++errors;
	}
}

void GameTypeManager::LoadGameTypesFromTextImpl(uint& file_errors)
{
	while(!t.IsEof())
	{
		int id = t.MustGetKeywordId(G_GAMETYPE);
		GameType& gametype = GetGameType((GameTypeId)id);
		t.Next();

		if(t.IsSymbol('{'))
		{
			tokenizer::Pos block_start_pos = t.GetPos();
			t.Next();
			bool ok = true;
			while(!t.IsSymbol('}'))
			{
				tokenizer::Pos entry_start_pos = t.GetPos();
				if(!LoadGameTypesFromTextSingle(gametype))
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
			if(!LoadGameTypesFromTextSingle(gametype))
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
						t.SkipToKeywordGroup(G_GAMETYPE);
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
					t.SkipToKeywordGroup(G_GAMETYPE);
				}
			}
		}
	}
}

bool GameTypeManager::LoadGameTypesFromTextSingle(GameType& gametype)
{
	GameTypeProxy proxy(gametype);
	GameTypeHandler& handler = *gametype.handler;

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

		if(gametype.fields[0]->handler)
		{
			// custom handler for gametype
			gametype.fields[0]->handler->LoadText(t, proxy.item);
			handler.Insert(proxy.item);
			gametype.loaded++;
			proxy.item = nullptr;
			return true;
		}

		uint set_fields = 1u;

		while(!t.IsSymbol('}'))
		{
			int keyword = t.MustGetKeywordId(gametype.group);
			GameType::Field& field = *gametype.fields[keyword];
			t.Next();

			if(IS_SET(set_fields, 1 << keyword))
				t.Throw("Field '%s' already used.", field.name.c_str());

			switch(field.type)
			{
			case GameType::Field::STRING:
			case GameType::Field::MESH:
				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();
				break;
			case GameType::Field::FLAGS:
				proxy.Get<int>(field.offset) = ReadFlags(t, field.keyword_group);
				t.Next();
				break;
			case GameType::Field::REFERENCE:
				{
					const string& ref_id = t.MustGetString();
					GameTypeItem found = GetGameType(field.gametype_id).handler->Find(ref_id, false);
					if(!found)
						t.Throw("Missing gametype %s '%s' for field '%s'.", GetGameType(field.gametype_id).id.c_str(), ref_id.c_str(), field.name.c_str());
					proxy.Get<void*>(field.offset) = found;
					t.Next();
					if(field.callback >= 0)
						gametype.handler->Callback(proxy.item, found, field.callback);
				}
				break;
			case GameType::Field::CUSTOM:
				field.handler->LoadText(t, proxy.item);
				break;
			}

			set_fields |= (1 << keyword);
		}
		t.Next();

		if(!IS_ALL_SET(set_fields, gametype.required_fields))
		{
			LocalString s = "Missing required fields: ";
			bool first = true;
			for(uint i = 1, count = gametype.fields.size(); i<count; ++i)
			{
				if(IS_SET(gametype.required_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
				{
					if(first)
						first = false;
					else
						s += ", ";
					s += gametype.fields[i]->name;
				}
			}
			s += ".";
			t.Throw(s.c_str());
		}

		handler.Insert(proxy.item);
		gametype.loaded++;
		proxy.item = nullptr;
		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("GameTypeManager: Failed to parse '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void GameTypeManager::LoadStringsFromText()
{
	for(File& f : lang_files)
		LoadStringsFromText(f.path.c_str());

	VerifyStrings();
}

void GameTypeManager::LoadStringsFromText(cstring filename)
{
	assert(filename);

	try
	{
		t.FromFile(filename);
		t.Next();

		while(!t.IsEof())
		{
			int id = t.MustGetKeywordId(G_GAMETYPE);
			GameType& gametype = GetGameType((GameTypeId)id);

			bool ok;
			if(gametype.localized_fields.empty())
			{
				ERROR(Format("GameTypeManager: Type '%s' don't have any localized fields.", gametype.id.c_str()));
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
						ok = LoadStringsFromTextSingle(gametype);
						if(!ok)
							break;
					}
					if(ok)
						t.Next();
				}
				else
					ok = LoadStringsFromTextSingle(gametype);
			}

			if(!ok)
				t.SkipToKeywordGroup(G_GAMETYPE);
		}
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("GameTypeManager: Failed to parse gametypes: %s", e.ToString()));
		++errors;
	}
}

bool GameTypeManager::LoadStringsFromTextSingle(GameType& gametype)
{
	GameTypeProxy proxy(gametype, false, false);

	try
	{
		bool alias = false;

		if(t.IsString())
		{
			const string& id = t.MustGetString();
			proxy.item = gametype.handler->Find(id, false);
			if(!proxy.item)
			{
				ERROR(Format("GameTypeManager: Missing '%s %s'.", gametype.id.c_str(), id.c_str()));
				++errors;
				return false;
			}
			t.Next();
		}
		else if(t.IsKeyword(0, G_STRING_KEYWORD))
		{
			if(gametype.alias == -1)
				t.Throw("GameType '%s' don't have alias.", gametype.id.c_str());
			t.Next();
			const string& id = t.MustGetString();
			GameTypeItem item = GetByAlias(gametype, &id);
			if(!item)
			{
				WARN(Format("GameTypeManager: Unused '%s' alias '%s'.", gametype.id.c_str(), id.c_str()));
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
			if(gametype.localized_fields.size() != 1u)
				t.Throw("Can't use simple string assigment when gametype have multiple fields.");
			t.Next();
			proxy.Get<string>(gametype.localized_fields[0]->offset) = t.MustGetString();
			t.Next();
			set_fields = 1u;
		}
		else
		{
			t.AssertSymbol('{');
			t.Next();
			
			while(!t.IsSymbol('}'))
			{
				int keyword = t.MustGetKeywordId(gametype.localized_group);
				GameType::LocalizedField& field = *gametype.localized_fields[keyword];
				t.Next();

				if(IS_SET(set_fields, 1 << keyword))
					t.Throw("Field '%s' already used.", field.name.c_str());

				proxy.Get<string>(field.offset) = t.MustGetString();
				t.Next();

				set_fields |= (1 << keyword);
			}
			t.Next();

			if(!IS_ALL_SET(set_fields, gametype.required_localized_fields))
			{
				LocalString s = "Missing required fields: ";
				bool first = true;
				for(uint i = 1, count = gametype.localized_fields.size(); i<count; ++i)
				{
					if(IS_SET(gametype.required_localized_fields, 1 << i) && !IS_SET(set_fields, 1 << i))
					{
						if(first)
							first = false;
						else
							s += ", ";
						s += gametype.localized_fields[i]->name;
					}
				}
				s += ".";
				t.Throw(s.c_str());
			}
		}

		if(alias)
		{
			GameTypeItem parent = proxy.item;
			GameTypeItem child;
			while((child = GetByAlias(gametype)) != nullptr)
			{
				for(uint i = 0, count = gametype.localized_fields.size(); i < count; ++i)
				{
					if(IS_SET(set_fields, 1 << i))
					{
						uint offset = gametype.localized_fields[i]->offset;
						offset_cast<string>(child, offset) = offset_cast<string>(parent, offset);
					}
				}
			}
		}

		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("GameTypeManager: Failed to parse localized '%s': %s", proxy.GetName(), e.ToString()));
		++errors;
		return false;
	}
}

void GameTypeManager::VerifyStrings()
{
	for(GameType* dt_ptr : gametypes)
	{
		GameType& gametype = *dt_ptr;
		if(gametype.required_localized_fields != 0u)
		{
			uint id_offset = gametype.GetIdOffset();
			GameTypeItem item = gametype.handler->GetFirstItem();
			while(item)
			{
				for(GameType::LocalizedField* field : gametype.localized_fields)
				{
					if(field->required)
					{
						if(offset_cast<string>(item, field->offset).empty())
						{
							++errors;
							WARN(Format("GameTypeManager: Missing localized string '%s' for '%s %s'.", field->name.c_str(), gametype.id.c_str(),
								offset_cast<string>(item, id_offset).c_str()));
						}
					}
				}
				item = gametype.handler->GetNextItem();
			}
		}
	}
}

GameTypeItem GameTypeManager::GetByAlias(GameType& gametype, const string* alias)
{
	GameTypeItem item = nullptr;
	if(alias)
	{
		last_alias_offset = gametype.fields[gametype.alias]->offset;
		last_alias = *alias;
		item = gametype.handler->GetFirstItem();
	}
	else
		item = gametype.handler->GetNextItem();

	while(item && offset_cast<string>(item, last_alias_offset) != last_alias)
		item = gametype.handler->GetNextItem();

	return item;
}

int GameTypeManager::AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name)
{
	t.AddKeywords(free_group, keywords, group_name);
	return free_group++;
}

void GameTypeManager::CalculateCrc()
{
	if(!need_calculate_crc)
		return;
	for(GameType* gametype : gametypes)
	{
		gametype->CalculateCrc();
		INFO(Format("GameTypeManager: Crc for %s (%p).", gametype->id.c_str(), gametype->crc));
	}
	need_calculate_crc = false;
}

void GameTypeManager::WriteCrc(BitStream& stream)
{
	for(GameType* gametype : gametypes)
		stream.Write(gametype->crc);
}

bool GameTypeManager::ReadCrc(BitStream& stream)
{
	bool ok = true;
	for(GameType* gametype : gametypes)
		ok = stream.Read(gametype->other_crc) && ok;
	return ok;
}

bool GameTypeManager::GetCrc(GameTypeId gametype_id, uint& my_crc, cstring& name)
{
	if((int)gametype_id < 0 || gametype_id >= GT_Max)
		return false;
	GameType* gametype = gametypes[(int)gametype_id];
	my_crc = gametype->crc;
	name = gametype->id.c_str();
	return true;
}

bool GameTypeManager::ValidateCrc(GameTypeId& gametype_id, uint& my_crc, uint& other_crc, cstring& name)
{
	for(GameType* gametype : gametypes)
	{
		if(gametype->crc != gametype->other_crc)
		{
			gametype_id = gametype->gametype_id;
			my_crc = gametype->crc;
			other_crc = gametype->other_crc;
			name = gametype->id.c_str();
			return false;
		}
	}
	return true;
}

bool GameTypeManager::LoadFilelist()
{
	Tokenizer t;
	bool ok = LoadGameTypeFilelist(t);
	return LoadStringsFilelist(t) && ok;
}

bool GameTypeManager::LoadGameTypeFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", system_path.c_str());
	if(!core::io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(system_files);
		file.path = Format("%s%s", system_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		ERROR("GameTypeManager: Failed to load gametype filelist.");
		return false;
	}
	else
		return true;
}

bool GameTypeManager::LoadStringsFilelist(Tokenizer& t)
{
	cstring pattern = Format("%s*.txt", lang_path.c_str());
	if(!core::io::FindFiles(pattern, [this](const WIN32_FIND_DATA& find_data) {
		File& file = Add1(lang_files);
		file.path = Format("%s%s", lang_path.c_str(), find_data.cFileName);
		file.size = find_data.nFileSizeLow;
		return true;
	}))
	{
		ERROR("GameTypeManager: Failed to load language gametype filelist.");
		return false;
	}
	else
		return true;
}

void GameTypeManager::LogLoadedGameTypes()
{
	for(GameType* gametype : gametypes)
		INFO(Format("GameTypeManager: Loaded %s(s): %u.", gametype->id.c_str(), gametype->loaded));
}
