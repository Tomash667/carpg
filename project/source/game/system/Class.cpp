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
		return c->crazy_data;
	else
		return c->hero_data;
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
	// TODO
	/*for(int i = 0; i < (int)Class::MAX; ++i)
	{
		ClassInfo& ci = ClassInfo::classes[i];
		if(ci.class_id != (Class)i)
		{
			++err;
			Warn("Test: Class %s: id mismatch.", ci.id);
		}
		if(ci.name.empty())
		{
			++err;
			Warn("Test: Class %s: empty name.", ci.id);
		}
		if(ci.desc.empty())
		{
			++err;
			Warn("Test: Class %s: empty desc.", ci.id);
		}
		if(ci.about.empty())
		{
			++err;
			Warn("Test: Class %s: empty about.", ci.id);
		}
		if(!ci.icon)
		{
			++err;
			Warn("Test: Class %s: missing icon file '%s'.", ci.id, ci.icon_file);
		}
		if(IsPickable(ci.class_id))
		{
			if(!ci.unit_data_id)
			{
				++err;
				Warn("Test: Class %s: missing unit data.", ci.id);
			}
			else if(!ci.unit_data)
			{
				++err;
				Warn("Test: Class %s: invalid unit data '%s'.", ci.id, ci.unit_data_id);
			}
		}
	}*/
}

void Class::ClassesInfo::Initialize()
{
	old_to_new[(int)old::ClassId::Warrior] = (ClassId)TryGet("warrior")->index;
	old_to_new[(int)old::ClassId::Hunter] = (ClassId)TryGet("hunter")->index;
	old_to_new[(int)old::ClassId::Rogue] = (ClassId)TryGet("rogue")->index;
	old_to_new[(int)old::ClassId::Mage] = (ClassId)TryGet("mage")->index;
	old_to_new[(int)old::ClassId::Cleric] = (ClassId)TryGet("cleric")->index;

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
