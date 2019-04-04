#include "Pch.h"
#include "GameCore.h"
#include "CommandParser.h"
#include "BitStreamFunc.h"
#include "ConsoleCommands.h"
#include "Level.h"
#include "PlayerInfo.h"
#include "NetChangePlayer.h"
#include "Arena.h"
#include "Tokenizer.h"
#include "Game.h"

bool CommandParser::ParseStream(BitStreamReader& f, PlayerInfo& info)
{
	LocalString str;
	PrintMsgFunc prev_func = g_print_func;
	g_print_func = [&str](cstring s)
	{
		if(!str.empty())
			str += "\n";
		str += s;
	};

	bool result = ParseStreamInner(f);

	g_print_func = prev_func;

	if(result && !str.empty())
	{
		NetChangePlayer& c = Add1(info.changes);
		c.type = NetChangePlayer::GENERIC_CMD_RESPONSE;
		c.str = str.Pin();
	}

	return result;
}

void CommandParser::ParseStringCommand(int cmd, const string& s, PlayerInfo& info)
{
	LocalString str;
	PrintMsgFunc prev_func = g_print_func;
	g_print_func = [&str](cstring s)
	{
		if(!str.empty())
			str += "\n";
		str += s;
	};

	switch((CMD)cmd)
	{
	case CMD_ARENA:
		ArenaCombat(s.c_str());
		break;
	}

	g_print_func = prev_func;

	if(!str.empty())
	{
		NetChangePlayer& c = Add1(info.changes);
		c.type = NetChangePlayer::GENERIC_CMD_RESPONSE;
		c.str = str.Pin();
	}
}

bool CommandParser::ParseStreamInner(BitStreamReader& f)
{
	CMD cmd = (CMD)f.Read<byte>();
	switch(cmd)
	{
	case CMD_ADD_EFFECT:
		{
			int netid;
			Effect e;

			f >> netid;
			f.ReadCasted<char>(e.effect);
			f.ReadCasted<char>(e.source);
			f.ReadCasted<char>(e.source_id);
			f.ReadCasted<char>(e.value);
			f >> e.power;
			f >> e.time;

			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_EFFECT: Missing unit %d.", netid);
				return false;
			}
			if(e.effect >= EffectId::Max)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid effect %d.", e.effect);
				return false;
			}
			if(e.source >= EffectSource::Max)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid effect source %d.", e.source);
				return false;
			}
			if(e.source == EffectSource::Perk)
			{
				if(e.source_id >= (int)Perk::Max)
				{
					Error("CommandParser CMD_ADD_EFFECT: Invalid source id %d for perk source.", e.source_id);
					return false;
				}
			}
			else if(e.source_id != -1)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid source id %d for source %d.", e.source_id, e.source);
				return false;
			}
			if(e.time > 0 && e.source != EffectSource::Temporary)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid time %g for source %d.", e.time, e.source);
				return false;
			}

			EffectInfo& info = EffectInfo::effects[(int)e.effect];
			bool ok_value;
			if(info.value_type == EffectInfo::None)
				ok_value = (e.value == -1);
			else if(info.value_type == EffectInfo::Attribute)
				ok_value = (e.value >= 0 && e.value < (int)AttributeId::MAX);
			else
				ok_value = (e.value >= 0 && e.value < (int)SkillId::MAX);
			if(!ok_value)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid value %d for effect %s.", e.value, info.id);
				return false;
			}

			unit->AddEffect(e);
		}
		break;
	case CMD_REMOVE_EFFECT:
		{
			int netid;
			EffectId effect;
			EffectSource source;
			int source_id;
			int value;

			f >> netid;
			f.ReadCasted<char>(effect);
			f.ReadCasted<char>(source);
			f.ReadCasted<char>(source_id);
			f.ReadCasted<char>(value);

			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Missing unit %d.", netid);
				return false;
			}
			if(effect >= EffectId::Max)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid effect %d.", effect);
				return false;
			}
			if(source >= EffectSource::Max)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid effect source %d.", source);
				return false;
			}
			if(source == EffectSource::Perk)
			{
				if(source_id >= (int)Perk::Max)
				{
					Error("CommandParser CMD_REMOVE_EFFECT: Invalid source id %d for perk source.", source_id);
					return false;
				}
			}
			else if(source_id != -1)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid source id %d for source %d.", source_id, source);
				return false;
			}

			if((int)effect >= 0)
			{
				EffectInfo& info = EffectInfo::effects[(int)effect];
				bool ok_value;
				if(info.value_type == EffectInfo::None)
					ok_value = (value == -1);
				else if(info.value_type == EffectInfo::Attribute)
					ok_value = (value >= 0 && value < (int)AttributeId::MAX);
				else
					ok_value = (value >= 0 && value < (int)SkillId::MAX);
				if(!ok_value)
				{
					Error("CommandParser CMD_REMOVE_EFFECT: Invalid value %d for effect %s.", value, info.id);
					return false;
				}
			}

			RemoveEffect(unit, effect, source, source_id, value);
		}
		break;
	case CMD_LIST_EFFECTS:
		{
			int netid;
			f >> netid;
			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_EFFECTS: Missing unit %d.", netid);
				return false;
			}
			ListEffects(unit);
		}
		break;
	case CMD_ADD_PERK:
		{
			int netid, value;
			Perk perk;

			f >> netid;
			f.ReadCasted<char>(perk);
			f.ReadCasted<char>(value);

			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_PERK: Missing unit %d.", netid);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_ADD_PERK: Unit %d is not player.", netid);
				return false;
			}
			if(perk >= Perk::Max)
			{
				Error("CommandParser CMD_ADD_PERK: Invalid perk %d.", perk);
				return false;
			}
			PerkInfo& info = PerkInfo::perks[(int)perk];
			if(info.value_type == PerkInfo::None)
			{
				if(value != -1)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}
			else if(info.value_type == PerkInfo::Attribute)
			{
				if(value <= (int)AttributeId::MAX)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}
			else if(info.value_type == PerkInfo::Skill)
			{
				if(value <= (int)SkillId::MAX)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}

			AddPerk(unit->player, perk, value);
		}
		break;
	case CMD_REMOVE_PERK:
		{
			int netid, value;
			Perk perk;

			f >> netid;
			f.ReadCasted<char>(perk);
			f.ReadCasted<char>(value);

			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_REMOVE_PERK: Missing unit %d.", netid);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_REMOVE_PERK: Unit %d is not player.", netid);
				return false;
			}
			if(perk >= Perk::Max)
			{
				Error("CommandParser CMD_REMOVE_PERK: Invalid perk %d.", perk);
				return false;
			}
			PerkInfo& info = PerkInfo::perks[(int)perk];
			if(info.value_type == PerkInfo::None)
			{
				if(value != -1)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}
			else if(info.value_type == PerkInfo::Attribute)
			{
				if(value <= (int)AttributeId::MAX)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}
			else if(info.value_type == PerkInfo::Skill)
			{
				if(value <= (int)SkillId::MAX)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, info.id);
					return false;
				}
			}

			RemovePerk(unit->player, perk, value);
		}
		break;
	case CMD_LIST_PERKS:
		{
			int netid;
			f >> netid;
			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_PERKS: Missing unit %d.", netid);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_LIST_PERKS: Unit %d is not player.", netid);
				return false;
			}
			ListPerks(unit->player);
		}
		break;
	case CMD_LIST_STATS:
		{
			int netid;
			f >> netid;
			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_STAT: Missing unit %d.", netid);
				return false;
			}
			ListStats(unit);
		}
		break;
	case CMD_ADD_LEARNING_POINTS:
		{
			int netid, count;

			f >> netid;
			f >> count;

			Unit* unit = L.FindUnit(netid);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Missing unit %d.", netid);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Unit %d is not player.", netid);
				return false;
			}
			if(count < 1)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Invalid count %d.", count);
				return false;
			}

			unit->player->AddLearningPoint(count);
		}
		break;
	default:
		Error("Unknown generic command %u.", cmd);
		return false;
	}
	return true;
}

