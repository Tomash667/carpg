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
	if(!statProfile)
		sub.value = 0;
	else
	{
		sub = statProfile->GetRandomSubprofile();
		sub.level = level;
	}
	return GetStats(sub);
}

UnitStats* UnitData::GetStats(SubprofileInfo sub)
{
	assert(group != G_PLAYER);
	typedef std::map<pair<StatProfile*, SubprofileInfo>, UnitStats*> M;
	pair<M::iterator, bool> const& result = UnitStats::sharedStats.insert(M::value_type(std::make_pair(statProfile, sub), nullptr));
	if(result.second)
	{
		UnitStats*& stats = result.first->second;
		stats = new UnitStats;
		stats->fixed = true;
		stats->subprofile = sub;
		if(statProfile)
		{
			stats->Set(*statProfile);
			if(!statProfile->subprofiles.empty())
			{
				stats->priorities = statProfile->subprofiles[sub.index]->priorities;
				stats->tagPriorities = statProfile->subprofiles[sub.index]->tagPriorities;
			}
			else
			{
				stats->priorities = StatProfile::Subprofile::defaultPriorities;
				stats->tagPriorities = StatProfile::Subprofile::defaultTagPriorities;
			}
		}
		else
		{
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				stats->attrib[i] = 0;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				stats->skill[i] = 0;
			stats->priorities = StatProfile::Subprofile::defaultPriorities;
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
	statProfile = ud.statProfile;
	hp = ud.hp;
	stamina = ud.stamina;
	attack = ud.attack;
	def = ud.def;
	dmgType = ud.dmgType;
	flags = ud.flags;
	flags2 = ud.flags2;
	flags3 = ud.flags3 & ~F3_PARENT_DATA;
	abilities = ud.abilities;
	gold = ud.gold;
	gold2 = ud.gold2;
	dialog = ud.dialog;
	idleDialog = ud.idleDialog;
	group = ud.group;
	walkSpeed = ud.walkSpeed;
	runSpeed = ud.runSpeed;
	rotSpeed = ud.rotSpeed;
	width = ud.width;
	attackRange = ud.attackRange;
	blood = ud.blood;
	bloodSize = ud.bloodSize;
	sounds = ud.sounds;
	frames = ud.frames;
	tex = ud.tex;
	idles = ud.idles;
	armorType = ud.armorType;
	itemScript = ud.itemScript;
	clas = ud.clas;
	trader = nullptr; // not copied
	tint = ud.tint;
	appearance = ud.appearance;
	scale = ud.scale;
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
		if(!IsSet(unit->flags2, F2_FIXED_STATS) && !unit->statProfile)
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
