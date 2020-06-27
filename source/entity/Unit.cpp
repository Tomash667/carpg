#include "Pch.h"
#include "Unit.h"

#include "Ability.h"
#include "AIController.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "City.h"
#include "CombatHelper.h"
#include "Content.h"
#include "CraftPanel.h"
#include "EntityInterpolator.h"
#include "Game.h"
#include "GameFile.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "GroundItem.h"
#include "InsideLocation.h"
#include "Inventory.h"
#include "Level.h"
#include "LevelGui.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Mages.h"
#include "Quest_Scripted.h"
#include "Quest_Secret.h"
#include "SaveState.h"
#include "ScriptException.h"
#include "ScriptManager.h"
#include "Stock.h"
#include "Team.h"
#include "UnitEventHandler.h"
#include "UnitHelper.h"
#include "World.h"

#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <SoundManager.h>
#include <Terrain.h>

const float Unit::AUTO_TALK_WAIT = 0.333f;
const float Unit::STAMINA_BOW_ATTACK = 100.f;
const float Unit::STAMINA_BASH_ATTACK = 50.f;
const float Unit::STAMINA_UNARMED_ATTACK = 50.f;
const float Unit::STAMINA_RESTORE_TIMER = 0.5f;
const float Unit::EAT_SOUND_DIST = 0.5f;
const float Unit::DRINK_SOUND_DIST = 0.5f;
const float Unit::ATTACK_SOUND_DIST = 1.f;
const float Unit::TALK_SOUND_DIST = 1.f;
const float Unit::ALERT_SOUND_DIST = 2.f;
const float Unit::PAIN_SOUND_DIST = 1.f;
const float Unit::DIE_SOUND_DIST = 1.f;
const float Unit::YELL_SOUND_DIST = 2.f;
EntityType<Unit>::Impl EntityType<Unit>::impl;
static AIController* AI_PLACEHOLDER = (AIController*)1;

//=================================================================================================
Unit::~Unit()
{
	if(order)
		order->Free();
	if(bow_instance)
		game_level->FreeBowInstance(bow_instance);
	delete mesh_inst;
	delete human_data;
	delete hero;
	delete player;
	delete stock;
	if(stats && !stats->fixed)
		delete stats;
}

//=================================================================================================
void Unit::Release()
{
	if(--refs == 0)
	{
		assert(to_remove);
		delete this;
	}
}

//=================================================================================================
void Unit::Init(UnitData& base, int lvl)
{
	Register();
	fake_unit = true; // to prevent sending MP message set temporary as fake unit

	// stats
	data = &base;
	human_data = nullptr;
	pos = Vec3(0, 0, 0);
	rot = 0.f;
	used_item = nullptr;
	live_state = Unit::ALIVE;
	for(int i = 0; i < SLOT_MAX; ++i)
		slots[i] = nullptr;
	action = A_NONE;
	weapon_taken = W_NONE;
	weapon_hiding = W_NONE;
	weapon_state = WeaponState::Hidden;
	if(lvl == -2)
		level = base.level.Random();
	else if(lvl == -3)
		level = base.level.Clamp(game_level->location->st);
	else
		level = base.level.Clamp(lvl);
	player = nullptr;
	ai = nullptr;
	speed = prev_speed = 0.f;
	hurt_timer = 0.f;
	talking = false;
	usable = nullptr;
	frozen = FROZEN::NO;
	in_arena = -1;
	event_handler = nullptr;
	to_remove = false;
	temporary = false;
	quest_id = -1;
	bubble = nullptr;
	busy = Unit::Busy_No;
	interp = nullptr;
	dont_attack = false;
	assist = false;
	attack_team = false;
	last_bash = 0.f;
	alcohol = 0.f;
	moved = false;
	running = false;

	if(base.group == G_PLAYER)
	{
		stats = new UnitStats;
		stats->fixed = false;
		stats->subprofile.value = 0;
		stats->Set(base.GetStatProfile());
	}
	else
		stats = base.GetStats(level);
	CalculateStats();
	hp = hpmax = CalculateMaxHp();
	mp = mpmax = CalculateMaxMp();
	stamina = stamina_max = CalculateMaxStamina();
	stamina_timer = 0;

	// items
	weight = 0;
	CalculateLoad();
	if(base.group != G_PLAYER && base.item_script)
	{
		ItemScript* script = base.item_script;
		if(base.stat_profile && !base.stat_profile->subprofiles.empty() && base.stat_profile->subprofiles[stats->subprofile.index]->item_script)
			script = base.stat_profile->subprofiles[stats->subprofile.index]->item_script;
		script->Parse(*this);
		SortItems(items);
		RecalculateWeight();
	}
	if(base.trader)
	{
		stock = new TraderStock;
		stock->date = world->GetWorldtime();
		base.trader->stock->Parse(stock->items);
	}

	// gold
	float t;
	if(base.level.x == base.level.y)
		t = 1.f;
	else
		t = float(level - base.level.x) / (base.level.y - base.level.x);
	gold = Int2::Lerp(base.gold, base.gold2, t).Random();

	// human data
	if(base.type == UNIT_TYPE::HUMAN)
	{
		human_data = new Human;
		human_data->Init(base.appearance);
	}

	// hero data
	if(IsSet(base.flags, F_HERO))
	{
		hero = new Hero;
		hero->Init(*this);
	}
	else
		hero = nullptr;

	fake_unit = false;
}

//=================================================================================================
float Unit::CalculateMaxHp() const
{
	float maxhp = (float)data->hp + GetEffectSum(EffectId::Health);
	if(IsSet(data->flags2, F2_FIXED_STATS))
		maxhp += data->hp_lvl * (level - data->level.x);
	else
	{
		float v = 0.8f * Get(AttributeId::END) + 0.2f * Get(AttributeId::STR);
		if(v >= 50.f)
			maxhp += (v - 25) * 20.f;
		else
			maxhp += 500.f - (50.f - v) * 10.f;
	}
	return maxhp;
}

//=================================================================================================
float Unit::CalculateMaxMp() const
{
	float maxmp = (float)data->mp + GetEffectSum(EffectId::Mana);
	if(IsSet(data->flags2, F2_FIXED_STATS))
		maxmp += data->mp_lvl * (level - data->level.x);
	else
		maxmp += 4.f * (Get(AttributeId::WIS) - 50.f) + 4.f * Get(SkillId::CONCENTRATION);
	return maxmp;
}

//=================================================================================================
float Unit::GetMpRegen() const
{
	float value = mpmax / 100.f;
	value *= (1.f + GetEffectSum(EffectId::ManaRegeneration));
	return value;
}

//=================================================================================================
float Unit::CalculateMaxStamina() const
{
	float stamina = (float)data->stamina + GetEffectSum(EffectId::Stamina);
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return stamina;
	float v = 0.6f * Get(AttributeId::END) + 0.4f * Get(AttributeId::DEX);
	return stamina + 250.f + v * 2.f;
}

//=================================================================================================
float Unit::CalculateAttack() const
{
	if(HaveWeapon())
		return CalculateAttack(&GetWeapon());
	else if(IsSet(data->flags2, F2_FIXED_STATS))
		return (float)(data->attack + data->attack_lvl * (level - data->level.x));
	{
		float bonus = GetEffectSum(EffectId::MeleeAttack);
		return Get(SkillId::UNARMED) + (Get(AttributeId::STR) + Get(AttributeId::DEX)) / 2 - 25.f + bonus;
	}
}

//=================================================================================================
float Unit::CalculateAttack(const Item* weapon) const
{
	assert(weapon);

	float attack = (float)data->attack;

	if(IsSet(data->flags2, F2_FIXED_STATS))
	{
		if(weapon->type == IT_WEAPON)
			attack += weapon->ToWeapon().dmg;
		else if(weapon->type == IT_BOW)
			attack += weapon->ToBow().dmg;
		else
			attack += 0.5f * weapon->ToShield().block;
		return attack + data->attack_lvl * (level - data->level.x);
	}

	int str = Get(AttributeId::STR),
		dex = Get(AttributeId::DEX);

	if(weapon->type == IT_WEAPON)
	{
		const Weapon& w = weapon->ToWeapon();
		const WeaponTypeInfo& wi = WeaponTypeInfo::info[w.weapon_type];
		float p;
		if(str >= w.reqStr)
			p = 1.f;
		else
			p = float(str) / w.reqStr;
		attack += GetEffectSum(EffectId::MeleeAttack)
			+ wi.str2dmg * (str - 25)
			+ wi.dex2dmg * (dex - 25)
			+ w.dmg * p
			+ (Get(SkillId::ONE_HANDED_WEAPON) + Get(wi.skill)) / 2;
	}
	else if(weapon->type == IT_BOW)
	{
		const Bow& b = weapon->ToBow();
		float p;
		if(str >= b.reqStr)
			p = 1.f;
		else
			p = float(str) / b.reqStr;
		attack += GetEffectSum(EffectId::RangedAttack)
			+ float(dex - 25)
			+ b.dmg * p
			+ Get(SkillId::BOW);
	}
	else
	{
		const Shield& s = weapon->ToShield();
		float p;
		if(str >= s.reqStr)
			p = 1.f;
		else
			p = float(str) / s.reqStr;
		attack += GetEffectSum(EffectId::MeleeAttack)
			+ s.block * p
			+ s.attackMod * (Get(SkillId::SHIELD) + str - 25);
	}

	return attack;
}

//=================================================================================================
float Unit::CalculateBlock(const Item* shield) const
{
	if(!shield)
		shield = slots[SLOT_SHIELD];
	assert(shield && shield->type == IT_SHIELD);

	const Shield& s = shield->ToShield();
	float p;
	int str = Get(AttributeId::STR);
	if(str >= s.reqStr)
		p = 1.f;
	else
		p = float(str) / s.reqStr;

	return float(s.block) * p
		+ float(str - 25)
		+ Get(SkillId::SHIELD);
}

//=================================================================================================
float Unit::CalculateDefense(const Item* armor) const
{
	float def = (float)data->def + GetEffectSum(EffectId::Defense);
	if(!IsSet(data->flags2, F2_FIXED_STATS))
	{
		def += float(Get(AttributeId::END) - 25);

		if(!armor)
			armor = slots[SLOT_ARMOR];
		if(armor)
		{
			const Armor& a = armor->ToArmor();
			float skill_val = (float)Get(a.GetSkill());
			int str = Get(AttributeId::STR);
			if(str < a.reqStr)
				skill_val *= float(str) / a.reqStr;
			def += a.def + skill_val;
		}
	}
	else
		def += data->def_lvl * (level - data->level.x);
	return def;
}

//=================================================================================================
Unit::LoadState Unit::GetArmorLoadState(const Item* armor) const
{
	LoadState state = GetLoadState();
	if(armor)
	{
		SkillId skill = armor->ToArmor().GetSkill();
		if(skill == SkillId::HEAVY_ARMOR)
		{
			if(state < LS_HEAVY)
				state = LS_HEAVY;
		}
		else if(skill == SkillId::MEDIUM_ARMOR)
		{
			if(state < LS_MEDIUM)
				state = LS_MEDIUM;
		}
	}
	return state;
}

//=================================================================================================
void Unit::SetGold(int new_gold)
{
	if(new_gold == gold)
		return;
	int dif = new_gold - gold;
	gold = new_gold;
	if(IsPlayer())
	{
		game_gui->messages->AddFormattedMessage(player, GMS_GOLD_ADDED, -1, dif);
		if(player->is_local)
			sound_mgr->PlaySound2d(game_res->sCoins);
		else
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::SOUND;
			c.id = 0;
			player->player_info->UpdateGold();
		}
	}
}

//=================================================================================================
bool Unit::CanWear(const Item* item) const
{
	if(item->IsWearable())
	{
		if(item->type == IT_ARMOR)
			return item->ToArmor().armor_unit_type == data->armor_type;
		return true;
	}
	return false;
}

//=================================================================================================
bool Unit::WantItem(const Item* item) const
{
	if(item->type != IT_CONSUMABLE)
		return false;

	const Consumable& cons = item->ToConsumable();
	switch(cons.aiType)
	{
	case Consumable::AiType::None:
	default:
		return false;
	case Consumable::AiType::Healing:
		return true;
	case Consumable::AiType::Mana:
		return IsUsingMp();
	}
}

//=================================================================================================
bool Unit::DropItem(int index)
{
	bool no_more = false;

	ItemSlot& s = items[index];
	--s.count;
	weight -= s.item->weight;

	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->Register();
		item->item = s.item;
		item->count = 1;
		if(s.team_count > 0)
		{
			--s.team_count;
			item->team_count = 1;
		}
		else
			item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot) * 0.25f;
		item->pos.z -= cos(rot) * 0.25f;
		item->rot = Quat::RotY(Random(MAX_ANGLE));
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}
		if(!quest_mgr->quest_secret->CheckMoonStone(item, *this))
			game_level->AddGroundItem(*area, item);

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;
		}
	}
	else
	{
		if(s.team_count > 0)
			--s.team_count;
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}

		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_ITEM;
		c.id = index;
		c.count = 1;
	}

	return no_more;
}

//=================================================================================================
void Unit::DropItem(ITEM_SLOT slot)
{
	assert(slots[slot]);
	const Item*& item2 = slots[slot];

	weight -= item2->weight;

	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		RemoveItemEffects(item2, slot);

		GroundItem* item = new GroundItem;
		item->Register();
		item->item = item2;
		item->count = 1;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot) * 0.25f;
		item->pos.z -= cos(rot) * 0.25f;
		item->rot = Quat::RotY(Random(MAX_ANGLE));
		item2 = nullptr;
		game_level->AddGroundItem(*area, item);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;

			if(IsVisible(slot))
			{
				NetChange& c2 = Add1(Net::changes);
				c2.unit = this;
				c2.id = slot;
				c2.type = NetChange::CHANGE_EQUIPMENT;
			}
		}
	}
	else
	{
		item2 = nullptr;

		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_ITEM;
		c.id = SlotToIIndex(slot);
		c.count = 1;
	}
}

//=================================================================================================
bool Unit::DropItems(int index, uint count)
{
	bool no_more = false;

	ItemSlot& s = items[index];
	assert(count <= s.count);
	if(count == 0)
		count = s.count;
	s.count -= count;

	weight -= s.item->weight * count;

	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->Register();
		item->item = s.item;
		item->count = count;
		item->team_count = min(count, s.team_count);
		s.team_count -= item->team_count;
		item->pos = pos;
		item->pos.x -= sin(rot) * 0.25f;
		item->pos.z -= cos(rot) * 0.25f;
		item->rot = Quat::RotY(Random(MAX_ANGLE));
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}
		game_level->AddGroundItem(*area, item);

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;
		}
	}
	else
	{
		s.team_count -= min(count, s.team_count);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}

		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_ITEM;
		c.id = index;
		c.count = count;
	}

	return no_more;
}

//=================================================================================================
void Unit::RecalculateWeight()
{
	weight = 0;

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] && slots[i] != QUEST_ITEM_PLACEHOLDER)
			weight += slots[i]->weight;
	}

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item != QUEST_ITEM_PLACEHOLDER)
			weight += it->item->weight * it->count;
	}
}

//=================================================================================================
int Unit::ConsumeItem(int index)
{
	assert(index >= 0 && index < int(items.size()));

	ItemSlot& slot = items[index];
	assert(slot.item && slot.item->type == IT_CONSUMABLE);

	// jeœli coœ robi to nie mo¿e u¿yæ
	if(action != A_NONE &&
		!((action == A_ATTACK && animation_state == AS_ATTACK_PREPARE)
		|| (action == A_SHOOT && animation_state == AS_SHOOT_PREPARE)))
	{
		if(action == A_TAKE_WEAPON && weapon_state == WeaponState::Hiding)
		{
			// jeœli chowa broñ to u¿yj miksturki jak schowa
			if(IsPlayer())
			{
				if(player->is_local)
				{
					player->next_action = NA_CONSUME;
					player->next_action_data.index = index;
					player->next_action_data.item = slot.item;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
					return 2;
				}
				else
				{
					action = A_NONE;
					SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
				}
			}
			else
			{
				ai->potion = index;
				return 2;
			}
		}
		else if(!CanDoWhileUsing())
			return 3;
	}

	// jeœli broñ jest wyjêta to schowaj
	if(weapon_state != WeaponState::Hidden)
	{
		HideWeapon();
		if(IsPlayer())
		{
			assert(player->is_local);
			player->next_action = NA_CONSUME;
			player->next_action_data.index = index;
			player->next_action_data.item = slot.item;
			if(Net::IsClient())
				Net::PushChange(NetChange::SET_NEXT_ACTION);
		}
		else
			ai->potion = index;
		return 2;
	}

	const Consumable& cons = slot.item->ToConsumable();

	// usuñ przedmiot
	--slot.count;
	weight -= cons.weight;
	bool removed = false;
	if(slot.team_count)
	{
		--slot.team_count;
		used_item_is_team = true;
	}
	else
		used_item_is_team = false;
	if(slot.count == 0)
	{
		items.erase(items.begin() + index);
		removed = true;
	}

	ConsumeItemAnim(cons);

	// wyœlij komunikat
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CONSUME_ITEM;
		if(Net::IsServer())
		{
			c.unit = this;
			c.id = (int)used_item;
			c.count = 0;
		}
		else
			c.id = index;
	}

	return removed ? 0 : 1;
}

//=================================================================================================
void Unit::ConsumeItem(const Consumable& item, bool force, bool send)
{
	if(action != A_NONE && action != A_USE_USABLE)
	{
		if(Net::IsLocal())
		{
			assert(0);
			return;
		}
		else
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	}

	used_item_is_team = true;
	ConsumeItemAnim(item);

	if(send)
	{
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CONSUME_ITEM;
			c.unit = this;
			c.id = (int)&item;
			c.count = (force ? 1 : 0);
		}
	}
}

//=================================================================================================
void Unit::ConsumeItemS(const Item* item)
{
	if(!item || item->type != IT_CONSUMABLE)
		return;
	if(action != A_NONE && action != A_USE_USABLE)
		return;
	ConsumeItem(item->ToConsumable());
}

//=================================================================================================
void Unit::ConsumeItemAnim(const Consumable& cons)
{
	cstring anim_name;
	if(Any(cons.subtype, Consumable::Subtype::Food, Consumable::Subtype::Herb))
	{
		action = A_EAT;
		animation_state = AS_EAT_START;
		anim_name = "je";
	}
	else
	{
		action = A_DRINK;
		animation_state = AS_DRINK_START;
		anim_name = "pije";
	}

	if(current_animation == ANI_PLAY && !usable)
	{
		animation = ANI_STAND;
		current_animation = ANI_NONE;
	}

	mesh_inst->Play(anim_name, PLAY_ONCE | PLAY_PRIO1, 1);
	game_res->PreloadItem(&cons);
	used_item = &cons;
}

//=================================================================================================
void Unit::TakeWeapon(WeaponType type)
{
	assert(type == W_ONE_HANDED || type == W_BOW);

	if(!Any(action, A_NONE, A_TAKE_WEAPON))
		return;

	SetWeaponState(true, type, true);
}

//=================================================================================================
// Dodaje przedmiot(y) do ekwipunku, zwraca true jeœli przedmiot siê zestackowa³
//=================================================================================================
bool Unit::AddItem(const Item* item, uint count, uint team_count)
{
	assert(item && count != 0 && team_count <= count);

	if(item->type == IT_GOLD && team->IsTeamMember(*this))
	{
		if(Net::IsLocal())
		{
			if(team_count && IsTeamMember())
			{
				team->AddGold(team_count);
				uint normal_gold = count - team_count;
				if(normal_gold)
				{
					gold += normal_gold;
					if(IsPlayer() && !player->is_local)
						player->player_info->UpdateGold();
				}
			}
			else
			{
				gold += count;
				if(IsPlayer() && !player->is_local)
					player->player_info->UpdateGold();
			}
		}
		return true;
	}

	weight += item->weight * count;

	return InsertItem(items, item, count, team_count);
}

//=================================================================================================
void Unit::AddItem2(const Item* item, uint count, uint team_count, bool show_msg, bool notify)
{
	assert(item && count && team_count <= count);

	game_res->PreloadItem(item);

	AddItem(item, count, team_count);

	// multiplayer notify
	if(notify && Net::IsServer())
	{
		if(IsPlayer())
		{
			if(!player->is_local)
			{
				NetChangePlayer& c = Add1(player->player_info->changes);
				c.type = NetChangePlayer::ADD_ITEMS;
				c.item = item;
				c.count = count;
				c.id = team_count;
			}
		}
		else
		{
			// check if unit is trading with someone that gets this item
			Unit* u = nullptr;
			for(Unit& member : team->active_members)
			{
				if(member.IsPlayer() && member.player->IsTradingWith(this))
				{
					u = &member;
					break;
				}
			}

			if(u && !u->player->is_local)
			{
				NetChangePlayer& c = Add1(u->player->player_info->changes);
				c.type = NetChangePlayer::ADD_ITEMS_TRADER;
				c.item = item;
				c.id = id;
				c.count = count;
				c.a = team_count;
			}
		}
	}

	if(show_msg && IsPlayer())
		game_gui->messages->AddItemMessage(player, count);

	// rebuild inventory
	int rebuild_id = -1;
	if(IsLocalPlayer())
	{
		if(game_gui->inventory->inv_mine->visible || game_gui->inventory->gp_trade->visible)
			rebuild_id = 0;
	}
	else if(game_gui->inventory->gp_trade->visible && game_gui->inventory->inv_trade_other->unit == this)
		rebuild_id = 1;
	if(rebuild_id != -1)
		game_gui->inventory->BuildTmpInventory(rebuild_id);
}

//=================================================================================================
void Unit::AddEffect(Effect& e, bool send)
{
	effects.push_back(e);
	OnAddRemoveEffect(e);
	if(send && player && !player->IsLocal() && Net::IsServer())
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::ADD_EFFECT;
		c.id = (int)e.effect;
		c.count = (int)e.source;
		c.a1 = e.source_id;
		c.a2 = e.value;
		c.pos.x = e.power;
		c.pos.y = e.time;
	}
}

//=================================================================================================
void Unit::OnAddRemoveEffect(Effect& e)
{
	switch(e.effect)
	{
	case EffectId::Health:
		RecalculateHp();
		break;
	case EffectId::Carry:
		CalculateLoad();
		break;
	case EffectId::Stamina:
		RecalculateStamina();
		break;
	case EffectId::Attribute:
		ApplyStat((AttributeId)e.value);
		break;
	case EffectId::Skill:
		ApplyStat((SkillId)e.value);
		break;
	case EffectId::Mana:
		RecalculateMp();
		break;
	}
}

//=================================================================================================
void Unit::ApplyItemEffects(const Item* item, ITEM_SLOT slot)
{
	if(item->effects.empty())
		return;
	for(const ItemEffect& e : item->effects)
	{
		if(e.on_attack)
			continue;
		Effect effect;
		effect.effect = e.effect;
		effect.power = e.power;
		effect.source = EffectSource::Item;
		effect.source_id = (int)slot;
		effect.value = e.value;
		effect.time = 0.f;
		AddEffect(effect);
	}
}

//=================================================================================================
void Unit::RemoveItemEffects(const Item* item, ITEM_SLOT slot)
{
	if(item->effects.empty())
		return;
	RemoveEffects(EffectId::None, EffectSource::Item, (int)slot, -1);
}

//=================================================================================================
void Unit::ApplyConsumableEffect(const Consumable& item)
{
	for(const ItemEffect& effect : item.effects)
	{
		switch(effect.effect)
		{
		case EffectId::Heal:
			if(hp != hpmax)
			{
				hp += effect.power;
				if(hp > hpmax)
					hp = hpmax;
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = this;
				}
			}
			break;
		case EffectId::Poison:
		case EffectId::Alcohol:
			{
				float poison_res = GetPoisonResistance();
				if(poison_res > 0.f)
				{
					Effect e;
					e.effect = effect.effect;
					e.source = EffectSource::Temporary;
					e.source_id = -1;
					e.value = -1;
					e.time = item.time;
					e.power = effect.power / item.time * poison_res;
					AddEffect(e);
				}
			}
			break;
		case EffectId::Antidote:
			{
				uint index = 0;
				for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
				{
					if(it->effect == EffectId::Poison || it->effect == EffectId::Alcohol)
						_to_remove.push_back(index);
				}

				RemoveEffects();

				if(alcohol != 0.f)
				{
					alcohol = 0.f;
					if(IsPlayer() && !player->is_local)
						player->player_info->update_flags |= PlayerInfo::UF_ALCOHOL;
				}
			}
			break;
		case EffectId::FoodRegeneration:
			{
				Effect e;
				e.effect = effect.effect;
				e.source = EffectSource::Temporary;
				e.source_id = -1;
				e.value = -1;
				e.time = item.time;
				e.power = effect.power / item.time;
				AddEffect(e);
			}
			break;
		case EffectId::GreenHair:
			if(human_data)
			{
				human_data->hair_color = Vec4(0, 1, 0, 1);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::HAIR_COLOR;
					c.unit = this;
				}
			}
			break;
		case EffectId::RestoreMana:
			if(mp != mpmax)
			{
				mp += effect.power;
				if(mp > mpmax)
					mp = mpmax;
				if(Net::IsOnline() && IsTeamMember())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_MP;
					c.unit = this;
				}
			}
			break;
		default:
			if(item.time == 0.f && effect.effect == EffectId::Attribute)
				player->Train(false, effect.value, TrainMode::Potion);
			else
			{
				Effect e;
				e.effect = effect.effect;
				e.source = EffectSource::Temporary;
				e.source_id = -1;
				e.value = -1;
				e.time = item.time;
				e.power = effect.power;
				AddEffect(e);
			}
			break;
		}
	}
}

//=================================================================================================
uint Unit::RemoveEffects(EffectId effect, EffectSource source, int source_id, int value)
{
	uint index = 0;
	for(Effect& e : effects)
	{
		if((effect == EffectId::None || e.effect == effect)
			&& (value == -1 || e.value == value)
			&& (source == EffectSource::None || e.source == source)
			&& (source_id == -1 || e.source_id == source_id))
			_to_remove.push_back(index);
		++index;
	}

	uint count = _to_remove.size();
	RemoveEffects();
	return count;
}

