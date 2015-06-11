#include "Pch.h"
#include "Base.h"
#include "Language.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "Item.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
string g_lang_prefix;
LanguageMap g_language;
LanguageSections g_language2;
LanguageMap* tmp_language_map;
vector<LanguageMap*> g_languages;
extern string g_system_dir;
extern vector<string> name_random, nickname_random, crazy_name;
string tstr, tstr2;

//=================================================================================================
int strchr_index(cstring chrs, char c)
{
	int index = 0;
	do 
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	}
	while(*chrs);

	return -1;
}

//=================================================================================================
void Unescape(const string& sin, string& sout)
{
	sout.clear();
	sout.reserve(sin.length());

	cstring unesc = "nt\\\"";
	cstring esc = "\n\t\\\"";

	for(uint i=0, len=sin.length(); i<len; ++i)
	{
		if(sin[i] == '\\')
		{
			++i;
			if(i == len)
			{
				ERROR(Format("Unescape error in string \"%s\", character '\\' at end of string.", sin.c_str()));
				break;
			}
			int index = strchr_index(unesc, sin[i]);
			if(index != -1)
				sout += esc[index];
			else
			{
				ERROR(Format("Unescape error in string \"%s\", unknown escape sequence '\\%c'.", sin.c_str(), sin[i]));
				break;
			}
		}
		else
			sout += sin[i];
	}
}

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

	LocalString line, id, str;
	int n_line = 1;

	try
	{
		while(true)
		{
			// id
			t.Next();
			if(t.IsEof())
				break;
			if(t.IsKeyword())
				id = t.GetKeyword();
			else
				id = t.MustGetItem();

			// =
			t.Next();
			t.AssertSymbol('=');
			t.Next();

			// text
			Unescape(t.MustGetString(), str.get_ref());

			// sprawdŸ czy ju¿ istnieje, dodaj jeœli nie
			std::pair<LanguageMap::iterator,bool> const& r = g_language.insert(LanguageMap::value_type(id.get_ref(), str.get_ref()));
			if(!r.second)
				WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", id->c_str(), r.first->second.c_str(), str->c_str()));
		}
	}
	catch(cstring err)
	{
		ERROR(Format("LANG: Error while parsing file \"%s\": %s", path, err));
	}
}

