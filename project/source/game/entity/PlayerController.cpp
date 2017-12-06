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

		for(auto& p : attrib_pts)
		{
			p.value = 0;
			p.part = 0;
		}
		for(auto& p : skill_pts)
		{
			p.value = 0;
			p.part = 0;
		}

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
	switch(what)
	{
	case TrainWhat::Attack:
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
			float points = Lerp(200.f, 2450.f, ratio) * GetLevelMod(unit->level, level);
			Train(skill, points);
		}
		break;
	case TrainWhat::Move:
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
			float acro, ath;
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
		{
			auto& shield = unit->GetShield();
			if(shield.req_str > unit->GetBase(Attribute::STR))
			{
				// train str when too low for shield
				Train(Attribute::STR, 50);
			}

			// train shield skill
			Train(Skill::SHIELD, value * 2000);
		}
		break;
	case TrainWhat::TakeHit:
		{
			// train armor skill
			if(unit->HaveArmor())
				Train(unit->GetArmor().skill, value * 2000);

			// train shield skill / 5
			if(unit->HaveShield())
				Train(Skill::SHIELD, value * 400);
		}
		break;
	case TrainWhat::TakeDamage:
		{
			// train end
			Train(Attribute::END, value * 1250);

			// train athletics skill
			Train(Skill::ATHLETICS, value * 2500);
		}
		break;
	case TrainWhat::Regenerate:
		Train(Attribute::END, value);
		break;
	case TrainWhat::Stamina:
		Train(Attribute::END, value);
		break;
	case TrainWhat::Trade:
		Train(Skill::HAGGLE, value);
		break;
	case TrainWhat::Read:
		Train(Skill::LITERACY, 2500);
		break;
	}
}

