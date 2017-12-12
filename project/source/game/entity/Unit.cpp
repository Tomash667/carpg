#include "Pch.h"
#include "Core.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "Inventory.h"
#include "UnitHelper.h"
#include "QuestManager.h"
#include "AIController.h"
#include "Team.h"
#include "BitStreamFunc.h"

const float Unit::AUTO_TALK_WAIT = 0.333f;
const float Unit::STAMINA_BOW_ATTACK = 100.f;
const float Unit::STAMINA_BASH_ATTACK = 50.f;
const float Unit::STAMINA_UNARMED_ATTACK = 50.f;
const float Unit::STAMINA_RESTORE_TIMER = 1.f;
int Unit::netid_counter;

//=================================================================================================
Unit::~Unit()
{
	if(statsx && statsx->unique)
		delete statsx;
	delete mesh_inst;
	delete human_data;
	delete hero;
	delete player;
}

//=================================================================================================
float Unit::CalculateMaxHp() const
{
	float hp;
	if(IS_SET(data->flags3, F3_FIXED))
		hp = (float)data->hp;
	else
	{
		float end = (float)Get(Attribute::END);
		float ath = (float)Get(Skill::ATHLETICS);
		float lvl = IsPlayer() ? player->level : (float)level;
		hp = data->hp + (data->hp_bonus + (end - 50) / 5 + ath / 5) * (lvl + 2);
	}
	float effects_hp = GetEffectSum(EffectType::Health);
	hp += effects_hp;
	return hp;
}

//=================================================================================================
float Unit::CalculateMaxStamina() const
{
	float stamina;
	if(IS_SET(data->flags3, F3_FIXED))
		stamina = (float)data->stamina;
	else
		stamina = (float)(data->stamina + Get(Attribute::END) * 2 + Get(Skill::ACROBATICS));
	float effects_stamina = GetEffectSum(EffectType::Stamina);
	stamina += effects_stamina;
	return stamina;
}

//=================================================================================================
void Unit::CalculateLoad()
{
	weight_max = 200 + Get(Attribute::STR) * 10 + Get(Skill::ATHLETICS) * 5;
	float carry_mod = GetEffectModMultiply(EffectType::Carry);
	weight_max = int(carry_mod * weight_max);
}

//=================================================================================================
float Unit::CalculateAttack() const
{
	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->atk;
	if(HaveWeapon())
		return CalculateAttack(&GetWeapon());
	else
	{
		float atk = float(data->atk + data->atk_bonus * level);
		float mod = 1.f + 1.f / 100 * +Get(Skill::UNARMED);
		int str = Get(Attribute::STR),
			dex = Get(Attribute::DEX);
		atk += (0.5f * max(str - 50, 0) + 0.5f * max(dex - 50, 0)) * mod;
		float atk_bonus = GetEffectSum(EffectType::Attack);
		atk += atk_bonus;
		return atk;
	}
}

//=================================================================================================
float Unit::CalculateAttack(const Item* weapon) const
{
	assert(weapon);

	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->atk;

	float atk = float(data->atk + data->atk_bonus * level);
	int str = Get(Attribute::STR),
		dex = Get(Attribute::DEX);

	if(weapon->type == IT_WEAPON)
	{
		const Weapon& w = weapon->ToWeapon();
		const WeaponTypeInfo& wi = WeaponTypeInfo::info[w.weapon_type];
		int skill = Get(wi.skill);
		float p;
		if(str >= w.req_str)
			p = 1.f;
		else
			p = float(str) / w.req_str;
		atk += (w.dmg + wi.str2dmg * max(0, str - 50) + wi.dex2dmg * max(0, dex - 50)) * (p + skill / 100);
	}
	else
	{
		assert(weapon->type == IT_BOW);
		const Bow& bow = weapon->ToBow();
		int skill = Get(Skill::BOW);
		float p;
		if(str >= bow.req_str)
			p = 1.f;
		else
			p = float(str) / bow.req_str;
		atk += (bow.dmg + max(0, dex - 50)) * (p + skill / 100);
	}

	float atk_bonus = GetEffectSum(EffectType::Attack);
	atk += atk_bonus;

	return atk;
}

//=================================================================================================
float Unit::CalculateBlock(const Item* shield) const
{
	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->block;

	if(!shield)
		shield = slots[SLOT_SHIELD];
	if(!shield)
		return 0.f;

	assert(shield && shield->type == IT_SHIELD);
	const Shield& s = shield->ToShield();
	float p;
	int str = Get(Attribute::STR);
	if(str >= s.req_str)
		p = 1.f;
	else
		p = float(str) / s.req_str;

	return data->block + level * data->block_bonus + (max(0, str - 50) + s.block) * (1.f + 1.f / 100 * Get(Skill::SHIELD)) * p;
}

//=================================================================================================
float Unit::CalculateDefense(const Item* armor, const Item* shield) const
{
	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->def;

	// base
	float def = float(data->def + data->def_bonus * level);

	// endurance bonus
	float end = (float)Get(Attribute::END);
	def += (end - 50) / 5;

	// armor defense
	if(!armor)
		armor = slots[SLOT_ARMOR];
	if(armor)
	{
		assert(armor->type == IT_ARMOR);
		const Armor& a = armor->ToArmor();
		float skill = (float)Get(a.skill);
		def += a.def * (1.f + skill / 100);
	}

	// dexterity bonus
	auto load_state = GetArmorLoadState(armor);
	if(load_state < LS_HEAVY)
	{
		float dex = (float)Get(Attribute::DEX);
		float bonus = max(0.f, (dex - 50) / 5);
		if(load_state == LS_MEDIUM)
			bonus /= 2;
		def += bonus;
	}

	// shield defense
	if(!shield)
		shield = slots[SLOT_SHIELD];
	if(shield)
	{
		assert(shield->type == IT_SHIELD);
		const Shield& s = shield->ToShield();
		float skill = (float)Get(Skill::SHIELD);
		def += s.def * (1.f + skill / 200);
	}

	float def_bonus = GetEffectSum(EffectType::Defense);
	def += def_bonus;

	return def;
}

Unit::LoadState Unit::GetArmorLoadState(const Item* armor) const
{
	auto state = GetLoadState();
	if(armor)
	{
		auto skill = armor->ToArmor().skill;
		if(skill == Skill::HEAVY_ARMOR)
		{
			if(state < LS_HEAVY)
				state = LS_HEAVY;
		}
		else if(skill == Skill::MEDIUM_ARMOR)
		{
			if(state < LS_MEDIUM)
				state = LS_MEDIUM;
		}
	}
	return state;
}

//=================================================================================================
bool Unit::DropItem(int index)
{
	Game& game = Game::Get();
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
		if(!game.CheckMoonStone(item, *this))
			game.AddGroundItem(game.GetContext(*this), item);

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
		c.ile = 1;
	}

	return no_more;
}

//=================================================================================================
void Unit::DropItem(ITEM_SLOT slot)
{
	assert(slots[slot]);
	Game& game = Game::Get();
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
		game.AddGroundItem(game.GetContext(*this), item);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;

			NetChange& c2 = Add1(Net::changes);
			c2.unit = this;
			c2.id = slot;
			c2.type = NetChange::CHANGE_EQUIPMENT;
		}
	}
	else
	{
		item2 = nullptr;

		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_ITEM;
		c.id = SlotToIIndex(slot);
		c.ile = 1;
	}
}

