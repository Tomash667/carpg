#include "Pch.h"
#include "Base.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "Inventory.h"
#include "UnitHelper.h"

//=================================================================================================
Unit::~Unit()
{
	delete ani;
	delete human_data;
	delete hero;
	delete player;
}

//=================================================================================================
float Unit::CalculateMaxHp() const
{
	float v = 0.8f*Get(Attribute::END) + 0.2f*Get(Attribute::STR);
	if(v >= 50.f)
		return data->hp_bonus * (1.f + (v-50)/50);
	else
		return data->hp_bonus * (1.f - (50-v)/100);
}

//=================================================================================================
float Unit::CalculateAttack() const
{
	if(HaveWeapon())
		return CalculateAttack(&GetWeapon());
	else
		return (1.f + 1.f/200*(Get(Skill::ONE_HANDED_WEAPON) + Get(Skill::UNARMED))) * (Get(Attribute::STR) + Get(Attribute::DEX)/2);
}

//=================================================================================================
float Unit::CalculateAttack(const Item* _weapon) const
{
	assert(_weapon && OR2_EQ(_weapon->type, IT_WEAPON, IT_BOW));

	int str = Get(Attribute::STR),
		dex = Get(Attribute::DEX);

	if(_weapon->type == IT_WEAPON)
	{
		const Weapon& w = _weapon->ToWeapon();
		const WeaponTypeInfo& wi = weapon_type_info[w.weapon_type];
		float p;
		if(str >= w.req_str)
			p = 1.f;
		else
			p = float(str) / w.req_str;
		return wi.str2dmg * str + wi.dex2dmg * dex + (w.dmg * p * (1.f + 1.f / 200 * (Get(Skill::ONE_HANDED_WEAPON) + Get(wi.skill))));
	}
	else
	{
		const Bow& b = _weapon->ToBow();
		float p;
		if(str >= b.req_str)
			p = 1.f;
		else
			p = float(str) / b.req_str;
		return ((float)dex + b.dmg * (1.f + 1.f/100*Get(Skill::BOW))) * p;
	}
}

//=================================================================================================
float Unit::CalculateBlock(const Item* _shield) const
{
	assert(_shield && _shield->type == IT_SHIELD);

	const Shield& s = _shield->ToShield();
	float p;
	int str = Get(Attribute::STR);
	if(str >= s.req_str)
		p = 1.f;
	else
		p = float(str) / s.req_str;

	return float(s.def) * (1.f + 1.f/100*Get(Skill::SHIELD)) * p;
}

//=================================================================================================
float Unit::CalculateWeaponBlock() const
{
	const Weapon& w = GetWeapon();

	float p;
	int str = Get(Attribute::STR);
	if(str >= w.req_str)
		p = 1.f;
	else
		p = float(str) / w.req_str;

	return float(w.dmg) * 0.66f * (1.f + 0.008f*Get(Skill::SHIELD) + 0.002f*Get(Skill::ONE_HANDED_WEAPON)) * p;
}

//=================================================================================================
// WZÓR NA OBRONÊ
// kondycja/5 + pancerz * (skill * (1+max(1.f, si³a/wymagana)) + zrêcznoœæ/5*max(%obci¹¿enia, ciê¿ki_pancerz ? 0.5 : 0)
float Unit::CalculateDefense() const
{
	float def = CalculateBaseDefense();
	float load = GetLoad();

	// pancerz
	if(HaveArmor())
	{
		const Armor& a = GetArmor();

		// pancerz daje tyle ile bazowo * skill
		switch(a.skill)
		{
		case Skill::HEAVY_ARMOR:
			load *= 0.5f;
			break;
		case Skill::MEDIUM_ARMOR:
			load *= 0.75f;
			break;
		}

		float skill_val = (float)Get(a.skill);
		int str = Get(Attribute::STR);
		if(str < a.req_str)
			skill_val *= str / a.req_str;
		def += (skill_val/100+1)*a.def;
	}

	// zrêcznoœæ
	if(load < 1.f)
	{
		int dex = Get(Attribute::DEX);
		if(dex > 50)
			def += (float(dex - 50) / 3) * (1.f-load);
	}

	return def;
}

