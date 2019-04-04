#include "Pch.h"
#include "GameCore.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Class.h"
#include "Action.h"
#include "Level.h"
#include "GlobalGui.h"
#include "GameMessages.h"
#include "Team.h"
#include "World.h"
#include "ScriptException.h"

//=================================================================================================
PlayerController::~PlayerController()
{
	if(dialog_ctx && !dialog_ctx->is_local)
		delete dialog_ctx;
}

//=================================================================================================
void PlayerController::Init(Unit& _unit, bool partial)
{
	unit = &_unit;
	move_tick = 0.f;
	last_weapon = W_NONE;
	next_action = NA_NONE;
	last_dmg_poison = last_dmg = dmgc = poison_dmgc = 0.f;
	idle_timer = Random(1.f, 2.f);
	credit = 0;
	on_credit = false;
	godmode = false;
	noclip = false;
	action = Action_None;
	free_days = 0;
	recalculate_level = false;
	split_gold = 0.f;
	always_run = true;

	if(!partial)
	{
		kills = 0;
		dmg_done = 0;
		dmg_taken = 0;
		knocks = 0;
		arena_fights = 0;

		for(int i = 0; i < (int)SkillId::MAX; ++i)
		{
			skill[i].points = 0;
			skill[i].train = 0;
			skill[i].train_part = 0;
		}
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
		{
			attrib[i].points = 0;
			attrib[i].train = 0;
			attrib[i].train_part = 0;
		}

		action_charges = GetAction().charges;
		learning_points = 0;
		exp = 0;
		exp_level = 0;
		exp_need = GetExpNeed();
	}
}

//=================================================================================================
void PlayerController::Update(float dt, bool is_local)
{
	if(is_local)
	{
		// last damage
		dmgc += last_dmg;
		dmgc *= (1.f - dt * 2);
		if(last_dmg == 0.f && dmgc < 0.1f)
			dmgc = 0.f;

		// poison damage
		poison_dmgc += (last_dmg_poison - poison_dmgc) * dt;
		if(last_dmg_poison == 0.f && poison_dmgc < 0.1f)
			poison_dmgc = 0.f;

		if(recalculate_level)
		{
			recalculate_level = false;
			RecalculateLevel();
		}
	}

	// update action
	auto& action = GetAction();
	if(action_cooldown != 0)
	{
		action_cooldown -= dt;
		if(action_cooldown < 0)
			action_cooldown = 0.f;
	}
	if(action_recharge != 0)
	{
		action_recharge -= dt;
		if(action_recharge < 0)
		{
			++action_charges;
			if(action_charges == action.charges)
				action_recharge = 0;
			else
				action_recharge += action.recharge;
		}
	}
}

