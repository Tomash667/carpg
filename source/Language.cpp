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
#include "Ability.h"
#include "BaseUsable.h"
#include "UnitGroup.h"

enum Group
{
	G_KEYWORD,
	G_PROPERTY
};

enum Keyword
{
	K_ATTRIBUTE,
	K_SKILL_GROUP,
	K_SKILL,
	K_CLASS,
	K_NAME,
	K_NICKNAME,
	K_CRAZY,
	K_RANDOM,
	K_ITEM,
	K_PERK,
	K_UNIT,
	K_UNIT_GROUP,
	K_LOCATION_START,
	K_LOCATION_END,
	K_BUILDING,
	K_ABILITY,
	K_USABLE
};

enum Property
{
	P_NAME,
	P_NAME2,
	P_NAME3,
	P_REAL_NAME,
	P_DESC,
	P_ABOUT,
	P_TEXT,
	P_ENCOUNTER_TEXT
};

//-----------------------------------------------------------------------------
extern string g_system_dir;
extern vector<string> name_random, nickname_random, crazy_name, txLocationStart, txLocationEnd;
string Language::prefix, Language::tstr;
Language::Map Language::strs;
Language::Sections Language::sections;
vector<Language::Map*> Language::languages;
uint Language::errors;

//=================================================================================================
void Language::Init()
{
	sections[""] = &strs;
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
	Tokenizer t;
	return LoadFileInternal(t, path);
}