//=================================================================================================
float Unit::CalculateDefense(const Item* _armor) const
{
	assert(_armor && _armor->type == IT_ARMOR);

	float def = CalculateBaseDefense();
	float load = GetLoad();

	const Armor& a = _armor->ToArmor();

	// pancerz daje tyle ile bazowo * skill
	switch(a.skill)
	{
	case Skill::HEAVY_ARMOR:
		load *= 0.5f;
		break;
	case Skill::MEDIUM_ARMOR:
		load *= 0.75f;
		break;
	}

	float skill_val = (float)Get(a.skill);
	int str = Get(Attribute::STR);
	if(str < a.req_str)
		skill_val *= str / a.req_str;
	def += (skill_val/100+1)*a.def;

	// zrêcznoœæ
	if(load < 1.f)
	{
		int dex = Get(Attribute::DEX);
		if(dex > 50)
			def += (float(dex - 50) / 3) * (1.f-load);
	}

	return def;
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
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
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
		item->rot = random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}
		if(!game.CheckMoonStone(item, *this))
			game.AddGroundItem(game.GetContext(*this), item);
		
		if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
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
			items.erase(items.begin()+index);
		}

		NetChange& c = Add1(game.net_changes);
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
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = item2;
		item->count = 1;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		item2 = nullptr;
		game.AddGroundItem(game.GetContext(*this), item);

		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;

			NetChange& c2 = Add1(game.net_changes);
			c2.unit = this;
			c2.id = slot;
			c2.type = NetChange::CHANGE_EQUIPMENT;
		}
	}
	else
	{
		item2 = nullptr;

		NetChange& c = Add1(game.net_changes);
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
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = s.item;
		item->count = count;
		item->team_count = min(count, s.team_count);
		s.team_count -= item->team_count;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}
		game.AddGroundItem(game.GetContext(*this), item);

		if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
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
			items.erase(items.begin()+index);
		}

		NetChange& c = Add1(game.net_changes);
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

	for(int i=0; i<SLOT_MAX; ++i)
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
				if(player == Game::Get().pc)
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
			assert(Inventory::lock_id == LOCK_NO && Game::Get().pc == player);
			player->next_action = NA_CONSUME;
			Inventory::lock_index = index;
			Inventory::lock_id = LOCK_MY;
		}
		else
			ai->potion = index;
		return 2;
	}

	ItemSlot& slot = items[index];

	assert(slot.item && slot.item->type == IT_CONSUMEABLE);

	const Consumeable& cons = slot.item->ToConsumeable();

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
		items.erase(items.begin()+index);
		removed = true;
	}

	// zacznij spo¿ywaæ
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
	ani->Play(anim_name, PLAY_ONCE|PLAY_PRIO1, 1);
	used_item = &cons;

	// wyœlij komunikat
	Game& game = Game::Get();
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::CONSUME_ITEM;
		if(game.IsServer())
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
void Unit::ConsumeItem(const Consumeable& item, bool force, bool send)
{
	if(action != A_NONE && action != A_ANIMATION2)
	{
		if(Game::Get().IsLocal())
		{
			assert(0);
			return;
		}
		else
			weapon_state = WS_HIDDEN;
	}

	cstring anim_name;
	if(item.cons_type == Food)
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
	ani->Play(anim_name, PLAY_ONCE|PLAY_PRIO1, 1);
	used_item = &item;
	used_item_is_team = true;

	if(send)
	{
		Game& game = Game::Get();
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::CONSUME_ITEM;
			c.unit = this;
			c.id = (int)&item;
			c.ile = (force ? 1 : 0);
		}
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
			ani->Deactivate(1);
		}
		else
		{
			// wyj¹³ broñ z pasa, zacznij chowaæ
			weapon_hiding = weapon_taken;
			weapon_taken = W_NONE;
			weapon_state = WS_HIDING;
			animation_state = 0;
			SET_BIT(ani->groups[1].state, AnimeshInstance::FLAG_BACK);
		}
		break;
	case WS_TAKEN:
		ani->Play(GetTakeWeaponAnimation(weapon_taken == W_ONE_HANDED), PLAY_PRIO1 | PLAY_ONCE | PLAY_BACK, 1);
		weapon_hiding = weapon_taken;
		weapon_taken = W_NONE;
		animation_state = 0;
		action = A_TAKE_WEAPON;
		weapon_state = WS_HIDING;
		ani->frame_end_info2 = false;
		break;
	}

	Game& game = Game::Get();
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::TAKE_WEAPON;
		c.unit = this;
		c.id = 1;
	}
}

//=================================================================================================
void Unit::TakeWeapon(WeaponType _type)
{
	assert(_type == W_ONE_HANDED || _type == W_BOW);

	if(action != A_NONE)
		return;

	if(weapon_taken == _type)
		return;

	if(weapon_taken == W_NONE)
	{
		ani->Play(GetTakeWeaponAnimation(_type == W_ONE_HANDED), PLAY_PRIO1|PLAY_ONCE, 1);
		weapon_hiding = W_NONE;
		weapon_taken = _type;
		animation_state = 0;
		action = A_TAKE_WEAPON;
		weapon_state = WS_TAKING;
		ani->frame_end_info2 = false;

		Game& game = Game::Get();
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::TAKE_WEAPON;
			c.unit = this;
			c.id = 0;
		}
	}
	else
	{
		HideWeapon();
		weapon_taken = _type;
	}
}

