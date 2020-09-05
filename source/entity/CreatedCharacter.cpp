#include "Pch.h"
#include "CreatedCharacter.h"

#include "BitStreamFunc.h"
#include "UnitData.h"

//=================================================================================================
void CreatedCharacter::Clear(Class* clas)
{
	this->clas = clas;
	sp_max = StatProfile::MAX_TAGS;
	perks_max = StatProfile::MAX_PERKS;

	sp = sp_max;
	perks = perks_max;

	StatProfile& profile = clas->player->GetStatProfile();

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		a[i].value = profile.attrib[i];
		a[i].mod = false;
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
void CreatedCharacter::Random(Class* clas)
{
	Clear(clas);

	StatProfile& profile = clas->player->GetStatProfile();
	SubprofileInfo sub = profile.GetRandomSubprofile(&last_sub);
	StatProfile::Subprofile& subprofile = *profile.subprofiles[sub.index];
	last_sub = sub;

	// apply tag skills
	for(int i = 0; i < StatProfile::MAX_TAGS; ++i)
	{
		SkillId sk = subprofile.tag_skills[i];
		if(sk == SkillId::NONE)
			break;
		sk = sub.GetSkill(sk);
		s[(int)sk].Add(Skill::TAG_BONUS, true);
	}

	// apply perks
	PerkContext ctx(this);
	for(int i = 0; i < StatProfile::MAX_PERKS; ++i)
	{
		TakenPerk& tp = subprofile.perks[i];
		if(!tp.perk)
			break;
		int value = tp.value;
		if(tp.perk->value_type == Perk::Skill)
			value = (int)sub.GetSkill((SkillId)value);
		TakenPerk perk(tp.perk, value);
		perk.Apply(ctx);
		taken_perks.push_back(perk);
	}

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
		f << tp.perk->hash;
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
		if(IsSet(sk, 1 << i))
		{
			if(s[i].value < 0)
			{
				Error("Skill increase for disabled skill '%s'.", Skill::skills[i].id);
				return 3;
			}
			s[i].Add(Skill::TAG_BONUS, true);
			--sp;
		}
	}

	// perks
	PerkContext ctx(this);
	ctx.validate = true;
	taken_perks.resize(count, TakenPerk(nullptr));
	for(TakenPerk& tp : taken_perks)
	{
		int perk_hash = f.Read<int>();
		f >> tp.value;
		if(!f)
			return 1;
		tp.perk = Perk::Get(perk_hash);
		if(!tp.perk)
		{
			Error("Invalid perk id '%d'.", perk_hash);
			return 2;
		}
		if(!tp.CanTake(ctx))
		{
			Error("Can't take perk '%s'.", tp.perk->id.c_str());
			return 3;
		}
		tp.Apply(ctx);
	}

	// search for duplicates, too many types
	int history_perks = 0,
		flaw_perks = 0;
	for(vector<TakenPerk>::iterator it = taken_perks.begin(), end = taken_perks.end(); it != end; ++it)
	{
		for(vector<TakenPerk>::iterator it2 = it + 1; it2 != end; ++it2)
		{
			if(it->perk == it2->perk)
			{
				Error("Duplicated perk '%s'.", it->perk->id.c_str());
				return 3;
			}
		}
		if(IsSet(it->perk->flags, Perk::History))
			++history_perks;
		if(IsSet(it->perk->flags, Perk::Flaw))
			++flaw_perks;
	}
	if(history_perks > 1)
	{
		Error("Too many (%d) history perks.", history_perks);
		return 3;
	}
	if(flaw_perks > 2)
	{
		Error("Toom many (%d) flaw perks.", flaw_perks);
		return 3;
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
bool CreatedCharacter::HavePerk(Perk* perk) const
{
	for(const TakenPerk& tp : taken_perks)
	{
		if(tp.perk == perk)
			return true;
	}

	return false;
}

//=================================================================================================
void CreatedCharacter::GetStartingItems(array<const Item*, SLOT_MAX>& items)
{
	for(int i = 0; i < SLOT_MAX; ++i)
		items[i] = nullptr;

	const bool mage_items = IsSet(clas->flags, Class::F_MAGE_ITEMS);
	const bool poor = HavePerk(Perk::Get("poor"));

	if(HavePerk(Perk::Get("heirloom")))
	{
		SkillId best = SkillId::NONE;
		int best_value = 0, best_value2 = 0;
		for(const StartItem& item : StartItem::start_items)
		{
			if(item.value != HEIRLOOM)
				continue;
			int value = s[(int)item.skill].value;
			if(value >= best_value)
			{
				Skill& skill = Skill::skills[(int)item.skill];
				int value2 = a[(int)skill.attrib].value;
				if(skill.attrib2 == AttributeId::NONE)
					value2 *= 2;
				else
					value2 += a[(int)skill.attrib2].value;
				if(value > best_value || (value == best_value && value2 > best_value2))
				{
					best = skill.skill_id;
					best_value = value;
					best_value2 = value2;
				}
			}
		}

		const Item* heirloom = StartItem::GetStartItem(best, HEIRLOOM, mage_items);
		items[ItemTypeToSlot(heirloom->type)] = heirloom;
	}

	// weapon
	if(!items[SLOT_WEAPON])
	{
		const SkillId to_check[] = {
			SkillId::SHORT_BLADE,
			SkillId::LONG_BLADE,
			SkillId::AXE,
			SkillId::BLUNT,
			SkillId::MYSTIC_MAGIC
		};

		SkillId best = SkillId::NONE;
		int best_value = 0, best_value2 = 0;
		for(SkillId sk : to_check)
		{
			int value = s[(int)sk].value;
			if(value >= best_value)
			{
				Skill& skill = Skill::skills[(int)sk];
				int value2 = a[(int)skill.attrib].value;
				if(skill.attrib2 == AttributeId::NONE)
					value2 *= 2;
				else
					value2 += a[(int)skill.attrib2].value;
				if(value > best_value || (value == best_value && value2 > best_value2))
				{
					best = skill.skill_id;
					best_value = value;
					best_value2 = value2;
				}
			}
		}

		items[SLOT_WEAPON] = StartItem::GetStartItem(best, GetItemLevel(best_value, poor), mage_items);
	}

	// bow
	if(!items[SLOT_BOW])
	{
		int val = s[(int)SkillId::BOW].value;
		items[SLOT_BOW] = StartItem::GetStartItem(SkillId::BOW, GetItemLevel(val, poor), mage_items);
	}

	// shield
	if(!items[SLOT_SHIELD])
	{
		int val = s[(int)SkillId::SHIELD].value;
		items[SLOT_SHIELD] = StartItem::GetStartItem(SkillId::SHIELD, GetItemLevel(val, poor), mage_items);
	}

	// armor
	if(!items[SLOT_ARMOR])
	{
		const SkillId to_check[] = {
			SkillId::HEAVY_ARMOR,
			SkillId::MEDIUM_ARMOR,
			SkillId::LIGHT_ARMOR
		};

		SkillId best = SkillId::NONE;
		int best_value = 0, best_value2 = 0;
		for(SkillId sk : to_check)
		{
			int value = s[(int)sk].value;
			if(value >= best_value)
			{
				Skill& skill = Skill::skills[(int)sk];
				int value2 = a[(int)skill.attrib].value;
				if(skill.attrib2 == AttributeId::NONE)
					value2 *= 2;
				else
					value2 += a[(int)skill.attrib2].value;
				if(value > best_value || (value == best_value && value2 > best_value2))
				{
					best = skill.skill_id;
					best_value = value;
					best_value2 = value2;
				}
			}
		}

		items[SLOT_ARMOR] = StartItem::GetStartItem(best, GetItemLevel(best_value, poor), mage_items);
	}
}

//=================================================================================================
int CreatedCharacter::GetItemLevel(int level, bool poor)
{
	if(level > 5)
	{
		if(poor)
			level = max(level - 10, 5);
	}
	else if(level < 1)
		level = 1;
	return level;
}

//=================================================================================================
void WriteCharacterData(BitStreamWriter& f, Class* clas, const HumanData& hd, const CreatedCharacter& cc)
{
	f << clas->id;
	hd.Write(f);
	cc.Write(f);
}

//=================================================================================================
int ReadCharacterData(BitStreamReader& f, Class*& clas, HumanData& hd, CreatedCharacter& cc)
{
	const string& class_id = f.ReadString1();
	if(!f)
		return 1;
	clas = Class::TryGet(class_id);
	if(!clas || !clas->IsPickable())
		return 2;
	cc.Clear(clas);
	int result = 1;
	if((result = hd.Read(f)) != 0 || (result = cc.Read(f)) != 0)
		return result;
	return 0;
}
