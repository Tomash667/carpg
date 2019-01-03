#include "Pch.h"
#include "GameCore.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "Stock.h"
#include "BitStreamFunc.h"

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

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		a[i].value = profile.attrib[i];
		a[i].mod = false;
		a[i].required = 0;
	}

	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		s[i].value = profile.skill[i];
		s[i].base = s[i].value;
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

	SkillId sk1, sk2, sk3;
	AttributeId talent;

	switch(profile)
	{
	case 0: // light
		{
			sk1 = SkillId::LIGHT_ARMOR;
			if(Rand() % 2 == 0)
				sk2 = SkillId::SHORT_BLADE;
			else
				sk2 = RandomItem({ SkillId::LONG_BLADE, SkillId::AXE, SkillId::BLUNT });
			sk3 = SkillId::BOW;
			talent = AttributeId::DEX;
		}
		break;
	case 1: // medium
		{
			sk1 = SkillId::MEDIUM_ARMOR;
			sk2 = RandomItem({ SkillId::SHORT_BLADE, SkillId::LONG_BLADE, SkillId::AXE, SkillId::BLUNT });
			sk3 = RandomItem({ SkillId::BOW, SkillId::SHIELD });
			talent = RandomItem({ AttributeId::DEX, AttributeId::END });
		}
		break;
	case 2: // heavy
		{
			sk1 = SkillId::HEAVY_ARMOR;
			sk2 = RandomItem({ SkillId::LONG_BLADE, SkillId::AXE, SkillId::BLUNT });
			sk3 = SkillId::SHIELD;
			talent = RandomItem({ AttributeId::STR, AttributeId::END });
		}
		break;
	}

	s[(int)sk1].Add(GetBonus(sk1), true);
	s[(int)sk2].Add(GetBonus(sk2), true);
	s[(int)sk3].Add(GetBonus(sk3), true);
	TakenPerk tp(Perk::Talent, (int)talent);
	PerkContext ctx(this);
	tp.Apply(ctx);
	taken_perks.push_back(tp);
	SkillId focus = RandomItem({ sk1, sk2, sk3 });
	tp = TakenPerk(Perk::SkillFocus, (int)focus);
	tp.Apply(ctx);
	taken_perks.push_back(tp);

	sp = 0;
	perks = 0;
}

//=================================================================================================
void CreatedCharacter::Write(BitStreamWriter& f) const
{
	// picked skills
	int sk = 0;
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(s[i].add)
			sk |= (1 << i);
	}
	f << sk;

	// perks
	f.WriteCasted<byte>(taken_perks.size());
	for(const TakenPerk& tp : taken_perks)
	{
		f.WriteCasted<byte>(tp.perk);
		f << tp.value;
	}
}

