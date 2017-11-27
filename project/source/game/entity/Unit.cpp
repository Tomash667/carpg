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

const float Unit::AUTO_TALK_WAIT = 0.333f;
const float Unit::STAMINA_BOW_ATTACK = 100.f;
const float Unit::STAMINA_BASH_ATTACK = 50.f;
const float Unit::STAMINA_UNARMED_ATTACK = 50.f;
const float Unit::STAMINA_RESTORE_TIMER = 1.f;

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
	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->hp;
	else
	{
		float end = (float)Get(Attribute::END);
		float ath = (float)Get(Skill::ATHLETICS);
		float lvl = IsPlayer() ? player->level : (float)level;
		return data->hp + (data->hp_bonus + (end - 50) / 5 + ath / 5) * (lvl + 2);
	}
}

//=================================================================================================
float Unit::CalculateMaxStamina() const
{
	if(IS_SET(data->flags3, F3_FIXED))
		return (float)data->stamina;
	else
		return (float)(data->stamina + Get(Attribute::END) * 2 + Get(Skill::ACROBATICS));
}

//=================================================================================================
void Unit::CalculateLoad()
{
	weight_max = 200 + Get(Attribute::STR) * 10 + Get(Skill::ATHLETICS) * 5;
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

	// je�li co� robi to nie mo�e u�y�
	if(action != A_NONE)
	{
		if(action == A_TAKE_WEAPON && weapon_state == WS_HIDING)
		{
			// je�li chowa bro� to u�yj miksturki jak schowa
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

	// je�li bro� jest wyj�ta to schowaj
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

	// usu� przedmiot
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

	// wy�lij komunikat
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
		// anuluje wyci�ganie nast�pnej broni po schowaniu tej
		weapon_taken = W_NONE;
		return;
	case WS_TAKING:
		if(animation_state == 0)
		{
			// jeszcze nie wyj�� broni z pasa, po prostu wy��cz t� grupe
			action = A_NONE;
			weapon_taken = W_NONE;
			weapon_state = WS_HIDDEN;
			mesh_inst->Deactivate(1);
		}
		else
		{
			// wyj�� bro� z pasa, zacznij chowa�
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
// Dodaje przedmiot(y) do ekwipunku, zwraca true je�li przedmiot si� zestackowa�
//=================================================================================================
bool Unit::AddItem(const Item* item, uint count, uint team_count)
{
	assert(item && count != 0 && team_count <= count);

	Game& game = Game::Get();
	if(item->type == IT_GOLD && Team.IsTeamMember(*this))
	{
		if(Net::IsLocal())
		{
			if(team_count && IsTeamMember())
			{
				game.AddGold(team_count);
				uint normal_gold = count - team_count;
				if(normal_gold)
				{
					gold += normal_gold;
					if(IsPlayer() && player != game.pc)
						game.GetPlayerInfo(player).UpdateGold();
				}
			}
			else
			{
				gold += count;
				if(IsPlayer() && player != game.pc)
					game.GetPlayerInfo(player).UpdateGold();
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
	Game& game = Game::Get();

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
			Effect& e = Add1(effects);
			e.effect = item.effect;
			e.time = item.time;
			e.power = item.power / item.time;
		}
		break;
	case E_REGENERATE:
	case E_NATURAL:
	case E_ANTIMAGIC:
	case E_STAMINA:
		{
			Effect& e = Add1(effects);
			e.effect = item.effect;
			e.time = item.time;
			e.power = item.power;
		}
		break;
	case E_ANTIDOTE:
		{
			uint index = 0;
			for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
			{
				if(it->effect == E_POISON)
					_to_remove.push_back(index);
			}

			while(!_to_remove.empty())
			{
				index = _to_remove.back();
				_to_remove.pop_back();
				if(index == effects.size() - 1)
					effects.pop_back();
				else
				{
					std::iter_swap(effects.begin() + index, effects.end() - 1);
					effects.pop_back();
				}
			}

			if(alcohol != 0.f)
			{
				alcohol = 0.f;
				if(IsPlayer() && game.pc != player)
					game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
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
			Effect& e = Add1(effects);
			e.effect = E_FOOD;
			e.time = item.power;
			e.power = 1.f;
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
	Game& game = Game::Get();
	float best_reg = 0.f, food_heal = 0.f, poison_dmg = 0.f, alco_sum = 0.f, best_stamina = 0.f;

	// update effects timer
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == E_NATURAL)
			continue;
		switch(it->effect)
		{
		case E_REGENERATE:
			if(it->power > best_reg)
				best_reg = it->power;
			break;
		case E_POISON:
			poison_dmg += it->power;
			break;
		case E_ALCOHOL:
			alco_sum += it->power;
			break;
		case E_FOOD:
			food_heal += dt;
			break;
		case E_STAMINA:
			if(it->power > best_stamina)
				best_stamina = it->power;
			break;
		}
		if((it->time -= dt) <= 0.f)
			_to_remove.push_back(index);
	}

	// remove expired effects
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size() - 1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin() + index, effects.end() - 1);
			effects.pop_back();
		}
	}

	if(Net::IsClient())
		return;

	// healing from food
	if((best_reg > 0.f || food_heal > 0.f) && hp != hpmax)
	{
		float natural = 1.f;
		FindEffect(E_NATURAL, &natural);
		hp += (best_reg * dt + food_heal) * natural;
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
			game.UnitFall(*this);
		if(IsPlayer() && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
	}
	else if(alcohol != 0.f)
	{
		alcohol -= dt / 10 * Get(Attribute::END);
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
	}

	// update poison damage
	if(poison_dmg != 0.f)
		game.GiveDmg(game.GetContext(*this), nullptr, poison_dmg * dt, *this, nullptr, Game::DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(Net::IsOnline() && player != game.pc && player->last_dmg_poison != poison_dmg)
			game.game_players[player->id]->update_flags |= PlayerInfo::UF_POISON_DAMAGE;
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
		stamina += ((stamina_max * stamina_restore / 100) + best_stamina) * dt;
		if(stamina > stamina_max)
			stamina = stamina_max;
		if(Net::IsServer() && player && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_STAMINA;
	}
}

//=================================================================================================
void Unit::EndEffects(int days, int* best_nat)
{
	if(best_nat)
		*best_nat = 0;

	alcohol = 0.f;
	stamina = stamina_max;
	stamina_timer = 0;

	if(effects.empty())
		return;

	uint index = 0;
	float best_reg = 0.f, best_natural = 1.f, food = 0.f;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		switch(it->effect)
		{
		case E_REGENERATE:
			{
				float reg = it->power * it->time;
				if(reg > best_reg)
					best_reg = reg;
				_to_remove.push_back(index);
			}
			break;
		case E_POISON:
			hp -= it->power * it->time;
			_to_remove.push_back(index);
			break;
		case E_FOOD:
			food += it->time;
			_to_remove.push_back(index);
			break;
		case E_ALCOHOL:
		case E_ANTIMAGIC:
		case E_STAMINA:
		case E_STUN:
			_to_remove.push_back(index);
			break;
		case E_NATURAL:
			best_natural = 2.f;
			if(best_nat)
			{
				int t = Roundi(it->time);
				if(t > *best_nat)
					*best_nat = t;
			}
			it->time -= days;
			if(it->time <= 0.f)
				_to_remove.push_back(index);
			break;
		}
	}

	hp += (best_reg + food) * best_natural;
	if(hp < 1.f)
		hp = 1.f;
	else if(hp > hpmax)
		hp = hpmax;

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
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
// Dodaje przedmioty do ekwipunku i zak�ada je je�li nie ma nic za�o�onego. Dodane przedmioty s�
// traktowane jako dru�ynowe
//=================================================================================================
void Unit::AddItemAndEquipIfNone(const Item* item, uint count)
{
	assert(item && count != 0);

	if(item->IsStackable())
		AddItem(item, count, count);
	else
	{
		// za�� je�li nie ma
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
	// oblicz dps i tyle na t� wersj�
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
	if(ile)
		WriteFile(file, &effects[0], sizeof(Effect)*ile, &tmp, nullptr);

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
		live_state = LiveState(live_state + 2); // kolejno�� si� zmieni�a
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
		// w nowszych wersjach nie ma tej zmiennej, alkohol dzia�a inaczej
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

	// efekty
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	effects.resize(ile);
	if(ile)
		ReadFile(file, &effects[0], sizeof(Effect)*ile, &tmp, nullptr);

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
Effect* Unit::FindEffect(ConsumeEffect effect)
{
	for(Effect& e : effects)
	{
		if(e.effect == effect)
			return &e;
	}

	return nullptr;
}

//=================================================================================================
bool Unit::FindEffect(ConsumeEffect effect, float* value)
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
// szuka miksturek leczniczych w ekwipunku, zwraca -1 je�li nie odnaleziono
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

	assert(0 && "Nie znalaziono questowego przedmiotu do usuni�cia!");
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

	/* co wp�ywa na szybko�� ataku?
	+ rodzaj broni
	+ zr�czno��
	+ umiej�tno�ci
	+ brakuj�ca si�a
	+ ud�wig */
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
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == E_POISON)
			_to_remove.push_back(index);
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
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
void Unit::RemoveEffect(ConsumeEffect effect)
{
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == effect)
			_to_remove.push_back(index);
	}

	if(!_to_remove.empty())
	{
		if(Net::IsOnline() && Net::IsServer())
		{
			auto& c = Add1(Net::changes);
			c.type = NetChange::STUN;
			c.unit = this;
			c.f[0] = 0;
		}
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
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
float Unit::CalculateArmorDefense(const Armor* in_armor)
{
	if(!in_armor && !HaveArmor())
		return 0.f;

	// pancerz daje tyle ile bazowo * skill
	const Armor& armor = (in_armor ? *in_armor : GetArmor());
	float skill_val = (float)Get(armor.skill);
	int str = Get(Attribute::STR);
	if(str < armor.req_str)
		skill_val *= str / armor.req_str;

	return (skill_val / 100 + 1)*armor.def;
}

//=================================================================================================
float Unit::CalculateDexterityDefense(const Armor* in_armor)
{
	float load = GetLoad();
	float mod;

	// pancerz
	if(in_armor || HaveArmor())
	{
		const Armor& armor = (in_armor ? *in_armor : GetArmor());
		if(armor.skill == Skill::HEAVY_ARMOR)
			mod = 0.2f;
		else if(armor.skill == Skill::MEDIUM_ARMOR)
			mod = 0.5f;
		else
			mod = 1.f;
	}
	else
		mod = 2.f;

	// zr�czno��
	if(load < 1.f)
	{
		int dex = Get(Attribute::DEX);
		if(dex > 50)
			return (float(dex - 50) / 3) * (1.f - load) * mod;
	}

	return 0.f;
}

//=================================================================================================
float Unit::CalculateBaseDefense() const
{
	return 0.1f * Get(Attribute::END) + data->def_bonus;
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
				// zak�ada �e mo�na mie� tylko jeden stack takich przedmiot�w
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
				// brak animacja wyci�gania broni bez tarczy, u�yj zwyk�ej
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
	if(HaveEffect(E_ANTIMAGIC))
		mres *= 0.5f;
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
bool Unit::HaveEffect(ConsumeEffect e) const
{
	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == e)
			return true;
	}
	return false;
}

//=================================================================================================
int Unit::GetBuffs() const
{
	int b = 0;

	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		switch(it->effect)
		{
		case E_POISON:
			b |= BUFF_POISON;
			break;
		case E_ALCOHOL:
			b |= BUFF_ALCOHOL;
			break;
		case E_REGENERATE:
			b |= BUFF_REGENERATION;
			break;
		case E_FOOD:
			b |= BUFF_FOOD;
			break;
		case E_NATURAL:
			b |= BUFF_NATURAL;
			break;
		case E_ANTIMAGIC:
			b |= BUFF_ANTIMAGIC;
			break;
		case E_STAMINA:
			b |= BUFF_STAMINA;
			break;
		case E_STUN:
			b |= BUFF_STUN;
			break;
		}
	}

	if(alcohol / hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

//=================================================================================================
int Unit::Get(Skill s) const
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
		Game& game = Game::Get();
		if(game.pc != player)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_STAMINA;
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

	auto effect = FindEffect(E_STUN);
	if(effect)
		effect->time = max(effect->time, length);
	else
	{
		auto& effect = Add1(effects);
		effect.effect = E_STUN;
		effect.power = 0.f;
		effect.time = length;
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