//=================================================================================================
void Unit::UpdateEffects(float dt)
{
	float regen = 0.f, temp_regen = 0.f, poison_dmg = 0.f, alco_sum = 0.f, best_stamina = 0.f, stamina_mod = 1.f, food_heal = 0.f;

	// update effects timer
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == EffectId::NaturalHealingMod)
			continue;
		switch(it->effect)
		{
		case EffectId::Regeneration:
			if(it->source == EffectSource::Temporary && it->power > 0.f)
			{
				if(it->power > temp_regen)
					temp_regen = it->power;
			}
			else
				regen += it->power;
			break;
		case EffectId::Poison:
			poison_dmg += it->power;
			break;
		case EffectId::Alcohol:
			alco_sum += it->power;
			break;
		case EffectId::FoodRegeneration:
			food_heal += it->power;
			break;
		case EffectId::StaminaRegeneration:
			if(it->power > best_stamina)
				best_stamina = it->power;
			break;
		case EffectId::StaminaRegenerationMod:
			stamina_mod *= it->power;
			break;
		}
		if(it->source == EffectSource::Temporary)
		{
			if((it->time -= dt) <= 0.f)
				_to_remove.push_back(index);
		}
	}

	// remove expired effects
	RemoveEffects(false);

	if(Net::IsClient())
		return;

	// healing from food / regeneration
	if(hp != hpmax && (regen > 0 || temp_regen > 0 || food_heal > 0))
	{
		float natural = GetEffectMul(EffectId::NaturalHealingMod);
		hp += ((regen + temp_regen) + natural * food_heal) * dt;
		if(hp > hpmax)
			hp = hpmax;
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
	}

	// update alcohol value
	if(alco_sum > 0.f)
	{
		alcohol += alco_sum * dt;
		if(alcohol >= hpmax && live_state == ALIVE)
			Fall();
		if(IsPlayer() && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_ALCOHOL;
	}
	else if(alcohol != 0.f)
	{
		alcohol -= dt / 10 * Get(AttributeId::END);
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_ALCOHOL;
	}

	// update poison damage
	if(poison_dmg != 0.f)
		GiveDmg(poison_dmg * dt, nullptr, nullptr, DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(Net::IsOnline() && !player->is_local && player->last_dmg_poison != poison_dmg)
			player->player_info->update_flags |= PlayerInfo::UF_POISON_DAMAGE;
		player->last_dmg_poison = poison_dmg;
	}

	// restore mana
	if(mp != mpmax)
	{
		mp += GetMpRegen() * dt;
		if(mp > mpmax)
			mp = mpmax;
		if(Net::IsServer() && IsTeamMember())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_MP;
			c.unit = this;
		}
	}

	// restore stamina
	if(stamina_timer > 0)
	{
		stamina_timer -= dt;
		if(best_stamina > 0.f && stamina != stamina_max)
		{
			stamina += best_stamina * dt * stamina_mod;
			if(stamina > stamina_max)
				stamina = stamina_max;
			if(Net::IsServer() && IsTeamMember())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UPDATE_STAMINA;
				c.unit = this;
			}
		}
	}
	else if(stamina != stamina_max && (stamina_action != SA_DONT_RESTORE || best_stamina > 0.f))
	{
		float stamina_restore;
		switch(stamina_action)
		{
		case SA_RESTORE_MORE:
			stamina_restore = 66.66f;
			break;
		case SA_RESTORE:
		default:
			stamina_restore = 33.33f;
			break;
		case SA_RESTORE_SLOW:
			stamina_restore = 20.f;
			break;
		case SA_DONT_RESTORE:
			stamina_restore = 0.f;
			break;
		}
		switch(GetLoadState())
		{
		case LS_NONE:
		case LS_LIGHT:
		case LS_MEDIUM:
			break;
		case LS_HEAVY:
			stamina_restore -= 1.f;
			break;
		case LS_OVERLOADED:
			stamina_restore -= 2.5f;
			break;
		case LS_MAX_OVERLOADED:
			stamina_restore -= 5.f;
			break;
		}
		if(stamina_restore < 0)
			stamina_restore = 0;
		stamina += ((stamina_max * stamina_restore / 100) + best_stamina) * dt * stamina_mod;
		if(stamina > stamina_max)
			stamina = stamina_max;
		if(Net::IsServer() && IsTeamMember())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_STAMINA;
			c.unit = this;
		}
	}
}

//=================================================================================================
void Unit::EndEffects(int days, float* o_natural_mod)
{
	alcohol = 0.f;
	stamina = stamina_max;
	stamina_timer = 0;
	mp = mpmax;

	if(effects.empty())
	{
		if(o_natural_mod)
			*o_natural_mod = 1.f;
		return;
	}

	uint index = 0;
	float best_reg = 0.f, food = 0.f, natural_mod = 1.f, natural_tmp_mod = 1.f;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		bool remove = true;
		if(it->source != EffectSource::Temporary)
			remove = false;
		switch(it->effect)
		{
		case EffectId::Regeneration:
			if(it->source == EffectSource::Permanent)
				best_reg = -1.f;
			else if(best_reg >= 0.f)
			{
				float regen = it->power * it->time;
				if(regen > best_reg)
					best_reg = regen;
			}
			break;
		case EffectId::Poison:
			hp -= it->power * it->time;
			break;
		case EffectId::FoodRegeneration:
			food += it->power * it->time;
			break;
		case EffectId::NaturalHealingMod:
			if(it->source == EffectSource::Temporary)
			{
				float val = it->power * min((float)days, it->time) / days;
				if(val > natural_tmp_mod)
					natural_tmp_mod = val;
			}
			else
				natural_mod *= it->power;
			it->time -= days;
			if(it->time > 0.f)
				remove = false;
			break;
		}
		if(remove)
			_to_remove.push_back(index);
	}

	natural_mod *= natural_tmp_mod;
	if(o_natural_mod)
		*o_natural_mod = natural_mod;

	if(best_reg < 0.f)
		hp = hpmax; // hp regen from perk - full heal
	else
	{
		hp += best_reg + food * natural_mod;
		if(hp < 1.f)
			hp = 1.f;
		else if(hp > hpmax)
			hp = hpmax;
	}

	RemoveEffects(false);
}

//=================================================================================================
// Sum all permanent effects, for temporary sum negatives and use max positive
// So player can't drink 10 boost potions
float Unit::GetEffectSum(EffectId effect) const
{
	float value = 0.f,
		tmp_value = 0.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			if(e.source == EffectSource::Temporary && e.power > 0.f)
			{
				if(e.power > tmp_value)
					tmp_value = e.power;
			}
			else
				value += e.power;
		}
	}
	return value + tmp_value;
}

//=================================================================================================
float Unit::GetEffectMul(EffectId effect) const
{
	float value = 1.f,
		tmp_value_low = 1.f,
		tmp_value_high = 1.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			if(e.source == EffectSource::Temporary)
			{
				if(e.power > tmp_value_high)
					tmp_value_high = e.power;
				else if(e.power < tmp_value_low)
					tmp_value_low = e.power;
			}
			else
				value *= e.power;
		}
	}
	return value * tmp_value_low * tmp_value_high;
}

//=================================================================================================
float Unit::GetEffectMulInv(EffectId effect) const
{
	float value = 1.f,
		tmp_value_low = 1.f,
		tmp_value_high = 1.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			float power = (1.f - e.power);
			if(e.source == EffectSource::Temporary)
			{
				if(power > tmp_value_high)
					tmp_value_high = power;
				else if(power < tmp_value_low)
					tmp_value_low = power;
			}
			else
				value *= power;
		}
	}
	return value * tmp_value_low * tmp_value_high;
}

//=================================================================================================
float Unit::GetEffectMax(EffectId effect) const
{
	float value = 0.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect && e.power > value)
			value = e.power;
	}
	return value;
}

//=================================================================================================
// Dodaje przedmioty do ekwipunku i zak³ada je jeœli nie ma nic za³o¿onego. Dodane przedmioty s¹
// traktowane jako dru¿ynowe
//=================================================================================================
void Unit::AddItemAndEquipIfNone(const Item* item, uint count)
{
	assert(item && count != 0);

	if(item->IsStackable() || !CanWear(item))
	{
		AddItem(item, count, IsSet(item->flags, ITEM_NOT_TEAM) ? 0 : count);
		return;
	}

	ITEM_SLOT item_slot = ItemTypeToSlot(item->type);
	if(item_slot == SLOT_RING1 && slots[item_slot])
		item_slot = SLOT_RING2;

	if(!slots[item_slot])
	{
		slots[item_slot] = item;
		ApplyItemEffects(item, item_slot);
		--count;
	}

	if(count)
		AddItem(item, count, count);
}

//=================================================================================================
void Unit::CalculateLoad()
{
	if(IsSet(data->flags2, F2_FIXED_STATS))
		weight_max = 999;
	else
	{
		float mod = GetEffectMul(EffectId::Carry);
		weight_max = (int)(mod * (Get(AttributeId::STR) * 15));
	}
}

//=================================================================================================
void Unit::GetBox(Box& box) const
{
	float radius = GetUnitRadius();
	box.v1.x = pos.x - radius;
	box.v1.y = pos.y;
	box.v1.z = pos.z - radius;
	box.v2.x = pos.x + radius;
	box.v2.y = pos.y + max(MIN_H, GetUnitHeight());
	box.v2.z = pos.z + radius;
}

//=================================================================================================
int Unit::GetDmgType() const
{
	if(HaveWeapon())
		return GetWeapon().dmg_type;
	else
		return data->dmg_type;
}

//=================================================================================================
Vec3 Unit::GetLootCenter() const
{
	Mesh::Point* point2 = mesh_inst->mesh->GetPoint("centrum");

	if(!point2)
		return visual_pos;

	const Mesh::Point& point = *point2;
	Matrix matBone = point.mat * mesh_inst->mat_bones[point.bone];
	matBone = matBone * (Matrix::RotationY(rot) * Matrix::Translation(visual_pos));
	Vec3 center = Vec3::TransformZero(matBone);
	return center;
}

//=================================================================================================
float Unit::CalculateWeaponPros(const Weapon& weapon) const
{
	// oblicz dps i tyle na t¹ wersjê
	return CalculateAttack(&weapon) * GetAttackSpeed(&weapon);
}

//=================================================================================================
bool Unit::IsBetterWeapon(const Weapon& weapon, int* value, int* prev_value) const
{
	if(!HaveWeapon())
	{
		if(value)
		{
			*value = (int)CalculateWeaponPros(weapon);
			*prev_value = 0;
		}
		return true;
	}

	if(value)
	{
		float v = CalculateWeaponPros(weapon);
		float prev_v = CalculateWeaponPros(GetWeapon());
		*value = (int)v;
		*prev_value = (int)prev_v;
		return prev_v < v;
	}
	else
		return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(weapon);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& armor, int* value, int* prev_value) const
{
	if(!HaveArmor())
	{
		if(value)
		{
			*value = (int)CalculateDefense(&armor);
			*prev_value = 0;
		}
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&armor);
		float prev_v = CalculateDefense();
		*value = (int)v;
		*prev_value = (int)prev_v;
		return prev_v < v;
	}
	else
		return CalculateDefense() < CalculateDefense(&armor);
}

//=================================================================================================
void Unit::RecalculateHp()
{
	float hpp = hp / hpmax;
	hpmax = CalculateMaxHp();
	hp = hpmax * hpp;
	if(hp < 1)
		hp = 1;
	else if(hp > hpmax)
		hp = hpmax;
}

//=================================================================================================
void Unit::RecalculateMp()
{
	float mpp = mp / mpmax;
	mpmax = CalculateMaxMp();
	mp = mpmax * mpp;
	if(mp < 0)
		mp = 0;
	else if(mp > mpmax)
		mp = mpmax;
}

//=================================================================================================
void Unit::RecalculateStamina()
{
	float p = stamina / stamina_max;
	stamina_max = CalculateMaxStamina();
	stamina = stamina_max * p;
}

//=================================================================================================
float Unit::GetAttackFrame(int frame) const
{
	assert(InRange(frame, 0, 2));

	int attack_id = act.attack.index;
	assert(attack_id < data->frames->attacks);

	if(!data->frames->extra)
	{
		switch(frame)
		{
		case 0:
			return data->frames->t[F_ATTACK1_START + attack_id * 2 + 0];
		case 1:
			return data->frames->Lerp(F_ATTACK1_START + attack_id * 2);
		case 2:
			return data->frames->t[F_ATTACK1_START + attack_id * 2 + 1];
		default:
			assert(0);
			return data->frames->t[F_ATTACK1_START + attack_id * 2 + 1];
		}
	}
	else
	{
		switch(frame)
		{
		case 0:
			return data->frames->extra->e[attack_id].start;
		case 1:
			return data->frames->extra->e[attack_id].Lerp();
		case 2:
			return data->frames->extra->e[attack_id].end;
		default:
			assert(0);
			return data->frames->extra->e[attack_id].end;
		}
	}
}

//=================================================================================================
int Unit::GetRandomAttack() const
{
	if(data->frames->extra)
	{
		int a;

		switch(GetWeapon().weapon_type)
		{
		case WT_LONG_BLADE:
			a = A_LONG_BLADE;
			break;
		case WT_SHORT_BLADE:
			a = A_SHORT_BLADE;
			break;
		case WT_BLUNT:
			a = A_BLUNT;
			break;
		case WT_AXE:
			a = A_AXE;
			break;
		default:
			assert(0);
			a = A_LONG_BLADE;
			break;
		}

		do
		{
			int n = Rand() % data->frames->attacks;
			if(IsSet(data->frames->extra->e[n].flags, a))
				return n;
		}
		while(1);
	}
	else
		return Rand() % data->frames->attacks;
}

//=================================================================================================
void Unit::Save(GameWriter& f)
{
	f << id;
	f << data->id;

	// items
	for(uint i = 0; i < SLOT_MAX; ++i)
		f.WriteOptional(slots[i]);
	f << items.size();
	for(ItemSlot& slot : items)
	{
		if(slot.item)
		{
			f << slot.item->id;
			f << slot.count;
			f << slot.team_count;
			if(slot.item->id[0] == '$')
				f << slot.item->quest_id;
		}
		else
			f.Write0();
	}
	if(stock)
	{
		if(f.isLocal || world->GetWorldtime() - stock->date < 10)
		{
			f.Write1();
			SaveStock(f);
		}
		else
		{
			delete stock;
			stock = nullptr;
			f.Write0();
		}
	}
	else
		f.Write0();

	f << live_state;
	f << pos;
	f << rot;
	f << hp;
	f << hpmax;
	f << mp;
	f << mpmax;
	f << stamina;
	f << stamina_max;
	f << stamina_action;
	f << stamina_timer;
	f << level;
	if(IsPlayer())
		stats->Save(f);
	else
		f << stats->subprofile;
	f << gold;
	f << to_remove;
	f << temporary;
	f << quest_id;
	f << assist;
	f << dont_attack;
	f << attack_team;
	f << (event_handler ? event_handler->GetUnitEventHandlerQuestId() : -1);
	f << weight;
	f << summoner;
	if(live_state >= DYING)
		f << mark;

	assert((human_data != nullptr) == (data->type == UNIT_TYPE::HUMAN));
	if(human_data)
	{
		f.Write1();
		human_data->Save(f);
	}
	else
		f.Write0();

	if(f.isLocal)
	{
		mesh_inst->Save(f);
		f << animation;
		f << current_animation;

		f << prev_pos;
		f << speed;
		f << prev_speed;
		f << animation_state;
		f << action;
		f << weapon_taken;
		f << weapon_hiding;
		f << weapon_state;
		f << hurt_timer;
		f << target_pos;
		f << target_pos2;
		f << talking;
		f << talk_timer;
		f << timer;
		f << alcohol;
		f << raise_timer;

		switch(action)
		{
		case A_ATTACK:
			f << act.attack.index;
			f << act.attack.power;
			f << act.attack.run;
			f << act.attack.hitted;
			break;
		case A_CAST:
			f << act.cast.ability->hash;
			f << act.cast.target;
			break;
		case A_DASH:
			f << act.dash.ability->hash;
			f << act.dash.rot;
			break;
		case A_USE_USABLE:
			f << act.use_usable.rot;
			break;
		}

		if(used_item)
		{
			f << used_item->id;
			f << used_item_is_team;
		}
		else
			f.Write0();

		if(usable && !usable->container)
		{
			if(usable->user != this)
			{
				Warn("Invalid usable %s (%d) user %s.", usable->base->id.c_str(), usable->id, data->id.c_str());
				usable = nullptr;
				f << -1;
			}
			else
				f << usable->id;
		}
		else
			f << -1;

		f << last_bash;
		f << moved;
		f << running;
	}

	f.WriteVector4(effects);

	// dialogs
	f.Write(dialogs.size());
	for(QuestDialog& dialog : dialogs)
	{
		f << dialog.quest->id;
		f << dialog.dialog->id;
		f << dialog.priority;
	}

	// events
	f << events.size();
	for(Event& e : events)
	{
		f << e.type;
		f << e.quest->id;
	}

	// orders
	UnitOrderEntry* current_order = order;
	while(current_order)
	{
		f.Write1();
		f << current_order->order;
		f << current_order->timer;
		switch(current_order->order)
		{
		case ORDER_FOLLOW:
		case ORDER_GUARD:
			f << current_order->unit;
			break;
		case ORDER_LOOK_AT:
			f << current_order->pos;
			break;
		case ORDER_MOVE:
			f << current_order->pos;
			f << current_order->move_type;
			f << current_order->range;
			break;
		case ORDER_ESCAPE_TO:
			f << current_order->pos;
			break;
		case ORDER_ESCAPE_TO_UNIT:
			f << current_order->unit;
			f << current_order->pos;
			break;
		case ORDER_AUTO_TALK:
			f << current_order->auto_talk;
			if(current_order->auto_talk_dialog)
			{
				f << current_order->auto_talk_dialog->id;
				f << (current_order->auto_talk_quest ? current_order->auto_talk_quest->id : -1);
			}
			else
				f.Write0();
			break;
		}
		current_order = current_order->next;
	}
	f.Write0();

	if(player)
	{
		f.Write1();
		player->Save(f);
	}
	else
		f.Write0();

	if(hero)
		hero->Save(f);
}

//=================================================================================================
void Unit::SaveStock(GameWriter& f)
{
	vector<ItemSlot>& cnt = stock->items;

	f << stock->date;
	f << cnt.size();
	for(ItemSlot& slot : cnt)
	{
		if(slot.item)
		{
			f << slot.item->id;
			f << slot.count;
			if(slot.item->id[0] == '$')
				f << slot.item->quest_id;
		}
		else
			f.Write0();
	}
}

//=================================================================================================
void Unit::Load(GameReader& f)
{
	fake_unit = true; // to prevent sending MP message set temporary as fake unit
	human_data = nullptr;

	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	data = UnitData::Get(f.ReadString1());

	// items
	bool can_sort = true;
	int max_slots;
	if(LOAD_VERSION >= V_0_10)
		max_slots = SLOT_MAX;
	else if(LOAD_VERSION >= V_0_9)
		max_slots = 5;
	else
		max_slots = 4;
	for(int i = 0; i < max_slots; ++i)
		f.ReadOptional(slots[i]);
	if(LOAD_VERSION < V_0_9)
		slots[SLOT_AMULET] = nullptr;
	if(LOAD_VERSION < V_0_10)
	{
		slots[SLOT_RING1] = nullptr;
		slots[SLOT_RING2] = nullptr;
	}
	items.resize(f.Read<uint>());
	for(ItemSlot& slot : items)
	{
		const string& item_id = f.ReadString1();
		f >> slot.count;
		f >> slot.team_count;
		if(item_id[0] != '$')
			slot.item = Item::Get(item_id);
		else
		{
			int quest_item_id = f.Read<int>();
			quest_mgr->AddQuestItemRequest(&slot.item, item_id.c_str(), quest_item_id, &items, this);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}
	if(LOAD_VERSION >= V_0_8)
	{
		if(f.Read0())
			stock = nullptr;
		else
			LoadStock(f);
	}

	// stats
	f >> live_state;
	f >> pos;
	f >> rot;
	f >> hp;
	f >> hpmax;
	if(LOAD_VERSION >= V_0_12)
	{
		f >> mp;
		f >> mpmax;
	}
	f >> stamina;
	f >> stamina_max;
	f >> stamina_action;
	f >> stamina_timer;
	f >> level;
	if(content.require_update && data->group != G_PLAYER)
	{
		if(data->upgrade)
		{
			// upgrade unit - previously there was 'mage' unit, now it is split into 'mage novice', 'mage' and 'master mage'
			// calculate which one to use
			int best_dif = data->GetLevelDif(level);
			for(UnitData* u : *data->upgrade)
			{
				int dif = u->GetLevelDif(level);
				if(dif < best_dif)
				{
					best_dif = dif;
					data = u;
				}
			}
		}
		level = data->level.Clamp(level);
	}
	if(LOAD_VERSION >= V_0_8)
	{
		if(data->group == G_PLAYER)
		{
			stats = new UnitStats;
			stats->fixed = false;
			stats->subprofile.value = 0;
			stats->Load(f);
			if(LOAD_VERSION < V_0_10)
			{
				for(int i = 0; i < (int)SkillId::MAX; ++i)
				{
					if(stats->skill[i] == -1)
						stats->skill[i] = 0;
				}
			}
		}
		else
		{
			SubprofileInfo sub;
			f >> sub;
			stats = data->GetStats(sub);
		}
	}
	else
	{
		UnitStats::Skip(f); // old temporary stats
		if(data->group == G_PLAYER)
		{
			stats = new UnitStats;
			stats->fixed = false;
			stats->subprofile.value = 0;
			stats->Load(f);
			stats->skill[(int)SkillId::HAGGLE] = UnitStats::NEW_STAT;
		}
		else
		{
			stats = data->GetStats(level);
			UnitStats::Skip(f);
		}
	}
	f >> gold;
	bool old_invisible = false;
	if(LOAD_VERSION < V_0_10)
		f >> old_invisible;
	if(LOAD_VERSION < V_0_11)
		f.Skip<int>(); // old inside_building
	f >> to_remove;
	f >> temporary;
	f >> quest_id;
	f >> assist;

	AutoTalkMode old_auto_talk = AutoTalkMode::No;
	GameDialog* old_auto_talk_dialog = nullptr;
	float old_auto_talk_timer = 0;
	if(LOAD_VERSION < V_0_12)
	{
		// old auto talk
		f >> old_auto_talk;
		if(old_auto_talk != AutoTalkMode::No)
		{
			if(const string& dialog_id = f.ReadString1(); !dialog_id.empty())
				old_auto_talk_dialog = GameDialog::TryGet(dialog_id.c_str());
			else
				old_auto_talk_dialog = nullptr;
			f >> old_auto_talk_timer;
		}
	}

	f >> dont_attack;
	f >> attack_team;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	int unit_event_handler_quest_id = f.Read<int>();
	if(unit_event_handler_quest_id == -2)
		event_handler = quest_mgr->quest_contest;
	else if(unit_event_handler_quest_id == -1)
		event_handler = nullptr;
	else
	{
		event_handler = reinterpret_cast<UnitEventHandler*>(unit_event_handler_quest_id);
		game->load_unit_handler.push_back(this);
	}
	if(can_sort && content.require_update)
		SortItems(items);
	f >> weight;
	if(can_sort && content.require_update)
		RecalculateWeight();

	Entity<Unit> guard_target;
	if(LOAD_VERSION < V_0_12)
		f >> guard_target;
	f >> summoner;

	if(live_state >= DYING)
	{
		if(LOAD_VERSION >= V_0_8)
			f >> mark;
		else
		{
			for(ItemSlot& item : items)
			{
				if(item.item && IsSet(item.item->flags, ITEM_IMPORTANT))
				{
					mark = true;
					break;
				}
			}
		}
	}

	bubble = nullptr; // ustawianie przy wczytaniu SpeechBubble
	changed = false;
	busy = Busy_No;
	visual_pos = pos;

	if(f.Read1())
	{
		human_data = new Human;
		human_data->Load(f);
	}
	else
	{
		assert(data->type != UNIT_TYPE::HUMAN);
		human_data = nullptr;
	}

	if(f.isLocal)
	{
		float old_attack_power = 1.f;
		bool old_run_attack = false, old_hitted = false;

		CreateMesh(CREATE_MESH::LOAD);
		mesh_inst->Load(f, LOAD_VERSION >= V_0_13 ? 1 : 0);
		f >> animation;
		f >> current_animation;

		f >> prev_pos;
		f >> speed;
		f >> prev_speed;
		f >> animation_state;
		if(LOAD_VERSION < V_0_13)
			f >> ai_mode; // old attack_id, assigned to unused variable at client side to pass to AIController
		f >> action;
		f >> weapon_taken;
		f >> weapon_hiding;
		f >> weapon_state;
		if(LOAD_VERSION < V_0_13)
			f >> old_hitted;
		f >> hurt_timer;
		f >> target_pos;
		f >> target_pos2;
		f >> talking;
		f >> talk_timer;
		if(LOAD_VERSION < V_0_13)
		{
			f >> old_attack_power;
			f >> old_run_attack;
		}
		f >> timer;
		f >> alcohol;
		f >> raise_timer;

		switch(action)
		{
		case A_ATTACK:
			if(LOAD_VERSION >= V_0_13)
			{
				f >> act.attack.index;
				f >> act.attack.power;
				f >> act.attack.run;
				f >> act.attack.hitted;
			}
			else
			{
				act.attack.index = ai_mode;
				act.attack.power = old_attack_power;
				act.attack.run = old_run_attack;
				act.attack.hitted = old_hitted;
			}
			break;
		case A_CAST:
			if(LOAD_VERSION >= V_0_13)
			{
				act.cast.ability = Ability::Get(f.Read<int>());
				f >> act.cast.target;
			}
			else
			{
				if(LOAD_VERSION >= V_0_12)
					f >> act.cast.target;
				else
					act.cast.target = nullptr;
				if(data->group == G_PLAYER)
					act.cast.ability = data->clas->ability;
				else
					act.cast.ability = data->abilities->ability[ai_mode];
			}
			break;
		case A_DASH:
			if(LOAD_VERSION >= V_0_13)
				act.dash.ability = Ability::Get(f.Read<int>());
			else
				act.dash.ability = Ability::Get(animation_state == 0 ? "dash" : "bull_charge");
			f >> act.dash.rot;
			break;
		case A_USE_USABLE:
			if(LOAD_VERSION >= V_0_10)
				f >> act.use_usable.rot;
			else
				act.use_usable.rot = 0.f;
			break;
		}

		const string& item_id = f.ReadString1();
		if(!item_id.empty())
		{
			used_item = Item::Get(item_id);
			f >> used_item_is_team;
		}
		else
			used_item = nullptr;

		int usable_id = f.Read<int>();
		if(usable_id == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usable_id);

		if(action == A_SHOOT)
		{
			bow_instance = game_level->GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = mesh_inst->groups[1].speed;
			bow_instance->groups[0].time = Min(mesh_inst->groups[1].time, bow_instance->groups[0].anim->length);
		}

		f >> last_bash;
		f >> moved;
		if(LOAD_VERSION >= V_0_9)
			f >> running;
		else
			running = false;
	}
	else
	{
		mesh_inst = nullptr;
		ai = nullptr;
		usable = nullptr;
		used_item = nullptr;
		weapon_state = WeaponState::Hidden;
		weapon_taken = W_NONE;
		weapon_hiding = W_NONE;
		frozen = FROZEN::NO;
		talking = false;
		animation = current_animation = ANI_STAND;
		action = A_NONE;
		hurt_timer = 0.f;
		speed = prev_speed = 0.f;
		alcohol = 0.f;
		moved = false;
		running = false;
	}

	// effects
	if(LOAD_VERSION >= V_0_10)
	{
		f.ReadVector4(effects);
		if(LOAD_VERSION < V_0_14)
		{
			for(Effect& e : effects)
			{
				if(e.source == EffectSource::Perk)
					e.source_id = old::Convert((old::v2::Perk)e.source_id)->hash;
			}
		}
	}
	else if(LOAD_VERSION >= V_0_8)
	{
		effects.resize(f.Read<uint>());
		for(Effect& e : effects)
		{
			f >> e.effect;
			f >> e.source;
			f >> e.source_id;
			f >> e.time;
			f >> e.power;
			e.value = -1;
			if(e.source == EffectSource::Perk)
				e.source_id = old::Convert((old::v2::Perk)e.source_id)->hash;
		}
	}
	else
	{
		effects.resize(f.Read<uint>());
		for(Effect& e : effects)
		{
			e.effect = (EffectId)(f.Read<int>() - 1); // changed None effect from 0 to -1
			e.time = f.Read<float>();
			e.power = f.Read<float>();
			e.source = EffectSource::Temporary;
			e.source_id = -1;
		}
	}
	if(content.require_update)
	{
		RemoveEffects(EffectId::None, EffectSource::Item, -1, -1);
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i])
				ApplyItemEffects(slots[i], (ITEM_SLOT)i);
		}
	}

	if(LOAD_VERSION >= V_0_9)
	{
		// dialogs
		dialogs.resize(f.Read<uint>());
		for(QuestDialog& dialog : dialogs)
		{
			int quest_id = f.Read<int>();
			string* str = StringPool.Get();
			f >> *str;
			quest_mgr->AddQuestRequest(quest_id, (Quest**)&dialog.quest, [this, &dialog, str]()
			{
				dialog.dialog = dialog.quest->GetDialog(*str);
				StringPool.Free(str);
				dialog.quest->AddDialogPtr(this);
			});
			if(LOAD_VERSION >= V_0_14)
				f >> dialog.priority;
			else
				dialog.priority = 0;
		}
	}
	if(LOAD_VERSION >= V_0_10)
	{
		// events
		events.resize(f.Read<uint>());
		for(Event& e : events)
		{
			int quest_id;
			f >> e.type;
			f >> quest_id;
			quest_mgr->AddQuestRequest(quest_id, (Quest**)&e.quest, [&]()
			{
				EventPtr event;
				event.source = EventPtr::UNIT;
				event.type = e.type;
				event.unit = this;
				e.quest->AddEventPtr(event);
			});
		}

		// orders
		if(LOAD_VERSION >= V_0_12)
		{
			UnitOrderEntry* current_order = nullptr;
			while(f.Read1())
			{
				if(current_order)
				{
					current_order->next = UnitOrderEntry::Get();
					current_order = current_order->next;
				}
				else
				{
					order = UnitOrderEntry::Get();
					current_order = order;
				}

				f >> current_order->order;
				f >> current_order->timer;
				switch(current_order->order)
				{
				case ORDER_FOLLOW:
					f >> current_order->unit;
					break;
				case ORDER_LOOK_AT:
					f >> current_order->pos;
					break;
				case ORDER_MOVE:
					f >> current_order->pos;
					f >> current_order->move_type;
					if(LOAD_VERSION >= V_0_14)
						f >> current_order->range;
					else
						current_order->range = 0.1f;
					break;
				case ORDER_ESCAPE_TO:
					f >> current_order->pos;
					break;
				case ORDER_ESCAPE_TO_UNIT:
					f >> current_order->unit;
					f >> current_order->pos;
					break;
				case ORDER_GUARD:
					f >> current_order->unit;
					break;
				case ORDER_AUTO_TALK:
					f >> current_order->auto_talk;
					if(const string& dialog_id = f.ReadString1(); !dialog_id.empty())
					{
						current_order->auto_talk_dialog = GameDialog::TryGet(dialog_id.c_str());
						int quest_id;
						f >> quest_id;
						if(quest_id != -1)
							quest_mgr->AddQuestRequest(quest_id, &current_order->auto_talk_quest);
						else
							current_order->auto_talk_quest = nullptr;
					}
					else
					{
						current_order->auto_talk_dialog = nullptr;
						current_order->auto_talk_quest = nullptr;
					}
					break;
				}
			}
		}
		else
		{
			UnitOrder unit_order;
			float timer;
			f >> unit_order;
			f >> timer;
			if(unit_order != ORDER_NONE)
			{
				order = UnitOrderEntry::Get();
				order->order = unit_order;
				order->timer = timer;
				switch(order->order)
				{
				case ORDER_FOLLOW:
					if(LOAD_VERSION >= V_0_11)
						f >> order->unit;
					else
						team->GetLeaderRequest(&order->unit);
					break;
				case ORDER_LOOK_AT:
					f >> order->pos;
					break;
				case ORDER_MOVE:
					f >> order->pos;
					f >> order->move_type;
					order->range = 0.1f;
					break;
				case ORDER_ESCAPE_TO:
					f >> order->pos;
					break;
				case ORDER_ESCAPE_TO_UNIT:
					f >> order->unit;
					f >> order->pos;
					break;
				case ORDER_GUARD:
					f >> order->unit;
					break;
				}
			}
		}
	}
	if(guard_target)
	{
		if(order)
			order->Free();
		order = UnitOrderEntry::Get();
		order->order = ORDER_GUARD;
		order->unit = guard_target;
		order->timer = 0.f;
	}
	if(old_auto_talk != AutoTalkMode::No)
	{
		if(order)
			order->Free();
		order = UnitOrderEntry::Get();
		order->order = ORDER_AUTO_TALK;
		order->timer = old_auto_talk_timer;
		order->auto_talk = old_auto_talk;
		order->auto_talk_dialog = old_auto_talk_dialog;
		order->auto_talk_quest = nullptr;
	}

	if(f.Read1())
	{
		player = new PlayerController;
		player->unit = this;
		player->Load(f);
		if(LOAD_VERSION < V_0_10)
			player->invisible = old_invisible;
	}
	else
		player = nullptr;

	if(f.isLocal && human_data)
		human_data->ApplyScale(mesh_inst);

	if(IsSet(data->flags, F_HERO))
	{
		hero = new Hero;
		hero->unit = this;
		hero->Load(f);
		if(LOAD_VERSION < V_0_12)
		{
			if(hero->team_member && hero->type == HeroType::Visitor &&
				(data->id == "q_zlo_kaplan" || data->id == "q_magowie_stary" || strncmp(data->id.c_str(), "q_orkowie_gorush", 16) == 0))
			{
				hero->type = HeroType::Free;
			}
		}
	}
	else
		hero = nullptr;

	in_arena = -1;
	ai = nullptr;
	interp = nullptr;
	frozen = FROZEN::NO;

	// fizyka
	if(f.isLocal)
		CreatePhysics(true);
	else
		cobj = nullptr;

	// zabezpieczenie
	if((Any(weapon_state, WeaponState::Taken, WeaponState::Taking) && weapon_taken == W_NONE)
		|| (weapon_state == WeaponState::Hiding && weapon_hiding == W_NONE))
	{
		SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		Warn("Unit '%s' had broken weapon state.", data->id.c_str());
	}

	// calculate new skills/attributes
	if(LOAD_VERSION >= V_0_8)
	{
		if(content.require_update)
			CalculateStats();
	}
	else
	{
		if(IsPlayer())
		{
			player->RecalculateLevel();
			// set new skills initial value & aptitude
			UnitStats old_stats;
			old_stats.subprofile.value = 0;
			old_stats.subprofile.level = level;
			old_stats.Set(data->GetStatProfile());
			for(int i = 0; i < (int)SkillId::MAX; ++i)
			{
				if(stats->skill[i] == UnitStats::NEW_STAT)
				{
					stats->skill[i] = old_stats.skill[i];
					player->skill[i].apt = stats->skill[i] / 5;
				}
			}
			player->SetRequiredPoints();
			player->RecalculateLevel();
		}
		CalculateStats();
	}

	// compatibility
	if(LOAD_VERSION < V_0_12)
		mp = mpmax = CalculateMaxMp();

	CalculateLoad();

	fake_unit = false;
}

