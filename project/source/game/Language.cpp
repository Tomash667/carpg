#include "Pch.h"
#include "GameCore.h"
#include "Tokenizer.h"
#include "Language.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "Item.h"
#include "Perk.h"
#include "UnitData.h"
#include "Building.h"
#include "Action.h"
#include "BaseUsable.h"

//-----------------------------------------------------------------------------
extern string g_system_dir;
extern vector<string> name_random, nickname_random, crazy_name, txLocationStart, txLocationEnd;
string Language::prefix, Language::tstr;
Language::LanguageMap Language::strs;
Language::LanguageSections Language::sections;
vector<Language::LanguageMap*> Language::languages;

//=================================================================================================
void Language::Init()
{
	sections[""] = &strs;
	sections["(placeholder)"] = new LanguageMap;
}

//=================================================================================================
void Language::Cleanup()
{
	for(auto& section : sections)
	{
		if(section.second != &strs)
			delete section.second;
	}
	sections.clear();
	DeleteElements(languages);
}

//=================================================================================================
bool Language::LoadFile(cstring filename)
{
	assert(filename);
	LocalString path = Format("%s/lang/%s/%s", g_system_dir.c_str(), prefix.c_str(), filename);
	return LoadFileInternal(path);
}

//=================================================================================================
// If lmap is passed it is used instead and sections are ignored
bool Language::LoadFileInternal(cstring path, LanguageMap* lmap)
{
	assert(path);

	Tokenizer t;
	LocalString current_section;
	LanguageMap* lm = lmap ? lmap : sections[""];

	try
	{
		if(!t.FromFile(path))
			t.Throw("Failed to open file.");

		while(t.Next())
		{
			if(t.IsSymbol('['))
			{
				// open section
				t.Next();
				if(t.IsSymbol(']'))
					current_section->clear();
				else
				{
					current_section = t.MustGetItem();
					t.Next();
					t.AssertSymbol(']');
				}

				if(!lmap)
				{
					LanguageSections::iterator it = sections.find(current_section.get_ref());
					if(it == sections.end())
					{
						lm = new LanguageMap;
						sections.insert(it, LanguageSections::value_type(current_section, lm));
					}
					else
						lm = it->second;
				}
			}
			else
			{
				// add new string
				tstr = t.MustGetItem();
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				const string& text = t.MustGetString();
				pair<LanguageMap::iterator, bool> const& r = lm->insert(LanguageMap::value_type(tstr, text));
				if(!r.second)
				{
					if(current_section->empty())
						Warn("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), text.c_str());
					else
					{
						Warn("LANG: String '%s.%s' already exists: \"%s\"; new text: \"%s\".", current_section->c_str(), tstr.c_str(),
							r.first->second.c_str(), text.c_str());
					}
				}
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load language file '%s': %s", path, e.ToString());
		return false;
	}

	return true;
}

//=================================================================================================
void Language::LoadLanguages()
{
	WIN32_FIND_DATA data;
	HANDLE fhandle = FindFirstFile(Format("%s/lang/*", g_system_dir.c_str()), &data);
	if(fhandle == INVALID_HANDLE_VALUE)
	{
		Error("LoadLanguages: FindFirstFile failed (%d).", GetLastError());
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
			if(LoadFileInternal(path, lmap))
			{
				LanguageMap::iterator it = lmap->find("dir"), end = lmap->end();
				if(!(it != end && it->second == data.cFileName && lmap->find("englishName") != end && lmap->find("localName") != end && lmap->find("locale") != end))
				{
					Warn("File '%s' is invalid language file.", path->c_str());
					continue;
				}
				languages.push_back(lmap);
				lmap = nullptr;
			}
		}
	}
	while(FindNextFile(fhandle, &data));

	if(lmap)
		delete lmap;
	FindClose(fhandle);
}

enum KEYWORD
{
	K_NAME,
	K_REAL_NAME,
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
	K_TEXT,
	K_BUILDING,
	K_ACTION,
	K_USABLE
};

//=================================================================================================
void Language::PrepareTokenizer(Tokenizer& t)
{
	t.AddKeywords(0, {
		{ "name", K_NAME },
		{ "real_name", K_REAL_NAME },
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
		{ "text", K_TEXT },
		{ "building", K_BUILDING },
		{ "action", K_ACTION },
		{ "usable", K_USABLE }
		});
}

//=================================================================================================
inline void GetString(Tokenizer& t, KEYWORD k, string& s)
{
	t.AssertKeyword(k);
	t.Next();
	s = t.MustGetString();
	t.Next();
}

//=================================================================================================
void Language::LoadObjectFile(Tokenizer& t, cstring filename)
{
	LocalString path = Format("%s/lang/%s/%s", g_system_dir.c_str(), prefix.c_str(), filename);
	Info("Reading text file \"%s\".", path.c_str());

	if(!t.FromFile(path))
	{
		Error("Failed to open language file '%s'.", path.c_str());
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
						const string& id = t.MustGetText();
						Attribute* ai = Attribute::Find(id);
						if(!ai)
							t.Throw("Invalid attribute '%s'.", id.c_str());
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
						const string& id = t.MustGetText();
						SkillGroup* sgi = SkillGroup::Find(id);
						if(!sgi)
							t.Throw("Invalid skill group '%s'.", id.c_str());
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
						const string& id = t.MustGetText();
						Skill* si = Skill::Find(id);
						if(!si)
							t.Throw("Invalid skill '%s'.", id.c_str());
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
						const string& id = t.MustGetText();
						ClassInfo* ci = ClassInfo::Find(id);
						if(!ci)
							t.Throw("Invalid class '%s'.", id.c_str());
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
						const string& id = t.MustGetText();
						Item* item = Item::TryGet(id);
						if(!item)
							t.Throw("Invalid item '%s'.", id.c_str());
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
						const string& id = t.MustGetText();
						PerkInfo* ci = PerkInfo::Find(id);
						if(!ci)
							t.Throw("Invalid perk '%s'.", id.c_str());
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
					// or
					// unit id {
					//      [name "text"]
					//      [real_name "text"]
					// }
					{
						t.Next();
						const string& id = t.MustGetText();
						UnitData* ud = UnitData::TryGet(id.c_str());
						if(!ud)
							t.Throw("Invalid unit '%s'.", id.c_str());
						t.Next();
						if(t.IsSymbol('{'))
						{
							t.Next();
							while(!t.IsSymbol('}'))
							{
								KEYWORD key = (KEYWORD)t.MustGetKeywordId(0);
								if(key != K_NAME && key != K_REAL_NAME)
									t.Unexpected();
								t.Next();
								switch(key)
								{
								case K_NAME:
									ud->name = t.MustGetString();
									break;
								case K_REAL_NAME:
									ud->real_name = t.MustGetString();
									break;
								}
								t.Next();
							}
						}
						else if(t.IsSymbol('='))
						{
							t.Next();
							ud->name = t.MustGetString();
						}
						else
							t.ThrowExpecting("symbol { or =");
						if(ud->parent)
						{
							if(ud->name.empty())
								ud->name = ud->parent->name;
							if(ud->real_name.empty())
								ud->real_name = ud->parent->real_name;
						}
						if(ud->name.empty())
							Warn("Missing unit '%s' name.", ud->id.c_str());
					}
					break;
				case K_BUILDING:
					// building id = "text"
					{
						t.Next();
						const string& id = t.MustGetText();
						Building* b = Building::TryGet(id.c_str());
						if(!b)
							t.Throw("Invalid building '%s'.", id.c_str());
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						b->name = t.MustGetString();
					}
					break;
				case K_ACTION:
					// action id {
					//		name "text"
					//		desc "text"
					// }
					{
						t.Next();
						const string& id = t.MustGetText();
						Action* action = Action::Find(id);
						if(!action)
							t.Throw("Invalid action '%s'.", id.c_str());
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						GetString(t, K_NAME, action->name);
						GetString(t, K_DESC, action->desc);
						t.AssertSymbol('}');
					}
					break;
				case K_USABLE:
					// usable id = "text"
					{
						t.Next();
						const string& id = t.MustGetText();
						BaseUsable* use = BaseUsable::TryGet(id.c_str());
						if(!use)
							t.Throw("Invalid usable '%s'.", id.c_str());
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						use->name = t.MustGetString();
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
				const string& text = t.MustGetString();
				pair<LanguageMap::iterator, bool> const& r = strs.insert(LanguageMap::value_type(tstr, text));
				if(!r.second)
					Warn("LANG: String '%s' already exists: \"%s\"; new text: \"%s\".", tstr.c_str(), r.first->second.c_str(), text.c_str());
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load language file '%s': %s", path.c_str(), e.ToString());
	}
}

//=================================================================================================
void Language::LoadLanguageFiles()
{
	Tokenizer t;

	PrepareTokenizer(t);

	LoadObjectFile(t, "actions.txt");
	LoadObjectFile(t, "attribute.txt");
	LoadObjectFile(t, "skill.txt");
	LoadObjectFile(t, "class.txt");
	LoadObjectFile(t, "names.txt");
	LoadObjectFile(t, "items.txt");
	LoadObjectFile(t, "perks.txt");
	LoadObjectFile(t, "units.txt");
	LoadObjectFile(t, "buildings.txt");
	LoadObjectFile(t, "objects.txt");

	if(txLocationStart.empty() || txLocationEnd.empty())
		throw "Missing locations names.";

	for(Building* building : Building::buildings)
	{
		if(IS_SET(building->flags, Building::HAVE_NAME) && building->name.empty())
			Warn("Building '%s' don't have name.", building->id.c_str());
	}

	for(BaseUsable* usable : BaseUsable::usables)
	{
		if(usable->name.empty())
			Warn("Usables '%s' don't have name.", usable->name.c_str());
	}
}

//=================================================================================================
cstring Language::TryGetString(cstring str, bool err)
{
	assert(str);
	auto it = strs.find(str);
	if(it != strs.end())
		return it->second.c_str();
	else
	{
		if(err)
			Error("Missing text string for '%s'.", str);
		return nullptr;
	}
}

//=================================================================================================
Language::LanguageSection Language::GetSection(cstring name)
{
	assert(name);
	auto it = sections.find(name);
	if(it != sections.end())
		return LanguageSection(*it->second, name);
	else
		return LanguageSection(*sections["(default)"], name);
}

//=================================================================================================
cstring Language::LanguageSection::Get(cstring str)
{
	assert(str);
	auto it = lm.find(str);
	if(it != lm.end())
		return it->second.c_str();
	else
	{
		Error("Missing text string for '%s.%s'.", name, str);
		return "";
	}
}