void CommandParser::RemoveEffect(Unit* u, EffectId effect, EffectSource source, int source_id, int value)
{
	uint removed = u->RemoveEffects(effect, source, source_id, value);
	Msg("%u effects removed.", removed);
}

void CommandParser::ListEffects(Unit* u)
{
	if(u->effects.empty())
	{
		Msg("Unit have no effects.");
		return;
	}

	LocalString s;
	s = Format("Unit effects (%u):", u->effects.size());
	for(Effect& e : u->effects)
	{
		EffectInfo& info = EffectInfo::effects[(int)e.effect];
		s += '\n';
		s += info.id;
		if(info.value_type == EffectInfo::Attribute)
			s += Format("(%s)", Attribute::attributes[e.value].id);
		else if(info.value_type == EffectInfo::Skill)
			s += Format("(%s)", Skill::skills[e.value].id);
		s += Format(", power %g, source ", e.power);
		switch(e.source)
		{
		case EffectSource::Temporary:
			s += Format("temporary, time %g", e.time);
			break;
		case EffectSource::Perk:
			s += Format("perk (%s)", PerkInfo::perks[e.source_id].id);
			break;
		case EffectSource::Permanent:
			s += "permanent";
			break;
		}
	}
	Msg(s.c_str());
}

void CommandParser::AddPerk(PlayerController* pc, Perk perk, int value)
{
	if(!pc->AddPerk(perk, value))
		Msg("Unit already have this perk.");
}

void CommandParser::RemovePerk(PlayerController* pc, Perk perk, int value)
{
	if(!pc->RemovePerk(perk, value))
		Msg("Unit don't have this perk.");
}

