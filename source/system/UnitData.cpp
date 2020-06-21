#include "Pch.h"
#include "UnitData.h"

#include "Ability.h"
#include "Item.h"
#include "ItemScript.h"

vector<AbilityList*> AbilityList::lists;
vector<SoundPack*> SoundPack::packs;
vector<IdlePack*> IdlePack::packs;
vector<TexPack*> TexPack::packs;
SetContainer<UnitData> UnitData::units;
std::map<string, UnitData*> UnitData::aliases;
vector<HumanData*> UnitData::appearances;

UnitStats* UnitData::GetStats(int level)
{
	SubprofileInfo sub;
	if(!stat_profile)
		sub.value = 0;
	else
	{
		sub = stat_profile->GetRandomSubprofile();
		sub.level = level;
	}
	return GetStats(sub);
}

UnitStats* UnitData::GetStats(SubprofileInfo sub)
{
	assert(group != G_PLAYER);
	typedef std::map<pair<StatProfile*, SubprofileInfo>, UnitStats*> M;
	pair<M::iterator, bool> const& result = UnitStats::shared_stats.insert(M::value_type(std::make_pair(stat_profile, sub), nullptr));
	if(result.second)
	{
		UnitStats*& stats = result.first->second;
		stats = new UnitStats;
		stats->fixed = true;
		stats->subprofile = sub;
		if(stat_profile)
		{
			stats->Set(*stat_profile);
			if(!stat_profile->subprofiles.empty())
			{
				stats->priorities = stat_profile->subprofiles[sub.index]->priorities;
				stats->tag_priorities = stat_profile->subprofiles[sub.index]->tag_priorities;
			}
			else
			{
				stats->priorities = StatProfile::Subprofile::default_priorities;
				stats->tag_priorities = StatProfile::Subprofile::default_tag_priorities;
			}
		}
		else
		{
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				stats->attrib[i] = 0;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				stats->skill[i] = 0;
			stats->priorities = StatProfile::Subprofile::default_priorities;
		}
		return stats;
	}
	else
		return result.first->second;
}

void UnitData::CopyFrom(UnitData& ud)
{
	parent = &ud;
	mesh = ud.mesh;
	mat = ud.mat;
	level = ud.level;
	stat_profile = ud.stat_profile;
	hp = ud.hp;
	stamina = ud.stamina;
	attack = ud.attack;
	def = ud.def;
	dmg_type = ud.dmg_type;
	flags = ud.flags;
	flags2 = ud.flags2;
	flags3 = ud.flags3 & ~F3_PARENT_DATA;
	abilities = ud.abilities;
	gold = ud.gold;
	gold2 = ud.gold2;
	dialog = ud.dialog;
	idleDialog = ud.idleDialog;
	group = ud.group;
	walk_speed = ud.walk_speed;
	run_speed = ud.run_speed;
	rot_speed = ud.rot_speed;
	width = ud.width;
	attack_range = ud.attack_range;
	blood = ud.blood;
	blood_size = ud.blood_size;
	sounds = ud.sounds;
	frames = ud.frames;
	tex = ud.tex;
	idles = ud.idles;
	armor_type = ud.armor_type;
	item_script = ud.item_script;
	clas = ud.clas;
	trader = nullptr; // not copied
	tint = ud.tint;
	appearance = ud.appearance;
}

int UnitData::GetLevelDif(int level) const
{
	return min(abs(level - this->level.x), abs(level - this->level.y));
}

AbilityList* AbilityList::TryGet(Cstring id)
{
	for(auto list : lists)
	{
		if(list->id == id)
			return list;
	}

	return nullptr;
}

SoundPack* SoundPack::TryGet(Cstring id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

IdlePack* IdlePack::TryGet(Cstring id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

TexPack* TexPack::TryGet(Cstring id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

UnitData* UnitData::TryGet(Cstring id)
{
	static UnitData unit_data_search;
	unit_data_search.id = id;
	auto it = units.find(&unit_data_search);
	if(it != units.end())
		return *it;

	auto it2 = aliases.find(id.s);
	if(it2 != aliases.end())
		return it2->second;

	return nullptr;
}

UnitData* UnitData::Get(Cstring id)
{
	auto unit = TryGet(id);
	if(!unit)
		throw Format("Can't find unit data '%s'!", id.s);
	return unit;
}

void UnitData::Validate(uint& err)
{
	for(auto unit : units)
	{
		if(unit->name.empty() && !IsSet(unit->flags3, F3_PARENT_DATA))
		{
			++err;
			Error("Test: Missing unit '%s' name.", unit->id.c_str());
		}
		if(!IsSet(unit->flags2, F2_FIXED_STATS) && !unit->stat_profile)
		{
			++err;
			Error("Test: Unit '%s' have no fixed stats nor profile.", unit->id.c_str());
		}
	}
}

bool TraderInfo::CanBuySell(const Item* item)
{
	assert(item);

	if(IsSet(types, (1 << item->type)))
	{
		switch(item->type)
		{
		case IT_CONSUMABLE:
			if(IsSet(consumableSubtypes, 1 << (int)item->ToConsumable().subtype))
				return true;
			break;
		case IT_OTHER:
			if(IsSet(otherSubtypes, 1 << (int)item->ToOther().subtype))
				return true;
			break;
		case IT_BOOK:
			if(IsSet(bookSubtypes, 1 << (int)item->ToBook().subtype))
				return true;
			break;
		default:
			return true;
		}
	}

	for(const Item* item2 : includes)
	{
		if(item2 == item)
			return true;
	}

	return false;
}
