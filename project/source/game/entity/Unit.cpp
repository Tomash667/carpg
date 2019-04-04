#include "Pch.h"
#include "GameCore.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "Inventory.h"
#include "UnitHelper.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest_Contest.h"
#include "Quest_Mages.h"
#include "AIController.h"
#include "Team.h"
#include "Content.h"
#include "SoundManager.h"
#include "GameFile.h"
#include "World.h"
#include "Level.h"
#include "BitStreamFunc.h"
#include "EntityInterpolator.h"
#include "UnitEventHandler.h"
#include "GameStats.h"
#include "GameMessages.h"
#include "GroundItem.h"
#include "ResourceManager.h"
#include "GlobalGui.h"
#include "PlayerInfo.h"
#include "Stock.h"
#include "Arena.h"
#include "Quest_Scripted.h"
#include "ScriptException.h"
#include "GameGui.h"
#include "ScriptManager.h"

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
vector<Unit*> Unit::refid_table;
vector<pair<Unit**, int>> Unit::refid_request;
int Unit::netid_counter;
static Unit* SUMMONER_PLACEHOLDER = (Unit*)0xFA4E1111;
static AIController* AI_PLACEHOLDER = (AIController*)1;

//=================================================================================================
Unit::~Unit()
{
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
float Unit::CalculateMaxHp() const
{
	float maxhp = (float)data->hp + GetEffectSum(EffectId::Health);
	if(IS_SET(data->flags2, F2_FIXED_STATS))
		maxhp = (float)(data->hp + data->hp_lvl * (level - data->level.x));
	else
	{
		float v = 0.8f*Get(AttributeId::END) + 0.2f*Get(AttributeId::STR);
		if(v >= 50.f)
			maxhp += (v - 25) * 20.f;
		else
			maxhp += 500.f - (50.f - v) * 10.f;
	}
	return maxhp;
}

//=================================================================================================
float Unit::CalculateMaxStamina() const
{
	float stamina = (float)data->stamina + GetEffectSum(EffectId::Stamina);
	if(IS_SET(data->flags2, F2_FIXED_STATS))
		return stamina;
	float v = 0.6f*Get(AttributeId::END) + 0.4f*Get(AttributeId::DEX);
	return stamina + 250.f + v * 2.f;
}

//=================================================================================================
float Unit::CalculateAttack() const
{
	if(HaveWeapon())
		return CalculateAttack(&GetWeapon());
	else if(IS_SET(data->flags2, F2_FIXED_STATS))
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

	if(IS_SET(data->flags2, F2_FIXED_STATS))
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
		if(str >= w.req_str)
			p = 1.f;
		else
			p = float(str) / w.req_str;
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
		if(str >= b.req_str)
			p = 1.f;
		else
			p = float(str) / b.req_str;
		attack += GetEffectSum(EffectId::RangedAttack)
			+ float(dex - 25)
			+ b.dmg * p
			+ Get(SkillId::BOW);
	}
	else
	{
		const Shield& s = weapon->ToShield();
		float p;
		if(str >= s.req_str)
			p = 1.f;
		else
			p = float(str) / s.req_str;
		attack += GetEffectSum(EffectId::MeleeAttack)
			+ s.block * p
			+ 0.5f * Get(SkillId::SHIELD)
			+ 0.5f * (str - 25);
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
	if(str >= s.req_str)
		p = 1.f;
	else
		p = float(str) / s.req_str;

	return float(s.block) * p
		+ float(str - 25)
		+ Get(SkillId::SHIELD);
}

//=================================================================================================
float Unit::CalculateDefense(const Item* armor) const
{
	float def = (float)data->def + GetEffectSum(EffectId::Defense);
	if(!IS_SET(data->flags2, F2_FIXED_STATS))
	{
		def += float(Get(AttributeId::END) - 25);

		if(!armor)
			armor = slots[SLOT_ARMOR];
		if(armor)
		{
			const Armor& a = armor->ToArmor();
			float skill_val = (float)Get(a.GetSkill());
			int str = Get(AttributeId::STR);
			if(str < a.req_str)
				skill_val *= float(str) / a.req_str;
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
	auto state = GetLoadState();
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
		if(player->is_local)
		{
			Game& game = Game::Get();
			game.gui->messages->AddGameMsg(Format(game.txGoldPlus, dif), 3.f);
			game.sound_mgr->PlaySound2d(game.sCoins);
		}
		else
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::GOLD_MSG;
			c.count = dif;
			c.id = 1;
			player->player_info->UpdateGold();
		}
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
	mesh_inst->groups[0].speed = 1.f;
	mesh_inst->frame_end_info = false;

	if(Net::IsLocal())
	{
		GroundItem* item = new GroundItem;
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
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = Random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}
		if(!QM.quest_secret->CheckMoonStone(item, *this))
			L.AddGroundItem(L.GetContext(*this), item);

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
	mesh_inst->groups[0].speed = 1.f;
	mesh_inst->frame_end_info = false;

	if(Net::IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = item2;
		item->count = 1;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = Random(MAX_ANGLE);
		item2 = nullptr;
		L.AddGroundItem(L.GetContext(*this), item);

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

	weight -= s.item->weight*count;

	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
	mesh_inst->groups[0].speed = 1.f;
	mesh_inst->frame_end_info = false;

	if(Net::IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = s.item;
		item->count = count;
		item->team_count = min(count, s.team_count);
		s.team_count -= item->team_count;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = Random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin() + index);
		}
		L.AddGroundItem(L.GetContext(*this), item);

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
	if(action != A_NONE)
	{
		if(action == A_TAKE_WEAPON && weapon_state == WS_HIDING)
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
					weapon_state = WS_HIDDEN;
					weapon_taken = W_NONE;
					weapon_hiding = W_NONE;
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
	if(weapon_state != WS_HIDDEN)
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
	if(action != A_NONE && action != A_ANIMATION2)
	{
		if(Net::IsLocal())
		{
			assert(0);
			return;
		}
		else
			weapon_state = WS_HIDDEN;
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
void Unit::ConsumeItemAnim(const Consumable& cons)
{
	cstring anim_name;
	if(cons.cons_type == Food)
	{
		action = A_EAT;
		anim_name = "je";
	}
	else
	{
		action = A_DRINK;
		anim_name = "pije";
	}

	animation_state = 0;
	if(current_animation == ANI_PLAY && !usable)
	{
		animation = ANI_STAND;
		current_animation = ANI_NONE;
	}
	mesh_inst->Play(anim_name, PLAY_ONCE | PLAY_PRIO1, 1);
	used_item = &cons;
}

//=================================================================================================
void Unit::UseItem(int index)
{
	assert(index >= 0 && index < int(items.size()));
	ItemSlot& slot = items[index];
	assert(slot.item);
	switch(slot.item->type)
	{
	case IT_CONSUMABLE:
		ConsumeItem(index);
		break;
	case IT_BOOK:
		assert(IS_SET(slot.item->flags, ITEM_MAGIC_SCROLL));
		if(Net::IsLocal())
		{
			action = A_USE_ITEM;
			used_item = slot.item;
			mesh_inst->frame_end_info2 = false;
			mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_ITEM;
				c.unit = this;
			}
		}
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::USE_ITEM;
			c.id = index;
			action = A_PREPARE;
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void Unit::HideWeapon()
{
	switch(weapon_state)
	{
	case WS_HIDDEN:
		return;
	case WS_HIDING:
		// anuluje wyci¹ganie nastêpnej broni po schowaniu tej
		weapon_taken = W_NONE;
		return;
	case WS_TAKING:
		if(animation_state == 0)
		{
			// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
			action = A_NONE;
			weapon_taken = W_NONE;
			weapon_state = WS_HIDDEN;
			mesh_inst->Deactivate(1);
		}
		else
		{
			// wyj¹³ broñ z pasa, zacznij chowaæ
			weapon_hiding = weapon_taken;
			weapon_taken = W_NONE;
			weapon_state = WS_HIDING;
			animation_state = 0;
			SET_BIT(mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
		}
		break;
	case WS_TAKEN:
		mesh_inst->Play(GetTakeWeaponAnimation(weapon_taken == W_ONE_HANDED), PLAY_PRIO1 | PLAY_ONCE | PLAY_BACK, 1);
		weapon_hiding = weapon_taken;
		weapon_taken = W_NONE;
		animation_state = 0;
		action = A_TAKE_WEAPON;
		weapon_state = WS_HIDING;
		mesh_inst->frame_end_info2 = false;
		break;
	}

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TAKE_WEAPON;
		c.unit = this;
		c.id = 1;
	}
}

//=================================================================================================
void Unit::TakeWeapon(WeaponType type)
{
	assert(type == W_ONE_HANDED || type == W_BOW);

	if(action != A_NONE)
		return;

	if(weapon_taken == type)
		return;

	if(weapon_taken == W_NONE)
	{
		mesh_inst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_PRIO1 | PLAY_ONCE, 1);
		weapon_hiding = W_NONE;
		weapon_taken = type;
		animation_state = 0;
		action = A_TAKE_WEAPON;
		weapon_state = WS_TAKING;
		mesh_inst->frame_end_info2 = false;

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::TAKE_WEAPON;
			c.unit = this;
			c.id = 0;
		}
	}
	else
	{
		HideWeapon();
		weapon_taken = type;
	}
}

//=================================================================================================
// Dodaje przedmiot(y) do ekwipunku, zwraca true jeœli przedmiot siê zestackowa³
//=================================================================================================
bool Unit::AddItem(const Item* item, uint count, uint team_count)
{
	assert(item && count != 0 && team_count <= count);

	if(item->type == IT_GOLD && Team.IsTeamMember(*this))
	{
		if(Net::IsLocal())
		{
			if(team_count && IsTeamMember())
			{
				Team.AddGold(team_count);
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
void Unit::AddItem2(const Item* item, uint count, uint team_count, bool show_msg)
{
	Game::Get().PreloadItem(item);
	AddItem(item, count, team_count);

	if(IsPlayer())
	{
		if(!player->IsLocal())
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::ADD_ITEMS;
			c.item = item;
			c.id = team_count;
			c.count = count;
		}
		if(show_msg)
			player->AddItemMessage(count);
	}
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
		RecalculateHp(true);
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
	}
}

//=================================================================================================
void Unit::ApplyConsumableEffect(const Consumable& item)
{
	switch(item.effect)
	{
	case E_HEAL:
		hp += item.power;
		if(hp > hpmax)
			hp = hpmax;
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
		break;
	case E_POISON:
	case E_ALCOHOL:
		{
			float poison_res = GetPoisonResistance();
			if(poison_res > 0.f)
			{
				Effect e;
				e.effect = item.ToEffect();
				e.source = EffectSource::Temporary;
				e.source_id = -1;
				e.value = -1;
				e.time = item.time;
				e.power = item.power / item.time * poison_res;
				AddEffect(e);
			}
		}
		break;
	case E_REGENERATE:
	case E_NATURAL:
	case E_ANTIMAGIC:
	case E_STAMINA:
		{
			Effect e;
			e.effect = item.ToEffect();
			e.source = EffectSource::Temporary;
			e.source_id = -1;
			e.value = -1;
			e.time = item.time;
			e.power = item.power;
			AddEffect(e);
		}
		break;
	case E_ANTIDOTE:
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
	case E_NONE:
		break;
	case E_STR:
		if(IsPlayer())
			player->Train(false, (int)AttributeId::STR, TrainMode::Potion);
		break;
	case E_END:
		if(IsPlayer())
			player->Train(false, (int)AttributeId::END, TrainMode::Potion);
		break;
	case E_DEX:
		if(IsPlayer())
			player->Train(false, (int)AttributeId::DEX, TrainMode::Potion);
		break;
	case E_FOOD:
		{
			Effect e;
			e.effect = item.ToEffect();
			e.source = EffectSource::Temporary;
			e.source_id = -1;
			e.value = -1;
			e.time = item.power;
			e.power = 1.f;
			AddEffect(e);
		}
		break;
	case E_GREEN_HAIR:
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
	case E_STUN:
		ApplyStun(item.time);
		break;
	default:
		assert(0);
		break;
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
	Game& game = Game::Get();
	float regen = 0.f, temp_regen = 0.f, poison_dmg = 0.f, alco_sum = 0.f, best_stamina = 0.f, stamina_mod = 1.f;
	int food_heal = 0;

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
			++food_heal;
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
		game.GiveDmg(L.GetContext(*this), nullptr, poison_dmg * dt, *this, nullptr, Game::DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(Net::IsOnline() && !player->is_local && player->last_dmg_poison != poison_dmg)
			player->player_info->update_flags |= PlayerInfo::UF_POISON_DAMAGE;
		player->last_dmg_poison = poison_dmg;
	}

	// restore stamina
	if(stamina_timer > 0)
		stamina_timer -= dt;
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
		if(Net::IsServer() && player && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
	}
}

//=================================================================================================
void Unit::EndEffects(int days, float* o_natural_mod)
{
	alcohol = 0.f;
	stamina = stamina_max;
	stamina_timer = 0;

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
			food += it->time;
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

	if(item->IsStackable())
		AddItem(item, count, count);
	else
	{
		// za³ó¿ jeœli nie ma
		switch(item->type)
		{
		case IT_WEAPON:
			if(!HaveWeapon())
			{
				slots[SLOT_WEAPON] = item;
				--count;
			}
			break;
		case IT_BOW:
			if(!HaveBow())
			{
				slots[SLOT_BOW] = item;
				--count;
			}
			break;
		case IT_SHIELD:
			if(!HaveShield())
			{
				slots[SLOT_SHIELD] = item;
				--count;
			}
			break;
		case IT_ARMOR:
			if(!HaveArmor())
			{
				slots[SLOT_ARMOR] = item;
				--count;
			}
			break;
		case IT_AMULET:
			if(!HaveAmulet())
			{
				slots[SLOT_AMULET] = item;
				--count;
			}
			break;
		}

		if(count)
			AddItem(item, count, count);
	}
}

//=================================================================================================
void Unit::CalculateLoad()
{
	if(IS_SET(data->flags2, F2_FIXED_STATS))
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
bool Unit::IsBetterWeapon(const Weapon& weapon, int* value) const
{
	if(!HaveWeapon())
	{
		if(value)
			*value = (int)CalculateWeaponPros(weapon);
		return true;
	}

	if(value)
	{
		float v = CalculateWeaponPros(weapon);
		*value = (int)v;
		return CalculateWeaponPros(GetWeapon()) < v;
	}
	else
		return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(weapon);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& armor, int* value) const
{
	if(!HaveArmor())
	{
		if(value)
			*value = (int)CalculateDefense(&armor);
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&armor);
		*value = (int)v;
		return CalculateDefense() < v;
	}
	else
		return CalculateDefense() < CalculateDefense(&armor);
}

//=================================================================================================
void Unit::RecalculateHp(bool send)
{
	float hpp = hp / hpmax;
	hpmax = CalculateMaxHp();
	hp = hpmax * hpp;
	if(hp < 1)
		hp = 1;
	else if(hp > hpmax)
		hp = hpmax;

	if(send && player && !player->IsLocal() && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UPDATE_HP;
		c.unit = this;
	}
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
			if(IS_SET(data->frames->extra->e[n].flags, a))
				return n;
		} while(1);
	}
	else
		return Rand() % data->frames->attacks;
}

//=================================================================================================
void Unit::Save(GameWriter& f, bool local)
{
	// unit data
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
				f << slot.item->refid;
		}
		else
			f.Write0();
	}
	if(stock)
	{
		if(local || W.GetWorldtime() - stock->date < 10)
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
	f << invisible;
	f << in_building;
	f << to_remove;
	f << temporary;
	f << quest_refid;
	f << assist;
	f << auto_talk;
	if(auto_talk != AutoTalkMode::No)
	{
		f << (auto_talk_dialog ? auto_talk_dialog->id.c_str() : "");
		f << auto_talk_timer;
	}
	f << dont_attack;
	f << attack_team;
	f << netid;
	f << (event_handler ? event_handler->GetUnitEventHandlerQuestRefid() : -1);
	f << weight;
	f << (guard_target ? guard_target->refid : -1);
	f << (summoner ? summoner->refid : -1);
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

	if(local)
	{
		mesh_inst->Save(f);
		f << animation;
		f << current_animation;

		f << prev_pos;
		f << speed;
		f << prev_speed;
		f << animation_state;
		f << attack_id;
		f << action;
		f << weapon_taken;
		f << weapon_hiding;
		f << weapon_state;
		f << hitted;
		f << hurt_timer;
		f << target_pos;
		f << target_pos2;
		f << talking;
		f << talk_timer;
		f << attack_power;
		f << run_attack;
		f << timer;
		f << alcohol;
		f << raise_timer;

		if(action == A_DASH)
			f << use_rot;

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
				Warn("Invalid usable %s (%d) user %s.", usable->base->id.c_str(), usable->refid, data->id.c_str());
				usable = nullptr;
				f << -1;
			}
			else
				f << usable->refid;
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
		f << dialog.quest->refid;
		f << dialog.dialog->id;
	}

	// events
	f << events.size();
	for(Event& e : events)
	{
		f << e.type;
		f << e.quest->refid;
	}

	// order
	f << order;
	f << order_timer;
	switch(order)
	{
	case ORDER_LOOK_AT:
		f << order_pos;
		break;
	case ORDER_MOVE:
		f << order_pos;
		f << order_move_type;
		break;
	case ORDER_ESCAPE_TO:
		f << order_pos;
		break;
	case ORDER_ESCAPE_TO_UNIT:
		f << order_unit->refid;
		f << order_pos;
		break;
	}

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
				f << slot.item->refid;
		}
		else
			f.Write0();
	}
}

//=================================================================================================
void Unit::Load(GameReader& f, bool local)
{
	human_data = nullptr;

	// unit data
	data = UnitData::Get(f.ReadString1());

	// items
	bool can_sort = true;
	int max_slots;
	if(LOAD_VERSION >= V_DEV)
		max_slots = SLOT_MAX;
	else if(LOAD_VERSION >= V_0_9)
		max_slots = 5;
	else
		max_slots = 4;
	for(int i = 0; i < max_slots; ++i)
		f.ReadOptional(slots[i]);
	if(LOAD_VERSION < V_0_9)
		slots[SLOT_AMULET] = nullptr;
	if(LOAD_VERSION < V_DEV)
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
			int quest_item_refid = f.Read<int>();
			QM.AddQuestItemRequest(&slot.item, item_id.c_str(), quest_item_refid, &items, this);
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
	if(LOAD_VERSION >= V_0_5)
	{
		f >> stamina;
		f >> stamina_max;
		f >> stamina_action;
		if(LOAD_VERSION >= V_0_7)
			f >> stamina_timer;
		else
			stamina_timer = 0;
	}
	if(LOAD_VERSION < V_0_5)
		f.Skip<int>(); // old type
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
		}
		else
		{
			SubprofileInfo sub;
			f >> sub;
			stats = data->GetStats(sub);
		}
	}
	else if(LOAD_VERSION >= V_0_4)
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
	else
	{
		if(data->group == G_PLAYER)
		{
			stats = new UnitStats;
			stats->fixed = false;
			stats->subprofile.value = 0;
			for(int i = 0; i < 3; ++i)
				f >> stats->attrib[i];
			for(int i = 3; i < (int)AttributeId::MAX; ++i)
				stats->attrib[i] = UnitStats::NEW_STAT;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				stats->skill[i] = UnitStats::NEW_STAT;
			int old_skill[(int)old::SkillId::MAX];
			f >> old_skill;
			stats->skill[(int)SkillId::ONE_HANDED_WEAPON] = old_skill[(int)old::SkillId::WEAPON];
			stats->skill[(int)SkillId::BOW] = old_skill[(int)old::SkillId::BOW];
			stats->skill[(int)SkillId::SHIELD] = old_skill[(int)old::SkillId::SHIELD];
			stats->skill[(int)SkillId::LIGHT_ARMOR] = old_skill[(int)old::SkillId::LIGHT_ARMOR];
			stats->skill[(int)SkillId::HEAVY_ARMOR] = old_skill[(int)old::SkillId::HEAVY_ARMOR];
		}
		else
		{
			stats = data->GetStats(level);
			f.Skip(sizeof(int) * (3 + (int)old::SkillId::MAX));
		}
	}
	f >> gold;
	f >> invisible;
	f >> in_building;
	f >> to_remove;
	f >> temporary;
	f >> quest_refid;
	f >> assist;

	// auto talking
	if(LOAD_VERSION >= V_0_5)
	{
		f >> auto_talk;
		if(auto_talk != AutoTalkMode::No)
		{
			const string& dialog_id = f.ReadString1();
			if(dialog_id.empty())
				auto_talk_dialog = nullptr;
			else
				auto_talk_dialog = GameDialog::TryGet(dialog_id.c_str());
			f >> auto_talk_timer;
		}
	}
	else
	{
		auto_talk_timer = Unit::AUTO_TALK_WAIT;
		auto_talk_dialog = nullptr;

		int old_auto_talk;
		f >> old_auto_talk;
		if(old_auto_talk == 2)
		{
			f >> auto_talk_timer;
			auto_talk_timer = 1.f - auto_talk_timer;
		}
		auto_talk = (AutoTalkMode)old_auto_talk;
	}

	f >> dont_attack;
	f >> attack_team;
	f >> netid;
	int unit_event_handler_quest_refid = f.Read<int>();
	if(unit_event_handler_quest_refid == -2)
		event_handler = QM.quest_contest;
	else if(unit_event_handler_quest_refid == -1)
		event_handler = nullptr;
	else
	{
		event_handler = reinterpret_cast<UnitEventHandler*>(unit_event_handler_quest_refid);
		Game::Get().load_unit_handler.push_back(this);
	}
	CalculateLoad();
	if(can_sort && content.require_update)
		SortItems(items);
	f >> weight;
	if(can_sort && content.require_update)
		RecalculateWeight();

	int guard_refid = f.Read<int>();
	if(guard_refid == -1)
		guard_target = nullptr;
	else
		AddRequest(&guard_target, guard_refid);

	if(LOAD_VERSION >= V_0_5)
	{
		int summoner_refid = f.Read<int>();
		if(summoner_refid == -1)
			summoner = nullptr;
		else
			AddRequest(&summoner, summoner_refid);
	}
	else
		summoner = nullptr;

	if(live_state >= DYING)
	{
		if(LOAD_VERSION >= V_0_8)
			f >> mark;
		else
		{
			for(ItemSlot& item : items)
			{
				if(item.item && IS_SET(item.item->flags, ITEM_IMPORTANT))
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

	if(local)
	{
		CreateMesh(CREATE_MESH::LOAD);
		mesh_inst->Load(f);
		f >> animation;
		f >> current_animation;

		f >> prev_pos;
		f >> speed;
		f >> prev_speed;
		f >> animation_state;
		f >> attack_id;
		f >> action;
		f >> weapon_taken;
		f >> weapon_hiding;
		f >> weapon_state;
		f >> hitted;
		f >> hurt_timer;
		f >> target_pos;
		f >> target_pos2;
		f >> talking;
		f >> talk_timer;
		f >> attack_power;
		f >> run_attack;
		f >> timer;
		f >> alcohol;
		f >> raise_timer;

		if(action == A_DASH)
			f >> use_rot;

		const string& item_id = f.ReadString1();
		if(!item_id.empty())
		{
			used_item = Item::Get(item_id);
			f >> used_item_is_team;
		}
		else
			used_item = nullptr;

		int usable_refid = f.Read<int>();
		if(usable_refid == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usable_refid, this);

		if(action == A_SHOOT)
		{
			bow_instance = Game::Get().GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = mesh_inst->groups[1].speed;
			bow_instance->groups[0].time = mesh_inst->groups[1].time;
		}

		f >> last_bash;
		if(LOAD_VERSION >= V_0_5)
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
		weapon_state = WS_HIDDEN;
		weapon_taken = W_NONE;
		weapon_hiding = W_NONE;
		frozen = FROZEN::NO;
		talking = false;
		animation = current_animation = ANI_STAND;
		action = A_NONE;
		run_attack = false;
		hurt_timer = 0.f;
		speed = prev_speed = 0.f;
		alcohol = 0.f;
		moved = false;
		running = false;
	}

	// effects
	if(LOAD_VERSION >= V_DEV)
		f.ReadVector4(effects);
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

	if(LOAD_VERSION >= V_0_9)
	{
		// dialogs
		dialogs.resize(f.Read<uint>());
		for(QuestDialog& dialog : dialogs)
		{
			int quest_refid = f.Read<int>();
			string* str = StringPool.Get();
			f >> *str;
			QM.AddQuestRequest(quest_refid, (Quest**)&dialog.quest, [this, &dialog, str]()
			{
				dialog.dialog = dialog.quest->GetDialog(*str);
				StringPool.Free(str);
				dialog.quest->AddDialogPtr(this);
			});
		}
	}
	if(LOAD_VERSION >= V_DEV)
	{
		// events
		events.resize(f.Read<uint>());
		for(Event& e : events)
		{
			int refid;
			f >> e.type;
			f >> refid;
			QM.AddQuestRequest(refid, (Quest**)&e.quest, [&]()
			{
				EventPtr event;
				event.source = EventPtr::UNIT;
				event.unit = this;
				e.quest->AddEventPtr(event);
			});
		}

		// order
		f >> order;
		f >> order_timer;
		switch(order)
		{
		case ORDER_LOOK_AT:
			f >> order_pos;
			break;
		case ORDER_MOVE:
			f >> order_pos;
			f >> order_move_type;
			break;
		case ORDER_ESCAPE_TO:
			f >> order_pos;
			break;
		case ORDER_ESCAPE_TO_UNIT:
			order_unit = Unit::GetByRefid(f.Read<int>());
			f >> order_pos;
			break;
		}
	}
	else
	{
		order = ORDER_NONE;
		order_timer = 0.f;
	}

	if(f.Read1())
	{
		player = new PlayerController;
		player->unit = this;
		player->Load(f);
	}
	else
		player = nullptr;

	if(local && human_data)
		human_data->ApplyScale(mesh_inst->mesh);

	if(IS_SET(data->flags, F_HERO))
	{
		hero = new HeroData;
		hero->unit = this;
		hero->Load(f);
	}
	else
		hero = nullptr;

	in_arena = -1;
	ai = nullptr;
	look_target = nullptr;
	interp = nullptr;
	frozen = FROZEN::NO;

	// fizyka
	if(local)
		CreatePhysics(true);
	else
		cobj = nullptr;

	// zabezpieczenie
	if(((weapon_state == WS_TAKEN || weapon_state == WS_TAKING) && weapon_taken == W_NONE) ||
		(weapon_state == WS_HIDING && weapon_hiding == W_NONE))
	{
		weapon_state = WS_HIDDEN;
		weapon_taken = W_NONE;
		weapon_hiding = W_NONE;
		Warn("Unit '%s' had broken weapon state.", data->id.c_str());
	}

	// calculate new skills/attributes
	if(LOAD_VERSION >= V_0_8)
	{
		if(content.require_update)
			CalculateStats();
	}
	else if(LOAD_VERSION >= V_0_4)
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
	else
	{
		if(IsPlayer())
		{
			player->RecalculateLevel();

			// set new attributes & skills
			StatProfile& profile = data->GetStatProfile();
			stats->subprofile.value = 0;
			stats->subprofile.level = level;
			stats->SetForNew(profile);

			// set apptitude
			UnitStats base_stats;
			base_stats.subprofile.value = 0;
			base_stats.fixed = false;
			base_stats.Set(profile);
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				player->attrib[i].apt = (base_stats.attrib[i] - 50) / 5;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				player->skill[i].apt = base_stats.skill[i] / 5;
			player->SetRequiredPoints();
		}
		CalculateStats();
	}

	// compability
	if(LOAD_VERSION < V_0_5)
	{
		stamina_max = CalculateMaxStamina();
		stamina = stamina_max;
		stamina_action = SA_RESTORE_MORE;
		stamina_timer = 0;
	}
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
			int quest_refid;
			f >> quest_refid;
			QM.AddQuestItemRequest(&slot.item, item_id.c_str(), quest_refid, &cnt);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && content.require_update)
		SortItems(cnt);
}

//=================================================================================================
void Unit::Write(BitStreamWriter& f)
{
	// main
	f << data->id;
	f << netid;

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
	f << hp;
	f << hpmax;
	f << netid;
	f.WriteCasted<char>(in_arena);
	f << (summoner != nullptr);
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
		f << b;
		f << hero->credit;
	}
	else if(IsPlayer())
	{
		f << player->name;
		f.WriteCasted<byte>(player->id);
		f << player->credit;
		f << player->free_days;
	}
	if(IsAI())
		f << GetAiMode();

	// loaded data
	if(N.mp_load || L.reenter)
	{
		f << netid;
		mesh_inst->Write(f);
		f.WriteCasted<byte>(animation);
		f.WriteCasted<byte>(current_animation);
		f.WriteCasted<byte>(animation_state);
		f.WriteCasted<byte>(attack_id);
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
		f << (usable ? usable->netid : -1);
	}
}