//=================================================================================================
void Unit::LoadStock(GameReader& f)
{
	if(!stock)
		stock = new TraderStock;

	vector<ItemSlot>& cnt = stock->items;

	f >> stock->date;
	uint count;
	f >> count;
	if(count == 0)
	{
		cnt.clear();
		return;
	}

	bool can_sort = true;
	cnt.resize(count);
	for(ItemSlot& slot : cnt)
	{
		const string& item_id = f.ReadString1();
		f >> slot.count;
		if(item_id[0] != '$')
			slot.item = Item::Get(item_id);
		else
		{
			int quest_id;
			f >> quest_id;
			quest_mgr->AddQuestItemRequest(&slot.item, item_id.c_str(), quest_id, &cnt);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && content.require_update)
		SortItems(cnt);
}

//=================================================================================================
void Unit::Write(BitStreamWriter& f) const
{
	// main
	f << id;
	f << data->id;

	// human data
	if(data->type == UNIT_TYPE::HUMAN)
	{
		f.WriteCasted<byte>(human_data->hair);
		f.WriteCasted<byte>(human_data->beard);
		f.WriteCasted<byte>(human_data->mustache);
		f << human_data->hair_color;
		f << human_data->height;
	}

	// items
	if(data->type != UNIT_TYPE::ANIMAL)
	{
		for(int i = 0; i < SLOT_MAX_VISIBLE; ++i)
		{
			if(slots[i])
				f << slots[i]->id;
			else
				f.Write0();
		}
	}
	f.WriteCasted<byte>(live_state);
	f << pos;
	f << rot;
	f << GetHpp();
	if(IsTeamMember())
	{
		f.Write1();
		f << GetMpp();
		f << GetStaminap();
	}
	else
		f.Write0();
	f.WriteCasted<char>(in_arena);
	f << summoner;
	f << mark;

	// hero/player data
	byte b;
	if(IsHero())
		b = 1;
	else if(IsPlayer())
		b = 2;
	else
		b = 0;
	f << b;
	if(IsHero())
	{
		f << hero->name;
		b = 0;
		if(hero->know_name)
			b |= 0x01;
		if(hero->team_member)
			b |= 0x02;
		if(hero->type != HeroType::Normal)
			b |= 0x04;
		f << b;
		f << hero->credit;
	}
	else if(IsPlayer())
	{
		f.WriteCasted<byte>(player->id);
		f << player->name;
		f << player->credit;
		f << player->free_days;
	}
	if(IsAI())
		f << GetAiMode();

	// loaded data
	if(net->mp_load)
	{
		mesh_inst->Write(f);
		f.WriteCasted<byte>(animation);
		f.WriteCasted<byte>(current_animation);
		f.WriteCasted<byte>(animation_state);
		f.WriteCasted<byte>(action);
		f.WriteCasted<byte>(weapon_taken);
		f.WriteCasted<byte>(weapon_hiding);
		f.WriteCasted<byte>(weapon_state);
		f << target_pos;
		f << target_pos2;
		f << timer;
		if(used_item)
			f << used_item->id;
		else
			f.Write0();
		f << (usable ? usable->id : -1);
		if(action == A_ATTACK)
			f << act.attack.index;
		else if(action == A_USE_USABLE)
			f << act.use_usable.rot;
	}
	else
	{
		// player changing dungeon level keeps weapon state
		f.WriteCasted<byte>(weapon_taken);
		f.WriteCasted<byte>(weapon_state);
	}
}

//=================================================================================================
bool Unit::Read(BitStreamReader& f)
{
	// main
	f >> id;
	const string& unit_data_id = f.ReadString1();
	if(!f)
		return false;
	Register();
	data = UnitData::TryGet(unit_data_id);
	if(!data)
	{
		Error("Missing base unit id '%s'!", unit_data_id.c_str());
		return false;
	}

	// human data
	if(data->type == UNIT_TYPE::HUMAN)
	{
		human_data = new Human;
		f.ReadCasted<byte>(human_data->hair);
		f.ReadCasted<byte>(human_data->beard);
		f.ReadCasted<byte>(human_data->mustache);
		f >> human_data->hair_color;
		f >> human_data->height;
		if(!f)
			return false;
		if(human_data->hair == 0xFF)
			human_data->hair = -1;
		if(human_data->beard == 0xFF)
			human_data->beard = -1;
		if(human_data->mustache == 0xFF)
			human_data->mustache = -1;
		if(human_data->hair < -1
			|| human_data->hair >= MAX_HAIR
			|| human_data->beard < -1
			|| human_data->beard >= MAX_BEARD
			|| human_data->mustache < -1
			|| human_data->mustache >= MAX_MUSTACHE
			|| !InRange(human_data->height, 0.85f, 1.15f))
		{
			Error("Invalid human data (hair:%d, beard:%d, mustache:%d, height:%g).", human_data->hair, human_data->beard,
				human_data->mustache, human_data->height);
			return false;
		}
	}
	else
		human_data = nullptr;

	// equipped items
	if(data->type != UNIT_TYPE::ANIMAL)
	{
		for(int i = 0; i < SLOT_MAX_VISIBLE; ++i)
		{
			const string& item_id = f.ReadString1();
			if(!f)
				return false;
			if(item_id.empty())
				slots[i] = nullptr;
			else
			{
				const Item* item = Item::TryGet(item_id);
				if(item && ItemTypeToSlot(item->type) == (ITEM_SLOT)i)
				{
					game_res->PreloadItem(item);
					slots[i] = item;
				}
				else
				{
					if(item)
						Error("Invalid slot type (%d != %d).", ItemTypeToSlot(item->type), i);
					return false;
				}
			}
		}
		for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
			slots[i] = nullptr;
	}
	else
	{
		for(int i = 0; i < SLOT_MAX; ++i)
			slots[i] = nullptr;
	}

	// variables
	hpmax = 1.f;
	mpmax = 1.f;
	stamina_max = 1.f;
	level = 0;
	f.ReadCasted<byte>(live_state);
	f >> pos;
	f >> rot;
	f >> hp;
	if(f.Read1())
	{
		f >> mp;
		f >> stamina;
	}
	else
	{
		mp = 1.f;
		stamina = 1.f;
	}
	f.ReadCasted<char>(in_arena);
	f >> summoner;
	f >> mark;
	if(!f)
		return false;
	if(live_state >= LIVESTATE_MAX)
	{
		Error("Invalid live state %d.", live_state);
		return false;
	}

	// hero/player data
	byte type;
	f >> type;
	if(!f)
		return false;
	if(type == 1)
	{
		// hero
		byte flags;
		ai = AI_PLACEHOLDER;
		player = nullptr;
		hero = new Hero;
		hero->unit = this;
		f >> hero->name;
		f >> flags;
		f >> hero->credit;
		if(!f)
			return false;
		hero->know_name = IsSet(flags, 0x01);
		hero->team_member = IsSet(flags, 0x02);
		hero->type = IsSet(flags, 0x04) ? HeroType::Visitor : HeroType::Normal;
	}
	else if(type == 2)
	{
		// player
		int id;
		f.ReadCasted<byte>(id);
		if(id == team->my_id)
		{
			assert(game->pc);
			player = game->pc;
		}
		else
			player = new PlayerController;
		ai = nullptr;
		hero = nullptr;
		player->id = id;
		player->unit = this;
		alcohol = 0.f;
		f >> player->name;
		f >> player->credit;
		f >> player->free_days;
		if(!f)
			return false;
		if(player->credit < 0)
		{
			Error("Invalid player %d credit %d.", player->id, player->credit);
			return false;
		}
		if(player->free_days < 0)
		{
			Error("Invalid player %d free days %d.", player->id, player->free_days);
			return false;
		}
		player->player_info = net->TryGetPlayer(player->id);
		if(!player->player_info)
		{
			Error("Invalid player id %d.", player->id);
			return false;
		}
		player->player_info->u = this;
	}
	else
	{
		// ai
		ai = AI_PLACEHOLDER;
		hero = nullptr;
		player = nullptr;
	}

	// ai variables
	if(IsAI())
	{
		f.ReadCasted<byte>(ai_mode);
		if(!f)
			return false;
	}

	// mesh
	CreateMesh(net->mp_load ? CREATE_MESH::PRELOAD : CREATE_MESH::NORMAL);

	action = A_NONE;
	weapon_taken = W_NONE;
	weapon_hiding = W_NONE;
	weapon_state = WeaponState::Hidden;
	talking = false;
	busy = Busy_No;
	frozen = FROZEN::NO;
	usable = nullptr;
	used_item = nullptr;
	bow_instance = nullptr;
	ai = nullptr;
	animation = ANI_STAND;
	current_animation = ANI_STAND;
	timer = 0.f;
	to_remove = false;
	bubble = nullptr;
	interp = EntityInterpolator::Pool.Get();
	interp->Reset(pos, rot);
	visual_pos = pos;

	if(net->mp_load)
	{
		// get current state in multiplayer
		if(!mesh_inst->Read(f))
			return false;
		f.ReadCasted<byte>(animation);
		f.ReadCasted<byte>(current_animation);
		f.ReadCasted<byte>(animation_state);
		f.ReadCasted<byte>(action);
		f.ReadCasted<byte>(weapon_taken);
		f.ReadCasted<byte>(weapon_hiding);
		f.ReadCasted<byte>(weapon_state);
		f >> target_pos;
		f >> target_pos2;
		f >> timer;
		const string& used_item_id = f.ReadString1();
		int usable_id = f.Read<int>();

		// used item
		if(!used_item_id.empty())
		{
			used_item = Item::TryGet(used_item_id);
			if(!used_item)
			{
				Error("Missing used item '%s'.", used_item_id.c_str());
				return false;
			}
		}
		else
			used_item = nullptr;

		// usable
		if(usable_id == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usable_id);

		// action
		switch(action)
		{
		case A_ATTACK:
			f >> act.attack.index;
			break;
		case A_SHOOT:
			bow_instance = game_level->GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = mesh_inst->groups[1].speed;
			bow_instance->groups[0].time = Min(mesh_inst->groups[1].time, bow_instance->groups[0].anim->length);
			break;
		case A_USE_USABLE:
			f >> act.use_usable.rot;
			break;
		}

		if(!f)
			return false;
	}
	else
	{
		// player changing dungeon level keeps weapon state
		f.ReadCasted<byte>(weapon_taken);
		f.ReadCasted<byte>(weapon_state);
		if(weapon_state == WeaponState::Taking)
			weapon_state = WeaponState::Hidden;
	}

	// physics
	CreatePhysics(true);

	// boss music
	if(IsSet(data->flags2, F2_BOSS))
		world->AddBossLevel();

	prev_pos = pos;
	speed = prev_speed = 0.f;
	talking = false;

	Register();
	return true;
}

//=================================================================================================
Effect* Unit::FindEffect(EffectId effect)
{
	for(Effect& e : effects)
	{
		if(e.effect == effect)
			return &e;
	}

	return nullptr;
}

//=================================================================================================
bool Unit::FindEffect(EffectId effect, float* value)
{
	Effect* top = nullptr;
	float topv = 0.f;

	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == effect && it->power > topv)
		{
			top = &*it;
			topv = it->power;
		}
	}

	if(top)
	{
		if(value)
			*value = topv;
		return true;
	}
	else
		return false;
}

//=================================================================================================
// szuka miksturek leczniczych w ekwipunku, zwraca -1 jeœli nie odnaleziono
int Unit::FindHealingPotion() const
{
	float healed_hp,
		missing_hp = hpmax - hp;
	int potion_index = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->item->type != IT_CONSUMABLE)
			continue;

		const Consumable& pot = it->item->ToConsumable();
		if(pot.aiType != Consumable::AiType::Healing)
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
int Unit::FindManaPotion() const
{
	float missing = mpmax - mp, gain = 0, gain2 = Inf();
	int id = -1, id2 = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->item->type != IT_CONSUMABLE)
			continue;

		const Consumable& pot = it->item->ToConsumable();
		if(pot.aiType != Consumable::AiType::Mana)
			continue;

		float power = pot.GetEffectPower(EffectId::RestoreMana);
		if(power <= missing)
		{
			if(power > gain)
			{
				gain = power;
				id = index;
			}
		}
		else
		{
			if(power < gain2)
			{
				gain2 = power;
				id2 = index;
			}
		}
	}

	if(id != -1)
	{
		if(id2 != -1)
		{
			if(missing - gain < gain2 - missing)
				return id;
			else
				return id2;
		}
		else
			return id;
	}
	else
		return id2;
}

//=================================================================================================
void Unit::ReequipItems()
{
	assert(Net::IsLocal());
	if(net->active_players > 1)
	{
		const Item* prev_slots[SLOT_MAX];
		for(int i = 0; i < SLOT_MAX; ++i)
			prev_slots[i] = slots[i];

		ReequipItemsInternal();

		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] != prev_slots[i] && IsVisible((ITEM_SLOT)i))
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_EQUIPMENT;
				c.unit = this;
				c.id = i;
			}
		}
	}
	else
		ReequipItemsInternal();
}

//=================================================================================================
void Unit::ReequipItemsInternal()
{
	bool changes = false;
	for(ItemSlot& item_slot : items)
	{
		if(!item_slot.item)
			continue;

		if(item_slot.item->type == IT_GOLD)
		{
			gold += item_slot.count;
			item_slot.item = nullptr;
			changes = true;
		}
		else if(CanWear(item_slot.item))
		{
			ITEM_SLOT slot = ItemTypeToSlot(item_slot.item->type);
			assert(slot != SLOT_INVALID);

			if(slots[slot])
			{
				if(slots[slot]->value < item_slot.item->value)
				{
					const Item* item = slots[slot];
					RemoveItemEffects(item, slot);
					ApplyItemEffects(item_slot.item, slot);
					slots[slot] = item_slot.item;
					item_slot.item = item;
				}
			}
			else
			{
				ApplyItemEffects(item_slot.item, slot);
				slots[slot] = item_slot.item;
				item_slot.item = nullptr;
				changes = true;
			}
		}
	}
	if(changes)
	{
		RemoveNullItems(items);
		SortItems(items);
	}

	// add item if unit have none
	if(data->type != UNIT_TYPE::ANIMAL && !HaveWeapon())
	{
		const Item* item = UnitHelper::GetBaseWeapon(*this);
		game_res->PreloadItem(item);
		AddItemAndEquipIfNone(item);
	}
}

//=================================================================================================
bool Unit::HaveQuestItem(int quest_id)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(quest_id))
			return true;
	}
	return false;
}

//=================================================================================================
void Unit::RemoveQuestItem(int quest_id)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(quest_id))
		{
			weight -= it->item->weight;
			items.erase(it);
			if(IsOtherPlayer())
			{
				NetChangePlayer& c = Add1(player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_QUEST_ITEM;
				c.id = quest_id;
			}
			return;
		}
	}

	assert(0 && "Nie znalaziono questowego przedmiotu do usuniêcia!");
}

//=================================================================================================
void Unit::RemoveQuestItemS(Quest* quest)
{
	RemoveQuestItem(quest->id);
}

//=================================================================================================
bool Unit::HaveItem(const Item* item, bool owned) const
{
	for(const Item* slot_item : slots)
	{
		if(slot_item == item)
			return true;
	}
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item == item && (!owned || it->team_count != it->count))
			return true;
	}
	return false;
}

//=================================================================================================
bool Unit::HaveItemEquipped(const Item* item) const
{
	ITEM_SLOT slot_type = ItemTypeToSlot(item->type);
	if(slot_type == SLOT_INVALID)
		return false;
	else if(slot_type == SLOT_RING1)
		return slots[SLOT_RING1] == item || slots[SLOT_RING2] == item;
	else
		return slots[slot_type] == item;
}

//=================================================================================================
bool Unit::SlotRequireHideWeapon(ITEM_SLOT slot) const
{
	WeaponType type;
	switch(slot)
	{
	case SLOT_WEAPON:
	case SLOT_SHIELD:
		type = W_ONE_HANDED;
		break;
	case SLOT_BOW:
		type = W_BOW;
		break;
	default:
		return false;
	}

	switch(weapon_state)
	{
	case WeaponState::Taken:
	case WeaponState::Taking:
		return weapon_taken == type;
	case WeaponState::Hiding:
		return weapon_hiding == type;
	default:
		return false;
	}
}

//=================================================================================================
float Unit::GetAttackSpeed(const Weapon* used_weapon) const
{
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return 1.f;

	const Weapon* wep;

	if(used_weapon)
		wep = used_weapon;
	else if(HaveWeapon())
		wep = &GetWeapon();
	else
		wep = nullptr;

	/* co wp³ywa na szybkoœæ ataku?
	+ rodzaj broni
	+ zrêcznoœæ
	+ umiejêtnoœci
	+ brakuj¹ca si³a
	+ udŸwig */
	float mod = 1.f;
	float base_speed;
	if(wep)
	{
		const WeaponTypeInfo& info = wep->GetInfo();

		float dex_mod = min(0.25f, info.dex_speed * Get(AttributeId::DEX));
		mod += float(Get(info.skill)) / 200 + dex_mod - GetAttackSpeedModFromStrength(*wep);
		base_speed = info.base_speed;
	}
	else
	{
		float dex_mod = min(0.25f, 0.02f * Get(AttributeId::DEX));
		mod += float(Get(SkillId::UNARMED)) / 200 + dex_mod;
		base_speed = 1.f;
	}

	float mobility = CalculateMobility();
	if(mobility < 100)
		mod -= Lerp(0.1f, 0.f, mobility / 100);
	else if(mobility > 100)
		mod += Lerp(0.f, 0.1f, (mobility - 100) / 100);

	if(mod < 0.5f)
		mod = 0.5f;

	float speed = mod * base_speed;
	return speed;
}

//=================================================================================================
float Unit::GetBowAttackSpeed() const
{
	float base_mod = GetStaminaAttackSpeedMod();
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return base_mod;

	// values range
	//	1 dex, 0 skill = 0.8
	// 50 dex, 10 skill = 1.1
	// 100 dex, 100 skill = 1.7
	float mod = 0.8f + float(Get(SkillId::BOW)) / 200 + 0.004f * Get(AttributeId::DEX) - GetAttackSpeedModFromStrength(GetBow());

	float mobility = CalculateMobility();
	if(mobility < 100)
		mod -= Lerp(0.1f, 0.f, mobility / 100);
	else if(mobility > 100)
		mod += Lerp(0.f, 0.1f, (mobility - 100) / 100);

	mod *= base_mod;
	if(mod < 0.5f)
		mod = 0.5f;

	return mod;
}

//=================================================================================================
void Unit::MakeItemsTeam(bool is_team)
{
	if(is_team)
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->team_count = it->count;
	}
	else
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->team_count = 0;
	}
}

//=================================================================================================
// Used when unit stand up
// Restore hp to 1 and heal poison
void Unit::HealPoison()
{
	if(Net::IsLocal())
	{
		if(hp < 1.f)
			hp = 1.f;
	}
	else
	{
		if(hp <= 0.f)
			hp = 0.001f;
	}

	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == EffectId::Poison)
			_to_remove.push_back(index);
	}

	RemoveEffects();
}

