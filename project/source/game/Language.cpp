#include "Pch.h"
#include "Base.h"
#include "Language.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "Item.h"
#include "Perk.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
string g_lang_prefix;
LanguageMap g_language;
LanguageSections g_language2;
LanguageMap* tmp_language_map;
vector<LanguageMap*> g_languages;
extern string g_system_dir;
extern vector<string> name_random, nickname_random, crazy_name, txLocationStart, txLocationEnd;
string tstr, tstr2;

//=================================================================================================
void LoadLanguageFile(cstring filename)
{
	cstring path = Format("%s/lang/%s/%s", g_system_dir.c_str(), g_lang_prefix.c_str(), filename);

	Tokenizer t;
	if(!t.FromFile(path))
	{
		ERROR(Format("LANG: Failed to open file \"%s\".", path));
		return;
	}
	
	LOG(Format("Reading text file \"%s\".", path));

	LocalString id, str;

	try
	{
		while(true)
		{
			// id
			t.Next();
			if(t.IsEof())
				break;
			if(t.IsKeyword())
				tstr = t.GetKeyword()->name;
			else
				tstr = t.MustGetItem();

			// =
			t.Next();
			t.AssertSymbol('=');
			t.Next();

			// text
			tstr2 = t.MustGetString();

			// sprawdü czy juø istnieje, dodaj jeúli nie
			std::pair<LanguageMap::iterator,bool> const& r = g_language.insert(LanguageMap::value_type(tstr, tstr2));
			if(!r.second)
				WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), tstr2.c_str()));
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("LANG: Error while parsing file \"%s\": %s", path, e.ToString()));
	}
}

//=================================================================================================
bool LoadLanguageFile2(cstring filename, cstring section, LanguageMap* lmap)
{
	assert(filename);		

	Tokenizer t;
	t.FromFile(filename);

	LocalString current_section;
	LanguageMap* clmap;
	bool inside, added;
	
	if(!section)
	{
		LanguageSections::iterator it = g_language2.find(string());
		if(it == g_language2.end())
		{
			if(!tmp_language_map)
				tmp_language_map = new LanguageMap;
			clmap = tmp_language_map;
			added = false;
		}
		else
		{
			clmap = it->second;
			added = true;
		}		
		inside = true;
	}
	else
	{
		assert(lmap);
		if(strlen(section) == 0)
		{
			clmap = lmap;
			inside = true;
		}
		else
		{
			clmap = nullptr;
			inside = false;
		}
		added = true;
	}

	try
	{
		while(t.Next())
		{
			if(t.IsSymbol('['))
			{
				// close previous section
				if(clmap && !added && clmap->size() != 0)
				{
					g_language2[current_section.get_ref()] = clmap;
					tmp_language_map = nullptr;
				}
				// get next section
				t.Next();
				current_section = t.MustGetItem();
				t.Next();
				t.AssertSymbol(']');
				if(!section)
				{
					LanguageSections::iterator it = g_language2.find(current_section.get_ref());
					if(it == g_language2.end())
					{
						if(!tmp_language_map)
							tmp_language_map = new LanguageMap;
						clmap = tmp_language_map;
						added = false;
					}
					else
					{
						clmap = it->second;
						added = true;
					}		
					inside = true;
				}
				else
				{
					if(current_section == section)
					{
						clmap = lmap;
						inside = true;
					}
					else
					{
						clmap = nullptr;
						inside = false;
					}
				}
			}
			else
			{
				tstr = t.MustGetItem();
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				if(inside)
				{
					tstr2 = t.MustGetString();
					std::pair<LanguageMap::iterator,bool> const& r = clmap->insert(LanguageMap::value_type(tstr, tstr2));
					if(!r.second)
					{
						if(current_section->empty())
							WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), tstr2.c_str()));
						else
							WARN(Format("LANG: String '[%s]%s' already exists: \"%s\"; new text: \"%s\".", current_section->c_str(), tstr.c_str(), r.first->second.c_str(), tstr2.c_str()));
					}
				}
				else
					t.AssertString();
			}
		}

		// close previous section
		if(clmap && !added && clmap->size() != 0)
		{
			g_language2[current_section.get_ref()] = clmap;
			tmp_language_map = nullptr;
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		if(clmap && !added)
			tmp_language_map->clear();
		ERROR(Format("Failed to load language file '%s': %s", filename, e.ToString()));
		return false;
	}

	return true;
}