//=================================================================================================
bool Unit::DropItems(int index, uint count)
{
	Game& game = Game::Get();
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
		game.AddGroundItem(game.GetContext(*this), item);

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
		c.ile = count;
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
					assert(Inventory::lock_id == LOCK_NO);
					player->next_action = NA_CONSUME;
					Inventory::lock_index = index;
					Inventory::lock_id = LOCK_MY;
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
			assert(Inventory::lock_id == LOCK_NO && player->is_local);
			player->next_action = NA_CONSUME;
			Inventory::lock_index = index;
			Inventory::lock_id = LOCK_MY;
		}
		else
			ai->potion = index;
		return 2;
	}

	ItemSlot& slot = items[index];

	assert(slot.item && slot.item->type == IT_CONSUMABLE);

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
			c.ile = 0;
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
			c.ile = (force ? 1 : 0);
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
	if(current_animation == ANI_PLAY)
	{
		animation = ANI_STAND;
		current_animation = ANI_NONE;
	}
	mesh_inst->Play(anim_name, PLAY_ONCE | PLAY_PRIO1, 1);
	used_item = &cons;
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
				Game::Get().AddGold(team_count);
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
		if(!IS_SET(data->flags, F_POISON_RES))
		{
			Effect* e = Effect::Get();
			e->effect = (item.effect == E_POISON ? EffectType::Poison : EffectType::Alcohol);
			e->time = item.time;
			e->value = -1;
			e->power = item.power / item.time;
			e->source = EffectSource::Potion;
			e->source_id = -1;
			e->refs = 1;
			AddEffect(e);
		}
		break;
	case E_REGENERATE:
	case E_NATURAL:
	case E_ANTIMAGIC:
	case E_STAMINA:
		{
			Effect* e = Effect::Get();
			switch(item.effect)
			{
			case E_REGENERATE:
				e->effect = EffectType::Regeneration;
				break;
			case E_NATURAL:
				e->effect = EffectType::NaturalHealingMod;
				break;
			case E_ANTIMAGIC:
				e->effect = EffectType::MagicResistance;
				break;
			case E_STAMINA:
				e->effect = EffectType::StaminaRegeneration;
				break;
			}
			e->time = item.time;
			e->value = -1;
			e->power = item.power;
			e->source = EffectSource::Potion;
			e->source_id = -1;
			e->refs = 1;
			AddEffect(e);
		}
		break;
	case E_ANTIDOTE:
		{
			uint index = 0;
			for(auto& e : effects)
			{
				if(e.effect == EffectType::Poison || e.effect == EffectType::Alcohol)
					_to_remove.push_back(index);
				++index;
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
		player->Train(PlayerController::TM_POTION, (int)Attribute::STR, false);
		break;
	case E_END:
		player->Train(PlayerController::TM_POTION, (int)Attribute::END, false);
		break;
	case E_DEX:
		player->Train(PlayerController::TM_POTION, (int)Attribute::DEX, false);
		break;
	case E_FOOD:
		{
			Effect* e = Effect::Get();
			e->effect = EffectType::FoodRegeneration;
			e->time = item.power;
			e->power = 1.f;
			e->source = EffectSource::Potion;
			e->source_id = -1;
			e->refs = 1;
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
void Unit::UpdateEffects(float dt)
{
	EffectSumBuffer reg, stamina_reg;
	float food_heal = 0.f, poison_dmg = 0.f, alco_sum = 0.f;

	// update effects timer
	uint index = 0;
	for(auto& e : effects)
	{
		if(e.effect == EffectType::NaturalHealingMod) // timer is in days
		{
			++index;
			continue;
		}

		switch(e.effect)
		{
		case EffectType::Regeneration:
			reg += e;
			break;
		case EffectType::Poison:
			poison_dmg += e.power;
			break;
		case EffectType::Alcohol:
			alco_sum += e.power;
			break;
		case EffectType::FoodRegeneration:
			food_heal += dt;
			break;
		case EffectType::StaminaRegeneration:
			stamina_reg += e;
			break;
		}

		if(e.IsTimed())
		{
			if((e.time -= dt) <= 0.f)
				_to_remove.push_back(index);
		}

		++index;
	}

	// remove expired effects
	RemoveEffects(false);

	if(Net::IsClient())
		return;

	// health regeneration
	if(hp != hpmax && (reg != 0.f || stamina_reg != 0.f))
	{
		float natural = GetNaturalHealingMod();
		hp += reg * dt + food_heal * natural;
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
		alcohol += alco_sum*dt;
		if(alcohol >= hpmax && live_state == ALIVE)
			Game::Get().UnitFall(*this);
		if(IsPlayer() && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_ALCOHOL;
	}
	else if(alcohol != 0.f)
	{
		alcohol -= dt / 10 * Get(Attribute::END);
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_ALCOHOL;
	}

	// update poison damage
	if(poison_dmg != 0.f)
		Game::Get().GiveDmg(Game::Get().GetContext(*this), nullptr, poison_dmg * dt, *this, nullptr, Game::DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(Net::IsOnline() && !player->is_local && player->last_dmg_poison != poison_dmg)
			player->player_info->update_flags |= PlayerInfo::UF_POISON_DAMAGE;
		player->last_dmg_poison = poison_dmg;
	}

	// restore stamina
	if(stamina_timer > 0)
		stamina_timer -= dt;
	else if(stamina != stamina_max && (stamina_action != SA_DONT_RESTORE || stamina_reg > 0.f))
	{
		float stamina_restore;
		switch(stamina_action)
		{
		case SA_RESTORE_MORE:
			stamina_restore = 30.f;
			break;
		case SA_RESTORE:
		default:
			stamina_restore = 20.f;
			break;
		case SA_RESTORE_SLOW:
			stamina_restore = 15.f;
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
		stamina += ((stamina_max * stamina_restore / 100) + stamina_reg) * dt;
		if(stamina > stamina_max)
			stamina = stamina_max;
		if(Net::IsServer() && player && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
	}
}

//=================================================================================================
void Unit::EndEffects()
{
	alcohol = 0.f;
	stamina = stamina_max;
	stamina_timer = 0;

	if(effects.empty())
		return;

	// end effects
	uint index = 0;
	EffectSumBuffer reg;
	EffectMulBuffer natural;
	float food = 0.f;
	for(auto& e : effects)
	{
		switch(e.effect)
		{
		case EffectType::Regeneration:
			e.power *= e.time;
			reg += e;
			break;
		case EffectType::Poison:
			hp -= e.power * e.time;
			break;
		case EffectType::FoodRegeneration:
			food += e.time;
			break;
		case EffectType::NaturalHealingMod:
			natural *= e;
			break;
		default:
			break;
		}

		if(e.IsTimed() && e.effect != EffectType::NaturalHealingMod)
			_to_remove.push_back(index);

		++index;
	}

	if(hp != hpmax)
	{
		hp += reg + food * natural;
		if(hp < 1.f)
			hp = 1.f;
		else if(hp > hpmax)
			hp = hpmax;

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
	}

	RemoveEffects();
}

//=================================================================================================
void Unit::EndLongEffects(int days)
{
	bool any = false;
	uint index = 0;
	for(auto& e : effects)
	{
		if(e.IsTimed())
		{
			if((e.time -= days) <= 0.f && Net::IsLocal())
				_to_remove.push_back(index);
			any = true;
		}
		++index;
	}

	if(any && Net::IsServer() && IsPlayer() && !player->is_local)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::UPDATE_LONG_EFFECTS;
		c.id = days;
	}

	RemoveEffects();
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
		}

		if(count)
			AddItem(item, count, count);
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
		return pos;

	const Mesh::Point& point = *point2;
	Matrix matBone = point.mat * mesh_inst->mat_bones[point.bone];
	matBone = matBone * (Matrix::RotationY(rot) * Matrix::Translation(pos));
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
bool Unit::IsBetterWeapon(const Weapon& weapon) const
{
	if(!HaveWeapon())
		return true;

	return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(weapon);
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
bool Unit::IsBetterArmor(const Armor& armor) const
{
	if(!HaveArmor())
		return true;

	return CalculateDefense() < CalculateDefense(&armor, nullptr);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& armor, int* value) const
{
	if(!HaveArmor())
	{
		if(value)
			*value = (int)CalculateDefense(&armor, nullptr);
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&armor, nullptr);
		*value = (int)v;
		return CalculateDefense() < v;
	}
	else
		return CalculateDefense() < CalculateDefense(&armor, nullptr);
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
void Unit::RecalculateStamina()
{
	float p = stamina / stamina_max;
	stamina_max = CalculateMaxStamina();
	stamina = stamina_max * p;
}

//=================================================================================================
float Unit::CalculateShieldAttack() const
{
	assert(HaveShield());

	const Shield& s = GetShield();
	float p;

	int str = Get(Attribute::STR);
	if(str >= s.req_str)
		p = 1.f;
	else
		p = float(str) / s.req_str;

	return 0.5f * str / 2 + 0.25f * Get(Attribute::DEX) + (s.def * p * (1.f + float(Get(Skill::SHIELD)) / 200));
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
		case WT_LONG:
			a = A_LONG_BLADE;
			break;
		case WT_SHORT:
			a = A_SHORT_BLADE;
			break;
		case WT_MACE:
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
void Unit::Save(HANDLE file, bool local)
{
	// id postaci
	WriteString1(file, data->id);

	// przedmioty
	for(uint i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i])
			WriteString1(file, slots[i]->id);
		else
		{
			byte zero = 0;
			WriteFile(file, &zero, sizeof(zero), &tmp, nullptr);
		}
	}
	uint ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item)
		{
			WriteString1(file, it->item->id);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
			WriteFile(file, &it->team_count, sizeof(it->team_count), &tmp, nullptr);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, nullptr);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		}
	}

	WriteFile(file, &live_state, sizeof(live_state), &tmp, nullptr);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &hp, sizeof(hp), &tmp, nullptr);
	WriteFile(file, &hpmax, sizeof(hpmax), &tmp, nullptr);
	WriteFile(file, &stamina, sizeof(stamina), &tmp, nullptr);
	WriteFile(file, &stamina_max, sizeof(stamina_max), &tmp, nullptr);
	WriteFile(file, &stamina_action, sizeof(stamina_action), &tmp, nullptr);
	WriteFile(file, &stamina_timer, sizeof(stamina_timer), &tmp, nullptr);
	WriteFile(file, &level, sizeof(level), &tmp, nullptr);
	FileWriter f(file);
	statsx->Save(f);
	WriteFile(file, &gold, sizeof(gold), &tmp, nullptr);
	WriteFile(file, &invisible, sizeof(invisible), &tmp, nullptr);
	WriteFile(file, &in_building, sizeof(in_building), &tmp, nullptr);
	WriteFile(file, &to_remove, sizeof(to_remove), &tmp, nullptr);
	WriteFile(file, &temporary, sizeof(temporary), &tmp, nullptr);
	WriteFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
	WriteFile(file, &assist, sizeof(assist), &tmp, nullptr);
	f << auto_talk;
	if(auto_talk != AutoTalkMode::No)
	{
		f << (auto_talk_dialog ? auto_talk_dialog->id.c_str() : "");
		f << auto_talk_timer;
	}
	WriteFile(file, &dont_attack, sizeof(dont_attack), &tmp, nullptr);
	WriteFile(file, &attack_team, sizeof(attack_team), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
	int unit_event_handler_quest_refid = (event_handler ? event_handler->GetUnitEventHandlerQuestRefid() : -1);
	WriteFile(file, &unit_event_handler_quest_refid, sizeof(unit_event_handler_quest_refid), &tmp, nullptr);
	WriteFile(file, &weight, sizeof(weight), &tmp, nullptr);
	int guard_refid = (guard_target ? guard_target->refid : -1);
	WriteFile(file, &guard_refid, sizeof(guard_refid), &tmp, nullptr);
	int summoner_refid = (summoner ? summoner->refid : -1);
	WriteFile(file, &summoner_refid, sizeof(summoner_refid), &tmp, nullptr);

	assert((human_data != nullptr) == (data->type == UNIT_TYPE::HUMAN));
	if(human_data)
	{
		byte b = 1;
		WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		human_data->Save(file);
	}
	else
	{
		byte b = 0;
		WriteFile(file, &b, sizeof(b), &tmp, nullptr);
	}

	if(local)
	{
		mesh_inst->Save(file);
		WriteFile(file, &animation, sizeof(animation), &tmp, nullptr);
		WriteFile(file, &current_animation, sizeof(current_animation), &tmp, nullptr);

		WriteFile(file, &prev_pos, sizeof(prev_pos), &tmp, nullptr);
		WriteFile(file, &speed, sizeof(speed), &tmp, nullptr);
		WriteFile(file, &prev_speed, sizeof(prev_speed), &tmp, nullptr);
		WriteFile(file, &animation_state, sizeof(animation_state), &tmp, nullptr);
		WriteFile(file, &attack_id, sizeof(attack_id), &tmp, nullptr);
		WriteFile(file, &action, sizeof(action), &tmp, nullptr);
		WriteFile(file, &weapon_taken, sizeof(weapon_taken), &tmp, nullptr);
		WriteFile(file, &weapon_hiding, sizeof(weapon_hiding), &tmp, nullptr);
		WriteFile(file, &weapon_state, sizeof(weapon_state), &tmp, nullptr);
		WriteFile(file, &hitted, sizeof(hitted), &tmp, nullptr);
		WriteFile(file, &hurt_timer, sizeof(hurt_timer), &tmp, nullptr);
		WriteFile(file, &target_pos, sizeof(target_pos), &tmp, nullptr);
		WriteFile(file, &target_pos2, sizeof(target_pos2), &tmp, nullptr);
		WriteFile(file, &talking, sizeof(talking), &tmp, nullptr);
		WriteFile(file, &talk_timer, sizeof(talk_timer), &tmp, nullptr);
		WriteFile(file, &attack_power, sizeof(attack_power), &tmp, nullptr);
		WriteFile(file, &run_attack, sizeof(run_attack), &tmp, nullptr);
		WriteFile(file, &timer, sizeof(timer), &tmp, nullptr);
		WriteFile(file, &alcohol, sizeof(alcohol), &tmp, nullptr);
		WriteFile(file, &raise_timer, sizeof(raise_timer), &tmp, nullptr);

		if(action == A_DASH)
			WriteFile(file, &use_rot, sizeof(use_rot), &tmp, nullptr);

		if(used_item)
		{
			WriteString1(file, used_item->id);
			WriteFile(file, &used_item_is_team, sizeof(used_item_is_team), &tmp, nullptr);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		}

		if(usable)
		{
			if(usable->user != this)
			{
				Warn("Invalid usable %s (%d) user %s.", usable->base->id.c_str(), usable->refid, data->id.c_str());
				usable = nullptr;
				int refi = -1;
				WriteFile(file, &refi, sizeof(refi), &tmp, nullptr);
			}
			else
				WriteFile(file, &usable->refid, sizeof(usable->refid), &tmp, nullptr);
		}
		else
		{
			int refi = -1;
			WriteFile(file, &refi, sizeof(refi), &tmp, nullptr);
		}

		WriteFile(file, &last_bash, sizeof(last_bash), &tmp, nullptr);
		WriteFile(file, &moved, sizeof(moved), &tmp, nullptr);
	}

	// efekty
	ile = effects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(auto& e : effects)
		WriteFile(file, &e, sizeof(Effect), &tmp, nullptr);

	if(player)
	{
		byte b = 1;
		WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		player->Save(file);
	}
	else
	{
		byte b = 0;
		WriteFile(file, &b, sizeof(b), &tmp, nullptr);
	}

	if(hero)
		hero->Save(file);
}

//=================================================================================================
void Unit::Load(HANDLE file, bool local)
{
	human_data = nullptr;

	// unit data
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, nullptr);
	data = UnitData::Get(BUF);

	// items
	bool can_sort = true;
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		ReadString1(file);
		slots[i] = (BUF[0] ? Item::Get(BUF) : nullptr);
	}
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	items.resize(ile);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		ReadString1(file);
		ReadFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
		ReadFile(file, &it->team_count, sizeof(it->team_count), &tmp, nullptr);
		if(BUF[0] != '$')
			it->item = Item::Get(BUF);
		else
		{
			int quest_item_refid;
			ReadFile(file, &quest_item_refid, sizeof(quest_item_refid), &tmp, nullptr);
			QuestManager::Get().AddQuestItemRequest(&it->item, BUF, quest_item_refid, &items, this);
			it->item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	// stats
	ReadFile(file, &live_state, sizeof(live_state), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_20 && live_state != ALIVE)
		live_state = LiveState(live_state + 2); // kolejnoœæ siê zmieni³a
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &hp, sizeof(hp), &tmp, nullptr);
	ReadFile(file, &hpmax, sizeof(hpmax), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_5)
	{
		ReadFile(file, &stamina, sizeof(stamina), &tmp, nullptr);
		ReadFile(file, &stamina_max, sizeof(stamina_max), &tmp, nullptr);
		ReadFile(file, &stamina_action, sizeof(stamina_action), &tmp, nullptr);
		if(LOAD_VERSION >= V_CURRENT)
			ReadFile(file, &stamina_timer, sizeof(stamina_timer), &tmp, nullptr);
		else
			stamina_timer = 0;
	}
	if(LOAD_VERSION < V_0_5)
	{
		int old_type;
		ReadFile(file, &old_type, sizeof(old_type), &tmp, nullptr);
	}

	ReadFile(file, &level, sizeof(level), &tmp, nullptr);
	if(LOAD_VERSION < V_CURRENT)
	{
		// due to bug evil boss have random level 20-120 (with same stats)
		if(data->id == "q_zlo_boss")
			level = 20;
	}

	FileReader f(file);
	if(LOAD_VERSION >= V_CURRENT)
		statsx = StatsX::Load(f);
	else if(LOAD_VERSION >= V_0_4)
	{
		statsx = new StatsX;
		// current stats (there was no modifiers then so current == base)
		f.Skip(sizeof(old::UnitStats));
		// base stats
		f >> statsx->attrib;
		int old_skill[(int)old::v1::Skill::MAX];
		f >> old_skill;
		statsx->skill[(int)Skill::ONE_HANDED_WEAPON] = old_skill[(int)old::v1::Skill::ONE_HANDED_WEAPON];
		statsx->skill[(int)Skill::SHORT_BLADE] = old_skill[(int)old::v1::Skill::SHORT_BLADE];
		statsx->skill[(int)Skill::LONG_BLADE] = old_skill[(int)old::v1::Skill::LONG_BLADE];
		statsx->skill[(int)Skill::BLUNT] = old_skill[(int)old::v1::Skill::BLUNT];
		statsx->skill[(int)Skill::AXE] = old_skill[(int)old::v1::Skill::AXE];
		statsx->skill[(int)Skill::BOW] = old_skill[(int)old::v1::Skill::BOW];
		statsx->skill[(int)Skill::SHIELD] = old_skill[(int)old::v1::Skill::SHIELD];
		statsx->skill[(int)Skill::LIGHT_ARMOR] = old_skill[(int)old::v1::Skill::LIGHT_ARMOR];
		statsx->skill[(int)Skill::MEDIUM_ARMOR] = old_skill[(int)old::v1::Skill::MEDIUM_ARMOR];
		statsx->skill[(int)Skill::HEAVY_ARMOR] = old_skill[(int)old::v1::Skill::HEAVY_ARMOR];
		statsx->skill[(int)Skill::UNARMED] = -1;
		statsx->skill[(int)Skill::ATHLETICS] = -1;
		statsx->skill[(int)Skill::ACROBATICS] = -1;
		statsx->skill[(int)Skill::HAGGLE] = -1;
		statsx->skill[(int)Skill::LITERACY] = -1;
		statsx->seed.level = level;
	}
	else
	{
		statsx = new StatsX;
		for(int i = 0; i < 3; ++i)
			f >> statsx->attrib[i];
		for(int i = 3; i < 6; ++i)
			statsx->attrib[i] = -1;
		for(int i = 0; i < (int)Skill::MAX; ++i)
			statsx->skill[i] = -1;
		int old_skill[(int)old::v0::Skill::MAX];
		f >> old_skill;
		statsx->skill[(int)Skill::ONE_HANDED_WEAPON] = old_skill[(int)old::v0::Skill::WEAPON];
		statsx->skill[(int)Skill::BOW] = old_skill[(int)old::v0::Skill::BOW];
		statsx->skill[(int)Skill::SHIELD] = old_skill[(int)old::v0::Skill::SHIELD];
		statsx->skill[(int)Skill::LIGHT_ARMOR] = old_skill[(int)old::v0::Skill::LIGHT_ARMOR];
		statsx->skill[(int)Skill::HEAVY_ARMOR] = old_skill[(int)old::v0::Skill::HEAVY_ARMOR];
		statsx->seed.level = -1;
	}
	ReadFile(file, &gold, sizeof(gold), &tmp, nullptr);
	ReadFile(file, &invisible, sizeof(invisible), &tmp, nullptr);
	ReadFile(file, &in_building, sizeof(in_building), &tmp, nullptr);
	ReadFile(file, &to_remove, sizeof(to_remove), &tmp, nullptr);
	ReadFile(file, &temporary, sizeof(temporary), &tmp, nullptr);
	ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_20)
	{
		// w nowszych wersjach nie ma tej zmiennej, alkohol dzia³a inaczej
		bool niesmierc;
		ReadFile(file, &niesmierc, sizeof(niesmierc), &tmp, nullptr);
	}
	ReadFile(file, &assist, sizeof(assist), &tmp, nullptr);

	// auto talking
	if(LOAD_VERSION >= V_0_5)
	{
		f >> auto_talk;
		if(auto_talk != AutoTalkMode::No)
		{
			f.ReadStringBUF();
			if(BUF[0])
				auto_talk_dialog = FindDialog(BUF);
			else
				auto_talk_dialog = nullptr;
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

	ReadFile(file, &dont_attack, sizeof(dont_attack), &tmp, nullptr);
	ReadFile(file, &attack_team, sizeof(attack_team), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
	int unit_event_handler_quest_refid;
	ReadFile(file, &unit_event_handler_quest_refid, sizeof(unit_event_handler_quest_refid), &tmp, nullptr);
	if(unit_event_handler_quest_refid == -2)
		event_handler = &Game::Get();
	else if(unit_event_handler_quest_refid == -1)
		event_handler = nullptr;
	else
	{
		event_handler = (UnitEventHandler*)unit_event_handler_quest_refid;
		Game::Get().load_unit_handler.push_back(this);
	}
	CalculateLoad();
	if(can_sort && LOAD_VERSION < V_0_2_20)
		SortItems(items);
	ReadFile(file, &weight, sizeof(weight), &tmp, nullptr);
	RecalculateWeight();

	int guard_refid;
	ReadFile(file, &guard_refid, sizeof(guard_refid), &tmp, nullptr);
	if(guard_refid == -1)
		guard_target = nullptr;
	else
		AddRequest(&guard_target, guard_refid);

	if(LOAD_VERSION >= V_0_5)
	{
		int summoner_refid;
		ReadFile(file, &summoner_refid, sizeof(summoner_refid), &tmp, nullptr);
		if(summoner_refid == -1)
			summoner = nullptr;
		else
			AddRequest(&summoner, summoner_refid);
	}
	else
		summoner = nullptr;

	bubble = nullptr; // ustawianie przy wczytaniu SpeechBubble
	changed = false;
	busy = Busy_No;
	visual_pos = pos;

	byte b;
	ReadFile(file, &b, sizeof(b), &tmp, nullptr);
	if(b == 1)
	{
		human_data = new Human;
		human_data->Load(file);
	}
	else
	{
		assert(data->type != UNIT_TYPE::HUMAN);
		human_data = nullptr;
	}

	if(local)
	{
		CreateMesh(CREATE_MESH::LOAD);
		mesh_inst->Load(file);
		ReadFile(file, &animation, sizeof(animation), &tmp, nullptr);
		ReadFile(file, &current_animation, sizeof(current_animation), &tmp, nullptr);

		ReadFile(file, &prev_pos, sizeof(prev_pos), &tmp, nullptr);
		ReadFile(file, &speed, sizeof(speed), &tmp, nullptr);
		ReadFile(file, &prev_speed, sizeof(prev_speed), &tmp, nullptr);
		ReadFile(file, &animation_state, sizeof(animation_state), &tmp, nullptr);
		ReadFile(file, &attack_id, sizeof(attack_id), &tmp, nullptr);
		ReadFile(file, &action, sizeof(action), &tmp, nullptr);
		if(LOAD_VERSION < V_0_2_20 && action >= A_EAT)
			action = ACTION(action + 1);
		ReadFile(file, &weapon_taken, sizeof(weapon_taken), &tmp, nullptr);
		ReadFile(file, &weapon_hiding, sizeof(weapon_hiding), &tmp, nullptr);
		ReadFile(file, &weapon_state, sizeof(weapon_state), &tmp, nullptr);
		ReadFile(file, &hitted, sizeof(hitted), &tmp, nullptr);
		ReadFile(file, &hurt_timer, sizeof(hurt_timer), &tmp, nullptr);
		ReadFile(file, &target_pos, sizeof(target_pos), &tmp, nullptr);
		ReadFile(file, &target_pos2, sizeof(target_pos2), &tmp, nullptr);
		ReadFile(file, &talking, sizeof(talking), &tmp, nullptr);
		ReadFile(file, &talk_timer, sizeof(talk_timer), &tmp, nullptr);
		ReadFile(file, &attack_power, sizeof(attack_power), &tmp, nullptr);
		ReadFile(file, &run_attack, sizeof(run_attack), &tmp, nullptr);
		ReadFile(file, &timer, sizeof(timer), &tmp, nullptr);
		if(LOAD_VERSION >= V_0_2_20)
		{
			ReadFile(file, &alcohol, sizeof(alcohol), &tmp, nullptr);
			ReadFile(file, &raise_timer, sizeof(raise_timer), &tmp, nullptr);
		}
		else
		{
			alcohol = 0.f;
			if(action == A_ANIMATION2 && animation_state > AS_ANIMATION2_MOVE_TO_OBJECT)
				++animation_state;
			raise_timer = timer;
		}

		if (action == A_DASH)
			ReadFile(file, &use_rot, sizeof(use_rot), &tmp, nullptr);

		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, nullptr);
			used_item = Item::Get(BUF);
			ReadFile(file, &used_item_is_team, sizeof(used_item_is_team), &tmp, nullptr);
		}
		else
			used_item = nullptr;

		int refi;
		ReadFile(file, &refi, sizeof(refi), &tmp, nullptr);
		if(refi == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, refi, this);

		if(action == A_SHOOT)
		{
			bow_instance = Game::Get().GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = mesh_inst->groups[1].speed;
			bow_instance->groups[0].time = mesh_inst->groups[1].time;
		}

		ReadFile(file, &last_bash, sizeof(last_bash), &tmp, nullptr);
		if(LOAD_VERSION >= V_0_5)
			ReadFile(file, &moved, sizeof(moved), &tmp, nullptr);
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
	}

	// effects
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	effects.reserve(ile);
	if(ile)
	{
		if(LOAD_VERSION >= V_CURRENT)
		{
			for(uint i = 0; i < ile; ++i)
			{
				auto& e = effects.add();
				ReadFile(file, &e, sizeof(Effect), &tmp, nullptr);
				e.refs = 1;
			}
		}
		else
		{
			for(uint i = 0; i < ile; ++i)
			{
				auto& e = effects.add();

				ConsumeEffect effect;
				e.source = EffectSource::Potion;
				e.source_id = -1;
				e.value = -1;
				ReadFile(file, &effect, sizeof(effect), &tmp, nullptr);
				ReadFile(file, &e.time, sizeof(e.time), &tmp, nullptr);
				ReadFile(file, &e.power, sizeof(e.power), &tmp, nullptr);
				switch(effect)
				{
				case E_REGENERATE:
					e.effect = EffectType::Regeneration;
					break;
				case E_NATURAL:
					e.effect = EffectType::NaturalHealingMod;
					break;
				case E_POISON:
					e.effect = EffectType::Poison;
					break;
				case E_ALCOHOL:
					e.effect = EffectType::Alcohol;
					break;
				case E_ANTIMAGIC:
					e.effect = EffectType::MagicResistance;
					e.power = 0.5f;
					break;
				case E_FOOD:
					e.effect = EffectType::FoodRegeneration;
					break;
				case E_STAMINA:
					e.effect = EffectType::StaminaRegeneration;
					break;
				case E_STUN:
					e.effect = EffectType::Stun;
					e.source = EffectSource::Action;
					break;
				}
				e.netid = Effect::netid_counter++;
			}
		}
	}

	ReadFile(file, &b, sizeof(b), &tmp, nullptr);
	if(b == 1)
	{
		player = new PlayerController;
		player->unit = this;
		player->Load(file);
	}
	else
		player = nullptr;

	if(local && human_data)
		human_data->ApplyScale(mesh_inst->mesh);

	if(IS_SET(data->flags, F_HERO))
	{
		hero = new HeroData;
		hero->unit = this;
		hero->Load(file);
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
		Game::Get().CreateUnitPhysics(*this, true);
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

	// calculate new attributes
	if(LOAD_VERSION < V_CURRENT)
	{
		if(IsPlayer())
		{
			statsx->profile = data->stat_profile;
			statsx->Upgrade();
			player->SetRequiredPoints();
			player->RecalculateLevel(true);
			player->perk_points = level / 2;

			// rescale skill points counter
			for(uint i = 0; i < (uint)Skill::MAX; ++i)
			{
				auto& train_data = player->skill_pts[i];
				if(train_data.value > 0)
				{
					int old_required = old::GetRequiredSkillPoints(GetBase((Skill)i));
					float ratio = float(train_data.next) / old_required;
					train_data.value = (int)(ratio * train_data.value);
				}
			}
		}
		else
		{
			delete statsx;
			statsx = StatsX::GetRandom(&data->GetStatProfile(), level);
		}
		CalculateStats();
	}
	if(LOAD_VERSION < V_0_5)
	{
		stamina_max = CalculateMaxStamina();
		stamina = stamina_max;
		stamina_action = SA_RESTORE_MORE;
		stamina_timer = 0;
	}
}

//=================================================================================================
Effect* Unit::FindEffect(EffectType effect)
{
	for(Effect& e : effects)
	{
		if(e.effect == effect)
			return &e;
	}

	return nullptr;
}

//=================================================================================================
bool Unit::FindEffect(EffectType effect, float* value)
{
	Effect* top = nullptr;
	float topv = 0.f;

	for(auto& e : effects)
	{
		if(e.effect == effect && e.power > topv)
		{
			top = &e;
			topv = e.power;
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
	if(data->type == UNIT_TYPE::HUMANOID && !HaveWeapon())
		AddItemAndEquipIfNone(UnitHelper::GetBaseWeapon(*this));
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
			return;
		}
	}

	assert(0 && "Nie znalaziono questowego przedmiotu do usuniêcia!");
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
	if(IS_SET(data->flags3, F3_FIXED))
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

		// max 0.25 at
		//	125 dex for short blade (0.02)
		//	150 dex for long blade (0.016)
		//	175 dex for axe (0.014)
		//	200 dex for blunt (0.0125)
		float dex_mod = min(0.25f, info.dex_speed * Get(Attribute::DEX));
		mod += float(Get(info.skill)) / 200 + dex_mod - GetAttackSpeedModFromStrength(*wep);
		base_speed = info.base_speed;
	}
	else
	{
		float dex_mod = min(0.25f, 0.02f * Get(Attribute::DEX));
		mod += float(Get(Skill::UNARMED)) / 200 + dex_mod;
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
	// values range
	//	1 dex, 0 skill = 0.8
	// 50 dex, 10 skill = 1.1
	// 100 dex, 100 skill = 1.7
	float mod = 0.8f + float(Get(Skill::BOW)) / 200 + 0.004f*Get(Attribute::DEX) - GetAttackSpeedModFromStrength(GetBow());

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
void Unit::HealPoison()
{
	if(hp < 1.f)
		hp = 1.f;

	uint index = 0;
	for(auto& e : effects)
	{
		if(e.effect == EffectType::Poison)
			_to_remove.push_back(index);
		++index;
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
// currently using this on pc, looted units is not written
void Unit::RemoveItem(int iindex, bool active_location)
{
	assert(!player);
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
		if(active_location)
		{
			NetChange& c = Add1(Net::changes);
			c.unit = this;
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.id = s;
		}
	}
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
		player->ostatnia = W_NONE;
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
bool Unit::IsBetterItem(const Item* item) const
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return IsBetterWeapon(item->ToWeapon());
	case IT_ARMOR:
		return IsBetterArmor(item->ToArmor());
	case IT_SHIELD:
		if(HaveShield())
			return item->value > GetShield().value;
		else
			return true;
	case IT_BOW:
		if(HaveBow())
			return item->value > GetBow().value;
		else
			return true;
	default:
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
		int ile = 0;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
				++ile;
		}
		return ile;
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
	if(IS_SET(data->flags2, F2_MAGIC_RES25))
		mres = 0.75f;
	else if(IS_SET(data->flags2, F2_MAGIC_RES50))
		mres = 0.5f;
	if(HaveArmor())
	{
		const Armor& a = GetArmor();
		if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
		else if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
	}
	if(HaveShield())
	{
		const Shield& s = GetShield();
		if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
		else if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
	}
	float effects_mres = GetEffectModMultiply(EffectType::MagicResistance);
	mres *= effects_mres;
	return mres;
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
bool Unit::HaveEffect(EffectType effect) const
{
	for(auto& e : effects)
	{
		if(e.effect == effect)
			return true;
	}
	return false;
}

//=================================================================================================
bool Unit::HaveEffect(int netid) const
{
	for(auto& e : effects)
	{
		if(e.netid == netid)
			return true;
	}
	return false;
}

//=================================================================================================
int Unit::GetBuffs() const
{
	int b = 0;

	for(auto& e : effects)
	{
		// don't show perk effects
		if(e.source == EffectSource::Perk)
			continue;

		switch(e.effect)
		{
		case EffectType::Poison:
			b |= BUFF_POISON;
			break;
		case EffectType::Alcohol:
			b |= BUFF_ALCOHOL;
			break;
		case EffectType::Regeneration:
			b |= BUFF_REGENERATION;
			break;
		case EffectType::FoodRegeneration:
			b |= BUFF_FOOD;
			break;
		case EffectType::NaturalHealingMod:
			b |= BUFF_NATURAL;
			break;
		case EffectType::MagicResistance:
			b |= BUFF_ANTIMAGIC;
			break;
		case EffectType::StaminaRegeneration:
			b |= BUFF_STAMINA;
			break;
		case EffectType::Stun:
			b |= BUFF_STUN;
			break;
		}
	}

	if(alcohol / hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

//=================================================================================================
int Unit::Get(Attribute a, StatState& state) const
{
	int value = statsx->Get(a);
	float effect_mod = GetEffectSum(EffectType::Attribute, (int)a, state);
	value += (int)effect_mod;
	value = Clamp(value, AttributeInfo::MIN, AttributeInfo::MAX);
	return value;
}

//=================================================================================================
int Unit::Get(Skill s, StatState& state) const
{
	int index = (int)s;
	int value = statsx->Get(s);
	auto& info = SkillInfo::skills[index];

	// similar skill bonus
	if(info.similar != SimilarSkill::None)
	{
		int best = value;
		for(int i = index - 1; i >= 0; --i)
		{
			auto& info2 = SkillInfo::skills[i];
			if(info2.similar != info.similar)
				break;
			int value2 = statsx->Get((Skill)i);
			if(value2 > best)
				best = value2;
		}
		for(int i = index + 1; i < (int)Skill::MAX; ++i)
		{
			auto& info2 = SkillInfo::skills[i];
			if(info2.similar != info.similar)
				break;
			int value2 = statsx->Get((Skill)i);
			if(value2 > best)
				best = value2;
		}
		if(best != value)
			value += (best - value) / 2;

		// combat style bonus
		if(info.similar == SimilarSkill::Weapon)
			value += GetBase(Skill::ONE_HANDED_WEAPON) / 10;
	}

	// attribute bonus
	if(info.attrib2 != Attribute::NONE)
	{
		value += (GetBase(info.attrib) - 50) / 10;
		value += (GetBase(info.attrib2) - 50) / 10;
	}
	else
		value += (GetBase(info.attrib) - 50) / 5;

	// effects
	float effect_mod = GetEffectSum(EffectType::Skill, index, state);
	value += (int)effect_mod;
	value = Clamp(value, SkillInfo::MIN, SkillInfo::MAX);

	return value;
}

//=================================================================================================
// Similar better skills get +4 aptitude bonus
int Unit::GetAptitude(Skill s) const
{
	int index = (int)s;
	int apt = statsx->skill_apt[index];
	int value = statsx->Get(s);
	auto& info = SkillInfo::skills[index];
	if(info.similar != SimilarSkill::None)
	{
		int best = value;
		for(int i = index - 1; i >= 0; --i)
		{
			auto& info2 = SkillInfo::skills[i];
			if(info2.similar != info.similar)
				break;
			int value2 = statsx->Get((Skill)i);
			if(value2 > best)
				best = value2;
		}
		for(int i = index + 1; i < (int)Skill::MAX; ++i)
		{
			auto& info2 = SkillInfo::skills[i];
			if(info2.similar != info.similar)
				break;
			int value2 = statsx->Get((Skill)i);
			if(value2 > best)
				best = value2;
		}
		if(best != value)
			apt += 4;
	}

	if(IS_SET(statsx->perk_flags, PF_SLOW_LERNER))
		--apt;

	return apt;
}

//=================================================================================================
void Unit::ApplyStat(Attribute a)
{
	// recalculate other stats
	switch(a)
	{
	case Attribute::STR:
		// load depends on str
		CalculateLoad();
		break;
	case Attribute::END:
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
	case Attribute::DEX:
	case Attribute::INT:
	case Attribute::WIS:
	case Attribute::CHA:
		break;
	}

	if(player)
		player->recalculate_level = true;
}

//=================================================================================================
void Unit::ApplyStat(Skill s)
{
	switch(s)
	{
	case Skill::ATHLETICS:
		{
			// hp/load depends on athletics
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
	case Skill::ACROBATICS:
		{
			// stamina depends on acrobatics
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
	}

	if(player)
		player->recalculate_level = true;
}

//=================================================================================================
void Unit::CalculateStats(bool initial)
{
	if(initial)
	{
		hp = hpmax = CalculateMaxHp();
		stamina = stamina_max = CalculateMaxStamina();
	}
	else
	{
		RecalculateHp();
		RecalculateStamina();
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
			if(IsPlayer() && !player->is_local)
				player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
		}
	}
	CalculateLoad();
}

//=================================================================================================
float Unit::CalculateMobility(const Armor* armor) const
{
	if(IS_SET(data->flags3, F3_FIXED))
		return 100;

	if(!armor)
		armor = (const Armor*)slots[SLOT_ARMOR];

	// calculate base mobility (75-150)
	float mobility = 75.f + 0.5f * Get(Attribute::DEX) + 0.25f * Get(Skill::ACROBATICS);

	if(armor)
	{
		// calculate armor mobility (0-100)
		int armor_mobility = armor->mobility;
		int skill = min(Get(armor->skill), 100);
		armor_mobility += skill / 4;
		if(armor_mobility > 100)
			armor_mobility = 100;
		int str = Get(Attribute::STR);
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

	if(mobility > 200)
		mobility = 200;

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
Skill Unit::GetBestWeaponSkill() const
{
	const Skill weapon_skills[] = {
		Skill::SHORT_BLADE,
		Skill::LONG_BLADE,
		Skill::AXE,
		Skill::BLUNT
	};

	Skill best = Skill::NONE;
	int val = 0, index = 0;

	for(Skill s : weapon_skills)
	{
		int s_val = Get(s);
		if(s_val >= val)
		{
			val = s_val;
			best = s;
		}
		++index;
	}

	return best;
}

//=================================================================================================
Skill Unit::GetBestArmorSkill() const
{
	const Skill armor_skills[] = {
		Skill::LIGHT_ARMOR,
		Skill::MEDIUM_ARMOR,
		Skill::HEAVY_ARMOR
	};

	Skill best = Skill::NONE;
	int val = 0, index = 0;

	for(Skill s : armor_skills)
	{
		int s_val = Get(s);
		if(s_val >= val)
		{
			val = s_val;
			best = s;
		}
		++index;
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
		case ANI_WALK_TYL:
		case ANI_LEFT:
		case ANI_RIGHT:
		case ANI_KNEELS:
			if(stamina_action == SA_RESTORE_MORE)
				stamina_action = SA_RESTORE;
			break;
		case ANI_RUN:
			if(stamina_action > SA_RESTORE_SLOW)
				stamina_action = SA_RESTORE_SLOW;
			break;
		}
	}
}

//=================================================================================================
void Unit::RemoveStamina(float value)
{
	stamina -= value;
	stamina_timer = STAMINA_RESTORE_TIMER;
	if(player)
	{
		player->Train(TrainWhat::Stamina, value, 0);
		if(!player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
	}
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
				Game::Get().units_mesh_load.push_back(std::pair<Unit*, bool>(this, mode == CREATE_MESH::ON_WORLDMAP));
			if(data->state == ResourceState::NotLoaded)
			{
				ResourceManager::Get<Mesh>().AddLoadTask(mesh);
				if(data->sounds)
				{
					auto& sound_mgr = ResourceManager::Get<Sound>();
					for(int i = 0; i<SLOT_MAX; ++i)
					{
						if(data->sounds->sound[i])
							sound_mgr.AddLoadTask(data->sounds->sound[i]);
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
					Unit* unit = (Unit*)td.ptr;
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
				for(int i = 0; i<SLOT_MAX; ++i)
				{
					if(data->sounds->sound[i])
						sound_mgr.Load(data->sounds->sound[i]);
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

	Effect* e = Effect::Get();
	e->effect = EffectType::Stun;
	e->value = -1;
	e->power = 0.f;
	e->time = length;
	e->source = EffectSource::Action;
	e->source_id = -1;
	e->refs = 1;
	AddOrUpdateEffect(e);
	animation = ANI_STAND;
}

//=================================================================================================
void Unit::SetBase(Attribute attrib, int value, bool startup, bool mod)
{
	assert(statsx->unique);

	int index = (int)attrib;

	// calculate new base value
	int new_base_value = mod ? (statsx->attrib_base[index] + value) : value;
	new_base_value = Clamp(new_base_value, AttributeInfo::MIN, AttributeInfo::MAX);
	if(new_base_value == statsx->attrib_base[index])
		return;

	// calculate new value
	int dif = new_base_value - statsx->attrib_base[index];
	int new_value = statsx->attrib[index] + dif;
	new_value = Clamp(new_value, AttributeInfo::MIN, AttributeInfo::MAX);

	// apply new values
	statsx->attrib_base[index] = new_base_value;
	statsx->attrib[index] = new_value;
	statsx->attrib_apt[index] = StatsX::AttributeToAptitude(new_base_value);
	if(startup)
		statsx->attrib[index] = new_value;
	else
		Set(attrib, new_value);

	// send update
	if(!player->is_local && player->player_info)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::STAT_CHANGED;
		c.id = (int)ChangedStatType::BASE_ATTRIBUTE;
		c.ile = new_base_value;
	}
}

//=================================================================================================
void Unit::SetBase(Skill skill, int value, bool startup, bool mod)
{
	assert(statsx->unique);

	int index = (int)skill;

	// calculate new base value
	int new_base_value = mod ? (statsx->skill_base[index] + value) : value;
	new_base_value = Clamp(new_base_value, SkillInfo::MIN, SkillInfo::MAX);
	if(new_base_value == statsx->skill_base[index])
		return;

	// calculate new value
	int dif = new_base_value - statsx->skill_base[index];
	int new_value = statsx->skill[index] + dif;
	new_value = Clamp(new_value, SkillInfo::MIN, SkillInfo::MAX);

	// apply new values
	statsx->skill_base[index] = new_base_value;
	statsx->skill[index] = new_value;
	statsx->skill_apt[index] = StatsX::SkillToAptitude(new_base_value);
	if(startup)
		statsx->skill[index] = new_value;
	else
		Set(skill, new_value);

	// send update
	if(!player->is_local && player->player_info)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::STAT_CHANGED;
		c.id = (int)ChangedStatType::BASE_SKILL;
		c.ile = new_base_value;
	}
}

//=================================================================================================
float Unit::GetEffectSum(EffectType effect) const
{
	EffectSumBuffer buf;
	for(auto& e : effects)
	{
		if(e.effect == effect)
			buf += e;
	}
	return buf;
}

//=================================================================================================
float Unit::GetEffectSum(EffectType effect, int value, StatState& state) const
{
	EffectSumBufferWithState buf;
	for(auto& e : effects)
	{
		if(e.effect == effect && e.value == value)
			buf += e;
	}
	state = buf.GetState();
	return buf;
}

//=================================================================================================
bool Unit::GetEffectModMultiply(EffectType effect, float& value) const
{
	bool any = false;
	bool potion_first = true;
	EffectMulBuffer b(value);

	for(auto& e : effects)
	{
		if(e.effect == effect)
		{
			if(e.source == EffectSource::Potion)
			{
				if(e.power > b.best_potion || potion_first)
				{
					b.best_potion = e.power;
					potion_first = false;
				}
			}
			else
				b.mul *= e.power;
			any = true;
		}
	}

	value = b.mul * b.best_potion;
	return any;
}

//=================================================================================================
void Unit::RemovePerkEffects(Perk perk)
{
	uint index = 0;
	for(auto& e : effects)
	{
		if(e.source == EffectSource::Perk && e.source_id == (int)perk)
			_to_remove.push_back(index);
		++index;
	}
	RemoveEffects();
}

//=================================================================================================
bool Unit::HavePerk(Perk perk, int value)
{
	for(auto& p : statsx->perks)
	{
		if(p.perk == perk && (value == -1 || p.value == value))
			return true;
	}
	return false;
}

//=================================================================================================
int Unit::GetPerkIndex(Perk perk)
{
	int index = 0;
	for(auto& p : statsx->perks)
	{
		if(p.perk == perk)
			return index;
		++index;
	}
	return -1;
}

//=================================================================================================
void Unit::AddPerk(Perk perk, int value)
{
	assert(statsx->unique && player);

	TakenPerk tp(perk, value);
	PerkContext ctx(player);
	ctx.startup = false;
	tp.Apply(ctx);
}

//=================================================================================================
void Unit::RemovePerk(int index)
{
	assert(statsx->unique && player);

	TakenPerk& tp = statsx->perks[index];
	PerkContext ctx(player);
	ctx.startup = false;
	ctx.index = index;
	tp.Remove(ctx);
}

//=================================================================================================
void Unit::AddEffect(Effect* e, bool update)
{
	bool added = false;
	if(update)
	{
		if(Net::IsLocal())
			added = true;
		else
		{
			for(auto& ef : effects)
			{
				if(ef.netid == e->netid)
				{
					ef.effect = e->effect;
					ef.power = e->power;
					ef.time = e->time;
					ef.source = e->source;
					ef.source_id = e->source_id;
					e->Release();

					added = true;
					break;
				}
			}
		}
	}

	if(!added)
	{
		if(Net::IsLocal())
			e->netid = Effect::netid_counter++;
		effects.push_back(e);
	}

	EffectStatUpdate(*e);

	if(Net::IsServer())
	{
		if(EffectInfo::effects[(int)e->effect].observable)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::ADD_OBSERVABLE_EFFECT;
			c.id = netid;
			c.effect = e;
			e->update = added;
			e->refs++;
		}

		if(IsPlayer() && !player->is_local && player->player_info)
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::ADD_EFFECT;
			c.effect = e;
			e->update = added;
			e->refs++;
		}
	}
}

//=================================================================================================
// Keep effect with longest time (power should don't matter - for example stun)
void Unit::AddOrUpdateEffect(Effect* e)
{
	auto effect = FindEffect(e->effect);
	if(effect)
	{
		if(effect->time < e->time)
		{
			effect->time = e->time;
			effect->source = e->source;
			effect->source_id = e->source_id;
			AddEffect(effect, true);
		}
		e->Release();
	}
	else
		AddEffect(e);
}

//=================================================================================================
void Unit::RemoveEffect(const Effect* effect, bool notify)
{
	EffectStatUpdate(*effect);

	if(notify && Net::IsServer())
	{
		if(EffectInfo::effects[(int)effect->effect].observable)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::REMOVE_OBSERVABLE_EFFECT;
			c.unit = this;
			c.id = effect->netid;
		}

		if(IsPlayer() && !player->is_local && player->player_info)
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::REMOVE_EFFECT;
			c.id = effect->netid;
		}
	}
}

//=================================================================================================
void Unit::EffectStatUpdate(const Effect& e)
{
	switch(e.effect)
	{
	case EffectType::Attribute:
		ApplyStat((Attribute)e.value);
		break;
	case EffectType::Skill:
		ApplyStat((Skill)e.value);
		break;
	case EffectType::Health:
		RecalculateHp();
		break;
	case EffectType::Stamina:
		RecalculateStamina();
		break;
	case EffectType::Carry:
		CalculateLoad();
		break;
	}
}

//=================================================================================================
void Unit::RemoveEffect(EffectType effect)
{
	assert(Net::IsLocal());

	uint index = 0;
	for(auto& e : effects)
	{
		if(e.effect == effect)
			_to_remove.push_back(index);
		++index;
	}

	RemoveEffects();
}

//=================================================================================================
bool Unit::RemoveEffect(int netid)
{
	for(auto it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->netid == netid)
		{
			Effect* e = it.ptr();
			effects.erase(it);
			RemoveEffect(e, true);
			e->Release();
			return true;
		}
	}
	return false;
}

//=================================================================================================
void Unit::RemoveEffects(EffectType effect, EffectSource source, Perk source_id)
{
	assert(Net::IsLocal());

	uint index = 0;
	for(auto& e : effects)
	{
		if((e.effect == effect || effect == EffectType::None)
			&& (e.source == source || source == EffectSource::None)
			&& (e.source_id == (int)source_id || source_id == Perk::None))
		{
			_to_remove.push_back(index);
		}
		++index;
	}
	RemoveEffects();
}

//=================================================================================================
void Unit::RemoveEffects(bool notify)
{
	while(!_to_remove.empty())
	{
		uint index = _to_remove.back();
		auto e = effects.ptr(index);
		_to_remove.pop_back();
		if(index == effects.size() - 1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin() + index, effects.end() - 1);
			effects.pop_back();
		}
		RemoveEffect(e, notify);
		e->Release();
	}
}

//=================================================================================================
void Unit::WriteEffects(BitStream& stream)
{
	stream.WriteCasted<byte>(effects.size());
	for(auto& e : effects)
	{
		stream.Write(e.netid);
		stream.WriteCasted<byte>(e.effect);
		stream.Write(e.value);
		stream.Write(e.power);
		stream.Write(e.time);
		stream.WriteCasted<byte>(e.source);
		stream.WriteCasted<byte>((e.source_id == -1 ? 0xFF : e.source_id));
	}
}

//=================================================================================================
bool Unit::ReadEffects(BitStream& stream)
{
	byte count;
	if(!stream.Read(count) || !EnsureSize(stream, 15 * count))
		return false;

	// clear existing observable effects
	effects.clear();

	effects.reserve(count);
	for(uint i=0; i<count; ++i)
	{
		auto& e = effects.add();
		stream.Read(e.netid);
		stream.ReadCasted<byte>(e.effect);
		stream.Read(e.value);
		stream.Read(e.power);
		stream.Read(e.time);
		stream.ReadCasted<byte>(e.source);
		byte source_id;
		stream.Read(source_id);
		e.source_id = (source_id == 0xFF ? -1 : (int)source_id);
	}

	return true;
}

//=================================================================================================
void Unit::AddObservableEffect(EffectType effect, int netid, float time, bool update)
{
	assert(Net::IsClient());

	if(update)
	{
		for(auto& e : effects)
		{
			if(e.netid == netid)
			{
				e.effect = effect;
				e.time = time;
				return;
			}
		}
	}

	Effect* e = Effect::Get();
	e->effect = effect;
	e->netid = netid;
	e->value = -1;
	e->power = 0;
	e->time = time;
	e->source = EffectSource::Action;
	e->source_id = -1;
	e->refs = 1;
	effects.push_back(e);
}

//=================================================================================================
bool Unit::RemoveObservableEffect(int netid)
{
	assert(Net::IsClient());

	for(auto it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->netid == netid)
		{
			Effect* e = it.ptr();
			effects.erase(it);
			e->Release();
			return true;
		}
	}
	return false;
}

//=================================================================================================
void Unit::WriteObservableEffects(BitStream& stream)
{
	uint effects_count_pos = PatchByte(stream);
	uint effects_count = 0u;
	for(auto& e : effects)
	{
		auto& info = EffectInfo::effects[(int)e.effect];
		if(info.observable)
		{
			++effects_count;
			stream.Write(e.netid);
			stream.WriteCasted<byte>(e.effect);
			stream.Write(e.time);
		}
	}
	if(effects_count)
		PatchByteApply(stream, effects_count_pos, effects_count);
}

//=================================================================================================
bool Unit::ReadObservableEffects(BitStream& stream)
{
	byte effects_count;
	if(!stream.Read(effects_count) || !EnsureSize(stream, 9 * effects_count))
		return false;

	effects.reserve(effects_count);
	for(uint i=0; i<effects_count; ++i)
	{
		auto& e = effects.add();
		stream.Read(e.netid);
		stream.ReadCasted<byte>(e.effect);
		stream.Read(e.time);
		e.power = 0;
		e.source = EffectSource::Action;
		e.source_id = -1;
	}

	return true;
}

//=================================================================================================
void Unit::PassTime(int days, PassTimeType type)
{
	// gaining experience by heroes
	if(IsHero())
	{
		if(type == TRAVEL)
			hero->expe += days;
		hero->expe += days;

		if(level != MAX_LEVEL && level != data->level.y)
		{
			int req = (level*(level + 1) + 10) * 5;
			if(hero->expe >= req)
			{
				hero->expe -= req;
				hero->LevelUp();
			}
		}
	}

	// update effects that work for days, end other
	float prev_hp = hp,
		prev_stamina = stamina;
	EndEffects();

	// regenerate hp
	if(hp != hpmax)
	{
		struct Heal
		{
			float power;
			int days;
			bool potion;
		};
		static vector<Heal> heal;
		heal.clear();
		for(auto& e : effects)
		{
			if(e.effect == EffectType::NaturalHealingMod)
				heal.push_back({ e.power, (int)e.time, e.source == EffectSource::Potion });
		}
		float heal_mod;
		if(!heal.empty())
		{
			heal_mod = 0.f;
			for(int day = 0; day < days; ++day)
			{
				EffectMulBuffer buf(1.f);
				bool any = false;
				for(auto& h : heal)
				{
					if(day < h.days)
					{
						if(h.potion)
						{
							if(h.power > buf.best_potion)
								buf.best_potion = h.power;
						}
						else
							buf.mul *= h.power;
						any = true;
					}
				}
				if(any)
					heal_mod += buf.mul * buf.best_potion;
				else
				{
					heal_mod += (float)(days - day);
					break;
				}
			}
		}
		else
			heal_mod = (float)days;

		float hp_to_heal = heal_mod * 0.5f * Get(Attribute::END);
		if(type == REST)
			hp_to_heal *= 2;

		hp_to_heal = min(hp_to_heal, hpmax - hp);
		hp += hp_to_heal;

		if(IsPlayer())
			player->Train(TrainWhat::Regenerate, hp_to_heal, 0);
	}

	// end effects that work for days
	EndLongEffects(days);

	// send update
	if(Net::IsOnline() && type != TRAVEL)
	{
		if(hp != prev_hp)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}

		if(stamina != prev_stamina && IsPlayer() && !player->is_local)
			player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
	}
}
