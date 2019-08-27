#include "Pch.h"
#include "GameCore.h"
#include "ClassLoader.h"
#include "Class.h"
#include "ResourceManager.h"
#include "Action.h"
#include "UnitData.h"

enum Group
{
	G_CLASS,
	G_KEYWORD
};

enum Keyword
{
	K_PLAYER,
	K_HERO,
	K_CRAZY,
	K_ICON,
	K_ACTION,
	K_MP_BAR,
	K_LEVEL
};

//=================================================================================================
void ClassLoader::DoLoading()
{
	Load("classes.txt", G_CLASS);
}

//=================================================================================================
void ClassLoader::Cleanup()
{
	DeleteElements(Class::classes);
}

//=================================================================================================
void ClassLoader::InitTokenizer()
{
	t.AddKeyword("class", 0, G_CLASS);

	t.AddKeywords(G_KEYWORD, {
		{ "player", K_PLAYER },
		{ "hero", K_HERO },
		{ "crazy", K_CRAZY },
		{ "icon", K_ICON },
		{ "action", K_ACTION },
		{ "mp_bar", K_MP_BAR },
		{ "level", K_LEVEL }
		});
}

//=================================================================================================
void ClassLoader::LoadEntity(int top, const string& id)
{
	if(Class::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<Class> clas;
	clas->id = id;

	t.Next();
	t.AssertSymbol('{');
	t.Next();
	while(!t.IsSymbol('}'))
	{
		Keyword key = t.MustGetKeywordId<Keyword>(G_KEYWORD);
		t.Next();
		switch(key)
		{
		case K_PLAYER:
			clas->player_id = t.MustGetItem();
			t.Next();
			if(t.IsInt())
			{
				clas->player_weight = t.MustGetInt();
				t.Next();
			}
			else
				clas->player_weight = 1;
			break;
		case K_HERO:
			clas->hero_id = t.MustGetItem();
			t.Next();
			if(t.IsInt())
			{
				clas->hero_weight = t.MustGetInt();
				t.Next();
			}
			else
				clas->hero_weight = 1;
			break;
		case K_CRAZY:
			clas->crazy_id = t.MustGetItem();
			t.Next();
			if(t.IsInt())
			{
				clas->crazy_weight = t.MustGetInt();
				t.Next();
			}
			else
				clas->crazy_weight = 1;
			break;
		case K_ICON:
			{
				const string& icon_file = t.MustGetString();
				clas->icon = res_mgr->TryLoad<Texture>(icon_file);
				if(!clas->icon)
					LoadError("Missing icon '%s'.", icon_file.c_str());
				t.Next();
			}
			break;
		case K_ACTION:
			{
				const string& action_id = t.MustGetItem();
				clas->action = Action::Find(action_id);
				if(!clas->action)
					LoadError("Missing action '%s'.", action_id.c_str());
				t.Next();
			}
			break;
		case K_MP_BAR:
			clas->mp_bar = t.MustGetBool();
			t.Next();
			break;
		case K_LEVEL:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				Class::LevelEntry entry;
				ParseLevelEntry(entry);
				t.Next();
				if(t.IsItem("required"))
				{
					entry.required = true;
					t.Next();
				}
				else
					entry.required = (entry.type == Class::LevelEntry::T_ATTRIBUTE);
				entry.priority = t.MustGetFloat();
				if(entry.priority > 0.f)
					clas->level.push_back(entry);
				t.Next();
			}
			t.Next();
			break;
		}
	}

	if(!clas->icon)
		LoadError("Icon not set.");
	Class::classes.push_back(clas.Pin());
}

//=================================================================================================
void ClassLoader::ParseLevelEntry(Class::LevelEntry& entry)
{
	const string& item = t.MustGetItem();
	if(item == "weapon")
	{
		entry.type = Class::LevelEntry::T_SKILL_SPECIAL;
		entry.value = Class::LevelEntry::S_WEAPON;
		return;
	}
	else if(item == "armor")
	{
		entry.type = Class::LevelEntry::T_SKILL_SPECIAL;
		entry.value = Class::LevelEntry::S_ARMOR;
		return;
	}

	Attribute* a = Attribute::Find(item);
	if(a)
	{
		entry.type = Class::LevelEntry::T_ATTRIBUTE;
		entry.value = (int)a->attrib_id;
		return;
	}

	Skill* s = Skill::Find(item);
	if(s)
	{
		entry.type = Class::LevelEntry::T_SKILL;
		entry.value = (int)s->skill_id;
		return;
	}

	t.Unexpected();
}

//=================================================================================================
void ClassLoader::Finalize()
{
	Info("Loaded classes (%u).", Class::classes.size());
}

//=================================================================================================
void ClassLoader::ApplyUnits()
{
	for(Class* clas : Class::classes)
	{
		SetLocalId(clas->id);
		if(!clas->player_id.empty())
		{
			clas->player = UnitData::TryGet(clas->player_id);
			if(!clas->player)
				LoadError("Missing player unit data '%s'.", clas->player_id.c_str());
			else
			{
				if(!clas->player->stat_profile)
					LoadError("Player unit is missing profile.");
				else if(clas->player->stat_profile->subprofiles.empty())
					LoadError("Player unit is missing subprofiles.");
				else
				{
					for(StatProfile::Subprofile* sub : clas->player->stat_profile->subprofiles)
					{
						if(sub->perks[StatProfile::MAX_PERKS - 1].perk == Perk::None)
							LoadError("Subprofile %s.%s: Missing perks.", clas->player->stat_profile->id.c_str(), sub->id.c_str());
						else if(sub->tag_skills[StatProfile::MAX_TAGS - 1] == SkillId::NONE)
							LoadError("Subprofile %s.%s: Missing tag skills.", clas->player->stat_profile->id.c_str(), sub->id.c_str());
					}
				}
			}
			if(clas->level.empty())
				LoadError("Missing level entries.");
		}
		if(!clas->hero_id.empty())
		{
			clas->hero = UnitData::TryGet(clas->hero_id);
			if(!clas->hero)
				LoadError("Missing hero unit data '%s'.", clas->hero_id.c_str());
		}
		if(!clas->crazy_id.empty())
		{
			clas->crazy = UnitData::TryGet(clas->crazy_id);
			if(!clas->crazy)
				LoadError("Missing crazy unit data '%s'.", clas->crazy_id.c_str());
		}
	}

	Class::InitLists();
}