//=================================================================================================
bool Unit::Read(BitStreamReader& f)
{
	Game& game = Game::Get();

	// main
	const string& id = f.ReadString1();
	f >> netid;
	if(!f)
		return false;
	data = UnitData::TryGet(id);
	if(!data)
	{
		Error("Missing base unit id '%s'!", id.c_str());
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
					game.PreloadItem(item);
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
	f.ReadCasted<byte>(live_state);
	f >> pos;
	f >> rot;
	f >> hp;
	f >> hpmax;
	f >> netid;
	f.ReadCasted<char>(in_arena);
	bool is_summoned = f.Read<bool>();
	if(!f)
		return false;
	if(live_state >= Unit::LIVESTATE_MAX)
	{
		Error("Invalid live state %d.", live_state);
		return false;
	}
	summoner = (is_summoned ? SUMMONER_PLACEHOLDER : nullptr);
	f >> mark;

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
		hero = new HeroData;
		hero->unit = this;
		f >> hero->name;
		f >> flags;
		f >> hero->credit;
		if(!f)
			return false;
		hero->know_name = IS_SET(flags, 0x01);
		hero->team_member = IS_SET(flags, 0x02);
	}
	else if(type == 2)
	{
		// player
		ai = nullptr;
		hero = nullptr;
		player = new PlayerController;
		player->unit = this;
		alcohol = 0.f;
		f >> player->name;
		f.ReadCasted<byte>(player->id);
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
		PlayerInfo* info = N.TryGetPlayer(player->id);
		if(!info)
		{
			Error("Invalid player id %d.", player->id);
			return false;
		}
		info->u = this;
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
	CreateMesh(N.mp_load ? Unit::CREATE_MESH::PRELOAD : Unit::CREATE_MESH::NORMAL);

	action = A_NONE;
	weapon_taken = W_NONE;
	weapon_hiding = W_NONE;
	weapon_state = WS_HIDDEN;
	talking = false;
	busy = Unit::Busy_No;
	in_building = -1;
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
	animation_state = 0;

	if(N.mp_load || L.reenter)
	{
		// get current state in multiplayer
		f >> netid;
		if(!mesh_inst->Read(f))
			return false;
		f.ReadCasted<byte>(animation);
		f.ReadCasted<byte>(current_animation);
		f.ReadCasted<byte>(animation_state);
		f.ReadCasted<byte>(attack_id);
		f.ReadCasted<byte>(action);
		f.ReadCasted<byte>(weapon_taken);
		f.ReadCasted<byte>(weapon_hiding);
		f.ReadCasted<byte>(weapon_state);
		f >> target_pos;
		f >> target_pos2;
		f >> timer;
		const string& used_item_id = f.ReadString1();
		int usable_netid = f.Read<int>();
		if(!f)
			return false;

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
		if(usable_netid == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usable_netid, this);

		// bow animesh instance
		if(action == A_SHOOT)
		{
			bow_instance = game.GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = mesh_inst->groups[1].speed;
			bow_instance->groups[0].time = mesh_inst->groups[1].time;
		}
	}

	// physics
	CreatePhysics(true);

	// boss music
	if(IS_SET(data->flags2, F2_BOSS))
		W.AddBossLevel();

	prev_pos = pos;
	speed = prev_speed = 0.f;
	talking = false;

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
	float missing = hpmax - hp, heal = 0, heal2 = Inf();
	int id = -1, id2 = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item && it->item->type == IT_CONSUMABLE)
		{
			const Consumable& pot = it->item->ToConsumable();
			if(!pot.IsHealingPotion())
				continue;

			if(pot.power <= missing)
			{
				if(pot.power > heal)
				{
					heal = pot.power;
					id = index;
				}
			}
			else
			{
				if(pot.power < heal2)
				{
					heal2 = pot.power;
					id2 = index;
				}
			}
		}
	}

	if(id != -1)
	{
		if(id2 != -1)
		{
			if(missing - heal < heal2 - missing)
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
	if(N.active_players > 1)
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
		else if(item_slot.item->IsWearableByHuman())
		{
			ITEM_SLOT slot = ItemTypeToSlot(item_slot.item->type);
			assert(slot != SLOT_INVALID);

			if(slots[slot])
			{
				if(slots[slot]->value < item_slot.item->value)
				{
					const Item* item = slots[slot];
					slots[slot] = item_slot.item;
					item_slot.item = item;
				}
			}
			else
			{
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
		Game::Get().PreloadItem(item);
		AddItemAndEquipIfNone(item);
	}
}

//=================================================================================================
bool Unit::HaveQuestItem(int quest_refid)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(quest_refid))
			return true;
	}
	return false;
}

