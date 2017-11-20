#include "Pch.h"
#include "Core.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Class.h"
#include "Action.h"

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
	recalculate_level = true;
	level = 0.f;

	if(!partial)
	{
		kills = 0;
		dmg_done = 0;
		dmg_taken = 0;
		knocks = 0;
		arena_fights = 0;

		for(int i = 0; i < (int)Skill::MAX; ++i)
			sp[i] = 0;
		for(int i = 0; i < (int)Attribute::MAX; ++i)
			ap[i] = 0;

		action_charges = GetAction().charges;
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
	}

	if(Net::IsLocal())
		RecalculateLevel();

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
void PlayerController::Train(TrainWhat what, float value, int level, Skill skill)
{
	cstring s;
	switch(what)
	{
	case TrainWhat::Attack:
		s = "Attack";
		{
			if(skill == Skill::NONE)
				skill = unit->GetWeaponSkill();

			// train weapon skill
			Train(skill, 50);

			// train str when too low for weapon/bow/shield
			int req_str;
			if(skill == Skill::BOW)
			{
				auto& bow = unit->GetBow();
				req_str = bow.req_str;
			}
			else if(skill == Skill::SHIELD)
			{
				auto& shield = unit->GetShield();
				req_str = shield.req_str;
			}
			else if(skill != Skill::UNARMED)
			{
				auto& weapon = unit->GetWeapon();
				req_str = weapon.req_str;
			}
			if(req_str > unit->GetBase(Attribute::STR))
				Train(Attribute::STR, 50);

			// train dex when too low for weapon/bow
		}
		break;
	case TrainWhat::Hit:
		s = "Hit";
		{
			// value = 0 (no damage)
			// value < 1 (damage scale)
			// vale >= 1 (killing hit, scale above 1)
			float ratio = value;
			if(ratio >= 1.f)
				ratio = value - 1.f + TRAIN_KILL_RATIO;
			if(skill == Skill::NONE)
				skill = unit->GetWeaponSkill();

			// train weapon skill
			int points = (int)(Lerp(200.f, 2450.f, ratio) * GetLevelMod(unit->level, level));
			Train(skill, points);
		}
		break;
	case TrainWhat::Move:
		s = "Move";
		{
			int weight = 0;

			if(unit->HaveArmor())
			{
				auto& armor = unit->GetArmor();
				if(armor.req_str > unit->GetBase(Attribute::STR))
				{
					// train str when too low for armor
					Train(Attribute::STR, 250);
				}

				// train armor skill
				Train(armor.skill, 250);

				switch(armor.skill)
				{
				case Skill::MEDIUM_ARMOR:
					weight = 2;
					break;
				case Skill::HEAVY_ARMOR:
					weight = 3;
					break;
				}
			}

			auto load_state = unit->GetLoadState();
			if(load_state >= Unit::LS_MEDIUM)
			{
				// train str when overcarrying
				float ratio;
				switch(load_state)
				{
				case Unit::LS_MEDIUM:
					ratio = 0.5f;
					weight = max(weight, 1);
					break;
				case Unit::LS_HEAVY:
					ratio = 1.f;
					weight = max(weight, 3);
					break;
				default:
					ratio = 1.5f;
					weight = max(weight, 4);
					break;
				}
			}

			// train skills
			int acro, ath;
			switch(weight)
			{
			case 0:
				acro = 300;
				ath = 0;
				break;
			case 1:
				acro = 250;
				ath = 100;
				break;
			case 2:
				acro = 200;
				ath = 250;
				break;
			case 3:
				acro = 100;
				ath = 300;
				break;
			case 4:
				acro = 0;
				ath = 500;
				break;
			}
			if(acro > 0)
				Train(Skill::ACROBATICS, acro);
			if(ath > 0)
				Train(Skill::ATHLETICS, ath);
		}
		break;
	case TrainWhat::Block:
		s = "Block";
		{
			auto& shield = unit->GetShield();
			if(shield.req_str > unit->GetBase(Attribute::STR))
			{
				// train str when too low for shield
				Train(Attribute::STR, 50);
			}

			// train shield skill
			Train(Skill::SHIELD, (int)(value * 2000));
		}
		break;
	case TrainWhat::TakeHit:
		s = "TakeHit";
		{
			// train armor skill
			if(unit->HaveArmor())
				Train(unit->GetArmor().skill, (int)(value * 2000));

			// train shield skill / 5
			if(unit->HaveShield())
				Train(Skill::SHIELD, (int)(value * 400));
		}
		break;
	case TrainWhat::TakeDamage:
		s = "TakeDamage";
		{
			// train end
			Train(Attribute::END, (int)(value * 1250));

			// train athletics skill
			Train(Skill::ATHLETICS, (int)(value * 2500));
		}
		break;
	case TrainWhat::Regenerate:
		s = "Regenerate";
		Train(Attribute::END, (int)value);
		break;
	case TrainWhat::Stamina:
		s = "Stamina";
		Train(Attribute::END, (int)value);
		break;
	case TrainWhat::Trade:
		s = "Trade";
		Train(Skill::HAGGLE, (int)value);
		break;
	case TrainWhat::Read:
		s = "Read";
		Train(Skill::LITERACY, 2500);
		break;
	}

	cstring sk;
	if(skill == Skill::NONE)
		sk = "none";
	else
		sk = SkillInfo::skills[(int)skill].id;
	Info("Train %s (%g, %d, %s) - %s", s, value, level, sk, name.c_str());
}

