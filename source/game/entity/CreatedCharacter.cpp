#include "Pch.h"
#include "Base.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"

//-----------------------------------------------------------------------------
struct TakeRatio
{
	float str, end, dex;
};

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
void CreatedCharacter::Random(Class c)
{
	Clear(c);

	int profile;
	switch(c)
	{
	case Class::WARRIOR:
		profile = rand2() % 2 + 1;
		break;
	default:
		assert(0);
	case Class::HUNTER:
	case Class::ROGUE:
		profile = rand2() % 2;
		break;
	}

	Skill sk1, sk2, sk3;
	Attribute strength;

	switch(profile)
	{
	case 0: // light
		{
			sk1 = Skill::LIGHT_ARMOR;
			if(rand2() % 2 == 0)
				sk2 = Skill::SHORT_BLADE;
			else
				sk2 = RandomItem({ Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = Skill::BOW;
			strength = Attribute::DEX;
		}
		break;
	case 1: // medium
		{
			sk1 = Skill::MEDIUM_ARMOR;
			sk2 = RandomItem({ Skill::SHORT_BLADE, Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = RandomItem({ Skill::BOW, Skill::SHIELD });
			strength = RandomItem({ Attribute::DEX, Attribute::END });
		}
		break;
	case 2: // heavy
		{
			sk1 = Skill::HEAVY_ARMOR;
			sk2 = RandomItem({ Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = Skill::SHIELD;
			strength = RandomItem({ Attribute::STR, Attribute::END });
		}
		break;
	}

	s[(int)sk1].Add(5, true);
	s[(int)sk2].Add(5, true);
	s[(int)sk3].Add(5, true);
	taken_perks.push_back(TakenPerk(Perk::Strength, (int)strength));
	a[(int)strength].Mod(5, true);
	Skill talent = RandomItem({ sk1, sk2, sk3 });
	taken_perks.push_back(TakenPerk(Perk::Talent, (int)talent));
	s[(int)talent].Mod(5, true);

	sp = 0;
	perks = 0;
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

	// apply skills
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		if(s[i].add)
			pc.base_stats.skill[i] += 5;
	}

	// apply perks
	pc.perks = taken_perks;
	for(TakenPerk& tp : pc.perks)
		tp.Apply(pc);

	// set stats
	for(int i = 0; i<(int)Attribute::MAX; ++i)
		pc.unit->unmod_stats.attrib[i] = pc.base_stats.attrib[i];
	for(int i = 0; i<(int)Skill::MAX; ++i)
		pc.unit->unmod_stats.skill[i] = pc.base_stats.skill[i];
	
	pc.unit->CalculateStats();
	pc.unit->CalculateLoad();
	pc.unit->hp = pc.unit->hpmax = pc.unit->CalculateMaxHp();
	pc.unit->level = pc.unit->CalculateLevel();

	pc.SetRequiredPoints();

	// inventory
	Game::Get().ParseItemScript(*pc.unit, pc.unit->data->items, pc.unit->data->new_items);
	const Item* items[4];
	GetStartingItems(items);
	for(int i = 0; i < 4; ++i)
		pc.unit->slots[i] = items[i];
	if(HavePerk(Perk::AlchemistApprentice))
	{
		ParseStockScript(FindStockScript("alchemist_apprentice"), 0, false, pc.unit->items);
		SortItems(pc.unit->items);
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
void CreatedCharacter::GetStartingItems(const Item* (&items)[4])
{
	items[SLOT_WEAPON] = nullptr;
	items[SLOT_BOW] = nullptr;
	items[SLOT_SHIELD] = nullptr;
	items[SLOT_ARMOR] = nullptr;

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

		const TakeRatio ratio[] = {
			0.5f, 0, 0.5f,
			0.75f, 0, 0.25f,
			0.85f, 0, 0.15f,
			0.8f, 0, 0.2f,
			0, 0, 1,
			0.5f, 0, 0.5f,
			0, 0, 1,
			0, 0.5f, 0.5f,
			0.5f, 0.5f, 0
		};

		Skill best = Skill::NONE;
		int val = 0, val2 = 0;

		int index = 0;
		for(Skill sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				int s_val2 = int(ratio[index].str * Get(Attribute::STR)
					+ ratio[index].end * Get(Attribute::END)
					+ ratio[index].dex * Get(Attribute::DEX));
				if(s_val > val || s_val2 > val2)
				{
					val = s_val;
					val2 = s_val2;
					best = sk;
				}
			}
			++index;
		}

		const Item* heirloom = GetStartItem(best, HEIRLOOM);
		items[ItemTypeToSlot(heirloom->type)] = heirloom;
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

		Skill best = Skill::NONE;
		int val = 0, val2 = 0;

		for(Skill sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				const WeaponTypeInfo& info = GetWeaponTypeInfo(sk);
				int s_val2 = int(info.str2dmg * Get(Attribute::STR) + info.dex2dmg * Get(Attribute::DEX));
				if(s_val > val || s_val2 > val2)
				{
					val = s_val;
					val2 = s_val2;
					best = sk;
				}
			}
		}

		items[SLOT_WEAPON] = GetStartItem(best, val);
	}

	// bow
	if(!items[SLOT_BOW])
	{
		int val = s[(int)Skill::BOW].value;
		items[SLOT_BOW] = GetStartItem(Skill::BOW, val);
	}

	// shield
	if(!items[SLOT_SHIELD])
	{
		int val = s[(int)Skill::SHIELD].value;
		items[SLOT_SHIELD] = GetStartItem(Skill::SHIELD, val);
	}

	// armor
	if(!items[SLOT_ARMOR])
	{
		const Skill to_check[] = {
			Skill::HEAVY_ARMOR,
			Skill::MEDIUM_ARMOR,
			Skill::LIGHT_ARMOR
		};

		const TakeRatio ratio[] = {
			0.f, 0.f, 1.f,
			0.f, 0.5f, 0.5f,
			0.5f, 0.5f, 0.f
		};

		Skill best = Skill::NONE;
		int val = 0, val2 = 0;

		int index = 0;
		for(Skill sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				int s_val2 = int(ratio[index].str * Get(Attribute::STR)
					+ ratio[index].end * Get(Attribute::END)
					+ ratio[index].dex * Get(Attribute::DEX));
				if(s_val > val || s_val2 > val2)
				{
					val = s_val;
					val2 = s_val2;
					best = sk;
				}
			}
			++index;
		}

		items[SLOT_ARMOR] = GetStartItem(best, val);
	}
}

//=================================================================================================
void WriteCharacterData(BitStream& stream, Class c, const HumanData& hd, const CreatedCharacter& cc)
{
	stream.WriteCasted<byte>(c);
	hd.Write(stream);
	cc.Write(stream);
}

//=================================================================================================
int ReadCharacterData(BitStream& stream, Class& c, HumanData& hd, CreatedCharacter& cc)
{
	if(!stream.ReadCasted<byte>(c))
		return 1;
	if(!ClassInfo::IsPickable(c))
		return 2;
	cc.Clear(c);
	int result = 1;
	if((result = hd.Read(stream)) != 0 || (result = cc.Read(stream)) != 0)
		return result;
	return 0;
}