//=================================================================================================
void Unit::RemoveQuestItem(int quest_refid)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(quest_refid))
		{
			weight -= it->item->weight;
			items.erase(it);
			if(IsClient())
			{
				NetChangePlayer& c = Add1(player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_QUEST_ITEM;
				c.id = quest_refid;
			}
			return;
		}
	}

	assert(0 && "Nie znalaziono questowego przedmiotu do usuniêcia!");
}

//=================================================================================================
void Unit::RemoveQuestItemS(Quest* quest)
{
	RemoveQuestItem(quest->refid);
}

//=================================================================================================
bool Unit::HaveItem(const Item* item)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item == item)
			return true;
	}
	return false;
}

//=================================================================================================
float Unit::GetAttackSpeed(const Weapon* used_weapon) const
{
	if(IS_SET(data->flags2, F2_FIXED_STATS))
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
	if(IS_SET(data->flags2, F2_FIXED_STATS))
		return 1.f;

	// values range
	//	1 dex, 0 skill = 0.8
	// 50 dex, 10 skill = 1.1
	// 100 dex, 100 skill = 1.7
	float mod = 0.8f + float(Get(SkillId::BOW)) / 200 + 0.004f*Get(AttributeId::DEX) - GetAttackSpeedModFromStrength(GetBow());

	float mobility = CalculateMobility();
	if(mobility < 100)
		mod -= Lerp(0.1f, 0.f, mobility / 100);
	else if(mobility > 100)
		mod += Lerp(0.f, 0.1f, (mobility - 100) / 100);

	if(mod < 0.5f)
		mod = 0.5f;

	return mod;
}