//=================================================================================================
void PlayerController::Train(TrainMode mode, int what, bool is_skill)
{
	int value, *train_points, *train_next;
	if(is_skill)
	{
		if(unit->statsx->skill[what] == SkillInfo::MAX)
		{
			sp[what] = sn[what];
			return;
		}
		value = unit->statsx->skill[what];
		train_points = &sp[what];
		train_next = &sn[what];
	}
	else
	{
		if(unit->statsx->attrib[what] == AttributeInfo::MAX)
		{
			ap[what] = an[what];
			return;
		}
		value = unit->statsx->attrib[what];
		train_points = &ap[what];
		train_next = &an[what];
	}

	int count;
	if(mode == TM_NORMAL)
		count = 10 - value / 10;
	else if(mode == TM_SINGLE_POINT)
		count = 1;
	else
		count = 12 - value / 12;

	if(count >= 1)
	{
		value += count;
		*train_points /= 2;

		if(is_skill)
		{
			*train_next = GetRequiredSkillPoints(value);
			unit->Set((Skill)what, value);
		}
		else
		{
			*train_next = GetRequiredAttributePoints(value);
			unit->Set((Attribute)what, value);
		}

		if(is_local)
			Game::Get().ShowStatGain(is_skill, what, count);
		else
		{
			NetChangePlayer& c = Add1(Net::player_changes);
			c.type = NetChangePlayer::GAIN_STAT;
			c.pc = this;
			c.id = (is_skill ? 1 : 0);
			c.a = what;
			c.ile = count;

			NetChangePlayer& c2 = Add1(Net::player_changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
			c2.pc = this;
			c2.id = int(is_skill ? ChangedStatType::SKILL : ChangedStatType::ATTRIBUTE);
			c2.a = what;
			c2.ile = value;

			player_info->NeedUpdate();
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
		int pts = (int)(m * *train_next);
		if(is_skill)
			Train((Skill)what, pts, false);
		else
			Train((Attribute)what, pts);
	}
}

//=================================================================================================
void PlayerController::Train(Attribute attrib, int points)
{
	int a = (int)attrib;
	points = (int)(unit->GetAptitudeMod(attrib) * points);
	assert(points != 0);
	Info("--- %s %d", AttributeInfo::attributes[(int)attrib].name.c_str(), points);

	ap[a] += points;

	int gained = 0,
		value = unit->GetBase(attrib);

	while(ap[a] >= an[a])
	{
		ap[a] -= an[a];
		if(unit->statsx->attrib[a] != AttributeInfo::MAX)
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
void PlayerController::Train(Skill skill, int points, bool train_attrib)
{
	int s = (int)skill;
	int mod_points = (int)(unit->GetAptitudeMod(skill) * points);
	assert(mod_points != 0);
	Info(Format("--- %s %d (%d)", SkillInfo::skills[s].name.c_str(), mod_points, points));

	sp[s] += mod_points;

	int gained = 0,
		value = unit->GetBase(skill);

	while(sp[s] >= sn[s])
	{
		sp[s] -= sn[s];
		if(value != SkillInfo::MAX)
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

	if(train_attrib)
	{
		auto& info = SkillInfo::skills[s];
		if(info.similar == SimilarSkill::Weapon)
		{
			// train in one handed weapon
			Train(Skill::ONE_HANDED_WEAPON, points, false);
		}
		Train(info.attrib, (int)(info.attrib_ratio * points));
		if(info.attrib2 != Attribute::NONE)
			Train(info.attrib2, (int)((1.f - info.attrib_ratio) * points));
	}
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
		float heal = 0.5f * unit->Get(Attribute::END);
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

		Train(TrainWhat::Regenerate, heal, 0);
	}

	// send update
	Game& game = Game::Get();
	if(Net::IsOnline() && !travel)
	{
		if(unit->hp != prev_hp)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = unit;
		}

		if(unit->stamina != prev_stamina && this != game.pc)
			game.GetPlayerInfo(this).update_flags |= PlayerInfo::UF_STAMINA;
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
void PlayerController::Save(HANDLE file)
{
	RecalculateLevel();

	WriteFile(file, &clas, sizeof(clas), &tmp, nullptr);
	WriteString1(file, name);
	WriteFile(file, &level, sizeof(level), &tmp, nullptr);
	WriteFile(file, &move_tick, sizeof(move_tick), &tmp, nullptr);
	WriteFile(file, &last_dmg, sizeof(last_dmg), &tmp, nullptr);
	WriteFile(file, &last_dmg_poison, sizeof(last_dmg_poison), &tmp, nullptr);
	WriteFile(file, &dmgc, sizeof(dmgc), &tmp, nullptr);
	WriteFile(file, &poison_dmgc, sizeof(poison_dmgc), &tmp, nullptr);
	WriteFile(file, &idle_timer, sizeof(idle_timer), &tmp, nullptr);
	WriteFile(file, ap, sizeof(ap), &tmp, nullptr);
	WriteFile(file, sp, sizeof(sp), &tmp, nullptr);
	WriteFile(file, &action_key, sizeof(action_key), &tmp, nullptr);
	WriteFile(file, &next_action, sizeof(next_action), &tmp, nullptr);
	WriteFile(file, &next_action_idx, sizeof(next_action_idx), &tmp, nullptr);
	WriteFile(file, &ostatnia, sizeof(ostatnia), &tmp, nullptr);
	WriteFile(file, &credit, sizeof(credit), &tmp, nullptr);
	WriteFile(file, &godmode, sizeof(godmode), &tmp, nullptr);
	WriteFile(file, &noclip, sizeof(noclip), &tmp, nullptr);
	WriteFile(file, &id, sizeof(id), &tmp, nullptr);
	WriteFile(file, &free_days, sizeof(free_days), &tmp, nullptr);
	WriteFile(file, &kills, sizeof(kills), &tmp, nullptr);
	WriteFile(file, &knocks, sizeof(knocks), &tmp, nullptr);
	WriteFile(file, &dmg_done, sizeof(dmg_done), &tmp, nullptr);
	WriteFile(file, &dmg_taken, sizeof(dmg_taken), &tmp, nullptr);
	WriteFile(file, &arena_fights, sizeof(arena_fights), &tmp, nullptr);
	FileWriter f(file);
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
	f << book_read.size();
	for(auto book : book_read)
		f << book->id;
}

//=================================================================================================
void PlayerController::Load(HANDLE file)
{
	ReadFile(file, &clas, sizeof(clas), &tmp, nullptr);
	if(LOAD_VERSION < V_0_4)
		clas = ClassInfo::OldToNew(clas);
	ReadString1(file, name);
	if(LOAD_VERSION >= V_CURRENT)
		ReadFile(file, &level, sizeof(level), &tmp, nullptr);
	else
		recalculate_level = true;
	ReadFile(file, &move_tick, sizeof(move_tick), &tmp, nullptr);
	ReadFile(file, &last_dmg, sizeof(last_dmg), &tmp, nullptr);
	ReadFile(file, &last_dmg_poison, sizeof(last_dmg_poison), &tmp, nullptr);
	ReadFile(file, &dmgc, sizeof(dmgc), &tmp, nullptr);
	ReadFile(file, &poison_dmgc, sizeof(poison_dmgc), &tmp, nullptr);
	ReadFile(file, &idle_timer, sizeof(idle_timer), &tmp, nullptr);
	FileReader f(file);
	if(LOAD_VERSION >= V_0_4)
	{
		f >> ap;
		if(LOAD_VERSION >= V_CURRENT)
			f >> sp;
		else
		{
			int old_sp[(int)old::v1::Skill::MAX];
			f >> old_sp;

			// map old skills to new
			sp[(int)Skill::ONE_HANDED_WEAPON] = old_sp[(int)old::v1::Skill::ONE_HANDED_WEAPON];
			sp[(int)Skill::SHORT_BLADE] = old_sp[(int)old::v1::Skill::SHORT_BLADE];
			sp[(int)Skill::LONG_BLADE] = old_sp[(int)old::v1::Skill::LONG_BLADE];
			sp[(int)Skill::BLUNT] = old_sp[(int)old::v1::Skill::BLUNT];
			sp[(int)Skill::AXE] = old_sp[(int)old::v1::Skill::AXE];
			sp[(int)Skill::BOW] = old_sp[(int)old::v1::Skill::BOW];
			sp[(int)Skill::SHIELD] = old_sp[(int)old::v1::Skill::SHIELD];
			sp[(int)Skill::LIGHT_ARMOR] = old_sp[(int)old::v1::Skill::LIGHT_ARMOR];
			sp[(int)Skill::MEDIUM_ARMOR] = old_sp[(int)old::v1::Skill::MEDIUM_ARMOR];
			sp[(int)Skill::HEAVY_ARMOR] = old_sp[(int)old::v1::Skill::HEAVY_ARMOR];
			sp[(int)Skill::UNARMED] = 0;
			sp[(int)Skill::ATHLETICS] = 0;
			sp[(int)Skill::ACROBATICS] = 0;
			sp[(int)Skill::HAGGLE] = 0;
			sp[(int)Skill::LITERACY] = 0;
		}
		SetRequiredPoints();
	}
	else
	{
		// skill points
		int old_sp[(int)old::v0::Skill::MAX];
		f >> old_sp;
		for(int i = 0; i < (int)Skill::MAX; ++i)
			sp[i] = 0;
		sp[(int)Skill::ONE_HANDED_WEAPON] = old_sp[(int)old::v0::Skill::WEAPON];
		sp[(int)Skill::BOW] = old_sp[(int)old::v0::Skill::BOW];
		sp[(int)Skill::SHIELD] = old_sp[(int)old::v0::Skill::SHIELD];
		sp[(int)Skill::LIGHT_ARMOR] = old_sp[(int)old::v0::Skill::LIGHT_ARMOR];
		sp[(int)Skill::HEAVY_ARMOR] = old_sp[(int)old::v0::Skill::HEAVY_ARMOR];
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
		// __int64 spg[(int)Skill::MAX][T_MAX], apg[(int)Attribute::MAX][T_MAX];
		// T_MAX = 4
		// Skill::MAX = 5
		// Attribute::MAX = 3
		// size = sizeof(__int64) * T_MAX * (S_MAX + A_MAX)
		// size = 8 * 4 * (5 + 3) = 256
		f.Skip(256);

		// SetRequiredPoints called from Unit::Load after setting new attributes/skills
	}
	if(LOAD_VERSION < V_CURRENT)
	{
		// rescale skill points counter
		for(uint i = 0; i < (uint)Skill::MAX; ++i)
		{
			if(sp[i] > 0)
			{
				int old_required = old::GetRequiredSkillPoints(unit->GetBase((Skill)i));
				float ratio = float(sn[i]) / old_required;
				sp[i] = (int)(ratio * sp[i]);
			}
		}
	}
	ReadFile(file, &action_key, sizeof(action_key), &tmp, nullptr);
	ReadFile(file, &next_action, sizeof(next_action), &tmp, nullptr);
	ReadFile(file, &next_action_idx, sizeof(next_action_idx), &tmp, nullptr);
	ReadFile(file, &ostatnia, sizeof(ostatnia), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_20)
	{
		// stary raise_timer, teraz jest w Unit
		float raise_timer;
		ReadFile(file, &raise_timer, sizeof(raise_timer), &tmp, nullptr);
	}
	ReadFile(file, &credit, sizeof(credit), &tmp, nullptr);
	ReadFile(file, &godmode, sizeof(godmode), &tmp, nullptr);
	ReadFile(file, &noclip, sizeof(noclip), &tmp, nullptr);
	ReadFile(file, &id, sizeof(id), &tmp, nullptr);
	ReadFile(file, &free_days, sizeof(free_days), &tmp, nullptr);
	ReadFile(file, &kills, sizeof(kills), &tmp, nullptr);
	ReadFile(file, &knocks, sizeof(knocks), &tmp, nullptr);
	ReadFile(file, &dmg_done, sizeof(dmg_done), &tmp, nullptr);
	ReadFile(file, &dmg_taken, sizeof(dmg_taken), &tmp, nullptr);
	ReadFile(file, &arena_fights, sizeof(arena_fights), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_4)
	{
		if(LOAD_VERSION < V_CURRENT)
		{
			// skip old stats
			f.Skip(sizeof(old::UnitStats) + sizeof(StatState) * ((int)Attribute::MAX + (int)old::v1::Skill::MAX));
		}
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

	uint count;
	f >> count;
	book_read.reserve(count);
	for(uint i = 0; i < count; ++i)
	{
		f.ReadStringBUF();
		book_read.push_back(Item::Get(BUF));
	}

	action = Action_None;
}

//=================================================================================================
void PlayerController::SetRequiredPoints()
{
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		an[i] = GetRequiredAttributePoints(unit->GetBase((Attribute)i));
	for(int i = 0; i < (int)Skill::MAX; ++i)
		sn[i] = GetRequiredSkillPoints(unit->GetBase((Skill)i));
}

//=================================================================================================
// Used to send per-player data in WritePlayerData
void PlayerController::Write(BitStream& stream) const
{
	stream.Write(level);
	stream.Write(kills);
	stream.Write(dmg_done);
	stream.Write(dmg_taken);
	stream.Write(knocks);
	stream.Write(arena_fights);
	stream.WriteCasted<byte>(perks.size());
	for(const TakenPerk& perk : perks)
	{
		stream.WriteCasted<byte>(perk.perk);
		stream.Write(perk.value);
	}
	stream.Write(action_cooldown);
	stream.Write(action_recharge);
	stream.Write(action_charges);
}

//=================================================================================================
// Used to sent per-player data in ReadPlayerData
bool PlayerController::Read(BitStream& stream)
{
	byte count;
	if(!stream.Read(level)
		|| !stream.Read(kills)
		|| !stream.Read(dmg_done)
		|| !stream.Read(dmg_taken)
		|| !stream.Read(knocks)
		|| !stream.Read(arena_fights)
		|| !stream.Read(count)
		|| !EnsureSize(stream, 5 * count))
		return false;
	perks.resize(count);
	for(TakenPerk& perk : perks)
	{
		if(!stream.ReadCasted<byte>(perk.perk)
			|| !stream.Read(perk.value))
			return false;
	}
	if(!stream.Read(action_cooldown)
		|| !stream.Read(action_recharge)
		|| !stream.Read(action_charges))
		return false;
	unit->level = (int)level;
	return true;
}

//=================================================================================================
Action& PlayerController::GetAction()
{
	auto action = ClassInfo::classes[(int)clas].action;
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
void PlayerController::RecalculateLevel()
{
	if(!recalculate_level || Net::IsClient())
		return;
	recalculate_level = false;
	float new_level = unit->statsx->CalculateLevel();
	if(new_level > level)
	{
		level = new_level;
		unit->level = (int)level;
		unit->RecalculateHp();

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = unit;
		}

		if(!is_local)
		{
			NetChangePlayer& c = Add1(Net::player_changes);
			c.type = NetChangePlayer::UPDATE_LEVEL;
			c.pc = this;
			c.v = level;
			player_info->NeedUpdate();
		}
	}
}

//=================================================================================================
void PlayerController::OnReadBook(int i_index)
{
	if(Net::IsLocal())
	{
		auto item = unit->GetIIndexItem(i_index);
		assert(item->type == IT_BOOK);
		if(IS_SET(item->flags, ITEM_MAGIC_SCROLL))
		{
			unit->action = A_USE_ITEM;
			unit->used_item = item;
			unit->mesh_inst->frame_end_info2 = false;
			unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_ITEM;
				c.unit = unit;
			}
		}
		else
		{
			bool ok = true;
			for(auto book : book_read)
			{
				if(book == item)
				{
					ok = false;
					break;
				}
			}
			if(ok)
			{
				Train(TrainWhat::Read, 0.f, 0);
				book_read.push_back(item);
			}
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::READ_BOOK;
		c.id = i_index;
	}
}