//=================================================================================================
// Dodaje przedmiot(y) do ekwipunku, zwraca true jeœli przedmiot siê zestackowa³
//=================================================================================================
bool Unit::AddItem(const Item* item, uint count, uint team_count)
{
	assert(item && count != 0 && team_count <= count);

	Game& game = Game::Get();
	if(item->type == IT_GOLD && game.IsTeamMember(*this))
	{
		if(game.IsLocal())
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
void Unit::ApplyConsumeableEffect(const Consumeable& item)
{
	Game& game = Game::Get();

	switch(item.effect)
	{
	case E_HEAL:
		hp += item.power;
		if(hp > hpmax)
			hp = hpmax;
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
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
			e.power = item.power/item.time;
		}
		break;
	case E_REGENERATE:
	case E_NATURAL:
	case E_ANTIMAGIC:
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
				if(index == effects.size()-1)
					effects.pop_back();
				else
				{
					std::iter_swap(effects.begin()+index, effects.end()-1);
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
		game.Train(*this, false, (int)Attribute::STR, 2);
		break;
	case E_END:
		game.Train(*this, false, (int)Attribute::END, 2);
		break;
	case E_DEX:
		game.Train(*this, false, (int)Attribute::DEX, 2);
		break;
	case E_FOOD:
		{
			Effect& e = Add1(effects);
			e.effect = E_FOOD;
			e.time = item.power;
			e.power = 1.f;
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void Unit::UpdateEffects(float dt)
{
	float best_reg = 0.f, food_heal = 0.f, poison_dmg = 0.f, alco_sum = 0.f;

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
		}
		if((it->time -= dt) <= 0.f)
			_to_remove.push_back(index);
	}

	Game& game = Game::Get();

	if((best_reg > 0.f || food_heal > 0.f) && hp != hpmax)
	{
		float natural = 1.f;
		FindEffect(E_NATURAL, &natural);
		hp += (best_reg * dt + food_heal) * natural;
		if(hp > hpmax)
			hp = hpmax;
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
	}

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
		alcohol -= dt/10*Get(Attribute::END);
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
	}

	if(poison_dmg != 0.f)
		game.GiveDmg(game.GetContext(*this), nullptr, poison_dmg * dt, *this, nullptr, DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(game.IsOnline() && player != game.pc && player->last_dmg_poison != poison_dmg)
			game.game_players[player->id].update_flags |= PlayerInfo::UF_POISON_DAMAGE;
		player->last_dmg_poison = poison_dmg;
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
void Unit::EndEffects(int days, int* best_nat)
{
	if(best_nat)
		*best_nat = 0;

	alcohol = 0.f;
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
			_to_remove.push_back(index);
			break;
		case E_NATURAL:
			best_natural = 2.f;
			if(best_nat)
			{
				int t = roundi(it->time);
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
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
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
void Unit::GetBox(BOX& _box) const
{
	float radius = GetUnitRadius();
	_box.v1.x = pos.x - radius;
	_box.v1.y = pos.y;
	_box.v1.z = pos.z - radius;
	_box.v2.x = pos.x + radius;
	_box.v2.y = pos.y + max(MIN_H, GetUnitHeight());
	_box.v2.z = pos.z + radius;
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
VEC3 Unit::GetLootCenter() const
{
	Animesh::Point* point2 = ani->ani->GetPoint("centrum");

	if(!point2)
		return pos;

	const Animesh::Point& point = *point2;
	MATRIX matBone = point.mat * ani->mat_bones[point.bone];

	MATRIX matPos, matRot;
	D3DXMatrixTranslation(&matPos, pos);
	D3DXMatrixRotationY(&matRot, rot);

	matBone = matBone * (matRot * matPos);

	VEC3 pos(0,0,0), out;
	D3DXVec3TransformCoord(&out, &pos, &matBone);

	return out;
}

//=================================================================================================
float Unit::CalculateWeaponPros(const Weapon& _weapon) const
{
	// oblicz dps i tyle na t¹ wersjê
	return CalculateAttack(&_weapon) * GetAttackSpeed(&_weapon);
}

//=================================================================================================
bool Unit::IsBetterWeapon(const Weapon& _weapon) const
{
	if(!HaveWeapon())
		return true;

	return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(_weapon);
}

//=================================================================================================
bool Unit::IsBetterWeapon(const Weapon& _weapon, int* value) const
{
	if(!HaveWeapon())
	{
		if(value)
			*value = (int)CalculateWeaponPros(_weapon);
		return true;
	}

	if(value)
	{
		float v = CalculateWeaponPros(_weapon);
		*value = (int)v;
		return CalculateWeaponPros(GetWeapon()) < v;
	}
	else
		return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(_weapon);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& _armor) const
{
	if(!HaveArmor())
		return true;

	return CalculateDefense(&GetArmor()) < CalculateDefense(&_armor);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& _armor, int* value) const
{
	if(!HaveArmor())
	{
		if(value)
			*value = (int)CalculateDefense(&_armor);
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&_armor);
		*value = (int)v;
		return CalculateDefense(&GetArmor()) < v;
	}
	else
		return CalculateDefense(&GetArmor()) < CalculateDefense(&_armor);
}

//=================================================================================================
void Unit::RecalculateHp()
{
	float hpp = hp/hpmax;
	hpmax = CalculateMaxHp();
	hp = hpmax * hpp;
	if(hp < 1)
		hp = 1;
	else if(hp > hpmax)
		hp = hpmax;
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
	assert(in_range(frame, 0, 2));

	assert(attack_id < data->frames->attacks);

	if(!data->frames->extra)
	{
		switch(frame)
		{
		case 0:
			return data->frames->t[F_ATTACK1_START+attack_id*2+0];
		case 1:
			return data->frames->lerp(F_ATTACK1_START+attack_id*2);
		case 2:
			return data->frames->t[F_ATTACK1_START+attack_id*2+1];
		default:
			assert(0);
			return data->frames->t[F_ATTACK1_START+attack_id*2+1];
		}
	}
	else
	{
		switch(frame)
		{
		case 0:
			return data->frames->extra->e[attack_id].start;
		case 1:
			return data->frames->extra->e[attack_id].lerp();
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
			int n = rand2()%data->frames->attacks;
			if(IS_SET(data->frames->extra->e[n].flags, a))
				return n;
		}
		while(1);
	}
	else
		return rand2()%data->frames->attacks;
}

//=================================================================================================
void Unit::Save(HANDLE file, bool local)
{
	// id postaci
	WriteString1(file, data->id);

	// przedmioty
	for(uint i=0; i<SLOT_MAX; ++i)
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
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	WriteFile(file, &level, sizeof(level), &tmp, nullptr);
	FileWriter f(file);
	stats.Save(f);
	unmod_stats.Save(f);
	WriteFile(file, &gold, sizeof(gold), &tmp, nullptr);
	WriteFile(file, &invisible, sizeof(invisible), &tmp, nullptr);
	WriteFile(file, &in_building, sizeof(in_building), &tmp, nullptr);
	WriteFile(file, &to_remove, sizeof(to_remove), &tmp, nullptr);
	WriteFile(file, &temporary, sizeof(temporary), &tmp, nullptr);
	WriteFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
	WriteFile(file, &assist, sizeof(assist), &tmp, nullptr);
	WriteFile(file, &auto_talk, sizeof(auto_talk), &tmp, nullptr);
	if(auto_talk == 2)
		WriteFile(file, &auto_talk_timer, sizeof(auto_talk_timer), &tmp, nullptr);
	WriteFile(file, &dont_attack, sizeof(dont_attack), &tmp, nullptr);
	WriteFile(file, &attack_team, sizeof(attack_team), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
	int unit_event_handler_quest_refid = (event_handler ? event_handler->GetUnitEventHandlerQuestRefid() : -1);
	WriteFile(file, &unit_event_handler_quest_refid, sizeof(unit_event_handler_quest_refid), &tmp, nullptr);
	WriteFile(file, &weight, sizeof(weight), &tmp, nullptr);
	int guard_refid = (guard_target ? guard_target->refid : -1);
	WriteFile(file, &guard_refid, sizeof(guard_refid), &tmp, nullptr);

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
		ani->Save(file);
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

		if(useable)
		{
			if(useable->user != this)
			{
				WARN(Format("Invalid useable %s (%d) user %s.", useable->GetBase()->id, useable->refid, data->id.c_str()));
				useable = nullptr;
				int refi = -1;
				WriteFile(file, &refi, sizeof(refi), &tmp, nullptr);
			}
			else
				WriteFile(file, &useable->refid, sizeof(useable->refid), &tmp, nullptr);
		}
		else
		{
			int refi = -1;
			WriteFile(file, &refi, sizeof(refi), &tmp, nullptr);
		}

		WriteFile(file, &last_bash, sizeof(last_bash), &tmp, nullptr);
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

inline bool IsEmptySlot(const ItemSlot& slot)
{
	return !slot.item;
}

//=================================================================================================
void Unit::Load(HANDLE file, bool local)
{
	human_data = nullptr;

	// id postaci
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, nullptr);
	data = FindUnitData(BUF);

	// przedmioty
	bool can_sort = true;
	if(LOAD_VERSION >= V_0_2_10)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			ReadString1(file);
			slots[i] = (BUF[0] ? ::FindItem(BUF) : nullptr);
		}
	}
	else
	{
		for(int i=0; i<SLOT_MAX; ++i)
			slots[i] = nullptr;
	}
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	items.resize(ile);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, nullptr);
			ReadFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
			ReadFile(file, &it->team_count, sizeof(it->team_count), &tmp, nullptr);
			if(LOAD_VERSION < V_0_2_10)
			{
				int equipped_as;
				ReadFile(file, &equipped_as, sizeof(equipped_as), &tmp, nullptr);
			}
			if(BUF[0] != '$')
				it->item = ::FindItem(BUF);
			else
			{
				int quest_item_refid;
				ReadFile(file, &quest_item_refid, sizeof(quest_item_refid), &tmp, nullptr);
				Game::Get().AddQuestItemRequest(&it->item, BUF, quest_item_refid, &items, this);
				it->item = QUEST_ITEM_PLACEHOLDER;
				can_sort = false;
			}
		}
		else
		{
			assert(LOAD_VERSION < V_0_2_10);
			it->item = nullptr;
			it->count = 0;
		}
	}

	ReadFile(file, &live_state, sizeof(live_state), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_20 && live_state != ALIVE)
		live_state = LiveState(live_state + 2); // kolejnoœæ siê zmieni³a
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &hp, sizeof(hp), &tmp, nullptr);
	ReadFile(file, &hpmax, sizeof(hpmax), &tmp, nullptr);
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_10)
	{
		int weapon, bow, shield, armor;
		ReadFile(file, &weapon, sizeof(weapon), &tmp, nullptr);
		ReadFile(file, &shield, sizeof(shield), &tmp, nullptr);
		ReadFile(file, &bow, sizeof(bow), &tmp, nullptr);
		ReadFile(file, &armor, sizeof(armor), &tmp, nullptr);
		if(weapon != -1)
		{
			slots[SLOT_WEAPON] = items[weapon].item;
			items[weapon].item = nullptr;
		}
		if(bow != -1)
		{
			slots[SLOT_BOW] = items[bow].item;
			items[bow].item = nullptr;
		}
		if(shield != -1)
		{
			slots[SLOT_SHIELD] = items[shield].item;
			items[shield].item = nullptr;
		}
		if(armor != -1)
		{
			slots[SLOT_ARMOR] = items[armor].item;
			items[armor].item = nullptr;
		}
		RemoveElements(items, IsEmptySlot);
	}
	ReadFile(file, &level, sizeof(level), &tmp, nullptr);
	FileReader f(file);
	if(LOAD_VERSION >= V_0_4)
	{
		stats.Load(f);
		unmod_stats.Load(f);
	}
	else
	{
		for(int i = 0; i < 3; ++i)
			f >> unmod_stats.attrib[i];
		for(int i = 3; i < 6; ++i)
			unmod_stats.attrib[i] = -1;
		for(int i = 0; i< (int)Skill::MAX; ++i)
			unmod_stats.skill[i] = -1;
		int old_skill[(int)OldSkill::MAX];
		f >> old_skill;
		unmod_stats.skill[(int)Skill::ONE_HANDED_WEAPON] = old_skill[(int)OldSkill::WEAPON];
		unmod_stats.skill[(int)Skill::BOW] = old_skill[(int)OldSkill::BOW];
		unmod_stats.skill[(int)Skill::SHIELD] = old_skill[(int)OldSkill::SHIELD];
		unmod_stats.skill[(int)Skill::LIGHT_ARMOR] = old_skill[(int)OldSkill::LIGHT_ARMOR];
		unmod_stats.skill[(int)Skill::HEAVY_ARMOR] = old_skill[(int)OldSkill::HEAVY_ARMOR];
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
	if(LOAD_VERSION < V_0_2_10)
	{
		bool old_auto_talk;
		ReadFile(file, &old_auto_talk, sizeof(old_auto_talk), &tmp, nullptr);
		auto_talk = (old_auto_talk ? 1 : 0);
	}
	else
	{
		ReadFile(file, &auto_talk, sizeof(auto_talk), &tmp, nullptr);
		if(auto_talk == 2)
			ReadFile(file, &auto_talk_timer, sizeof(auto_talk_timer), &tmp, nullptr);
	}
	ReadFile(file, &dont_attack, sizeof(dont_attack), &tmp, nullptr);
	if(LOAD_VERSION == V_0_2)
		attack_team = false;
	else
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
	if(LOAD_VERSION < V_0_2_10)
	{
		weight = 0;
		if(can_sort)
		{
			RemoveNullItems(items);
			RecalculateWeight();
			SortItems(items);
		}
	}
	else
	{
		if(can_sort && LOAD_VERSION < V_0_2_20)
			SortItems(items);
		ReadFile(file, &weight, sizeof(weight), &tmp, nullptr);
		RecalculateWeight();
	}
	if(LOAD_VERSION < V_0_2_10)
		guard_target = nullptr;
	else
	{
		int guard_refid;
		ReadFile(file, &guard_refid, sizeof(guard_refid), &tmp, nullptr);
		if(guard_refid == -1)
			guard_target = nullptr;
		else
		{
			guard_target = (Unit*)guard_refid;
			Game::Get().load_unit_refid.push_back(&guard_target);
		}
	}

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
		human_data = nullptr;

	if(local)
	{
		if(IS_SET(data->flags, F_HUMAN))
			ani = new AnimeshInstance(Game::Get().aHumanBase);
		else
			ani = new AnimeshInstance(data->mesh);
		ani->Load(file);
		ani->ptr = this;
		ReadFile(file, &animation, sizeof(animation), &tmp, nullptr);
		ReadFile(file, &current_animation, sizeof(current_animation), &tmp, nullptr);

		ReadFile(file, &prev_pos, sizeof(prev_pos), &tmp, nullptr);
		ReadFile(file, &speed, sizeof(speed), &tmp, nullptr);
		ReadFile(file, &prev_speed, sizeof(prev_speed), &tmp, nullptr);
		ReadFile(file, &animation_state, sizeof(animation_state), &tmp, nullptr);
		ReadFile(file, &attack_id, sizeof(attack_id), &tmp, nullptr);
		ReadFile(file, &action, sizeof(action), &tmp, nullptr);
		if(LOAD_VERSION < V_0_2_20 && action >= A_EAT)
			action = ACTION(action+1);
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
		if(LOAD_VERSION < V_0_2_10)
			attack_power += 1.f;
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

		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, nullptr);
			used_item = ::FindItem(BUF);
			if(LOAD_VERSION < V_0_2_10)
				used_item_is_team = true;
			else
				ReadFile(file, &used_item_is_team, sizeof(used_item_is_team), &tmp, nullptr);
		}
		else
			used_item = nullptr;

		int refi;
		ReadFile(file, &refi, sizeof(refi), &tmp, nullptr);
		if(refi == -1)
			useable = nullptr;
		else
			Useable::AddRequest(&useable, refi, this);

		if(action == A_SHOOT)
		{
			bow_instance = Game::Get().GetBowInstance(GetBow().mesh);
			bow_instance->Play(&bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = ani->groups[1].speed;
			bow_instance->groups[0].time = ani->groups[1].time;
		}

		if(LOAD_VERSION < V_0_2_10)
			last_bash = 0.f;
		else
			ReadFile(file, &last_bash, sizeof(last_bash), &tmp, nullptr);
	}
	else
	{
		ani = nullptr;
		ai = nullptr;
		useable = nullptr;
		used_item = nullptr;
		weapon_state = WS_HIDDEN;
		weapon_taken = W_NONE;
		weapon_hiding = W_NONE;
		frozen = 0;
		talking = false;
		animation = current_animation = ANI_STAND;
		action = A_NONE;
		run_attack = false;
		hurt_timer = 0.f;
		speed = prev_speed = 0.f;
		alcohol = 0.f;
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
		human_data->ApplyScale(ani->ani);

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
	frozen = 0;

	// fizyka
	if(local)
	{
		btCapsuleShape* caps = new btCapsuleShape(GetUnitRadius(), max(MIN_H, GetUnitHeight()));
		cobj = new btCollisionObject;
		cobj->setCollisionShape(caps);
		cobj->setUserPointer(this);
		cobj->setCollisionFlags(CG_UNIT);
		Game::Get().phy_world->addCollisionObject(cobj);
		Game::Get().UpdateUnitPhysics(*this, IsAlive() ? pos : VEC3(1000,1000,1000));
	}
	else
		cobj = nullptr;

	// konwersja ekwipunku z V0
	if(LOAD_VERSION == V_0_2 && IS_SET(data->flags2, F2_UPDATE_V0_ITEMS))
	{
		ClearInventory();
		Game::Get().ParseItemScript(*this, data->items, data->new_items);
	}

	// zabezpieczenie
	if(((weapon_state == WS_TAKEN || weapon_state == WS_TAKING) && weapon_taken == W_NONE) ||
		(weapon_state == WS_HIDING && weapon_hiding == W_NONE))
	{
		weapon_state = WS_HIDDEN;
		weapon_taken = W_NONE;
		weapon_hiding = W_NONE;
		WARN(Format("Unit '%s' had broken weapon state.", data->id.c_str()));
	}

	// calculate new attributes
	if(LOAD_VERSION < V_0_4)
	{
		UnitData* ud = data;
		if(IsPlayer())
			level = CalculateLevel();

		StatProfile& profile = ud->GetStatProfile();
		profile.SetForNew(level, unmod_stats);
		CalculateStats();

		if(IsPlayer())
		{
			profile.Set(0, player->base_stats);
			player->SetRequiredPoints();
		}
	}
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
// szuka miksturek leczniczych w ekwipunku, zwraca -1 jeœli nie odnaleziono
int Unit::FindHealingPotion() const
{
	float missing = hpmax - hp, heal=0, heal2=inf();
	int id = -1, id2 = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item && it->item->type == IT_CONSUMEABLE)
		{
			const Consumeable& pot = it->item->ToConsumeable();
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
	int index = 0;
	for(ItemSlot& item_slot : items)
	{
		if(!item_slot.item)
		{
			++index;
			continue;
		}

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

		++index;
	}
	if(changes)
	{
		RemoveNullItems(items);
		SortItems(items);
	}

	// add item if unit have none
	if(type == HUMANOID && !HaveWeapon())
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
	if(wep)
	{
		const WeaponTypeInfo& info = wep->GetInfo();

		float mod = 1.f + float(Get(Skill::ONE_HANDED_WEAPON) + Get(info.skill)) / 400 + info.dex_speed*Get(Attribute::DEX) - GetAttackSpeedModFromStrength(*wep);

		if(IsPlayer())
			mod -= GetAttackSpeedModFromLoad();

		if(mod < 0.1f)
			mod = 0.1f;

		return GetWeapon().GetInfo().base_speed * mod;
	}
	else
		return 1.f + float(Get(Skill::ONE_HANDED_WEAPON) + Get(Skill::UNARMED)) / 400 + 0.001f*Get(Attribute::DEX);
}

//=================================================================================================
float Unit::GetBowAttackSpeed() const
{
	float mod = 0.8f + float(Get(Skill::BOW)) / 200 + 0.004f*Get(Attribute::DEX) - GetAttackSpeedModFromStrength(GetBow());
	if(IsPlayer())
		mod -= GetAttackSpeedModFromLoad();
	if(mod < 0.25f)
		mod = 0.25f;
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
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
void Unit::RemovePoison()
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
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
int Unit::FindItem(const Item* item, int refid) const
{
	assert(item);

	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(slots[i] == item && (refid == -1 || slots[i]->IsQuest(refid)))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item && (refid == -1 || it->item->IsQuest(refid)))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
int Unit::FindQuestItem(int quest_refid) const
{
	for(int i = 0; i<SLOT_MAX; ++i)
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
			NetChange& c = Add1(Game::Get().net_changes);
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
	for(int i=0; i<SLOT_MAX; ++i)
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
VEC3 Unit::GetEyePos() const
{
	const Animesh::Point& point = *ani->ani->GetPoint("oczy");
	MATRIX matBone = point.mat * ani->mat_bones[point.bone];

	MATRIX matPos, matRot;
	D3DXMatrixTranslation(&matPos, pos);
	D3DXMatrixRotationY(&matRot, rot);

	matBone = matBone * (matRot * matPos);

	VEC3 pos(0,0,0), out;
	D3DXVec3TransformCoord(&out, &pos, &matBone);

	return out;
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

	// zrêcznoœæ
	if(load < 1.f)
	{
		int dex = Get(Attribute::DEX);
		if(dex > 50)
			return (float(dex - 50) / 3) * (1.f-load) * mod;
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
const Item* Unit::GetIIndexItem( int i_index ) const
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
Animesh::Animation* Unit::GetTakeWeaponAnimation(bool melee) const
{
	if(melee)
	{
		if(HaveShield())
			return ani->ani->GetAnimation(NAMES::ani_take_weapon);
		else
		{
			Animesh::Animation* anim = ani->ani->GetAnimation(NAMES::ani_take_weapon_no_shield);
			if(!anim)
			{
				// brak animacja wyci¹gania broni bez tarczy, u¿yj zwyk³ej
				return ani->ani->GetAnimation(NAMES::ani_take_weapon);
			}
			else
				return anim;
		}
	}
	else
		return ani->ani->GetAnimation(NAMES::ani_take_bow);
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
		}
	}
	
	if(alcohol/hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

//=================================================================================================
int Unit::CalculateLevel()
{
	if(player)
		return CalculateLevel(player->clas);
	else
		return level;
}

//=================================================================================================
int Unit::CalculateLevel(Class clas)
{
	UnitData* ud = g_classes[(int)clas].unit_data;

	float tlevel = 0.f;
	float weight_sum = 0.f;
	StatProfile& profile = ud->GetStatProfile();

	// calculate player level based on attributes and skills that are important for that class
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		int base = profile.attrib[i] - 50;
		if(base > 0 && unmod_stats.attrib[i] > 0)
		{
			int dif = unmod_stats.attrib[i] - profile.attrib[i], weight;
			float mod = AttributeInfo::GetModifier(base, weight);
			tlevel += (float(dif) / mod) * weight * 5;
			weight_sum += weight;
		}
	}

	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		int base = profile.skill[i];
		if(base > 0 && unmod_stats.skill[i] > 0)
		{
			int dif = unmod_stats.skill[i] - base, weight;
			float mod = SkillInfo::GetModifier(base, weight);
			tlevel += (float(dif) / mod) * weight * 5;
			weight_sum += weight;
		}
	}

	return (int)floor(tlevel / weight_sum);
}

//=================================================================================================
void Unit::RecalculateStat(Attribute a, bool apply)
{
	int id = (int)a;
	int old = stats.attrib[id];
	StatState state;

	// calculate value = base + effect modifiers
	int value = unmod_stats.attrib[id] + GetEffectModifier(EffectType::Attribute, id, (IsPlayer() ? &state : nullptr));

	if(value == old)
		return;

	// apply new value
	stats.attrib[id] = value;
	if(IsPlayer())
		player->attrib_state[id] = state;

	if(apply)
		ApplyStat(a, old, true);
}

//=================================================================================================
void Unit::ApplyStat(Attribute a, int old, bool calculate_skill)
{
	// recalculate other stats
	switch(a)
	{
	case Attribute::STR:
		{
			// hp depends on str
			RecalculateHp();
			if(!fake_unit)
			{
				Game& game = Game::Get();
				if(game.IsServer())
				{
					NetChange& c = Add1(game.net_changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = this;
				}
			}
			// max load depends on str
			CalculateLoad();
		}
		break;
	case Attribute::END:
		{
			// hp depends on end
			RecalculateHp();
			if(!fake_unit)
			{
				Game& game = Game::Get();
				if(game.IsServer())
				{
					NetChange& c = Add1(game.net_changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = this;
				}
			}
		}
		break;
	case Attribute::DEX:
	case Attribute::INT:
	case Attribute::WIS:
	case Attribute::CHA:
		break;
	}

	if(calculate_skill)
	{
		// recalculate skills bonuses
		int old_mod = old / 10;
		int mod = stats.attrib[(int)a] / 10;
		if(mod != old_mod)
		{
			for(SkillInfo& si : g_skills)
			{
				if(si.attrib == a || si.attrib2 == a)
					RecalculateStat(si.skill_id, true);
			}
		}
	}
}

//=================================================================================================
void Unit::RecalculateStat(Skill s, bool apply)
{
	int id = (int)s;
	int old = stats.skill[id];
	StatState state;

	// calculate value
	int value = unmod_stats.skill[id];

	// apply effect modifiers
	ValueBuffer buf;
	SkillInfo& info = g_skills[id];
	/*for(Effect2& e : effects2)
	{
		if(e.e->type == EffectType::Skill)
		{
			if(e.e->a == id)
				buf.Add(e.e->b);
		}
		else if(e.e->type == EffectType::SkillPack)
		{
			if(e.e->a == (int)info.pack)
				buf.Add(e.e->b);
		}
	}*/
	if(IsPlayer())
		value += buf.Get(state);
	else
		value += buf.Get();

	// apply attributes bonus
	if(info.attrib2 == Attribute::NONE)
		value += Get(info.attrib) / 10;
	else
		value += Get(info.attrib) / 20 + Get(info.attrib2) / 20;

	if(!apply)
	{
		stats.skill[id] = value;
		return;
	}

	// apply skill synergy
	//int type = 0;
	switch(s)
	{
	case Skill::LIGHT_ARMOR:
	case Skill::HEAVY_ARMOR:
		{
			int other_val = Get(Skill::MEDIUM_ARMOR);
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 1;
		}
		break;
	case Skill::MEDIUM_ARMOR:
		{
			int other_val = max(Get(Skill::LIGHT_ARMOR), Get(Skill::HEAVY_ARMOR));
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 1;
		}
		break;
	case Skill::SHORT_BLADE:
		{
			int other_val = max(max(Get(Skill::LONG_BLADE), Get(Skill::BLUNT)), Get(Skill::AXE));
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 2;
		}
		break;
	case Skill::LONG_BLADE:
		{
			int other_val = max(max(Get(Skill::SHORT_BLADE), Get(Skill::BLUNT)), Get(Skill::AXE));
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 2;
		}
		break;
	case Skill::BLUNT:
		{
			int other_val = max(max(Get(Skill::LONG_BLADE), Get(Skill::SHORT_BLADE)), Get(Skill::AXE));
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 2;
		}
		break;
	case Skill::AXE:
		{
			int other_val = max(max(Get(Skill::LONG_BLADE), Get(Skill::BLUNT)), Get(Skill::SHORT_BLADE));
			if(other_val > value)
				value += (other_val - value) / 2;
			//type = 2;
		}
		break;
	}

	if(value == old)
		return;

	stats.skill[id] = value;
	if(IsPlayer())
		player->skill_state[id] = state;
}

//=================================================================================================
void Unit::CalculateStats()
{
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		RecalculateStat((Attribute)i, false);
	for(int i = 0; i < (int)Skill::MAX; ++i)
		RecalculateStat((Skill)i, false);

	for(int i = 0; i < (int)Attribute::MAX; ++i)
		ApplyStat((Attribute)i, -1, false);
	for(int i = 0; i < (int)Skill::MAX; ++i)
		RecalculateStat((Skill)i, true);
}

//=================================================================================================
int Unit::GetEffectModifier(EffectType type, int id, StatState* state) const
{
	ValueBuffer buf;

	/*for(const Effect2& e : effects2)
	{
		if(e.e->type == type && e.e->a == id)
			buf.Add(e.e->b);
	}*/

	if(state)
		return buf.Get(*state);
	else
		return buf.Get();
}

//=================================================================================================
int Unit::CalculateMobility() const
{
	if(HaveArmor())
		return CalculateMobility(GetArmor());
	else
		return Get(Attribute::DEX);
}

//=================================================================================================
int Unit::CalculateMobility(const Armor& armor) const
{
	int dex = Get(Attribute::DEX);
	int str = Get(Attribute::STR);
	float dexf = (float)dex;
	if(armor.req_str > str)
		dexf *= float(str) / armor.req_str;

	int max_dex;
	switch(armor.skill)
	{
	case Skill::LIGHT_ARMOR:
	default:
		max_dex = int((1.f + float(Get(Skill::LIGHT_ARMOR)) / 100)*armor.mobility);
		break;
	case Skill::MEDIUM_ARMOR:
		max_dex = int((1.f + float(Get(Skill::MEDIUM_ARMOR)) / 150)*armor.mobility);
		break;
	case Skill::HEAVY_ARMOR:
		max_dex = int((1.f + float(Get(Skill::HEAVY_ARMOR)) / 200)*armor.mobility);
		break;
	}

	if(dexf > (float)max_dex)
		return max_dex + int((dexf - max_dex) * ((float)max_dex / dexf));

	return (int)dexf;
}

//=================================================================================================
int Unit::Get(SubSkill ss) const
{
	int id = (int)ss;
	SubSkillInfo& info = g_sub_skills[id];
	int v = Get(info.skill);
	ValueBuffer buf;

	/*for(const Effect2& e : effects2)
	{
		if(e.e->type == EffectType::SubSkill && e.e->a == id)
			buf.Add(e.e->b);
	}*/

	return v + buf.Get();
}

struct TMod
{
	float str, end, dex;
};

//=================================================================================================
Skill Unit::GetBestWeaponSkill() const
{
	const Skill weapon_skills[] = {
		Skill::SHORT_BLADE,
		Skill::LONG_BLADE,
		Skill::AXE,
		Skill::BLUNT
	};

	const TMod weapon_mod[] = {
		0.5f, 0, 0.5f,
		0.75f, 0, 0.25f,
		0.85f, 0, 0.15f,
		0.8f, 0, 0.2f
	};

	Skill best = Skill::NONE;
	int val = 0, val2 = 0, index = 0;

	for(Skill s : weapon_skills)
	{
		int s_val = Get(s);
		if(s_val >= val)
		{
			int s_val2 = int(weapon_mod[index].str * Get(Attribute::STR) + weapon_mod[index].dex * Get(Attribute::DEX));
			if(s_val2 > val2)
			{
				val = s_val;
				val2 = s_val2;
				best = s;
			}
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

	const TMod armor_mod[] = {
		0, 0, 1,
		0, 0.5f, 0.5f,
		0.5f, 0.5f, 0
	};

	Skill best = Skill::NONE;
	int val = 0, val2 = 0, index = 0;

	for(Skill s : armor_skills)
	{
		int s_val = Get(s);
		if(s_val >= val)
		{
			int s_val2 = int(armor_mod[index].str * Get(Attribute::STR)
				+ armor_mod[index].end * Get(Attribute::END)
				+ armor_mod[index].dex * Get(Attribute::DEX));
			if(s_val2 > val2)
			{
				val = s_val;
				val2 = s_val2;
				best = s;
			}
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
		ani->SetToEnd(anim_name, mat_scale);
	else
		ani->SetToEnd(mat_scale);
}
