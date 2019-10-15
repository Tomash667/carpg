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
#include "GameGui.h"
#include "GameMessages.h"
#include "Team.h"
#include "World.h"
#include "ScriptException.h"
#include "AIController.h"
#include "SoundManager.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "Inventory.h"
#include "Arena.h"
#include "Spell.h"
#include "ParticleSystem.h"
#include "FOV.h"
#include "LevelGui.h"
#include "BookPanel.h"
#include "Door.h"
#include "GroundItem.h"
#include "ScriptManager.h"
#include "Quest_Scripted.h"
#include "GameResources.h"

LocalPlayerData PlayerController::data;

PlayerAction InventoryModeToActionRequired(InventoryMode imode)
{
	switch(imode)
	{
	case I_NONE:
	case I_INVENTORY:
		return PlayerAction::None;
	case I_LOOT_BODY:
		return PlayerAction::LootUnit;
	case I_LOOT_CHEST:
		return PlayerAction::LootChest;
	case I_TRADE:
		return PlayerAction::Trade;
	case I_SHARE:
		return PlayerAction::ShareItems;
	case I_GIVE:
		return PlayerAction::GiveItems;
	case I_LOOT_CONTAINER:
		return PlayerAction::LootContainer;
	default:
		assert(0);
		return PlayerAction::None;
	}
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
	last_weapon = W_NONE;
	next_action = NA_NONE;
	last_dmg_poison = last_dmg = dmgc = poison_dmgc = 0.f;
	idle_timer = Random(1.f, 2.f);
	credit = 0;
	on_credit = false;
	action = PlayerAction::None;
	free_days = 0;
	recalculate_level = false;
	split_gold = 0.f;

	if(!partial)
	{
		godmode = false;
		noclip = false;
		invisible = false;
		always_run = true;
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
		InitShortcuts();
	}
}

//=================================================================================================
void PlayerController::InitShortcuts()
{
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		shortcuts[i].type = Shortcut::TYPE_NONE;
		shortcuts[i].trigger = false;
	}
	shortcuts[0].type = Shortcut::TYPE_SPECIAL;
	shortcuts[0].value = Shortcut::SPECIAL_MELEE_WEAPON;
	shortcuts[1].type = Shortcut::TYPE_SPECIAL;
	shortcuts[1].value = Shortcut::SPECIAL_RANGED_WEAPON;
	shortcuts[2].type = Shortcut::TYPE_SPECIAL;
	shortcuts[2].value = Shortcut::SPECIAL_ACTION;
	shortcuts[3].type = Shortcut::TYPE_SPECIAL;
	shortcuts[3].value = Shortcut::SPECIAL_HEALING_POTION;
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
		game_gui->messages->AddFormattedMessage(this, GMS_GAIN_SKILL, s, gained);
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
		game_gui->messages->AddFormattedMessage(this, GMS_GAIN_ATTRIBUTE, a, gained);
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
		prev_mp = unit->mp,
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

		if(unit->mp != prev_mp)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_MP;
			c.unit = unit;
		}

		if(unit->stamina != prev_stamina)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_STAMINA;
			c.unit = unit;
		}

		if(!is_local)
		{
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
	case NA_EQUIP_DRAW:
		f << next_action_data.index;
		f << next_action_data.item->id;
		break;
	case NA_USE:
		f << next_action_data.usable->id;
		break;
	default:
		assert(0);
		break;
	}
	f << last_weapon;
	f << credit;
	f << godmode;
	f << noclip;
	f << invisible;
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
		for(Entity<Unit> unit : action_targets)
			f << unit;
	}
	f << split_gold;
	f << always_run;
	f << exp;
	f << exp_level;
	f << last_ring;
	for(Shortcut& shortcut : shortcuts)
	{
		f << shortcut.type;
		if(shortcut.type == Shortcut::TYPE_SPECIAL)
			f << shortcut.value;
		else if(shortcut.type == Shortcut::TYPE_ITEM)
			f << shortcut.item->id;
	}
}

