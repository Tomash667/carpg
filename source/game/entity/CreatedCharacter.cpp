#include "Pch.h"
#include "Base.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"

//=================================================================================================
void CreatedCharacter::Clear(Class c)
{
	ClassInfo& info = g_classes[(int)c];

	sp_max = 3;
	perks_max = 2;

	sp = sp_max;
	perks = perks_max;

	StatProfile& profile = info.unit_data->GetStatProfile();

	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		a[i].value = profile.attrib[i];
		a[i].mod = false;
	}

	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		s[i].value = profile.skill[i];
		s[i].add = false;
		s[i].mod = false;
	}

	taken_perks.clear();
	update_skills = false;
	to_update.clear();
}

//=================================================================================================
void CreatedCharacter::Write(BitStream& stream) const
{
	// picked skills
	int sk = 0;
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(s[i].add)
			sk |= (1 << i);
	}
	stream.Write(sk);

	// perks
	stream.WriteCasted<byte>(taken_perks.size());
	for(const TakenPerk& tp : taken_perks)
	{
		stream.WriteCasted<byte>(tp.perk);
		stream.Write(tp.value);
	}
}

//=================================================================================================
int CreatedCharacter::Read(BitStream& stream)
{
	// picked skills
	int sk;
	byte count;

	if(!stream.Read(sk) || !stream.Read(count))
		return 1;

	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(IS_SET(sk, 1 << i))
		{
			s[i].Add(5, true);
			--sp;
		}
	}

	// perks
	taken_perks.resize(count);
	for(TakenPerk& tp : taken_perks)
	{
		if(!stream.ReadCasted<byte>(tp.perk) || !stream.Read(tp.value))
			return 1;
		if(tp.perk >= Perk::Max)
		{
			ERROR(Format("Invalid perk id '%d'.", tp.perk));
			return 2;
		}
		tp.Apply(*this, true);
	}

	// search for duplicates
	for(vector<TakenPerk>::iterator it = taken_perks.begin(), end = taken_perks.end(); it != end; ++it)
	{
		const PerkInfo& info = g_perks[(int)it->perk];
		if(!IS_SET(info.flags, PerkInfo::Multiple))
		{
			for(vector<TakenPerk>::iterator it2 = it + 1; it2 != end; ++it2)
			{
				if(it->perk == it2->perk)
				{
					ERROR(Format("Multiple same perks '%s'.", info.id));
					return 3;
				}
			}
		}
	}

	// check for too many sp/perks
	if(sp < 0 || perks < 0)
	{
		ERROR(Format("Too many skill points (%d) / perks (%d).", -min(sp, 0), -min(perks, 0)));
		return 3;
	}

	return 0;
}

//=================================================================================================
void CreatedCharacter::Apply(PlayerController& pc)
{
	pc.unit->data->GetStatProfile().Set(0, pc.base_stats);

	for(int i = 0; i<(int)Attribute::MAX; ++i)
		pc.unit->unmod_stats.attrib[i] = pc.base_stats.attrib[i];
	for(int i = 0; i<(int)Skill::MAX; ++i)
		pc.unit->unmod_stats.skill[i] = pc.base_stats.skill[i];

	// apply skills
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		if(s[i].add)
			pc.base_stats.skill[i] += 5;
	}

	// apply perks
	pc.perks = std::move(taken_perks);
	for(TakenPerk& tp : pc.perks)
		tp.Apply(pc);
	
	pc.unit->CalculateStats();
	pc.unit->CalculateLoad();
	pc.unit->hp = pc.unit->hpmax = pc.unit->CalculateMaxHp();
	pc.unit->level = pc.unit->CalculateLevel();

	pc.SetRequiredPoints();

	// inventory
	Game::Get().ParseItemScript(*pc.unit, pc.unit->data->items);
	cstring items[4];
	GetStartingItems(items);
	for(int i = 0; i < 4; ++i)
	{
		if(items[i])
			pc.unit->slots[i] = FindItem(items[i]);
	}
	if(HavePerk(Perk::AlchemistApprentice))
	{
		pc.unit->AddItem(FindItem("potion_smallheal"), 15, false);
		pc.unit->AddItem(FindItem("potion_mediumheal"), 3, false);
		pc.unit->AddItem(FindItem("potion_smallnreg"), 5, false);
	}
	pc.unit->MakeItemsTeam(false);
	pc.unit->RecalculateWeight();
}

//=================================================================================================
bool CreatedCharacter::HavePerk(Perk perk) const
{
	for(const TakenPerk& tp : taken_perks)
	{
		if(tp.perk == perk)
			return true;
	}

	return false;
}