//=================================================================================================
void Unit::RemoveEffect(EffectId effect)
{
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == effect)
			_to_remove.push_back(index);
	}

	if(effect == EffectId::Stun && !_to_remove.empty() && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::STUN;
		c.unit = this;
		c.f[0] = 0;
	}

	RemoveEffects();
}

//=================================================================================================
int Unit::FindItem(const Item* item, int quest_id) const
{
	assert(item);

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] == item && (quest_id == -1 || slots[i]->IsQuest(quest_id)))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item && (quest_id == -1 || it->item->IsQuest(quest_id)))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
int Unit::FindItem(delegate<bool(const ItemSlot& slot)> callback) const
{
	int index = 0;
	for(const ItemSlot& slot : items)
	{
		if(callback(slot))
			return index;
		++index;
	}
	return INVALID_IINDEX;
}

//=================================================================================================
int Unit::FindQuestItem(int quest_id) const
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] && slots[i]->IsQuest(quest_id))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(quest_id))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
bool Unit::FindQuestItem(cstring id, Quest** out_quest, int* i_index, bool not_active, int quest_id)
{
	assert(id);

	if(id[1] == '$')
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] && slots[i]->IsQuest() && (quest_id == -1 || quest_id == slots[i]->quest_id))
			{
				Quest* quest = quest_mgr->FindQuest(slots[i]->quest_id, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za³o¿onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = items.begin(), end2 = items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && it2->item->IsQuest() && (quest_id == -1 || quest_id == it2->item->quest_id))
			{
				Quest* quest = quest_mgr->FindQuest(it2->item->quest_id, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}
	else
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] && slots[i]->IsQuest() && slots[i]->id == id)
			{
				Quest* quest = quest_mgr->FindQuest(slots[i]->quest_id, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za³o¿onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = items.begin(), end2 = items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && it2->item->IsQuest() && it2->item->id == id)
			{
				Quest* quest = quest_mgr->FindQuest(it2->item->quest_id, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}

	return false;
}

//=================================================================================================
// currently using this on pc, looted units is not written
void Unit::RemoveItem(int iindex, bool active_location)
{
	assert(!player);
	assert(Net::IsLocal());
	if(iindex >= 0)
	{
		assert(iindex < (int)items.size());
		RemoveElementIndex(items, iindex);
	}
	else
	{
		ITEM_SLOT s = IIndexToSlot(iindex);
		assert(slots[s]);
		slots[s] = nullptr;
		if(active_location && IsVisible(s))
		{
			NetChange& c = Add1(Net::changes);
			c.unit = this;
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.id = s;
		}
	}
}

//=================================================================================================
// remove item from inventory (handle open inventory, lock & multiplayer), for 0 count remove all items
uint Unit::RemoveItem(int i_index, uint count)
{
	// remove item
	uint removed;
	if(i_index >= 0)
	{
		ItemSlot& s = items[i_index];
		removed = (count == 0 ? s.count : min(s.count, count));
		s.count -= removed;
		weight -= s.item->weight * removed;
		if(s.count == 0)
			items.erase(items.begin() + i_index);
		else if(s.team_count > 0)
			s.team_count -= min(s.team_count, removed);
	}
	else
	{
		ITEM_SLOT type = IIndexToSlot(i_index);
		weight -= slots[type]->weight;
		slots[type] = nullptr;
		removed = 1;

		if(Net::IsServer() && net->active_players > 1 && IsVisible(type))
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = this;
			c.id = type;
		}
	}

	// notify
	if(Net::IsServer())
	{
		if(IsPlayer())
		{
			if(!player->is_local)
			{
				NetChangePlayer& c = Add1(player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS;
				c.id = i_index;
				c.count = count;
			}
		}
		else
		{
			// search for player trading with this unit
			Unit* t = nullptr;
			for(Unit& member : team->active_members)
			{
				if(member.IsPlayer() && member.player->IsTradingWith(this))
				{
					t = &member;
					break;
				}
			}

			if(t && t->player != game->pc)
			{
				NetChangePlayer& c = Add1(t->player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS_TRADER;
				c.id = id;
				c.count = count;
				c.a = i_index;
			}
		}
	}

	// update temporary inventory
	if(game->pc->unit == this)
	{
		if(game_gui->inventory->inv_mine->visible || game_gui->inventory->gp_trade->visible)
			game_gui->inventory->BuildTmpInventory(0);
	}
	else if(game_gui->inventory->gp_trade->visible && game_gui->inventory->inv_trade_other->unit == this)
		game_gui->inventory->BuildTmpInventory(1);

	return removed;
}

//=================================================================================================
uint Unit::RemoveItem(const Item* item, uint count)
{
	int i_index = FindItem(item);
	if(i_index == INVALID_IINDEX)
		return 0;
	return RemoveItem(i_index, count);
}

//=================================================================================================
uint Unit::RemoveItemS(const string& item_id, uint count)
{
	const Item* item = Item::TryGet(item_id);
	if(!item)
		return 0;
	return RemoveItem(item, count);
}

//=================================================================================================
void Unit::SetName(const string& name)
{
	if(hero)
		hero->name = name;
}

//=================================================================================================
void Unit::ClearInventory()
{
	items.clear();
	for(int i = 0; i < SLOT_MAX; ++i)
		slots[i] = nullptr;
	weight = 0;
	SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	if(player)
		player->last_weapon = W_NONE;
}

//=================================================================================================
cstring NAMES::point_weapon = "bron";
cstring NAMES::point_hidden_weapon = "schowana";
cstring NAMES::point_shield = "tarcza";
cstring NAMES::point_shield_hidden = "tarcza_plecy";
cstring NAMES::point_bow = "luk";
cstring NAMES::point_cast = "castpoint";
cstring NAMES::points[] = {
	"bron",
	"schowana",
	"tarcza",
	"tarcza_plecy",
	"luk"
};
uint NAMES::n_points = countof(NAMES::points);

//=================================================================================================
cstring NAMES::ani_take_weapon = "wyjmuje";
cstring NAMES::ani_take_weapon_no_shield = "wyjmuje_bez_tarczy";
cstring NAMES::ani_take_bow = "wyjmij_luk";
cstring NAMES::ani_move = "idzie";
cstring NAMES::ani_run = "biegnie";
cstring NAMES::ani_left = "w_lewo";
cstring NAMES::ani_right = "w_prawo";
cstring NAMES::ani_stand = "stoi";
cstring NAMES::ani_battle = "bojowy";
cstring NAMES::ani_battle_bow = "bojowy_luk";
cstring NAMES::ani_die = "umiera";
cstring NAMES::ani_hurt = "trafiony";
cstring NAMES::ani_shoot = "atak_luk";
cstring NAMES::ani_block = "blok";
cstring NAMES::ani_bash = "bash";
cstring NAMES::ani_base[] = {
	"idzie",
	"w_lewo",
	"w_prawo",
	"stoi",
	"bojowy",
	"umiera"
};
cstring NAMES::ani_humanoid[] = {
	"wyjmuje",
	"wyjmij_luk",
	"bojowy_luk",
	"atak_luk",
	"blok",
	"bash"
};
cstring NAMES::ani_attacks[] = {
	"atak1",
	"atak2",
	"atak3",
	"atak4",
	"atak5",
	"atak6"
};
uint NAMES::n_ani_base = countof(NAMES::ani_base);
uint NAMES::n_ani_humanoid = countof(NAMES::ani_humanoid);
int NAMES::max_attacks = countof(ani_attacks);

//=================================================================================================
Vec3 Unit::GetEyePos() const
{
	const Mesh::Point& point = *mesh_inst->mesh->GetPoint("oczy");
	Matrix matBone = point.mat * mesh_inst->mat_bones[point.bone];
	matBone = matBone * (Matrix::RotationY(rot) * Matrix::Translation(pos));
	Vec3 eye = Vec3::TransformZero(matBone);
	return eye;
}

//=================================================================================================
bool Unit::IsBetterItem(const Item* item, int* value, int* prev_value, ITEM_SLOT* target_slot) const
{
	assert(item);

	if(target_slot)
		*target_slot = ItemTypeToSlot(item->type);

	switch(item->type)
	{
	case IT_WEAPON:
		if(!IsSet(data->flags, F_MAGE))
			return IsBetterWeapon(item->ToWeapon(), value, prev_value);
		else
		{
			int v = item->ai_value;
			int prev_v = HaveWeapon() ? GetWeapon().ai_value : 0;
			if(value)
			{
				*value = v;
				*prev_value = prev_v;
			}
			return v > prev_v;
		}
	case IT_BOW:
		{
			int v = item->ToBow().dmg;
			int prev_v = HaveBow() ? GetBow().dmg : 0;
			if(value)
			{
				*value = v * 2;
				*prev_value = prev_v * 2;
			}
			return v > prev_v;
		}
	case IT_SHIELD:
		{
			int v = item->ToShield().block;
			int prev_v = HaveShield() ? GetShield().block : 0;
			if(value)
			{
				*value = v * 2;
				*prev_value = prev_v * 2;
			}
			return v > prev_v;
		}
	case IT_ARMOR:
		if(!IsSet(data->flags, F_MAGE))
			return IsBetterArmor(item->ToArmor(), value, prev_value);
		else
		{
			int v = item->ai_value;
			int prev_v = HaveArmor() ? GetArmor().ai_value : 0;
			if(value)
			{
				*value = v;
				*prev_value = prev_v;
			}
			return v > prev_v;
		}
	case IT_AMULET:
		{
			float v = GetItemAiValue(item);
			float prev_v = HaveAmulet() ? GetItemAiValue(&GetAmulet()) : 0;
			if(value)
			{
				*value = (int)v;
				*prev_value = (int)prev_v;
			}
			return v > prev_v && v > 0;
		}
	case IT_RING:
		{
			float v = GetItemAiValue(item);
			float prev_v;
			ITEM_SLOT best_slot;
			if(!slots[SLOT_RING1])
			{
				prev_v = 0;
				best_slot = SLOT_RING1;
			}
			else if(!slots[SLOT_RING2])
			{
				prev_v = 0;
				best_slot = SLOT_RING2;
			}
			else
			{
				float prev_v1 = GetItemAiValue(slots[SLOT_RING1]),
					prev_v2 = GetItemAiValue(slots[SLOT_RING2]);
				if(prev_v1 > prev_v2)
				{
					prev_v = prev_v2;
					best_slot = SLOT_RING2;
				}
				else
				{
					prev_v = prev_v1;
					best_slot = SLOT_RING1;
				}
			}
			if(value)
			{
				*value = (int)v;
				*prev_value = (int)prev_v;
			}
			if(target_slot)
				*target_slot = best_slot;
			return v > prev_v && v > 0;
		}
	default:
		assert(0);
		return false;
	}
}

//=================================================================================================
float Unit::GetItemAiValue(const Item* item) const
{
	assert(Any(item->type, IT_AMULET, IT_RING)); // TODO

	const float* priorities = stats->tag_priorities;
	const ItemTag* tags;
	if(item->type == IT_AMULET)
		tags = item->ToAmulet().tag;
	else
		tags = item->ToRing().tag;

	float value = (float)item->ai_value;
	for(int i = 0; i < MAX_ITEM_TAGS; ++i)
	{
		if(tags[i] == TAG_NONE)
			break;
		value *= priorities[tags[i]];
	}

	return value;
}

//=================================================================================================
int Unit::CountItem(const Item* item)
{
	assert(item);

	if(item->IsStackable())
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
			{
				// zak³ada ¿e mo¿na mieæ tylko jeden stack takich przedmiotów
				return it->count;
			}
		}
		return 0;
	}
	else
	{
		int count = 0;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
				++count;
		}
		return count;
	}
}

//=================================================================================================
const Item* Unit::GetIIndexItem(int i_index) const
{
	if(i_index >= 0)
	{
		if(i_index < (int)items.size())
			return items[i_index].item;
		else
			return nullptr;
	}
	else
	{
		ITEM_SLOT slot_type = IIndexToSlot(i_index);
		if(slot_type < SLOT_MAX)
			return slots[slot_type];
		else
			return nullptr;
	}
}

//=================================================================================================
Mesh::Animation* Unit::GetTakeWeaponAnimation(bool melee) const
{
	if(melee)
	{
		if(HaveShield())
			return mesh_inst->mesh->GetAnimation(NAMES::ani_take_weapon);
		else
		{
			Mesh::Animation* anim = mesh_inst->mesh->GetAnimation(NAMES::ani_take_weapon_no_shield);
			if(!anim)
			{
				// brak animacja wyci¹gania broni bez tarczy, u¿yj zwyk³ej
				return mesh_inst->mesh->GetAnimation(NAMES::ani_take_weapon);
			}
			else
				return anim;
		}
	}
	else
		return mesh_inst->mesh->GetAnimation(NAMES::ani_take_bow);
}

//=================================================================================================
// 0-immune, 0.5-resists 50%, 1-normal, 1.5-50% extra damage etc
float Unit::CalculateMagicResistance() const
{
	float mres = 1.f;
	if(IsSet(data->flags2, F2_MAGIC_RES50))
		mres = 0.5f;
	else if(IsSet(data->flags2, F2_MAGIC_RES25))
		mres = 0.75f;
	float effect_mres = GetEffectMulInv(EffectId::MagicResistance);
	return mres * effect_mres;
}

//=================================================================================================
float Unit::GetPoisonResistance() const
{
	if(IsSet(data->flags, F_POISON_RES))
		return 0.f;
	return GetEffectMulInv(EffectId::PoisonResistance);
}

//=================================================================================================
float Unit::GetBackstabMod(const Item* item) const
{
	float mod = 0.25f;
	if(IsSet(data->flags, F2_BACKSTAB))
		mod += 0.25f;
	if(item)
	{
		for(const ItemEffect& e : item->effects)
		{
			if(e.on_attack && e.effect == EffectId::Backstab)
				mod += e.power;
		}
	}
	mod += GetEffectSum(EffectId::Backstab);
	return mod;
}

//=================================================================================================
bool Unit::HaveEffect(EffectId e) const
{
	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == e)
			return true;
	}
	return false;
}

//=================================================================================================
void Unit::RemoveEffects(bool send)
{
	if(_to_remove.empty())
		return;

	send = (send && player && !player->IsLocal() && Net::IsServer());

	while(!_to_remove.empty())
	{
		uint index = _to_remove.back();
		Effect e = effects[index];
		if(send)
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::REMOVE_EFFECT;
			c.id = (int)e.effect;
			c.count = (int)e.source;
			c.a1 = e.source_id;
			c.a2 = e.value;
		}

		_to_remove.pop_back();
		if(index == effects.size() - 1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin() + index, effects.end() - 1);
			effects.pop_back();
		}

		OnAddRemoveEffect(e);
	}
}

//=================================================================================================
int Unit::GetBuffs() const
{
	int b = 0;

	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->source != EffectSource::Temporary)
			continue;

		switch(it->effect)
		{
		case EffectId::Poison:
			b |= BUFF_POISON;
			break;
		case EffectId::Alcohol:
			b |= BUFF_ALCOHOL;
			break;
		case EffectId::Regeneration:
			b |= BUFF_REGENERATION;
			break;
		case EffectId::FoodRegeneration:
			b |= BUFF_FOOD;
			break;
		case EffectId::NaturalHealingMod:
			b |= BUFF_NATURAL;
			break;
		case EffectId::MagicResistance:
			b |= BUFF_ANTIMAGIC;
			break;
		case EffectId::StaminaRegeneration:
			b |= BUFF_STAMINA;
			break;
		case EffectId::Stun:
			b |= BUFF_STUN;
			break;
		case EffectId::PoisonResistance:
			b |= BUFF_POISON_RES;
			break;
		}
	}

	if(alcohol / hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

//=================================================================================================
bool Unit::CanTalk(Unit& unit) const
{
	if(Any(action, A_EAT, A_DRINK, A_STAND_UP))
		return false;
	if(GetOrder() == ORDER_AUTO_TALK && order->auto_talk == AutoTalkMode::Leader && !team->IsLeader(unit))
		return false;
	return true;
}

//=================================================================================================
bool Unit::CanAct() const
{
	if(talking || !IsStanding() || action == A_STAND_UP)
		return false;
	if(ai)
	{
		if(ai->state != AIController::Idle)
			return false;
	}
	return true;
}

//=================================================================================================
int Unit::Get(AttributeId a, StatState* state) const
{
	int value = stats->attrib[(int)a];
	StatInfo stat_info;

	for(const Effect& e : effects)
	{
		if(e.effect == EffectId::Attribute && e.value == (int)a)
		{
			value += (int)e.power;
			stat_info.Mod((int)e.power);
		}
	}

	if(state)
		*state = stat_info.GetState();
	return value;
}

//=================================================================================================
int Unit::Get(SkillId s, StatState* state, bool skill_bonus) const
{
	int index = (int)s;
	int base = stats->skill[index];
	int value = base;
	StatInfo stat_info;

	for(const Effect& e : effects)
	{
		if(e.effect == EffectId::Skill && e.value == (int)s)
		{
			value += (int)e.power;
			stat_info.Mod((int)e.power);
		}
	}

	// apply skill synergy
	if(skill_bonus && base > 0)
	{
		switch(s)
		{
		case SkillId::LIGHT_ARMOR:
		case SkillId::HEAVY_ARMOR:
			{
				int other_val = GetBase(SkillId::MEDIUM_ARMOR);
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		case SkillId::MEDIUM_ARMOR:
			{
				int other_val = max(GetBase(SkillId::LIGHT_ARMOR), GetBase(SkillId::HEAVY_ARMOR));
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		case SkillId::SHORT_BLADE:
			{
				int other_val = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::AXE));
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		case SkillId::LONG_BLADE:
			{
				int other_val = max(max(GetBase(SkillId::SHORT_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::AXE));
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		case SkillId::BLUNT:
			{
				int other_val = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::SHORT_BLADE)), GetBase(SkillId::AXE));
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		case SkillId::AXE:
			{
				int other_val = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::SHORT_BLADE));
				if(other_val > value)
					value += (other_val - value) / 2;
			}
			break;
		}
	}

	if(state)
		*state = stat_info.GetState();
	return value;
}

//=================================================================================================
void Unit::Set(AttributeId a, int value)
{
	assert(!stats->fixed);
	if(stats->attrib[(int)a] != value)
	{
		stats->attrib[(int)a] = value;
		ApplyStat(a);
		if(player)
			player->recalculate_level = true;
	}
}

//=================================================================================================
void Unit::Set(SkillId s, int value)
{
	assert(!stats->fixed);
	if(stats->skill[(int)s] != value)
	{
		stats->skill[(int)s] = value;
		ApplyStat(s);
		if(player)
			player->recalculate_level = true;
	}
}

//=================================================================================================
void Unit::ApplyStat(AttributeId a)
{
	// recalculate other stats
	switch(a)
	{
	case AttributeId::STR:
		RecalculateHp();
		CalculateLoad();
		break;
	case AttributeId::END:
		RecalculateHp();
		RecalculateStamina();
		break;
	case AttributeId::DEX:
		RecalculateStamina();
		break;
	case AttributeId::WIS:
		RecalculateMp();
		break;
	case AttributeId::INT:
	case AttributeId::CHA:
		break;
	}
}

//=================================================================================================
void Unit::ApplyStat(SkillId s)
{
	if(s == SkillId::CONCENTRATION)
		RecalculateMp();
	else if(s == SkillId::ALCHEMY)
	{
		if(IsPlayer())
		{
			int skill = GetBase(SkillId::ALCHEMY);
			for(pair<const int, Recipe*>& p : Recipe::items)
			{
				Recipe* recipe = p.second;
				if(recipe->autolearn && skill >= recipe->skill)
					player->AddRecipe(recipe);
			}
		}
	}
}

//=================================================================================================
void Unit::CalculateStats()
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		ApplyStat((AttributeId)i);
}

//=================================================================================================
float Unit::CalculateMobility(const Armor* armor) const
{
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return 100;

	if(!armor)
		armor = (const Armor*)slots[SLOT_ARMOR];

	// calculate base mobility (75-125)
	float mobility = 75.f + 0.5f * Get(AttributeId::DEX);

	// add bonus
	float mobility_bonus = GetEffectSum(EffectId::Mobility);
	mobility += mobility_bonus;

	if(armor)
	{
		// calculate armor mobility (0-100)
		int armor_mobility = armor->mobility;
		int skill = min(Get(armor->GetSkill()), 100);
		armor_mobility += skill / 4;
		if(armor_mobility > 100)
			armor_mobility = 100;
		int str = Get(AttributeId::STR);
		if(str < armor->reqStr)
		{
			armor_mobility -= armor->reqStr - str;
			if(armor_mobility < 0)
				armor_mobility = 0;
		}

		// multiply mobility by armor mobility
		mobility = (float(armor_mobility) / 100 * mobility);
	}

	LoadState load_state = GetLoadState();
	switch(load_state)
	{
	case LS_NONE:
	case LS_LIGHT:
		break;
	case LS_MEDIUM:
		mobility *= 0.95f;
		break;
	case LS_HEAVY:
		mobility *= 0.85f;
		break;
	case LS_OVERLOADED:
		mobility *= 0.5f;
		break;
	case LS_MAX_OVERLOADED:
		mobility = 0;
		break;
	}

	mobility = Clamp(mobility, 1.f, 200.f);
	return mobility;
}

//=================================================================================================
float Unit::GetMobilityMod(bool run) const
{
	float mob = CalculateMobility();
	float m = 0.5f + mob / 200.f;
	if(!run && m < 1.f)
		m = 1.f - (1.f - m) / 2;
	return m;
}

//=================================================================================================
WEAPON_TYPE Unit::GetBestWeaponType() const
{
	WEAPON_TYPE best = (WEAPON_TYPE)0;
	int best_value = GetBase(WeaponTypeInfo::info[0].skill);
	for(int i = 1; i < WT_MAX; ++i)
	{
		int value = GetBase(WeaponTypeInfo::info[i].skill);
		if(value > best_value)
		{
			best = (WEAPON_TYPE)i;
			best_value = value;
		}
	}
	return best;
}

//=================================================================================================
ARMOR_TYPE Unit::GetBestArmorType() const
{
	ARMOR_TYPE best = (ARMOR_TYPE)0;
	int best_value = GetBase(GetArmorTypeSkill(best));
	for(int i = 1; i < AT_MAX; ++i)
	{
		int value = GetBase(GetArmorTypeSkill((ARMOR_TYPE)i));
		if(value > best_value)
		{
			best = (ARMOR_TYPE)i;
			best_value = value;
		}
	}
	return best;
}

//=================================================================================================
int Unit::ItemsToSellWeight() const
{
	int w = 0;
	for(const ItemSlot& slot : items)
	{
		if(slot.item && slot.item->IsWearable() && !slot.item->IsQuest())
			w += slot.item->weight;
	}
	return w;
}

//=================================================================================================
void Unit::SetAnimationAtEnd(cstring anim_name)
{
	if(anim_name)
		mesh_inst->SetToEnd(anim_name);
	else
		mesh_inst->SetToEnd();
}

//=================================================================================================
void Unit::SetDontAttack(bool new_dont_attack)
{
	if(new_dont_attack == dont_attack)
		return;
	dont_attack = new_dont_attack;
	if(ai)
		ai->change_ai_mode = true;
}

//=================================================================================================
void Unit::UpdateStaminaAction()
{
	if(usable)
	{
		if(IsSet(usable->base->use_flags, BaseUsable::SLOW_STAMINA_RESTORE))
			stamina_action = SA_RESTORE_SLOW;
		else
			stamina_action = SA_RESTORE_MORE;
	}
	else
	{
		switch(action)
		{
		case A_TAKE_WEAPON:
		case A_PICKUP:
		case A_POSITION:
		case A_PAIN:
		case A_CAST:
			stamina_action = SA_RESTORE;
			break;
		case A_SHOOT:
			if(animation_state < AS_SHOOT_SHOT)
				stamina_action = SA_RESTORE_SLOW;
			else
				stamina_action = SA_RESTORE;
			break;
		case A_EAT:
		case A_DRINK:
		case A_ANIMATION:
		case A_USE_USABLE:
		default:
			stamina_action = SA_RESTORE_MORE;
			break;
		case A_BLOCK:
		case A_DASH:
			stamina_action = SA_RESTORE_SLOW;
			break;
		case A_BASH:
		case A_ATTACK:
			stamina_action = SA_DONT_RESTORE;
			break;
		}

		switch(animation)
		{
		case ANI_WALK:
		case ANI_WALK_BACK:
		case ANI_LEFT:
		case ANI_RIGHT:
		case ANI_KNEELS:
		case ANI_RUN:
			if(stamina_action == SA_RESTORE_MORE)
				stamina_action = SA_RESTORE;
			break;
		}
	}
}

//=================================================================================================
void Unit::RemoveMana(float value)
{
	mp -= value;
	if(player && Net::IsLocal())
		player->Train(TrainWhat::Mana, value, 0);
	if(Net::IsServer() && IsTeamMember())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UPDATE_MP;
		c.unit = this;
	}
}

//=================================================================================================
void Unit::RemoveStamina(float value)
{
	stamina -= value;
	if(stamina < -stamina_max / 2)
		stamina = -stamina_max / 2;
	stamina_timer = STAMINA_RESTORE_TIMER;
	if(player && Net::IsLocal())
		player->Train(TrainWhat::Stamina, value, 0);
	if(Net::IsServer() && IsTeamMember())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UPDATE_STAMINA;
		c.unit = this;
	}
}

//=================================================================================================
void Unit::RemoveStaminaBlock(float value)
{
	float skill = (float)Get(SkillId::SHIELD);
	value *= Lerp(0.5f, 0.25f, Max(skill, 100.f) / 100.f);
	if(value > 0.f)
		RemoveStamina(value);
}