//=================================================================================================
void LoadLanguages()
{
	WIN32_FIND_DATA data;
	HANDLE fhandle = FindFirstFile(Format("%s/lang/*", g_system_dir.c_str()), &data);
	if(fhandle == INVALID_HANDLE_VALUE)
	{
		ERROR(Format("LoadLanguages: FindFirstFile failed (%d).", GetLastError()));
		return;
	}

	LanguageMap* lmap = nullptr;

	do 
	{
		if(strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0 && IS_SET(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			LocalString path = Format("%s/lang/%s/info.txt", g_system_dir.c_str(), data.cFileName);
			if(lmap)
				lmap->clear();
			else
				lmap = new LanguageMap;
			if(LoadLanguageFile2(path, "PickLanguage", lmap))
			{
				LanguageMap::iterator it = lmap->find("dir"), end = lmap->end();
				if(!(it != end && it->second == data.cFileName && lmap->find("englishName") != end && lmap->find("localName") != end && lmap->find("locale") != end))
				{
					WARN(Format("File '%s' is invalid language file.", path->c_str()));
					continue;
				}
				g_languages.push_back(lmap);
				lmap = nullptr;
			}
		}
	}
	while(FindNextFile(fhandle, &data));

	if(lmap)
		delete lmap;
	FindClose(fhandle);
}

//=================================================================================================
void ClearLanguages()
{
	DeleteElements(g_languages);
}

enum KEYWORD
{
	K_NAME,
	K_DESC,
	K_ABOUT,
	K_ATTRIBUTE,
	K_SKILL_GROUP,
	K_SKILL,	
	K_CLASS,
	K_NICKNAME,
	K_CRAZY,
	K_RANDOM,
	K_ITEM,
	K_PERK,
	K_UNIT,
	K_LOCATION_START,
	K_LOCATION_END,
	K_TEXT
};

//=================================================================================================
static void PrepareTokenizer(Tokenizer& t)
{
	t.AddKeywords(0, {
		{ "name", K_NAME },
		{ "desc", K_DESC },
		{ "about", K_ABOUT },
		{ "attribute", K_ATTRIBUTE },
		{ "skill_group", K_SKILL_GROUP },
		{ "skill", K_SKILL },
		{ "class", K_CLASS },
		{ "nickname", K_NICKNAME },
		{ "crazy", K_CRAZY },
		{ "random", K_RANDOM },
		{ "item", K_ITEM },
		{ "perk", K_PERK },
		{ "unit", K_UNIT },
		{ "location_start", K_LOCATION_START },
		{ "location_end", K_LOCATION_END },
		{ "text", K_TEXT }
	});
}

//=================================================================================================
static inline void GetString(Tokenizer& t, KEYWORD k, string& s)
{
	t.AssertKeyword(k);
	t.Next();
	s = t.MustGetString();
	t.Next();
}

//=================================================================================================
static inline void GetStringOrEndBlock(Tokenizer& t, KEYWORD k, string& s)
{
	if(t.IsSymbol('}'))
		return;
	GetString(t, k, s);
	t.AssertSymbol('}');
}

//=================================================================================================
static void LoadLanguageFile3(Tokenizer& t, cstring filename)
{
	cstring path = Format("%s/lang/%s/%s", g_system_dir.c_str(), g_lang_prefix.c_str(), filename);
	LOG(Format("Reading text file \"%s\".", path));

	if(!t.FromFile(path))
	{
		ERROR(Format("Failed to open language file '%s'.", path));
		return;
	}

	LocalString lstr;

	try
	{
		while(t.Next())
		{
			if(t.IsKeyword())
			{
				KEYWORD k = (KEYWORD)t.GetKeywordId();
				switch(k)
				{
				case K_ATTRIBUTE:
					// attribute id {
					//		name "text"
					//		desc "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						AttributeInfo* ai = AttributeInfo::Find(s);
						if(!ai)
							t.Throw(Format("Invalid attribute '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						GetString(t, K_NAME, ai->name);
						GetString(t, K_DESC, ai->desc);
						t.AssertSymbol('}');	
					}
					break;
				case K_SKILL_GROUP:
					// skill_group id = "text"
					{
						t.Next();
						const string& s = t.MustGetText();
						SkillGroupInfo* sgi = SkillGroupInfo::Find(s);
						if(!sgi)
							t.Throw(Format("Invalid skill group '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						sgi->name = t.MustGetString();
					}
					break;
				case K_SKILL:
					// skill id {
					//		name "text"
					//		desc "text
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						SkillInfo* si = SkillInfo::Find(s);
						if(!si)
							t.Throw(Format("Invalid skill '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						GetString(t, K_NAME, si->name);
						GetString(t, K_DESC, si->desc);
						t.AssertSymbol('}');
					}
					break;
				case K_CLASS:
					// class id {
					//		name "text"
					//		desc "text"
					//		about "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						ClassInfo* ci = ClassInfo::Find(s);
						if(!ci)
							t.Throw(Format("Invalid class '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						GetString(t, K_NAME, ci->name);
						GetString(t, K_DESC, ci->desc);
						GetString(t, K_ABOUT, ci->about);
						t.AssertSymbol('}');
					}
					break;
				case K_NAME:
				case K_NICKNAME:
					// (nick)name type {
					//		"text"
					//		"text"
					//		...
					// }
					{
						bool nickname = (k == K_NICKNAME);
						vector<string>* names = nullptr;
						t.Next();
						if(t.IsKeyword())
						{
							int id = t.GetKeywordId();
							if(id == K_CRAZY)
							{
								if(nickname)
									t.Throw("Crazies can't have nicknames.");
								names = &crazy_name;
							}
							else if(id == K_RANDOM)
							{
								if(nickname)
									names = &nickname_random;
								else
									names = &name_random;
							}
							else if(id == K_LOCATION_START)
							{
								if(nickname)
									t.Unexpected();
								names = &txLocationStart;
							}
							else if(id == K_LOCATION_END)
							{
								if(nickname)
									t.Unexpected();
								names = &txLocationEnd;
							}
							else
								t.Unexpected();
						}
						else
						{
							ClassInfo* ci = ClassInfo::Find(t.MustGetItem());
							if(ci)
							{
								if(nickname)
									names = &ci->nicknames;
								else
									names = &ci->names;
							}
							else
								t.Unexpected();
						}
						t.Next();
						t.AssertSymbol('{');
						while(true)
						{
							t.Next();
							if(t.IsSymbol('}'))
								break;
							names->push_back(t.MustGetString());
						}
					}
					break;
				case K_ITEM:
					// item id {
					//		name "text"
					//		[desc "text"]
					//      [text "text" (for books)]
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						ItemListResult lis;
						Item* item = (Item*)FindItem(s.c_str(), false, &lis);
						if(!item)
							t.Throw(Format("Invalid item '%s'.", s.c_str()));
						if(lis.lis)
							t.Throw(Format("Item '%s' is list.", s.c_str()));
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						while(!t.IsSymbol('}'))
						{
							KEYWORD key = (KEYWORD)t.MustGetKeywordId(0);
							if(key != K_NAME && key != K_DESC && key != K_TEXT)
								t.Unexpected();
							t.Next();
							switch(key)
							{
							case K_NAME:
								item->name = t.MustGetString();
								break;
							case K_DESC:
								item->desc = t.MustGetString();
								break;
							case K_TEXT:
								if(item->type != IT_BOOK)
									t.Throw("Item '%s' can't have text element.", item->id.c_str());
								item->ToBook().text = t.MustGetString();
								break;
							}
							t.Next();
						}							
					}
					break;
				case K_PERK:
					// perk id {
					//		name "text"
					//		desc "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						PerkInfo* ci = PerkInfo::Find(s);
						if(!ci)
							t.Throw(Format("Invalid perk '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						GetString(t, K_NAME, ci->name);
						GetString(t, K_DESC, ci->desc);
						t.AssertSymbol('}');	
					}
					break;
				case K_UNIT:
					// unit id = "text"
					{
						t.Next();
						const string& s = t.MustGetText();
						UnitData* ud = FindUnitData(s.c_str(), false);
						if(!ud)
							t.Throw(Format("Invalid unit '%s'.", s.c_str()));
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						ud->name = t.MustGetString();
					}
					break;
				default:
					t.Unexpected();
					break;
				}
			}
			else
			{
				tstr = t.MustGetItem();
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				tstr2 = t.MustGetString();
				std::pair<LanguageMap::iterator, bool> const& r = g_language.insert(LanguageMap::value_type(tstr, tstr2));
				if(!r.second)
					WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), tstr2.c_str()));
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load language file '%s': %s", path, e.ToString()));
	}
}

//=================================================================================================
void LoadLanguageFiles()
{
	Tokenizer t;

	PrepareTokenizer(t);

	LoadLanguageFile3(t, "attribute.txt");
	LoadLanguageFile3(t, "skill.txt");
	LoadLanguageFile3(t, "class.txt");
	LoadLanguageFile3(t, "names.txt");
	LoadLanguageFile3(t, "items.txt");
	LoadLanguageFile3(t, "perks.txt");
	LoadLanguageFile3(t, "units.txt");

	if(txLocationStart.empty() || txLocationEnd.empty())
		throw "Missing locations names.";
}