//=================================================================================================
void CreatedCharacter::GetStartingItems(cstring (&items)[4])
{
	items[SLOT_WEAPON] = NULL;
	items[SLOT_BOW] = NULL;
	items[SLOT_SHIELD] = NULL;
	items[SLOT_ARMOR] = NULL;

	if(HavePerk(Perk::FamilyHeirloom))
	{
		// find best skill for family heirloom
		const Skill to_check[] = {
			Skill::SHORT_BLADE,
			Skill::LONG_BLADE,
			Skill::AXE,
			Skill::BLUNT,
			Skill::BOW,
			Skill::SHIELD,
			Skill::LIGHT_ARMOR,
			Skill::MEDIUM_ARMOR,
			Skill::HEAVY_ARMOR
		};

		Skill best = Skill::NONE;
		int val = 0;

		for(Skill sk : to_check)
		{
			if(s[(int)sk].value > val)
			{
				best = sk;
				val = s[(int)sk].value;
			}
		}

		switch(best)
		{
		case Skill::SHORT_BLADE:
			items[SLOT_WEAPON] = "dagger_assassin";
			break;
		case Skill::LONG_BLADE:
			items[SLOT_WEAPON] = "sword_serrated";
			break;
		case Skill::AXE:
			items[SLOT_WEAPON] = "axe_crystal";
			break;
		case Skill::BLUNT:
			items[SLOT_WEAPON] = "blunt_dwarven";
			break;
		case Skill::BOW:
			items[SLOT_BOW] = "bow_elven";
			break;
		case Skill::SHIELD:
			items[SLOT_SHIELD] = "shield_mithril";
			break;
		case Skill::LIGHT_ARMOR:
			items[SLOT_ARMOR] = "al_chain_shirt_m";
			break;
		case Skill::MEDIUM_ARMOR:
			items[SLOT_ARMOR] = "am_breastplate_m";
			break;
		case Skill::HEAVY_ARMOR:
			items[SLOT_ARMOR] = "ah_plate_hq";
			break;
		}
	}

	// weapon
	if(!items[SLOT_WEAPON])
	{
		const Skill to_check[] = {
			Skill::SHORT_BLADE,
			Skill::LONG_BLADE,
			Skill::AXE,
			Skill::BLUNT
		};

		cstring& weapon = items[SLOT_WEAPON];
		Skill best = Skill::NONE;
		int val = 0;

		for(Skill sk : to_check)
		{
			if(s[(int)sk].value > val)
			{
				best = sk;
				val = s[(int)sk].value;
			}
		}

		switch(best)
		{
		case Skill::SHORT_BLADE:
			if(val >= 25)
				weapon = "dagger_rapier";
			else if(val >= 20)
				weapon = "dagger_sword";
			else
				weapon = "dagger_short";
			break;
		case Skill::LONG_BLADE:
			if(val >= 25)
				weapon = "sword_orcish";
			else if(val >= 20)
				weapon = "sword_scimitar";
			else
				weapon = "sword_long";
			break;
		case Skill::AXE:
			if(val >= 25)
				weapon = "axe_orcish";
			else if(val >= 20)
				weapon = "axe_battle";
			else
				weapon = "axe_small";
			break;
		case Skill::BLUNT:
			if(val >= 25)
				weapon = "blunt_morningstar";
			else if(val >= 20)
				weapon = "blunt_orcish";
			else if(val >= 15)
				weapon = "blunt_mace";
			else
				weapon = "blunt_club";
			break;
		}
	}

	// bow
	if(!items[SLOT_BOW])
	{
		int v = s[(int)Skill::BOW].value;
		cstring& bow = items[SLOT_BOW];
		if(v >= 30)
			bow = "bow_composite";
		else if(v >= 20)
			bow = "bow_long";
		else if(v >= 10)
			bow = "bow_short";
	}

	// shield
	if(!items[SLOT_SHIELD])
	{
		int v = s[(int)Skill::SHIELD].value;
		cstring& shield = items[SLOT_SHIELD];
		if(v >= 30)
			shield = "shield_steel";
		else if(v >= 20)
			shield = "shield_iron";
		else if(v >= 10)
			shield = "shield_wood";
	}

	// armor
	if(!items[SLOT_ARMOR])
	{
		const Skill to_check[] = {
			Skill::LIGHT_ARMOR,
			Skill::MEDIUM_ARMOR,
			Skill::HEAVY_ARMOR
		};

		cstring& armor = items[SLOT_ARMOR];
		Skill best = Skill::NONE;
		int val = 0;

		for(Skill sk : to_check)
		{
			if(s[(int)sk].value > val)
			{
				best = sk;
				val = s[(int)sk].value;
			}
		}

		switch(best)
		{
		case Skill::LIGHT_ARMOR:
			if(val >= 30)
				armor = "al_chain_shirt";
			else if(val >= 25)
				armor = "al_studded_hq";
			else if(val >= 20)
				armor = "al_studded";
			else if(val >= 15)
				armor = "al_leather";
			else
				armor = "al_padded";
			break;
		case Skill::MEDIUM_ARMOR:
			if(val >= 30)
				armor = "am_scale";
			else if(val >= 20)
				armor = "am_chainmail";
			else
				armor = "am_hide";
			break;
		case Skill::HEAVY_ARMOR:
			if(val >= 30)
				armor = "ah_plated";
			else if(val >= 25)
				armor = "ah_splint_hq";
			else
				armor = "ah_splint";
			break;
		}
	}
}

//=================================================================================================
void WriteCharacterData(BitStream& s, Class c, const HumanData& hd, const CreatedCharacter& cc)
{
	s.WriteCasted<byte>(c);
	hd.Write(s);
	cc.Write(s);
}

//=================================================================================================
int ReadCharacterData(BitStream& s, Class& c, HumanData& hd, CreatedCharacter& cc)
{
	if(!s.ReadCasted<byte>(c))
		return 1;
	if(!ClassInfo::IsPickable(c))
		return 2;
	cc.Clear(c);
	int result = 1;
	if((result = hd.Read(s)) != 0 || (result = cc.Read(s)) != 0)
		return result;
	return 0;
}