//=================================================================================================
void PlayerController::Train(SkillId skill, float points)
{
	assert(Net::IsLocal());
	int s = (int)skill;
	StatData& stat = this->skill[s];
	points += stat.train_part;
	int int_points = (int)points;
	stat.train_part = points - int_points;
	stat.points += int_points;

	int gained = 0,
		value = unit->GetBase(skill);

	while(stat.points >= stat.next)
	{
		stat.points -= stat.next;
		if(value != Skill::MAX)
		{
			++gained;
			++value;
			stat.next = GetRequiredSkillPoints(value);
		}
		else
		{
			stat.points = stat.next;
			break;
		}
	}

	if(gained)
	{
		recalculate_level = true;
		unit->Set(skill, value);
		Game::Get().gui->messages->AddFormattedMessage(this, GMS_GAIN_SKILL, s, gained);
		if(!is_local)
		{
			NetChangePlayer& c2 = Add1(player_info->changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
			c2.id = (int)ChangedStatType::SKILL;
			c2.a = s;
			c2.count = value;
		}
	}
}

//=================================================================================================
void PlayerController::Train(AttributeId attrib, float points)
{
	assert(Net::IsLocal());
	int a = (int)attrib;
	StatData& stat = this->attrib[a];
	points += stat.train_part;
	int int_points = (int)points;
	stat.train_part = points - int_points;
	stat.points += int_points;

	int gained = 0,
		value = unit->GetBase(attrib);

	while(stat.points >= stat.next)
	{
		stat.points -= stat.next;
		if(unit->stats->attrib[a] != Attribute::MAX)
		{
			++gained;
			++value;
			stat.next = GetRequiredAttributePoints(value);
		}
		else
		{
			stat.points = stat.next;
			break;
		}
	}

	if(gained)
	{
		recalculate_level = true;
		unit->Set(attrib, value);
		Game::Get().gui->messages->AddFormattedMessage(this, GMS_GAIN_ATTRIBUTE, a, gained);
		if(!is_local)
		{
			NetChangePlayer& c2 = Add1(player_info->changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
			c2.id = (int)ChangedStatType::ATTRIBUTE;
			c2.a = a;
			c2.count = value;
		}
	}
}

//=================================================================================================
void PlayerController::TrainMove(float dist)
{
	move_tick += dist;
	if(move_tick >= 100.f)
	{
		float r = floor(move_tick / 100);
		move_tick -= r * 100;
		Train(TrainWhat::Move, r, 0);
	}
}

//=================================================================================================
void PlayerController::Rest(int days, bool resting, bool travel)
{
	// update effects that work for days, end other
	float natural_mod;
	float prev_hp = unit->hp,
		prev_stamina = unit->stamina;
	unit->EndEffects(days, &natural_mod);

	// regenerate hp
	if(Net::IsLocal() && unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(AttributeId::END);
		if(resting)
			heal *= 2;
		heal *= natural_mod * days;
		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;

		Train(TrainWhat::NaturalHealing, heal / unit->hpmax, 0);
	}

	// send update
	if(Net::IsServer() && !travel)
	{
		if(unit->hp != prev_hp)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = unit;
		}

		if(!is_local)
		{
			if(unit->stamina != prev_stamina)
				player_info->update_flags |= PlayerInfo::UF_STAMINA;
			NetChangePlayer& c = Add1(player_info->changes);
			c.type = NetChangePlayer::ON_REST;
			c.count = days;
		}
	}

	// reset last damage
	last_dmg = 0;
	last_dmg_poison = 0;
	dmgc = 0;
	poison_dmgc = 0;

	// reset action
	action_cooldown = 0;
	action_recharge = 0;
	action_charges = GetAction().charges;
}

//=================================================================================================
void PlayerController::Save(FileWriter& f)
{
	if(recalculate_level)
	{
		RecalculateLevel();
		recalculate_level = false;
	}

	f << name;
	f << move_tick;
	f << last_dmg;
	f << last_dmg_poison;
	f << dmgc;
	f << poison_dmgc;
	f << idle_timer;
	for(StatData& stat : attrib)
	{
		f << stat.points;
		f << stat.train;
		f << stat.apt;
		f << stat.train_part;
		f << stat.blocked;
	}
	for(StatData& stat : skill)
	{
		f << stat.points;
		f << stat.train;
		f << stat.apt;
		f << stat.train_part;
	}
	f << action_key;
	NextAction saved_next_action = next_action;
	if(Any(saved_next_action, NA_SELL, NA_PUT, NA_GIVE))
		saved_next_action = NA_NONE; // inventory is closed, don't save this next action
	f << saved_next_action;
	switch(saved_next_action)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f << next_action_data.slot;
		break;
	case NA_EQUIP:
	case NA_CONSUME:
		f << next_action_data.index;
		f << next_action_data.item->id;
		break;
	case NA_USE:
		f << next_action_data.usable->refid;
		break;
	default:
		assert(0);
		break;
	}
	f << last_weapon;
	f << credit;
	f << godmode;
	f << noclip;
	f << id;
	f << free_days;
	f << kills;
	f << knocks;
	f << dmg_done;
	f << dmg_taken;
	f << arena_fights;
	f << learning_points;
	f << (byte)perks.size();
	for(TakenPerk& tp : perks)
	{
		f << (byte)tp.perk;
		f << tp.value;
	}
	f << action_cooldown;
	f << action_recharge;
	f << (byte)action_charges;
	if(unit->action == A_DASH && unit->animation_state == 1)
	{
		f << action_targets.size();
		for(auto target : action_targets)
			f << target->refid;
	}
	f << split_gold;
	f << always_run;
	f << exp;
	f << exp_level;
	f << last_ring;
}

