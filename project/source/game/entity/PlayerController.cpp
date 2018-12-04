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

//=================================================================================================
PlayerController::~PlayerController()
{
	if(dialog_ctx && !dialog_ctx->is_local)
		delete dialog_ctx;
}

//=================================================================================================
float PlayerController::CalculateAttack() const
{
	WeaponType b;

	switch(unit->weapon_state)
	{
	case WS_HIDING:
		b = unit->weapon_hiding;
		break;
	case WS_HIDDEN:
		if(ostatnia == W_NONE)
		{
			if(unit->HaveWeapon())
				b = W_ONE_HANDED;
			else if(unit->HaveBow())
				b = W_BOW;
			else
				b = W_NONE;
		}
		else
			b = ostatnia;
		break;
	case WS_TAKING:
	case WS_TAKEN:
		b = unit->weapon_taken;
		break;
	default:
		assert(0);
		break;
	}

	if(b == W_ONE_HANDED)
	{
		if(!unit->HaveWeapon())
		{
			if(!unit->HaveBow())
				b = W_NONE;
			else
				b = W_BOW;
		}
	}
	else if(b == W_BOW)
	{
		if(!unit->HaveBow())
		{
			if(!unit->HaveWeapon())
				b = W_NONE;
			else
				b = W_ONE_HANDED;
		}
	}

	if(b == W_ONE_HANDED)
		return unit->CalculateAttack(&unit->GetWeapon());
	else if(b == W_BOW)
		return unit->CalculateAttack(&unit->GetBow());
	else
		return 0.5f * unit->Get(AttributeId::STR) + 0.5f * unit->Get(AttributeId::DEX);
}

//=================================================================================================
void PlayerController::Init(Unit& _unit, bool partial)
{
	unit = &_unit;
	move_tick = 0.f;
	ostatnia = W_NONE;
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
			unit->level = unit->CalculateLevel();
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
		if(unit->stats.attrib[a] != Attribute::MAX)
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
		Info("Train move %s", name.c_str());
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
		unit->level = unit->CalculateLevel();
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
	f << ostatnia;
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
	if(LOAD_VERSION >= V_DEV)
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
		SetRequiredPoints();
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
	f >> ostatnia;
	if(LOAD_VERSION < V_0_2_20)
		f.Skip<float>(); // old rise_timer
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
		if(LOAD_VERSION < V_DEV)
		{
			UnitStats old_stats;
			f.Read(old_stats);
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				attrib[i].apt = (old_stats.attrib[i] - 50) / 5;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				skill[i].apt = old_stats.skill[i] / 5;
			f.Skip(sizeof(StatState) * ((int)AttributeId::MAX + (int)SkillId::MAX)); // old stat state
		}
		// perk points
		if(LOAD_VERSION >= V_DEV)
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
		if(LOAD_VERSION < V_DEV)
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
	if(LOAD_VERSION >= V_DEV)
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

	action = Action_None;
}

//=================================================================================================
void PlayerController::SetRequiredPoints()
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		attrib[i].next = GetRequiredAttributePoints(unit->base_stat.attrib[i]);
	for(int i = 0; i < (int)SkillId::MAX; ++i)
		skill[i].next = GetRequiredSkillPoints(unit->base_stat.skill[i]);
}

const float level_mod[21] = {
	0.5f, // -10
	0.55f, // -9
	0.64f, // -8
	0.72f, // -7
	0.79f, // -6
	0.85f, // -5
	0.9f, // -4
	0.94f, // -3
	0.97f, // -2
	0.99f, // -1
	1.f, // 0
	1.01f, // 1
	1.03f, // 2
	1.06f, //3
	1.1f, // 4
	1.15f, // 5
	1.21f, // 6
	1.28f, // 7
	1.36f, // 8
	1.45f, // 9
	1.5f, // 10
};

inline float GetLevelMod(int my_level, int target_level)
{
	return level_mod[Clamp(my_level - target_level + 10, 0, 20)];
}

//=================================================================================================
void PlayerController::Train(TrainWhat what, float value, int level)
{
	switch(what)
	{
	case TrainWhat::TakeDamage:
		TrainMod(AttributeId::END, value * 2500 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::NaturalHealing:
		TrainMod(AttributeId::END, value * 1000);
		break;
	case TrainWhat::TakeDamageArmor:
		if(unit->HaveArmor())
			TrainMod(unit->GetArmor().skill, value * 2000 * GetLevelMod(unit->level, level));
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
		TrainMod(SkillId::SHIELD, value * 2000 * GetLevelMod(unit->level, level));
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
		TrainMod(SkillId::SHIELD, (200.f + value * 1800.f) * GetLevelMod(unit->level, level));
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
				int skill = unit->GetBase(armor.skill);
				if(skill < 25)
					TrainMod(armor.skill, 250.f);
				else if(skill < 50)
					TrainMod(armor.skill, 125.f);
			}
		}
		break;
	case TrainWhat::Talk:
		TrainMod(AttributeId::CHA, 10.f);
		break;
	case TrainWhat::Trade:
		TrainMod(SkillId::HAGGLE, value / 2);
		break;
	case TrainWhat::Stamina:
		TrainMod(AttributeId::END, value * 0.75f);
		TrainMod(AttributeId::DEX, value * 0.5f);
		break;
	default:
		assert(0);
		break;
	}
}

float GetAptitudeMod(int apt)
{
	switch(apt)
	{
	case -3:
		return 0.7f;
	case -2:
		return 0.8f;
	case -1:
		return 0.9f;
	case 0:
		return 1.f;
	case 1:
		return 1.1f;
	case 2:
		return 1.2f;
	case 3:
		return 1.3f;
	default:
		if(apt < -3)
			return 0.6f;
		else
			return 1.4f;
	}
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
	float mod = GetAptitudeMod(skill[(int)s].apt);
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
		if(unit->base_stat.skill[id] == Skill::MAX)
		{
			stat->points = stat->next;
			return;
		}
		value = unit->base_stat.skill[id];
	}
	else
	{
		stat = &attrib[id];
		if(unit->base_stat.attrib[id] == Attribute::MAX)
		{
			stat->points = stat->next;
			return;
		}
		value = unit->base_stat.attrib[id];
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
	return true;
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

	Game::Get().AddGold(count, units, false);

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
