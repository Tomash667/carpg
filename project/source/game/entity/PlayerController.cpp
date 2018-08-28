#include "Pch.h"
#include "GameCore.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Class.h"
#include "Action.h"

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

	ResetStatState();

	if(!partial)
	{
		kills = 0;
		dmg_done = 0;
		dmg_taken = 0;
		knocks = 0;
		arena_fights = 0;

		for(int i = 0; i < (int)SkillId::MAX; ++i)
			sp[i] = 0;
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			ap[i] = 0;

		action_charges = GetAction().charges;
	}
}

//=================================================================================================
void PlayerController::ResetStatState()
{
	// currently it isn't working
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		attrib_state[i] = StatState::NORMAL;
	for(int i = 0; i < (int)SkillId::MAX; ++i)
		skill_state[i] = StatState::NORMAL;
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
void PlayerController::Train(SkillId skill, int points)
{
	int s = (int)skill;

	sp[s] += points;

	int gained = 0,
		value = unit->GetUnmod(skill);
	//base = base_stats.skill[s];

	while(sp[s] >= sn[s])
	{
		sp[s] -= sn[s];
		if(value != Skill::MAX)
		{
			++gained;
			++value;
			sn[s] = GetRequiredSkillPoints(value);
		}
		else
		{
			sp[s] = sn[s];
			break;
		}
	}

	if(gained)
	{
		recalculate_level = true;
		unit->Set(skill, value);
		Game& game = Game::Get();
		if(is_local)
			game.ShowStatGain(true, s, gained);
		else
		{
			NetChangePlayer& c = game.AddChange(NetChangePlayer::GAIN_STAT, this);
			c.id = 1;
			c.a = s;
			c.ile = gained;

			NetChangePlayer& c2 = game.AddChange(NetChangePlayer::STAT_CHANGED, this);
			c2.id = (int)ChangedStatType::SKILL;
			c2.a = s;
			c2.ile = value;
		}
	}
}

//=================================================================================================
void PlayerController::Train(AttributeId attrib, int points)
{
	int a = (int)attrib;

	ap[a] += points;

	int gained = 0,
		value = unit->GetUnmod(attrib);
	//base = base_stats.attrib[a];

	while(ap[a] >= an[a])
	{
		ap[a] -= an[a];
		if(unit->stats.attrib[a] != Attribute::MAX)
		{
			++gained;
			++value;
			an[a] = GetRequiredAttributePoints(value);
		}
		else
		{
			ap[a] = an[a];
			break;
		}
	}

	if(gained)
	{
		recalculate_level = true;
		unit->Set(attrib, value);
		Game& game = Game::Get();
		if(is_local)
			game.ShowStatGain(false, a, gained);
		else
		{
			NetChangePlayer& c = game.AddChange(NetChangePlayer::GAIN_STAT, this);
			c.id = 0;
			c.a = a;
			c.ile = gained;

			NetChangePlayer& c2 = game.AddChange(NetChangePlayer::STAT_CHANGED, this);
			c2.id = (int)ChangedStatType::ATTRIBUTE;
			c2.a = a;
			c2.ile = value;
		}
	}
}

//=================================================================================================
void PlayerController::TrainMove(float dt, bool run)
{
	move_tick += (run ? dt : dt / 10);
	if(move_tick >= 1.f)
	{
		move_tick -= 1.f;
		Train(TrainWhat::Move, 0.f, 0);
	}
}

//=================================================================================================
void PlayerController::TravelTick()
{
	Rest(1, false, true);
	Train(TrainWhat::Move, 0.f, 0);
}

//=================================================================================================
void PlayerController::Rest(int days, bool resting, bool travel)
{
	// update effects that work for days, end other
	int best_nat;
	float prev_hp = unit->hp,
		prev_stamina = unit->stamina;
	unit->EndEffects(days, &best_nat);

	// regenerate hp
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(AttributeId::END);
		if(resting)
			heal *= 2;
		if(best_nat)
		{
			if(best_nat != days)
				heal = heal*best_nat * 2 + heal*(days - best_nat);
			else
				heal *= 2 * days;
		}
		else
			heal *= days;

		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;

		Train(AttributeId::END, int(heal));
	}

	// send update
	if(Net::IsOnline() && !travel)
	{
		if(unit->hp != prev_hp)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = unit;
		}

		if(unit->stamina != prev_stamina && !is_local)
			player_info->update_flags |= PlayerInfo::UF_STAMINA;
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
	f << ap;
	f << sp;
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
	base_stats.Save(f);
	f << attrib_state;
	f << skill_state;
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
	if(LOAD_VERSION >= V_0_4)
	{
		// attribute points
		f >> ap;
		// skill points
		f >> sp;

		SetRequiredPoints();
	}
	else
	{
		// skill points
		int old_sp[(int)OldSkill::MAX];
		f >> old_sp;
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			sp[i] = 0;
		sp[(int)SkillId::ONE_HANDED_WEAPON] = old_sp[(int)OldSkill::WEAPON];
		sp[(int)SkillId::BOW] = old_sp[(int)OldSkill::BOW];
		sp[(int)SkillId::SHIELD] = old_sp[(int)OldSkill::SHIELD];
		sp[(int)SkillId::LIGHT_ARMOR] = old_sp[(int)OldSkill::LIGHT_ARMOR];
		sp[(int)SkillId::HEAVY_ARMOR] = old_sp[(int)OldSkill::HEAVY_ARMOR];
		// skip skill need
		f.Skip(5 * sizeof(int));
		// attribute points (str, end, dex)
		for(int i = 0; i < 3; ++i)
			f >> ap[i];
		for(int i = 3; i < 6; ++i)
			ap[i] = 0;
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
		base_stats.Load(f);
		f >> attrib_state;
		f >> skill_state;
		// perks
		byte count;
		f >> count;
		perks.resize(count);
		for(TakenPerk& tp : perks)
		{
			tp.perk = (Perk)f.Read<byte>();
			f >> tp.value;
		}
	}
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
	if(LOAD_VERSION < V_0_5_1)
		ResetStatState();
	if(LOAD_VERSION >= V_0_7)
		f >> split_gold;
	else
		split_gold = 0.f;
	if(LOAD_VERSION >= V_DEV)
		f >> always_run;
	else
		always_run = true;

	action = Action_None;
}