//=================================================================================================
void PlayerController::Load(FileReader& f)
{
	if(LOAD_VERSION < V_0_7)
		f.Skip<Class>(); // old class info
	f >> name;
	f >> move_tick;
	f >> last_dmg;
	f >> last_dmg_poison;
	f >> dmgc;
	f >> poison_dmgc;
	f >> idle_timer;
	if(LOAD_VERSION >= V_0_8)
	{
		for(StatData& stat : attrib)
		{
			f >> stat.points;
			f >> stat.train;
			f >> stat.apt;
			f >> stat.train_part;
			f >> stat.blocked;
		}
		for(StatData& stat : skill)
		{
			f >> stat.points;
			f >> stat.train;
			f >> stat.apt;
			f >> stat.train_part;
			stat.blocked = false;
		}
		SetRequiredPoints();
	}
	else if(LOAD_VERSION >= V_0_4)
	{
		for(StatData& stat : attrib)
		{
			f >> stat.points;
			stat.train = 0;
			stat.apt = 0;
			stat.train_part = 0;
			stat.blocked = false;
		}
		for(StatData& stat : skill)
		{
			f >> stat.points;
			stat.train = 0;
			stat.apt = 0;
			stat.train_part = 0;
			stat.blocked = false;
		}
	}
	else
	{
		for(StatData& stat : attrib)
		{
			stat.points = 0;
			stat.train = 0;
			stat.apt = 0;
			stat.train_part = 0;
			stat.blocked = false;
		}
		for(StatData& stat : skill)
		{
			stat.points = 0;
			stat.train = 0;
			stat.apt = 0;
			stat.train_part = 0;
			stat.blocked = false;
		}
		// skill points
		int old_sp[(int)old::SkillId::MAX];
		f >> old_sp;
		skill[(int)SkillId::ONE_HANDED_WEAPON].points = old_sp[(int)old::SkillId::WEAPON];
		skill[(int)SkillId::BOW].points = old_sp[(int)old::SkillId::BOW];
		skill[(int)SkillId::SHIELD].points = old_sp[(int)old::SkillId::SHIELD];
		skill[(int)SkillId::LIGHT_ARMOR].points = old_sp[(int)old::SkillId::LIGHT_ARMOR];
		skill[(int)SkillId::HEAVY_ARMOR].points = old_sp[(int)old::SkillId::HEAVY_ARMOR];
		// skip skill need
		f.Skip(5 * sizeof(int));
		// attribute points (str, end, dex)
		for(int i = 0; i < 3; ++i)
			f >> attrib[i].points;
		// skip attribute need
		f.Skip(3 * sizeof(int));

		// skip old version trainage data
		// __int64 spg[(int)SkillId::MAX][T_MAX], apg[(int)AttributeId::MAX][T_MAX];
		// T_MAX = 4
		// SkillId::MAX = 5
		// AttributeId::MAX = 3
		// size = sizeof(__int64) * T_MAX * (S_MAX + A_MAX)
		// size = 8 * 4 * (5 + 3) = 256
		f.Skip(256);

		// SetRequiredPoints called from Unit::Load after setting new attributes/skills
	}
	f >> action_key;
	f >> next_action;
	if(LOAD_VERSION < V_0_7_1)
	{
		next_action = NA_NONE;
		f.Skip<int>();
	}
	else
	{
		switch(next_action)
		{
		case NA_NONE:
			break;
		case NA_REMOVE:
		case NA_DROP:
			f >> next_action_data.slot;
			break;
		case NA_EQUIP:
		case NA_CONSUME:
			f >> next_action_data.index;
			next_action_data.item = Item::Get(f.ReadString1());
			break;
		case NA_USE:
			Usable::AddRequest(&next_action_data.usable, f.Read<int>(), nullptr);
			break;
		case NA_SELL:
		case NA_PUT:
		case NA_GIVE:
		default:
			assert(0);
			next_action = NA_NONE;
			break;
		}
	}
	f >> last_weapon;
	f >> credit;
	f >> godmode;
	f >> noclip;
	f >> id;
	f >> free_days;
	f >> kills;
	f >> knocks;
	f >> dmg_done;
	f >> dmg_taken;
	f >> arena_fights;
	if(LOAD_VERSION >= V_0_4)
	{
		if(LOAD_VERSION < V_0_8)
		{
			UnitStats old_stats;
			old_stats.Load(f);
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				attrib[i].apt = (old_stats.attrib[i] - 50) / 5;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				skill[i].apt = old_stats.skill[i] / 5;
			f.Skip(sizeof(StatState) * ((int)AttributeId::MAX + (int)SkillId::MAX)); // old stat state
		}
		// perk points
		if(LOAD_VERSION >= V_0_8)
			f >> learning_points;
		else
			learning_points = 0;
		// perks
		byte count;
		f >> count;
		perks.resize(count);
		for(TakenPerk& tp : perks)
		{
			tp.perk = (Perk)f.Read<byte>();
			f >> tp.value;
		}
		// translate perks
		if(LOAD_VERSION < V_0_8)
		{
			LoopAndRemove(perks, [this](TakenPerk& tp)
			{
				switch((old::Perk)tp.perk)
				{
				case old::Perk::Weakness:
					{
						switch((AttributeId)tp.value)
						{
						case AttributeId::STR:
							tp.perk = Perk::BadBack;
							break;
						case AttributeId::DEX:
							tp.perk = Perk::Sluggish;
							break;
						case AttributeId::END:
							tp.perk = Perk::ChronicDisease;
							break;
						case AttributeId::INT:
							tp.perk = Perk::SlowLearner;
							break;
						case AttributeId::WIS:
							return true;
						case AttributeId::CHA:
							tp.perk = Perk::Asocial;
							break;
						}
						PerkContext ctx(this, false);
						ctx.upgrade = true;
						tp.Apply(ctx);
						tp.value = 0;
					}
					break;
				case old::Perk::Strength:
					tp.perk = Perk::Talent;
					break;
				case old::Perk::Skilled:
					tp.perk = Perk::Skilled;
					break;
				case old::Perk::SkillFocus:
					return true;
				case old::Perk::Talent:
					tp.perk = Perk::SkillFocus;
					break;
				case old::Perk::AlchemistApprentice:
					tp.perk = Perk::AlchemistApprentice;
					break;
				case old::Perk::Wealthy:
					tp.perk = Perk::Wealthy;
					break;
				case old::Perk::VeryWealthy:
					tp.perk = Perk::VeryWealthy;
					break;
				case old::Perk::FamilyHeirloom:
					tp.perk = Perk::FamilyHeirloom;
					break;
				case old::Perk::Leader:
					tp.perk = Perk::Leader;
					break;
				}
				return false;
			});
		}
	}
	else
		learning_points = 0;
	if(LOAD_VERSION >= V_0_5)
	{
		f >> action_cooldown;
		f >> action_recharge;
		action_charges = f.Read<byte>();
		if(unit->action == A_DASH && unit->animation_state == 1)
		{
			uint count;
			f >> count;
			action_targets.resize(count);
			for(uint i = 0; i < count; ++i)
			{
				int refid;
				f >> refid;
				Unit::AddRequest(&action_targets[i], refid);
			}
		}
	}
	else
	{
		action_cooldown = 0.f;
		action_recharge = 0.f;
		action_charges = GetAction().charges;
	}
	if(LOAD_VERSION >= V_0_7)
		f >> split_gold;
	else
		split_gold = 0.f;
	if(LOAD_VERSION >= V_0_8)
	{
		f >> always_run;
		f >> exp;
		f >> exp_level;
	}
	else
	{
		always_run = true;
		exp = 0;
		exp_level = 0;
	}
	exp_need = GetExpNeed();
	if(LOAD_VERSION >= V_DEV)
		f >> last_ring;
	else
		last_ring = false;

	action = Action_None;
}