//=================================================================================================
void PlayerController::Load(FileReader& f)
{
	if(LOAD_VERSION < V_0_7)
		f.Skip<int>(); // old class info
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
	else
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
		case NA_EQUIP_DRAW:
			f >> next_action_data.index;
			next_action_data.item = Item::Get(f.ReadString1());
			break;
		case NA_USE:
			Usable::AddRequest(&next_action_data.usable, f.Read<int>());
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
	if(LOAD_VERSION >= V_0_10)
		f >> invisible;
	f >> id;
	f >> free_days;
	f >> kills;
	f >> knocks;
	f >> dmg_done;
	f >> dmg_taken;
	f >> arena_fights;
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
	f >> action_cooldown;
	f >> action_recharge;
	action_charges = f.Read<byte>();
	if(unit->action == A_DASH && unit->animation_state == 1)
	{
		uint count;
		f >> count;
		action_targets.resize(count);
		for(Entity<Unit>& unit : action_targets)
			f >> unit;
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
	if(LOAD_VERSION >= V_0_10)
		f >> last_ring;
	else
		last_ring = false;
	if(LOAD_VERSION >= V_0_10)
	{
		for(Shortcut& shortcut : shortcuts)
		{
			f >> shortcut.type;
			if(shortcut.type == Shortcut::TYPE_SPECIAL)
				f >> shortcut.value;
			else if(shortcut.type == Shortcut::TYPE_ITEM)
				shortcut.item = Item::Get(f.ReadString1());
			shortcut.trigger = false;
		}
	}
	else
		InitShortcuts();

	action = PlayerAction::None;
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
	float prio = 0.f;
	UnitStats& stats = *unit->stats;
	Class* clas = unit->GetClass();

	if(clas->level.empty())
		return 1;

	// apply attributes/skills
	static vector<pair<float, float>> values;
	values.clear();
	for(Class::LevelEntry& entry : clas->level)
	{
		float e_level, e_prio = -1;
		switch(entry.type)
		{
		case Class::LevelEntry::T_ATTRIBUTE:
			{
				float value = (float)stats.attrib[entry.value];
				e_level = (value - 25) / 5 * entry.priority;
				e_prio = entry.priority;
			}
			break;
		case Class::LevelEntry::T_SKILL:
			{
				float value = (float)stats.skill[entry.value];
				e_level = value / 5 * entry.priority;
				e_prio = entry.priority;
			}
			break;
		case Class::LevelEntry::T_SKILL_SPECIAL:
			if(entry.value == Class::LevelEntry::S_WEAPON)
			{
				SkillId skill = WeaponTypeInfo::info[unit->GetBestWeaponType()].skill;
				float value = (float)stats.skill[(int)skill];
				e_level = value / 5 * entry.priority;
				e_prio = entry.priority;
			}
			else if(entry.value == Class::LevelEntry::S_ARMOR)
			{
				SkillId skill = GetArmorTypeSkill(unit->GetBestArmorType());
				float value = (float)stats.skill[(int)skill];
				e_level = value / 5 * entry.priority;
				e_prio = entry.priority;
			}
			break;
		}

		if(e_prio > 0)
		{
			if(entry.required)
			{
				level += e_level;
				prio += e_prio;
			}
			else
				values.push_back({ e_level, e_prio });
		}
	}

	// use 4 most important combat skill from currently 5 (weapon, one handed, armor, bow, shield)
	std::sort(values.begin(), values.end(), [](const pair<float, float>& v1, const pair<float, float>& v2)
	{
		return v1.first > v2.first;
	});
	for(uint i = 0; i < min(4u, values.size()); ++i)
	{
		pair<float, float>& p = values[i];
		level += p.first;
		prio += p.second;
	}

	return int(level / prio);
}

//=================================================================================================
void PlayerController::RecalculateLevel()
{
	int level = CalculateLevel();
	if(level != unit->level)
	{
		unit->level = level;
		if(Net::IsLocal() && !game_level->entering)
		{
			team->CalculatePlayersLevel();
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
	case TrainWhat::Mana:
		TrainMod(SkillId::CONCENTRATION, value * 5);
		break;
	case TrainWhat::Cast:
		TrainMod(SkillId::GODS_MAGIC, 2000.f);
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

		game_gui->messages->AddFormattedMessage(this, is_skill ? GMS_GAIN_SKILL : GMS_GAIN_ATTRIBUTE, id, count);

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
void PlayerController::WriteStart(BitStreamWriter& f) const
{
	f << invisible;
	f << noclip;
	f << godmode;
	f << always_run;
	for(const Shortcut& shortcut : shortcuts)
	{
		f << shortcut.type;
		if(shortcut.type == Shortcut::TYPE_SPECIAL)
			f << shortcut.value;
		else if(shortcut.type == Shortcut::TYPE_ITEM)
			f << shortcut.item->id;
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
	case NA_EQUIP_DRAW:
		f << GetNextActionItemIndex();
		break;
	case NA_USE:
		f << next_action_data.usable->id;
		break;
	default:
		assert(0);
		break;
	}
	f << last_ring;
}

//=================================================================================================
void PlayerController::ReadStart(BitStreamReader& f)
{
	f >> invisible;
	f >> noclip;
	f >> godmode;
	f >> always_run;
	for(Shortcut& shortcut : shortcuts)
	{
		f >> shortcut.type;
		if(shortcut.type == Shortcut::TYPE_SPECIAL)
			f >> shortcut.value;
		else if(shortcut.type == Shortcut::TYPE_ITEM)
			shortcut.item = Item::Get(f.ReadString1());
		shortcut.trigger = false;
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
	case NA_EQUIP_DRAW:
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
			next_action_data.usable = game_level->FindUsable(index);
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
	return team->IsLeader(unit);
}

//=================================================================================================
Action& PlayerController::GetAction() const
{
	Action* action = unit->GetClass()->action;
	assert(action);
	return *action;
}

//=================================================================================================
bool PlayerController::CanUseAction() const
{
	if(action_charges == 0 || action_cooldown > 0)
		return false;
	if(quest_mgr->quest_tutorial->in_tutorial)
		return false;
	Action& action = GetAction();
	if(unit->action != A_NONE)
	{
		if(!Any(unit->action, A_ATTACK, A_BLOCK, A_BASH))
			return false;
		if(action.use_cast)
			return false;
	}
	if(action.use_mana)
		return unit->mp >= action.cost;
	else
		return unit->stamina >= action.cost;
}

//=================================================================================================
bool PlayerController::UseActionCharge()
{
	if(action_charges == 0 || action_cooldown > 0)
		return false;
	Action& action = GetAction();
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
void PlayerController::PayCredit(int count)
{
	rvector<Unit> units;
	for(Unit& u : team->active_members)
	{
		if(&u != unit)
			units.push_back(&u);
	}

	team->AddGold(count, &units);

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

		for(PlayerInfo& info : net->players)
		{
			if(info.left == PlayerInfo::LEFT_NO && info.pc != this)
				info.pc->free_days += count;
		}

		world->Update(count, World::UM_NORMAL);
	}

	Net::PushChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
void PlayerController::StartDialog(Unit* talker, GameDialog* dialog, Quest* quest)
{
	assert(talker);

	DialogContext& ctx = *dialog_ctx;
	assert(!ctx.dialog_mode);

	if(!is_local)
	{
		NetChangePlayer& c = Add1(player_info->changes);
		c.type = NetChangePlayer::START_DIALOG;
		c.id = talker->id;
	}

	ctx.StartDialog(talker, dialog, quest);
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
	game_gui->messages->AddFormattedMessage(this, GMS_GAIN_LEARNING_POINTS, -1, count);
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

//=================================================================================================
void PlayerController::Yell()
{
	if(Sound* sound = unit->GetSound(SOUND_SEE_ENEMY))
	{
		game->PlayAttachedSound(*unit, sound, Unit::ALERT_SOUND_DIST);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UNIT_SOUND;
			c.unit = unit;
			c.id = SOUND_SEE_ENEMY;
		}
	}
	unit->Talk(RandomString(game->txYell), 0);

	LevelArea& area = *unit->area;
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(u2.IsAI() && u2.IsStanding() && !unit->IsEnemy(u2) && !unit->IsFriend(u2) && u2.busy == Unit::Busy_No && u2.frozen == FROZEN::NO && !u2.usable
			&& u2.ai->state == AIController::Idle && !IsSet(u2.data->flags, F_AI_STAY)
			&& Any(u2.ai->idle_action, AIController::Idle_None, AIController::Idle_Animation, AIController::Idle_Rot, AIController::Idle_Look))
		{
			u2.ai->idle_action = AIController::Idle_MoveAway;
			u2.ai->idle_data.unit = unit;
			u2.ai->timer = Random(3.f, 5.f);
		}
	}
}

//=================================================================================================
int PlayerController::GetHealingPotion() const
{
	float healed_hp,
		missing_hp = unit->hpmax - unit->hp;
	int potion_index = -1, index = 0;

	for(vector<ItemSlot>::iterator it = unit->items.begin(), end = unit->items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->item->type != IT_CONSUMABLE)
			continue;

		const Consumable& pot = it->item->ToConsumable();
		if(pot.ai_type != ConsumableAiType::Healing)
			continue;

		float power = pot.GetEffectPower(EffectId::Heal);
		if(potion_index == -1)
		{
			potion_index = index;
			healed_hp = power;
		}
		else
		{
			if(power > missing_hp)
			{
				if(power < healed_hp)
				{
					potion_index = index;
					healed_hp = power;
				}
			}
			else if(power > healed_hp)
			{
				potion_index = index;
				healed_hp = power;
			}
		}
	}

	return potion_index;
}

//=================================================================================================
void PlayerController::ClearShortcuts()
{
	for(int i = 0; i < Shortcut::MAX; ++i)
		shortcuts[i].trigger = false;
}

//=================================================================================================
void PlayerController::SetShortcut(int index, Shortcut::Type type, int value)
{
	assert(index >= 0 && index < Shortcut::MAX);
	shortcuts[index].type = type;
	shortcuts[index].value = value;
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SET_SHORTCUT;
		c.id = index;
	}
}

//=================================================================================================
float PlayerController::GetActionPower() const
{
	// currently only used by heal spell
	return 150.f + 5.f * (unit->Get(AttributeId::CHA) - 50.f) + 5.f * unit->Get(SkillId::GODS_MAGIC);
}

//=================================================================================================
float PlayerController::GetShootAngle() const
{
	const float pt0 = 4.6662526f;
	return game_level->camera.rot.y - pt0;
}

//=================================================================================================
void PlayerController::CheckObjectDistance(const Vec3& pos, void* ptr, float& best_dist, BeforePlayer type)
{
	assert(ptr);

	if(pos.y < unit->pos.y - 0.5f || pos.y > unit->pos.y + 2.f)
		return;

	float dist = Vec3::Distance2d(unit->pos, pos);
	if(dist < PICKUP_RANGE && dist < best_dist)
	{
		float angle = AngleDiff(Clip(unit->rot + PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos)));
		assert(angle >= 0.f);
		if(angle < PI / 4)
		{
			if(type == BP_CHEST)
			{
				Chest* chest = (Chest*)ptr;
				if(AngleDiff(Clip(chest->rot - PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos))) > PI / 2)
					return;
			}
			else if(type == BP_USABLE)
			{
				Usable* use = (Usable*)ptr;
				auto& bu = *use->base;
				if(IsSet(bu.use_flags, BaseUsable::CONTAINER))
				{
					float allowed_dif;
					switch(bu.limit_rot)
					{
					default:
					case 0:
						allowed_dif = PI * 2;
						break;
					case 1:
						allowed_dif = PI / 2;
						break;
					case 2:
						allowed_dif = PI / 4;
						break;
					}
					if(AngleDiff(Clip(use->rot - PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos))) > allowed_dif)
						return;
				}
			}
			dist += angle;
			if(dist < best_dist && game_level->CanSee(*unit->area, unit->pos, pos, type == BP_DOOR, ptr))
			{
				best_dist = dist;
				data.before_player_ptr.any = ptr;
				data.before_player = type;
			}
		}
	}
}