//=================================================================================================
// NORMAL - load if instant mode, add tasks when loadscreen mode
// ON_WORLDMAP - like normal but don't play animations
// PRELOAD - create MeshInstance with preload option when mesh not loaded
// AFTER_PRELOAD - call ApplyPreload on MeshInstance
// LOAD - always load mesh, add tasks for other
void Unit::CreateMesh(CREATE_MESH mode)
{
	Mesh* mesh = data->mesh;
	if(data->state != ResourceState::Loaded)
	{
		assert(mode != CREATE_MESH::AFTER_PRELOAD);
		if(res_mgr->IsLoadScreen())
		{
			if(mode == CREATE_MESH::LOAD)
				res_mgr->LoadInstant(mesh);
			else if(!mesh->IsLoaded())
				game->units_mesh_load.push_back(pair<Unit*, bool>(this, mode == CREATE_MESH::ON_WORLDMAP));
			if(data->state == ResourceState::NotLoaded)
			{
				res_mgr->Load(mesh);
				if(data->sounds)
				{
					for(int i = 0; i < SOUND_MAX; ++i)
					{
						for(SoundPtr sound : data->sounds->sounds[i])
							res_mgr->Load(sound);
					}
				}
				if(data->tex)
				{
					for(TexOverride& tex_o : data->tex->textures)
					{
						if(tex_o.diffuse)
							res_mgr->Load(tex_o.diffuse);
					}
				}
				data->state = ResourceState::Loading;
				res_mgr->AddTask(this, TaskCallback([](TaskData& td)
				{
					Unit* unit = static_cast<Unit*>(td.ptr);
					unit->data->state = ResourceState::Loaded;
				}));
			}
		}
		else
		{
			res_mgr->Load(mesh);
			if(data->sounds)
			{
				for(int i = 0; i < SOUND_MAX; ++i)
				{
					for(SoundPtr sound : data->sounds->sounds[i])
						res_mgr->Load(sound);
				}
			}
			if(data->tex)
			{
				for(TexOverride& tex_o : data->tex->textures)
				{
					if(tex_o.diffuse)
						res_mgr->Load(tex_o.diffuse);
				}
			}
			data->state = ResourceState::Loaded;
		}
	}

	if(mesh->IsLoaded())
	{
		if(mode != CREATE_MESH::AFTER_PRELOAD)
		{
			assert(!mesh_inst);
			mesh_inst = new MeshInstance(mesh);
			mesh_inst->ptr = this;

			if(mode != CREATE_MESH::ON_WORLDMAP)
			{
				if(IsAlive())
				{
					mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1 | PLAY_NO_BLEND, 0);
					animation = current_animation = ANI_STAND;
				}
				else
				{
					SetAnimationAtEnd(NAMES::ani_die);
					animation = current_animation = ANI_DIE;
				}
			}

			if(mesh_inst->mesh->head.n_groups > 1)
				mesh_inst->groups[1].state = 0;
			if(human_data)
				human_data->ApplyScale(mesh_inst);
		}
		else
		{
			assert(mesh_inst);
			mesh_inst->ApplyPreload(mesh);
		}
	}
	else if(mode == CREATE_MESH::PRELOAD)
	{
		assert(!mesh_inst);
		mesh_inst = new MeshInstance(nullptr);
	}
}

//=================================================================================================
void Unit::ApplyStun(float length)
{
	if(Net::IsLocal() && IsSet(data->flags2, F2_STUN_RESISTANCE))
		length /= 2;

	Effect* effect = FindEffect(EffectId::Stun);
	if(effect)
		effect->time = max(effect->time, length);
	else
	{
		Effect e;
		e.effect = EffectId::Stun;
		e.source = EffectSource::Temporary;
		e.source_id = -1;
		e.value = -1;
		e.power = 0.f;
		e.time = length;
		AddEffect(e);

		BreakAction();
	}

	if(Net::IsServer())
	{
		auto& c = Add1(Net::changes);
		c.type = NetChange::STUN;
		c.unit = this;
		c.f[0] = length;
	}
}

//=================================================================================================
void Unit::UseUsable(Usable* new_usable)
{
	if(new_usable)
	{
		assert(!usable && !new_usable->user);
		usable = new_usable;
		usable->user = this;
	}
	else
	{
		assert(usable && usable->user == this);
		usable->user = nullptr;
		usable = nullptr;
	}
}

//=================================================================================================
void Unit::RevealName(bool set_name)
{
	if(!hero || hero->know_name)
		return;
	hero->know_name = true;
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TELL_NAME;
		c.unit = this;
		c.id = set_name ? 1 : 0;
	}
}

//=================================================================================================
bool Unit::GetKnownName() const
{
	if(IsHero())
		return hero->know_name;
	return true;
}

//=================================================================================================
void Unit::SetKnownName(bool known)
{
	if(!hero || hero->know_name)
		return;
	hero->know_name = true;
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TELL_NAME;
		c.unit = this;
		c.id = 0;
	}
}

//=================================================================================================
bool Unit::IsIdle() const
{
	if(Net::IsLocal())
		return ai->state == AIController::Idle;
	else
		return !IsSet(ai_mode, AI_MODE_IDLE);
}

//=================================================================================================
bool Unit::IsAssist() const
{
	if(Net::IsLocal())
		return assist;
	else
		return IsSet(ai_mode, AI_MODE_ASSIST);
}

//=================================================================================================
bool Unit::IsDontAttack() const
{
	if(Net::IsLocal())
		return dont_attack;
	else
		return IsSet(ai_mode, AI_MODE_DONT_ATTACK);
}

//=================================================================================================
bool Unit::WantAttackTeam() const
{
	if(Net::IsLocal())
		return attack_team;
	else
		return IsSet(ai_mode, AI_MODE_ATTACK_TEAM);
}

//=================================================================================================
byte Unit::GetAiMode() const
{
	byte mode = 0;
	if(dont_attack)
		mode |= AI_MODE_DONT_ATTACK;
	if(assist)
		mode |= AI_MODE_ASSIST;
	if(ai->state != AIController::Idle)
		mode |= AI_MODE_IDLE;
	if(attack_team)
		mode |= AI_MODE_ATTACK_TEAM;
	return mode;
}

//=================================================================================================
void Unit::BreakAction(BREAK_ACTION_MODE mode, bool notify, bool allow_animation)
{
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.unit = this;
		c.type = NetChange::BREAK_ACTION;
	}

	switch(action)
	{
	case A_POSITION:
		return;
	case A_SHOOT:
		if(bow_instance)
			game_level->FreeBowInstance(bow_instance);
		action = A_NONE;
		break;
	case A_DRINK:
		if(animation_state == AS_DRINK_START)
		{
			if(Net::IsLocal())
				AddItem2(used_item, 1, used_item_is_team, false);
			if(mode != BREAK_ACTION_MODE::FALL)
				used_item = nullptr;
		}
		else
			used_item = nullptr;
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_EAT:
		if(animation_state < AS_EAT_EFFECT)
		{
			if(Net::IsLocal())
				AddItem2(used_item, 1, used_item_is_team, false);
			if(mode != BREAK_ACTION_MODE::FALL)
				used_item = nullptr;
		}
		else
			used_item = nullptr;
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		SetTakeHideWeaponAnimationToEnd(false, true);
		action = A_NONE;
		break;
	case A_BLOCK:
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_DASH:
		if(act.dash.ability->effect == Ability::Stun && mode != BREAK_ACTION_MODE::ON_LEAVE)
			mesh_inst->Deactivate(1);
		break;
	}

	if(mode == BREAK_ACTION_MODE::ON_LEAVE)
	{
		if(usable)
		{
			StopUsingUsable();
			UseUsable(nullptr);
			visual_pos = pos = target_pos;
		}
	}
	else if(usable && !(player && player->action == PlayerAction::LootContainer))
	{
		if(mode == BREAK_ACTION_MODE::INSTANT)
		{
			action = A_NONE;
			SetAnimationAtEnd(NAMES::ani_stand);
			UseUsable(nullptr);
			used_item = nullptr;
			animation = ANI_STAND;
		}
		else
		{
			target_pos2 = target_pos = pos;
			const Item* prev_used_item = used_item;
			StopUsingUsable(mode != BREAK_ACTION_MODE::FALL && notify);
			if(prev_used_item && slots[SLOT_WEAPON] == prev_used_item && !HaveShield())
				SetWeaponStateInstant(WeaponState::Taken, W_ONE_HANDED);
			else if(mode == BREAK_ACTION_MODE::FALL)
				used_item = prev_used_item;
			action = A_POSITION;
			animation_state = AS_POSITION_NORMAL;
		}

		if(Net::IsLocal() && IsAI() && ai->state == AIController::Idle && ai->st.idle.action != AIController::Idle_None)
		{
			ai->st.idle.action = AIController::Idle_None;
			ai->timer = Random(1.f, 2.f);
		}
	}
	else
	{
		if(!Any(action, A_ANIMATION, A_STAND_UP) || !allow_animation)
			action = A_NONE;
	}

	mesh_inst->ClearEndResult();

	if(mode != BREAK_ACTION_MODE::INSTANT)
	{
		if(mesh_inst->groups.size() == 2u)
			mesh_inst->Deactivate(1);
		animation = ANI_STAND;
	}

	if(mode == BREAK_ACTION_MODE::ON_LEAVE)
		return;

	if(IsPlayer())
	{
		player->next_action = NA_NONE;
		if(player->is_local)
		{
			player->data.ability_ready = nullptr;
			game_gui->inventory->EndLock();
			if(game_gui->inventory->mode > I_INVENTORY)
				game->CloseInventory();

			if(game_gui->craft->visible)
				game_gui->craft->Hide();

			if(player->action == PlayerAction::Talk)
			{
				if(Net::IsLocal())
				{
					player->action_unit->busy = Busy_No;
					player->action_unit->look_target = nullptr;
					player->dialog_ctx->dialog_mode = false;
				}
				else
					game->dialog_context.dialog_mode = false;
				look_target = nullptr;
				player->action = PlayerAction::None;
			}
		}
		else if(Net::IsLocal())
		{
			if(player->action == PlayerAction::Talk)
			{
				player->action_unit->busy = Busy_No;
				player->action_unit->look_target = nullptr;
				player->dialog_ctx->dialog_mode = false;
				look_target = nullptr;
				player->action = PlayerAction::None;
			}
		}
	}
	else if(Net::IsLocal())
	{
		ai->potion = -1;
		if(busy == Busy_Talking)
		{
			DialogContext* ctx = game->FindDialogContext(this);
			if(ctx)
				ctx->EndDialog();
			busy = Busy_No;
		}
	}
}

//=================================================================================================
void Unit::Fall()
{
	ACTION prev_action = action;
	live_state = FALLING;

	if(Net::IsLocal())
	{
		// przerwij akcjê
		BreakAction(BREAK_ACTION_MODE::FALL);

		// wstawanie
		raise_timer = Random(5.f, 7.f);

		// event
		if(event_handler)
			event_handler->HandleUnitEvent(UnitEventHandler::FALL, this);

		// komunikat
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::FALL;
			c.unit = this;
		}

		UpdatePhysics();

		if(player && player->is_local)
			player->data.before_player = BP_NONE;
	}
	else
	{
		// przerwij akcjê
		BreakAction(BREAK_ACTION_MODE::FALL);

		if(player && player->is_local)
		{
			raise_timer = Random(5.f, 7.f);
			player->data.before_player = BP_NONE;
		}
	}

	if(prev_action == A_ANIMATION)
	{
		action = A_NONE;
		current_animation = ANI_STAND;
	}
	animation = ANI_DIE;
	talking = false;
	mesh_inst->need_update = true;
}

//=================================================================================================
void Unit::TryStandup(float dt)
{
	if(in_arena != -1 || game->death_screen != 0)
		return;

	raise_timer -= dt;
	bool ok = false;

	if(live_state == DEAD)
	{
		if(IsTeamMember())
		{
			if(hp > 0.f && raise_timer > 0.1f)
				raise_timer = 0.1f;

			if(raise_timer <= 0.f)
			{
				RemovePoison();

				if(alcohol > hpmax)
				{
					// móg³by wstaæ ale jest zbyt pijany
					live_state = FALL;
					UpdatePhysics();
				}
				else
				{
					// sprawdŸ czy nie ma wrogów
					ok = true;
					for(Unit* unit : area->units)
					{
						if(unit->IsStanding() && IsEnemy(*unit) && Vec3::Distance(pos, unit->pos) <= 20.f && game_level->CanSee(*this, *unit))
						{
							ok = false;
							break;
						}
					}
				}

				if(!ok)
				{
					if(hp > 0.f)
						raise_timer = 0.1f;
					else
						raise_timer = Random(1.f, 2.f);
				}
			}
		}
	}
	else if(live_state == FALL)
	{
		if(raise_timer <= 0.f)
		{
			if(alcohol < hpmax)
				ok = true;
			else
				raise_timer = Random(1.f, 2.f);
		}
	}

	if(ok)
		Standup();
}

//=================================================================================================
void Unit::Standup(bool warp, bool leave)
{
	HealPoison();
	live_state = ALIVE;
	if(leave)
	{
		action = A_NONE;
		animation = ANI_STAND;
	}
	else
	{
		Mesh::Animation* anim = mesh_inst->mesh->GetAnimation("wstaje2");
		if(anim)
		{
			mesh_inst->Play(anim, PLAY_ONCE | PLAY_PRIO3, 0);
			action = A_STAND_UP;
			animation = ANI_PLAY;
		}
		else
		{
			action = A_NONE;
			animation = ANI_STAND;
		}
	}
	used_item = nullptr;
	if(weapon_state != WeaponState::Hidden)
	{
		WeaponType target_weapon;
		if(weapon_state == WeaponState::Taken || weapon_state == WeaponState::Taking)
			target_weapon = weapon_taken;
		else
			target_weapon = weapon_hiding;
		if((target_weapon == W_ONE_HANDED && !HaveWeapon())
			|| (target_weapon == W_BOW && !HaveBow()))
		{
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		}
	}

	if(Net::IsLocal())
	{
		if(IsAI())
		{
			ReequipItems();
			if(!leave)
				ai->Reset();
		}
		if(warp)
			game_level->WarpUnit(*this, pos);
	}

	if(!game_level->entering && !leave && (Net::IsServer() || (Net::IsClient() && IsLocalPlayer())))
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::STAND_UP;
		c.unit = this;
	}
}

//=================================================================================================
void Unit::Die(Unit* killer)
{
	ACTION prev_action = action;

	if(live_state == FALL)
	{
		// unit already on ground, add blood
		game_level->CreateBlood(*area, *this);
		live_state = DEAD;
	}
	else
		live_state = DYING;
	BreakAction(BREAK_ACTION_MODE::FALL);

	if(Net::IsLocal())
	{
		// add gold to inventory
		if(gold && !(IsPlayer() || IsFollower()))
		{
			AddItem(Item::gold, (uint)gold, (uint)gold);
			gold = 0;
		}

		// mark if unit have important item in inventory
		for(ItemSlot& item : items)
		{
			if(item.item && IsSet(item.item->flags, ITEM_IMPORTANT))
			{
				mark = true;
				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::MARK_UNIT;
					c.unit = this;
					c.id = 1;
				}
				break;
			}
		}

		// notify about death
		for(vector<Unit*>::iterator it = area->units.begin(), end = area->units.end(); it != end; ++it)
		{
			if((*it)->IsPlayer() || !(*it)->IsStanding() || !IsFriend(**it))
				continue;

			if(Vec3::Distance(pos, (*it)->pos) <= 20.f && game_level->CanSee(*this, **it))
				(*it)->ai->morale -= 2.f;
		}

		// rising team members / check is location cleared
		if(IsTeamMember())
			raise_timer = Random(5.f, 7.f);
		else
			game_level->CheckIfLocationCleared();

		// event
		if(event_handler)
			event_handler->HandleUnitEvent(UnitEventHandler::DIE, this);
		for(Event& event : events)
		{
			if(event.type == EVENT_DIE)
			{
				ScriptEvent e(EVENT_DIE);
				e.on_die.unit = this;
				event.quest->FireEvent(e);
			}
		}

		// message
		if(Net::IsOnline())
		{
			NetChange& c2 = Add1(Net::changes);
			c2.type = NetChange::DIE;
			c2.unit = this;
		}

		// stats
		++game_stats->total_kills;
		if(killer && killer->IsPlayer())
		{
			++killer->player->kills;
			killer->player->stat_flags |= STAT_KILLS;
		}
		if(IsPlayer())
		{
			++player->knocks;
			player->stat_flags |= STAT_KNOCKS;
			if(player->is_local)
				game->pc->data.before_player = BP_NONE;
		}

		// give experience
		if(killer)
		{
			if(killer->in_arena == -1)
			{
				if(killer->IsTeamMember() || killer->assist)
				{
					int exp = level * 50;
					if(killer->assist)
						exp /= 2;
					team->AddExp(exp);
				}
			}
			else
			{
				Arena* arena = game->arena;
				if(arena)
					arena->RewardExp(this);
			}
		}
	}
	else
	{
		hp = 0.f;

		// player rising
		if(player && player->is_local)
		{
			raise_timer = Random(5.f, 7.f);
			player->data.before_player = BP_NONE;
		}
	}

	// end boss music
	if(IsSet(data->flags2, F2_BOSS) && world->RemoveBossLevel(Int2(game_level->location_index, game_level->dungeon_level)))
		game->SetMusic();

	if(prev_action == A_ANIMATION)
	{
		action = A_NONE;
		current_animation = ANI_STAND;
	}
	animation = ANI_DIE;
	talking = false;
	mesh_inst->need_update = true;

	// sound
	Sound* sound = nullptr;
	if(data->sounds->Have(SOUND_DEATH))
		sound = data->sounds->Random(SOUND_DEATH);
	else if(data->sounds->Have(SOUND_PAIN))
		sound = data->sounds->Random(SOUND_PAIN);
	if(sound)
		PlaySound(sound, DIE_SOUND_DIST);

	// move physics
	UpdatePhysics();
}

//=================================================================================================
void Unit::DropGold(int count)
{
	gold -= count;
	sound_mgr->PlaySound2d(game_res->sCoins);

	// animacja wyrzucania
	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		// stwórz przedmiot
		GroundItem* item = new GroundItem;
		item->Register();
		item->item = Item::gold;
		item->count = count;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot) * 0.25f;
		item->pos.z -= cos(rot) * 0.25f;
		item->rot = Quat::RotY(Random(MAX_ANGLE));
		game_level->AddGroundItem(*area, item);

		// wyœlij info o animacji
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;
		}
	}
	else
	{
		// wyœlij komunikat o wyrzucaniu z³ota
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_GOLD;
		c.id = count;
	}
}

//=================================================================================================
bool Unit::IsDrunkman() const
{
	if(IsSet(data->flags, F_AI_DRUNKMAN))
		return true;
	else if(IsSet(data->flags3, F3_DRUNK_MAGE))
		return quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::MageCured;
	else if(IsSet(data->flags3, F3_DRUNKMAN_AFTER_CONTEST))
		return quest_mgr->quest_contest->state == Quest_Contest::CONTEST_DONE;
	else
		return false;
}

//=================================================================================================
void Unit::PlaySound(Sound* sound, float range)
{
	sound_mgr->PlaySound3d(sound, GetHeadSoundPos(), range);
}

//=================================================================================================
void Unit::CreatePhysics(bool position)
{
	float r = GetUnitRadius();
	float h = max(MIN_H, GetUnitHeight());

	btCapsuleShape* caps = new btCapsuleShape(r, h - r * 2);
	cobj = new btCollisionObject;
	cobj->setCollisionShape(caps);
	cobj->setUserPointer(this);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_UNIT);
	phy_world->addCollisionObject(cobj, CG_UNIT);

	if(position)
		UpdatePhysics();
}

//=================================================================================================
void Unit::UpdatePhysics(const Vec3* target_pos)
{
	Vec3 phy_pos = target_pos ? *target_pos : pos;
	if(live_state == ALIVE)
		phy_pos.y += max(MIN_H, GetUnitHeight()) / 2;

	btVector3 a_min, a_max;
	cobj->getWorldTransform().setOrigin(ToVector3(phy_pos));
	phy_world->UpdateAabb(cobj);
}

//=================================================================================================
Sound* Unit::GetSound(SOUND_ID sound_id) const
{
	if(data->sounds->Have(sound_id))
		return data->sounds->Random(sound_id);
	return nullptr;
}