void CommandParser::ListPerks(PlayerController* pc)
{
	if(pc->perks.empty())
	{
		Msg("Unit have no perks.");
		return;
	}

	LocalString s;
	s = Format("Unit perks (%u):", pc->perks.size());
	for(TakenPerk& tp : pc->perks)
	{
		PerkInfo& info = PerkInfo::perks[(int)tp.perk];
		s += Format("\n%s", info.id);
		if(info.value_type == PerkInfo::Attribute)
			s += Format(" (%s)", Attribute::attributes[tp.value].id);
		else if(info.value_type == PerkInfo::Skill)
			s += Format(" (%s)", Skill::skills[tp.value].id);
	}
	Msg(s.c_str());
}

void CommandParser::ListStats(Unit* u)
{
	int hp = int(u->hp);
	if(hp == 0 && u->hp > 0)
		hp = 1;
	Msg("--- %s (%s) level %d ---", u->GetName(), u->data->id.c_str(), u->level);
	if(u->data->stat_profile && !u->data->stat_profile->subprofiles.empty() && !u->IsPlayer())
	{
		Msg("Profile %s.%s (weapon:%s armor:%s)",
			u->data->stat_profile->id.c_str(),
			u->data->stat_profile->subprofiles[u->stats->subprofile.index]->id.c_str(),
			Skill::skills[(int)WeaponTypeInfo::info[u->stats->subprofile.weapon].skill].id,
			Skill::skills[(int)GetArmorTypeSkill((ARMOR_TYPE)u->stats->subprofile.armor)].id);
	}
	Msg("Health: %d/%d (bonus: %+g, regeneration: %+g/sec, natural: x%g)", hp, (int)u->hpmax, u->GetEffectSum(EffectId::Health),
		u->GetEffectSum(EffectId::Regeneration), u->GetEffectMul(EffectId::NaturalHealingMod));
	Msg("Stamina: %d/%d (bonus: %+g, regeneration: %+g/sec, mod: x%g)", (int)u->stamina, (int)u->stamina_max, u->GetEffectSum(EffectId::Stamina),
		u->GetEffectMax(EffectId::StaminaRegeneration), u->GetEffectMul(EffectId::StaminaRegenerationMod));
	Msg("Melee attack: %s (bonus: %+g), ranged: %s (bonus: %+g)",
		(u->HaveWeapon() || u->data->type == UNIT_TYPE::ANIMAL) ? Format("%d", (int)u->CalculateAttack()) : "-",
		u->GetEffectSum(EffectId::MeleeAttack),
		u->HaveBow() ? Format("%d", (int)u->CalculateAttack(&u->GetBow())) : "-",
		u->GetEffectSum(EffectId::RangedAttack));
	Msg("Defense %d (bonus: %+g), block: %s", (int)u->CalculateDefense(), u->GetEffectSum(EffectId::Defense),
		u->HaveShield() ? Format("%d", (int)u->CalculateBlock()) : "");
	Msg("Mobility: %d (bonus %+g)", (int)u->CalculateMobility(), u->GetEffectSum(EffectId::Mobility));
	Msg("Carry: %g/%g (mod: x%g)", float(u->weight) / 10, float(u->weight_max) / 10, u->GetEffectMul(EffectId::Carry));
	Msg("Magic resistance: %d%%", (int)((1.f - u->CalculateMagicResistance()) * 100));
	Msg("Poison resistance: %d%%", (int)((1.f - u->GetEffectMul(EffectId::PoisonResistance)) * 100));
	LocalString s = "Attributes: ";
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		s += Format("%s:%d ", Attribute::attributes[i].id, u->stats->attrib[i]);
	Msg(s.c_str());
	s = "Skills: ";
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(u->stats->skill[i] > 0)
			s += Format("%s:%d ", Skill::skills[i].id, u->stats->skill[i]);
	}
	Msg(s.c_str());
}

void CommandParser::ArenaCombat(cstring str)
{
	vector<Arena::Enemy> units;
	Tokenizer t;
	t.FromString(str);
	bool any[2] = { false,false };
	try
	{
		bool side = false;
		t.Next();
		while(!t.IsEof())
		{
			if(t.IsItem("vs"))
			{
				side = true;
				t.Next();
			}
			uint count = 1;
			if(t.IsInt())
			{
				count = t.GetInt();
				if(count < 1)
					t.Throw("Invalid count %d.", count);
				t.Next();
			}
			const string& id = t.MustGetItem();
			UnitData* ud = UnitData::TryGet(id);
			if(!ud || IS_SET(ud->flags, F_SECRET))
				t.Throw("Missing unit '%s'.", id.c_str());
			else if(ud->group == G_PLAYER)
				t.Throw("Unit '%s' can't be spawned.", id.c_str());
			t.Next();
			int level = -2;
			if(t.IsItem("lvl"))
			{
				t.Next();
				level = t.MustGetInt();
				t.Next();
			}
			units.push_back({ ud, count, level, side });
			any[side ? 1 : 0] = true;
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Msg("Broken units list: %s", e.ToString());
		return;
	}

	if(units.size() == 0)
		Msg("Empty units list.");
	else if(!any[0] || !any[1])
		Msg("Missing other units.");
	else
		Game::Get().arena->SpawnUnit(units);
}
