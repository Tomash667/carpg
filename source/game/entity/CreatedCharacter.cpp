#include "Pch.h"
#include "Base.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"

//=================================================================================================
void CreatedCharacter::Clear(Class c)
{
	ClassInfo& info = g_classes[(int)c];

	sp = 2;
	sp_max = 2;
	perks = 2;
	perks_max = 2;

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
			s[i].value += 5;
			s[i].add = true;
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
		PerkInfo& info = g_perks[(int)tp.perk];
		if(!IS_SET(info.flags, PerkInfo::Free))
			--perks;
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			++perks;
			++perks_max;
		}
		switch(tp.perk)
		{
		case Perk::Weakness:
			if(tp.value < 0 || tp.value >= (int)Attribute::MAX)
			{
				ERROR(Format("Perk 'weakness', invalid attribute %d.", tp.value));
				return 2;
			}
			if(a[tp.value].mod)
			{
				ERROR(Format("Perk 'weakness', attribute %d is already modified.", tp.value));
				return 3;
			}
			a[tp.value].value -= 5;
			a[tp.value].mod = true;
			break;
		case Perk::Strength:
			if(tp.value < 0 || tp.value >= (int)Attribute::MAX)
			{
				ERROR(Format("Perk 'strength', invalid attribute %d.", tp.value));
				return 2;
			}
			if(a[tp.value].mod)
			{
				ERROR(Format("Perk 'strength', attribute %d is already modified.", tp.value));
				return 3;
			}
			a[tp.value].value += 5;
			a[tp.value].mod = true;
			break;
		case Perk::Skilled:
			sp += 2;
			sp_max += 2;
			break;
		case Perk::SkillFocus:
			{
				int v[3] = {
					(tp.value & 0xFF),
					((tp.value & 0xFF00) >> 8),
					((tp.value & 0xFF0000) >> 16)
				};

				for(int i = 0; i < 3; ++i)
				{
					if(v[i] < 0 || v[i] >= (int)Skill::MAX)
					{
						ERROR(Format("Perk 'skill_focus', invalid skill %d (%d).", v[i], i));
						return 2;
					}
					if(a[v[i]].mod)
					{
						ERROR(Format("Perk 'skill_focus', skill %d is already modified (%d).", v[i], i));
						return 3;
					}
					s[v[i]].mod = true;
				}

				s[v[0]].value += 10;
				s[v[1]].value -= 5;
				s[v[2]].value -= 5;
			}
			break;
		case Perk::Talent:
			if(tp.value < 0 || tp.value >= (int)Skill::MAX)
			{
				ERROR(Format("Perk 'talent', invalid skill %d.", tp.value));
				return 2;
			}
			if(s[tp.value].mod)
			{
				ERROR(Format("Perk 'talent', skill %d is already modified.", tp.value));
				return 3;
			}
			s[tp.value].mod = true;
			s[tp.value].value += 5;
			break;
		case Perk::CraftingTradition:
			{
				const int cr = (int)Skill::CRAFTING;
				if(s[cr].mod)
				{
					ERROR("Perk 'crafting_tradition', skill is already modified.");
					return 3;
				}
				s[cr].value += 10;
				s[cr].mod = true;
			}
			break;
		}
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
