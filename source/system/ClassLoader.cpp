#include "Pch.h"
#include "ClassLoader.h"

#include "Ability.h"
#include "Class.h"
#include "UnitData.h"

#include <ResourceManager.h>

enum Group
{
	G_CLASS,
	G_KEYWORD,
	G_FLAGS
};

enum Keyword
{
	K_PLAYER,
	K_HERO,
	K_CRAZY,
	K_ICON,
	K_ABILITY,
	K_FLAGS,
	K_LEVEL,
	K_POTIONS
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
		{ "ability", K_ABILITY },
		{ "flags", K_FLAGS },
		{ "level", K_LEVEL },
		{ "potions", K_POTIONS }
		});

	t.AddKeywords(G_FLAGS, {
		{ "mpBar", Class::F_MP_BAR },
		{ "mageItems", Class::F_MAGE_ITEMS },
		{ "fromForest", Class::F_FROM_FOREST },
		{ "autoheal", Class::F_AUTOHEAL }
		});
}

//=================================================================================================
void ClassLoader::LoadEntity(int top, const string& id)
{
	if(Class::TryGet(id))
		t.Throw("Id must be unique.");

	Scoped<Class> clas;
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
			clas->playerId = t.MustGetItem();
			t.Next();
			break;
		case K_HERO:
			clas->heroId = t.MustGetItem();
			t.Next();
			break;
		case K_CRAZY:
			clas->crazyId = t.MustGetItem();
			t.Next();
			break;
		case K_ICON:
			{
				const string& iconFile = t.MustGetString();
				clas->icon = resMgr->TryLoad<Texture>(iconFile);
				if(!clas->icon)
					LoadError("Missing icon '%s'.", iconFile.c_str());
				t.Next();
			}
			break;
		case K_ABILITY:
			{
				const string& id = t.MustGetItem();
				clas->ability = Ability::Get(id);
				if(!clas->ability)
					LoadError("Missing ability '%s'.", id.c_str());
				t.Next();
			}
			break;
		case K_FLAGS:
			t.ParseFlags(G_FLAGS, clas->flags);
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
		case K_POTIONS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				t.AssertKeyword(K_LEVEL, G_KEYWORD);
				t.Next();
				Class::PotionEntry& p = Add1(clas->potions);
				p.level = t.MustGetInt();
				t.Next();
				t.AssertSymbol('{');
				t.Next();
				while(!t.IsSymbol('}'))
				{
					const string& itemid = t.MustGetItem();
					const Item* item = Item::TryGet(itemid);
					if(!item)
						t.Throw("Missing item '%s'.", itemid.c_str());
					t.Next();
					int count = t.MustGetInt();
					t.Next();
					p.items.push_back({ item, count });
				}
				t.Next();
			}
			std::sort(clas->potions.begin(), clas->potions.end(), [](const Class::PotionEntry& pe1, const Class::PotionEntry& pe2)
			{
				return pe1.level > pe2.level;
			});
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
		entry.value = (int)a->attribId;
		return;
	}

	const Skill* s = Skill::Find(item);
	if(s)
	{
		entry.type = Class::LevelEntry::T_SKILL;
		entry.value = (int)s->skillId;
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
		if(!clas->playerId.empty())
		{
			clas->player = UnitData::TryGet(clas->playerId);
			if(!clas->player)
				LoadError("Missing player unit data '%s'.", clas->playerId.c_str());
			else
			{
				if(!clas->player->statProfile)
					LoadError("Player unit is missing profile.");
				else if(clas->player->statProfile->subprofiles.empty())
					LoadError("Player unit is missing subprofiles.");
				else
				{
					for(StatProfile::Subprofile* sub : clas->player->statProfile->subprofiles)
					{
						if(!sub->perks[StatProfile::MAX_PERKS - 1].perk)
							LoadError("Subprofile %s.%s: Missing perks.", clas->player->statProfile->id.c_str(), sub->id.c_str());
						else if(sub->tagSkills[StatProfile::MAX_TAGS - 1] == SkillId::NONE)
							LoadError("Subprofile %s.%s: Missing tag skills.", clas->player->statProfile->id.c_str(), sub->id.c_str());
					}
				}
			}
			if(clas->level.empty())
				LoadError("Missing level entries.");
		}
		if(!clas->heroId.empty())
		{
			clas->hero = UnitData::TryGet(clas->heroId);
			if(!clas->hero)
				LoadError("Missing hero unit data '%s'.", clas->heroId.c_str());
		}
		if(!clas->crazyId.empty())
		{
			clas->crazy = UnitData::TryGet(clas->crazyId);
			if(!clas->crazy)
				LoadError("Missing crazy unit data '%s'.", clas->crazyId.c_str());
		}
	}

	content.errors += Class::InitLists();
}