//=================================================================================================
bool Unit::SetWeaponState(bool takes_out, WeaponType type, bool send)
{
	if(takes_out)
	{
		switch(weapon_state)
		{
		case WeaponState::Hidden:
			// wyjmij bron
			mesh_inst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
			action = A_TAKE_WEAPON;
			animation_state = AS_TAKE_WEAPON_START;
			weapon_taken = type;
			weapon_state = WeaponState::Taking;
			if(IsPlayer())
				player->last_weapon = type;
			break;
		case WeaponState::Hiding:
			if(weapon_hiding == type)
			{
				if(animation_state == AS_TAKE_WEAPON_START)
				{
					// jeszcze nie schowa³ tej broni, wy³¹cz grupê
					mesh_inst->Deactivate(1);
					action = usable ? A_USE_USABLE : A_NONE;
					weapon_taken = weapon_hiding;
					weapon_hiding = W_NONE;
					weapon_state = WeaponState::Taken;
				}
				else
				{
					// schowa³ broñ, zacznij wyci¹gaæ
					mesh_inst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					weapon_taken = weapon_hiding;
					weapon_hiding = W_NONE;
					weapon_state = WeaponState::Taking;
					animation_state = AS_TAKE_WEAPON_START;
					if(IsPlayer())
						player->last_weapon = type;
				}
			}
			else
			{
				if(animation_state == AS_TAKE_WEAPON_MOVED)
				{
					// unit is hiding weapon & animation almost ended, start taking out weapon
					mesh_inst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					animation_state = AS_TAKE_WEAPON_START;
					weapon_taken = type;
					weapon_hiding = W_NONE;
					weapon_state = WeaponState::Taking;
					if(IsPlayer())
						player->last_weapon = type;
				}
				else
				{
					// set next weapon to take out
					weapon_taken = type;
				}
			}
			break;
		case WeaponState::Taking:
			if(weapon_taken == type)
				return false;
			if(animation_state == AS_TAKE_WEAPON_START)
			{
				// jeszcze nie wyj¹³ broni, zacznij wyjmowaæ inn¹
				mesh_inst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				weapon_state = WeaponState::Taking;
				weapon_hiding = W_NONE;
				if(IsPlayer())
					player->last_weapon = type;
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				SetBit(mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
				weapon_state = WeaponState::Hiding;
				weapon_hiding = weapon_taken;
			}
			weapon_taken = type;
			animation_state = AS_TAKE_WEAPON_START;
			break;
		case WeaponState::Taken:
			if(weapon_taken == type)
				return false;
			// hide previous weapon
			if(action == A_SHOOT)
				game_level->FreeBowInstance(bow_instance);
			mesh_inst->Play(GetTakeWeaponAnimation(weapon_taken), PLAY_ONCE | PLAY_PRIO1 | PLAY_BACK, 1);
			action = A_TAKE_WEAPON;
			animation_state = AS_TAKE_WEAPON_START;
			weapon_state = WeaponState::Hiding;
			weapon_hiding = weapon_taken;
			weapon_taken = type;
			break;
		}
	}
	else // chowa
	{
		switch(weapon_state)
		{
		case WeaponState::Hidden:
			// schowana to schowana, nie ma co sprawdzaæ czy to ta
			return false;
		case WeaponState::Hiding:
			if(type == W_NONE)
				weapon_taken = W_NONE;
			if(weapon_hiding == type || type == W_NONE)
				return false;
			// chowa z³¹ broñ, zamieñ
			weapon_hiding = type;
			break;
		case WeaponState::Taking:
			if(animation_state == AS_TAKE_WEAPON_START)
			{
				// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
				mesh_inst->Deactivate(1);
				action = usable ? A_USE_USABLE : A_NONE;
				weapon_taken = W_NONE;
				weapon_state = WeaponState::Hidden;
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				SetBit(mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
				weapon_hiding = weapon_taken;
				weapon_taken = W_NONE;
				weapon_state = WeaponState::Hiding;
				animation_state = AS_TAKE_WEAPON_START;
			}
			break;
		case WeaponState::Taken:
			// zacznij chowaæ
			if(action == A_SHOOT)
				game_level->FreeBowInstance(bow_instance);
			mesh_inst->Play(GetTakeWeaponAnimation(weapon_taken == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
			weapon_hiding = weapon_taken;
			weapon_taken = W_NONE;
			weapon_state = WeaponState::Hiding;
			action = A_TAKE_WEAPON;
			animation_state = AS_TAKE_WEAPON_START;
			break;
		}
	}

	if(send && Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TAKE_WEAPON;
		c.unit = this;
		c.id = takes_out ? 0 : 1;
	}

	return true;
}

//=============================================================================
void Unit::SetWeaponStateInstant(WeaponState weapon_state, WeaponType type)
{
	this->weapon_state = weapon_state;
	this->weapon_taken = type;
	weapon_hiding = W_NONE;
}

//=============================================================================
void Unit::SetTakeHideWeaponAnimationToEnd(bool hide, bool break_action)
{
	if(hide || weapon_state == WeaponState::Hiding)
	{
		if(break_action && animation_state == AS_TAKE_WEAPON_START)
			SetWeaponStateInstant(WeaponState::Taken, weapon_hiding);
		else
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	}
	else if(weapon_state == WeaponState::Taking)
	{
		if(break_action && animation_state == AS_TAKE_WEAPON_START)
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		else
			SetWeaponStateInstant(WeaponState::Taken, weapon_taken);
	}
	if(action == A_TAKE_WEAPON)
		action = A_NONE;
}

//=================================================================================================
void Unit::UpdateInventory(bool notify)
{
	bool changes = false;
	int index = 0;
	const Item* prev_slots[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
		prev_slots[i] = slots[i];

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->team_count != 0 || !CanWear(it->item))
			continue;

		ITEM_SLOT target_slot;
		if(IsBetterItem(it->item, nullptr, nullptr, &target_slot))
		{
			if(slots[target_slot])
			{
				RemoveItemEffects(slots[target_slot], target_slot);
				ApplyItemEffects(it->item, target_slot);
				std::swap(slots[target_slot], it->item);
			}
			else
			{
				ApplyItemEffects(it->item, target_slot);
				slots[target_slot] = it->item;
				it->item = nullptr;
			}
			changes = true;
		}
	}

	if(changes)
	{
		RemoveNullItems(items);
		SortItems(items);

		if(Net::IsOnline() && net->active_players > 1 && notify)
		{
			for(int i = 0; i < SLOT_MAX; ++i)
			{
				if(slots[i] != prev_slots[i] && IsVisible((ITEM_SLOT)i))
				{
					NetChange& c = Add1(Net::changes);
					c.unit = this;
					c.type = NetChange::CHANGE_EQUIPMENT;
					c.id = i;
				}
			}
		}
	}
}

//=================================================================================================
bool Unit::IsEnemy(Unit& u, bool ignore_dont_attack) const
{
	if(in_arena == -1 && u.in_arena == -1)
	{
		if(!ignore_dont_attack)
		{
			if(IsAI() && IsDontAttack())
				return false;
			if(u.IsAI() && u.IsDontAttack())
				return false;
		}

		UNIT_GROUP g1 = data->group,
			g2 = u.data->group;

		if(IsTeamMember())
			g1 = G_TEAM;
		if(u.IsTeamMember())
			g2 = G_TEAM;

		if(g1 == g2)
			return false;
		else if(g1 == G_CITIZENS)
		{
			if(g2 == G_CRAZIES)
				return true;
			else if(g2 == G_TEAM)
				return team->IsBandit() || WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_CRAZIES)
		{
			if(g2 == G_CITIZENS)
				return true;
			else if(g2 == G_TEAM)
				return team->crazies_attack || WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_TEAM)
		{
			if(u.WantAttackTeam())
				return true;
			else if(g2 == G_CITIZENS)
				return team->IsBandit();
			else if(g2 == G_CRAZIES)
				return team->crazies_attack;
			else
				return true;
		}
		else
			return true;
	}
	else
	{
		if(in_arena == -1 || u.in_arena == -1)
			return false;
		else if(in_arena == u.in_arena)
			return false;
		else
			return true;
	}
}

//=================================================================================================
bool Unit::IsFriend(Unit& u, bool check_arena_attack) const
{
	if(in_arena == -1 && u.in_arena == -1)
	{
		if(IsTeamMember())
		{
			if(u.IsTeamMember())
				return true;
			else if(u.IsAI() && !team->IsBandit() && u.IsAssist())
				return true;
			else
				return false;
		}
		else if(u.IsTeamMember())
		{
			if(IsAI() && !team->IsBandit() && IsAssist())
				return true;
			else
				return false;
		}
		else
			return (data->group == u.data->group);
	}
	else
	{
		if(check_arena_attack)
		{
			// prevent attacking viewers when on arena
			if(in_arena == -1 || u.in_arena == -1)
				return true;
		}
		return in_arena == u.in_arena;
	}
}

//=================================================================================================
Color Unit::GetRelationColor(Unit& u) const
{
	if(IsEnemy(u))
		return Color::Red;
	else if(IsFriend(u))
		return Color::Green;
	else
		return Color::Yellow;
}

//=================================================================================================
void Unit::RefreshStock()
{
	assert(data->trader);

	bool refresh;
	int worldtime = world->GetWorldtime();
	if(stock)
		refresh = (worldtime - stock->date >= 10 && busy != Busy_Trading);
	else
	{
		stock = new TraderStock;
		refresh = true;
	}

	if(refresh)
	{
		stock->date = worldtime;
		stock->items.clear();
		data->trader->stock->Parse(stock->items);
		if(!game_level->entering)
		{
			for(ItemSlot& slot : stock->items)
				game_res->PreloadItem(slot.item);
		}
	}
}

//=================================================================================================
void Unit::AddDialog(Quest_Scripted* quest, GameDialog* dialog, int priority)
{
	assert(quest && dialog);
	dialogs.push_back({ dialog, quest, priority });
	quest->AddDialogPtr(this);
}

//=================================================================================================
void Unit::AddDialogS(Quest_Scripted* quest, const string& dialog_id, int priority)
{
	GameDialog* dialog = quest->GetDialog(dialog_id);
	if(!dialog)
		throw ScriptException("Missing quest dialog '%s'.", dialog_id.c_str());
	AddDialog(quest, dialog, priority);
}

//=================================================================================================
void Unit::RemoveDialog(Quest_Scripted* quest, bool cleanup)
{
	assert(quest);
	LoopAndRemove(dialogs, [quest](QuestDialog& dialog)
	{
		if(dialog.quest == quest)
			return true;
		return false;
	});
	if(!cleanup)
	{
		quest->RemoveDialogPtr(this);
		if(busy == Busy_Talking)
		{
			DialogContext* ctx = game->FindDialogContext(this);
			if(ctx)
				ctx->RemoveQuestDialog(quest);
		}
	}
}

//=================================================================================================
void Unit::AddEventHandler(Quest_Scripted* quest, EventType type)
{
	assert(type == EVENT_UPDATE || type == EVENT_DIE);

	Event e;
	e.quest = quest;
	e.type = type;
	events.push_back(e);

	EventPtr event;
	event.source = EventPtr::UNIT;
	event.type = type;
	event.unit = this;
	quest->AddEventPtr(event);
}

//=================================================================================================
void Unit::RemoveEventHandler(Quest_Scripted* quest, EventType type, bool cleanup)
{
	LoopAndRemove(events, [=](Event& e)
	{
		return e.quest == quest && (type == EVENT_ANY || type == e.type);
	});

	if(!cleanup)
	{
		EventPtr event;
		event.source = EventPtr::UNIT;
		event.type = type;
		event.unit = this;
		quest->RemoveEventPtr(event);
	}
}

//=================================================================================================
void Unit::RemoveAllEventHandlers()
{
	for(Event& e : events)
	{
		EventPtr event;
		event.source = EventPtr::UNIT;
		event.type = EVENT_ANY;
		event.unit = this;
		e.quest->RemoveEventPtr(event);
	}
	events.clear();
}

//=================================================================================================
void Unit::OrderClear()
{
	if(order)
		order->Free();
	if(hero && hero->team_member)
	{
		order = UnitOrderEntry::Get();
		order->order = ORDER_FOLLOW;
		order->unit = team->GetLeader();
		order->timer = 0.f;
	}
	else
	{
		order = nullptr;
		if(ai && ai->state == AIController::Idle)
		{
			ai->st.idle.action = AIController::Idle_None;
			ai->timer = 0.f;
		}
	}
}

//=================================================================================================
void Unit::OrderNext()
{
	if(!order)
		return;
	UnitOrderEntry* next = order->next;
	if(next)
	{
		order->next = nullptr;
		order->Free();
		order = next;

		switch(order->order)
		{
		case ORDER_WANDER:
			if(ai)
				ai->loc_timer = Random(5.f, 10.f);
			break;
		case ORDER_ESCAPE_TO:
		case ORDER_ESCAPE_TO_UNIT:
			if(ai)
			{
				ai->state = AIController::Escape;
				ai->st.escape.room = nullptr;
			}
			break;
		}
	}
	else
		OrderClear();
}

//=================================================================================================
void Unit::OrderReset()
{
	if(order)
		order->Free();
	order = UnitOrderEntry::Get();
	order->timer = -1.f;
	if(ai && ai->state == AIController::Idle)
	{
		ai->timer = 0.f;
		ai->st.idle.action = AIController::Idle_None;
	}
}

//=================================================================================================
void Unit::OrderAttack()
{
	if(data->group == G_CRAZIES)
	{
		if(!team->crazies_attack)
		{
			team->crazies_attack = true;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_FLAGS;
			}
		}
	}
	else
	{
		for(Unit* unit : area->units)
		{
			if(unit->dont_attack && unit->IsEnemy(*team->leader, true) && !IsSet(unit->data->flags, F_PEACEFUL))
			{
				unit->dont_attack = false;
				unit->ai->change_ai_mode = true;
			}
		}
	}
}

//=================================================================================================
UnitOrderEntry* Unit::OrderWander()
{
	OrderReset();
	order->order = ORDER_WANDER;
	if(ai)
		ai->loc_timer = Random(5.f, 10.f);
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderWait()
{
	OrderReset();
	order->order = ORDER_WAIT;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderFollow(Unit* target)
{
	assert(target);
	OrderReset();
	order->order = ORDER_FOLLOW;
	order->unit = target;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderLeave()
{
	OrderReset();
	order->order = ORDER_LEAVE;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderMove(const Vec3& pos)
{
	OrderReset();
	order->order = ORDER_MOVE;
	order->pos = pos;
	order->move_type = MOVE_RUN;
	order->range = 0.1f;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderLookAt(const Vec3& pos)
{
	OrderReset();
	order->order = ORDER_LOOK_AT;
	order->pos = pos;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderEscapeTo(const Vec3& pos)
{
	OrderReset();
	order->order = ORDER_ESCAPE_TO;
	order->pos = pos;
	if(ai)
	{
		ai->state = AIController::Escape;
		ai->st.escape.room = nullptr;
	}
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderEscapeToUnit(Unit* unit)
{
	assert(unit);
	OrderReset();
	order->order = ORDER_ESCAPE_TO_UNIT;
	order->unit = unit;
	order->pos = unit->pos;
	if(ai)
	{
		ai->state = AIController::Escape;
		ai->st.escape.room = nullptr;
	}
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderGoToInn()
{
	OrderReset();
	order->order = ORDER_GOTO_INN;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderGuard(Unit* target)
{
	assert(target);
	OrderReset();
	order->order = ORDER_GUARD;
	order->unit = target;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderAutoTalk(bool leader, GameDialog* dialog, Quest* quest)
{
	if(quest)
		assert(dialog);

	OrderReset();
	order->order = ORDER_AUTO_TALK;
	if(!leader)
	{
		order->auto_talk = AutoTalkMode::Yes;
		order->timer = AUTO_TALK_WAIT;
	}
	else
	{
		order->auto_talk = AutoTalkMode::Leader;
		order->timer = 0.f;
	}
	order->auto_talk_dialog = dialog;
	order->auto_talk_quest = quest;
	return order;
}

//=================================================================================================
void Unit::Talk(cstring text, int play_anim)
{
	assert(text && Net::IsLocal());

	game_gui->level_gui->AddSpeechBubble(this, text);

	// animation
	int ani = 0;
	if(play_anim != 0 && data->type == UNIT_TYPE::HUMAN)
	{
		if(play_anim == -1)
		{
			if(Rand() % 3 != 0)
				ani = Rand() % 2 + 1;
		}
		else
			ani = Clamp(play_anim, 1, 2);
	}
	if(ani != 0)
	{
		mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		animation = ANI_PLAY;
		action = A_ANIMATION;
	}

	// sent
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = this;
		c.str = StringPool.Get();
		*c.str = text;
		c.id = ani;
		c.count = 0;
		net->net_strs.push_back(c.str);
	}
}

//=================================================================================================
float Unit::GetStaminaAttackSpeedMod() const
{
	float sp = max(0.f, GetStaminap());
	const float limit = 0.25f;
	const float mod = 0.4f;
	if(sp >= limit)
		return 1.f;
	float t = (limit - sp) * (1.f / limit);
	return 1.f - mod * t;
}

//=================================================================================================
void Unit::RotateTo(const Vec3& pos, float dt)
{
	float dir = Vec3::LookAtAngle(this->pos, pos);
	if(!Equal(rot, dir))
	{
		const float rot_speed = GetRotationSpeed() * dt;
		const float rot_diff = AngleDiff(rot, dir);
		if(ShortestArc(rot, dir) > 0.f)
			animation = ANI_RIGHT;
		else
			animation = ANI_LEFT;
		if(rot_diff < rot_speed)
			rot = dir;
		else
		{
			const float d = Sign(ShortestArc(rot, dir)) * rot_speed;
			rot = Clip(rot + d);
		}
		changed = true;
	}
}

//=================================================================================================
void Unit::RotateTo(const Vec3& pos)
{
	rot = Vec3::LookAtAngle(this->pos, pos);
	if(game_level->entering && ai)
		ai->start_rot = rot;
	else
		changed = true;
}

void Unit::RotateTo(float rot)
{
	this->rot = rot;
	if(game_level->entering && ai)
		ai->start_rot = rot;
	else
		changed = true;
}

UnitOrderEntry* UnitOrderEntry::NextOrder()
{
	next = UnitOrderEntry::Get();
	next->timer = -1.f;
	return next;
}

UnitOrderEntry* UnitOrderEntry::WithTimer(float timer)
{
	this->timer = timer;
	return this;
}

UnitOrderEntry* UnitOrderEntry::WithMoveType(MoveType move_type)
{
	this->move_type = move_type;
	return this;
}

UnitOrderEntry* UnitOrderEntry::WithRange(float range)
{
	this->range = range;
	return this;
}

UnitOrderEntry* UnitOrderEntry::ThenWander()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_WANDER;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenWait()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_WAIT;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenFollow(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_FOLLOW;
	o->unit = target;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenLeave()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_LEAVE;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenMove(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_MOVE;
	o->pos = pos;
	o->move_type = MOVE_RUN;
	o->range = 0.1f;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenLookAt(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_LOOK_AT;
	o->pos = pos;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenEscapeTo(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_ESCAPE_TO;
	o->pos = pos;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenEscapeToUnit(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_ESCAPE_TO_UNIT;
	o->unit = target;
	o->pos = target->pos;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenGoToInn()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_GOTO_INN;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenGuard(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_GUARD;
	o->unit = target;
	return o;
}

UnitOrderEntry* UnitOrderEntry::ThenAutoTalk(bool leader, GameDialog* dialog, Quest* quest)
{
	if(quest)
		assert(dialog);

	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_AUTO_TALK;
	if(!leader)
	{
		o->auto_talk = AutoTalkMode::Yes;
		o->timer = Unit::AUTO_TALK_WAIT;
	}
	else
	{
		o->auto_talk = AutoTalkMode::Leader;
		o->timer = 0.f;
	}
	o->auto_talk_dialog = dialog;
	o->auto_talk_quest = quest;
	return o;
}

//=================================================================================================
void Unit::StopUsingUsable(bool send)
{
	animation = ANI_STAND;
	animation_state = AS_USE_USABLE_MOVE_TO_ENDPOINT;
	timer = 0.f;
	used_item = nullptr;

	const float unit_radius = GetUnitRadius();

	game_level->global_col.clear();
	Level::IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { this, nullptr };
	ignore.ignored_units = ignore_units;
	game_level->GatherCollisionObjects(*area, game_level->global_col, pos, 2.f + unit_radius, &ignore);

	Vec3 tmp_pos = target_pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!game_level->Collide(game_level->global_col, tmp_pos, unit_radius))
		{
			if(i != 0 && area->have_terrain)
				tmp_pos.y = game_level->terrain->GetH(tmp_pos);
			target_pos = tmp_pos;
			ok = true;
			break;
		}

		tmp_pos = target_pos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

		if(i < 10)
			radius += 0.2f;
	}

	assert(ok);

	if(cobj)
		UpdatePhysics(&target_pos);

	if(send && Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.unit = this;
		c.id = usable->id;
		c.count = USE_USABLE_STOP;
	}
}

//=================================================================================================
void Unit::CheckAutoTalk(float dt)
{
	if(action != A_NONE && action != A_USE_USABLE)
	{
		if(order->auto_talk == AutoTalkMode::Wait)
		{
			order->auto_talk = AutoTalkMode::Yes;
			order->timer = AUTO_TALK_WAIT;
		}
		return;
	}

	// timer to not check everything every frame
	order->timer -= dt;
	if(order->timer > 0.f)
		return;
	order->timer = AUTO_TALK_WAIT;

	// find near players
	struct NearUnit
	{
		Unit* unit;
		float dist;
	};
	static vector<NearUnit> near_units;

	const bool leader_mode = (order->auto_talk == AutoTalkMode::Leader);

	for(Unit& u : team->members)
	{
		if(u.IsPlayer())
		{
			// if not leader (in leader mode) or busy - don't check this unit
			if((leader_mode && &u != team->leader)
				|| (u.player->dialog_ctx->dialog_mode || u.busy != Busy_No
				|| !u.IsStanding() || u.player->action != PlayerAction::None))
				continue;
			float dist = Vec3::Distance(pos, u.pos);
			if(dist <= 8.f || leader_mode)
				near_units.push_back({ &u, dist });
		}
	}

	// if no nearby available players found, return
	if(near_units.empty())
	{
		if(order->auto_talk == AutoTalkMode::Wait)
			order->auto_talk = AutoTalkMode::Yes;
		return;
	}

	// sort by distance
	std::sort(near_units.begin(), near_units.end(), [](const NearUnit& nu1, const NearUnit& nu2) { return nu1.dist < nu2.dist; });

	// get near player that don't have enemies nearby
	PlayerController* talk_player = nullptr;
	for(auto& near_unit : near_units)
	{
		Unit& talk_target = *near_unit.unit;
		if(game_level->CanSee(*this, talk_target))
		{
			bool ok = true;
			for(vector<Unit*>::iterator it2 = area->units.begin(), end2 = area->units.end(); it2 != end2; ++it2)
			{
				Unit& check_unit = **it2;
				if(&talk_target == &check_unit || this == &check_unit)
					continue;

				if(check_unit.IsAlive() && talk_target.IsEnemy(check_unit) && check_unit.IsAI() && !check_unit.dont_attack
					&& Vec3::Distance2d(talk_target.pos, check_unit.pos) < ALERT_RANGE && game_level->CanSee(check_unit, talk_target))
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				talk_player = talk_target.player;
				break;
			}
		}
	}

	// start dialog or wait
	if(talk_player)
	{
		if(order->auto_talk == AutoTalkMode::Yes)
		{
			order->auto_talk = AutoTalkMode::Wait;
			order->timer = 1.f;
		}
		else
		{
			talk_player->StartDialog(this, order->auto_talk_dialog, order->auto_talk_quest);
			OrderNext();
		}
	}
	else if(order->auto_talk == AutoTalkMode::Wait)
	{
		order->auto_talk = AutoTalkMode::Yes;
		order->timer = AUTO_TALK_WAIT;
	}

	near_units.clear();
}

//=================================================================================================
float Unit::GetAbilityPower(Ability& ability) const
{
	float bonus;
	if(IsSet(ability.flags, Ability::Cleric))
		bonus = float(Get(AttributeId::CHA) - 25 + Get(SkillId::GODS_MAGIC));
	else if(IsSet(ability.flags, Ability::Mage))
	{
		if(IsSet(data->flags2, F2_FIXED_STATS))
			bonus = (float)data->spell_power / 20;
		else
			bonus = float(Get(AttributeId::INT) - 25 + Get(SkillId::MYSTIC_MAGIC) + CalculateMagicPower() * 10) / 20;
	}
	else if(IsSet(ability.flags, Ability::Strength))
		bonus = float(Get(AttributeId::STR));
	else
		bonus = float(CalculateMagicPower()) / 10 + level;
	return bonus * ability.dmg_bonus + ability.dmg;
}

//=================================================================================================
void Unit::CastSpell()
{
	Ability& ability = *act.cast.ability;

	Mesh::Point* point = mesh_inst->mesh->GetPoint(NAMES::point_cast);
	assert(point);

	mesh_inst->SetupBones();

	Matrix m = point->mat * mesh_inst->mat_bones[point->bone] * (Matrix::RotationY(rot) * Matrix::Translation(pos));

	Vec3 coord = Vec3::TransformZero(m);
	float dmg = GetAbilityPower(ability);

	switch(ability.type)
	{
	case Ability::Ball:
	case Ability::Point:
		{
			int count = 1;
			if(IsSet(ability.flags, Ability::Triple))
				count = 3;

			float expected_rot = Clip(-Vec3::Angle2d(coord, target_pos) + PI / 2);
			float current_rot = Clip(rot + PI);
			AdjustAngle(current_rot, expected_rot, ToRadians(10.f));

			for(int i = 0; i < count; ++i)
			{
				Bullet* bullet = new Bullet;
				area->tmp->bullets.push_back(bullet);

				bullet->Register();
				bullet->level = level + CalculateMagicPower();
				bullet->backstab = 0.25f;
				bullet->pos = coord;
				bullet->attack = dmg;
				bullet->rot = Vec3(0, Clip(current_rot + (IsPlayer() ? Random(-0.025f, 0.025f) : Random(-0.05f, 0.05f))), 0);
				bullet->mesh = ability.mesh;
				bullet->tex = ability.tex;
				bullet->tex_size = ability.size;
				bullet->speed = ability.speed;
				bullet->timer = ability.range / (ability.speed - 1);
				bullet->owner = this;
				bullet->trail = nullptr;
				bullet->pe = nullptr;
				bullet->ability = &ability;
				bullet->start_pos = bullet->pos;

				// ustal z jak¹ si³¹ rzuciæ kul¹
				if(ability.type == Ability::Ball)
				{
					float dist = Vec3::Distance2d(pos, target_pos);
					float t = dist / ability.speed;
					float h = (target_pos.y + (IsPlayer() ? Random(-0.25f, 0.25f) : Random(-0.5f, 0.5f))) - bullet->pos.y;
					bullet->yspeed = h / t + (10.f * t) / 2;
				}
				else if(ability.type == Ability::Point)
				{
					float dist = Vec3::Distance2d(pos, target_pos);
					float t = dist / ability.speed;
					float h = (target_pos.y + (IsPlayer() ? Random(-0.25f, 0.25f) : Random(-0.5f, 0.5f))) - bullet->pos.y;
					bullet->yspeed = h / t;
				}

				if(ability.tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = ability.tex_particle;
					pe->emision_interval = 0.1f;
					pe->life = -1;
					pe->particle_life = 0.5f;
					pe->emisions = -1;
					pe->spawn_min = 3;
					pe->spawn_max = 4;
					pe->max_particles = 50;
					pe->pos = bullet->pos;
					pe->speed_min = Vec3(-1, -1, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-ability.size, -ability.size, -ability.size);
					pe->pos_max = Vec3(ability.size, ability.size, ability.size);
					pe->size = ability.size_particle;
					pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					area->tmp->pes.push_back(pe);
					bullet->pe = pe;
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CREATE_SPELL_BALL;
					c << ability.hash
						<< bullet->id
						<< id
						<< bullet->start_pos
						<< bullet->rot.y
						<< bullet->yspeed;
				}
			}
		}
		break;
	case Ability::Ray:
		if(ability.effect == Ability::Electro)
		{
			Electro* e = new Electro;
			e->Register();
			e->area = area;
			e->hitted.push_back(this);
			e->dmg = dmg;
			e->owner = this;
			e->ability = &ability;
			e->startPos = pos;

			Vec3 hitpoint;
			Unit* hitted;

			target_pos.y += Random(-0.5f, 0.5f);
			Vec3 dir = target_pos - coord;
			dir.Normalize();
			Vec3 target = coord + dir * ability.range;

			if(game_level->RayTest(coord, target, this, hitpoint, hitted))
			{
				if(hitted)
				{
					// trafiono w cel
					e->valid = true;
					e->hitsome = true;
					e->hitted.push_back(hitted);
					e->AddLine(coord, hitpoint);
				}
				else
				{
					// trafienie w obiekt
					e->valid = false;
					e->hitsome = true;
					e->AddLine(coord, hitpoint);
				}
			}
			else
			{
				// w nic nie trafiono
				e->valid = false;
				e->hitsome = false;
				e->AddLine(coord, target);
			}

			area->tmp->electros.push_back(e);

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CREATE_ELECTRO;
				c.e_id = e->id;
				c.pos = e->lines[0].from;
				memcpy(c.f, &e->lines[0].to, sizeof(Vec3));
			}
		}
		else if(ability.effect == Ability::Drain)
		{
			Vec3 hitpoint;
			Unit* hitted;

			target_pos.y += Random(-0.5f, 0.5f);

			if(game_level->RayTest(coord, target_pos, this, hitpoint, hitted) && hitted)
			{
				// trafiono w cel
				if(!IsSet(hitted->data->flags2, F2_BLOODLESS) && !IsFriend(*hitted, true))
				{
					Drain& drain = Add1(area->tmp->drains);
					drain.target = this;

					hitted->GiveDmg(dmg, this, nullptr, DMG_MAGICAL);

					drain.pe = area->tmp->pes.back();
					drain.t = 0.f;
					drain.pe->manual_delete = 1;
					drain.pe->speed_min = Vec3(-3, 0, -3);
					drain.pe->speed_max = Vec3(3, 3, 3);

					dmg *= hitted->CalculateMagicResistance();
					hp += dmg;
					if(hp > hpmax)
						hp = hpmax;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::UPDATE_HP;
						c.unit = this;

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::CREATE_DRAIN;
						c2.unit = this;
					}
				}
			}
		}
		break;
	case Ability::Target:
		{
			Unit* target = act.cast.target;
			if(!target) // pre V_0_12
			{
				for(Unit* u : area->units)
				{
					if(u->live_state == DEAD
						&& !IsEnemy(*u)
						&& IsSet(u->data->flags, F_UNDEAD)
						&& Vec3::Distance(target_pos, u->pos) < 0.5f)
					{
						target = u;
						break;
					}
				}
			}

			if(ai)
			{
				ai->state = AIController::Idle;
				ai->st.idle.action = AIController::Idle_None;
			}

			// check if target is not too far
			if(!target || target->area != area || Vec3::Distance(pos, target->pos) > ability.range * 1.5f)
				break;

			if(ability.effect == Ability::Raise)
			{
				target->Standup();
				target->hp = target->hpmax;
				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = target;
				}

				// particle effect
				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = ability.tex_particle;
				pe->emision_interval = 0.01f;
				pe->life = 0.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 16;
				pe->spawn_max = 25;
				pe->max_particles = 25;
				pe->pos = target->pos;
				pe->pos.y += target->GetUnitHeight() / 2;
				pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
				pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
				pe->pos_min = Vec3(-ability.size, -ability.size, -ability.size);
				pe->pos_max = Vec3(ability.size, ability.size, ability.size);
				pe->size = ability.size_particle;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				area->tmp->pes.push_back(pe);

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::RAISE_EFFECT;
					c.pos = pe->pos;
				}
			}
			else if(ability.effect == Ability::Heal)
			{
				// heal target
				if(!IsSet(target->data->flags, F_UNDEAD) && !IsSet(target->data->flags2, F2_CONSTRUCT) && target->hp != target->hpmax)
				{
					target->hp = Min(target->hp + dmg, target->hpmax);
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
				pe->tex = ability.tex_particle;
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
				pe->size = ability.size_particle;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				area->tmp->pes.push_back(pe);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::HEAL_EFFECT;
					c.pos = pe->pos;
				}
			}
		}
		break;
	case Ability::Summon:
		{
			// despawn old
			Unit* existing_unit = game_level->FindUnit([&](Unit* u) { return u->summoner == this; });
			if(existing_unit)
			{
				team->RemoveMember(existing_unit);
				game_level->RemoveUnit(existing_unit);
			}

			// spawn new
			Unit* new_unit = game_level->SpawnUnitNearLocation(*area, target_pos, *ability.unit, nullptr, level);
			if(new_unit)
			{
				new_unit->summoner = this;
				new_unit->in_arena = in_arena;
				if(new_unit->in_arena != -1)
					game->arena->units.push_back(new_unit);
				team->AddMember(new_unit, HeroType::Visitor);
				new_unit->order->unit = this; // follow summoner
				game_level->SpawnUnitEffect(*new_unit);
			}
		}
		break;
	}

	// train
	if(IsPlayer())
	{
		if(IsSet(ability.flags, Ability::Cleric))
			player->Train(TrainWhat::CastCleric, 2000.f, 0);
		else if(IsSet(ability.flags, Ability::Mage))
		{
			const int value = ability.level > 0 ? ability.level * 200 : 100;
			player->Train(TrainWhat::CastMage, (float)value, 0);
		}
	}

	// sound effect
	if(ability.sound_cast)
	{
		sound_mgr->PlaySound3d(ability.sound_cast, coord, ability.sound_cast_dist);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPELL_SOUND;
			c.ability = &ability;
			c.pos = coord;
		}
	}
}

//=================================================================================================
void Unit::Update(float dt)
{
	// update effects and mouth moving
	if(IsAlive())
	{
		UpdateEffects(dt);

		if(IsStanding() && talking)
		{
			talk_timer += dt;
			mesh_inst->need_update = true;
		}

		if(Net::IsLocal())
		{
			// hurt sound timer since last hit, timer since last stun (to prevent stunlock)
			hurt_timer -= dt;
			last_bash -= dt;

			if(moved)
			{
				// unstuck units after being force moved (by bull charge)
				static vector<Unit*> targets;
				targets.clear();
				float t;
				bool in_dash = false;
				game_level->ContactTest(cobj, [&](btCollisionObject* obj, bool first)
				{
					if(first)
					{
						int flags = obj->getCollisionFlags();
						if(!IsSet(flags, CG_UNIT))
							return false;
						else
						{
							if(obj->getUserPointer() == this)
								return false;
							return true;
						}
					}
					else
					{
						Unit* target = (Unit*)obj->getUserPointer();
						if(target->action == A_DASH)
							in_dash = true;
						targets.push_back(target);
						return true;
					}
				}, true);

				if(!in_dash)
				{
					if(targets.empty())
						moved = false;
					else
					{
						Vec3 center(0, 0, 0);
						for(auto target : targets)
						{
							center += pos - target->pos;
							target->moved = true;
						}
						center /= (float)targets.size();
						center.y = 0;
						center.Normalize();
						center *= dt;
						game_level->LineTest(cobj->getCollisionShape(), GetPhysicsPos(), center, [&](btCollisionObject* obj, bool)
						{
							int flags = obj->getCollisionFlags();
							if(IsSet(flags, CG_TERRAIN | CG_UNIT))
								return LT_IGNORE;
							return LT_COLLIDE;
						}, t);
						if(t == 1.f)
						{
							pos += center;
							Moved(false, true);
						}
					}
				}
			}
		}
	}

	// zmieñ podstawow¹ animacjê
	if(animation != current_animation)
	{
		changed = true;
		switch(animation)
		{
		case ANI_WALK:
			mesh_inst->Play(NAMES::ani_move, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				mesh_inst->groups[0].speed = GetWalkSpeed() / data->walk_speed;
			break;
		case ANI_WALK_BACK:
			mesh_inst->Play(NAMES::ani_move, PLAY_BACK | PLAY_PRIO1, 0);
			if(!Net::IsClient())
				mesh_inst->groups[0].speed = GetWalkSpeed() / data->walk_speed;
			break;
		case ANI_RUN:
			mesh_inst->Play(NAMES::ani_run, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				mesh_inst->groups[0].speed = GetRunSpeed() / data->run_speed;
			break;
		case ANI_LEFT:
			mesh_inst->Play(NAMES::ani_left, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				mesh_inst->groups[0].speed = GetRotationSpeed() / data->rot_speed;
			break;
		case ANI_RIGHT:
			mesh_inst->Play(NAMES::ani_right, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				mesh_inst->groups[0].speed = GetRotationSpeed() / data->rot_speed;
			break;
		case ANI_STAND:
			mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
			break;
		case ANI_BATTLE:
			mesh_inst->Play(NAMES::ani_battle, PLAY_PRIO1, 0);
			break;
		case ANI_BATTLE_BOW:
			mesh_inst->Play(NAMES::ani_battle_bow, PLAY_PRIO1, 0);
			break;
		case ANI_DIE:
			mesh_inst->Play(NAMES::ani_die, PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
			break;
		case ANI_PLAY:
			break;
		case ANI_IDLE:
			break;
		case ANI_KNEELS:
			mesh_inst->Play("kleka", PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
			break;
		default:
			assert(0);
			break;
		}
		current_animation = animation;
	}

	// aktualizuj animacjê
	mesh_inst->Update(dt);

	// koniec animacji idle
	if(animation == ANI_IDLE && mesh_inst->IsEnded())
	{
		mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		animation = ANI_STAND;
	}

	// zmieñ stan z umiera na umar³ i stwórz krew (chyba ¿e tylko upad³)
	if(!IsStanding())
	{
		// move corpse that thanks to animation is now not lootable
		if(Net::IsLocal() && (Any(live_state, DYING, FALLING) || action == A_POSITION_CORPSE))
		{
			Vec3 center = GetLootCenter();
			game_level->global_col.clear();
			Level::IgnoreObjects ignore = { 0 };
			ignore.ignore_units = true;
			ignore.ignore_doors = true;
			game_level->GatherCollisionObjects(*area, game_level->global_col, center, 0.25f, &ignore);
			if(game_level->Collide(game_level->global_col, center, 0.25f))
			{
				Vec3 dir = pos - center;
				dir.y = 0;
				pos += dir * dt * 2;
				visual_pos = pos;
				moved = true;
				action = A_POSITION_CORPSE;
				changed = true;
			}
			else
				action = A_NONE;
		}

		if(Any(live_state, DYING, FALLING) && mesh_inst->IsEnded())
		{
			if(live_state == DYING)
			{
				live_state = DEAD;
				game_level->CreateBlood(*area, *this);
				if(summoner && Net::IsLocal())
				{
					team->RemoveMember(this);
					action = A_DESPAWN;
					timer = Random(2.5f, 5.f);
					summoner = nullptr;
				}
			}
			else
				live_state = FALL;
		}

		if(action != A_POSITION && action != A_DESPAWN)
		{
			UpdateStaminaAction();
			return;
		}
	}

	const int group_index = mesh_inst->mesh->head.n_groups - 1;

	// aktualizuj akcjê
	switch(action)
	{
	case A_NONE:
		break;
	case A_TAKE_WEAPON:
		if(weapon_state == WeaponState::Taking)
		{
			if(animation_state == AS_TAKE_WEAPON_START && (mesh_inst->GetProgress(1) >= data->frames->t[F_TAKE_WEAPON] || mesh_inst->IsEnded(1)))
				animation_state = AS_TAKE_WEAPON_MOVED;
			if(mesh_inst->IsEnded(1))
			{
				weapon_state = WeaponState::Taken;
				if(usable)
				{
					action = A_USE_USABLE;
					animation_state = AS_USE_USABLE_USING;
				}
				else
					action = A_NONE;
				mesh_inst->Deactivate(1);
			}
		}
		else
		{
			// chowanie broni
			if(animation_state == AS_TAKE_WEAPON_START && (mesh_inst->GetProgress(1) <= data->frames->t[F_TAKE_WEAPON] || mesh_inst->IsEnded(1)))
				animation_state = AS_TAKE_WEAPON_MOVED;
			if(weapon_taken != W_NONE && (animation_state == AS_TAKE_WEAPON_MOVED || mesh_inst->IsEnded(1)))
			{
				mesh_inst->Play(GetTakeWeaponAnimation(weapon_taken == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				weapon_state = WeaponState::Taking;
				weapon_hiding = W_NONE;
				animation_state = AS_TAKE_WEAPON_START;
				if(IsPlayer())
					player->last_weapon = weapon_taken;

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.unit = this;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(mesh_inst->IsEnded(1))
			{
				weapon_state = WeaponState::Hidden;
				weapon_hiding = W_NONE;
				action = A_NONE;
				mesh_inst->Deactivate(1);

				if(IsLocalPlayer())
				{
					switch(player->next_action)
					{
						// unequip item
					case NA_REMOVE:
						if(slots[player->next_action_data.slot])
							game_gui->inventory->inv_mine->RemoveSlotItem(player->next_action_data.slot);
						break;
						// equip item after unequiping old one
					case NA_EQUIP:
					case NA_EQUIP_DRAW:
						{
							int index = player->GetNextActionItemIndex();
							if(index != -1)
							{
								game_gui->inventory->inv_mine->EquipSlotItem(index);
								if(player->next_action == NA_EQUIP_DRAW)
								{
									switch(player->next_action_data.item->type)
									{
									case IT_WEAPON:
										player->unit->TakeWeapon(W_ONE_HANDED);
										break;
									case IT_SHIELD:
										if(player->unit->HaveWeapon())
											player->unit->TakeWeapon(W_ONE_HANDED);
										break;
									case IT_BOW:
										player->unit->TakeWeapon(W_BOW);
										break;
									}
								}
							}
						}
						break;
						// drop item after hiding it
					case NA_DROP:
						if(slots[player->next_action_data.slot])
							game_gui->inventory->inv_mine->DropSlotItem(player->next_action_data.slot);
						break;
						// use consumable
					case NA_CONSUME:
						{
							int index = player->GetNextActionItemIndex();
							if(index != -1)
								game_gui->inventory->inv_mine->ConsumeItem(index);
						}
						break;
						//  use usable
					case NA_USE:
						if(!player->next_action_data.usable->user)
							player->UseUsable(player->next_action_data.usable, true);
						break;
						// sell equipped item
					case NA_SELL:
						if(slots[player->next_action_data.slot])
							game_gui->inventory->inv_trade_mine->SellSlotItem(player->next_action_data.slot);
						break;
						// put equipped item in container
					case NA_PUT:
						if(slots[player->next_action_data.slot])
							game_gui->inventory->inv_trade_mine->PutSlotItem(player->next_action_data.slot);
						break;
						// give equipped item
					case NA_GIVE:
						if(slots[player->next_action_data.slot])
							game_gui->inventory->inv_trade_mine->GiveSlotItem(player->next_action_data.slot);
						break;
					}

					player->ClearNextAction();

					if(action == A_NONE && usable && !usable->container)
					{
						action = A_USE_USABLE;
						animation_state = AS_USE_USABLE_USING;
					}
				}
				else if(Net::IsLocal() && !fake_unit && IsAI() && ai->potion != -1)
				{
					ConsumeItem(ai->potion);
					ai->potion = -1;
				}
			}
		}
		break;
	case A_SHOOT:
		stamina_timer = STAMINA_RESTORE_TIMER;
		if(!mesh_inst)
		{
			// fix na skutek, nie na przyczynê ;(
			game->ReportError(4, Format("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", GetName(), live_state, action, animation,
				current_animation, animation_state));
			goto koniec_strzelania;
		}
		if(fake_unit)
		{
			if(mesh_inst->IsEnded(1))
			{
				mesh_inst->Deactivate(1);
				action = A_NONE;
				game_level->FreeBowInstance(bow_instance);
				break;
			}
		}
		else if(animation_state == AS_SHOOT_PREPARE)
		{
			if(mesh_inst->GetProgress(1) > 20.f / 40)
				mesh_inst->groups[1].time = 20.f / 40 * mesh_inst->groups[1].anim->length;
		}
		else if(animation_state == AS_SHOOT_CAN)
		{
			if(Net::IsLocal() && mesh_inst->GetProgress(1) > 20.f / 40)
			{
				Bullet* bullet = new Bullet;
				area->tmp->bullets.push_back(bullet);

				mesh_inst->SetupBones();

				Mesh::Point* point = mesh_inst->mesh->GetPoint(NAMES::point_weapon);
				assert(point);

				Matrix m2 = point->mat * mesh_inst->mat_bones[point->bone] * (Matrix::RotationY(rot) * Matrix::Translation(pos));

				bullet->Register();
				bullet->attack = CalculateAttack(&GetBow());
				bullet->level = level;
				bullet->backstab = GetBackstabMod(&GetBow());
				bullet->rot = Vec3(PI / 2, rot + PI, 0);
				bullet->pos = Vec3::TransformZero(m2);
				bullet->mesh = game_res->aArrow;
				bullet->speed = GetArrowSpeed();
				bullet->timer = ARROW_TIMER;
				bullet->owner = this;
				bullet->pe = nullptr;
				bullet->ability = nullptr;
				bullet->tex = nullptr;
				bullet->poison_attack = 0.f;
				bullet->start_pos = bullet->pos;

				float dist = Vec3::Distance2d(bullet->pos, target_pos);
				float t = dist / bullet->speed;
				float h = target_pos.y - bullet->pos.y;
				bullet->yspeed = h / t;

				if(IsPlayer())
					player->Train(TrainWhat::BowStart, 0.f, 0);
				else if(ai->state == AIController::Idle && ai->st.idle.action == AIController::Idle_TrainBow)
					bullet->attack = -100.f;

				// random variation
				int sk = Get(SkillId::BOW);
				if(IsPlayer())
					sk += 10;
				else
					sk -= 10;
				if(sk < 50)
				{
					int chance;
					float variation_x, variation_y;
					if(sk < 10)
					{
						chance = 100;
						variation_x = PI / 16;
						variation_y = 5.f;
					}
					else if(sk < 20)
					{
						chance = 80;
						variation_x = PI / 20;
						variation_y = 4.f;
					}
					else if(sk < 30)
					{
						chance = 60;
						variation_x = PI / 26;
						variation_y = 3.f;
					}
					else if(sk < 40)
					{
						chance = 40;
						variation_x = PI / 34;
						variation_y = 2.f;
					}
					else
					{
						chance = 20;
						variation_x = PI / 48;
						variation_y = 1.f;
					}

					if(Rand() % 100 < chance)
						bullet->rot.y += RandomNormalized(variation_x);
					if(Rand() % 100 < chance)
						bullet->yspeed += RandomNormalized(variation_y);
				}

				bullet->rot.y = Clip(bullet->rot.y);

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				tpe->color1 = Vec4(1, 1, 1, 0.5f);
				tpe->color2 = Vec4(1, 1, 1, 0);
				tpe->Init(50);
				area->tmp->tpes.push_back(tpe);
				bullet->trail = tpe;

				sound_mgr->PlaySound3d(game_res->sBow[Rand() % 2], bullet->pos, SHOOT_SOUND_DIST);

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::SHOOT_ARROW;
					c << bullet->id
						<< id
						<< bullet->start_pos
						<< bullet->rot.x
						<< bullet->rot.y
						<< bullet->speed
						<< bullet->yspeed;
				}
			}
			if(mesh_inst->GetProgress(1) > 20.f / 40)
				animation_state = AS_SHOOT_SHOT;
		}
		else if(mesh_inst->GetProgress(1) > 35.f / 40)
		{
			animation_state = AS_SHOOT_FINISHED;
			if(mesh_inst->IsEnded(1))
			{
			koniec_strzelania:
				mesh_inst->Deactivate(1);
				action = A_NONE;
				game_level->FreeBowInstance(bow_instance);
				if(Net::IsLocal() && IsAI())
				{
					float v = 1.f - float(Get(SkillId::BOW)) / 100;
					ai->next_attack = Random(v / 2, v);
				}
				break;
			}
		}
		bow_instance->groups[0].time = min(mesh_inst->groups[1].time, bow_instance->groups[0].anim->length);
		bow_instance->need_update = true;
		break;
	case A_ATTACK:
		stamina_timer = STAMINA_RESTORE_TIMER;
		if(fake_unit)
		{
			if(mesh_inst->IsEnded(1))
			{
				animation = ANI_BATTLE;
				current_animation = ANI_STAND;
				action = A_NONE;
			}
		}
		else if(animation_state == AS_ATTACK_PREPARE)
		{
			float t = GetAttackFrame(0);
			float p = mesh_inst->GetProgress(group_index);
			if(p > t)
			{
				if(Net::IsLocal() && IsAI())
				{
					float speed = (1.f + GetAttackSpeed()) * GetStaminaAttackSpeedMod();
					mesh_inst->groups[group_index].speed = speed;
					act.attack.power = 2.f;
					animation_state = AS_ATTACK_CAN_HIT;
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = this;
						c.id = AID_Attack;
						c.f[1] = speed;
					}
				}
				else
					mesh_inst->groups[group_index].time = t * mesh_inst->groups[group_index].anim->length;
			}
			else if(IsPlayer() && Net::IsLocal())
			{
				float dif = p - timer;
				float stamina_to_remove_total = GetWeapon().GetInfo().stamina / 2;
				float stamina_used = dif / t * stamina_to_remove_total;
				timer = p;
				RemoveStamina(stamina_used);
			}
		}
		else
		{
			if(animation_state == AS_ATTACK_CAN_HIT && mesh_inst->GetProgress(group_index) > GetAttackFrame(0))
			{
				if(Net::IsLocal() && !act.attack.hitted && mesh_inst->GetProgress(group_index) >= GetAttackFrame(1))
				{
					if(DoAttack())
						act.attack.hitted = true;
				}
				if(mesh_inst->GetProgress(group_index) >= GetAttackFrame(2) || mesh_inst->IsEnded(group_index))
				{
					// koniec mo¿liwego ataku
					animation_state = AS_ATTACK_FINISHED;
					mesh_inst->groups[group_index].speed = 1.f;
					act.attack.run = false;
				}
			}
			if(animation_state == AS_ATTACK_FINISHED && mesh_inst->IsEnded(group_index))
			{
				if(group_index == 0)
				{
					animation = ANI_BATTLE;
					current_animation = ANI_STAND;
				}
				else
					mesh_inst->Deactivate(1);
				action = A_NONE;
				if(Net::IsLocal() && IsAI())
				{
					float v = 1.f - float(Get(SkillId::ONE_HANDED_WEAPON)) / 100;
					ai->next_attack = Random(v / 2, v);
				}
			}
		}
		break;
	case A_BLOCK:
		break;
	case A_BASH:
		stamina_timer = STAMINA_RESTORE_TIMER;
		if(animation_state == AS_BASH_ANIMATION)
		{
			if(mesh_inst->GetProgress(1) >= data->frames->t[F_BASH])
				animation_state = AS_BASH_CAN_HIT;
		}
		if(Net::IsLocal() && animation_state == AS_BASH_CAN_HIT)
		{
			if(DoShieldSmash())
				animation_state = AS_BASH_HITTED;
		}
		if(mesh_inst->IsEnded(1))
		{
			action = A_NONE;
			mesh_inst->Deactivate(1);
		}
		break;
	case A_DRINK:
		{
			float p = mesh_inst->GetProgress(1);
			if(p >= 28.f / 52.f && animation_state == AS_DRINK_START)
			{
				PlaySound(game_res->sGulp, DRINK_SOUND_DIST);
				animation_state = AS_DRINK_EFFECT;
				if(Net::IsLocal())
					ApplyConsumableEffect(used_item->ToConsumable());
			}
			if(p >= 49.f / 52.f && animation_state == AS_DRINK_EFFECT)
			{
				animation_state = AS_DRINK_EFFECT;
				used_item = nullptr;
			}
			if(mesh_inst->IsEnded(1))
			{
				if(usable)
				{
					animation_state = AS_USE_USABLE_USING;
					action = A_USE_USABLE;
				}
				else
					action = A_NONE;
				mesh_inst->Deactivate(1);
			}
		}
		break;
	case A_EAT:
		{
			float p = mesh_inst->GetProgress(1);
			if(p >= 32.f / 70 && animation_state == AS_EAT_START)
			{
				animation_state = AS_EAT_SOUND;
				PlaySound(game_res->sEat, EAT_SOUND_DIST);
			}
			if(p >= 48.f / 70 && animation_state == AS_EAT_SOUND)
			{
				animation_state = AS_EAT_EFFECT;
				if(Net::IsLocal())
					ApplyConsumableEffect(used_item->ToConsumable());
			}
			if(p >= 60.f / 70 && animation_state == AS_EAT_EFFECT)
			{
				animation_state = AS_EAT_END;
				used_item = nullptr;
			}
			if(mesh_inst->IsEnded(1))
			{
				if(usable)
				{
					animation_state = AS_USE_USABLE_USING;
					action = A_USE_USABLE;
				}
				else
					action = A_NONE;
				mesh_inst->Deactivate(1);
			}
		}
		break;
	case A_PAIN:
		if(mesh_inst->mesh->head.n_groups == 2)
		{
			if(mesh_inst->IsEnded(1))
			{
				action = A_NONE;
				mesh_inst->Deactivate(1);
			}
		}
		else if(mesh_inst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_BATTLE;
		}
		break;
	case A_CAST:
		if(Net::IsLocal())
		{
			if(IsOtherPlayer()
				? animation_state == AS_CAST_TRIGGER
				: (animation_state == AS_CAST_ANIMATION && mesh_inst->GetProgress(group_index) >= data->frames->t[F_CAST]))
			{
				animation_state = AS_CAST_CASTED;
				CastSpell();
			}
		}
		else if(IsLocalPlayer() && animation_state == AS_CAST_ANIMATION && mesh_inst->GetProgress(group_index) >= data->frames->t[F_CAST])
		{
			animation_state = AS_CAST_CASTED;
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CAST_SPELL;
			c.pos = target_pos;
		}
		if(mesh_inst->IsEnded(group_index))
		{
			if(group_index == 1)
			{
				action = A_NONE;
				mesh_inst->Deactivate(1);
			}
			else
			{
				action = A_NONE;
				animation = ANI_BATTLE;
			}
			if(Net::IsLocal() && IsAI())
				ai->next_attack = Random(0.25f, 0.75f);
		}
		break;
	case A_ANIMATION:
		if(mesh_inst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_STAND;
			current_animation = (Animation)-1;
		}
		break;
	case A_USE_USABLE:
		{
			bool allow_move = true;
			if(Net::IsServer())
			{
				if(IsOtherPlayer())
					allow_move = false;
			}
			else if(Net::IsClient())
			{
				if(!IsPlayer() || !player->is_local)
					allow_move = false;
			}
			if(animation_state == AS_USE_USABLE_MOVE_TO_ENDPOINT)
			{
				timer += dt;
				if(allow_move && timer >= 0.5f)
				{
					action = A_NONE;
					visual_pos = pos = target_pos;
					changed = true;
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::USE_USABLE;
						c.unit = this;
						c.id = usable->id;
						c.count = USE_USABLE_END;
					}
					if(Net::IsLocal())
						UseUsable(nullptr);
					break;
				}

				if(allow_move)
				{
					// przesuñ postaæ
					visual_pos = pos = Vec3::Lerp(target_pos2, target_pos, timer * 2);

					// obrót
					float target_rot = Vec3::LookAtAngle(target_pos, usable->pos);
					float dif = AngleDiff(rot, target_rot);
					if(NotZero(dif))
					{
						const float rot_speed = GetRotationSpeed() * 2 * dt;
						const float arc = ShortestArc(rot, target_rot);

						if(dif <= rot_speed)
							rot = target_rot;
						else
							rot = Clip(rot + Sign(arc) * rot_speed);
					}

					changed = true;
				}
			}
			else
			{
				BaseUsable& bu = *usable->base;

				if(animation_state > AS_USE_USABLE_MOVE_TO_OBJECT)
				{
					// odtwarzanie dŸwiêku
					if(bu.sound)
					{
						if(mesh_inst->GetProgress() >= bu.sound_timer)
						{
							if(animation_state == AS_USE_USABLE_USING)
							{
								animation_state = AS_USE_USABLE_USING_SOUND;
								sound_mgr->PlaySound3d(bu.sound, GetCenter(), Usable::SOUND_DIST);
								if(Net::IsServer())
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::USABLE_SOUND;
									c.unit = this;
								}
							}
						}
						else if(animation_state == AS_USE_USABLE_USING_SOUND)
							animation_state = AS_USE_USABLE_USING;
					}
				}
				else if(Net::IsLocal() || IsLocalPlayer())
				{
					// ustal docelowy obrót postaci
					float target_rot;
					if(bu.limit_rot == 0)
						target_rot = rot;
					else if(bu.limit_rot == 1)
					{
						float rot1 = Clip(act.use_usable.rot + PI / 2),
							dif1 = AngleDiff(rot1, usable->rot),
							rot2 = Clip(usable->rot + PI),
							dif2 = AngleDiff(rot1, rot2);

						if(dif1 < dif2)
							target_rot = usable->rot;
						else
							target_rot = rot2;
					}
					else if(bu.limit_rot == 2)
						target_rot = usable->rot;
					else if(bu.limit_rot == 3)
					{
						float rot1 = Clip(act.use_usable.rot + PI),
							dif1 = AngleDiff(rot1, usable->rot),
							rot2 = Clip(usable->rot + PI),
							dif2 = AngleDiff(rot1, rot2);

						if(dif1 < dif2)
							target_rot = usable->rot;
						else
							target_rot = rot2;
					}
					else
						target_rot = usable->rot + PI;
					target_rot = Clip(target_rot);

					// obrót w strone obiektu
					const float dif = AngleDiff(rot, target_rot);
					const float rot_speed = GetRotationSpeed() * 2;
					if(allow_move && NotZero(dif))
					{
						const float rot_speed_dt = rot_speed * dt;
						if(dif <= rot_speed_dt)
							rot = target_rot;
						else
						{
							const float arc = ShortestArc(rot, target_rot);
							rot = Clip(rot + Sign(arc) * rot_speed_dt);
						}
					}

					// czy musi siê obracaæ zanim zacznie siê przesuwaæ?
					if(dif < rot_speed * 0.5f)
					{
						timer += dt;
						if(timer >= 0.5f)
						{
							timer = 0.5f;
							animation_state = AS_USE_USABLE_USING;
							if(IsLocalPlayer() && IsSet(usable->base->use_flags, BaseUsable::ALCHEMY))
								game_gui->craft->Show();
						}

						// przesuñ postaæ i fizykê
						if(allow_move)
						{
							visual_pos = pos = Vec3::Lerp(target_pos, target_pos2, timer * 2);
							changed = true;
							game_level->global_col.clear();
							float my_radius = GetUnitRadius();
							bool ok = true;
							for(vector<Unit*>::iterator it2 = area->units.begin(), end2 = area->units.end(); it2 != end2; ++it2)
							{
								if(this == *it2 || !(*it2)->IsStanding())
									continue;

								float radius = (*it2)->GetUnitRadius();
								if(Distance((*it2)->pos.x, (*it2)->pos.z, pos.x, pos.z) <= radius + my_radius)
								{
									ok = false;
									break;
								}
							}
							if(ok)
								UpdatePhysics();
						}
					}
				}
			}
		}
		break;
	case A_POSITION:
		if(Net::IsClient())
		{
			if(!IsLocalPlayer())
				break;
		}
		else
		{
			if(IsOtherPlayer())
				break;
		}
		timer += dt;
		if(animation_state == AS_POSITION_HURT)
		{
			// obs³uga animacji cierpienia
			if(mesh_inst->mesh->head.n_groups == 2)
			{
				if(mesh_inst->IsEnded(1) || timer >= 0.5f)
				{
					mesh_inst->Deactivate(1);
					animation_state = AS_POSITION_HURT_END;
				}
			}
			else if(mesh_inst->IsEnded() || timer >= 0.5f)
			{
				animation = ANI_BATTLE;
				animation_state = AS_POSITION_HURT_END;
			}
		}
		if(timer >= 0.5f)
		{
			action = A_NONE;
			visual_pos = pos = target_pos;

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = this;
				c.id = usable->id;
				c.count = USE_USABLE_END;
			}

			if(Net::IsLocal())
				UseUsable(nullptr);
		}
		else
			visual_pos = pos = Vec3::Lerp(target_pos2, target_pos, timer * 2);
		changed = true;
		break;
	case A_PICKUP:
		if(mesh_inst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_STAND;
			current_animation = (Animation)-1;
		}
		break;
	case A_DASH:
		// update unit dash/bull charge
		{
			assert(player); // todo
			float dt_left = min(dt, timer);
			float t;
			const float eps = 0.05f;
			float len = speed * dt_left;
			Vec3 dir(sin(act.dash.rot) * (len + eps), 0, cos(act.dash.rot) * (len + eps));
			Vec3 dir_normal = dir.Normalized();
			bool ok = true;
			Vec3 from = GetPhysicsPos();

			if(act.dash.ability->effect != Ability::Stun)
			{
				// dash
				game_level->LineTest(cobj->getCollisionShape(), from, dir, [&](btCollisionObject* obj, bool)
				{
					int flags = obj->getCollisionFlags();
					if(IsSet(flags, CG_TERRAIN))
						return LT_IGNORE;
					if(IsSet(flags, CG_UNIT))
					{
						Unit* unit = reinterpret_cast<Unit*>(obj->getUserPointer());
						if(unit == this || !unit->IsStanding())
							return LT_IGNORE;
					}
					return LT_COLLIDE;
				}, t);
			}
			else
			{
				// bull charge, do line test and find targets
				static vector<Unit*> targets;
				targets.clear();
				game_level->LineTest(cobj->getCollisionShape(), from, dir, [&](btCollisionObject* obj, bool first)
				{
					int flags = obj->getCollisionFlags();
					if(first)
					{
						if(IsSet(flags, CG_TERRAIN))
							return LT_IGNORE;
						if(IsSet(flags, CG_UNIT))
						{
							Unit* unit = reinterpret_cast<Unit*>(obj->getUserPointer());
							if(unit == this || !unit->IsAlive())
								return LT_IGNORE;
						}
					}
					else
					{
						if(IsSet(obj->getCollisionFlags(), CG_UNIT))
						{
							Unit* unit = reinterpret_cast<Unit*>(obj->getUserPointer());
							targets.push_back(unit);
							return LT_IGNORE;
						}
					}
					return LT_COLLIDE;
				}, t, nullptr, true);

				// check all hitted
				if(Net::IsLocal())
				{
					float move_angle = Angle(0, 0, dir.x, dir.z);
					Vec3 dir_left(sin(act.dash.rot + PI / 2) * len, 0, cos(act.dash.rot + PI / 2) * len);
					Vec3 dir_right(sin(act.dash.rot - PI / 2) * len, 0, cos(act.dash.rot - PI / 2) * len);
					for(Unit* unit : targets)
					{
						// deal damage/stun
						bool move_forward = true;
						if(!unit->IsFriend(*this, true))
						{
							if(!player->IsHit(unit))
							{
								float attack = unit->GetAbilityPower(*act.dash.ability);
								float def = unit->CalculateDefense();
								float dmg = CombatHelper::CalculateDamage(attack, def);
								game->PlayHitSound(MAT_IRON, unit->GetBodyMaterial(), unit->GetCenter(), HIT_SOUND_DIST, true);
								if(unit->IsPlayer())
									unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, level);
								unit->GiveDmg(dmg, this);
								if(IsPlayer())
									player->Train(TrainWhat::BullsCharge, 0.f, unit->level);
								if(!unit->IsAlive())
									continue;
								else
									player->ability_targets.push_back(unit);
							}
							unit->ApplyStun(1.5f);
						}
						else
							move_forward = false;

						auto unit_clbk = [unit](btCollisionObject* obj, bool)
						{
							int flags = obj->getCollisionFlags();
							if(IsSet(flags, CG_TERRAIN | CG_UNIT))
								return LT_IGNORE;
							return LT_COLLIDE;
						};

						// try to push forward
						if(move_forward)
						{
							Vec3 move_dir = unit->pos - pos;
							move_dir.y = 0;
							move_dir.Normalize();
							move_dir += dir_normal;
							move_dir.Normalize();
							move_dir *= len;
							float t;
							game_level->LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), move_dir, unit_clbk, t);
							if(t == 1.f)
							{
								unit->moved = true;
								unit->pos += move_dir;
								unit->Moved(false, true);
								continue;
							}
						}

						// try to push, left or right
						float angle = Angle(pos.x, pos.z, unit->pos.x, unit->pos.z);
						float best_dir = ShortestArc(move_angle, angle);
						bool inner_ok = false;
						for(int i = 0; i < 2; ++i)
						{
							float t;
							Vec3& actual_dir = (best_dir < 0 ? dir_left : dir_right);
							game_level->LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), actual_dir, unit_clbk, t);
							if(t == 1.f)
							{
								inner_ok = true;
								unit->moved = true;
								unit->pos += actual_dir;
								unit->Moved(false, true);
								break;
							}
						}
						if(!inner_ok)
							ok = false;
					}
				}
			}

			// move if there is free space
			float actual_len = (len + eps) * t - eps;
			if(actual_len > 0 && ok)
			{
				pos += Vec3(sin(act.dash.rot), 0, cos(act.dash.rot)) * actual_len;
				Moved(false, true);
			}

			// end dash
			timer -= dt;
			if(timer <= 0 || t < 1.f || !ok)
			{
				if(act.dash.ability->effect == Ability::Stun)
					mesh_inst->Deactivate(1);
				action = A_NONE;
				if(Net::IsLocal() || IsLocalPlayer())
					mesh_inst->groups[0].speed = GetRunSpeed() / data->run_speed;
			}
		}
		break;
	case A_DESPAWN:
		timer -= dt;
		if(timer <= 0.f)
			game_level->RemoveUnit(this);
		break;
	case A_PREPARE:
		assert(Net::IsClient());
		break;
	case A_STAND_UP:
		if(mesh_inst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_STAND;
			current_animation = (Animation)-1;
		}
		break;
	case A_USE_ITEM:
		if(mesh_inst->IsEnded(1))
		{
			if(Net::IsLocal() && IsPlayer())
			{
				// magic scroll effect
				game_gui->messages->AddGameMsg3(player, GMS_TOO_COMPLICATED);
				ApplyStun(2.5f);
				RemoveItem(used_item, 1u);
				used_item = nullptr;
			}
			sound_mgr->PlaySound3d(game_res->sZap, GetCenter(), MAGIC_SCROLL_SOUND_DIST);
			action = A_NONE;
			animation = ANI_STAND;
			current_animation = (Animation)-1;
		}
		break;
	default:
		assert(0);
		break;
	}

	UpdateStaminaAction();
}

//=============================================================================
void Unit::Moved(bool warped, bool dash)
{
	if(game_level->location->outside)
	{
		if(area->area_type == LevelArea::Type::Outside)
		{
			if(game_level->terrain->IsInside(pos))
			{
				game_level->terrain->SetY(pos);
				if(warped)
					return;
				if(IsPlayer() && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
				{
					bool allow_exit = false;

					if(game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveExit))
					{
						for(vector<EntryPoint>::const_iterator it = game_level->city_ctx->entry_points.begin(), end = game_level->city_ctx->entry_points.end(); it != end; ++it)
						{
							if(it->exit_region.IsInside(pos))
							{
								if(!team->IsLeader())
									game_gui->messages->AddGameMsg3(GMS_NOT_LEADER);
								else
								{
									if(Net::IsLocal())
									{
										CanLeaveLocationResult result = game_level->CanLeaveLocation(*this);
										if(result == CanLeaveLocationResult::Yes)
										{
											allow_exit = true;
											world->SetTravelDir(pos);
										}
										else
											game_gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										net->OnLeaveLocation(ENTER_FROM_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(game_level->location_index != quest_mgr->quest_secret->where2
						&& (pos.x < 33.f || pos.x > 256.f - 33.f || pos.z < 33.f || pos.z > 256.f - 33.f))
					{
						if(!team->IsLeader())
							game_gui->messages->AddGameMsg3(GMS_NOT_LEADER);
						else if(!Net::IsLocal())
							net->OnLeaveLocation(ENTER_FROM_OUTSIDE);
						else
						{
							CanLeaveLocationResult result = game_level->CanLeaveLocation(*this);
							if(result == CanLeaveLocationResult::Yes)
							{
								allow_exit = true;
								world->SetTravelDir(pos);
							}
							else
								game_gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
					}

					if(allow_exit)
					{
						game->fallback_type = FALLBACK::EXIT;
						game->fallback_t = -1.f;
						for(Unit& unit : team->members)
							unit.frozen = FROZEN::YES;
						if(Net::IsOnline())
							Net::PushChange(NetChange::LEAVE_LOCATION);
					}
				}
			}
			else
				pos.y = 0.f;

			if(warped)
				return;

			if(IsPlayer() && game_level->location->type == L_CITY && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
			{
				int id = 0;
				for(vector<InsideBuilding*>::iterator it = game_level->city_ctx->inside_buildings.begin(), end = game_level->city_ctx->inside_buildings.end(); it != end; ++it, ++id)
				{
					if((*it)->enter_region.IsInside(pos))
					{
						if(Net::IsLocal())
						{
							// wejdŸ do budynku
							game->fallback_type = FALLBACK::ENTER;
							game->fallback_t = -1.f;
							game->fallback_1 = id;
							game->fallback_2 = -1;
							frozen = FROZEN::YES;
						}
						else
						{
							// info do serwera
							game->fallback_type = FALLBACK::WAIT_FOR_WARP;
							game->fallback_t = -1.f;
							frozen = FROZEN::YES;
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ENTER_BUILDING;
							c.id = id;
						}
						break;
					}
				}
			}
		}
		else
		{
			if(warped)
				return;

			// jest w budynku
			// sprawdŸ czy nie wszed³ na wyjœcie (tylko gracz mo¿e opuszczaæ budynek, na razie)
			InsideBuilding& building = *static_cast<InsideBuilding*>(area);

			if(IsPlayer() && building.exit_region.IsInside(pos) && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
			{
				if(Net::IsLocal())
				{
					// opuœæ budynek
					game->fallback_type = FALLBACK::ENTER;
					game->fallback_t = -1.f;
					game->fallback_1 = -1;
					game->fallback_2 = -1;
					frozen = FROZEN::YES;
				}
				else
				{
					// info do serwera
					game->fallback_type = FALLBACK::WAIT_FOR_WARP;
					game->fallback_t = -1.f;
					frozen = FROZEN::YES;
					Net::PushChange(NetChange::EXIT_BUILDING);
				}
			}
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)game_level->location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		Int2 pt = PosToPt(pos);

		if(pt == lvl.prevEntryPt)
			MovedToEntry(lvl.prevEntryType, pt, lvl.prevEntryDir, !warped && !dash, true);
		else if(pt == lvl.nextEntryPt)
			MovedToEntry(lvl.nextEntryType, pt, lvl.nextEntryDir, !warped && !dash, false);

		if(warped)
			return;
	}

	// obs³uga portali
	if(frozen == FROZEN::NO && IsPlayer() && player->WantExitLevel() && !dash)
	{
		Portal* portal = game_level->location->portal;
		int index = 0;
		while(portal)
		{
			if(portal->target_loc != -1 && Vec3::Distance2d(pos, portal->pos) < 2.f)
			{
				if(CircleToRotatedRectangle(pos.x, pos.z, GetUnitRadius(), portal->pos.x, portal->pos.z, 0.67f, 0.1f, portal->rot))
				{
					if(team->IsLeader())
					{
						if(Net::IsLocal())
						{
							CanLeaveLocationResult result = game_level->CanLeaveLocation(*this);
							if(result == CanLeaveLocationResult::Yes)
							{
								game->fallback_type = FALLBACK::USE_PORTAL;
								game->fallback_t = -1.f;
								game->fallback_1 = index;
								for(Unit& unit : team->members)
									unit.frozen = FROZEN::YES;
								if(Net::IsOnline())
									Net::PushChange(NetChange::LEAVE_LOCATION);
							}
							else
								game_gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							net->OnLeaveLocation(ENTER_FROM_PORTAL + index);
					}
					else
						game_gui->messages->AddGameMsg3(GMS_NOT_LEADER);

					break;
				}
			}
			portal = portal->next_portal;
			++index;
		}
	}

	if(Net::IsLocal() || !IsLocalPlayer() || net->interpolate_timer <= 0.f)
	{
		visual_pos = pos;
		changed = true;
	}
	UpdatePhysics();
}

//=================================================================================================
void Unit::MovedToEntry(EntryType type, const Int2& pt, GameDirection dir, bool canWarp, bool isPrev)
{
	Box2d box;
	switch(type)
	{
	case ENTRY_STAIRS_UP:
		switch(dir)
		{
		case GDIR_DOWN:
			pos.y = (pos.z - 2.f * pt.y) / 2;
			box = Box2d(2.f * pt.x, 2.f * pt.y + 1.4f, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_LEFT:
			pos.y = (pos.x - 2.f * pt.x) / 2;
			box = Box2d(2.f * pt.x + 1.4f, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_UP:
			pos.y = (2.f * pt.y - pos.z) / 2 + 1.f;
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * pt.y + 0.6f);
			break;
		case GDIR_RIGHT:
			pos.y = (2.f * pt.x - pos.x) / 2 + 1.f;
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * pt.x + 0.6f, 2.f * (pt.y + 1));
			break;
		}
		break;
	case ENTRY_STAIRS_DOWN:
	case ENTRY_STAIRS_DOWN_IN_WALL:
		switch(dir)
		{
		case GDIR_DOWN:
			pos.y = (pos.z - 2.f * pt.y) * -1.f;
			box = Box2d(2.f * pt.x, 2.f * pt.y + 1.4f, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_LEFT:
			pos.y = (pos.x - 2.f * pt.x) * -1.f;
			box = Box2d(2.f * pt.x + 1.4f, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_UP:
			pos.y = (2.f * pt.y - pos.z) * -1.f - 2.f;
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * pt.y + 0.6f);
			break;
		case GDIR_RIGHT:
			pos.y = (2.f * pt.x - pos.x) * -1.f - 2.f;
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * pt.x + 0.6f, 2.f * (pt.y + 1));
			break;
		}
		break;
	case ENTRY_DOOR:
		switch(dir)
		{
		case GDIR_DOWN:
			box = Box2d(2.f * pt.x, 2.f * pt.y + 1.4f, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_LEFT:
			box = Box2d(2.f * pt.x + 1.4f, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * (pt.y + 1));
			break;
		case GDIR_UP:
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * (pt.x + 1), 2.f * pt.y + 0.6f);
			break;
		case GDIR_RIGHT:
			box = Box2d(2.f * pt.x, 2.f * pt.y, 2.f * pt.x + 0.6f, 2.f * (pt.y + 1));
			break;
		}
		break;
	}

	if(canWarp && IsPlayer() && player->WantExitLevel() && box.IsInside(pos) && frozen == FROZEN::NO)
	{
		if(team->IsLeader())
		{
			if(Net::IsLocal())
			{
				CanLeaveLocationResult result = game_level->CanLeaveLocation(*this);
				if(result == CanLeaveLocationResult::Yes)
				{
					game->fallback_type = FALLBACK::CHANGE_LEVEL;
					game->fallback_t = -1.f;
					game->fallback_1 = isPrev ? -1 : +1;
					for(Unit& unit : team->members)
						unit.frozen = FROZEN::YES;
					if(Net::IsOnline())
						Net::PushChange(NetChange::LEAVE_LOCATION);
				}
				else
					game_gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
			}
			else
				net->OnLeaveLocation(isPrev ? ENTER_FROM_PREV_LEVEL : ENTER_FROM_NEXT_LEVEL);
		}
		else
			game_gui->messages->AddGameMsg3(GMS_NOT_LEADER);
	}
}

//=================================================================================================
void Unit::ChangeBase(UnitData* ud, bool update_items)
{
	assert(ud);

	if(data == ud)
		return;

	data = ud;
	game->PreloadUnit(this);
	level = ud->level.Clamp(level);
	stats = data->GetStats(level);
	CalculateStats();

	if(update_items)
	{
		ud->item_script->Parse(*this);
		for(const Item* item : slots)
		{
			if(item)
				game_res->PreloadItem(item);
		}
		for(ItemSlot& slot : items)
			game_res->PreloadItem(slot.item);
		if(IsTeamMember())
			MakeItemsTeam(false);
		UpdateInventory();
	}

	if(IsHero() && IsSet(ud->flags2, F2_MELEE))
		hero->melee = true;

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_UNIT_BASE;
		c.unit = this;
	}
}

//=================================================================================================
void Unit::MoveToArea(LevelArea* area, const Vec3& pos)
{
	assert(area && !IsTeamMember());
	if(area == this->area)
		return;
	assert(game_level->entering); // TODO
	const bool is_active = mesh_inst != nullptr;
	const bool activate = area->IsActive();
	RemoveElement(this->area->units, this);
	area->units.push_back(this);
	this->area = area;
	this->pos = pos;
	visual_pos = pos;

	if(is_active != activate)
	{
		if(activate)
		{
			CreateMesh(CREATE_MESH::NORMAL);
			CreatePhysics();

			ai = new AIController;
			ai->Init(this);
			game->ais.push_back(ai);
		}
		else
		{
			BreakAction(BREAK_ACTION_MODE::ON_LEAVE);

			// physics
			if(cobj)
			{
				delete cobj->getCollisionShape();
				cobj = nullptr;
			}

			// speech bubble
			bubble = nullptr;
			talking = false;

			// mesh
			delete mesh_inst;
			mesh_inst = nullptr;
			delete ai;
			ai = nullptr;
			EndEffects();
		}
	}
}

//=================================================================================================
void Unit::Kill()
{
	assert(game_level->entering); // TODO
	live_state = DEAD;
	if(data->mesh->IsLoaded())
	{
		animation = current_animation = ANI_DIE;
		SetAnimationAtEnd(NAMES::ani_die);
		game_level->CreateBlood(*game_level->lvl, *this, true);
	}
	else
		game_level->blood_to_spawn.push_back(this);
	hp = 0.f;
	++game_stats->total_kills;
	UpdatePhysics();
	if(event_handler)
		event_handler->HandleUnitEvent(UnitEventHandler::DIE, this);
}

//=================================================================================================
void Unit::GiveDmg(float dmg, Unit* giver, const Vec3* hitpoint, int dmg_flags)
{
	// blood particles
	if(!IsSet(dmg_flags, DMG_NO_BLOOD))
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = game_res->tBlood[data->blood];
		pe->emision_interval = 0.01f;
		pe->life = 5.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 10;
		pe->spawn_max = 15;
		pe->max_particles = 15;
		if(hitpoint)
			pe->pos = *hitpoint;
		else
		{
			pe->pos = pos;
			pe->pos.y += GetUnitHeight() * 2.f / 3;
		}
		pe->speed_min = Vec3(-1, 0, -1);
		pe->speed_max = Vec3(1, 1, 1);
		pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
		pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
		pe->size = 0.3f;
		pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->mode = 0;
		pe->Init();
		area->tmp->pes.push_back(pe);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPAWN_BLOOD;
			c.id = data->blood;
			c.pos = pe->pos;
		}
	}

	// apply magic resistance
	if(IsSet(dmg_flags, DMG_MAGICAL))
		dmg *= CalculateMagicResistance();

	if(giver && giver->IsPlayer())
	{
		// update player damage done
		giver->player->dmg_done += (int)dmg;
		giver->player->stat_flags |= STAT_DMG_DONE;
	}

	if(IsPlayer())
	{
		// update player damage taken
		player->dmg_taken += (int)dmg;
		player->stat_flags |= STAT_DMG_TAKEN;

		// train endurance
		player->Train(TrainWhat::TakeDamage, min(dmg, hp) / hpmax, (giver ? giver->level : -1));

		// red screen
		player->last_dmg += dmg;
	}

	// aggravate units
	if(giver && giver->IsPlayer())
		AttackReaction(*giver);

	if((hp -= dmg) <= 0.f && !IsImmortal())
	{
		// unit killed
		Die(giver);
	}
	else
	{
		// unit hurt sound
		if(hurt_timer <= 0.f && data->sounds->Have(SOUND_PAIN))
		{
			game->PlayAttachedSound(*this, data->sounds->Random(SOUND_PAIN), Unit::PAIN_SOUND_DIST);
			hurt_timer = Random(1.f, 1.5f);
			if(IsSet(dmg_flags, DMG_NO_BLOOD))
				hurt_timer += 1.f;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UNIT_SOUND;
				c.unit = this;
				c.id = SOUND_PAIN;
			}
		}

		// immortality
		if(hp < 1.f)
			hp = 1.f;

		// send update hp
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
	}
}

//=================================================================================================
void Unit::AttackReaction(Unit& attacker)
{
	if(attacker.IsPlayer() && IsAI() && in_arena == -1 && !attack_team)
	{
		if(data->group == G_CITIZENS)
		{
			if(!team->is_bandit)
			{
				if(dont_attack && IsSet(data->flags, F_PEACEFUL))
				{
					if(ai->state == AIController::Idle)
					{
						ai->state = AIController::Escape;
						ai->timer = Random(2.f, 4.f);
						ai->target = attacker;
						ai->target_last_pos = attacker.pos;
						ai->st.escape.room = nullptr;
						ai->ignore = 0.f;
						ai->in_combat = false;
					}
				}
				else
					team->SetBandit(true);
			}
		}
		else if(data->group == G_CRAZIES)
		{
			if(!team->crazies_attack)
			{
				team->crazies_attack = true;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(dont_attack && !IsSet(data->flags, F_PEACEFUL))
		{
			for(vector<Unit*>::iterator it = game_level->local_area->units.begin(), end = game_level->local_area->units.end(); it != end; ++it)
			{
				if((*it)->dont_attack && !IsSet((*it)->data->flags, F_PEACEFUL))
				{
					(*it)->dont_attack = false;
					(*it)->ai->change_ai_mode = true;
				}
			}
		}
	}
}

//=================================================================================================
bool Unit::DoAttack()
{
	Vec3 hitpoint;
	Unit* hitted;

	if(!area->CheckForHit(*this, hitted, hitpoint))
		return false;

	if(!hitted)
		return true;

	float power;
	if(data->frames->extra)
		power = 1.f;
	else
		power = data->frames->attack_power[act.attack.index];
	DoGenericAttack(*hitted, hitpoint, CalculateAttack() * act.attack.power * power, GetDmgType(), false);

	return true;
}

//=================================================================================================
bool Unit::DoShieldSmash()
{
	assert(HaveShield());

	Vec3 hitpoint;
	Unit* hitted;
	Mesh* mesh = GetShield().mesh;

	if(!mesh)
		return false;

	if(!area->CheckForHit(*this, hitted, *mesh->FindPoint("hit"), mesh_inst->mesh->GetPoint(NAMES::point_shield), hitpoint))
		return false;

	if(!hitted)
		return true;

	if(!IsSet(hitted->data->flags, F_DONT_SUFFER) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->Get(AttributeId::END)) / 50.f;

		hitted->BreakAction();

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animation_state = AS_POSITION_HURT;

		if(hitted->mesh_inst->mesh->head.n_groups == 2)
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
		else
		{
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
			hitted->animation = ANI_PLAY;
		}

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.unit = hitted;
			c.type = NetChange::STUNNED;
		}
	}

	DoGenericAttack(*hitted, hitpoint, CalculateAttack(&GetShield()), DMG_BLUNT, true);

	return true;
}

//=================================================================================================
void Unit::DoGenericAttack(Unit& hitted, const Vec3& hitpoint, float attack, int dmg_type, bool bash)
{
	// calculate modifiers
	int mod = CombatHelper::CalculateModifier(dmg_type, hitted.data->flags);
	float m = 1.f;
	if(mod == -1)
		m += 0.25f;
	else if(mod == 1)
		m -= 0.25f;
	if(hitted.IsNotFighting())
		m += 0.1f;
	else if(hitted.IsHoldingBow())
		m += 0.05f;
	if(hitted.action == A_PAIN)
		m += 0.1f;

	// backstab bonus
	float angle_dif = AngleDiff(Clip(rot + PI), hitted.rot);
	float backstab_mod = GetBackstabMod(bash ? slots[SLOT_SHIELD] : slots[SLOT_WEAPON]);
	if(IsSet(hitted.data->flags2, F2_BACKSTAB_RES))
		backstab_mod /= 2;
	m += angle_dif / PI * backstab_mod;

	// apply modifiers
	attack *= m;

	// blocking
	if(hitted.IsBlocking() && angle_dif < PI / 2)
	{
		// play sound
		MATERIAL_TYPE hitted_mat = hitted.GetShield().material;
		MATERIAL_TYPE weapon_mat = (!bash ? GetWeaponMaterial() : GetShield().material);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.count = hitted_mat;
		}
		sound_mgr->PlaySound3d(game_res->GetMaterialSound(weapon_mat, hitted_mat), hitpoint, HIT_SOUND_DIST);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, attack / hitted.hpmax, level);

		// reduce damage
		float block = hitted.CalculateBlock() * hitted.GetBlockMod();
		float stamina = min(attack, block);
		if(IsSet(data->flags2, F2_IGNORE_BLOCK))
			block *= 2.f / 3;
		if(act.attack.power >= 1.9f)
			stamina *= 4.f / 3;
		attack -= block;
		hitted.RemoveStaminaBlock(stamina);

		// pain animation & break blocking
		if(hitted.stamina < 0)
		{
			hitted.BreakAction();

			if(!IsSet(hitted.data->flags, F_DONT_SUFFER))
			{
				if(hitted.action != A_POSITION)
					hitted.action = A_PAIN;
				else
					hitted.animation_state = AS_POSITION_HURT;

				if(hitted.mesh_inst->mesh->head.n_groups == 2)
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
				else
				{
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
					hitted.animation = ANI_PLAY;
				}
			}
		}

		// attack fully blocked
		if(attack < 0)
		{
			if(IsPlayer())
			{
				// player attack blocked
				player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
				// aggravate
				hitted.AttackReaction(*this);
			}
			return;
		}
	}

	// calculate defense
	float def = hitted.CalculateDefense();

	// calculate damage
	float dmg = CombatHelper::CalculateDamage(attack, def);

	// hit sound
	game->PlayHitSound(!bash ? GetWeaponMaterial() : GetShield().material, hitted.GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

	// train player armor skill
	if(hitted.IsPlayer())
		hitted.player->Train(TrainWhat::TakeDamageArmor, attack / hitted.hpmax, level);

	// fully blocked by armor
	if(dmg < 0)
	{
		if(IsPlayer())
		{
			// player attack blocked
			player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
			// aggravate
			hitted.AttackReaction(*this);
		}
		return;
	}

	if(IsPlayer())
	{
		// player hurt someone - train
		float dmgf = (float)dmg;
		float ratio;
		if(hitted.hp - dmgf <= 0.f && !hitted.IsImmortal())
			ratio = Clamp(dmgf / hitted.hpmax, TRAIN_KILL_RATIO, 1.f);
		else
			ratio = min(1.f, dmgf / hitted.hpmax);
		player->Train(bash ? TrainWhat::BashHit : TrainWhat::AttackHit, ratio, hitted.level);
	}

	hitted.GiveDmg(dmg, this, &hitpoint);

	// apply poison
	if(IsSet(data->flags, F_POISON_ATTACK))
	{
		float poison_res = hitted.GetPoisonResistance();
		if(poison_res > 0.f)
		{
			Effect e;
			e.effect = EffectId::Poison;
			e.source = EffectSource::Temporary;
			e.source_id = -1;
			e.value = -1;
			e.power = dmg / 10 * poison_res;
			e.time = 5.f;
			hitted.AddEffect(e);
		}
	}
}