//=================================================================================================
void Unit::MakeItemsTeam(bool team)
{
	if(team)
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
	if(hp < 1.f)
		hp = 1.f;

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
int Unit::FindItem(const Item* item, int quest_refid) const
{
	assert(item);

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] == item && (quest_refid == -1 || slots[i]->IsQuest(quest_refid)))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item && (quest_refid == -1 || it->item->IsQuest(quest_refid)))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
int Unit::FindQuestItem(int quest_refid) const
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] && slots[i]->IsQuest(quest_refid))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(quest_refid))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
bool Unit::FindQuestItem(cstring id, Quest** out_quest, int* i_index, bool not_active)
{
	assert(id);

	if(id[1] == '$')
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] && slots[i]->IsQuest())
			{
				Quest* quest = QM.FindQuest(slots[i]->refid, !not_active);
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
			if(it2->item && it2->item->IsQuest())
			{
				Quest* quest = QM.FindQuest(it2->item->refid, !not_active);
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
				Quest* quest = QM.FindQuest(slots[i]->refid, !not_active);
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
				Quest* quest = QM.FindQuest(it2->item->refid, !not_active);
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
	Game& game = Game::Get();

	// remove item
	uint removed;
	if(i_index >= 0)
	{
		ItemSlot& s = items[i_index];
		removed = (count == 0 ? s.count : min(s.count, count));
		s.count -= removed;
		if(s.count == 0)
			items.erase(items.begin() + i_index);
		else if(s.team_count > 0)
			s.team_count -= min(s.team_count, removed);
		weight -= s.item->weight*removed;
	}
	else
	{
		ITEM_SLOT type = IIndexToSlot(i_index);
		weight -= slots[type]->weight;
		slots[type] = nullptr;
		removed = 1;

		if(Net::IsServer() && N.active_players > 1 && IsVisible(type))
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
			for(Unit* member : Team.active_members)
			{
				if(member->IsPlayer() && member->player->IsTradingWith(this))
				{
					t = member;
					break;
				}
			}

			if(t && t->player != game.pc)
			{
				NetChangePlayer& c = Add1(t->player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS_TRADER;
				c.id = netid;
				c.count = count;
				c.a = i_index;
			}
		}
	}

	// update temporary inventory
	if(game.pc->unit == this)
	{
		if(game.gui->inventory->inv_mine->visible || game.gui->inventory->gp_trade->visible)
			game.gui->inventory->BuildTmpInventory(0);
	}
	else if(game.gui->inventory->gp_trade->visible && game.gui->inventory->inv_trade_other->unit == this)
		game.gui->inventory->BuildTmpInventory(1);

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
	weapon_taken = W_NONE;
	weapon_hiding = W_NONE;
	weapon_state = WS_HIDDEN;
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
float Unit::GetBlockSpeed() const
{
	// 	const Shield& s = GetShield();
	// 	float block_speed = 0.33f;
	// 	if(attrib[A_STR] < s.sila)
	// 		block_speed
	return 0.1f;
}

//=================================================================================================
bool Unit::IsBetterItem(const Item* item, int* value) const
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		if(!HaveWeapon())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else if(!IS_SET(data->flags, F_MAGE))
			return IsBetterWeapon(item->ToWeapon(), value);
		else
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > GetWeapon().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_BOW:
		if(!HaveBow())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else
		{
			if(GetBow().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_ARMOR:
		if(!IS_SET(data->flags, F_MAGE))
			return IsBetterArmor(item->ToArmor(), value);
		else
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > GetArmor().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_SHIELD:
		if(!HaveShield())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else
		{
			if(GetShield().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_AMULET:
		if(!HaveAmulet())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else
		{
			if(GetAmulet().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	default:
		assert(0);
		return false;
	}
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
float Unit::CalculateMagicResistance() const
{
	float mres = 1.f;
	if(IS_SET(data->flags2, F2_MAGIC_RES50))
		mres = 0.5f;
	else if(IS_SET(data->flags2, F2_MAGIC_RES25))
		mres = 0.75f;
	if(HaveArmor())
	{
		const Armor& a = GetArmor();
		if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
		else if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
	}
	if(HaveShield())
	{
		const Shield& s = GetShield();
		if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
		else if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
	}
	float effect_mres = GetEffectMul(EffectId::MagicResistance);
	return mres * effect_mres;
}

//=================================================================================================
float Unit::GetPoisonResistance() const
{
	if(IS_SET(data->flags, F_POISON_RES))
		return 0.f;
	return GetEffectMul(EffectId::PoisonResistance);
}

//=================================================================================================
int Unit::CalculateMagicPower() const
{
	int mlvl = 0;
	if(HaveArmor())
		mlvl += GetArmor().GetMagicPower();
	if(weapon_state == WS_TAKEN)
	{
		if(weapon_taken == W_ONE_HANDED)
			mlvl += GetWeapon().GetMagicPower();
		else
			mlvl += GetBow().GetMagicPower();
	}
	return mlvl;
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
		Effect& e = effects[index];
		if(send)
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::REMOVE_EFFECT;
			c.id = (int)e.effect;
			c.count = (int)e.source;
			c.a1 = e.source_id;
			c.a2 = e.value;
		}

		OnAddRemoveEffect(e);

		_to_remove.pop_back();
		if(index == effects.size() - 1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin() + index, effects.end() - 1);
			effects.pop_back();
		}
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
		}
	}

	if(alcohol / hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

//=================================================================================================
bool Unit::CanAct()
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
int Unit::Get(SkillId s, StatState* state) const
{
	int index = (int)s;
	int value = stats->skill[index];
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
	stats->skill[(int)s] = value;
	if(player)
		player->recalculate_level = true;
}

//=================================================================================================
void Unit::ApplyStat(AttributeId a)
{
	// recalculate other stats
	switch(a)
	{
	case AttributeId::STR:
		{
			// hp/load depends on str
			if(Net::IsLocal())
			{
				RecalculateHp();
				if(!fake_unit && Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = this;
				}
			}
			else
				hpmax = CalculateMaxHp();
			CalculateLoad();
		}
		break;
	case AttributeId::END:
		{
			// hp/stamina depends on end
			if(Net::IsLocal())
			{
				RecalculateHp();
				RecalculateStamina();
				if(!fake_unit && Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = this;
					if(IsPlayer() && !player->is_local)
						player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
				}
			}
			else
			{
				hpmax = CalculateMaxHp();
				stamina_max = CalculateMaxStamina();
			}
		}
		break;
	case AttributeId::DEX:
		{
			// stamina depends on dex
			if(Net::IsLocal())
			{
				RecalculateStamina();
				if(!fake_unit && Net::IsServer() && IsPlayer() && !player->is_local)
					player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
			}
			else
				stamina_max = CalculateMaxStamina();
		}
		break;
	case AttributeId::INT:
	case AttributeId::WIS:
	case AttributeId::CHA:
		break;
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
	if(IS_SET(data->flags2, F2_FIXED_STATS))
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
		if(str < armor->req_str)
		{
			armor_mobility -= armor->req_str - str;
			if(armor_mobility < 0)
				armor_mobility = 0;
		}

		// multiply mobility by armor mobility
		mobility = (float(armor_mobility) / 100 * mobility);
	}

	auto load_state = GetLoadState();
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
	auto mat_scale = (human_data ? human_data->mat_scale.data() : nullptr);
	if(anim_name)
		mesh_inst->SetToEnd(anim_name, mat_scale);
	else
		mesh_inst->SetToEnd(mat_scale);
}

//=================================================================================================
void Unit::StartAutoTalk(bool leader, GameDialog* dialog)
{
	if(!leader)
	{
		auto_talk = AutoTalkMode::Yes;
		auto_talk_timer = AUTO_TALK_WAIT;
	}
	else
	{
		auto_talk = AutoTalkMode::Leader;
		auto_talk_timer = 0.f;
	}
	auto_talk_dialog = dialog;
}

//=================================================================================================
void Unit::SetAutoTalk(bool new_auto_talk)
{
	if(new_auto_talk == GetAutoTalk())
		return;
	if(new_auto_talk)
		StartAutoTalk();
	else
		auto_talk = AutoTalkMode::No;
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
		if(IS_SET(usable->base->use_flags, BaseUsable::SLOW_STAMINA_RESTORE))
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
			if(animation_state < 2)
				stamina_action = SA_RESTORE_SLOW;
			else
				stamina_action = SA_RESTORE;
			break;
		case A_EAT:
		case A_DRINK:
		case A_ANIMATION:
		case A_ANIMATION2:
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
void Unit::RemoveStamina(float value)
{
	stamina -= value;
	stamina_timer = STAMINA_RESTORE_TIMER;
	if(player && Net::IsLocal())
	{
		player->Train(TrainWhat::Stamina, value, 0);
		if(!player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
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
		if(ResourceManager::Get().IsLoadScreen())
		{
			if(mode == CREATE_MESH::LOAD)
				ResourceManager::Get<Mesh>().Load(mesh);
			else if(!mesh->IsLoaded())
				Game::Get().units_mesh_load.push_back(pair<Unit*, bool>(this, mode == CREATE_MESH::ON_WORLDMAP));
			if(data->state == ResourceState::NotLoaded)
			{
				ResourceManager::Get<Mesh>().AddLoadTask(mesh);
				if(data->sounds)
				{
					auto& sound_mgr = ResourceManager::Get<Sound>();
					for(int i = 0; i < SOUND_MAX; ++i)
					{
						for(SoundPtr sound : data->sounds->sounds[i])
							sound_mgr.AddLoadTask(sound);
					}
				}
				if(data->tex)
				{
					auto& tex_mgr = ResourceManager::Get<Texture>();
					for(auto& tex : data->tex->textures)
					{
						if(tex.tex)
							tex_mgr.AddLoadTask(tex.tex);
					}
				}
				data->state = ResourceState::Loading;
				ResourceManager::Get().AddTask(this, TaskCallback([](TaskData& td)
				{
					Unit* unit = static_cast<Unit*>(td.ptr);
					unit->data->state = ResourceState::Loaded;
				}));
			}
		}
		else
		{
			ResourceManager::Get<Mesh>().Load(mesh);
			if(data->sounds)
			{
				auto& sound_mgr = ResourceManager::Get<Sound>();
				for(int i = 0; i < SOUND_MAX; ++i)
				{
					for(SoundPtr sound : data->sounds->sounds[i])
						sound_mgr.Load(sound);
				}
			}
			if(data->tex)
			{
				auto& tex_mgr = ResourceManager::Get<Texture>();
				for(auto& tex : data->tex->textures)
				{
					if(tex.tex)
						tex_mgr.Load(tex.tex);
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

			mesh_inst->groups[0].speed = 1.f;
			if(mesh_inst->mesh->head.n_groups > 1)
				mesh_inst->groups[1].state = 0;
			if(human_data)
				human_data->ApplyScale(mesh_inst->mesh);
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
		mesh_inst = new MeshInstance(nullptr, true);
	}
}

//=================================================================================================
void Unit::ApplyStun(float length)
{
	if(Net::IsLocal() && IS_SET(data->flags2, F2_STUN_RESISTANCE))
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
		animation = ANI_STAND;
	}

	if(Net::IsOnline() && Net::IsServer())
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
		return !IS_SET(ai_mode, AI_MODE_IDLE);
}

//=================================================================================================
bool Unit::IsAssist() const
{
	if(Net::IsLocal())
		return assist;
	else
		return IS_SET(ai_mode, AI_MODE_ASSIST);
}

//=================================================================================================
bool Unit::IsDontAttack() const
{
	if(Net::IsLocal())
		return dont_attack;
	else
		return IS_SET(ai_mode, AI_MODE_DONT_ATTACK);
}

//=================================================================================================
bool Unit::WantAttackTeam() const
{
	if(Net::IsLocal())
		return attack_team;
	else
		return IS_SET(ai_mode, AI_MODE_ATTACK_TEAM);
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
	Game& game = Game::Get();

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
		{
			game.bow_instances.push_back(bow_instance);
			bow_instance = nullptr;
		}
		action = A_NONE;
		break;
	case A_DRINK:
		if(animation_state == 0)
		{
			if(Net::IsLocal())
				game.AddItem(*this, used_item, 1, used_item_is_team);
			if(mode != BREAK_ACTION_MODE::FALL)
				used_item = nullptr;
		}
		else
			used_item = nullptr;
		mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_EAT:
		if(animation_state < 2)
		{
			if(Net::IsLocal())
				game.AddItem(*this, used_item, 1, used_item_is_team);
			if(mode != BREAK_ACTION_MODE::FALL)
				used_item = nullptr;
		}
		else
			used_item = nullptr;
		mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		if(weapon_state == WS_HIDING)
		{
			if(animation_state == 0)
			{
				weapon_state = WS_TAKEN;
				weapon_taken = weapon_hiding;
				weapon_hiding = W_NONE;
			}
			else
			{
				weapon_state = WS_HIDDEN;
				weapon_taken = weapon_hiding = W_NONE;
			}
		}
		else
		{
			if(animation_state == 0)
			{
				weapon_state = WS_HIDDEN;
				weapon_taken = W_NONE;
			}
			else
				weapon_state = WS_TAKEN;
		}
		action = A_NONE;
		break;
	case A_BLOCK:
		mesh_inst->Deactivate(1);
		action = A_NONE;
		break;
	case A_DASH:
		if(animation_state == 1)
		{
			mesh_inst->Deactivate(1);
			mesh_inst->groups[1].blend_max = 0.33f;
		}
		break;
	}

	if(usable && !(player && player->action == PlayerController::Action_LootContainer))
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
			game.Unit_StopUsingUsable(L.GetContext(*this), *this, mode != BREAK_ACTION_MODE::FALL && notify);
			if(prev_used_item && slots[SLOT_WEAPON] == prev_used_item && !HaveShield())
			{
				weapon_state = WS_TAKEN;
				weapon_taken = W_ONE_HANDED;
				weapon_hiding = W_NONE;
			}
			else if(mode == BREAK_ACTION_MODE::FALL)
				used_item = prev_used_item;
			action = A_POSITION;
			animation_state = 0;
		}

		if(Net::IsLocal() && IsAI() && ai->idle_action != AIController::Idle_None)
		{
			ai->idle_action = AIController::Idle_None;
			ai->timer = Random(1.f, 2.f);
		}
	}
	else
	{
		if(!Any(action, A_ANIMATION, A_STAND_UP) || !allow_animation)
			action = A_NONE;
	}

	mesh_inst->frame_end_info = false;
	mesh_inst->frame_end_info2 = false;
	run_attack = false;

	if(IsPlayer())
	{
		player->next_action = NA_NONE;
		if(player == game.pc)
		{
			game.pc_data.action_ready = false;
			game.gui->inventory->lock = nullptr;
			if(game.gui->inventory->mode > I_INVENTORY)
				game.CloseInventory();

			if(player->action == PlayerController::Action_Talk)
			{
				if(Net::IsLocal())
				{
					player->action_unit->busy = Busy_No;
					player->action_unit->look_target = nullptr;
					player->dialog_ctx->dialog_mode = false;
				}
				else
					game.dialog_context.dialog_mode = false;
				look_target = nullptr;
				player->action = PlayerController::Action_None;
			}
		}
		else if(Net::IsLocal())
		{
			if(player->action == PlayerController::Action_Talk)
			{
				player->action_unit->busy = Busy_No;
				player->action_unit->look_target = nullptr;
				player->dialog_ctx->dialog_mode = false;
				look_target = nullptr;
				player->action = PlayerController::Action_None;
			}
		}
	}
	else if(Net::IsLocal())
	{
		ai->potion = -1;
		if(busy == Busy_Talking)
		{
			DialogContext* ctx = game.FindDialogContext(this);
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

		if(player && player->is_local)
			Game::Get().pc_data.before_player = BP_NONE;
	}
	else
	{
		// przerwij akcjê
		BreakAction(BREAK_ACTION_MODE::FALL);

		if(player && player->is_local)
		{
			raise_timer = Random(5.f, 7.f);
			Game::Get().pc_data.before_player = BP_NONE;
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
	Game& game = Game::Get();
	if(in_arena != -1 || game.death_screen != 0)
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
					UpdatePhysics(pos);
				}
				else
				{
					// sprawdŸ czy nie ma wrogów
					LevelContext& ctx = L.GetContext(*this);
					ok = true;
					for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
					{
						if((*it)->IsStanding() && IsEnemy(**it) && Vec3::Distance(pos, (*it)->pos) <= 20.f && L.CanSee(*this, **it))
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
	{
		Standup();

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::STAND_UP;
			c.unit = this;
		}
	}
}

//=================================================================================================
void Unit::Standup()
{
	HealPoison();
	live_state = ALIVE;
	Mesh::Animation* anim = mesh_inst->mesh->GetAnimation("wstaje2");
	if(anim)
	{
		mesh_inst->Play(anim, PLAY_ONCE | PLAY_PRIO3, 0);
		mesh_inst->groups[0].speed = 1.f;
		action = A_STAND_UP;
		animation = ANI_PLAY;
	}
	else
		action = A_NONE;
	used_item = nullptr;

	if(Net::IsLocal())
	{
		if(IsAI())
		{
			if(ai->state != AIController::Idle)
			{
				ai->state = AIController::Idle;
				ai->change_ai_mode = true;
			}
			ai->alert_target = nullptr;
			ai->idle_action = AIController::Idle_None;
			ai->target = nullptr;
			ai->timer = Random(2.f, 5.f);
		}

		L.WarpUnit(*this, pos);
	}
}

//=================================================================================================
void Unit::Die(LevelContext* ctx, Unit* killer)
{
	ACTION prev_action = action;
	Game& game = Game::Get();

	if(live_state == FALL)
	{
		// unit already on ground, add blood
		L.CreateBlood(*ctx, *this);
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
			AddItem(Item::gold, (uint)gold);
			gold = 0;
		}

		// mark if unit have important item in inventory
		for(ItemSlot& item : items)
		{
			if(item.item && IS_SET(item.item->flags, ITEM_IMPORTANT))
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
		for(vector<Unit*>::iterator it = ctx->units->begin(), end = ctx->units->end(); it != end; ++it)
		{
			if((*it)->IsPlayer() || !(*it)->IsStanding() || !IsFriend(**it))
				continue;

			if(Vec3::Distance(pos, (*it)->pos) <= 20.f && L.CanSee(*this, **it))
				(*it)->ai->morale -= 2.f;
		}

		// rising team members / check is location cleared
		if(IsTeamMember())
			raise_timer = Random(5.f, 7.f);
		else
			L.CheckIfLocationCleared();

		// event
		if(event_handler)
			event_handler->HandleUnitEvent(UnitEventHandler::DIE, this);
		for(Event& event : events)
		{
			if(event.type == EVENT_DIE)
			{
				ScriptEvent e(EVENT_DIE);
				e.unit = this;
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
		++GameStats::Get().total_kills;
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
				game.pc_data.before_player = BP_NONE;
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
					Team.AddExp(exp);
				}
			}
			else
				Game::Get().arena->RewardExp(this);
		}
	}
	else
	{
		hp = 0.f;

		// player rising
		if(player && player->is_local)
		{
			raise_timer = Random(5.f, 7.f);
			game.pc_data.before_player = BP_NONE;
		}
	}

	// end boss music
	if(IS_SET(data->flags2, F2_BOSS) && W.RemoveBossLevel(Int2(L.location_index, L.dungeon_level)))
		game.SetMusic();

	if(prev_action == A_ANIMATION)
	{
		action = A_NONE;
		current_animation = ANI_STAND;
	}
	animation = ANI_DIE;
	talking = false;
	mesh_inst->need_update = true;

	// sound
	SOUND snd = nullptr;
	if(data->sounds->Have(SOUND_DEATH))
		snd = data->sounds->Random(SOUND_DEATH)->sound;
	else if(data->sounds->Have(SOUND_PAIN))
		snd = data->sounds->Random(SOUND_PAIN)->sound;
	if(snd)
		PlaySound(snd, Unit::DIE_SOUND_DIST);

	// move physics
	UpdatePhysics(pos);
}

//=================================================================================================
void Unit::DropGold(int count)
{
	Game& game = Game::Get();

	gold -= count;
	game.sound_mgr->PlaySound2d(game.sCoins);

	// animacja wyrzucania
	action = A_ANIMATION;
	mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
	mesh_inst->groups[0].speed = 1.f;
	mesh_inst->frame_end_info = false;

	if(Net::IsLocal())
	{
		// stwórz przedmiot
		GroundItem* item = new GroundItem;
		item->item = Item::gold;
		item->count = count;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = Random(MAX_ANGLE);
		L.AddGroundItem(L.GetContext(*this), item);

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
	if(IS_SET(data->flags, F_AI_DRUNKMAN))
		return true;
	else if(IS_SET(data->flags3, F3_DRUNK_MAGE))
		return QM.quest_mages2->mages_state < Quest_Mages2::State::MageCured;
	else if(IS_SET(data->flags3, F3_DRUNKMAN_AFTER_CONTEST))
		return QM.quest_contest->state == Quest_Contest::CONTEST_DONE;
	else
		return false;
}

//=================================================================================================
void Unit::PlaySound(SOUND snd, float range)
{
	Game::Get().sound_mgr->PlaySound3d(snd, GetHeadSoundPos(), range);
}

//=================================================================================================
void Unit::CreatePhysics(bool position)
{
	btCapsuleShape* caps = new btCapsuleShape(GetUnitRadius(), max(MIN_H, GetUnitHeight()));
	cobj = new btCollisionObject;
	cobj->setCollisionShape(caps);
	cobj->setUserPointer(this);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_UNIT);

	if(position)
	{
		btVector3 bpos(ToVector3(IsAlive() ? pos : Vec3(1000, 1000, 1000)));
		bpos.setY(pos.y + max(MIN_H, GetUnitHeight()) / 2);
		cobj->getWorldTransform().setOrigin(bpos);
	}

	L.phy_world->addCollisionObject(cobj, CG_UNIT);
}

//=================================================================================================
void Unit::UpdatePhysics(const Vec3& pos)
{
	btVector3 a_min, a_max, bpos(ToVector3(IsAlive() ? pos : Vec3(1000, 1000, 1000)));
	bpos.setY(pos.y + max(MIN_H, GetUnitHeight()) / 2);
	cobj->getWorldTransform().setOrigin(bpos);
	L.phy_world->UpdateAabb(cobj);
}

//=================================================================================================
SOUND Unit::GetTalkSound() const
{
	if(data->sounds->Have(SOUND_TALK))
		return data->sounds->Random(SOUND_TALK)->sound;
	return nullptr;
}

//=================================================================================================
void Unit::SetWeaponState(bool takes_out, WeaponType co)
{
	if(takes_out)
	{
		switch(weapon_state)
		{
		case WS_HIDDEN:
			// wyjmij bron
			mesh_inst->Play(GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
			action = A_TAKE_WEAPON;
			weapon_taken = co;
			weapon_state = WS_TAKING;
			animation_state = 0;
			break;
		case WS_HIDING:
			if(weapon_hiding == co)
			{
				if(animation_state == 0)
				{
					// jeszcze nie schowa³ tej broni, wy³¹cz grupê
					action = A_NONE;
					weapon_taken = weapon_hiding;
					weapon_hiding = W_NONE;
					weapon_state = WS_TAKEN;
					mesh_inst->Deactivate(1);
				}
				else
				{
					// schowa³ broñ, zacznij wyci¹gaæ
					weapon_taken = weapon_hiding;
					weapon_hiding = W_NONE;
					weapon_state = WS_TAKING;
					CLEAR_BIT(mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
				}
			}
			else
			{
				// chowa broñ, zacznij wyci¹gaæ
				mesh_inst->Play(GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				action = A_TAKE_WEAPON;
				weapon_taken = co;
				weapon_hiding = W_NONE;
				weapon_state = WS_TAKING;
				animation_state = 0;
			}
			break;
		case WS_TAKING:
		case WS_TAKEN:
			if(weapon_taken != co)
			{
				// wyjmuje z³¹ broñ, zacznij wyjmowaæ dobr¹
				// lub
				// powinien mieæ wyjêt¹ broñ, ale nie t¹!
				mesh_inst->Play(GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				action = A_TAKE_WEAPON;
				weapon_taken = co;
				weapon_hiding = W_NONE;
				weapon_state = WS_TAKING;
				animation_state = 0;
			}
			break;
		}
	}
	else // chowa
	{
		switch(weapon_state)
		{
		case WS_HIDDEN:
			// schowana to schowana, nie ma co sprawdzaæ czy to ta
			break;
		case WS_HIDING:
			if(weapon_hiding != co)
			{
				// chowa z³¹ broñ, zamieñ
				weapon_hiding = co;
			}
			break;
		case WS_TAKING:
			if(animation_state == 0)
			{
				// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
				action = A_NONE;
				weapon_taken = W_NONE;
				weapon_state = WS_HIDDEN;
				mesh_inst->Deactivate(1);
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				weapon_hiding = weapon_taken;
				weapon_taken = W_NONE;
				weapon_state = WS_HIDING;
				animation_state = 0;
				SET_BIT(mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
			}
			break;
		case WS_TAKEN:
			// zacznij chowaæ
			mesh_inst->Play(GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
			weapon_hiding = co;
			weapon_taken = W_NONE;
			weapon_state = WS_HIDING;
			action = A_TAKE_WEAPON;
			animation_state = 0;
			break;
		}
	}
}

//=================================================================================================
void Unit::UpdateInventory(bool notify)
{
	assert(IsTeamMember()); // works only for team members!

	bool changes = false;
	int index = 0;
	const Item* prev_slots[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
		prev_slots[i] = slots[i];

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->team_count != 0)
			continue;

		switch(it->item->type)
		{
		case IT_WEAPON:
			if(!HaveWeapon())
			{
				slots[SLOT_WEAPON] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(IS_SET(data->flags, F_MAGE))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(GetWeapon().flags, ITEM_MAGE))
					{
						if(GetWeapon().value < it->item->value)
						{
							std::swap(slots[SLOT_WEAPON], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(GetWeapon().flags, ITEM_MAGE) && IsBetterWeapon(it->item->ToWeapon()))
					{
						std::swap(slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
			}
			else if(IsBetterWeapon(it->item->ToWeapon()))
			{
				std::swap(slots[SLOT_WEAPON], it->item);
				changes = true;
			}
			break;
		case IT_BOW:
			if(!HaveBow())
			{
				slots[SLOT_BOW] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(GetBow().value < it->item->value)
			{
				std::swap(slots[SLOT_BOW], it->item);
				changes = true;
			}
			break;
		case IT_ARMOR:
			if(!HaveArmor())
			{
				slots[SLOT_ARMOR] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(IS_SET(data->flags, F_MAGE))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(GetArmor().flags, ITEM_MAGE))
					{
						if(it->item->value > GetArmor().value)
						{
							std::swap(slots[SLOT_ARMOR], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(GetArmor().flags, ITEM_MAGE) && IsBetterArmor(it->item->ToArmor()))
					{
						std::swap(slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
			}
			else if(IsBetterArmor(it->item->ToArmor()))
			{
				std::swap(slots[SLOT_ARMOR], it->item);
				changes = true;
			}
			break;
		case IT_SHIELD:
			if(!HaveShield())
			{
				slots[SLOT_SHIELD] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(GetShield().value < it->item->value)
			{
				std::swap(slots[SLOT_SHIELD], it->item);
				changes = true;
			}
			break;
		case IT_AMULET:
			if(!HaveAmulet())
			{
				slots[SLOT_AMULET] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(GetAmulet().value < it->item->value)
			{
				std::swap(slots[SLOT_AMULET], it->item);
				changes = true;
			}
			break;
		default:
			break;
		}
	}

	if(changes)
	{
		RemoveNullItems(items);
		SortItems(items);

		if(Net::IsOnline() && N.active_players > 1 && notify)
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
bool Unit::IsEnemy(Unit &u, bool ignore_dont_attack) const
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
				return Team.IsBandit() || WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_CRAZIES)
		{
			if(g2 == G_CITIZENS)
				return true;
			else if(g2 == G_TEAM)
				return Team.crazies_attack || WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_TEAM)
		{
			if(u.WantAttackTeam())
				return true;
			else if(g2 == G_CITIZENS)
				return Team.IsBandit();
			else if(g2 == G_CRAZIES)
				return Team.crazies_attack;
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
bool Unit::IsFriend(Unit& u) const
{
	if(in_arena == -1 && u.in_arena == -1)
	{
		if(IsTeamMember())
		{
			if(u.IsTeamMember())
				return true;
			else if(u.IsAI() && !Team.IsBandit() && u.IsAssist())
				return true;
			else
				return false;
		}
		else if(u.IsTeamMember())
		{
			if(IsAI() && !Team.IsBandit() && IsAssist())
				return true;
			else
				return false;
		}
		else
			return (data->group == u.data->group);
	}
	else
		return in_arena == u.in_arena;
}

//=================================================================================================
void Unit::RefreshStock()
{
	assert(data->trader);

	bool refresh;
	int worldtime = W.GetWorldtime();
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
		if(!L.entering)
		{
			Game& game = Game::Get();
			for(ItemSlot& slot : stock->items)
				game.PreloadItem(slot.item);
		}
	}
}

//=================================================================================================
void Unit::AddDialog(Quest_Scripted* quest, GameDialog* dialog)
{
	assert(quest && dialog);
	dialogs.push_back({ dialog, quest });
	quest->AddDialogPtr(this);
}

//=================================================================================================
void Unit::AddDialogS(Quest_Scripted* quest, const string& dialog_id)
{
	GameDialog* dialog = quest->GetDialog(dialog_id);
	if(!dialog)
		throw ScriptException("Missing quest dialog '%s'.", dialog_id.c_str());
	AddDialog(quest, dialog);
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
		quest->RemoveDialogPtr(this);
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
	event.unit = this;
	quest->AddEventPtr(event);
}

//=================================================================================================
void Unit::RemoveEventHandler(Quest_Scripted* quest, bool cleanup)
{
	LoopAndRemove(events, [quest](Event& e)
	{
		return e.quest == quest;
	});

	if(!cleanup)
	{
		EventPtr event;
		event.source = EventPtr::UNIT;
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
		event.unit = this;
		e.quest->RemoveEventPtr(event);
	}
	events.clear();
}

//=================================================================================================
void Unit::OrderEscapeToUnit(Unit* unit)
{
	assert(unit);
	SetOrder(ORDER_ESCAPE_TO_UNIT);
	order_unit = unit;
	order_pos = unit->pos;
	if(ai)
	{
		ai->state = AIController::Escape;
		ai->escape_room = nullptr;
	}
}

//=================================================================================================
void Unit::SetOrder(UnitOrder order)
{
	this->order = order;
	order_timer = -1.f;
	if(ai)
	{
		ai->timer = 0.f;
		ai->idle_action = AIController::Idle_None;
		if(order == ORDER_WANDER)
			ai->loc_timer = Random(5.f, 10.f);
	}
}

//=================================================================================================
void Unit::OrderAttack()
{
	if(data->group == G_CRAZIES)
	{
		if(!Team.crazies_attack)
		{
			Team.crazies_attack = true;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_FLAGS;
			}
		}
	}
	else
	{
		LevelContext& ctx = L.GetContext(*this);
		for(Unit* unit : *ctx.units)
		{
			if(unit->dont_attack && unit->IsEnemy(*Team.leader, true) && !IS_SET(unit->data->flags, F_PEACEFUL))
			{
				unit->dont_attack = false;
				unit->ai->change_ai_mode = true;
			}
		}
	}
}

//=================================================================================================
void Unit::OrderClear()
{
	if(hero && hero->team_member)
		order = ORDER_FOLLOW;
	else
		order = ORDER_NONE;
	order_timer = 0.f;
}

//=================================================================================================
void Unit::OrderMove(const Vec3& pos, MoveType move_type)
{
	SetOrder(ORDER_MOVE);
	order_pos = pos;
	order_move_type = move_type;
}

//=================================================================================================
void Unit::OrderLookAt(const Vec3& pos)
{
	SetOrder(ORDER_LOOK_AT);
	order_pos = pos;
}

//=================================================================================================
void Unit::Talk(const string& text, int play_anim)
{
	int ani = 0;
	Game::Get().gui->game_gui->AddSpeechBubble(this, text.c_str());
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
		mesh_inst->groups[0].speed = 1.f;
		animation = ANI_PLAY;
		action = A_ANIMATION;
	}

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = this;
		c.str = StringPool.Get();
		*c.str = text;
		c.id = ani;
		c.count = 0;
		N.net_strs.push_back(c.str);
	}
}