//=================================================================================================
void PlayerController::SetRequiredPoints()
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		an[i] = GetRequiredAttributePoints(unit->unmod_stats.attrib[i]);
	for(int i = 0; i < (int)SkillId::MAX; ++i)
		sn[i] = GetRequiredSkillPoints(unit->unmod_stats.skill[i]);
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
	case TrainWhat::TakeDamageArmor:
		if(unit->HaveArmor())
			TrainMod(unit->GetArmor().skill, value * 2000 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::AttackStart:
		{
			const float c_points = 50.f;
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			if(weapon.req_str > unit->Get(AttributeId::STR))
				TrainMod(AttributeId::STR, c_points);
			TrainMod(SkillId::ONE_HANDED_WEAPON, c_points);
			TrainMod2(weapon.GetInfo().skill, c_points);
			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackNoDamage:
		{
			const float c_points = 150.f;
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod(SkillId::ONE_HANDED_WEAPON, c_points);
			TrainMod2(weapon.GetInfo().skill, c_points);
			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackHit:
		{
			const float c_points = 2450.f * value;
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod(SkillId::ONE_HANDED_WEAPON, c_points);
			TrainMod2(weapon.GetInfo().skill, c_points);
			TrainMod(AttributeId::STR, c_points * info.str2dmg);
			TrainMod(AttributeId::DEX, c_points * info.dex2dmg);
		}
		break;
	case TrainWhat::BlockBullet:
	case TrainWhat::BlockAttack:
		TrainMod(SkillId::SHIELD, value * 2000 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BashStart:
		if(unit->GetShield().req_str > unit->Get(AttributeId::STR))
			TrainMod(AttributeId::STR, 50.f);
		TrainMod(SkillId::SHIELD, 50.f);
		break;
	case TrainWhat::BashNoDamage:
		TrainMod(SkillId::SHIELD, 150.f * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BashHit:
		TrainMod(SkillId::SHIELD, value * 1950 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BowStart:
		if(unit->GetBow().req_str > unit->Get(AttributeId::STR))
			TrainMod(AttributeId::STR, 50.f);
		TrainMod(SkillId::BOW, 50.f);
		break;
	case TrainWhat::BowNoDamage:
		TrainMod(SkillId::BOW, 150.f * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BowAttack:
		TrainMod(SkillId::BOW, 2450.f * value * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::Move:
		{
			int dex, str;

			switch(unit->GetLoadState())
			{
			case 0: // brak
				dex = 49;
				str = 1;
				break;
			case 1: // lekkie
				dex = 40;
				str = 10;
				break;
			case 2: // œrednie
				dex = 25;
				str = 25;
				break;
			case 3: // ciê¿kie
				dex = 10;
				str = 40;
				break;
			case 4: // maksymalne
			default:
				dex = 1;
				str = 49;
				break;
			}

			if(unit->HaveArmor())
			{
				const Armor& armor = unit->GetArmor();
				if(armor.req_str > unit->Get(AttributeId::STR))
					str += 50;
				TrainMod(armor.skill, 50.f);
			}

			TrainMod(AttributeId::STR, (float)str);
			TrainMod(AttributeId::DEX, (float)dex);
		}
		break;
	case TrainWhat::Talk:
		TrainMod(AttributeId::CHA, 10.f);
		break;
	case TrainWhat::Trade:
		TrainMod(AttributeId::CHA, 100.f);
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

//=================================================================================================
void PlayerController::TrainMod(AttributeId a, float points)
{
	Train(a, int(points * GetBaseAttributeMod(GetBase(a))));
}

//=================================================================================================
void PlayerController::TrainMod2(SkillId s, float points)
{
	Train(s, int(points * GetBaseSkillMod(GetBase(s))));
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
// Used to send per-player data in WritePlayerData
void PlayerController::Write(BitStreamWriter& f) const
{
	f << kills;
	f << dmg_done;
	f << dmg_taken;
	f << knocks;
	f << arena_fights;
	base_stats.Write(f);
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
	base_stats.Read(f);
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
			next_action_data.usable = Game::Get().FindUsable(index);
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