//=================================================================================================
void PlayerController::UseUsable(Usable* usable, bool after_action)
{
	Unit& u = *unit;
	Usable& use = *usable;
	BaseUsable& bu = *use.base;

	bool ok = true;
	if(bu.item)
	{
		if(!u.HaveItem(bu.item) && u.slots[SLOT_WEAPON] != bu.item)
		{
			game_gui->messages->AddGameMsg2(Format(game->txNeedItem, bu.item->name.c_str()), 2.f);
			ok = false;
		}
		else if(unit->weapon_state != WeaponState::Hidden && (bu.item != &unit->GetWeapon() || unit->HaveShield()))
		{
			if(after_action)
				return;
			u.HideWeapon();
			next_action = NA_USE;
			next_action_data.usable = &use;
			if(Net::IsClient())
				Net::PushChange(NetChange::SET_NEXT_ACTION);
			ok = false;
		}
		else
			u.used_item = bu.item;
	}

	if(!ok)
		return;

	if(Net::IsLocal())
	{
		u.UseUsable(&use);
		data.before_player = BP_NONE;

		if(IsSet(bu.use_flags, BaseUsable::CONTAINER))
		{
			// loot container
			game_gui->inventory->StartTrade2(I_LOOT_CONTAINER, &use);
		}
		else
		{
			u.action = A_ANIMATION2;
			u.animation = ANI_PLAY;
			u.mesh_inst->Play(bu.anim.c_str(), PLAY_PRIO1, 0);
			u.target_pos = u.pos;
			u.target_pos2 = use.pos;
			if(use.base->limit_rot == 4)
				u.target_pos2 -= Vec3(sin(use.rot)*1.5f, 0, cos(use.rot)*1.5f);
			u.timer = 0.f;
			u.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
			u.use_rot = Vec3::LookAtAngle(u.pos, u.usable->pos);
		}

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::USE_USABLE;
			c.unit = &u;
			c.id = u.usable->id;
			c.count = USE_USABLE_START;
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.id = data.before_player_ptr.usable->id;
		c.count = USE_USABLE_START;

		if(IsSet(bu.use_flags, BaseUsable::CONTAINER))
		{
			action = PlayerAction::LootContainer;
			action_usable = data.before_player_ptr.usable;
			chest_trade = &action_usable->container->items;
		}

		unit->action = A_PREPARE;
	}
}

//=================================================================================================
void PlayerController::CastSpell()
{
	Action& action = GetAction();
	LevelArea& area = *unit->area;

	if(strcmp(action.id, "summon_wolf") == 0)
	{
		// despawn old
		Unit* existing_unit = game_level->FindUnit([&](Unit* u) { return u->summoner == unit; });
		if(existing_unit)
		{
			team->RemoveTeamMember(existing_unit);
			game_level->RemoveUnit(existing_unit);
		}

		// spawn new
		Unit* new_unit = game_level->SpawnUnitNearLocation(area, unit->target_pos, *UnitData::Get("white_wolf_sum"), nullptr, unit->level);
		if(new_unit)
		{
			new_unit->summoner = unit;
			new_unit->in_arena = unit->in_arena;
			if(new_unit->in_arena != -1)
				game->arena->units.push_back(new_unit);
			team->AddTeamMember(new_unit, HeroType::Visitor);
			new_unit->order->unit = unit; // follow summoner
			game_level->SpawnUnitEffect(*new_unit);
		}
	}
	else if(strcmp(action.id, "heal") == 0)
	{
		Spell& spell = *Spell::TryGet("heal");
		Unit* target = unit->action_unit;

		// check if target is not too far
		if(target && target->area == unit->area && Vec3::Distance(unit->pos, target->pos) <= action.area_size.x * 1.5f)
		{
			// heal target
			if(!IsSet(target->data->flags, F_UNDEAD) && !IsSet(target->data->flags2, F2_CONSTRUCT) && target->hp != target->hpmax)
			{
				float power = GetActionPower();
				target->hp += power;
				if(target->hp > target->hpmax)
					target->hp = target->hpmax;
				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = target;
				}
			}

			// particle effect
			float r = target->GetUnitRadius(),
				h = target->GetUnitHeight();
			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = spell.tex_particle;
			pe->emision_interval = 0.01f;
			pe->life = 0.f;
			pe->particle_life = 0.5f;
			pe->emisions = 1;
			pe->spawn_min = 16;
			pe->spawn_max = 25;
			pe->max_particles = 25;
			pe->pos = target->pos;
			pe->pos.y += h / 2;
			pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
			pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
			pe->pos_min = Vec3(-r, -h / 2, -r);
			pe->pos_max = Vec3(r, h / 2, r);
			pe->size = spell.size_particle;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->alpha = 0.9f;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->mode = 1;
			pe->Init();
			area.tmp->pes.push_back(pe);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::HEAL_EFFECT;
				c.pos = pe->pos;
			}

			// sound effect
			if(spell.sound_cast)
			{
				Vec3 pos = target->GetCenter();
				sound_mgr->PlaySound3d(spell.sound_cast, pos, spell.sound_cast_dist);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::SPELL_SOUND;
					c.spell = &spell;
					c.pos = pos;
				}
			}

			Train(TrainWhat::Cast, 0.f, 0);
		}
	}
}

