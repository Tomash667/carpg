#include "Pch.h"
#include "Core.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "Stock.h"

//-----------------------------------------------------------------------------
struct TakeRatio
{
	float str, end, dex;
};

//=================================================================================================
void CreatedCharacter::Clear(Class c)
{
	ClassInfo& info = ClassInfo::classes[(int)c];

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
		profile = Rand() % 2 + 1;
		break;
	default:
		assert(0);
	case Class::HUNTER:
	case Class::ROGUE:
		profile = Rand() % 2;
		break;
	}

	Skill sk1, sk2, sk3;
	Attribute talent;

	switch(profile)
	{
	case 0: // light
		{
			sk1 = Skill::LIGHT_ARMOR;
			if(Rand() % 2 == 0)
				sk2 = Skill::SHORT_BLADE;
			else
				sk2 = RandomItem({ Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = Skill::BOW;
			talent = Attribute::DEX;
		}
		break;
	case 1: // medium
		{
			sk1 = Skill::MEDIUM_ARMOR;
			sk2 = RandomItem({ Skill::SHORT_BLADE, Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = RandomItem({ Skill::BOW, Skill::SHIELD });
			talent = RandomItem({ Attribute::DEX, Attribute::END });
		}
		break;
	case 2: // heavy
		{
			sk1 = Skill::HEAVY_ARMOR;
			sk2 = RandomItem({ Skill::LONG_BLADE, Skill::AXE, Skill::BLUNT });
			sk3 = Skill::SHIELD;
			talent = RandomItem({ Attribute::STR, Attribute::END });
		}
		break;
	}

	s[(int)sk1].Add(5, true);
	s[(int)sk2].Add(5, true);
	s[(int)sk3].Add(5, true);

	PerkContext ctx(this);

	TakenPerk taken_perk(Perk::Talent, (int)talent);
	taken_perk.Apply(ctx);

	taken_perk.perk = Perk::SkillFocus;
	taken_perk.value = (int)RandomItem({ sk1, sk2, sk3 });
	taken_perk.Apply(ctx);

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
	PerkContext ctx(this);
	ctx.validate = true;
	TakenPerk tp;
	taken_perks.reserve(count);
	for(uint i=0; i<count; ++i)
	{
		if(!stream.ReadCasted<byte>(tp.perk) || !stream.Read(tp.value))
			return 1;
		if(tp.perk >= Perk::Max)
		{
			Error("Invalid perk id '%d'.", tp.perk);
			return 2;
		}
		if(!tp.Validate())
		{
			auto& info = PerkInfo::perks[(int)tp.perk];
			Error("Broken perk '%s' value.", info.id);
			return 2;
		}
		if(!tp.Apply(ctx))
		{
			Error("Failed to validate perks for character.");
			return 3;
		}
	}

	// search for duplicates
	for(vector<TakenPerk>::iterator it = taken_perks.begin(), end = taken_perks.end(); it != end; ++it)
	{
		const PerkInfo& info = PerkInfo::perks[(int)it->perk];
		for(vector<TakenPerk>::iterator it2 = it + 1; it2 != end; ++it2)
		{
			if(it->perk == it2->perk)
			{
				Error("Multiple same perks '%s'.", info.id);
				return 3;
			}
		}
	}

	// check for too many sp/perks
	if(sp < 0 || perks < 0)
	{
		Error("Too many skill points (%d) / perks (%d).", -min(sp, 0), -min(perks, 0));
		return 3;
	}

	return 0;
}

//=================================================================================================
void CreatedCharacter::Apply(PlayerController& pc)
{
	pc.unit->statsx->ApplyBase(pc.unit->data->stat_profile);

	// apply skills
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(s[i].add)
		{
			int value = (pc.unit->data->stat_profile->ShouldDoubleSkill((Skill)i) ? 10 : 5);
			pc.unit->statsx->skill_base[i] += value;
			pc.unit->statsx->skill_apt[i] = StatsX::SkillToAptitude(pc.unit->statsx->skill_base[i]);
		}
	}

	// apply perks
	PerkContext ctx(&pc);
	for(TakenPerk& tp : taken_perks)
		tp.Apply(ctx);
	pc.perk_points = perks_max - perks;

	// set stats
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		pc.unit->statsx->attrib[i] = pc.unit->statsx->attrib_base[i];
	for(int i = 0; i < (int)Skill::MAX; ++i)
		pc.unit->statsx->skill[i] = pc.unit->statsx->skill_base[i];
	pc.unit->CalculateStats(true);
	pc.RecalculateLevel();
	pc.SetRequiredPoints();

	// inventory
	Game::Get().ParseItemScript(*pc.unit, pc.unit->data->item_script);
	const Item* items[SLOT_MAX];
	GetStartingItems(items);
	for(int i = 0; i < SLOT_MAX; ++i)
		pc.unit->slots[i] = items[i];
	if(HavePerk(Perk::AlchemistApprentice))
	{
		Stock::Get("alchemist_apprentice")->Parse(0, false, pc.unit->items);
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
void CreatedCharacter::GetStartingItems(const Item* (&items)[SLOT_MAX])
{
	items[SLOT_WEAPON] = nullptr;
	items[SLOT_BOW] = nullptr;
	items[SLOT_SHIELD] = nullptr;
	items[SLOT_ARMOR] = nullptr;

	int mod = 0;
	if(HavePerk(Perk::Poor))
		mod = -5;
	else if(HavePerk(Perk::VeryWealthy))
		mod = 5;
	else if(HavePerk(Perk::FilthyRich))
		mod = 10;

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

		const Item* heirloom = StartItem::GetStartItem(best, HEIRLOOM);
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

		items[SLOT_WEAPON] = StartItem::GetStartItem(best, max(0, val + mod));
	}

	// bow
	if(!items[SLOT_BOW])
	{
		int val = s[(int)Skill::BOW].value;
		items[SLOT_BOW] = StartItem::GetStartItem(Skill::BOW, max(0, val + mod));
	}

	// shield
	if(!items[SLOT_SHIELD])
	{
		int val = s[(int)Skill::SHIELD].value;
		items[SLOT_SHIELD] = StartItem::GetStartItem(Skill::SHIELD, max(0, val + mod));
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

		items[SLOT_ARMOR] = StartItem::GetStartItem(best, max(0, val + mod));
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
