#include "Pch.h"
#include "Core.h"
#include "UnitData.h"
#include "ItemScript.h"
#include "Dialog.h"
#include "Spell.h"
#include "Item.h"

//-----------------------------------------------------------------------------
vector<SpellList*> SpellList::lists;
vector<SoundPack*> SoundPack::packs;
vector<IdlePack*> IdlePack::packs;
vector<TexPack*> TexPack::packs;
SetContainer<UnitData> UnitData::units;
std::map<string, UnitData*> UnitData::aliases;

//=================================================================================================
void UnitData::CopyFrom(UnitData& ud)
{
	mesh_id = ud.mesh_id;
	mat = ud.mat;
	level = ud.level;
	stat_profile = ud.stat_profile;
	hp = ud.hp;
	hp_bonus = ud.hp_bonus;
	atk = ud.atk;
	atk_bonus = ud.atk_bonus;
	def = ud.def;
	def_bonus = ud.def_bonus;
	stamina = ud.stamina;
	block = ud.block;
	block_bonus = ud.block_bonus;
	dmg_type = ud.dmg_type;
	flags = ud.flags;
	flags2 = ud.flags2;
	flags3 = ud.flags3 & ~F3_PARENT_DATA;
	spells = ud.spells;
	gold = ud.gold;
	gold2 = ud.gold2;
	dialog = ud.dialog;
	group = ud.group;
	walk_speed = ud.walk_speed;
	run_speed = ud.run_speed;
	rot_speed = ud.rot_speed;
	width = ud.width;
	attack_range = ud.attack_range;
	blood = ud.blood;
	sounds = ud.sounds;
	frames = ud.frames;
	tex = ud.tex;
	idles = ud.idles;
	armor_type = ud.armor_type;
	item_script = ud.item_script;
	clas = ud.clas;
}

//=================================================================================================
SpellList* SpellList::TryGet(const AnyString& id)
{
	for(auto list : lists)
	{
		if(list->id == id)
			return list;
	}

	return nullptr;
}

//=================================================================================================
SoundPack* SoundPack::TryGet(const AnyString& id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

//=================================================================================================
IdlePack* IdlePack::TryGet(const AnyString& id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

//=================================================================================================
TexPack* TexPack::TryGet(const AnyString& id)
{
	for(auto pack : packs)
	{
		if(pack->id == id)
			return pack;
	}

	return nullptr;
}

//=================================================================================================
UnitData* UnitData::TryGet(const AnyString& id)
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

//=================================================================================================
UnitData* UnitData::Get(const AnyString& id)
{
	auto unit = TryGet(id);
	if(!unit)
		throw Format("Can't find unit data '%s'!", id.s);
	return unit;
}

//=================================================================================================
void UnitData::Validate(uint& err)
{
	for(auto unit : units)
	{
		if(unit->name.empty() && !IS_SET(unit->flags3, F3_PARENT_DATA))
		{
			++err;
			Error("Test: Missing unit '%s' name.", unit->id.c_str());
		}
	}
}