//=================================================================================================
void PlayerController::SetRequiredPoints()
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		attrib[i].next = GetRequiredAttributePoints(unit->stats->attrib[i]);
	for(int i = 0; i < (int)SkillId::MAX; ++i)
		skill[i].next = GetRequiredSkillPoints(unit->stats->skill[i]);
}

//=================================================================================================
int PlayerController::CalculateLevel()
{
	float level = 0.f;
	UnitStats& stats = *unit->stats;

	// only str, end & dex is currently important for players
	for(int i = 0; i < 3; ++i)
		level += float(stats.attrib[i] - 25) / 5;

	// tag 4 most important player skill from currently 5 (weapon, one handed, armor, bow, shield)
	SkillId skills[5] = {
		WeaponTypeInfo::info[unit->GetBestWeaponType()].skill,
		GetArmorTypeSkill(unit->GetBestArmorType()),
		SkillId::ONE_HANDED_WEAPON,
		SkillId::BOW,
		SkillId::SHIELD
	};
	int values[5];
	for(int i = 0; i < 5; ++i)
		values[i] = stats.skill[(int)skills[i]];
	std::sort(values, values + 5, std::greater<int>());
	for(int i = 0; i < 4; ++i)
		level += float(stats.skill[i]) / 5;

	return int(level / 7);
}

//=================================================================================================
void PlayerController::RecalculateLevel()
{
	int level = CalculateLevel();
	if(level != unit->level)
	{
		unit->level = level;
		if(Net::IsLocal() && !L.entering)
		{
			Team.CalculatePlayersLevel();
			if(player_info && !IsLocal())
				player_info->update_flags |= PlayerInfo::UF_LEVEL;
		}
	}
}