//=================================================================================================
int CreatedCharacter::Read(BitStreamReader& f)
{
	// picked skills
	int sk;
	byte count;

	f >> sk;
	f >> count;
	if(!f)
		return 1;

	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(IS_SET(sk, 1 << i))
		{
			if(s[i].value < 0)
			{
				Error("Skill increase for disabled skill '%s'.", Skill::skills[i].id);
				return 3;
			}
			s[i].Add(GetBonus((SkillId)i), true);
			--sp;
		}
	}

	// perks
	PerkContext ctx(this);
	ctx.validate = true;
	taken_perks.resize(count, TakenPerk(Perk::None));
	for(TakenPerk& tp : taken_perks)
	{
		f.ReadCasted<byte>(tp.perk);
		f >> tp.value;
		if(!f)
			return 1;
		if(tp.perk >= Perk::Max)
		{
			Error("Invalid perk id '%d'.", tp.perk);
			return 2;
		}
		if(!tp.CanTake(ctx))
		{
			Error("Can't take perk '%s'.", PerkInfo::perks[(int)tp.perk].id);
			return 3;
		}
		tp.Apply(ctx);
	}
	for(uint i = 0, count = taken_perks.size(); i < count; ++i)
	{
		for(uint j = i + 1; j < count; ++j)
		{
			if(taken_perks[i].perk == taken_perks[j].perk)
			{
				Error("Duplicated perk '%s'.", PerkInfo::perks[(int)taken_perks[i].perk].id);
				return 3;
			}
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
	pc.unit->data->GetStatProfile().Set(0, *pc.unit->stats);

	// reset blocked stats, apply skill bonus
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		pc.attrib[i].blocked = false;
		pc.attrib[i].apt = (pc.unit->stats->attrib[i] - 50) / 5;
	}
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		pc.skill[i].blocked = false;
		if(s[i].add)
			pc.unit->stats->skill[i] += GetBonus((SkillId)i);
		pc.skill[i].apt = pc.unit->stats->skill[i] / 5;
	}

	// apply perks
	PerkContext ctx(&pc, true);
	pc.perks = taken_perks;
	for(TakenPerk& tp : pc.perks)
		tp.Apply(ctx);

	pc.unit->CalculateStats();
	pc.unit->CalculateLoad();
	pc.RecalculateLevel(false);
	pc.unit->hp = pc.unit->hpmax = pc.unit->CalculateMaxHp();

	pc.SetRequiredPoints();

	// inventory
	Game::Get().ParseItemScript(*pc.unit, pc.unit->data->item_script);
	const Item* items[SLOT_MAX];
	GetStartingItems(items);
	for(int i = 0; i < SLOT_MAX; ++i)
		pc.unit->slots[i] = items[i];
	if(HavePerk(Perk::AlchemistApprentice))
		Stock::Get("alchemist_apprentice")->Parse(pc.unit->items);
	pc.unit->MakeItemsTeam(false);
	pc.unit->RecalculateWeight();
	if(!pc.HavePerk(Perk::Poor))
		pc.unit->gold += pc.unit->GetBase(SkillId::HAGGLE);
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
	for(int i = 0; i < SLOT_MAX; ++i)
		items[i] = nullptr;

	if(HavePerk(Perk::FamilyHeirloom))
	{
		// find best skill for family heirloom
		const SkillId to_check[] = {
			SkillId::SHORT_BLADE,
			SkillId::LONG_BLADE,
			SkillId::AXE,
			SkillId::BLUNT,
			SkillId::BOW,
			SkillId::SHIELD,
			SkillId::LIGHT_ARMOR,
			SkillId::MEDIUM_ARMOR,
			SkillId::HEAVY_ARMOR
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

		SkillId best = SkillId::NONE;
		int val = 0, val2 = 0;

		int index = 0;
		for(SkillId sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				int s_val2 = int(ratio[index].str * Get(AttributeId::STR)
					+ ratio[index].end * Get(AttributeId::END)
					+ ratio[index].dex * Get(AttributeId::DEX));
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

	int mod = HavePerk(Perk::Poor) ? -10 : 0;

	// weapon
	if(!items[SLOT_WEAPON])
	{
		const SkillId to_check[] = {
			SkillId::SHORT_BLADE,
			SkillId::LONG_BLADE,
			SkillId::AXE,
			SkillId::BLUNT
		};

		SkillId best = SkillId::NONE;
		int val = 0, val2 = 0;

		for(SkillId sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				const WeaponTypeInfo& info = GetWeaponTypeInfo(sk);
				int s_val2 = int(info.str2dmg * Get(AttributeId::STR) + info.dex2dmg * Get(AttributeId::DEX));
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
		int val = s[(int)SkillId::BOW].value;
		items[SLOT_BOW] = StartItem::GetStartItem(SkillId::BOW, max(0, val + mod));
	}

	// shield
	if(!items[SLOT_SHIELD])
	{
		int val = s[(int)SkillId::SHIELD].value;
		items[SLOT_SHIELD] = StartItem::GetStartItem(SkillId::SHIELD, max(0, val + mod));
	}

	// armor
	if(!items[SLOT_ARMOR])
	{
		const SkillId to_check[] = {
			SkillId::HEAVY_ARMOR,
			SkillId::MEDIUM_ARMOR,
			SkillId::LIGHT_ARMOR
		};

		const TakeRatio ratio[] = {
			0.f, 0.f, 1.f,
			0.f, 0.5f, 0.5f,
			0.5f, 0.5f, 0.f
		};

		SkillId best = SkillId::NONE;
		int val = 0, val2 = 0;

		int index = 0;
		for(SkillId sk : to_check)
		{
			int s_val = s[(int)sk].value;
			if(s_val >= val)
			{
				int s_val2 = int(ratio[index].str * Get(AttributeId::STR)
					+ ratio[index].end * Get(AttributeId::END)
					+ ratio[index].dex * Get(AttributeId::DEX));
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
int CreatedCharacter::GetBonus(SkillId skill)
{
	int base = s[(int)skill].base;
	if(base == 0)
		return 15;
	else if(base < 20)
		return 10;
	else
		return 5;
}

//=================================================================================================
void WriteCharacterData(BitStreamWriter& f, Class c, const HumanData& hd, const CreatedCharacter& cc)
{
	f.WriteCasted<byte>(c);
	hd.Write(f);
	cc.Write(f);
}

//=================================================================================================
int ReadCharacterData(BitStreamReader& f, Class& c, HumanData& hd, CreatedCharacter& cc)
{
	f.ReadCasted<byte>(c);
	if(!f)
		return 1;
	if(!ClassInfo::IsPickable(c))
		return 2;
	cc.Clear(c);
	int result = 1;
	if((result = hd.Read(f)) != 0 || (result = cc.Read(f)) != 0)
		return result;
	return 0;
}