//=================================================================================================
void PlayerController::UseAction(bool from_server, const Vec3* pos, Unit* target)
{
	if(is_local && from_server)
		return;

	Action& action = GetAction();
	if(action.sound)
		game->PlayAttachedSound(*unit, action.sound, action.sound_dist);

	if(!from_server)
	{
		UseActionCharge();
		if(action.cost > 0)
		{
			if(action.use_mana)
				unit->RemoveMana(action.cost);
			else
				unit->RemoveStamina(action.cost);
		}
	}

	if(action.use_cast)
	{
		// cast animation
		unit->action = A_CAST;
		unit->attack_id = -1;
		unit->animation_state = 0;
		unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
		if(is_local)
		{
			unit->target_pos = data.action_point;
			unit->action_unit = data.action_target;
		}
		else if(Net::IsServer())
		{
			unit->target_pos = *pos;
			unit->action_unit = target;
		}
	}

	Vec3 action_point;
	if(strcmp(action.id, "dash") == 0 || strcmp(action.id, "bull_charge") == 0)
	{
		bool dash = (strcmp(action.id, "dash") == 0);
		if(dash && Net::IsLocal())
			Train(TrainWhat::Dash, 0.f, 0);
		unit->action = A_DASH;
		unit->run_attack = false;
		if(Net::IsLocal() || !from_server)
		{
			unit->use_rot = Clip(data.action_rot + unit->rot + PI);
			action_point = Vec3(data.action_rot, 0, 0);
		}
		unit->animation = ANI_RUN;
		unit->current_animation = ANI_RUN;
		unit->mesh_inst->Play(NAMES::ani_run, PLAY_PRIO1 | PLAY_NO_BLEND, 0);
		if(dash)
		{
			unit->animation_state = 0;
			unit->timer = 0.33f;
			unit->speed = action.area_size.x / 0.33f;
			unit->mesh_inst->groups[0].speed = 3.f;
		}
		else
		{
			unit->animation_state = 1;
			unit->timer = 0.5f;
			unit->speed = action.area_size.x / 0.5f;
			unit->mesh_inst->groups[0].speed = 2.5f;
			action_targets.clear();
			unit->mesh_inst->groups[1].blend_max = 0.1f;
			unit->mesh_inst->Play("charge", PLAY_PRIO1, 1);
		}
	}
	else if(strcmp(action.id, "summon_wolf") == 0 || strcmp(action.id, "heal") == 0)
	{
		if(!from_server)
			action_point = data.action_point;
	}

	if(Net::IsOnline())
	{
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PLAYER_ACTION;
			c.unit = unit;
		}
		else if(!from_server)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PLAYER_ACTION;
			c.unit = data.action_target;
			c.pos = data.action_point;
		}
	}

	if(is_local)
		data.action_ready = false;
}