const float level_mod[21] = {
	0.0f, // -10
	0.1f, // -9
	0.2f, // -8
	0.3f, // -7
	0.4f, // -6
	0.5f, // -5
	0.6f, // -4
	0.7f, // -3
	0.8f, // -2
	0.9f, // -1
	1.f, // 0
	1.1f, // 1
	1.2f, // 2
	1.3f, //3
	1.4f, // 4
	1.5f, // 5
	1.6f, // 6
	1.7f, // 7
	1.8f, // 8
	1.9f, // 9
	2.0f, // 10
};

inline float GetLevelMod(int my_level, int target_level)
{
	return level_mod[Clamp(target_level - my_level + 10, 0, 20)];
}

//=================================================================================================
void PlayerController::Train(TrainWhat what, float value, int level)
{
	switch(what)
	{
	case TrainWhat::TakeDamage:
		TrainMod(AttributeId::END, value * 3500 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::NaturalHealing:
		TrainMod(AttributeId::END, value * 1000);
		break;
	case TrainWhat::TakeDamageArmor:
		if(unit->HaveArmor())
			TrainMod(unit->GetArmor().GetSkill(), value * 3000 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::AttackStart:
		{
			const float c_points = 50.f;
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();

			int skill = unit->GetBase(info.skill);
			if(skill < 25)
				TrainMod2(info.skill, c_points);
			else if(skill < 50)
				TrainMod2(info.skill, c_points / 2);

			skill = unit->GetBase(SkillId::ONE_HANDED_WEAPON);
			if(skill < 25)
				TrainMod2(SkillId::ONE_HANDED_WEAPON, c_points);
			else if(skill < 50)
				TrainMod2(SkillId::ONE_HANDED_WEAPON, c_points / 2);

			int str = unit->Get(AttributeId::STR);
			if(weapon.req_str > str)
				TrainMod(AttributeId::STR, c_points);
			else if(weapon.req_str + 10 > str)
				TrainMod(AttributeId::STR, c_points / 2);

			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackNoDamage:
		{
			const float c_points = 200.f * GetLevelMod(unit->level, level);
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod2(SkillId::ONE_HANDED_WEAPON, c_points);
			TrainMod2(info.skill, c_points);
			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackHit:
		{
			const float c_points = (200.f + 2300.f * value) * GetLevelMod(unit->level, level);
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod2(SkillId::ONE_HANDED_WEAPON, c_points);
			TrainMod2(info.skill, c_points);
			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::BlockBullet:
	case TrainWhat::BlockAttack:
		TrainMod(SkillId::SHIELD, value * 3000 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BashStart:
		{
			int str = unit->Get(AttributeId::STR);
			if(unit->GetShield().req_str > str)
				TrainMod(AttributeId::STR, 50.f);
			else if(unit->GetShield().req_str + 10 > str)
				TrainMod(AttributeId::STR, 25.f);
			int skill = unit->GetBase(SkillId::SHIELD);
			if(skill < 25)
				TrainMod(SkillId::SHIELD, 50.f);
			else if(skill < 50)
				TrainMod(SkillId::SHIELD, 25.f);
		}
		break;
	case TrainWhat::BashNoDamage:
		TrainMod(SkillId::SHIELD, 200.f * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BashHit:
		TrainMod(SkillId::SHIELD, (200.f + value * 2300.f) * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BowStart:
		{
			int str = unit->Get(AttributeId::STR);
			if(unit->GetBow().req_str > str)
				TrainMod(AttributeId::STR, 50.f);
			else if(unit->GetBow().req_str + 10 > str)
				TrainMod(AttributeId::STR, 25.f);
			int skill = unit->GetBase(SkillId::BOW);
			if(skill < 25)
				TrainMod(SkillId::BOW, 50.f);
			else if(skill < 50)
				TrainMod(SkillId::BOW, 25.f);
		}
		break;
	case TrainWhat::BowNoDamage:
		TrainMod(SkillId::BOW, 200.f * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BowAttack:
		TrainMod(SkillId::BOW, (200.f + 2300.f * value) * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::Move:
		{
			int dex, str;
			switch(unit->GetLoadState())
			{
			case Unit::LS_NONE:
				dex = 100;
				str = 0;
				break;
			case Unit::LS_LIGHT:
				dex = 50;
				str = 50;
				break;
			case Unit::LS_MEDIUM:
				dex = 25;
				str = 100;
				break;
			case Unit::LS_HEAVY:
				dex = 10;
				str = 200;
				break;
			case Unit::LS_OVERLOADED:
			default:
				dex = 0;
				str = 250;
				break;
			}
			TrainMod(AttributeId::STR, (float)str);
			TrainMod(AttributeId::DEX, (float)dex);

			if(unit->HaveArmor())
			{
				const Armor& armor = unit->GetArmor();
				int str = unit->Get(AttributeId::STR);
				if(armor.req_str > str)
					TrainMod(AttributeId::STR, 250.f);
				else if(armor.req_str + 10 > str)
					TrainMod(AttributeId::STR, 125.f);
				SkillId s = armor.GetSkill();
				int skill = unit->GetBase(s);
				if(skill < 25)
					TrainMod(s, 250.f);
				else if(skill < 50)
					TrainMod(s, 125.f);
			}
		}
		break;
	case TrainWhat::Talk:
		TrainMod(AttributeId::CHA, 10.f);
		break;
	case TrainWhat::Trade:
		TrainMod(SkillId::HAGGLE, value);
		break;
	case TrainWhat::Stamina:
		TrainMod(AttributeId::END, value * 0.75f);
		TrainMod(AttributeId::DEX, value * 0.5f);
		break;
	case TrainWhat::BullsCharge:
		TrainMod(AttributeId::STR, 200.f * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::Dash:
		TrainMod(AttributeId::DEX, 50.f);
		break;
	default:
		assert(0);
		break;
	}
}

float GetAptitudeMod(int apt)
{
	if(apt < 0)
		return 1.f - 0.1f * apt;
	else if(apt == 0)
		return 1.f;
	else
		return 1.f + 0.2f * apt;
}

//=================================================================================================
void PlayerController::TrainMod(AttributeId a, float points)
{
	float mod = GetAptitudeMod(attrib[(int)a].apt);
	Train(a, mod * points);
}

//=================================================================================================
void PlayerController::TrainMod2(SkillId s, float points)
{
	float mod = GetAptitudeMod(GetAptitude(s));
	Train(s, mod * points);
}

//=================================================================================================
void PlayerController::TrainMod(SkillId s, float points)
{
	TrainMod2(s, points);
	Skill& info = Skill::skills[(int)s];
	if(info.attrib2 != AttributeId::NONE)
	{
		points /= 2;
		TrainMod(info.attrib2, points);
	}
	TrainMod(info.attrib, points);
}

//=================================================================================================
void PlayerController::Train(bool is_skill, int id, TrainMode mode)
{
	StatData* stat;
	int value;
	if(is_skill)
	{
		stat = &skill[id];
		if(unit->stats->skill[id] == Skill::MAX)
		{
			stat->points = stat->next;
			return;
		}
		value = unit->stats->skill[id];
	}
	else
	{
		stat = &attrib[id];
		if(unit->stats->attrib[id] == Attribute::MAX)
		{
			stat->points = stat->next;
			return;
		}
		value = unit->stats->attrib[id];
	}

	int count;
	if(mode == TrainMode::Normal)
		count = (9 + stat->apt) / 2 - value / 20;
	else if(mode == TrainMode::Potion)
		count = 1;
	else
		count = (12 + stat->apt) / 2 - value / 24;

	if(count >= 1)
	{
		value += count;
		stat->points /= 2;

		if(is_skill)
		{
			stat->next = GetRequiredSkillPoints(value);
			unit->Set((SkillId)id, value);
		}
		else
		{
			stat->next = GetRequiredAttributePoints(value);
			unit->Set((AttributeId)id, value);
		}

		Game::Get().gui->messages->AddFormattedMessage(this, is_skill ? GMS_GAIN_SKILL : GMS_GAIN_ATTRIBUTE, id, count);

		if(!IsLocal())
		{
			NetChangePlayer& c2 = Add1(player_info->changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
			c2.id = int(is_skill ? ChangedStatType::SKILL : ChangedStatType::ATTRIBUTE);
			c2.a = id;
			c2.count = value;
		}
	}
	else
	{
		float m;
		if(count == 0)
			m = 0.5f;
		else if(count == -1)
			m = 0.25f;
		else
			m = 0.125f;
		float pts = m * stat->next;
		if(is_skill)
			TrainMod2((SkillId)id, pts);
		else
			TrainMod((AttributeId)id, pts);
	}
}

//=================================================================================================
// Used to send per-player data in WritePlayerData
void PlayerController::Write(BitStreamWriter& f) const
{
	f << kills;
	f << dmg_done;
	f << dmg_taken;
	f << knocks;
	f << arena_fights;
	f << learning_points;
	f.WriteCasted<byte>(perks.size());
	for(const TakenPerk& perk : perks)
	{
		f.WriteCasted<byte>(perk.perk);
		f << perk.value;
	}
	f << action_cooldown;
	f << action_recharge;
	f << action_charges;
	f << always_run;
	f.WriteCasted<byte>(next_action);
	switch(next_action)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f.WriteCasted<byte>(next_action_data.slot);
		break;
	case NA_EQUIP:
	case NA_CONSUME:
		f << GetNextActionItemIndex();
		break;
	case NA_USE:
		f << next_action_data.usable->netid;
		break;
	default:
		assert(0);
		break;
	}
	f << last_ring;
}

//=================================================================================================
// Used to read per-player data in ReadPlayerData
bool PlayerController::Read(BitStreamReader& f)
{
	byte count;
	f >> kills;
	f >> dmg_done;
	f >> dmg_taken;
	f >> knocks;
	f >> arena_fights;
	f >> learning_points;
	f >> count;
	if(!f || !f.Ensure(5 * count))
		return false;
	perks.resize(count);
	for(TakenPerk& perk : perks)
	{
		f.ReadCasted<byte>(perk.perk);
		f >> perk.value;
	}
	f >> action_cooldown;
	f >> action_recharge;
	f >> action_charges;
	f >> always_run;
	f.ReadCasted<byte>(next_action);
	if(!f)
		return false;
	switch(next_action)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f.ReadCasted<byte>(next_action_data.slot);
		if(!f)
			return false;
		break;
	case NA_EQUIP:
	case NA_CONSUME:
		{
			f >> next_action_data.index;
			if(!f)
				return false;
			if(next_action_data.index == -1)
				next_action = NA_NONE;
			else
				next_action_data.item = unit->items[next_action_data.index].item;
		}
		break;
	case NA_USE:
		{
			int index = f.Read<int>();
			if(!f)
				return false;
			next_action_data.usable = L.FindUsable(index);
		}
		break;
	default:
		assert(0);
		break;
	}
	f >> last_ring;
	return true;
}

//=================================================================================================
bool PlayerController::IsLeader() const
{
	return Team.IsLeader(unit);
}

//=================================================================================================
Action& PlayerController::GetAction()
{
	auto action = ClassInfo::classes[(int)unit->GetClass()].action;
	assert(action);
	return *action;
}

//=================================================================================================
bool PlayerController::UseActionCharge()
{
	if (action_charges == 0 || action_cooldown > 0)
		return false;
	auto& action = GetAction();
	--action_charges;
	action_cooldown = action.cooldown;
	if(action_recharge == 0)
		action_recharge = action.recharge;
	return true;
}

//=================================================================================================
void PlayerController::RefreshCooldown()
{
	auto& action = GetAction();
	action_cooldown = 0;
	action_recharge = 0;
	action_charges = action.charges;
}

//=================================================================================================
bool PlayerController::IsHit(Unit* unit) const
{
	return IsInside(action_targets, unit);
}

//=================================================================================================
int PlayerController::GetNextActionItemIndex() const
{
	return FindItemIndex(unit->items, next_action_data.index, next_action_data.item, false);
}

//=================================================================================================
void PlayerController::AddItemMessage(uint count)
{
	assert(count != 0u);
	if(IsLocal())
	{
		Game& game = Game::Get();
		if(count == 1u)
			game.gui->messages->AddGameMsg3(GMS_ADDED_ITEM);
		else
			game.gui->messages->AddGameMsg(Format(game.txGmsAddedItems, count), 3.f);
	}
	else
	{
		NetChangePlayer& c2 = Add1(player_info->changes);
		if(count == 1u)
		{
			c2.type = NetChangePlayer::GAME_MESSAGE;
			c2.id = GMS_ADDED_ITEM;
		}
		else
		{
			c2.type = NetChangePlayer::ADDED_ITEMS_MSG;
			c2.count = count;
		}
	}
}

//=================================================================================================
void PlayerController::PayCredit(int count)
{
	LocalVector<Unit*> units;
	for(Unit* u : Team.active_members)
	{
		if(u != unit)
			units->push_back(u);
	}

	Team.AddGold(count, units);

	credit -= count;
	if(credit < 0)
	{
		Warn("Player '%s' paid %d credit and now have %d!", name.c_str(), count, credit);
		credit = 0;
	}

	if(Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

//=================================================================================================
void PlayerController::UseDays(int count)
{
	assert(count > 0);

	if(free_days >= count)
		free_days -= count;
	else
	{
		count -= free_days;
		free_days = 0;

		for(auto info : N.players)
		{
			if(info->left == PlayerInfo::LEFT_NO && info->pc != this)
				info->pc->free_days += count;
		}

		W.Update(count, World::UM_NORMAL);
	}

	Net::PushChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
void PlayerController::StartDialog(Unit* talker, GameDialog* dialog)
{
	assert(talker);

	DialogContext& ctx = *dialog_ctx;
	assert(!ctx.dialog_mode);

	if(!is_local)
	{
		NetChangePlayer& c = Add1(player_info->changes);
		c.type = NetChangePlayer::START_DIALOG;
		c.id = talker->netid;
	}

	ctx.StartDialog(talker, dialog);
}

//=================================================================================================
bool PlayerController::HavePerk(Perk perk, int value)
{
	for(TakenPerk& tp : perks)
	{
		if(tp.perk == perk && tp.value == value)
			return true;
	}
	return false;
}

//=================================================================================================
bool PlayerController::HavePerkS(const string& perk_id)
{
	PerkInfo* perk = PerkInfo::Find(perk_id);
	if(!perk)
		throw ScriptException("Invalid perk '%s'.", perk_id.c_str());
	return HavePerk(perk->perk_id);
}

//=================================================================================================
bool PlayerController::AddPerk(Perk perk, int value)
{
	if(HavePerk(perk, value))
		return false;
	TakenPerk tp(perk, value);
	perks.push_back(tp);
	PerkContext ctx(this, false);
	tp.Apply(ctx);
	if(Net::IsServer() && !IsLocal())
	{
		NetChangePlayer& c = Add1(player_info->changes);
		c.type = NetChangePlayer::ADD_PERK;
		c.id = (int)perk;
		c.count = value;
	}
	return true;
}

//=================================================================================================
bool PlayerController::RemovePerk(Perk perk, int value)
{
	for(auto it = perks.begin(), end = perks.end(); it != end; ++it)
	{
		TakenPerk& tp = *it;
		if(tp.perk == perk && tp.value == value)
		{
			TakenPerk tp_copy = tp;
			perks.erase(it);
			PerkContext ctx(this, false);
			tp_copy.Remove(ctx);
			if(Net::IsServer() && !IsLocal())
			{
				NetChangePlayer& c = Add1(player_info->changes);
				c.type = NetChangePlayer::REMOVE_PERK;
				c.id = (int)perk;
				c.count = value;
			}
			return true;
		}
	}
	return false;
}

//=================================================================================================
void PlayerController::AddLearningPoint(int count)
{
	learning_points += count;
	Game::Get().gui->messages->AddFormattedMessage(this, GMS_GAIN_LEARNING_POINTS, -1, count);
	if(!is_local)
		player_info->update_flags |= PlayerInfo::UF_LEARNING_POINTS;
}

//=================================================================================================
void PlayerController::AddExp(int xp)
{
	exp += xp;
	int points = 0;
	while(exp >= exp_need)
	{
		++points;
		++exp_level;
		exp -= exp_need;
		exp_need = GetExpNeed();
	}
	if(points > 0)
		AddLearningPoint(points);
}

//=================================================================================================
int PlayerController::GetExpNeed() const
{
	int exp_per_level = 1000 - (unit->GetBase(AttributeId::INT) - 50);
	return exp_per_level * (exp_level + 1);
}

//=================================================================================================
int PlayerController::GetAptitude(SkillId s)
{
	int apt = skill[(int)s].apt;
	SkillType skill_type = Skill::skills[(int)s].type;
	if(skill_type != SkillType::NONE)
	{
		int value = unit->GetBase(s);
		// cross training bonus
		if(skill_type == SkillType::WEAPON)
		{
			SkillId best = WeaponTypeInfo::info[unit->GetBestWeaponType()].skill;
			if(best != s && value - 1 > unit->GetBase(best))
				apt += 5;
		}
		else if(skill_type == SkillType::ARMOR)
		{
			SkillId best = GetArmorTypeSkill(unit->GetBestArmorType());
			if(best != s && value - 1 > unit->GetBase(best))
				apt += 5;
		}
	}
	return apt;
}

//=================================================================================================
int PlayerController::GetTrainCost(int train) const
{
	switch(train)
	{
	case 0:
	case 1:
		return 1;
	case 2:
	case 3:
	case 4:
		return 2;
	case 5:
	case 6:
	case 7:
	case 8:
		return 3;
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		return 4;
	default:
		return 5;
	}
}