//=================================================================================================
void PlayerController::Train(TrainMode mode, int what, bool is_skill)
{
	TrainData* train_data;
	int value;
	if(is_skill)
	{
		if(unit->statsx->skill[what] == SkillInfo::MAX)
		{
			skill_pts[what].value = skill_pts[what].next;
			return;
		}
		value = unit->statsx->skill[what];
		train_data = &skill_pts[what];
	}
	else
	{
		if(unit->statsx->attrib[what] == AttributeInfo::MAX)
		{
			attrib_pts[what].value = attrib_pts[what].next;
			return;
		}
		value = unit->statsx->attrib[what];
		train_data = &attrib_pts[what];
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
		train_data->value /= 2;

		if(is_skill)
		{
			train_data->next = GetRequiredSkillPoints(value);
			unit->Set((Skill)what, value);
		}
		else
		{
			train_data->next = GetRequiredAttributePoints(value);
			unit->Set((Attribute)what, value);
		}

		if(is_local)
			Game::Get().ShowStatGain(is_skill, what, count);
		else
		{
			NetChangePlayer& c = Add1(player_info->changes);
			c.type = NetChangePlayer::GAIN_STAT;
			c.id = (is_skill ? 1 : 0);
			c.a = what;
			c.ile = count;

			NetChangePlayer& c2 = Add1(player_info->changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
			c2.id = int(is_skill ? ChangedStatType::SKILL : ChangedStatType::ATTRIBUTE);
			c2.a = what;
			c2.ile = value;
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
		float pts = m * train_data->next;
		if(is_skill)
			Train((Skill)what, pts, false);
		else
			Train((Attribute)what, pts);
	}
}

//=================================================================================================
void PlayerController::Train(Attribute attrib, float points)
{
	int a = (int)attrib;
	auto& train_data = attrib_pts[a];
	points = unit->GetAptitudeMod(attrib) * points + train_data.part;
	int points_i = (int)points;
	train_data.part = points - points_i;
	if(points_i == 0)
		return;

	train_data.value += points_i;

	int gained = 0,
		value = unit->GetBase(attrib);

	while(train_data.value >= train_data.next)
	{
		train_data.value -= train_data.next;
		if(unit->statsx->attrib[a] != AttributeInfo::MAX)
		{
			++gained;
			++value;
			train_data.next = GetRequiredAttributePoints(value);
		}
		else
		{
			train_data.value = train_data.next;
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
void PlayerController::Train(Skill skill, float points, bool train_attrib)
{
	int s = (int)skill;
	auto& train_data = skill_pts[s];
	float mod_points = unit->GetAptitudeMod(skill) * points + train_data.part;
	int points_i = (int)mod_points;
	train_data.part = mod_points - points_i;
	if(points_i == 0)
		return;

	train_data.value += points_i;

	int gained = 0,
		value = unit->GetBase(skill);

	while(train_data.value >= train_data.next)
	{
		train_data.value -= train_data.next;
		if(value != SkillInfo::MAX)
		{
			++gained;
			++value;
			train_data.next = GetRequiredSkillPoints(value);
		}
		else
		{
			train_data.value = train_data.next;
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
		Train(info.attrib, info.attrib_ratio * points);
		if(info.attrib2 != Attribute::NONE)
			Train(info.attrib2, (1.f - info.attrib_ratio) * points);
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
	WriteFile(file, attrib_pts, sizeof(attrib_pts), &tmp, nullptr);
	WriteFile(file, skill_pts, sizeof(skill_pts), &tmp, nullptr);
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
	f << perk_points;
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
	if(LOAD_VERSION >= V_CURRENT)
	{
		f >> attrib_pts;
		f >> skill_pts;
	}
	else if(LOAD_VERSION >= V_0_4)
	{
		int old_ap[(int)Attribute::MAX];
		int old_sp[(int)old::v1::Skill::MAX];
		f >> old_ap;
		f >> old_sp;

		// map old attributes
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			attrib_pts[i].value = old_ap[i];
			attrib_pts[i].part = 0;
		}

		// map old skills
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			skill_pts[i].value = 0;
			skill_pts[i].part = 0;
		}
		skill_pts[(int)Skill::ONE_HANDED_WEAPON].value = old_sp[(int)old::v1::Skill::ONE_HANDED_WEAPON];
		skill_pts[(int)Skill::SHORT_BLADE].value = old_sp[(int)old::v1::Skill::SHORT_BLADE];
		skill_pts[(int)Skill::LONG_BLADE].value = old_sp[(int)old::v1::Skill::LONG_BLADE];
		skill_pts[(int)Skill::BLUNT].value = old_sp[(int)old::v1::Skill::BLUNT];
		skill_pts[(int)Skill::AXE].value = old_sp[(int)old::v1::Skill::AXE];
		skill_pts[(int)Skill::BOW].value = old_sp[(int)old::v1::Skill::BOW];
		skill_pts[(int)Skill::SHIELD].value = old_sp[(int)old::v1::Skill::SHIELD];
		skill_pts[(int)Skill::LIGHT_ARMOR].value = old_sp[(int)old::v1::Skill::LIGHT_ARMOR];
		skill_pts[(int)Skill::MEDIUM_ARMOR].value = old_sp[(int)old::v1::Skill::MEDIUM_ARMOR];
		skill_pts[(int)Skill::HEAVY_ARMOR].value = old_sp[(int)old::v1::Skill::HEAVY_ARMOR];
	}
	else
	{
		// skill points
		int old_sp[(int)old::v0::Skill::MAX];
		f >> old_sp;
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			skill_pts[i].value = 0;
			skill_pts[i].part = 0;
		}
		skill_pts[(int)Skill::ONE_HANDED_WEAPON].value = old_sp[(int)old::v0::Skill::WEAPON];
		skill_pts[(int)Skill::BOW].value = old_sp[(int)old::v0::Skill::BOW];
		skill_pts[(int)Skill::SHIELD].value = old_sp[(int)old::v0::Skill::SHIELD];
		skill_pts[(int)Skill::LIGHT_ARMOR].value = old_sp[(int)old::v0::Skill::LIGHT_ARMOR];
		skill_pts[(int)Skill::HEAVY_ARMOR].value = old_sp[(int)old::v0::Skill::HEAVY_ARMOR];

		// skip old skill need
		f.Skip(5 * sizeof(int));

		// attribute points (str, end, dex)
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			attrib_pts[i].value = 0;
			attrib_pts[i].part = 0;
		}
		for(int i = 0; i < 3; ++i)
			f >> attrib_pts[i].value;

		// skip old attribute need
		f.Skip(3 * sizeof(int));

		// skip old version train data
		f.Skip(256);
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
	if(LOAD_VERSION >= V_0_4 && LOAD_VERSION < V_CURRENT)
	{
		// skip old stats
		f.Skip(sizeof(old::UnitStats) + sizeof(StatState) * ((int)Attribute::MAX + (int)old::v1::Skill::MAX));

		// load old perks
		byte count;
		f >> count;
		unit->statsx->perks.reserve(count);
		for(byte i = 0; i < count; ++i)
		{
			old::Perk old_perk = (old::Perk)f.Read<byte>();
			int value = f.Read<int>();
			Perk perk;
			switch(old_perk)
			{
			case old::Perk::Weakness:
				switch((Attribute)value)
				{
				case Attribute::STR:
					perk = Perk::BadBack;
					break;
				case Attribute::END:
					perk = Perk::ChronicDisease;
					break;
				case Attribute::DEX:
					perk = Perk::Sluggish;
					break;
				case Attribute::INT:
					perk = Perk::SlowLearner;
					break;
				case Attribute::CHA:
					perk = Perk::Asocial;
					break;
				default:
					perk = Perk::None;
					break;
				}
				value = 0;
				break;
			case old::Perk::Strength:
				perk = Perk::Talent;
				break;
			case old::Perk::Skilled:
				perk = Perk::Skilled;
				break;
			case old::Perk::SkillFocus:
				perk = Perk::None;
				break;
			case old::Perk::Talent:
				perk = Perk::SkillFocus;
				break;
			case old::Perk::AlchemistApprentice:
				perk = Perk::AlchemistApprentice;
				break;
			case old::Perk::Wealthy:
				perk = Perk::Wealthy;
				break;
			case old::Perk::VeryWealthy:
				{
					TakenPerk new_perk;
					new_perk.perk = Perk::Wealthy;
					new_perk.value = 0;
					new_perk.hidden = true;
					unit->statsx->perks.push_back(perk);

					perk = Perk::VeryWealthy;
				}
				break;
			case old::Perk::FamilyHeirloom:
				perk = Perk::FamilyHeirloom;
				break;
			case old::Perk::Leader:
				perk = Perk::Leader;
				break;
			default:
				perk = Perk::None;
				break;
			}

			if(perk != Perk::None)
				unit->statsx->perks.push_back(TakenPerk(perk, value));
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

	if(LOAD_VERSION >= V_CURRENT)
	{
		// list of book read
		uint count;
		f >> count;
		book_read.reserve(count);
		for(uint i = 0; i < count; ++i)
		{
			f.ReadStringBUF();
			book_read.push_back(Item::Get(BUF));
		}

		// perk points
		f >> perk_points;
	}

	action = Action_None;
}

//=================================================================================================
void PlayerController::SetRequiredPoints()
{
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		attrib_pts[i].next = GetRequiredAttributePoints(unit->GetBase((Attribute)i));
	for(int i = 0; i < (int)Skill::MAX; ++i)
		skill_pts[i].next = GetRequiredSkillPoints(unit->GetBase((Skill)i));
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
	stream.Write(action_cooldown);
	stream.Write(action_recharge);
	stream.Write(action_charges);
	stream.Write(perk_points);
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
	if(!stream.Read(action_cooldown)
		|| !stream.Read(action_recharge)
		|| !stream.Read(action_charges)
		|| !stream.Read(perk_points))
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
void PlayerController::RecalculateLevel(bool initial)
{
	if(!recalculate_level || Net::IsClient())
		return;
	recalculate_level = false;
	float new_level = unit->statsx->CalculateLevel();
	if(new_level > level || initial)
	{
		level = new_level;
		unit->level = (int)level;

		if(!initial)
		{
			unit->RecalculateHp();

			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = unit;
			}

			if(!is_local)
			{
				NetChangePlayer& c = Add1(player_info->changes);
				c.type = NetChangePlayer::UPDATE_LEVEL;
				c.v = level;
			}
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