//=================================================================================================
bool LoadLanguageFile2(cstring filename, cstring section, LanguageMap* lmap)
{
	assert(filename);		

	Tokenizer t;
	t.FromFile(filename);

	LocalString current_section, id, str;
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
			clmap = NULL;
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
					tmp_language_map = NULL;
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
						clmap = NULL;
						inside = false;
					}
				}
			}
			else
			{
				id = t.MustGetItem();
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				if(inside)
				{
					Unescape(t.MustGetString(), str.get_ref());
					std::pair<LanguageMap::iterator,bool> const& r = clmap->insert(LanguageMap::value_type(id.get_ref(), str.get_ref()));
					if(!r.second)
					{
						if(current_section->empty())
							WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", id->c_str(), r.first->second.c_str(), str->c_str()));
						else
							WARN(Format("LANG: String '[%s]%s' already exists: \"%s\"; new text: \"%s\".", current_section->c_str(), id->c_str(), r.first->second.c_str(), str->c_str()));
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
			tmp_language_map = NULL;
		}
	}
	catch(cstring err)
	{
		if(clmap && !added)
			tmp_language_map->clear();
		ERROR(Format("Failed to load language file '%s': %s", filename, err));
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

	LanguageMap* lmap = NULL;

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
				lmap = NULL;
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
	K_PERK
};

//=================================================================================================
static void PrepareTokenizer(Tokenizer& t)
{
	t.AddKeyword("name", K_NAME);
	t.AddKeyword("desc", K_DESC);
	t.AddKeyword("about", K_ABOUT);
	t.AddKeyword("attribute", K_ATTRIBUTE);
	t.AddKeyword("skill_group", K_SKILL_GROUP);
	t.AddKeyword("skill", K_SKILL);	
	t.AddKeyword("class", K_CLASS);
	t.AddKeyword("nickname", K_NICKNAME);
	t.AddKeyword("crazy", K_CRAZY);
	t.AddKeyword("random", K_RANDOM);
	t.AddKeyword("item", K_ITEM);
	t.AddKeyword("perk", K_PERK);
}

//=================================================================================================
static inline void StartBlock(Tokenizer& t)
{
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	t.AssertSymbol('{');
}

//=================================================================================================
static inline void EndBlock(Tokenizer& t)
{
	t.Next();
	t.AssertSymbol('}');
}

//=================================================================================================
static inline void GetString(Tokenizer& t, KEYWORD k, string& s)
{
	t.Next();
	t.AssertKeyword(k);
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	Unescape(t.MustGetString(), s);
}

//=================================================================================================
static inline void GetStringOrEndBlock(Tokenizer& t, KEYWORD k, string& s)
{
	t.Next();
	if(t.IsSymbol('}'))
		return;
	t.AssertKeyword(k);
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	Unescape(t.MustGetString(), s);
	EndBlock(t);
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
					// attribute id = {
					//		name = "text"
					//		desc = "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						AttributeInfo* ai = AttributeInfo::Find(s);
						if(ai)
						{
							StartBlock(t);
							GetString(t, K_NAME, ai->name);
							GetString(t, K_DESC, ai->desc);
							EndBlock(t);
						}
						else
							t.Throw(Format("Invalid attribute '%s'.", s.c_str()));
					}
					break;
				case K_SKILL_GROUP:
					// skill_group id = "text"
					{
						t.Next();
						const string& s = t.MustGetText();
						SkillGroupInfo* sgi = SkillGroupInfo::Find(s);
						if(sgi)
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							Unescape(t.MustGetString(), sgi->name);
						}
						else
							t.Throw(Format("Invalid skill group '%s'.", s.c_str()));
					}
					break;
				case K_SKILL:
					// skill id = {
					//		name = "text"
					//		desc = "text
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						SkillInfo* si = SkillInfo::Find(s);
						if(si)
						{
							StartBlock(t);
							GetString(t, K_NAME, si->name);
							GetString(t, K_DESC, si->desc);
							EndBlock(t);
						}
						else
							t.Throw(Format("Invalid skill '%s'.", s.c_str()));
					}
					break;
				case K_CLASS:
					// class id = {
					//		name = "text"
					//		desc = "text"
					//		about = "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						ClassInfo* ci = ClassInfo::Find(s);
						if(ci)
						{
							StartBlock(t);
							GetString(t, K_NAME, ci->name);
							GetString(t, K_DESC, ci->desc);
							GetString(t, K_ABOUT, ci->about);
							EndBlock(t);
						}
						else
							t.Throw(Format("Invalid class '%s'.", s.c_str()));
					}
					break;
				case K_NAME:
				case K_NICKNAME:
					// (sur)name type = {
					//		"text"
					//		"text"
					//		...
					// }
					{
						bool nickname = (k == K_NICKNAME);
						vector<string>* names = NULL;
						t.Next();
						if(t.IsKeyword())
						{
							if(t.GetKeywordId() == K_CRAZY)
							{
								if(nickname)
									t.Throw("Crazies can't have nicknames.");
								names = &crazy_name;
							}
							else if(t.GetKeywordId() == K_RANDOM)
							{
								if(nickname)
									names = &nickname_random;
								else
									names = &name_random;
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
						StartBlock(t);
						while(true)
						{
							t.Next();
							if(t.IsSymbol('}'))
								break;
							string& s = Add1(*names);
							Unescape(t.MustGetString(), s);
						}
					}
					break;
				case K_ITEM:
					// item id = {
					//		name = "text"
					//		[desc = "text"]
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						const ItemList* list = NULL;
						Item* item = (Item*)FindItem(s.c_str(), false, &list);
						if(item)
						{
							if(!list)
							{
								StartBlock(t);
								GetString(t, K_NAME, item->name);
								GetStringOrEndBlock(t, K_DESC, item->desc);
							}
							else
								t.Throw(Format("Item '%s' is list.", s.c_str()));
						}
						else if(item)
							t.Throw(Format("Invalid item '%s'.", s.c_str()));
					}
					break;
				case K_PERK:
					// perk id = {
					//		name = "text"
					//		desc = "text"
					// }
					{
						t.Next();
						const string& s = t.MustGetText();
						PerkInfo* ci = PerkInfo::Find(s);
						if(ci)
						{
							StartBlock(t);
							GetString(t, K_NAME, ci->name);
							GetString(t, K_DESC, ci->desc);
							EndBlock(t);
						}
						else
							t.Throw(Format("Invalid perk '%s'.", s.c_str()));
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
				Unescape(t.MustGetString(), tstr2);
				std::pair<LanguageMap::iterator, bool> const& r = g_language.insert(LanguageMap::value_type(tstr, tstr2));
				if(!r.second)
					WARN(Format("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), tstr2.c_str()));
			}
		}
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to load language file '%s': %s", path, err));
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
}
