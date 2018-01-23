#include "Pch.h"
#include "Core.h"
#include "Class.h"

vector<Class*> Class::classes;
Class::ClassesInfo Class::info;

Class* Class::TryGet(const AnyString& id)
{
	for(Class* c : classes)
	{
		if(id == c->id)
			return c;
	}

	return nullptr;
}

ClassId Class::GetRandomPlayerClass()
{
	return info.player_classes[Rand() % info.player_classes.size()];
}

ClassId Class::GetRandomHeroClass(bool crazy)
{
	if(crazy)
		return GetRandomWeight(info.hero_classes, info.hero_total);
	else
		return GetRandomWeight(info.crazy_classes, info.crazy_total);
}

UnitData& Class::GetRandomHeroData(bool crazy)
{
	ClassId clas = GetRandomHeroClass(crazy);
	Class* c = classes[(int)clas];
	if(crazy)
		return *c->crazy_data;
	else
		return *c->hero_data;
}

UnitData& Class::GetHeroData(ClassId clas, bool crazy)
{
	int index = (int)clas;
	assert(index >= 0 && index < (int)classes.size());
	Class* c = classes[index];
	if(crazy)
	{
		assert(c->crazy_data);
		return *c->crazy_data;
	}
	else
	{
		assert(c->hero_data);
		return *c->hero_data;
	}
}

ClassId Class::OldToNew(old::ClassId clas)
{
	return info.old_to_new[(int)clas];
}

bool Class::IsPickable(ClassId clas)
{
	int index = (int)clas;
	if(index < 0 || index >= (int)classes.size())
		return false;
	return classes[index]->IsPickable();
}

void Class::Validate(uint& errors)
{
	for(Class* p_clas : classes)
	{
		Class& ci = *p_clas;
		if(ci.name.empty())
		{
			++errors;
			Warn("Test: Class %s: empty name.", ci.id.c_str());
		}
		if(ci.desc.empty())
		{
			++errors;
			Warn("Test: Class %s: empty desc.", ci.id.c_str());
		}
		if(ci.about.empty())
		{
			++errors;
			Warn("Test: Class %s: empty about.", ci.id.c_str());
		}
		if(!ci.icon && (ci.player_data || ci.hero_data || ci.crazy_data))
		{
			++errors;
			Warn("Test: Class %s: missing icon file.", ci.id.c_str());
		}
	}

	if(info.player_classes.empty())
	{
		++errors;
		Error("Test: No player classes.");
	}
	if(info.hero_total == 0)
	{
		++errors;
		Error("Test: No hero classes.");
	}
	if(info.crazy_total == 0)
	{
		++errors;
		Error("Test: No crazy classes.");
	}
}

ClassId Class::ClassesInfo::TryGetIndex(cstring id)
{
	Class* clas = TryGet(id);
	if(clas)
		return (ClassId)clas->index;
	else
		return ClassId::None;
}

void Class::ClassesInfo::Initialize()
{
	old_to_new[(int)old::ClassId::Warrior] = TryGetIndex("warrior");
	old_to_new[(int)old::ClassId::Hunter] = TryGetIndex("hunter");
	old_to_new[(int)old::ClassId::Rogue] = TryGetIndex("rogue");
	old_to_new[(int)old::ClassId::Mage] = TryGetIndex("mage");
	old_to_new[(int)old::ClassId::Cleric] = TryGetIndex("cleric");

	hero_total = 0;
	crazy_total = 0;
	for(Class* c : classes)
	{
		if(c->IsPickable())
			player_classes.push_back((ClassId)c->index);
		if(c->chance > 0)
		{
			if(c->hero_data)
			{
				hero_classes.push_back(std::pair<ClassId, uint>((ClassId)c->index, c->chance));
				hero_total += c->chance;
			}
			if(c->crazy_data)
			{
				crazy_classes.push_back(std::pair<ClassId, uint>((ClassId)c->index, c->chance));
				crazy_total += c->chance;
			}
		}
	}
}