//=================================================================================================
void PlayerController::Update(float dt)
{
	if(data.wasted_key != Key::None && input->Up(data.wasted_key))
		data.wasted_key = Key::None;

	Unit* look_target = unit->look_target;
	if(game->dialog_context.dialog_mode || look_target || game_gui->inventory->mode > I_INVENTORY)
	{
		Vec3 pos;
		if(look_target)
		{
			pos = look_target->pos;
			unit->animation = ANI_STAND;
		}
		else if(game_gui->inventory->mode == I_LOOT_CHEST)
		{
			assert(action == PlayerAction::LootChest);
			pos = action_chest->pos;
			unit->animation = ANI_KNEELS;
		}
		else if(game_gui->inventory->mode == I_LOOT_BODY)
		{
			assert(action == PlayerAction::LootUnit);
			pos = action_unit->GetLootCenter();
			unit->animation = ANI_KNEELS;
		}
		else if(game_gui->inventory->mode == I_LOOT_CONTAINER)
		{
			assert(action == PlayerAction::LootContainer);
			pos = action_usable->pos;
			unit->animation = ANI_STAND;
		}
		else if(game->dialog_context.dialog_mode)
		{
			pos = game->dialog_context.talker->pos;
			unit->animation = ANI_STAND;
		}
		else
		{
			assert(action == InventoryModeToActionRequired(game_gui->inventory->mode));
			pos = action_unit->pos;
			unit->animation = ANI_STAND;
		}

		unit->RotateTo(pos, dt);
		if(game_gui->inventory->mode > I_INVENTORY)
		{
			if(game_gui->inventory->mode == I_LOOT_BODY)
			{
				if(action_unit->IsAlive())
				{
					// grabiony cel o¿y³
					game->CloseInventory();
				}
			}
			else if(game_gui->inventory->mode == I_TRADE || game_gui->inventory->mode == I_SHARE || game_gui->inventory->mode == I_GIVE)
			{
				if(!action_unit->IsStanding() || !action_unit->IsIdle())
				{
					// handlarz umar³ / zaatakowany
					game->CloseInventory();
				}
			}
		}
		else if(game->dialog_context.dialog_mode && Net::IsLocal())
		{
			if(!game->dialog_context.talker->IsStanding() || !game->dialog_context.talker->IsIdle() || game->dialog_context.talker->to_remove
				|| game->dialog_context.talker->frozen != FROZEN::NO)
			{
				// rozmówca umar³ / jest usuwany / zaatakowa³ kogoœ
				game->dialog_context.EndDialog();
			}
		}

		data.before_player = BP_NONE;
		data.rot_buf = 0.f;
		data.autowalk = false;
		data.action_ready = false;
	}
	else if(!IsBlocking(unit->action) && !unit->HaveEffect(EffectId::Stun))
	{
		bool allow_rot = true;
		if(unit->action == A_CAST && unit->action_unit != unit)
		{
			Vec3 pos;
			if(Unit* target = unit->action_unit)
				pos = target->pos;
			else
				pos = unit->target_pos;
			unit->RotateTo(pos, dt);
			allow_rot = false;
		}
		UpdateMove(dt, allow_rot);
	}
	else
	{
		data.before_player = BP_NONE;
		data.rot_buf = 0.f;
		data.autowalk = false;
		data.action_ready = false;
	}

	UpdateCooldown(dt);
	ClearShortcuts();

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

//=================================================================================================
void PlayerController::UpdateMove(float dt, bool allow_rot)
{
	Unit& u = *unit;
	LevelArea& area = *u.area;

	// unit is on ground
	if(!u.IsStanding())
	{
		data.autowalk = false;
		data.action_ready = false;
		data.rot_buf = 0.f;
		u.TryStandup(dt);
		return;
	}

	// unit is frozen
	if(u.frozen >= FROZEN::YES)
	{
		data.autowalk = false;
		data.action_ready = false;
		data.rot_buf = 0.f;
		if(u.frozen == FROZEN::YES)
			u.animation = ANI_STAND;
		return;
	}

	// using usable
	if(u.usable)
	{
		if(u.action == A_ANIMATION2 && OR2_EQ(u.animation_state, AS_ANIMATION2_USING, AS_ANIMATION2_USING_SOUND))
		{
			if(GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE) || GKey.KeyPressedReleaseAllowed(GK_USE))
				u.StopUsingUsable();
		}
		data.rot_buf = 0.f;
		data.action_ready = false;
	}

	u.prev_pos = u.pos;
	u.changed = true;

	bool idle = true;
	int move = 0;

	if(!u.usable)
	{
		if(u.weapon_taken == W_NONE)
		{
			if(u.animation != ANI_IDLE)
				u.animation = ANI_STAND;
		}
		else if(u.weapon_taken == W_ONE_HANDED)
			u.animation = ANI_BATTLE;
		else if(u.weapon_taken == W_BOW)
			u.animation = ANI_BATTLE_BOW;

		if(GKey.KeyPressedReleaseAllowed(GK_TOGGLE_WALK))
		{
			always_run = !always_run;
			if(Net::IsClient())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ALWAYS_RUN;
				c.id = (always_run ? 1 : 0);
			}
		}

		int rotate = 0;
		if(allow_rot)
		{
			if(GKey.KeyDownAllowed(GK_ROTATE_LEFT))
				--rotate;
			if(GKey.KeyDownAllowed(GK_ROTATE_RIGHT))
				++rotate;
		}

		if(u.frozen == FROZEN::NO)
		{
			bool cancel_autowalk = (GKey.KeyPressedReleaseAllowed(GK_MOVE_FORWARD) || GKey.KeyDownAllowed(GK_MOVE_BACK));
			if(cancel_autowalk)
				data.autowalk = false;
			else if(GKey.KeyPressedReleaseAllowed(GK_AUTOWALK))
				data.autowalk = !data.autowalk;

			if(u.run_attack)
			{
				move = 10;
				if(GKey.KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(GKey.KeyDownAllowed(GK_MOVE_LEFT))
					--move;
			}
			else
			{
				if(GKey.KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(GKey.KeyDownAllowed(GK_MOVE_LEFT))
					--move;
				if(GKey.KeyDownAllowed(GK_MOVE_FORWARD) || data.autowalk)
					move += 10;
				if(GKey.KeyDownAllowed(GK_MOVE_BACK))
					move -= 10;
			}
		}
		else
			data.autowalk = false;

		if(u.action == A_NONE && !u.talking && GKey.KeyPressedReleaseAllowed(GK_YELL))
		{
			if(Net::IsLocal())
				Yell();
			else
				Net::PushChange(NetChange::YELL);
		}

		if((GKey.allow_input == GameKeys::ALLOW_INPUT || GKey.allow_input == GameKeys::ALLOW_MOUSE) && !game_level->camera.free_rot && allow_rot)
		{
			int div = (u.action == A_SHOOT ? 800 : 400);
			data.rot_buf *= (1.f - dt * 2);
			data.rot_buf += float(input->GetMouseDif().x) * game->settings.mouse_sensitivity_f / div;
			if(data.rot_buf > 0.1f)
				data.rot_buf = 0.1f;
			else if(data.rot_buf < -0.1f)
				data.rot_buf = -0.1f;
		}
		else
			data.rot_buf = 0.f;

		const bool rotating = (rotate != 0 || data.rot_buf != 0.f);

		u.running = false;
		if(rotating || move)
		{
			// rotating by mouse don't affect idle timer
			if(move || rotate)
				idle = false;

			if(rotating)
			{
				const float rot_speed_dt = u.GetRotationSpeed() * dt;
				const float mouse_rot = Clamp(data.rot_buf, -rot_speed_dt, rot_speed_dt);
				const float val = rot_speed_dt * rotate + mouse_rot;

				data.rot_buf -= mouse_rot;
				u.rot = Clip(u.rot + Clamp(val, -rot_speed_dt, rot_speed_dt));

				if(val > 0)
					u.animation = ANI_RIGHT;
				else if(val < 0)
					u.animation = ANI_LEFT;
			}

			if(move)
			{
				// ustal k¹t i szybkoœæ ruchu
				float angle = u.rot;
				bool run = always_run;
				if(!u.run_attack)
				{
					if(GKey.KeyDownAllowed(GK_WALK))
						run = !run;
					if(!u.CanRun())
						run = false;
				}
				else
					run = true;

				switch(move)
				{
				case 10: // przód
					angle += PI;
					break;
				case -10: // ty³
					run = false;
					break;
				case -1: // lewa
					angle += PI / 2;
					break;
				case 1: // prawa
					angle += PI * 3 / 2;
					break;
				case 9: // lewa góra
					angle += PI * 3 / 4;
					break;
				case 11: // prawa góra
					angle += PI * 5 / 4;
					break;
				case -11: // lewy ty³
					run = false;
					angle += PI / 4;
					break;
				case -9: // prawy ty³
					run = false;
					angle += PI * 7 / 4;
					break;
				}

				if(u.action == A_SHOOT)
					run = false;

				if(run)
					u.animation = ANI_RUN;
				else if(move < -9)
					u.animation = ANI_WALK_BACK;
				else if(move == -1)
					u.animation = ANI_LEFT;
				else if(move == 1)
					u.animation = ANI_RIGHT;
				else
					u.animation = ANI_WALK;

				u.speed = run ? u.GetRunSpeed() : u.GetWalkSpeed();
				u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed)*dt * 3), 0.f, u.speed);
				float speed = u.prev_speed * dt;

				u.prev_pos = u.pos;

				const Vec3 dir(sin(angle)*speed, 0, cos(angle)*speed);
				Int2 prev_tile(int(u.pos.x / 2), int(u.pos.z / 2));
				bool moved = false;

				if(noclip)
				{
					u.pos += dir;
					moved = true;
				}
				else if(game_level->CheckMove(u.pos, dir, u.GetUnitRadius(), &u))
					moved = true;

				if(moved)
				{
					u.Moved();

					// train by moving
					if(Net::IsLocal())
						TrainMove(speed);

					// revealing minimap
					if(!game_level->location->outside)
					{
						Int2 new_tile(int(u.pos.x / 2), int(u.pos.z / 2));
						if(new_tile != prev_tile)
							FOV::DungeonReveal(new_tile, game_level->minimap_reveal);
					}
				}

				if(run && abs(u.speed - u.prev_speed) < 0.25f)
					u.running = true;
			}
		}

		if(move == 0)
		{
			u.prev_speed -= dt * 3;
			if(u.prev_speed < 0)
				u.prev_speed = 0;
		}
	}

	// shortcuts
	Shortcut shortcut;
	shortcut.type = Shortcut::TYPE_NONE;
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		if(shortcuts[i].type != Shortcut::TYPE_NONE
			&& (GKey.KeyPressedReleaseAllowed((GAME_KEYS)(GK_SHORTCUT1 + i)) || shortcuts[i].trigger))
		{
			shortcut = shortcuts[i];
			if(shortcut.type == Shortcut::TYPE_ITEM)
			{
				const Item* item = reinterpret_cast<const Item*>(shortcut.value);
				if(item->IsWearable())
				{
					if(u.HaveItemEquipped(item))
					{
						if(item->type == IT_WEAPON || item->type == IT_SHIELD)
						{
							shortcut.type = Shortcut::TYPE_SPECIAL;
							shortcut.value = Shortcut::SPECIAL_MELEE_WEAPON;
						}
						else if(item->type == IT_BOW)
						{
							shortcut.type = Shortcut::TYPE_SPECIAL;
							shortcut.value = Shortcut::SPECIAL_RANGED_WEAPON;
						}
					}
					else if(u.CanWear(item))
					{
						bool ignore_team = !team->HaveOtherActiveTeamMember();
						int i_index = u.FindItem([=](const ItemSlot& slot)
						{
							return slot.item == item && (slot.team_count == 0u || ignore_team);
						});
						if(i_index != Unit::INVALID_IINDEX)
						{
							ITEM_SLOT slot_type = ItemTypeToSlot(item->type);
							bool ok = true;
							if(u.SlotRequireHideWeapon(slot_type))
							{
								u.HideWeapon();
								if(u.action == A_TAKE_WEAPON)
								{
									ok = false;
									next_action = NA_EQUIP_DRAW;
									next_action_data.item = item;
									next_action_data.index = i_index;
									if(Net::IsClient())
										Net::PushChange(NetChange::SET_NEXT_ACTION);
								}
							}

							if(ok)
							{
								game_gui->inventory->inv_mine->EquipSlotItem(slot_type, i_index);
								if(item->type == IT_WEAPON || item->type == IT_SHIELD)
								{
									shortcut.type = Shortcut::TYPE_SPECIAL;
									shortcut.value = Shortcut::SPECIAL_MELEE_WEAPON;
								}
								else if(item->type == IT_BOW)
								{
									shortcut.type = Shortcut::TYPE_SPECIAL;
									shortcut.value = Shortcut::SPECIAL_RANGED_WEAPON;
								}
							}
						}
					}
				}
				else if(item->type == IT_CONSUMABLE)
				{
					int i_index = u.FindItem(item);
					if(i_index != Unit::INVALID_IINDEX)
						game_gui->inventory->inv_mine->ConsumeItem(i_index);
				}
				else if(item->type == IT_BOOK)
				{
					int i_index = u.FindItem(item);
					if(i_index != Unit::INVALID_IINDEX)
					{
						if(IsSet(item->flags, ITEM_MAGIC_SCROLL))
						{
							if(!u.usable) // can't use when sitting
								u.UseItem(i_index);
						}
						else
						{
							OpenPanel open = game_gui->level_gui->GetOpenPanel();
							if(open != OpenPanel::Inventory)
								game_gui->level_gui->ClosePanels();
							game_gui->book->Show((const Book*)item);
						}
					}
				}
			}
			break;
		}
	}

	if(u.action == A_NONE || u.action == A_TAKE_WEAPON || u.CanDoWhileUsing() || (u.action == A_ATTACK && u.animation_state == 0)
		|| (u.action == A_SHOOT && u.animation_state == 0))
	{
		if(GKey.KeyPressedReleaseAllowed(GK_TAKE_WEAPON))
		{
			if(Any(u.weapon_state, WeaponState::Taken, WeaponState::Taking))
			{
				u.HideWeapon();
				idle = false;
			}
			else
			{
				// decide which weapon to take out
				WeaponType weapon = last_weapon;
				if(weapon == W_NONE)
				{
					if(u.HaveWeapon())
						weapon = W_ONE_HANDED;
					else if(u.HaveBow())
						weapon = W_BOW;
				}
				else if(weapon == W_ONE_HANDED)
				{
					if(!u.HaveWeapon())
					{
						if(u.HaveBow())
							weapon = W_BOW;
						else
							weapon = W_NONE;
					}
				}
				else
				{
					if(!u.HaveBow())
					{
						if(u.HaveWeapon())
							weapon = W_ONE_HANDED;
						else
							weapon = W_NONE;
					}
				}

				if(weapon == W_NONE)
					game_gui->messages->AddGameMsg3(GMS_NEED_WEAPON);
				else
				{
					if(u.SetWeaponState(true, weapon, true))
					{
						idle = false;
						ClearNextAction();
					}
				}
			}
		}
		else if(u.HaveWeapon() && (shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_MELEE_WEAPON))
		{
			if(u.SetWeaponState(true, W_ONE_HANDED, true))
			{
				idle = false;
				ClearNextAction();
			}
		}
		else if(u.HaveBow() && (shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_RANGED_WEAPON))
		{
			if(u.SetWeaponState(true, W_BOW, true))
			{
				idle = false;
				ClearNextAction();
			}
		}

		if((shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_HEALING_POTION) && !Equal(u.hp, u.hpmax))
		{
			// drink healing potion
			int potion_index = GetHealingPotion();
			if(potion_index != -1)
			{
				idle = false;
				u.ConsumeItem(potion_index);
			}
			else
				game_gui->messages->AddGameMsg3(GMS_NO_POTION);
		}
	} // allow_input == ALLOW_INPUT || allow_input == ALLOW_KEYBOARD

	if(u.usable)
		return;

	// check what is in front of player
	data.before_player = BP_NONE;
	float dist, best_dist = 3.0f;

	// doors in front of player
	for(Door* door : area.doors)
	{
		if(OR2_EQ(door->state, Door::Open, Door::Closed))
			CheckObjectDistance(door->pos, door, best_dist, BP_DOOR);
	}

	if(!data.action_ready && u.action != A_CAST)
	{
		// units in front of player
		for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
		{
			Unit& u2 = **it;
			if(&u == &u2 || u2.to_remove)
				continue;

			dist = Vec3::Distance2d(u.visual_pos, u2.visual_pos);

			if(u2.IsStanding())
				CheckObjectDistance(u2.visual_pos, &u2, best_dist, BP_UNIT);
			else if(u2.live_state == Unit::FALL || u2.live_state == Unit::DEAD)
				CheckObjectDistance(u2.GetLootCenter(), &u2, best_dist, BP_UNIT);
		}

		// chests in front of player
		for(Chest* chest : area.chests)
		{
			if(!chest->GetUser())
				CheckObjectDistance(chest->pos, chest, best_dist, BP_CHEST);
		}

		// ground items in front of player
		for(GroundItem* ground_item : area.items)
			CheckObjectDistance(ground_item->pos, ground_item, best_dist, BP_ITEM);

		// usable objects in front of player
		for(Usable* usable : area.usables)
		{
			if(!usable->user)
				CheckObjectDistance(usable->pos, usable, best_dist, BP_USABLE);
		}
	}

	// use something in front of player
	if(u.frozen == FROZEN::NO && data.before_player != BP_NONE
		&& (GKey.KeyPressedReleaseAllowed(GK_USE) || (u.IsNotFighting() && GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE))))
	{
		idle = false;
		if(data.before_player == BP_UNIT)
		{
			Unit* u2 = data.before_player_ptr.unit;

			// przed graczem jest jakaœ postaæ
			if(u2->live_state == Unit::DEAD || u2->live_state == Unit::FALL)
			{
				// grabienie zw³ok
				if(u.action != A_NONE)
				{
				}
				else if(u2->live_state == Unit::FALL)
				{
					// nie mo¿na okradaæ osoby która zaraz wstanie
					game_gui->messages->AddGameMsg3(GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// nie mo¿na okradaæ sojuszników
					game_gui->messages->AddGameMsg3(GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->in_arena != -1)
					game_gui->messages->AddGameMsg3(GMS_DONT_LOOT_ARENA);
				else if(Net::IsLocal())
				{
					if(Net::IsOnline() && u2->busy == Unit::Busy_Looted)
					{
						// ktoœ ju¿ ograbia zw³oki
						game_gui->messages->AddGameMsg3(GMS_IS_LOOTED);
					}
					else
					{
						// rozpoczynij wymianê przedmiotów
						action = PlayerAction::LootUnit;
						action_unit = u2;
						u2->busy = Unit::Busy_Looted;
						chest_trade = &u2->items;
						game_gui->inventory->StartTrade(I_LOOT_BODY, *u2);
					}
				}
				else
				{
					// wiadomoœæ o wymianie do serwera
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::LOOT_UNIT;
					c.id = u2->id;
					action = PlayerAction::LootUnit;
					action_unit = u2;
					chest_trade = &u2->items;
				}
			}
			else if(u2->IsAI() && u2->IsIdle() && u2->in_arena == -1 && u2->data->dialog && !u.IsEnemy(*u2))
			{
				if(Net::IsLocal())
				{
					if(u2->busy != Unit::Busy_No || !u2->CanTalk(u))
					{
						// osoba jest czymœ zajêta
						game_gui->messages->AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// rozpocznij rozmowê
						game->dialog_context.StartDialog(u2);
						data.before_player = BP_NONE;
					}
				}
				else
				{
					// wiadomoœæ o rozmowie do serwera
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TALK;
					c.id = u2->id;
					action = PlayerAction::Talk;
					action_unit = u2;
					game->predialog.clear();
				}
			}
		}
		else if(data.before_player == BP_CHEST)
		{
			// pl¹drowanie skrzyni
			if(u.action != A_NONE)
			{
			}
			else if(Net::IsLocal())
			{
				// rozpocznij ograbianie skrzyni
				game_gui->inventory->StartTrade2(I_LOOT_CHEST, data.before_player_ptr.chest);
				data.before_player_ptr.chest->OpenClose(unit);
			}
			else
			{
				// wyœlij wiadomoœæ o pl¹drowaniu skrzyni
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_CHEST;
				c.id = data.before_player_ptr.chest->id;
				action = PlayerAction::LootChest;
				action_chest = data.before_player_ptr.chest;
				chest_trade = &action_chest->items;
				u.action = A_PREPARE;
			}
		}
		else if(data.before_player == BP_DOOR)
		{
			// otwieranie/zamykanie drzwi
			Door* door = data.before_player_ptr.door;
			if(door->state == Door::Closed)
			{
				// otwieranie drzwi
				if(door->locked == LOCK_NONE)
				{
					if(!game_level->location->outside)
						game_level->minimap_opened_doors = true;
					door->state = Door::Opening;
					door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
					if(Rand() % 2 == 0)
						sound_mgr->PlaySound3d(game_res->sDoor[Rand() % 3], door->GetCenter(), Door::SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::USE_DOOR;
						c.id = door->id;
						c.count = 0;
					}
				}
				else
				{
					cstring key;
					switch(door->locked)
					{
					case LOCK_MINE:
						key = "key_kopalnia";
						break;
					case LOCK_ORCS:
						key = "q_orkowie_klucz";
						break;
					case LOCK_UNLOCKABLE:
					default:
						key = nullptr;
						break;
					}

					Vec3 center = door->GetCenter();
					if(key && u.HaveItem(Item::Get(key)))
					{
						sound_mgr->PlaySound3d(game_res->sUnlock, center, Door::UNLOCK_SOUND_DIST);
						game_gui->messages->AddGameMsg3(GMS_UNLOCK_DOOR);
						if(!game_level->location->outside)
							game_level->minimap_opened_doors = true;
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
						if(Rand() % 2 == 0)
							sound_mgr->PlaySound3d(game_res->sDoor[Rand() % 3], center, Door::SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::USE_DOOR;
							c.id = door->id;
							c.count = 0;
						}
					}
					else
					{
						game_gui->messages->AddGameMsg3(GMS_NEED_KEY);
						sound_mgr->PlaySound3d(game_res->sDoorClosed[Rand() % 2], center, Door::SOUND_DIST);
					}
				}
			}
			else
			{
				// zamykanie drzwi
				door->state = Door::Closing;
				door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
				if(Rand() % 2 == 0)
				{
					Sound* sound;
					if(Rand() % 2 == 0)
						sound = game_res->sDoorClose;
					else
						sound = game_res->sDoor[Rand() % 3];
					sound_mgr->PlaySound3d(sound, door->GetCenter(), Door::SOUND_DIST);
				}
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::USE_DOOR;
					c.id = door->id;
					c.count = 1;
				}
			}
		}
		else if(data.before_player == BP_ITEM)
		{
			// podnieœ przedmiot
			GroundItem& item = *data.before_player_ptr.item;
			if(u.action == A_NONE)
			{
				bool up_anim = (item.pos.y > u.pos.y + 0.5f);

				u.action = A_PICKUP;
				u.animation = ANI_PLAY;
				u.mesh_inst->Play(up_anim ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);

				if(Net::IsLocal())
				{
					u.AddItem2(item.item, item.count, item.team_count, false);

					if(item.item->type == IT_GOLD)
						sound_mgr->PlaySound2d(game_res->sCoins);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::PICKUP_ITEM;
						c.unit = unit;
						c.count = (up_anim ? 1 : 0);

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::REMOVE_ITEM;
						c2.id = item.id;
					}

					RemoveElement(area.items, &item);
					data.before_player = BP_NONE;

					for(Event& event : game_level->location->events)
					{
						if(event.type == EVENT_PICKUP)
						{
							ScriptEvent e(EVENT_PICKUP);
							e.item = &item;
							event.quest->FireEvent(e);
						}
					}

					delete &item;
				}
				else
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::PICKUP_ITEM;
					c.id = item.id;

					data.picking_item = &item;
					data.picking_item_state = 1;
				}
			}
		}
		else if(u.action == A_NONE)
			UseUsable(data.before_player_ptr.usable, false);
	}

	// atak
	if(u.weapon_state == WeaponState::Taken && !data.action_ready)
	{
		idle = false;
		if(u.weapon_taken == W_ONE_HANDED)
		{
			if(u.action == A_ATTACK)
			{
				if(u.animation_state == 0)
				{
					if(GKey.KeyUpAllowed(action_key))
					{
						// release attack
						u.attack_power = u.mesh_inst->groups[1].time / u.GetAttackFrame(0);
						float speed = (u.attack_power + u.GetAttackSpeed()) * u.GetStaminaAttackSpeedMod();
						u.mesh_inst->groups[1].speed = speed;
						u.animation_state = 1;
						u.attack_power += 1.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_Attack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
							Train(TrainWhat::AttackStart, 0.f, 0);
					}
				}
				else if(u.animation_state == 2)
				{
					Key k = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::Down);
					if(k != Key::None)
					{
						// prepare next attack
						float speed = u.GetPowerAttackSpeed() * u.GetStaminaAttackSpeedMod();
						u.action = A_ATTACK;
						u.attack_id = u.GetRandomAttack();
						u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE, 1);
						u.mesh_inst->groups[1].speed = speed;
						action_key = k;
						u.animation_state = 0;
						u.run_attack = false;
						u.hitted = false;
						u.timer = 0.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_PrepareAttack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina);
					}
				}
			}
			else if(u.action == A_BLOCK)
			{
				if(GKey.KeyUpAllowed(action_key))
				{
					// stop blocking
					u.action = A_NONE;
					u.mesh_inst->Deactivate(1);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = unit;
						c.id = AID_StopBlock;
						c.f[1] = 1.f;
					}
				}
				else if(!u.mesh_inst->groups[1].IsBlending() && u.HaveShield())
				{
					if(GKey.KeyPressedUpAllowed(GK_ATTACK_USE))
					{
						// shield bash
						float speed = u.GetBashSpeed();
						u.action = A_BASH;
						u.animation_state = 0;
						u.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1, 1);
						u.mesh_inst->groups[1].speed = speed;
						u.hitted = false;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_Bash;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
						{
							Train(TrainWhat::BashStart, 0.f, 0);
							u.RemoveStamina(Unit::STAMINA_BASH_ATTACK);
						}
					}
				}
			}
			else if(u.action == A_NONE && u.frozen == FROZEN::NO && !GKey.KeyDownAllowed(GK_BLOCK))
			{
				Key k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &Input::Down, data.wasted_key);
				if(k != Key::None)
				{
					u.action = A_ATTACK;
					u.attack_id = u.GetRandomAttack();
					u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE, 1);
					if(u.running)
					{
						// running attack
						float speed = u.GetAttackSpeed() * u.GetStaminaAttackSpeedMod();
						u.mesh_inst->groups[1].speed = speed;
						u.animation_state = 1;
						u.run_attack = true;
						u.attack_power = 1.5f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_RunningAttack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
						{
							Train(TrainWhat::AttackStart, 0.f, 0);
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina * 1.5f);
						}
					}
					else
					{
						// prepare attack
						float speed = u.GetPowerAttackSpeed() * u.GetStaminaAttackSpeedMod();
						u.mesh_inst->groups[1].speed = speed;
						action_key = k;
						u.animation_state = 0;
						u.run_attack = false;
						u.timer = 0.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_PrepareAttack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina);
					}
					u.hitted = false;
				}
			}
			if(u.frozen == FROZEN::NO && u.HaveShield() && !u.run_attack && (u.action == A_NONE || u.action == A_ATTACK))
			{
				int oks = 0;
				if(u.action == A_ATTACK)
				{
					if(u.attack_power > 1.5f && u.animation_state == 1)
						oks = 1;
					else
						oks = 2;
				}

				if(oks != 1)
				{
					Key k = GKey.KeyDoReturnIgnore(GK_BLOCK, &Input::Down, data.wasted_key);
					if(k != Key::None)
					{
						// start blocking
						float blend_max = (oks == 2 ? 0.33f : u.GetBlockSpeed());
						u.action = A_BLOCK;
						u.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END, 1);
						u.mesh_inst->groups[1].blend_max = blend_max;
						action_key = k;
						u.animation_state = 0;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = unit;
							c.id = AID_Block;
							c.f[1] = blend_max;
						}
					}
				}
			}
		}
		else
		{
			// shoting from bow
			if(u.action == A_SHOOT)
			{
				if(u.animation_state == 0 && GKey.KeyUpAllowed(action_key))
				{
					u.animation_state = 1;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = unit;
						c.id = AID_Shoot;
						c.f[1] = 1.f;
					}
				}
			}
			else if(u.frozen == FROZEN::NO)
			{
				Key k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &Input::Down, data.wasted_key);
				if(k != Key::None)
				{
					float speed = u.GetBowAttackSpeed();
					u.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE, 1);
					u.mesh_inst->groups[1].speed = speed;
					u.action = A_SHOOT;
					u.animation_state = 0;
					u.hitted = false;
					action_key = k;
					u.bow_instance = game_level->GetBowInstance(u.GetBow().mesh);
					u.bow_instance->Play(&u.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
					u.bow_instance->groups[0].speed = speed;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = unit;
						c.id = AID_StartShoot;
						c.f[1] = speed;
					}

					if(Net::IsLocal())
						u.RemoveStamina(Unit::STAMINA_BOW_ATTACK);
				}
			}
		}
	}

	// action
	if(!data.action_ready)
	{
		if(u.frozen == FROZEN::NO && shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_ACTION)
		{
			if(CanUseAction())
			{
				data.action_ready = true;
				data.action_ok = false;
				data.action_rot = 0.f;
				data.action_target = nullptr;
			}
			else
				sound_mgr->PlaySound2d(game_res->sCancel);
		}
	}
	else
	{
		auto& action = GetAction();
		if(action.area == Action::LINE && IsSet(action.flags, Action::F_PICK_DIR))
		{
			// adjust action dir
			switch(move)
			{
			case 0: // none
			case 10: // forward
				data.action_rot = 0.f;
				break;
			case -10: // backward
				data.action_rot = PI;
				break;
			case -1: // left
				data.action_rot = -PI / 2;
				break;
			case 1: // right
				data.action_rot = PI / 2;
				break;
			case 9: // left forward
				data.action_rot = -PI / 4;
				break;
			case 11: // right forward
				data.action_rot = PI / 4;
				break;
			case -11: // left backward
				data.action_rot = -PI * 3 / 4;
				break;
			case -9: // prawy ty³
				data.action_rot = PI * 3 / 4;
				break;
			}
		}

		data.wasted_key = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::PressedRelease);
		if(data.wasted_key != Key::None)
		{
			if(data.action_ok)
				UseAction(false);
			else
				data.wasted_key = Key::None;
		}
		else
		{
			data.wasted_key = GKey.KeyDoReturn(GK_BLOCK, &Input::PressedRelease);
			if(data.wasted_key != Key::None)
				data.action_ready = false;
		}
	}

	// animacja idle
	if(idle && u.action == A_NONE)
	{
		idle_timer += dt;
		if(idle_timer >= 4.f)
		{
			if(u.animation == ANI_LEFT || u.animation == ANI_RIGHT)
				idle_timer = Random(2.f, 3.f);
			else
			{
				int id = Rand() % u.data->idles->anims.size();
				idle_timer = Random(0.f, 0.5f);
				u.mesh_inst->Play(u.data->idles->anims[id].c_str(), PLAY_ONCE, 0);
				u.animation = ANI_IDLE;

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::IDLE;
					c.unit = unit;
					c.id = id;
				}
			}
		}
	}
	else
		idle_timer = Random(0.f, 0.5f);
}

//=================================================================================================
void PlayerController::UpdateCooldown(float dt)
{
	Action& action = GetAction();
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
bool PlayerController::WantExitLevel()
{
	return !GKey.KeyDownAllowed(GK_WALK);
}

//=================================================================================================
void PlayerController::ClearNextAction()
{
	if(next_action != NA_NONE)
	{
		next_action = NA_NONE;
		if(Net::IsClient())
			Net::PushChange(NetChange::SET_NEXT_ACTION);
	}
}