//=================================================================================================
// If lmap is passed it is used instead and sections are ignored
bool Language::LoadFileInternal(Tokenizer& t, cstring path, Map* lmap)
{
	assert(path);

	LocalString current_section;
	Map* lm = lmap ? lmap : sections[""];

	try
	{
		if(!t.FromFile(path))
			t.Throw("Failed to open file.");

		while(t.Next())
		{
			if(t.IsKeyword())
				ParseObject(t);
			else if(t.IsSymbol('['))
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
					Sections::iterator it = sections.find(current_section.get_ref());
					if(it == sections.end())
					{
						lm = new Map;
						sections.insert(it, Sections::value_type(current_section, lm));
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
				pair<Map::iterator, bool> const& r = lm->insert(Map::value_type(tstr, text));
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
		++errors;
		return false;
	}

	return true;
}

//=================================================================================================
void Language::LoadLanguages()
{
	Map* lmap = nullptr;
	Tokenizer t;

	io::FindFiles(Format("%s/lang/*", g_system_dir.c_str()), [&](const io::FileInfo& info)
	{
		if(!info.is_dir)
			return true;
		LocalString path = Format("%s/lang/%s/info.txt", g_system_dir.c_str(), info.filename);
		if(lmap)
			lmap->clear();
		else
			lmap = new Map;
		if(LoadFileInternal(t, path, lmap))
		{
			Map::iterator it = lmap->find("dir"), end = lmap->end();
			if(!(it != end && it->second == info.filename && lmap->find("englishName") != end && lmap->find("localName") != end && lmap->find("locale") != end))
			{
				Warn("File '%s' is invalid language file.", path->c_str());
				return true;
			}
			languages.push_back(lmap);
			lmap = nullptr;
		}
		return true;
	});

	if(lmap)
		delete lmap;
}

//=================================================================================================
void Language::PrepareTokenizer(Tokenizer& t)
{
	t.AddKeywords(G_KEYWORD, {
		{ "attribute", K_ATTRIBUTE },
		{ "skill_group", K_SKILL_GROUP },
		{ "skill", K_SKILL },
		{ "class", K_CLASS },
		{ "name", K_NAME },
		{ "nickname", K_NICKNAME },
		{ "crazy", K_CRAZY },
		{ "random", K_RANDOM },
		{ "item", K_ITEM },
		{ "perk", K_PERK },
		{ "unit", K_UNIT },
		{ "unit_group", K_UNIT_GROUP },
		{ "location_start", K_LOCATION_START },
		{ "location_end", K_LOCATION_END },
		{ "building", K_BUILDING },
		{ "ability", K_ABILITY },
		{ "usable", K_USABLE }
		});

	t.AddKeywords(G_PROPERTY, {
		{ "name", P_NAME },
		{ "name2", P_NAME2 },
		{ "name3", P_NAME3 },
		{ "real_name", P_REAL_NAME },
		{ "desc", P_DESC },
		{ "about", P_ABOUT },
		{ "text", P_TEXT },
		{ "encounter_text", P_ENCOUNTER_TEXT }
		});
}

//=================================================================================================
inline void GetString(Tokenizer& t, Property prop, string& s)
{
	t.AssertKeyword(prop, G_PROPERTY);
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
		++errors;
		return;
	}

	try
	{
		LoadFileInternal(t, path);
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load language file '%s': %s", path.c_str(), e.ToString());
		++errors;
	}
}

//=================================================================================================
void Language::ParseObject(Tokenizer& t)
{
	Keyword k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
	t.Next();
	switch(k)
	{
	case K_ATTRIBUTE:
		// attribute id {
		//		name "text"
		//		desc "text"
		// }
		{
			const string& id = t.MustGetText();
			Attribute* ai = Attribute::Find(id);
			if(!ai)
				t.Throw("Invalid attribute '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			GetString(t, P_NAME, ai->name);
			GetString(t, P_DESC, ai->desc);
			t.AssertSymbol('}');
		}
		break;
	case K_SKILL_GROUP:
		// skill_group id = "text"
		{
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
			const string& id = t.MustGetText();
			Skill* si = Skill::Find(id);
			if(!si)
				t.Throw("Invalid skill '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			GetString(t, P_NAME, si->name);
			GetString(t, P_DESC, si->desc);
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
			const string& id = t.MustGetText();
			Class* clas = Class::TryGet(id);
			if(!clas)
				t.Throw("Invalid class '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			GetString(t, P_NAME, clas->name);
			GetString(t, P_DESC, clas->desc);
			GetString(t, P_ABOUT, clas->about);
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
				Class* clas = Class::TryGet(t.MustGetItem());
				if(clas)
				{
					if(nickname)
						names = &clas->nicknames;
					else
						names = &clas->names;
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
			const string& id = t.MustGetText();
			Item* item = Item::TryGet(id);
			if(!item)
				t.Throw("Invalid item '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
				if(prop != P_NAME && prop != P_DESC && prop != P_TEXT)
					t.Unexpected();
				t.Next();
				switch(prop)
				{
				case P_NAME:
					item->name = t.MustGetString();
					break;
				case P_DESC:
					item->desc = t.MustGetString();
					break;
				case P_TEXT:
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
			const string& id = t.MustGetText();
			PerkInfo* perk = PerkInfo::Find(id);
			if(!perk)
				t.Throw("Invalid perk '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			GetString(t, P_NAME, perk->name);
			GetString(t, P_DESC, perk->desc);
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
			const string& id = t.MustGetText();
			UnitData* ud = UnitData::TryGet(id);
			if(!ud)
				t.Throw("Invalid unit '%s'.", id.c_str());
			t.Next();
			if(t.IsSymbol('{'))
			{
				t.Next();
				while(!t.IsSymbol('}'))
				{
					Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
					if(prop != P_NAME && prop != P_REAL_NAME)
						t.Unexpected();
					t.Next();
					switch(prop)
					{
					case P_NAME:
						ud->name = t.MustGetString();
						break;
					case P_REAL_NAME:
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
	case K_UNIT_GROUP:
		// unit_group id {
		//		name "text"
		//		[name2 "text"]
		//		[name3 "text"]
		//		[encounter_text "text"] - required when encounter_chance > 0
		// }
		{
			const string& id = t.MustGetText();
			UnitGroup* group = UnitGroup::TryGet(id);
			if(!group)
				t.Throw("Invalid unit group '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
				if(!Any(prop, P_NAME, P_NAME2, P_NAME3, P_ENCOUNTER_TEXT))
					t.Unexpected();
				t.Next();
				switch(prop)
				{
				case P_NAME:
					group->name = t.MustGetString();
					break;
				case P_NAME2:
					group->name2 = t.MustGetString();
					break;
				case P_NAME3:
					group->name3 = t.MustGetString();
					break;
				case P_ENCOUNTER_TEXT:
					group->encounter_text = t.MustGetString();
					break;
				}
				t.Next();
			}
			if(group->name2.empty())
				group->name2 = group->name;
			if(group->name3.empty())
				group->name3 = group->name;
		}
		break;
	case K_BUILDING:
		// building id = "text"
		{
			const string& id = t.MustGetText();
			Building* b = Building::TryGet(id);
			if(!b)
				t.Throw("Invalid building '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('=');
			t.Next();
			b->name = t.MustGetString();
		}
		break;
	case K_ABILITY:
		// ability id {
		//		name "text"
		//		desc "text"
		// }
		{
			const string& id = t.MustGetText();
			Ability* ability = Ability::Get(id);
			if(!ability)
				t.Throw("Invalid ability '%s'.", id.c_str());
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			GetString(t, P_NAME, ability->name);
			GetString(t, P_DESC, ability->desc);
			t.AssertSymbol('}');
		}
		break;
	case K_USABLE:
		// usable id = "text"
		{
			const string& id = t.MustGetText();
			BaseUsable* use = BaseUsable::TryGet(id);
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

//=================================================================================================
void Language::LoadLanguageFiles()
{
	LoadFile("menu.txt");
	LoadFile("talks.txt");

	Tokenizer t(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS);
	PrepareTokenizer(t);
	LoadObjectFile(t, "names.txt");
	LoadObjectFile(t, "items.txt");
	LoadObjectFile(t, "units.txt");
	LoadObjectFile(t, "buildings.txt");
	LoadObjectFile(t, "objects.txt");
	LoadObjectFile(t, "stats.txt");

	if(txLocationStart.empty() || txLocationEnd.empty())
		throw "Missing locations names.";

	for(Building* building : Building::buildings)
	{
		if(IsSet(building->flags, Building::HAVE_NAME) && building->name.empty())
		{
			Warn("Building '%s': empty name.", building->id.c_str());
			++errors;
		}
	}

	for(BaseUsable* usable : BaseUsable::usables)
	{
		if(usable->name.empty())
		{
			Warn("Usables '%s': empty name.", usable->name.c_str());
			++errors;
		}
	}

	for(Class* clas : Class::classes)
	{
		if(clas->name.empty())
		{
			Warn("Class %s: empty name.", clas->id.c_str());
			++errors;
		}
		if(clas->desc.empty())
		{
			Warn("Class %s: empty desc.", clas->id.c_str());
			++errors;
		}
		if(clas->about.empty())
		{
			Warn("Class %s: empty about.", clas->id.c_str());
			++errors;
		}
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
		{
			Error("Missing text string for '%s'.", str);
			++errors;
		}
		return nullptr;
	}
}

//=================================================================================================
Language::Section Language::GetSection(cstring name)
{
	assert(name);
	auto it = sections.find(name);
	if(it != sections.end())
		return Section(*it->second, name);
	else
	{
		Error("Missing text section '%s'.", name);
		++errors;
		return Section(*sections[""], name);
	}
}

//=================================================================================================
cstring Language::Section::Get(cstring str)
{
	assert(str);
	auto it = lm.get().find(str);
	if(it != lm.get().end())
		return it->second.c_str();
	else
	{
		Error("Missing text string for '%s.%s'.", name, str);
		++errors;
		return "";
	}
}
