#include "Pch.h"
#include "Base.h"
#include "CreatedCharacter.h"
#include "StatProfile.h"
#include "UnitData.h"
#include "PlayerController.h"
#include "Unit.h"

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

void CreatedCharacter::Apply(PlayerController& pc)
{
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

	pc.SetRequiredPoints();
	for(int i = 0; i<(int)Attribute::MAX; ++i)
		pc.unit->stats.attrib[i] = pc.unit->unmod_stats.attrib[i] = pc.base_stats.attrib[i];
	for(int i = 0; i<(int)Skill::MAX; ++i)
		pc.unit->stats.skill[i] = pc.unit->unmod_stats.skill[i] = pc.base_stats.skill[i];
	pc.unit->CalculateStats();
	pc.unit->CalculateLoad();
	pc.unit->hp = pc.unit->hpmax = pc.unit->CalculateMaxHp();
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
