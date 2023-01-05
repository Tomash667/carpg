#include "Pch.h"
#include "Unit.h"

#include "Ability.h"
#include "AIController.h"
#include "AITeam.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "City.h"
#include "CombatHelper.h"
#include "Content.h"
#include "CraftPanel.h"
#include "Electro.h"
#include "EntityInterpolator.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "GroundItem.h"
#include "InsideLocation.h"
#include "Inventory.h"
#include "Level.h"
#include "LevelGui.h"
#include "LevelPart.h"
#include "OffscreenLocation.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "Quest2.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Mages.h"
#include "Quest_Secret.h"
#include "ScriptException.h"
#include "Stock.h"
#include "Team.h"
#include "UnitEventHandler.h"
#include "UnitHelper.h"
#include "UnitList.h"
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
const float Unit::COUGHS_SOUND_DIST = 1.f;
EntityType<Unit>::Impl EntityType<Unit>::impl;
static AIController* AI_PLACEHOLDER = (AIController*)1;
vector<int> effectsToRemove;

//=================================================================================================
Unit::~Unit()
{
	if(order)
		order->Free();
	if(bowInstance)
		gameLevel->FreeBowInstance(bowInstance);
	delete meshInst;
	delete humanData;
	delete hero;
	delete player;
	delete stock;
	if(stats && !stats->fixed)
		delete stats;
	if(action == A_DASH && act.dash.hit)
		act.dash.hit->Free();
}

//=================================================================================================
void Unit::Release()
{
	if(--refs == 0)
	{
		assert(toRemove);
		delete this;
	}
}

//=================================================================================================
void Unit::Init(UnitData& base, int lvl)
{
	Register();
	fakeUnit = true; // to prevent sending MP message set temporary as fake unit

	// stats
	data = &base;
	humanData = nullptr;
	pos = Vec3(0, 0, 0);
	rot = 0.f;
	usedItem = nullptr;
	liveState = ALIVE;
	for(int i = 0; i < SLOT_MAX; ++i)
		slots[i] = nullptr;
	action = A_NONE;
	weaponTaken = W_NONE;
	weaponHiding = W_NONE;
	weaponState = WeaponState::Hidden;
	if(lvl == -2)
		level = base.level.Random();
	else if(lvl == -3)
		level = base.level.Clamp(gameLevel->location->st);
	else
		level = base.level.Clamp(lvl);
	player = nullptr;
	ai = nullptr;
	speed = prevSpeed = 0.f;
	hurtTimer = 0.f;
	talking = false;
	usable = nullptr;
	frozen = FROZEN::NO;
	inArena = -1;
	eventHandler = nullptr;
	toRemove = false;
	temporary = false;
	questId = -1;
	bubble = nullptr;
	busy = Busy_No;
	interp = nullptr;
	dontAttack = false;
	assist = false;
	attackTeam = false;
	lastBash = 0.f;
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
	stamina = staminaMax = CalculateMaxStamina();
	staminaTimer = 0;

	// items
	weight = 0;
	CalculateLoad();
	if(base.group != G_PLAYER && base.itemScript)
	{
		ItemScript* script = base.itemScript;
		if(base.statProfile && !base.statProfile->subprofiles.empty() && base.statProfile->subprofiles[stats->subprofile.index]->itemScript)
			script = base.statProfile->subprofiles[stats->subprofile.index]->itemScript;
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
		humanData = new Human;
		humanData->Init(base.appearance);
	}

	// hero data
	if(IsSet(base.flags, F_HERO))
	{
		hero = new Hero;
		hero->Init(*this);
	}
	else
		hero = nullptr;

	fakeUnit = false;
}

//=================================================================================================
float Unit::CalculateMaxHp() const
{
	float maxhp = (float)data->hp + GetEffectSum(EffectId::Health);
	if(IsSet(data->flags2, F2_FIXED_STATS))
		maxhp += data->hpLvl * (level - data->level.x);
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
		maxmp += data->mpLvl * (level - data->level.x);
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
		return (float)(data->attack + data->attackLvl * (level - data->level.x));
	else
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
		return attack + data->attackLvl * (level - data->level.x);
	}

	int str = Get(AttributeId::STR),
		dex = Get(AttributeId::DEX);

	if(weapon->type == IT_WEAPON)
	{
		const Weapon& w = weapon->ToWeapon();
		const WeaponTypeInfo& wi = WeaponTypeInfo::info[w.weaponType];
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
			float skillVal = (float)Get(a.GetSkill());
			int str = Get(AttributeId::STR);
			if(str < a.reqStr)
				skillVal *= float(str) / a.reqStr;
			def += a.def + skillVal;
		}
	}
	else
		def += data->defLvl * (level - data->level.x);
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
void Unit::SetGold(int newGold)
{
	if(newGold == gold)
		return;
	int dif = newGold - gold;
	gold = newGold;
	if(IsPlayer())
	{
		gameGui->messages->AddFormattedMessage(player, GMS_GOLD_ADDED, -1, dif);
		if(player->isLocal)
			soundMgr->PlaySound2d(gameRes->sCoins);
		else
		{
			NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::SOUND);
			c.id = 0;
			player->playerInfo->UpdateGold();
		}
	}
}

//=================================================================================================
bool Unit::CanWear(const Item* item) const
{
	if(item->IsWearable())
	{
		if(item->type == IT_ARMOR)
			return item->ToArmor().armorUnitType == data->armorType;
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
void Unit::ReplaceItem(ITEM_SLOT slot, const Item* item)
{
	assert(Net::IsClient()); // TODO
	if(item)
		gameRes->PreloadItem(item);
	slots[slot] = item;
}

//=================================================================================================
void Unit::ReplaceItems(array<const Item*, SLOT_MAX>& items)
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(items[i])
			gameRes->PreloadItem(items[i]);
		slots[i] = items[i];
	}
}

//=================================================================================================
bool Unit::DropItem(int index, uint count)
{
	bool noMore = false;
	ItemSlot& s = items[index];
	if(count == 0)
		count = s.count;
	else
		count = min(count, s.count);
	s.count -= count;

	weight -= s.item->weight * count;

	action = A_ANIMATION;
	meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		GroundItem* groundItem = new GroundItem;
		groundItem->Register();
		groundItem->item = s.item;
		groundItem->count = count;
		groundItem->teamCount = min(count, s.teamCount);
		s.teamCount -= groundItem->teamCount;
		groundItem->pos = pos;
		groundItem->pos.x -= sin(rot) * 0.25f;
		groundItem->pos.z -= cos(rot) * 0.25f;
		groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
		if(s.count == 0)
		{
			noMore = true;
			items.erase(items.begin() + index);
		}
		if(!questMgr->questSecret->CheckMoonStone(groundItem, *this))
			locPart->AddGroundItem(groundItem);

		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::DROP_ITEM);
			c.unit = this;
		}
	}
	else
	{
		s.teamCount -= min(count, s.teamCount);
		if(s.count == 0)
		{
			noMore = true;
			items.erase(items.begin() + index);
		}

		NetChange& c = Net::PushChange(NetChange::DROP_ITEM);
		c.id = index;
		c.count = count;
	}

	return noMore;
}

//=================================================================================================
void Unit::DropItem(ITEM_SLOT slot)
{
	assert(slots[slot]);
	const Item*& item = slots[slot];

	weight -= item->weight;

	action = A_ANIMATION;
	meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		RemoveItemEffects(item, slot);

		GroundItem* groundItem = new GroundItem;
		groundItem->Register();
		groundItem->item = item;
		groundItem->count = 1;
		groundItem->teamCount = 0;
		groundItem->pos = pos;
		groundItem->pos.x -= sin(rot) * 0.25f;
		groundItem->pos.z -= cos(rot) * 0.25f;
		groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
		item = nullptr;
		locPart->AddGroundItem(groundItem);

		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::DROP_ITEM);
			c.unit = this;

			if(IsVisible(slot))
			{
				NetChange& c2 = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
				c2.unit = this;
				c2.id = slot;
			}
		}
	}
	else
	{
		item = nullptr;

		NetChange& c = Net::PushChange(NetChange::DROP_ITEM);
		c.id = SlotToIIndex(slot);
		c.count = 1;
	}
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
		!((action == A_ATTACK && animationState == AS_ATTACK_PREPARE)
		|| (action == A_SHOOT && animationState == AS_SHOOT_PREPARE)))
	{
		if(action == A_TAKE_WEAPON && weaponState == WeaponState::Hiding)
		{
			// jeœli chowa broñ to u¿yj miksturki jak schowa
			if(IsPlayer())
			{
				if(player->isLocal)
				{
					player->nextAction = NA_CONSUME;
					player->nextActionData.index = index;
					player->nextActionData.item = slot.item;
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
	if(weaponState != WeaponState::Hidden)
	{
		HideWeapon();
		if(IsPlayer())
		{
			assert(player->isLocal);
			player->nextAction = NA_CONSUME;
			player->nextActionData.index = index;
			player->nextActionData.item = slot.item;
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
	if(slot.teamCount)
	{
		--slot.teamCount;
		usedItemIsTeam = true;
	}
	else
		usedItemIsTeam = false;
	if(slot.count == 0)
	{
		items.erase(items.begin() + index);
		removed = true;
	}

	ConsumeItemAnim(cons);

	// wyœlij komunikat
	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::CONSUME_ITEM);
		if(Net::IsServer())
		{
			c.unit = this;
			c.id = (int)usedItem;
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

	usedItemIsTeam = true;
	ConsumeItemAnim(item);

	if(send)
	{
		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::CONSUME_ITEM);
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
	cstring animName;
	if(Any(cons.subtype, Consumable::Subtype::Food, Consumable::Subtype::Herb))
	{
		action = A_EAT;
		animationState = AS_EAT_START;
		animName = "je";
	}
	else
	{
		action = A_DRINK;
		animationState = AS_DRINK_START;
		animName = "pije";
	}

	if(currentAnimation == ANI_PLAY && !usable)
	{
		animation = ANI_STAND;
		currentAnimation = ANI_NONE;
	}

	meshInst->Play(animName, PLAY_ONCE | PLAY_PRIO1, 1);
	gameRes->PreloadItem(&cons);
	usedItem = &cons;
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
bool Unit::AddItem(const Item* item, uint count, uint teamCount)
{
	assert(item && count != 0 && teamCount <= count);

	if(item->type == IT_GOLD && team->IsTeamMember(*this))
	{
		if(Net::IsLocal())
		{
			if(teamCount && IsTeamMember())
			{
				team->AddGold(teamCount);
				uint normalGold = count - teamCount;
				if(normalGold)
				{
					gold += normalGold;
					if(IsPlayer() && !player->isLocal)
						player->playerInfo->UpdateGold();
				}
			}
			else
			{
				gold += count;
				if(IsPlayer() && !player->isLocal)
					player->playerInfo->UpdateGold();
			}
		}
		return true;
	}

	weight += item->weight * count;

	return InsertItem(items, item, count, teamCount);
}

//=================================================================================================
void Unit::AddItem2(const Item* item, uint count, uint teamCount, bool showMsg, bool notify)
{
	assert(item && count && teamCount <= count);

	gameRes->PreloadItem(item);

	AddItem(item, count, teamCount);

	// multiplayer notify
	if(notify && Net::IsServer())
	{
		if(IsPlayer())
		{
			if(!player->isLocal)
			{
				NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::ADD_ITEMS);
				c.item = item;
				c.count = count;
				c.id = teamCount;
			}
		}
		else
		{
			// check if unit is trading with someone that gets this item
			Unit* u = nullptr;
			for(Unit& member : team->activeMembers)
			{
				if(member.IsPlayer() && member.player->IsTradingWith(this))
				{
					u = &member;
					break;
				}
			}

			if(u && !u->player->isLocal)
			{
				NetChangePlayer& c = u->player->playerInfo->PushChange(NetChangePlayer::ADD_ITEMS_TRADER);
				c.item = item;
				c.id = id;
				c.count = count;
				c.a = teamCount;
			}
		}
	}

	if(showMsg && IsPlayer())
		gameGui->messages->AddItemMessage(player, count);

	// rebuild inventory
	int rebuildId = -1;
	if(IsLocalPlayer())
	{
		if(gameGui->inventory->invMine->visible || gameGui->inventory->gpTrade->visible)
			rebuildId = 0;
	}
	else if(gameGui->inventory->gpTrade->visible && gameGui->inventory->invTradeOther->unit == this)
		rebuildId = 1;
	if(rebuildId != -1)
		gameGui->inventory->BuildTmpInventory(rebuildId);
}

//=================================================================================================
void Unit::AddEffect(Effect& e, bool send)
{
	if(Net::IsLocal())
	{
		switch(e.effect)
		{
		case EffectId::Stun:
			BreakAction();
			if(IsSet(data->flags2, F2_STUN_RESISTANCE))
				e.time /= 2;
			break;
		case EffectId::Rooted:
			if(IsSet(data->flags2, F2_ROOTED_RESISTANCE))
				e.time /= 2;
			break;
		}
	}

	effects.push_back(e);
	OnAddRemoveEffect(e);

	if(send && Net::IsServer())
	{
		if(player && !player->IsLocal())
		{
			NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::ADD_EFFECT);
			c.id = (int)e.effect;
			c.count = (int)e.source;
			c.a1 = e.sourceId;
			c.a2 = e.value;
			c.pos.x = e.power;
			c.pos.y = e.time;
		}

		if(e.IsVisible())
		{
			NetChange& c = Net::PushChange(NetChange::ADD_UNIT_EFFECT);
			c.unit = this;
			c.id = (int)e.effect;
			c.extraFloat = e.time;
		}
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
		if(e.onAttack)
			continue;
		Effect effect;
		effect.effect = e.effect;
		effect.power = e.power;
		effect.source = EffectSource::Item;
		effect.sourceId = (int)slot;
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
					NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
					c.unit = this;
				}
			}
			break;
		case EffectId::Poison:
		case EffectId::Alcohol:
			{
				float poisonRes = GetPoisonResistance();
				if(poisonRes > 0.f)
				{
					Effect e;
					e.effect = effect.effect;
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					e.value = -1;
					e.time = item.time;
					e.power = effect.power / item.time * poisonRes;
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
						effectsToRemove.push_back(index);
				}

				RemoveEffects();

				if(alcohol != 0.f)
				{
					alcohol = 0.f;
					if(IsPlayer() && !player->isLocal)
						player->playerInfo->updateFlags |= PlayerInfo::UF_ALCOHOL;
				}
			}
			break;
		case EffectId::FoodRegeneration:
			{
				Effect e;
				e.effect = effect.effect;
				e.source = EffectSource::Temporary;
				e.sourceId = -1;
				e.value = -1;
				e.time = item.time;
				e.power = effect.power / item.time;
				AddEffect(e);
			}
			break;
		case EffectId::GreenHair:
			if(humanData)
			{
				humanData->hairColor = Vec4(0, 1, 0, 1);
				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::HAIR_COLOR);
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
					NetChange& c = Net::PushChange(NetChange::UPDATE_MP);
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
				e.sourceId = -1;
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
uint Unit::RemoveEffects(EffectId effect, EffectSource source, int sourceId, int value)
{
	uint index = 0;
	for(Effect& e : effects)
	{
		if((effect == EffectId::None || e.effect == effect)
			&& (value == -1 || e.value == value)
			&& (source == EffectSource::None || e.source == source)
			&& (sourceId == -1 || e.sourceId == sourceId))
			effectsToRemove.push_back(index);
		++index;
	}

	uint count = effectsToRemove.size();
	RemoveEffects();
	return count;
}

//=================================================================================================
void Unit::UpdateEffects(float dt)
{
	float regen = 0.f, tmpRegen = 0.f, poisonDmg = 0.f, alcoSum = 0.f, bestStamina = 0.f, staminaMod = 1.f, foodHeal = 0.f;

	// update effects timer
	for(uint index = 0, count = effects.size(); index < count; ++index)
	{
		Effect& effect = effects[index];
		if(effect.effect == EffectId::NaturalHealingMod)
			continue;

		switch(effect.effect)
		{
		case EffectId::Regeneration:
			if(effect.source == EffectSource::Temporary && effect.power > 0.f)
			{
				if(effect.power > tmpRegen)
					tmpRegen = effect.power;
			}
			else
				regen += effect.power;
			break;
		case EffectId::Poison:
			poisonDmg += effect.power;
			break;
		case EffectId::Alcohol:
			alcoSum += effect.power;
			break;
		case EffectId::FoodRegeneration:
			foodHeal += effect.power;
			break;
		case EffectId::StaminaRegeneration:
			if(effect.power > bestStamina)
				bestStamina = effect.power;
			break;
		case EffectId::StaminaRegenerationMod:
			staminaMod *= effect.power;
			break;
		case EffectId::SlowMove:
			effect.power = effect.time;
			break;
		}

		if(effect.source == EffectSource::Temporary)
		{
			if((effect.time -= dt) <= 0.f)
			{
				effectsToRemove.push_back(index);
				if(Net::IsLocal() && effect.effect == EffectId::Rooted && effect.value == EffectValue_Rooted_Vines)
				{
					Effect e;
					e.effect = EffectId::SlowMove;
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					e.power = 1.f;
					e.time = 1.f;
					e.value = -1;
					AddEffect(e);
				}
			}
		}
	}

	// remove expired effects
	RemoveEffects(false);

	if(Net::IsClient())
		return;

	// healing from food / regeneration
	if(hp != hpmax && (regen > 0 || tmpRegen > 0 || foodHeal > 0))
	{
		float natural = GetEffectMul(EffectId::NaturalHealingMod);
		hp += ((regen + tmpRegen) + natural * foodHeal) * dt;
		if(hp > hpmax)
			hp = hpmax;
		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
			c.unit = this;
		}
	}

	// update alcohol value
	if(alcoSum > 0.f)
	{
		alcohol += alcoSum * dt;
		if(alcohol >= hpmax && liveState == ALIVE)
			Fall();
		if(IsPlayer() && !player->isLocal)
			player->playerInfo->updateFlags |= PlayerInfo::UF_ALCOHOL;
	}
	else if(alcohol != 0.f)
	{
		alcohol -= dt / 10 * Get(AttributeId::END);
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && !player->isLocal)
			player->playerInfo->updateFlags |= PlayerInfo::UF_ALCOHOL;
	}

	// update poison damage
	if(poisonDmg != 0.f)
		GiveDmg(poisonDmg * dt, nullptr, nullptr, DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(Net::IsOnline() && !player->isLocal && player->lastDmgPoison != poisonDmg)
			player->playerInfo->updateFlags |= PlayerInfo::UF_POISON_DAMAGE;
		player->lastDmgPoison = poisonDmg;
	}

	// restore mana
	if(mp != mpmax)
	{
		mp += GetMpRegen() * dt;
		if(mp > mpmax)
			mp = mpmax;
		if(Net::IsServer() && IsTeamMember())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_MP);
			c.unit = this;
		}
	}

	// restore stamina
	if(staminaTimer > 0)
	{
		staminaTimer -= dt;
		if(bestStamina > 0.f && stamina != staminaMax)
		{
			stamina += bestStamina * dt * staminaMod;
			if(stamina > staminaMax)
				stamina = staminaMax;
			if(Net::IsServer() && IsTeamMember())
			{
				NetChange& c = Net::PushChange(NetChange::UPDATE_STAMINA);
				c.unit = this;
			}
		}
	}
	else if(stamina != staminaMax && (staminaAction != SA_DONT_RESTORE || bestStamina > 0.f))
	{
		float staminaRestore;
		switch(staminaAction)
		{
		case SA_RESTORE_MORE:
			staminaRestore = 66.66f;
			break;
		case SA_RESTORE:
		default:
			staminaRestore = 33.33f;
			break;
		case SA_RESTORE_SLOW:
			staminaRestore = 20.f;
			break;
		case SA_DONT_RESTORE:
			staminaRestore = 0.f;
			break;
		}
		switch(GetLoadState())
		{
		case LS_NONE:
		case LS_LIGHT:
		case LS_MEDIUM:
			break;
		case LS_HEAVY:
			staminaRestore -= 1.f;
			break;
		case LS_OVERLOADED:
			staminaRestore -= 2.5f;
			break;
		case LS_MAX_OVERLOADED:
			staminaRestore -= 5.f;
			break;
		}
		if(staminaRestore < 0)
			staminaRestore = 0;
		stamina += ((staminaMax * staminaRestore / 100) + bestStamina) * dt * staminaMod;
		if(stamina > staminaMax)
			stamina = staminaMax;
		if(Net::IsServer() && IsTeamMember())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_STAMINA);
			c.unit = this;
		}
	}
}

//=================================================================================================
void Unit::EndEffects(int days, float* outNaturalMod)
{
	alcohol = 0.f;
	stamina = staminaMax;
	staminaTimer = 0;
	mp = mpmax;

	if(effects.empty())
	{
		if(outNaturalMod)
			*outNaturalMod = 1.f;
		return;
	}

	uint index = 0;
	float bestReg = 0.f, food = 0.f, naturalMod = 1.f, naturalTmpMod = 1.f;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		bool remove = true;
		if(it->source != EffectSource::Temporary)
			remove = false;
		switch(it->effect)
		{
		case EffectId::Regeneration:
			if(it->source == EffectSource::Permanent)
				bestReg = -1.f;
			else if(bestReg >= 0.f)
			{
				float regen = it->power * it->time;
				if(regen > bestReg)
					bestReg = regen;
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
				if(val > naturalTmpMod)
					naturalTmpMod = val;
			}
			else
				naturalMod *= it->power;
			it->time -= days;
			if(it->time > 0.f)
				remove = false;
			break;
		}
		if(remove)
			effectsToRemove.push_back(index);
	}

	naturalMod *= naturalTmpMod;
	if(outNaturalMod)
		*outNaturalMod = naturalMod;

	if(bestReg < 0.f)
		hp = hpmax; // hp regen from perk - full heal
	else
	{
		hp += bestReg + food * naturalMod;
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
		tmpValue = 0.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			if(e.source == EffectSource::Temporary && e.power > 0.f)
			{
				if(e.power > tmpValue)
					tmpValue = e.power;
			}
			else
				value += e.power;
		}
	}
	return value + tmpValue;
}

//=================================================================================================
float Unit::GetEffectMul(EffectId effect) const
{
	float value = 1.f,
		tmpValueLow = 1.f,
		tmpValueHigh = 1.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			if(e.source == EffectSource::Temporary)
			{
				if(e.power > tmpValueHigh)
					tmpValueHigh = e.power;
				else if(e.power < tmpValueLow)
					tmpValueLow = e.power;
			}
			else
				value *= e.power;
		}
	}
	return value * tmpValueLow * tmpValueHigh;
}

//=================================================================================================
float Unit::GetEffectMulInv(EffectId effect) const
{
	float value = 1.f,
		tmpValueLow = 1.f,
		tmpValueHigh = 1.f;
	for(const Effect& e : effects)
	{
		if(e.effect == effect)
		{
			float power = (1.f - e.power);
			if(e.source == EffectSource::Temporary)
			{
				if(power > tmpValueHigh)
					tmpValueHigh = power;
				else if(power < tmpValueLow)
					tmpValueLow = power;
			}
			else
				value *= power;
		}
	}
	return value * tmpValueLow * tmpValueHigh;
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

	ITEM_SLOT itemSlot = ItemTypeToSlot(item->type);
	if(itemSlot == SLOT_RING1 && slots[itemSlot])
		itemSlot = SLOT_RING2;

	if(!slots[itemSlot])
	{
		slots[itemSlot] = item;
		ApplyItemEffects(item, itemSlot);
		--count;
	}

	if(count)
		AddItem(item, count, count);
}

//=================================================================================================
void Unit::CalculateLoad()
{
	if(IsSet(data->flags2, F2_FIXED_STATS))
		weightMax = 999;
	else
	{
		float mod = GetEffectMul(EffectId::Carry);
		weightMax = (int)(mod * (Get(AttributeId::STR) * 15));
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
		return GetWeapon().dmgType;
	else
		return data->dmgType;
}

//=================================================================================================
Vec3 Unit::GetLootCenter() const
{
	Mesh::Point* point2 = meshInst->mesh->GetPoint("centrum");

	if(!point2)
		return visualPos;

	const Mesh::Point& point = *point2;
	Matrix matBone = point.mat * meshInst->matBones[point.bone];
	matBone = matBone * (Matrix::Scale(data->scale) * Matrix::RotationY(rot) * Matrix::Translation(visualPos));
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
bool Unit::IsBetterWeapon(const Weapon& weapon, int* value, int* prevValue) const
{
	if(!HaveWeapon())
	{
		if(value)
		{
			*value = (int)CalculateWeaponPros(weapon);
			*prevValue = 0;
		}
		return true;
	}

	if(value)
	{
		float v = CalculateWeaponPros(weapon);
		float prevV = CalculateWeaponPros(GetWeapon());
		*value = (int)v;
		*prevValue = (int)prevV;
		return prevV < v;
	}
	else
		return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(weapon);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& armor, int* value, int* prevValue) const
{
	if(!HaveArmor())
	{
		if(value)
		{
			*value = (int)CalculateDefense(&armor);
			*prevValue = 0;
		}
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&armor);
		float prevV = CalculateDefense();
		*value = (int)v;
		*prevValue = (int)prevV;
		return prevV < v;
	}
	else
		return CalculateDefense() < CalculateDefense(&armor);
}

//=================================================================================================
bool Unit::CanRun() const
{
	if(IsSet(data->flags, F_SLOW)
		|| Any(action, A_BLOCK, A_BASH, A_SHOOT, A_USE_ITEM, A_CAST)
		|| (action == A_ATTACK && !act.attack.run)
		|| IsOverloaded())
		return false;

	const float slow = GetEffectMax(EffectId::SlowMove);
	if(slow >= 0.5f)
		return false;

	return true;
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
	float p = stamina / staminaMax;
	staminaMax = CalculateMaxStamina();
	stamina = staminaMax * p;
}

//=================================================================================================
float Unit::GetAttackFrame(int frame) const
{
	assert(InRange(frame, 0, 2));

	int attackId = act.attack.index;
	assert(attackId < data->frames->attacks);

	if(!data->frames->extra)
	{
		switch(frame)
		{
		case 0:
			return data->frames->t[F_ATTACK1_START + attackId * 2 + 0];
		case 1:
			return data->frames->Lerp(F_ATTACK1_START + attackId * 2);
		case 2:
			return data->frames->t[F_ATTACK1_START + attackId * 2 + 1];
		default:
			assert(0);
			return data->frames->t[F_ATTACK1_START + attackId * 2 + 1];
		}
	}
	else
	{
		switch(frame)
		{
		case 0:
			return data->frames->extra->e[attackId].start;
		case 1:
			return data->frames->extra->e[attackId].Lerp();
		case 2:
			return data->frames->extra->e[attackId].end;
		default:
			assert(0);
			return data->frames->extra->e[attackId].end;
		}
	}
}

//=================================================================================================
int Unit::GetRandomAttack() const
{
	if(data->frames->extra)
	{
		int a;

		switch(GetWeapon().weaponType)
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
		f << slots[i];
	f << items.size();
	for(ItemSlot& slot : items)
	{
		if(slot.item)
		{
			f << slot.item->id;
			f << slot.count;
			f << slot.teamCount;
			if(slot.item->id[0] == '$')
				f << slot.item->questId;
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

	f << liveState;
	f << pos;
	f << rot;
	f << hp;
	f << hpmax;
	f << mp;
	f << mpmax;
	f << stamina;
	f << staminaMax;
	f << staminaAction;
	f << staminaTimer;
	f << level;
	if(IsPlayer())
		stats->Save(f);
	else
		f << stats->subprofile;
	f << gold;
	f << toRemove;
	f << temporary;
	f << questId;
	f << assist;
	f << dontAttack;
	f << attackTeam;
	f << (eventHandler ? eventHandler->GetUnitEventHandlerQuestId() : -1);
	f << weight;
	f << summoner;
	if(liveState >= DYING)
		f << mark;

	assert((humanData != nullptr) == (data->type == UNIT_TYPE::HUMAN));
	if(humanData)
	{
		f.Write1();
		humanData->Save(f);
	}
	else
		f.Write0();

	if(f.isLocal)
	{
		meshInst->Save(f);
		f << animation;
		f << currentAnimation;

		f << prevPos;
		f << speed;
		f << prevSpeed;
		f << animationState;
		f << action;
		f << weaponTaken;
		f << weaponHiding;
		f << weaponState;
		f << hurtTimer;
		f << targetPos;
		f << targetPos2;
		f << talking;
		f << talkTimer;
		f << timer;
		f << alcohol;
		f << raiseTimer;

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
			if(act.dash.hit)
				act.dash.hit->Save(f);
			break;
		case A_SHOOT:
			f << (act.shoot.ability ? act.shoot.ability->hash : 0);
			break;
		case A_USE_USABLE:
			f << act.useUsable.rot;
			break;
		}

		if(usedItem)
		{
			f << usedItem->id;
			f << usedItemIsTeam;
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

		f << lastBash;
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
	UnitOrderEntry* currentOrder = order;
	while(currentOrder)
	{
		f.Write1();
		f << currentOrder->order;
		f << currentOrder->timer;
		switch(currentOrder->order)
		{
		case ORDER_FOLLOW:
		case ORDER_GUARD:
			f << currentOrder->unit;
			break;
		case ORDER_LOOK_AT:
			f << currentOrder->pos;
			break;
		case ORDER_MOVE:
			f << currentOrder->pos;
			f << currentOrder->moveType;
			f << currentOrder->range;
			break;
		case ORDER_ESCAPE_TO:
			f << currentOrder->pos;
			break;
		case ORDER_ESCAPE_TO_UNIT:
			f << currentOrder->unit;
			f << currentOrder->pos;
			break;
		case ORDER_AUTO_TALK:
			f << currentOrder->autoTalk;
			if(currentOrder->autoTalkDialog)
			{
				f << currentOrder->autoTalkDialog->id;
				f << (currentOrder->autoTalkQuest ? currentOrder->autoTalkQuest->id : -1);
			}
			else
				f.Write0();
			break;
		case ORDER_ATTACK_OBJECT:
			f << currentOrder->usable;
			break;
		}
		currentOrder = currentOrder->next;
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
				f << slot.item->questId;
		}
		else
			f.Write0();
	}
}

//=================================================================================================
void Unit::Load(GameReader& f)
{
	fakeUnit = true; // to prevent sending MP message set temporary as fake unit
	humanData = nullptr;

	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	data = UnitData::Get(f.ReadString1());

	// items
	bool canSort = true;
	for(int i = 0; i < SLOT_MAX; ++i)
		f >> slots[i];
	items.resize(f.Read<uint>());
	for(ItemSlot& slot : items)
	{
		const string& itemId = f.ReadString1();
		f >> slot.count;
		f >> slot.teamCount;
		if(itemId[0] != '$')
			slot.item = Item::Get(itemId);
		else
		{
			int questItemId = f.Read<int>();
			questMgr->AddQuestItemRequest(&slot.item, itemId.c_str(), questItemId, &items, this);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			canSort = false;
		}
	}
	if(f.Read0())
		stock = nullptr;
	else
		LoadStock(f);

	// stats
	f >> liveState;
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
	f >> staminaMax;
	f >> staminaAction;
	f >> staminaTimer;
	f >> level;
	if(content.requireUpdate && data->group != G_PLAYER)
	{
		if(data->upgrade)
		{
			// upgrade unit - previously there was 'mage' unit, now it is split into 'mage novice', 'mage' and 'master mage'
			// calculate which one to use
			int bestDif = data->GetLevelDif(level);
			for(UnitData* u : *data->upgrade)
			{
				int dif = u->GetLevelDif(level);
				if(dif < bestDif)
				{
					bestDif = dif;
					data = u;
				}
			}
		}
		level = data->level.Clamp(level);
	}
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
	f >> gold;
	if(LOAD_VERSION < V_0_11)
		f.Skip<int>(); // old inside_building
	f >> toRemove;
	f >> temporary;
	f >> questId;
	f >> assist;

	AutoTalkMode oldAutoTalk = AutoTalkMode::No;
	GameDialog* oldAutoTalkDialog = nullptr;
	float oldAutoTalkTimer = 0;
	if(LOAD_VERSION < V_0_12)
	{
		// old auto talk
		f >> oldAutoTalk;
		if(oldAutoTalk != AutoTalkMode::No)
		{
			if(const string& dialogId = f.ReadString1(); !dialogId.empty())
				oldAutoTalkDialog = GameDialog::TryGet(dialogId.c_str());
			else
				oldAutoTalkDialog = nullptr;
			f >> oldAutoTalkTimer;
		}
	}

	f >> dontAttack;
	f >> attackTeam;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	int unitEventHandlerQuestId = f.Read<int>();
	if(unitEventHandlerQuestId == -2)
		eventHandler = questMgr->questContest;
	else if(unitEventHandlerQuestId == -1)
		eventHandler = nullptr;
	else
	{
		eventHandler = reinterpret_cast<UnitEventHandler*>(unitEventHandlerQuestId);
		game->loadUnitHandler.push_back(this);
	}
	if(canSort && content.requireUpdate)
		SortItems(items);
	f >> weight;
	if(canSort && content.requireUpdate)
		RecalculateWeight();

	Entity<Unit> guardTarget;
	if(LOAD_VERSION < V_0_12)
		f >> guardTarget;
	f >> summoner;

	if(liveState >= DYING)
		f >> mark;

	bubble = nullptr; // ustawianie przy wczytaniu SpeechBubble
	changed = false;
	busy = Busy_No;
	visualPos = pos;

	if(f.Read1())
	{
		humanData = new Human;
		humanData->Load(f);
	}
	else
	{
		assert(data->type != UNIT_TYPE::HUMAN);
		humanData = nullptr;
	}

	if(f.isLocal)
	{
		float oldAttackPower = 1.f;
		bool oldRunAttack = false, oldHitted = false;

		CreateMesh(CREATE_MESH::LOAD);
		meshInst->Load(f, LOAD_VERSION >= V_0_13 ? 1 : 0);
		f >> animation;
		f >> currentAnimation;

		f >> prevPos;
		f >> speed;
		f >> prevSpeed;
		f >> animationState;
		if(LOAD_VERSION < V_0_13)
			f >> aiMode; // old attackId, assigned to unused variable at client side to pass to AIController
		f >> action;
		f >> weaponTaken;
		f >> weaponHiding;
		f >> weaponState;
		if(LOAD_VERSION < V_0_13)
			f >> oldHitted;
		f >> hurtTimer;
		f >> targetPos;
		f >> targetPos2;
		f >> talking;
		f >> talkTimer;
		if(LOAD_VERSION < V_0_13)
		{
			f >> oldAttackPower;
			f >> oldRunAttack;
		}
		f >> timer;
		f >> alcohol;
		f >> raiseTimer;

		switch(action)
		{
		case A_ATTACK:
			if(LOAD_VERSION >= V_0_13)
			{
				f >> act.attack.index;
				f >> act.attack.power;
				f >> act.attack.run;
				if(LOAD_VERSION >= V_DEV)
					f >> act.attack.hitted;
				else
				{
					bool oldHitted;
					f >> oldHitted;
					act.attack.hitted = oldHitted ? 2 : 0;
				}
			}
			else
			{
				act.attack.index = aiMode;
				act.attack.power = oldAttackPower;
				act.attack.run = oldRunAttack;
				act.attack.hitted = oldHitted;
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
					act.cast.ability = data->abilities->ability[aiMode];
			}
			break;
		case A_DASH:
			if(LOAD_VERSION >= V_0_13)
				act.dash.ability = Ability::Get(f.Read<int>());
			else
				act.dash.ability = Ability::Get(animationState == 0 ? "dash" : "bull_charge");
			f >> act.dash.rot;
			if(LOAD_VERSION >= V_0_17 && act.dash.ability->RequireList())
			{
				act.dash.hit = UnitList::Get();
				act.dash.hit->Load(f);
			}
			break;
		case A_SHOOT:
			if(LOAD_VERSION >= V_0_18)
			{
				int hash;
				f >> hash;
				if(hash != 0)
					act.shoot.ability = Ability::Get(hash);
				else
					act.shoot.ability = nullptr;
			}
			else
				act.shoot.ability = nullptr;
			break;
		case A_USE_USABLE:
			f >> act.useUsable.rot;
			break;
		}

		const string& itemId = f.ReadString1();
		if(!itemId.empty())
		{
			usedItem = Item::Get(itemId);
			f >> usedItemIsTeam;
		}
		else
			usedItem = nullptr;

		int usableId = f.Read<int>();
		if(usableId == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usableId);

		if(action == A_SHOOT)
		{
			bowInstance = gameLevel->GetBowInstance(GetBow().mesh);
			bowInstance->Play(&bowInstance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bowInstance->groups[0].speed = meshInst->groups[1].speed;
			bowInstance->groups[0].time = Min(meshInst->groups[1].time, bowInstance->groups[0].anim->length);
		}

		f >> lastBash;
		f >> moved;
		f >> running;
	}
	else
	{
		meshInst = nullptr;
		ai = nullptr;
		usable = nullptr;
		usedItem = nullptr;
		weaponState = WeaponState::Hidden;
		weaponTaken = W_NONE;
		weaponHiding = W_NONE;
		frozen = FROZEN::NO;
		talking = false;
		animation = currentAnimation = ANI_STAND;
		action = A_NONE;
		hurtTimer = 0.f;
		speed = prevSpeed = 0.f;
		alcohol = 0.f;
		moved = false;
		running = false;
	}

	// effects
	f.ReadVector4(effects);
	if(LOAD_VERSION < V_0_14)
	{
		for(Effect& e : effects)
		{
			if(e.source == EffectSource::Perk)
				e.sourceId = old::Convert((old::Perk)e.sourceId)->hash;
		}
	}

	if(content.requireUpdate)
	{
		RemoveEffects(EffectId::None, EffectSource::Item, -1, -1);
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i])
				ApplyItemEffects(slots[i], (ITEM_SLOT)i);
		}
	}

	// dialogs
	dialogs.resize(f.Read<uint>());
	for(QuestDialog& dialog : dialogs)
	{
		int questId = f.Read<int>();
		string* str = StringPool.Get();
		f >> *str;
		questMgr->AddQuestRequest(questId, (Quest**)&dialog.quest, [this, &dialog, str]()
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

	// events
	events.resize(f.Read<uint>());
	for(Event& e : events)
	{
		int questId;
		f >> e.type;
		f >> questId;
		questMgr->AddQuestRequest(questId, (Quest**)&e.quest, [&]()
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
		UnitOrderEntry* currentOrder = nullptr;
		while(f.Read1())
		{
			if(currentOrder)
			{
				currentOrder->next = UnitOrderEntry::Get();
				currentOrder = currentOrder->next;
			}
			else
			{
				order = UnitOrderEntry::Get();
				currentOrder = order;
			}

			f >> currentOrder->order;
			f >> currentOrder->timer;
			switch(currentOrder->order)
			{
			case ORDER_FOLLOW:
				f >> currentOrder->unit;
				break;
			case ORDER_LOOK_AT:
				f >> currentOrder->pos;
				break;
			case ORDER_MOVE:
				f >> currentOrder->pos;
				f >> currentOrder->moveType;
				if(LOAD_VERSION >= V_0_14)
					f >> currentOrder->range;
				else
					currentOrder->range = 0.1f;
				break;
			case ORDER_ESCAPE_TO:
				f >> currentOrder->pos;
				break;
			case ORDER_ESCAPE_TO_UNIT:
				f >> currentOrder->unit;
				f >> currentOrder->pos;
				break;
			case ORDER_GUARD:
				f >> currentOrder->unit;
				break;
			case ORDER_AUTO_TALK:
				f >> currentOrder->autoTalk;
				if(const string& dialogId = f.ReadString1(); !dialogId.empty())
				{
					currentOrder->autoTalkDialog = GameDialog::TryGet(dialogId.c_str());
					int questId;
					f >> questId;
					if(questId != -1)
						questMgr->AddQuestRequest(questId, &currentOrder->autoTalkQuest);
					else
						currentOrder->autoTalkQuest = nullptr;
				}
				else
				{
					currentOrder->autoTalkDialog = nullptr;
					currentOrder->autoTalkQuest = nullptr;
				}
				break;
			case ORDER_ATTACK_OBJECT:
				f >> currentOrder->usable;
				break;
			}
		}
	}
	else
	{
		UnitOrder unitOrder;
		float timer;
		f >> unitOrder;
		f >> timer;
		if(unitOrder != ORDER_NONE)
		{
			order = UnitOrderEntry::Get();
			order->order = unitOrder;
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
				f >> order->moveType;
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

	if(guardTarget)
	{
		if(order)
			order->Free();
		order = UnitOrderEntry::Get();
		order->order = ORDER_GUARD;
		order->unit = guardTarget;
		order->timer = 0.f;
	}
	if(oldAutoTalk != AutoTalkMode::No)
	{
		if(order)
			order->Free();
		order = UnitOrderEntry::Get();
		order->order = ORDER_AUTO_TALK;
		order->timer = oldAutoTalkTimer;
		order->autoTalk = oldAutoTalk;
		order->autoTalkDialog = oldAutoTalkDialog;
		order->autoTalkQuest = nullptr;
	}

	if(f.Read1())
	{
		player = new PlayerController;
		player->unit = this;
		player->Load(f);
	}
	else
		player = nullptr;

	if(f.isLocal && humanData)
		humanData->ApplyScale(meshInst);

	if(IsSet(data->flags, F_HERO))
	{
		hero = new Hero;
		hero->unit = this;
		hero->Load(f);
		if(LOAD_VERSION < V_0_12)
		{
			if(hero->teamMember && hero->type == HeroType::Visitor &&
				(data->id == "q_zlo_kaplan" || data->id == "q_magowie_stary" || strncmp(data->id.c_str(), "q_orkowie_gorush", 16) == 0))
			{
				hero->type = HeroType::Free;
			}
		}
	}
	else
		hero = nullptr;

	inArena = -1;
	ai = nullptr;
	interp = nullptr;
	frozen = FROZEN::NO;

	// fizyka
	if(f.isLocal)
		CreatePhysics(true);
	else
		cobj = nullptr;

	// zabezpieczenie
	if((Any(weaponState, WeaponState::Taken, WeaponState::Taking) && weaponTaken == W_NONE)
		|| (weaponState == WeaponState::Hiding && weaponHiding == W_NONE))
	{
		SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		Warn("Unit '%s' had broken weapon state.", data->id.c_str());
	}

	// calculate new skills/attributes
	if(content.requireUpdate)
		CalculateStats();

	// compatibility
	if(LOAD_VERSION < V_0_12)
		mp = mpmax = CalculateMaxMp();

	CalculateLoad();

	fakeUnit = false;
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

	bool canSort = true;
	cnt.resize(count);
	for(ItemSlot& slot : cnt)
	{
		const string& itemId = f.ReadString1();
		f >> slot.count;
		if(itemId[0] != '$')
			slot.item = Item::Get(itemId);
		else
		{
			int questId;
			f >> questId;
			questMgr->AddQuestItemRequest(&slot.item, itemId.c_str(), questId, &cnt);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			canSort = false;
		}
	}

	if(canSort && content.requireUpdate)
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
		f.WriteCasted<byte>(humanData->hair);
		f.WriteCasted<byte>(humanData->beard);
		f.WriteCasted<byte>(humanData->mustache);
		f << humanData->hairColor;
		f << humanData->height;
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
	f.WriteCasted<byte>(liveState);
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
	f.WriteCasted<char>(inArena);
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
		if(hero->knowName)
			b |= 0x01;
		if(hero->teamMember)
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
		f << player->freeDays;
	}
	if(IsAI())
		f << GetAiMode();

	// loaded data
	if(net->mpLoad)
	{
		meshInst->Write(f);
		f.WriteCasted<byte>(animation);
		f.WriteCasted<byte>(currentAnimation);
		f.WriteCasted<byte>(animationState);
		f.WriteCasted<byte>(action);
		f.WriteCasted<byte>(weaponTaken);
		f.WriteCasted<byte>(weaponHiding);
		f.WriteCasted<byte>(weaponState);
		f << targetPos;
		f << targetPos2;
		f << timer;
		if(usedItem)
			f << usedItem->id;
		else
			f.Write0();
		f << (usable ? usable->id : -1);
		switch(action)
		{
		case A_ATTACK:
			f << act.attack.index;
			break;
		case A_USE_USABLE:
			f << act.useUsable.rot;
			break;
		case A_SHOOT:
			f << (act.shoot.ability ? act.shoot.ability->hash : 0);
			break;
		}

		// visible effects
		uint count = 0;
		for(const Effect& e : effects)
		{
			if(e.IsVisible())
				++count;
		}
		f.WriteCasted<byte>(count);
		if(count)
		{
			for(const Effect& e : effects)
			{
				if(e.IsVisible())
				{
					f.WriteCasted<byte>(e.effect);
					f << e.time;
				}
			}
		}
	}
	else
	{
		// player changing dungeon level keeps weapon state
		f.WriteCasted<byte>(weaponTaken);
		f.WriteCasted<byte>(weaponState);
	}
}

//=================================================================================================
bool Unit::Read(BitStreamReader& f)
{
	// main
	f >> id;
	const string& unitDataId = f.ReadString1();
	if(!f)
		return false;
	Register();
	data = UnitData::TryGet(unitDataId);
	if(!data)
	{
		Error("Missing base unit id '%s'!", unitDataId.c_str());
		return false;
	}

	// human data
	if(data->type == UNIT_TYPE::HUMAN)
	{
		humanData = new Human;
		f.ReadCasted<byte>(humanData->hair);
		f.ReadCasted<byte>(humanData->beard);
		f.ReadCasted<byte>(humanData->mustache);
		f >> humanData->hairColor;
		f >> humanData->height;
		if(!f)
			return false;
		if(humanData->hair == 0xFF)
			humanData->hair = -1;
		if(humanData->beard == 0xFF)
			humanData->beard = -1;
		if(humanData->mustache == 0xFF)
			humanData->mustache = -1;
		if(humanData->hair < -1
			|| humanData->hair >= MAX_HAIR
			|| humanData->beard < -1
			|| humanData->beard >= MAX_BEARD
			|| humanData->mustache < -1
			|| humanData->mustache >= MAX_MUSTACHE
			|| !InRange(humanData->height, 0.85f, 1.15f))
		{
			Error("Invalid human data (hair:%d, beard:%d, mustache:%d, height:%g).", humanData->hair, humanData->beard,
				humanData->mustache, humanData->height);
			return false;
		}
	}
	else
		humanData = nullptr;

	// equipped items
	if(data->type != UNIT_TYPE::ANIMAL)
	{
		for(int i = 0; i < SLOT_MAX_VISIBLE; ++i)
		{
			const string& itemId = f.ReadString1();
			if(!f)
				return false;
			if(itemId.empty())
				slots[i] = nullptr;
			else
			{
				const Item* item = Item::TryGet(itemId);
				if(item && ItemTypeToSlot(item->type) == (ITEM_SLOT)i)
				{
					gameRes->PreloadItem(item);
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
	staminaMax = 1.f;
	level = 0;
	f.ReadCasted<byte>(liveState);
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
	f.ReadCasted<char>(inArena);
	f >> summoner;
	f >> mark;
	if(!f)
		return false;
	if(liveState >= LIVESTATE_MAX)
	{
		Error("Invalid live state %d.", liveState);
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
		hero->knowName = IsSet(flags, 0x01);
		hero->teamMember = IsSet(flags, 0x02);
		hero->type = IsSet(flags, 0x04) ? HeroType::Visitor : HeroType::Normal;
	}
	else if(type == 2)
	{
		// player
		int id;
		f.ReadCasted<byte>(id);
		if(id == team->myId)
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
		f >> player->freeDays;
		if(!f)
			return false;
		if(player->credit < 0)
		{
			Error("Invalid player %d credit %d.", player->id, player->credit);
			return false;
		}
		if(player->freeDays < 0)
		{
			Error("Invalid player %d free days %d.", player->id, player->freeDays);
			return false;
		}
		player->playerInfo = net->TryGetPlayer(player->id);
		if(!player->playerInfo)
		{
			Error("Invalid player id %d.", player->id);
			return false;
		}
		player->playerInfo->u = this;
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
		f.ReadCasted<byte>(aiMode);
		if(!f)
			return false;
	}

	// mesh
	CreateMesh(net->mpLoad ? CREATE_MESH::PRELOAD : CREATE_MESH::NORMAL);

	action = A_NONE;
	weaponTaken = W_NONE;
	weaponHiding = W_NONE;
	weaponState = WeaponState::Hidden;
	talking = false;
	busy = Busy_No;
	frozen = FROZEN::NO;
	usable = nullptr;
	usedItem = nullptr;
	bowInstance = nullptr;
	ai = nullptr;
	animation = ANI_STAND;
	currentAnimation = ANI_STAND;
	timer = 0.f;
	toRemove = false;
	bubble = nullptr;
	interp = EntityInterpolator::Pool.Get();
	interp->Reset(pos, rot);
	visualPos = pos;

	if(net->mpLoad)
	{
		// get current state in multiplayer
		if(!meshInst->Read(f))
			return false;
		f.ReadCasted<byte>(animation);
		f.ReadCasted<byte>(currentAnimation);
		f.ReadCasted<byte>(animationState);
		f.ReadCasted<byte>(action);
		f.ReadCasted<byte>(weaponTaken);
		f.ReadCasted<byte>(weaponHiding);
		f.ReadCasted<byte>(weaponState);
		f >> targetPos;
		f >> targetPos2;
		f >> timer;
		const string& usedItemId = f.ReadString1();
		int usableId = f.Read<int>();

		// used item
		if(!usedItemId.empty())
		{
			usedItem = Item::TryGet(usedItemId);
			if(!usedItem)
			{
				Error("Missing used item '%s'.", usedItemId.c_str());
				return false;
			}
		}
		else
			usedItem = nullptr;

		// usable
		if(usableId == -1)
			usable = nullptr;
		else
			Usable::AddRequest(&usable, usableId);

		// action
		switch(action)
		{
		case A_ATTACK:
			f >> act.attack.index;
			break;
		case A_SHOOT:
			act.shoot.ability = Ability::Get(f.Read<int>());
			bowInstance = gameLevel->GetBowInstance(GetBow().mesh);
			bowInstance->Play(&bowInstance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			bowInstance->groups[0].speed = meshInst->groups[1].speed;
			bowInstance->groups[0].time = Min(meshInst->groups[1].time, bowInstance->groups[0].anim->length);
			break;
		case A_USE_USABLE:
			f >> act.useUsable.rot;
			break;
		}

		// visible effects
		effects.resize(f.Read<byte>());
		for(Effect& e : effects)
		{
			f.ReadCasted<byte>(e.effect);
			f >> e.time;
			e.source = EffectSource::Temporary;
			e.sourceId = -1;
			e.value = -1;
			e.power = 0;
		}

		if(!f)
			return false;
	}
	else
	{
		// player changing dungeon level keeps weapon state
		f.ReadCasted<byte>(weaponTaken);
		f.ReadCasted<byte>(weaponState);
		if(weaponState == WeaponState::Taking)
			weaponState = WeaponState::Hidden;
	}

	// physics
	CreatePhysics(true);

	prevPos = pos;
	speed = prevSpeed = 0.f;
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
	float healedHp,
		missingHp = hpmax - hp;
	int potionIndex = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->item->type != IT_CONSUMABLE)
			continue;

		const Consumable& pot = it->item->ToConsumable();
		if(pot.aiType != Consumable::AiType::Healing)
			continue;

		float power = pot.GetEffectPower(EffectId::Heal);
		if(potionIndex == -1)
		{
			potionIndex = index;
			healedHp = power;
		}
		else
		{
			if(power > missingHp)
			{
				if(power < healedHp)
				{
					potionIndex = index;
					healedHp = power;
				}
			}
			else if(power > healedHp)
			{
				potionIndex = index;
				healedHp = power;
			}
		}
	}

	return potionIndex;
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
	if(net->activePlayers > 1)
	{
		const Item* prevSlots[SLOT_MAX];
		for(int i = 0; i < SLOT_MAX; ++i)
			prevSlots[i] = slots[i];

		ReequipItemsInternal();

		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] != prevSlots[i] && IsVisible((ITEM_SLOT)i))
			{
				NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
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
	for(ItemSlot& itemSlot : items)
	{
		if(!itemSlot.item)
			continue;

		if(itemSlot.item->type == IT_GOLD)
		{
			gold += itemSlot.count;
			itemSlot.item = nullptr;
			changes = true;
		}
		else if(CanWear(itemSlot.item))
		{
			ITEM_SLOT slot = ItemTypeToSlot(itemSlot.item->type);
			assert(slot != SLOT_INVALID);

			if(slots[slot])
			{
				if(slots[slot]->value < itemSlot.item->value)
				{
					const Item* item = slots[slot];
					RemoveItemEffects(item, slot);
					ApplyItemEffects(itemSlot.item, slot);
					slots[slot] = itemSlot.item;
					itemSlot.item = item;
				}
			}
			else
			{
				ApplyItemEffects(itemSlot.item, slot);
				slots[slot] = itemSlot.item;
				itemSlot.item = nullptr;
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
		gameRes->PreloadItem(item);
		AddItemAndEquipIfNone(item);
	}
}

//=================================================================================================
bool Unit::HaveQuestItem(int questId)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(questId))
			return true;
	}
	return false;
}

//=================================================================================================
void Unit::RemoveQuestItem(int questId)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && it->item->IsQuest(questId))
		{
			weight -= it->item->weight;
			items.erase(it);
			if(IsOtherPlayer())
			{
				NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::REMOVE_QUEST_ITEM);
				c.id = questId;
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
	for(const Item* slotItem : slots)
	{
		if(slotItem == item)
			return true;
	}
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item == item && (!owned || it->teamCount != it->count))
			return true;
	}
	return false;
}

//=================================================================================================
bool Unit::HaveItemEquipped(const Item* item) const
{
	ITEM_SLOT slotType = ItemTypeToSlot(item->type);
	if(slotType == SLOT_INVALID)
		return false;
	else if(slotType == SLOT_RING1)
		return slots[SLOT_RING1] == item || slots[SLOT_RING2] == item;
	else
		return slots[slotType] == item;
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

	switch(weaponState)
	{
	case WeaponState::Taken:
	case WeaponState::Taking:
		return weaponTaken == type;
	case WeaponState::Hiding:
		return weaponHiding == type;
	default:
		return false;
	}
}

//=================================================================================================
float Unit::GetAttackSpeed(const Weapon* usedWeapon) const
{
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return 1.f;

	const Weapon* wep;

	if(usedWeapon)
		wep = usedWeapon;
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
	float baseSpeed;
	if(wep)
	{
		const WeaponTypeInfo& info = wep->GetInfo();

		float dexMod = min(0.25f, info.dexSpeed * Get(AttributeId::DEX));
		mod += float(Get(info.skill)) / 200 + dexMod - GetAttackSpeedModFromStrength(*wep);
		baseSpeed = info.baseSpeed;
	}
	else
	{
		float dexMod = min(0.25f, 0.02f * Get(AttributeId::DEX));
		mod += float(Get(SkillId::UNARMED)) / 200 + dexMod;
		baseSpeed = 1.f;
	}

	float mobility = CalculateMobility();
	if(mobility < 100)
		mod -= Lerp(0.1f, 0.f, mobility / 100);
	else if(mobility > 100)
		mod += Lerp(0.f, 0.1f, (mobility - 100) / 100);

	if(mod < 0.5f)
		mod = 0.5f;

	float speed = mod * baseSpeed;
	return speed;
}

//=================================================================================================
float Unit::GetBowAttackSpeed() const
{
	float baseMod = GetStaminaAttackSpeedMod();
	if(IsSet(data->flags2, F2_FIXED_STATS))
		return baseMod;

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

	mod *= baseMod;
	if(mod < 0.5f)
		mod = 0.5f;

	return mod;
}

//=================================================================================================
void Unit::MakeItemsTeam(bool isTeam)
{
	if(isTeam)
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->teamCount = it->count;
	}
	else
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->teamCount = 0;
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
			effectsToRemove.push_back(index);
	}

	RemoveEffects();
}

//=================================================================================================
void Unit::RemoveEffect(EffectId effect)
{
	bool visible = false;
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == effect)
		{
			effectsToRemove.push_back(index);
			if(it->IsVisible())
				visible = true;
		}
	}

	if(visible && Net::IsServer())
	{
		NetChange& c = Net::PushChange(NetChange::REMOVE_UNIT_EFFECT);
		c.unit = this;
		c.id = (int)effect;
	}

	RemoveEffects();
}

//=================================================================================================
int Unit::FindItem(const Item* item, int questId) const
{
	assert(item);

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] == item && (questId == -1 || slots[i]->IsQuest(questId)))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item && (questId == -1 || it->item->IsQuest(questId)))
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
int Unit::FindQuestItem(int questId) const
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(slots[i] && slots[i]->IsQuest(questId))
			return SlotToIIndex(ITEM_SLOT(i));
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(questId))
			return index;
	}

	return INVALID_IINDEX;
}

//=================================================================================================
bool Unit::FindQuestItem(cstring id, Quest** outQuest, int* iIndex, bool notActive, int questId)
{
	assert(id);

	if(id[1] == '$')
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i] && slots[i]->IsQuest() && (questId == -1 || questId == slots[i]->questId))
			{
				Quest* quest = questMgr->FindQuest(slots[i]->questId, !notActive);
				if(quest && (notActive || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(iIndex)
						*iIndex = SlotToIIndex(ITEM_SLOT(i));
					if(outQuest)
						*outQuest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za³o¿onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = items.begin(), end2 = items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && it2->item->IsQuest() && (questId == -1 || questId == it2->item->questId))
			{
				Quest* quest = questMgr->FindQuest(it2->item->questId, !notActive);
				if(quest && (notActive || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(iIndex)
						*iIndex = index;
					if(outQuest)
						*outQuest = quest;
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
				Quest* quest = questMgr->FindQuest(slots[i]->questId, !notActive);
				if(quest && (notActive || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(iIndex)
						*iIndex = SlotToIIndex(ITEM_SLOT(i));
					if(outQuest)
						*outQuest = quest;
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
				Quest* quest = questMgr->FindQuest(it2->item->questId, !notActive);
				if(quest && (notActive || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(iIndex)
						*iIndex = index;
					if(outQuest)
						*outQuest = quest;
					return true;
				}
			}
		}
	}

	return false;
}

//=================================================================================================
void Unit::EquipItem(int index)
{
	const Item* newItem = items[index].item;
	ITEM_SLOT slot = ItemTypeToSlot(newItem->type);

	// for rings - use empty slot or last equipped slot
	if(slot == SLOT_RING1 && IsPlayer())
	{
		if(slots[slot])
		{
			if(!slots[SLOT_RING2] || player->lastRing)
				slot = SLOT_RING2;
		}
		player->lastRing = (slot == SLOT_RING2);
	}

	const Item* prevItem = slots[slot];
	if(prevItem)
	{
		// replace equipped item
		if(Net::IsLocal())
		{
			RemoveItemEffects(prevItem, slot);
			ApplyItemEffects(newItem, slot);
		}
		slots[slot] = newItem;
		items.erase(items.begin() + index);
		AddItem(prevItem, 1, false);
		weight -= prevItem->weight;
	}
	else
	{
		// equip item
		slots[slot] = newItem;
		if(Net::IsLocal())
			ApplyItemEffects(newItem, slot);
		items.erase(items.begin() + index);
	}

	if(Net::IsServer() && IsVisible(slot))
	{
		NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
		c.unit = this;
		c.id = slot;
	}
}

//=================================================================================================
// Equip item out of nowhere, used only in tutorial
void Unit::EquipItem(const Item* item)
{
	assert(item);
	assert(!gameLevel->ready); // todo
	gameRes->PreloadItem(item);
	slots[ItemTypeToSlot(item->type)] = item;
	weight += item->weight;
}

//=================================================================================================
// currently using this on pc, looted units is not written
void Unit::RemoveItem(int iIndex, bool activeLocation)
{
	assert(!player);
	assert(Net::IsLocal());
	if(iIndex >= 0)
	{
		assert(iIndex < (int)items.size());
		RemoveElementIndex(items, iIndex);
	}
	else
	{
		ITEM_SLOT s = IIndexToSlot(iIndex);
		assert(slots[s]);
		slots[s] = nullptr;
		if(activeLocation && IsVisible(s))
		{
			NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
			c.unit = this;
			c.id = s;
		}
	}
}

//=================================================================================================
// remove item from inventory (handle open inventory, lock & multiplayer), for 0 count remove all items
uint Unit::RemoveItem(int iIndex, uint count)
{
	// remove item
	uint removed;
	if(iIndex >= 0)
	{
		ItemSlot& s = items[iIndex];
		removed = (count == 0 ? s.count : min(s.count, count));
		s.count -= removed;
		weight -= s.item->weight * removed;
		if(s.count == 0)
			items.erase(items.begin() + iIndex);
		else if(s.teamCount > 0)
			s.teamCount -= min(s.teamCount, removed);
	}
	else
	{
		ITEM_SLOT type = IIndexToSlot(iIndex);
		weight -= slots[type]->weight;
		slots[type] = nullptr;
		removed = 1;

		if(Net::IsServer() && net->activePlayers > 1 && IsVisible(type))
		{
			NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
			c.unit = this;
			c.id = type;
		}
	}

	// notify
	if(Net::IsServer())
	{
		if(IsPlayer())
		{
			if(!player->isLocal)
			{
				NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::REMOVE_ITEMS);
				c.id = iIndex;
				c.count = removed;
			}
		}
		else
		{
			// search for player trading with this unit
			Unit* t = nullptr;
			for(Unit& member : team->activeMembers)
			{
				if(member.IsPlayer() && member.player->IsTradingWith(this))
				{
					t = &member;
					break;
				}
			}

			if(t && t->player != game->pc)
			{
				NetChangePlayer& c = t->player->playerInfo->PushChange(NetChangePlayer::REMOVE_ITEMS_TRADER);
				c.id = id;
				c.count = removed;
				c.a = iIndex;
			}
		}
	}

	// update temporary inventory
	if(game->pc->unit == this)
	{
		if(gameGui->inventory->invMine->visible || gameGui->inventory->gpTrade->visible)
			gameGui->inventory->BuildTmpInventory(0);
	}
	else if(gameGui->inventory->gpTrade->visible && gameGui->inventory->invTradeOther->unit == this)
		gameGui->inventory->BuildTmpInventory(1);

	return removed;
}

//=================================================================================================
uint Unit::RemoveItem(const Item* item, uint count)
{
	int iIndex = FindItem(item);
	if(iIndex == INVALID_IINDEX)
		return 0;
	return RemoveItem(iIndex, count);
}

//=================================================================================================
uint Unit::RemoveItemS(const string& itemId, uint count)
{
	const Item* item = Item::TryGet(itemId);
	if(!item)
		return 0;
	return RemoveItem(item, count);
}

//=================================================================================================
void Unit::RemoveEquippedItem(ITEM_SLOT slot)
{
	const Item* item = GetEquippedItem(slot);
	if(slot == SLOT_WEAPON && slots[SLOT_WEAPON] == usedItem)
	{
		usedItem = nullptr;
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::REMOVE_USED_ITEM);
			c.unit = this;
		}
	}
	if(Net::IsLocal())
		RemoveItemEffects(item, slot);
	if(Net::IsServer() && IsVisible(slot))
	{
		NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
		c.unit = this;
		c.id = slot;
	}
	weight -= item->weight;
	slots[slot] = nullptr;
}

//=================================================================================================
void Unit::RemoveAllEquippedItems()
{
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(!slots[i])
			continue;

		if(Net::IsLocal())
			RemoveItemEffects(slots[i], (ITEM_SLOT)i);
		slots[i] = nullptr;

		if(Net::IsServer() && IsVisible((ITEM_SLOT)i))
		{
			NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
			c.unit = this;
			c.id = i;
		}
	}
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
		player->lastWeapon = W_NONE;
}

//=================================================================================================
cstring NAMES::pointWeapon = "bron";
cstring NAMES::pointHiddenWeapon = "schowana";
cstring NAMES::pointShield = "tarcza";
cstring NAMES::pointShieldHidden = "tarcza_plecy";
cstring NAMES::pointBow = "luk";
cstring NAMES::pointCast = "castpoint";
cstring NAMES::points[] = {
	"bron",
	"schowana",
	"tarcza",
	"tarcza_plecy",
	"luk"
};
uint NAMES::nPoints = countof(points);

//=================================================================================================
cstring NAMES::aniTakeWeapon = "wyjmuje";
cstring NAMES::aniTakeWeaponNoShield = "wyjmuje_bez_tarczy";
cstring NAMES::aniTakeBow = "wyjmij_luk";
cstring NAMES::aniMove = "idzie";
cstring NAMES::aniRun = "biegnie";
cstring NAMES::aniLeft = "w_lewo";
cstring NAMES::aniRight = "w_prawo";
cstring NAMES::aniStand = "stoi";
cstring NAMES::aniBattle = "bojowy";
cstring NAMES::aniBattleBow = "bojowy_luk";
cstring NAMES::aniDie = "umiera";
cstring NAMES::aniHurt = "trafiony";
cstring NAMES::aniShoot = "atak_luk";
cstring NAMES::aniBlock = "blok";
cstring NAMES::aniBash = "bash";
cstring NAMES::aniBase[] = {
	"idzie",
	"w_lewo",
	"w_prawo",
	"stoi",
	"bojowy",
	"umiera"
};
cstring NAMES::aniHumanoid[] = {
	"wyjmuje",
	"wyjmij_luk",
	"bojowy_luk",
	"atak_luk",
	"blok",
	"bash"
};
cstring NAMES::aniAttacks[] = {
	"atak1",
	"atak2",
	"atak3",
	"atak4",
	"atak5",
	"atak6"
};
uint NAMES::nAniBase = countof(aniBase);
uint NAMES::nAniHumanoid = countof(aniHumanoid);
int NAMES::maxAttacks = countof(aniAttacks);

//=================================================================================================
// unused?
Vec3 Unit::GetEyePos() const
{
	const Mesh::Point& point = *meshInst->mesh->GetPoint("oczy");
	Matrix matBone = point.mat * meshInst->matBones[point.bone];
	matBone = matBone * (Matrix::Scale(data->scale) * Matrix::RotationY(rot) * Matrix::Translation(pos));
	Vec3 eye = Vec3::TransformZero(matBone);
	return eye;
}

//=================================================================================================
bool Unit::IsBetterItem(const Item* item, int* value, int* prevValue, ITEM_SLOT* targetSlot) const
{
	assert(item);

	if(targetSlot)
		*targetSlot = ItemTypeToSlot(item->type);

	switch(item->type)
	{
	case IT_WEAPON:
		if(!IsSet(data->flags, F_MAGE))
			return IsBetterWeapon(item->ToWeapon(), value, prevValue);
		else
		{
			int v = item->aiValue;
			int prevV = HaveWeapon() ? GetWeapon().aiValue : 0;
			if(value)
			{
				*value = v;
				*prevValue = prevV;
			}
			return v > prevV;
		}
	case IT_BOW:
		{
			int v = item->ToBow().dmg;
			int prevV = HaveBow() ? GetBow().dmg : 0;
			if(value)
			{
				*value = v * 2;
				*prevValue = prevV * 2;
			}
			return v > prevV;
		}
	case IT_SHIELD:
		{
			int v = item->ToShield().block;
			int prevV = HaveShield() ? GetShield().block : 0;
			if(value)
			{
				*value = v * 2;
				*prevValue = prevV * 2;
			}
			return v > prevV;
		}
	case IT_ARMOR:
		if(!IsSet(data->flags, F_MAGE))
			return IsBetterArmor(item->ToArmor(), value, prevValue);
		else
		{
			int v = item->aiValue;
			int prevV = HaveArmor() ? GetArmor().aiValue : 0;
			if(value)
			{
				*value = v;
				*prevValue = prevV;
			}
			return v > prevV;
		}
	case IT_AMULET:
		{
			float v = GetItemAiValue(item);
			float prevV = HaveAmulet() ? GetItemAiValue(&GetAmulet()) : 0;
			if(value)
			{
				*value = (int)v;
				*prevValue = (int)prevV;
			}
			return v > prevV && v > 0;
		}
	case IT_RING:
		{
			float v = GetItemAiValue(item);
			float prevV;
			ITEM_SLOT bestSlot;
			if(!slots[SLOT_RING1])
			{
				prevV = 0;
				bestSlot = SLOT_RING1;
			}
			else if(!slots[SLOT_RING2])
			{
				prevV = 0;
				bestSlot = SLOT_RING2;
			}
			else
			{
				float prevV1 = GetItemAiValue(slots[SLOT_RING1]),
					prevV2 = GetItemAiValue(slots[SLOT_RING2]);
				if(prevV1 > prevV2)
				{
					prevV = prevV2;
					bestSlot = SLOT_RING2;
				}
				else
				{
					prevV = prevV1;
					bestSlot = SLOT_RING1;
				}
			}
			if(value)
			{
				*value = (int)v;
				*prevValue = (int)prevV;
			}
			if(targetSlot)
				*targetSlot = bestSlot;
			return v > prevV && v > 0;
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

	const float* priorities = stats->tagPriorities;
	const ItemTag* tags;
	if(item->type == IT_AMULET)
		tags = item->ToAmulet().tag;
	else
		tags = item->ToRing().tag;

	float value = (float)item->aiValue;
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
const Item* Unit::GetIIndexItem(int iIndex) const
{
	if(iIndex >= 0)
	{
		if(iIndex < (int)items.size())
			return items[iIndex].item;
		else
			return nullptr;
	}
	else
	{
		ITEM_SLOT slotType = IIndexToSlot(iIndex);
		if(slotType < SLOT_MAX)
			return slots[slotType];
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
			return meshInst->mesh->GetAnimation(NAMES::aniTakeWeapon);
		else
		{
			Mesh::Animation* anim = meshInst->mesh->GetAnimation(NAMES::aniTakeWeaponNoShield);
			if(!anim)
			{
				// brak animacja wyci¹gania broni bez tarczy, u¿yj zwyk³ej
				return meshInst->mesh->GetAnimation(NAMES::aniTakeWeapon);
			}
			else
				return anim;
		}
	}
	else
		return meshInst->mesh->GetAnimation(NAMES::aniTakeBow);
}

//=================================================================================================
// lower value is better
float Unit::GetBlockSpeed() const
{
	const bool inAttack = (action == A_ATTACK);
	if(!IsSet(data->flags2, F2_FIXED_STATS) && Get(AttributeId::STR) < GetShield().reqStr)
		return inAttack ? 0.5f : 0.3f;
	else
		return inAttack ? 0.3f : 0.1f;
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
	float effectMres = GetEffectMulInv(EffectId::MagicResistance);
	return mres * effectMres;
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
			if(e.onAttack && e.effect == EffectId::Backstab)
				mod += e.power;
		}
	}
	mod += GetEffectSum(EffectId::Backstab);
	return mod;
}

//=================================================================================================
bool Unit::HaveEffect(EffectId e, int value) const
{
	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == e && (value == -1 || it->value == value))
			return true;
	}
	return false;
}

//=================================================================================================
void Unit::RemoveEffects(bool send)
{
	if(effectsToRemove.empty())
		return;

	send = (send && player && !player->IsLocal() && Net::IsServer());

	while(!effectsToRemove.empty())
	{
		uint index = effectsToRemove.back();
		Effect e = effects[index];
		if(send)
		{
			NetChangePlayer& c = player->playerInfo->PushChange(NetChangePlayer::REMOVE_EFFECT);
			c.id = (int)e.effect;
			c.count = (int)e.source;
			c.a1 = e.sourceId;
			c.a2 = e.value;
		}

		effectsToRemove.pop_back();
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
		case EffectId::Rooted:
			b |= BUFF_ROOTED;
			break;
		case EffectId::SlowMove:
			b |= BUFF_SLOW_MOVE;
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
	if(GetOrder() == ORDER_AUTO_TALK && order->autoTalk == AutoTalkMode::Leader && !team->IsLeader(unit))
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
	StatInfo statInfo;

	for(const Effect& e : effects)
	{
		if(e.effect == EffectId::Attribute && e.value == (int)a)
		{
			value += (int)e.power;
			statInfo.Mod((int)e.power);
		}
	}

	if(state)
		*state = statInfo.GetState();
	return value;
}

//=================================================================================================
int Unit::Get(SkillId s, StatState* state, bool skillBonus) const
{
	int index = (int)s;
	int base = stats->skill[index];
	int value = base;
	StatInfo statInfo;

	for(const Effect& e : effects)
	{
		if(e.effect == EffectId::Skill && e.value == (int)s)
		{
			value += (int)e.power;
			statInfo.Mod((int)e.power);
		}
	}

	// apply skill synergy
	if(skillBonus && base > 0)
	{
		switch(s)
		{
		case SkillId::LIGHT_ARMOR:
		case SkillId::HEAVY_ARMOR:
			{
				int otherVal = GetBase(SkillId::MEDIUM_ARMOR);
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		case SkillId::MEDIUM_ARMOR:
			{
				int otherVal = max(GetBase(SkillId::LIGHT_ARMOR), GetBase(SkillId::HEAVY_ARMOR));
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		case SkillId::SHORT_BLADE:
			{
				int otherVal = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::AXE));
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		case SkillId::LONG_BLADE:
			{
				int otherVal = max(max(GetBase(SkillId::SHORT_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::AXE));
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		case SkillId::BLUNT:
			{
				int otherVal = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::SHORT_BLADE)), GetBase(SkillId::AXE));
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		case SkillId::AXE:
			{
				int otherVal = max(max(GetBase(SkillId::LONG_BLADE), GetBase(SkillId::BLUNT)), GetBase(SkillId::SHORT_BLADE));
				if(otherVal > value)
					value += (otherVal - value) / 2;
			}
			break;
		}
	}

	if(state)
		*state = statInfo.GetState();
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
			player->recalculateLevel = true;
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
			player->recalculateLevel = true;
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
	float mobilityBonus = GetEffectSum(EffectId::Mobility);
	mobility += mobilityBonus;

	if(armor)
	{
		// calculate armor mobility (0-100)
		int armorMobility = armor->mobility;
		int skill = min(Get(armor->GetSkill()), 100);
		armorMobility += skill / 4;
		if(armorMobility > 100)
			armorMobility = 100;
		int str = Get(AttributeId::STR);
		if(str < armor->reqStr)
		{
			armorMobility -= armor->reqStr - str;
			if(armorMobility < 0)
				armorMobility = 0;
		}

		// multiply mobility by armor mobility
		mobility = (float(armorMobility) / 100 * mobility);
	}

	LoadState loadState = GetLoadState();
	switch(loadState)
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
	int bestValue = GetBase(WeaponTypeInfo::info[0].skill);
	for(int i = 1; i < WT_MAX; ++i)
	{
		int value = GetBase(WeaponTypeInfo::info[i].skill);
		if(value > bestValue)
		{
			best = (WEAPON_TYPE)i;
			bestValue = value;
		}
	}
	return best;
}

//=================================================================================================
ARMOR_TYPE Unit::GetBestArmorType() const
{
	ARMOR_TYPE best = (ARMOR_TYPE)0;
	int bestValue = GetBase(GetArmorTypeSkill(best));
	for(int i = 1; i < AT_MAX; ++i)
	{
		int value = GetBase(GetArmorTypeSkill((ARMOR_TYPE)i));
		if(value > bestValue)
		{
			best = (ARMOR_TYPE)i;
			bestValue = value;
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
void Unit::SetAnimationAtEnd(cstring animName)
{
	if(animName)
		meshInst->SetToEnd(animName);
	else
		meshInst->SetToEnd();
}

//=================================================================================================
void Unit::SetDontAttack(bool dontAttack)
{
	if(this->dontAttack == dontAttack)
		return;
	this->dontAttack = dontAttack;
	if(ai)
		ai->changeAiMode = true;
}

//=================================================================================================
void Unit::UpdateStaminaAction()
{
	if(usable)
	{
		if(IsSet(usable->base->useFlags, BaseUsable::SLOW_STAMINA_RESTORE))
			staminaAction = SA_RESTORE_SLOW;
		else
			staminaAction = SA_RESTORE_MORE;
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
			staminaAction = SA_RESTORE;
			break;
		case A_SHOOT:
			if(animationState < AS_SHOOT_SHOT)
				staminaAction = SA_RESTORE_SLOW;
			else
				staminaAction = SA_RESTORE;
			break;
		case A_EAT:
		case A_DRINK:
		case A_ANIMATION:
		case A_USE_USABLE:
		default:
			staminaAction = SA_RESTORE_MORE;
			break;
		case A_BLOCK:
		case A_DASH:
			staminaAction = SA_RESTORE_SLOW;
			break;
		case A_BASH:
		case A_ATTACK:
			staminaAction = SA_DONT_RESTORE;
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
			if(staminaAction == SA_RESTORE_MORE)
				staminaAction = SA_RESTORE;
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
		NetChange& c = Net::PushChange(NetChange::UPDATE_MP);
		c.unit = this;
	}
}

//=================================================================================================
void Unit::RemoveStamina(float value)
{
	stamina -= value;
	if(stamina < -staminaMax / 2)
		stamina = -staminaMax / 2;
	staminaTimer = STAMINA_RESTORE_TIMER;
	if(player && Net::IsLocal())
		player->Train(TrainWhat::Stamina, value, 0);
	if(Net::IsServer() && IsTeamMember())
	{
		NetChange& c = Net::PushChange(NetChange::UPDATE_STAMINA);
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
float Unit::GetStaminaMod(const Item& item) const
{
	int reqStr;
	switch(item.type)
	{
	case IT_WEAPON:
		reqStr = item.ToWeapon().reqStr;
		break;
	case IT_SHIELD:
		reqStr = item.ToShield().reqStr;
		break;
	default:
		return 1.f;
	}

	const int str = Get(AttributeId::STR);
	if(str >= reqStr)
		return 1.f;
	else if(str * 2 < reqStr)
		return 2.f;
	else
		return 1.f + float(reqStr - str) / reqStr;
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
		if(resMgr->IsLoadScreen())
		{
			if(mode == CREATE_MESH::LOAD)
				resMgr->LoadInstant(mesh);
			else if(!mesh->IsLoaded())
				game->unitsMeshLoad.push_back(pair<Unit*, bool>(this, mode == CREATE_MESH::ON_WORLDMAP));
			if(data->state == ResourceState::NotLoaded)
			{
				resMgr->Load(mesh);
				if(data->sounds)
				{
					for(int i = 0; i < SOUND_MAX; ++i)
					{
						for(SoundPtr sound : data->sounds->sounds[i])
							resMgr->Load(sound);
					}
				}
				if(data->tex)
				{
					for(TexOverride& texOverride : data->tex->textures)
					{
						if(texOverride.diffuse)
							resMgr->Load(texOverride.diffuse);
					}
				}
				data->state = ResourceState::Loading;
				resMgr->AddTask(this, TaskCallback([](TaskData& td)
				{
					Unit* unit = static_cast<Unit*>(td.ptr);
					unit->data->state = ResourceState::Loaded;
				}));
			}
		}
		else
		{
			resMgr->Load(mesh);
			if(data->sounds)
			{
				for(int i = 0; i < SOUND_MAX; ++i)
				{
					for(SoundPtr sound : data->sounds->sounds[i])
						resMgr->Load(sound);
				}
			}
			if(data->tex)
			{
				for(TexOverride& texOverride : data->tex->textures)
				{
					if(texOverride.diffuse)
						resMgr->Load(texOverride.diffuse);
				}
			}
			data->state = ResourceState::Loaded;
		}
	}

	if(mesh->IsLoaded())
	{
		if(mode != CREATE_MESH::AFTER_PRELOAD)
		{
			assert(!meshInst);
			meshInst = new MeshInstance(mesh);
			meshInst->ptr = this;

			if(mode != CREATE_MESH::ON_WORLDMAP)
			{
				if(IsAlive())
				{
					meshInst->Play(NAMES::aniStand, PLAY_PRIO1 | PLAY_NO_BLEND, 0);
					animation = currentAnimation = ANI_STAND;
				}
				else
				{
					SetAnimationAtEnd(NAMES::aniDie);
					animation = currentAnimation = ANI_DIE;
				}
			}

			if(meshInst->mesh->head.nGroups > 1)
				meshInst->groups[1].state = 0;
			if(humanData)
				humanData->ApplyScale(meshInst);
		}
		else
		{
			assert(meshInst);
			meshInst->ApplyPreload(mesh);
		}
	}
	else if(mode == CREATE_MESH::PRELOAD)
	{
		assert(!meshInst);
		meshInst = new MeshInstance(nullptr);
	}
}

//=================================================================================================
void Unit::UseUsable(Usable* newUsable)
{
	if(newUsable)
	{
		assert(!usable && !newUsable->user);
		usable = newUsable;
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
void Unit::RevealName(bool setName)
{
	if(!hero || hero->knowName)
		return;
	hero->knowName = true;
	if(Net::IsServer())
	{
		NetChange& c = Net::PushChange(NetChange::TELL_NAME);
		c.unit = this;
		c.id = setName ? 1 : 0;
	}
}

//=================================================================================================
bool Unit::GetKnownName() const
{
	if(IsHero())
		return hero->knowName;
	return true;
}

//=================================================================================================
void Unit::SetKnownName(bool known)
{
	if(!hero || hero->knowName)
		return;
	hero->knowName = true;
	if(Net::IsServer())
	{
		NetChange& c = Net::PushChange(NetChange::TELL_NAME);
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
		return !IsSet(aiMode, AI_MODE_IDLE);
}

//=================================================================================================
bool Unit::IsAssist() const
{
	if(Net::IsLocal())
		return assist;
	else
		return IsSet(aiMode, AI_MODE_ASSIST);
}

//=================================================================================================
bool Unit::IsDontAttack() const
{
	if(Net::IsLocal())
		return dontAttack;
	else
		return IsSet(aiMode, AI_MODE_DONT_ATTACK);
}

//=================================================================================================
bool Unit::WantAttackTeam() const
{
	if(Net::IsLocal())
		return attackTeam;
	else
		return IsSet(aiMode, AI_MODE_ATTACK_TEAM);
}

//=================================================================================================
byte Unit::GetAiMode() const
{
	byte mode = 0;
	if(dontAttack)
		mode |= AI_MODE_DONT_ATTACK;
	if(assist)
		mode |= AI_MODE_ASSIST;
	if(ai->state != AIController::Idle)
		mode |= AI_MODE_IDLE;
	if(attackTeam)
		mode |= AI_MODE_ATTACK_TEAM;
	return mode;
}

//=================================================================================================
void Unit::BreakAction(BREAK_ACTION_MODE mode, bool notify, bool allowAnimation)
{
	if(notify && Net::IsServer())
	{
		NetChange& c = Net::PushChange(NetChange::BREAK_ACTION);
		c.unit = this;
	}

	switch(action)
	{
	case A_POSITION:
		return;
	case A_SHOOT:
		if(bowInstance)
			gameLevel->FreeBowInstance(bowInstance);
		action = A_NONE;
		break;
	case A_DRINK:
		if(animationState == AS_DRINK_START)
		{
			if(Net::IsLocal())
				AddItem2(usedItem, 1, usedItemIsTeam, false);
			if(mode != BREAK_ACTION_MODE::FALL)
				usedItem = nullptr;
		}
		else
			usedItem = nullptr;
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			meshInst->Deactivate(1);
		action = A_NONE;
		break;
	case A_EAT:
		if(animationState < AS_EAT_EFFECT)
		{
			if(Net::IsLocal())
				AddItem2(usedItem, 1, usedItemIsTeam, false);
			if(mode != BREAK_ACTION_MODE::FALL)
				usedItem = nullptr;
		}
		else
			usedItem = nullptr;
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			meshInst->Deactivate(1);
		action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		SetTakeHideWeaponAnimationToEnd(false, true);
		action = A_NONE;
		break;
	case A_BLOCK:
		if(mode != BREAK_ACTION_MODE::ON_LEAVE)
			meshInst->Deactivate(1);
		action = A_NONE;
		break;
	case A_DASH:
		if(act.dash.ability->effect == Ability::Stun && mode != BREAK_ACTION_MODE::ON_LEAVE)
			meshInst->Deactivate(1);
		if(act.dash.hit)
			act.dash.hit->Free();
		break;
	}

	if(mode == BREAK_ACTION_MODE::ON_LEAVE)
	{
		if(usable)
		{
			StopUsingUsable();
			UseUsable(nullptr);
			visualPos = pos = targetPos;
		}
	}
	else if(usable && !(player && player->action == PlayerAction::LootContainer))
	{
		if(mode == BREAK_ACTION_MODE::INSTANT)
		{
			action = A_NONE;
			SetAnimationAtEnd(NAMES::aniStand);
			UseUsable(nullptr);
			usedItem = nullptr;
			animation = ANI_STAND;
		}
		else
		{
			targetPos2 = targetPos = pos;
			const Item* prevUsedItem = usedItem;
			StopUsingUsable(mode != BREAK_ACTION_MODE::FALL && notify);
			if(prevUsedItem && slots[SLOT_WEAPON] == prevUsedItem && !HaveShield())
				SetWeaponStateInstant(WeaponState::Taken, W_ONE_HANDED);
			else if(mode == BREAK_ACTION_MODE::FALL)
				usedItem = prevUsedItem;
			action = A_POSITION;
			animationState = AS_POSITION_NORMAL;
		}

		if(Net::IsLocal() && IsAI() && ai->state == AIController::Idle && ai->st.idle.action != AIController::Idle_None)
		{
			ai->st.idle.action = AIController::Idle_None;
			ai->timer = Random(1.f, 2.f);
		}
	}
	else
	{
		if(!Any(action, A_ANIMATION, A_STAND_UP) || !allowAnimation)
			action = A_NONE;
	}

	meshInst->ClearEndResult();

	if(mode != BREAK_ACTION_MODE::INSTANT)
	{
		if(meshInst->groups.size() == 2u)
			meshInst->Deactivate(1);
		animation = ANI_STAND;
	}

	if(mode == BREAK_ACTION_MODE::ON_LEAVE)
		return;

	if(IsPlayer())
	{
		player->nextAction = NA_NONE;
		if(player->isLocal)
		{
			player->data.abilityReady = nullptr;
			gameGui->inventory->EndLock();
			if(gameGui->inventory->mode > I_INVENTORY)
				game->CloseInventory();

			if(gameGui->craft->visible)
				gameGui->craft->Hide();

			if(player->action == PlayerAction::Talk)
			{
				if(Net::IsLocal())
				{
					player->actionUnit->busy = Busy_No;
					player->actionUnit->lookTarget = nullptr;
					player->dialogCtx->dialogMode = false;
				}
				else
					game->dialogContext.dialogMode = false;
				lookTarget = nullptr;
				player->action = PlayerAction::None;
			}
		}
		else if(Net::IsLocal())
		{
			if(player->action == PlayerAction::Talk)
			{
				player->actionUnit->busy = Busy_No;
				player->actionUnit->lookTarget = nullptr;
				player->dialogCtx->dialogMode = false;
				lookTarget = nullptr;
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
	ACTION prevAction = action;
	liveState = FALLING;

	if(Net::IsLocal())
	{
		// przerwij akcjê
		BreakAction(BREAK_ACTION_MODE::FALL);

		// wstawanie
		raiseTimer = Random(5.f, 7.f);
		timer = 1.f;

		// event
		if(eventHandler)
			eventHandler->HandleUnitEvent(UnitEventHandler::FALL, this);

		// komunikat
		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::FALL);
			c.unit = this;
		}

		UpdatePhysics();

		if(player && player->isLocal)
			player->data.beforePlayer = BP_NONE;
	}
	else
	{
		// przerwij akcjê
		BreakAction(BREAK_ACTION_MODE::FALL);

		if(player && player->isLocal)
		{
			raiseTimer = Random(5.f, 7.f);
			player->data.beforePlayer = BP_NONE;
		}
	}

	if(prevAction == A_ANIMATION)
	{
		action = A_NONE;
		currentAnimation = ANI_STAND;
	}
	animation = ANI_DIE;
	talking = false;
	meshInst->needUpdate = true;
}

//=================================================================================================
void Unit::TryStandup(float dt)
{
	if(inArena != -1 || game->deathScreen != 0)
		return;

	bool ok = false;
	raiseTimer -= dt;
	timer -= dt;

	if(liveState == DEAD)
	{
		if(IsTeamMember())
		{
			if(hp > 0.f && raiseTimer > 0.5f)
				raiseTimer = 0.5f;

			if(raiseTimer <= 0.f)
			{
				RemoveEffect(EffectId::Poison);

				if(alcohol > hpmax)
				{
					// could stand up but is too drunk
					liveState = FALL;
					UpdatePhysics();
				}
				else
				{
					// check for near enemies
					ok = true;
					for(Unit* unit : locPart->units)
					{
						if(unit->IsStanding() && IsEnemy(*unit) && Vec3::Distance(pos, unit->pos) <= ALERT_RANGE && gameLevel->CanSee(*this, *unit))
						{
							AlertAllies(unit);
							ok = false;
							break;
						}
					}
				}

				if(!ok)
				{
					if(hp > 0.f)
						raiseTimer = 0.5f;
					else
						raiseTimer = Random(1.f, 2.f);
					timer = 0.5f;
				}
			}
			else if(timer <= 0.f)
			{
				for(Unit* unit : locPart->units)
				{
					if(unit->IsStanding() && IsEnemy(*unit) && Vec3::Distance(pos, unit->pos) <= ALERT_RANGE && gameLevel->CanSee(*this, *unit))
					{
						AlertAllies(unit);
						break;
					}
				}
				timer = 0.5f;
			}
		}
	}
	else if(liveState == FALL)
	{
		if(raiseTimer <= 0.f)
		{
			if(alcohol < hpmax)
				ok = true;
			else
				raiseTimer = Random(1.f, 2.f);
		}
	}

	if(ok)
		Standup();
}

//=================================================================================================
void Unit::Standup(bool warp, bool leave)
{
	HealPoison();
	liveState = ALIVE;
	if(leave)
	{
		action = A_NONE;
		animation = ANI_STAND;
	}
	else
	{
		action = A_STAND_UP;
		Mesh::Animation* anim = meshInst->mesh->GetAnimation("wstaje2");
		if(anim)
		{
			meshInst->Play(anim, PLAY_ONCE | PLAY_PRIO3, 0);
			animation = ANI_PLAY;
		}
		else
			animation = ANI_STAND;
	}
	usedItem = nullptr;
	if(weaponState != WeaponState::Hidden)
	{
		WeaponType targetWeapon;
		if(weaponState == WeaponState::Taken || weaponState == WeaponState::Taking)
			targetWeapon = weaponTaken;
		else
			targetWeapon = weaponHiding;
		if((targetWeapon == W_ONE_HANDED && !HaveWeapon())
			|| (targetWeapon == W_BOW && !HaveBow()))
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
			gameLevel->WarpUnit(*this, pos);
	}

	if(Net::IsServer() && gameLevel->ready && !leave)
	{
		NetChange& c = Net::PushChange(NetChange::STAND_UP);
		c.unit = this;
	}
}

//=================================================================================================
void Unit::Die(Unit* killer)
{
	ACTION prevAction = action;

	if(liveState == FALL)
	{
		// unit already on ground, add blood
		gameLevel->CreateBlood(*locPart, *this);
		liveState = DEAD;
	}
	else
		liveState = DYING;
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
					NetChange& c = Net::PushChange(NetChange::MARK_UNIT);
					c.unit = this;
					c.id = 1;
				}
				break;
			}
		}

		// notify about death
		for(vector<Unit*>::iterator it = locPart->units.begin(), end = locPart->units.end(); it != end; ++it)
		{
			if((*it)->IsPlayer() || !(*it)->IsStanding() || !IsFriend(**it))
				continue;

			if(Vec3::Distance(pos, (*it)->pos) <= ALERT_RANGE && gameLevel->CanSee(*this, **it))
				(*it)->ai->morale -= 2.f;
		}

		// rising team members / check is location cleared
		if(IsTeamMember())
		{
			raiseTimer = Random(5.f, 7.f);
			timer = 1.f;
		}
		else
		{
			gameLevel->CheckIfLocationCleared();
			if(IsHero() && hero->otherTeam)
				hero->otherTeam->Remove(this);
		}

		// event
		if(eventHandler)
			eventHandler->HandleUnitEvent(UnitEventHandler::DIE, this);
		ScriptEvent event(EVENT_DIE);
		event.onDie.unit = this;
		FireEvent(event);

		// message
		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::DIE);
			c.unit = this;
		}

		// stats
		++gameStats->totalKills;
		if(killer && killer->IsPlayer())
		{
			++killer->player->kills;
			killer->player->statFlags |= STAT_KILLS;
		}
		if(IsPlayer())
		{
			++player->knocks;
			player->statFlags |= STAT_KNOCKS;
			if(player->isLocal)
				game->pc->data.beforePlayer = BP_NONE;
		}

		// give experience
		if(killer)
		{
			if(killer->inArena == -1)
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
		if(player && player->isLocal)
		{
			raiseTimer = Random(5.f, 7.f);
			player->data.beforePlayer = BP_NONE;
		}
	}

	// end boss music
	if(IsSet(data->flags2, F2_BOSS))
		gameLevel->EndBossFight();

	if(prevAction == A_ANIMATION)
	{
		action = A_NONE;
		currentAnimation = ANI_STAND;
	}
	animation = ANI_DIE;
	talking = false;
	meshInst->needUpdate = true;

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
	soundMgr->PlaySound2d(gameRes->sCoins);

	// animacja wyrzucania
	action = A_ANIMATION;
	meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

	if(Net::IsLocal())
	{
		// stwórz przedmiot
		GroundItem* groundItem = new GroundItem;
		groundItem->Register();
		groundItem->item = Item::gold;
		groundItem->count = count;
		groundItem->teamCount = 0;
		groundItem->pos = pos;
		groundItem->pos.x -= sin(rot) * 0.25f;
		groundItem->pos.z -= cos(rot) * 0.25f;
		groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
		locPart->AddGroundItem(groundItem);

		// wyœlij info o animacji
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::DROP_ITEM);
			c.unit = this;
		}
	}
	else
	{
		// wyœlij komunikat o wyrzucaniu z³ota
		NetChange& c = Net::PushChange(NetChange::DROP_GOLD);
		c.id = count;
	}
}

//=================================================================================================
bool Unit::IsDrunkman() const
{
	if(IsSet(data->flags, F_AI_DRUNKMAN))
		return true;
	else if(IsSet(data->flags3, F3_DRUNK_MAGE))
		return questMgr->questMages2->magesState < Quest_Mages2::State::MageCured;
	else if(IsSet(data->flags3, F3_DRUNKMAN_AFTER_CONTEST))
		return questMgr->questContest->state == Quest_Contest::CONTEST_DONE;
	else
		return false;
}

//=================================================================================================
void Unit::PlaySound(Sound* sound, float range)
{
	soundMgr->PlaySound3d(sound, GetHeadSoundPos(), range);
}

//=================================================================================================
void Unit::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const Vec3& hitpoint, bool dmg)
{
	bool playBodyHit = false;
	if(mat == MAT_SPECIAL_UNIT)
	{
		if(HaveArmor())
		{
			mat = GetArmor().material;
			if(dmg && mat != MAT_BODY)
				playBodyHit = true;
		}
		else
			mat = data->mat;
	}

	soundMgr->PlaySound3d(gameRes->GetMaterialSound(mat2, mat), hitpoint, HIT_SOUND_DIST);
	if(playBodyHit)
		soundMgr->PlaySound3d(gameRes->GetMaterialSound(mat2, MAT_BODY), hitpoint, HIT_SOUND_DIST);

	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::HIT_SOUND);
		c.pos = hitpoint;
		c.id = mat2;
		c.count = mat;

		if(playBodyHit)
		{
			NetChange& c2 = Net::PushChange(NetChange::HIT_SOUND);
			c2.pos = hitpoint;
			c2.id = mat2;
			c2.count = MAT_BODY;
		}
	}
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
	phyWorld->addCollisionObject(cobj, CG_UNIT);

	if(position)
		UpdatePhysics();
}

//=================================================================================================
void Unit::UpdatePhysics(const Vec3* targetPos)
{
	Vec3 phyPos = targetPos ? *targetPos : pos;
	if(liveState == ALIVE)
		phyPos.y += max(MIN_H, GetUnitHeight()) / 2;

	cobj->getWorldTransform().setOrigin(ToVector3(phyPos));
	physics->UpdateAabb(cobj);
}

//=================================================================================================
Sound* Unit::GetSound(SOUND_ID soundId) const
{
	if(data->sounds->Have(soundId))
		return data->sounds->Random(soundId);
	return nullptr;
}

//=================================================================================================
bool Unit::SetWeaponState(bool takesOut, WeaponType type, bool send)
{
	if(takesOut)
	{
		switch(weaponState)
		{
		case WeaponState::Hidden:
			// wyjmij bron
			meshInst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
			action = A_TAKE_WEAPON;
			animationState = AS_TAKE_WEAPON_START;
			weaponTaken = type;
			weaponState = WeaponState::Taking;
			if(IsPlayer())
				player->lastWeapon = type;
			break;
		case WeaponState::Hiding:
			if(weaponHiding == type)
			{
				if(animationState == AS_TAKE_WEAPON_START)
				{
					// jeszcze nie schowa³ tej broni, wy³¹cz grupê
					meshInst->Deactivate(1);
					action = usable ? A_USE_USABLE : A_NONE;
					weaponTaken = weaponHiding;
					weaponHiding = W_NONE;
					weaponState = WeaponState::Taken;
				}
				else
				{
					// schowa³ broñ, zacznij wyci¹gaæ
					meshInst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					weaponTaken = weaponHiding;
					weaponHiding = W_NONE;
					weaponState = WeaponState::Taking;
					animationState = AS_TAKE_WEAPON_START;
					if(IsPlayer())
						player->lastWeapon = type;
				}
			}
			else
			{
				if(animationState == AS_TAKE_WEAPON_MOVED)
				{
					// unit is hiding weapon & animation almost ended, start taking out weapon
					meshInst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					animationState = AS_TAKE_WEAPON_START;
					weaponTaken = type;
					weaponHiding = W_NONE;
					weaponState = WeaponState::Taking;
					if(IsPlayer())
						player->lastWeapon = type;
				}
				else
				{
					// set next weapon to take out
					weaponTaken = type;
				}
			}
			break;
		case WeaponState::Taking:
			if(weaponTaken == type)
				return false;
			if(animationState == AS_TAKE_WEAPON_START)
			{
				// jeszcze nie wyj¹³ broni, zacznij wyjmowaæ inn¹
				meshInst->Play(GetTakeWeaponAnimation(type == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				weaponState = WeaponState::Taking;
				weaponHiding = W_NONE;
				if(IsPlayer())
					player->lastWeapon = type;
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				SetBit(meshInst->groups[1].state, MeshInstance::FLAG_BACK);
				weaponState = WeaponState::Hiding;
				weaponHiding = weaponTaken;
			}
			weaponTaken = type;
			animationState = AS_TAKE_WEAPON_START;
			break;
		case WeaponState::Taken:
			if(weaponTaken == type)
				return false;
			// hide previous weapon
			if(action == A_SHOOT)
				gameLevel->FreeBowInstance(bowInstance);
			meshInst->Play(GetTakeWeaponAnimation(weaponTaken), PLAY_ONCE | PLAY_PRIO1 | PLAY_BACK, 1);
			action = A_TAKE_WEAPON;
			animationState = AS_TAKE_WEAPON_START;
			weaponState = WeaponState::Hiding;
			weaponHiding = weaponTaken;
			weaponTaken = type;
			break;
		}
	}
	else // chowa
	{
		switch(weaponState)
		{
		case WeaponState::Hidden:
			// schowana to schowana, nie ma co sprawdzaæ czy to ta
			return false;
		case WeaponState::Hiding:
			if(type == W_NONE)
				weaponTaken = W_NONE;
			if(weaponHiding == type || type == W_NONE)
				return false;
			// chowa z³¹ broñ, zamieñ
			weaponHiding = type;
			break;
		case WeaponState::Taking:
			if(animationState == AS_TAKE_WEAPON_START)
			{
				// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
				meshInst->Deactivate(1);
				action = usable ? A_USE_USABLE : A_NONE;
				weaponTaken = W_NONE;
				weaponState = WeaponState::Hidden;
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				SetBit(meshInst->groups[1].state, MeshInstance::FLAG_BACK);
				weaponHiding = weaponTaken;
				weaponTaken = W_NONE;
				weaponState = WeaponState::Hiding;
				animationState = AS_TAKE_WEAPON_START;
			}
			break;
		case WeaponState::Taken:
			// zacznij chowaæ
			if(action == A_SHOOT)
				gameLevel->FreeBowInstance(bowInstance);
			meshInst->Play(GetTakeWeaponAnimation(weaponTaken == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
			weaponHiding = weaponTaken;
			weaponTaken = W_NONE;
			weaponState = WeaponState::Hiding;
			action = A_TAKE_WEAPON;
			animationState = AS_TAKE_WEAPON_START;
			break;
		}
	}

	if(send && Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::TAKE_WEAPON);
		c.unit = this;
		c.id = takesOut ? 0 : 1;
	}

	return true;
}

//=============================================================================
void Unit::SetWeaponStateInstant(WeaponState weaponState, WeaponType type)
{
	this->weaponState = weaponState;
	this->weaponTaken = type;
	weaponHiding = W_NONE;
}

//=============================================================================
void Unit::SetTakeHideWeaponAnimationToEnd(bool hide, bool breakAction)
{
	if(hide || weaponState == WeaponState::Hiding)
	{
		if(breakAction && animationState == AS_TAKE_WEAPON_START)
			SetWeaponStateInstant(WeaponState::Taken, weaponHiding);
		else
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	}
	else if(weaponState == WeaponState::Taking)
	{
		if(breakAction && animationState == AS_TAKE_WEAPON_START)
			SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		else
			SetWeaponStateInstant(WeaponState::Taken, weaponTaken);
	}
	if(action == A_TAKE_WEAPON)
		action = A_NONE;
}

//=================================================================================================
void Unit::UpdateInventory(bool notify)
{
	bool changes = false;
	int index = 0;
	const Item* prevSlots[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
		prevSlots[i] = slots[i];

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->teamCount != 0 || !CanWear(it->item))
			continue;

		ITEM_SLOT targetSlot;
		if(IsBetterItem(it->item, nullptr, nullptr, &targetSlot))
		{
			if(slots[targetSlot])
			{
				RemoveItemEffects(slots[targetSlot], targetSlot);
				ApplyItemEffects(it->item, targetSlot);
				std::swap(slots[targetSlot], it->item);
			}
			else
			{
				ApplyItemEffects(it->item, targetSlot);
				slots[targetSlot] = it->item;
				it->item = nullptr;
			}
			changes = true;
		}
	}

	if(changes)
	{
		RemoveNullItems(items);
		SortItems(items);

		if(Net::IsOnline() && net->activePlayers > 1 && notify)
		{
			for(int i = 0; i < SLOT_MAX; ++i)
			{
				if(slots[i] != prevSlots[i] && IsVisible((ITEM_SLOT)i))
				{
					NetChange& c = Net::PushChange(NetChange::CHANGE_EQUIPMENT);
					c.unit = this;
					c.id = i;
				}
			}
		}
	}
}

//=================================================================================================
bool Unit::IsEnemy(Unit& u, bool ignoreDontAttack) const
{
	if(inArena == -1 && u.inArena == -1)
	{
		if(!ignoreDontAttack)
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
				return team->craziesAttack || WantAttackTeam();
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
				return team->craziesAttack;
			else
				return true;
		}
		else
			return true;
	}
	else
	{
		if(inArena == -1 || u.inArena == -1)
			return false;
		else if(inArena == u.inArena)
			return false;
		else
			return true;
	}
}

//=================================================================================================
bool Unit::IsFriend(Unit& u, bool checkArenaAttack) const
{
	if(inArena == -1 && u.inArena == -1)
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
		if(checkArenaAttack)
		{
			// prevent attacking viewers when on arena
			if(inArena == -1 || u.inArena == -1)
				return true;
		}
		return inArena == u.inArena;
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
		if(gameLevel->ready)
		{
			for(ItemSlot& slot : stock->items)
				gameRes->PreloadItem(slot.item);
		}
	}
}

//=================================================================================================
void Unit::AddDialog(Quest2* quest, GameDialog* dialog, int priority)
{
	assert(quest && dialog);
	dialogs.push_back({ dialog, quest, priority });
	quest->AddDialogPtr(this);
}

//=================================================================================================
void Unit::AddDialogS(Quest2* quest, const string& dialogId, int priority)
{
	GameDialog* dialog = quest->GetDialog(dialogId);
	if(!dialog)
		throw ScriptException("Missing quest dialog '%s'.", dialogId.c_str());
	AddDialog(quest, dialog, priority);
}

//=================================================================================================
void Unit::RemoveDialog(Quest2* quest, bool cleanup)
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
void Unit::AddEventHandler(Quest2* quest, EventType type)
{
	assert(Any(type, EVENT_DIE, EVENT_ENTER, EVENT_KICK, EVENT_LEAVE, EVENT_PICKUP, EVENT_RECRUIT, EVENT_UPDATE));

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
void Unit::RemoveEventHandler(Quest2* quest, EventType type, bool cleanup)
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
	if(hero && hero->teamMember)
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
				ai->locTimer = Random(5.f, 10.f);
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
		if(!team->craziesAttack)
		{
			team->craziesAttack = true;
			if(Net::IsOnline())
				Net::PushChange(NetChange::CHANGE_FLAGS);
		}
	}
	else
	{
		for(Unit* unit : locPart->units)
		{
			if(unit->dontAttack && unit->IsEnemy(*team->leader, true) && !IsSet(unit->data->flags, F_PEACEFUL))
			{
				unit->dontAttack = false;
				unit->ai->changeAiMode = true;
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
		ai->locTimer = Random(5.f, 10.f);
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
	order->moveType = MOVE_RUN;
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
		order->autoTalk = AutoTalkMode::Yes;
		order->timer = AUTO_TALK_WAIT;
	}
	else
	{
		order->autoTalk = AutoTalkMode::Leader;
		order->timer = 0.f;
	}
	order->autoTalkDialog = dialog;
	order->autoTalkQuest = quest;
	return order;
}

//=================================================================================================
UnitOrderEntry* Unit::OrderAttackObject(Usable* usable)
{
	assert(usable);
	OrderReset();
	order->order = ORDER_ATTACK_OBJECT;
	order->usable = usable;
	return order;
}

//=================================================================================================
void Unit::Talk(cstring text, int playAnim)
{
	assert(text && Net::IsLocal());

	gameGui->levelGui->AddSpeechBubble(this, text);

	// animation
	int ani = 0;
	if(playAnim != 0 && data->type == UNIT_TYPE::HUMAN)
	{
		if(playAnim == -1)
		{
			if(Rand() % 3 != 0)
				ani = Rand() % 2 + 1;
		}
		else
			ani = Clamp(playAnim, 1, 2);
	}
	if(ani != 0)
	{
		meshInst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		animation = ANI_PLAY;
		action = A_ANIMATION;
	}

	// sent
	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::TALK);
		c.unit = this;
		c.str = StringPool.Get();
		*c.str = text;
		c.id = ani;
		c.count = 0;
		net->netStrs.push_back(c.str);
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
float Unit::GetBashSpeed() const
{
	float sp;
	const int str = Get(AttributeId::STR);
	const int reqStr = GetShield().reqStr;
	if(str >= reqStr)
		sp = 2.f;
	else if(str * 2 <= reqStr)
		sp = 1.f;
	else
		sp = 1.f + float(reqStr - str) / (reqStr / 2);
	return sp * GetStaminaAttackSpeedMod();
}

//=================================================================================================
void Unit::RotateTo(const Vec3& pos, float dt)
{
	float dir = Vec3::LookAtAngle(this->pos, pos);
	if(!Equal(rot, dir))
	{
		const float rotSpeed = GetRotationSpeed() * dt;
		const float rotDif = AngleDiff(rot, dir);
		if(ShortestArc(rot, dir) > 0.f)
			animation = ANI_RIGHT;
		else
			animation = ANI_LEFT;
		if(rotDif < rotSpeed)
			rot = dir;
		else
		{
			const float d = Sign(ShortestArc(rot, dir)) * rotSpeed;
			rot = Clip(rot + d);
		}
		changed = true;
	}
}

//=================================================================================================
void Unit::RotateTo(const Vec3& pos)
{
	rot = Vec3::LookAtAngle(this->pos, pos);
	if(!gameLevel->ready && ai)
		ai->startRot = rot;
	else
		changed = true;
}

//=================================================================================================
void Unit::RotateTo(float rot)
{
	this->rot = rot;
	if(!gameLevel->ready && ai)
		ai->startRot = rot;
	else
		changed = true;
}

//=================================================================================================
void Unit::StopUsingUsable(bool send)
{
	animation = ANI_STAND;
	animationState = AS_USE_USABLE_MOVE_TO_ENDPOINT;
	timer = 0.f;
	usedItem = nullptr;

	const float unitRadius = GetUnitRadius();

	gameLevel->globalCol.clear();
	Level::IgnoreObjects ignore{};
	const Unit* ignoredUnits[2]{ this };
	ignore.ignoredUnits = ignoredUnits;
	gameLevel->GatherCollisionObjects(*locPart, gameLevel->globalCol, pos, 2.f + unitRadius, &ignore);

	Vec3 tmpPos = targetPos;
	bool ok = false;
	float radius = unitRadius;

	for(int i = 0; i < 20; ++i)
	{
		if(!gameLevel->Collide(gameLevel->globalCol, tmpPos, unitRadius))
		{
			if(i != 0 && locPart->haveTerrain)
				tmpPos.y = gameLevel->terrain->GetH(tmpPos);
			targetPos = tmpPos;
			ok = true;
			break;
		}

		tmpPos = targetPos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

		if(i < 10)
			radius += 0.2f;
	}

	assert(ok);

	if(cobj)
		UpdatePhysics(&targetPos);

	if(send && Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::USE_USABLE);
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
		if(order->autoTalk == AutoTalkMode::Wait)
		{
			order->autoTalk = AutoTalkMode::Yes;
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
	static vector<NearUnit> nearUnits;

	const bool leaderMode = (order->autoTalk == AutoTalkMode::Leader);

	for(Unit& u : team->members)
	{
		if(u.IsPlayer())
		{
			// if not leader (in leader mode) or busy - don't check this unit
			if((leaderMode && &u != team->leader)
				|| (u.player->dialogCtx->dialogMode || u.busy != Busy_No
				|| !u.IsStanding() || u.player->action != PlayerAction::None))
				continue;
			float dist = Vec3::Distance(pos, u.pos);
			if(dist <= 8.f || leaderMode)
				nearUnits.push_back({ &u, dist });
		}
	}

	// if no nearby available players found, return
	if(nearUnits.empty())
	{
		if(order->autoTalk == AutoTalkMode::Wait)
			order->autoTalk = AutoTalkMode::Yes;
		return;
	}

	// sort by distance
	std::sort(nearUnits.begin(), nearUnits.end(), [](const NearUnit& nu1, const NearUnit& nu2) { return nu1.dist < nu2.dist; });

	// get near player that don't have enemies nearby
	PlayerController* talkPlayer = nullptr;
	for(auto& nearUnit : nearUnits)
	{
		Unit& talkTarget = *nearUnit.unit;
		if(gameLevel->CanSee(*this, talkTarget))
		{
			bool ok = true;
			for(vector<Unit*>::iterator it2 = locPart->units.begin(), end2 = locPart->units.end(); it2 != end2; ++it2)
			{
				Unit& checkUnit = **it2;
				if(&talkTarget == &checkUnit || this == &checkUnit)
					continue;

				if(checkUnit.IsAlive() && talkTarget.IsEnemy(checkUnit) && checkUnit.IsAI() && !checkUnit.dontAttack
					&& Vec3::Distance2d(talkTarget.pos, checkUnit.pos) < ALERT_RANGE && gameLevel->CanSee(checkUnit, talkTarget))
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				talkPlayer = talkTarget.player;
				break;
			}
		}
	}

	// start dialog or wait
	if(talkPlayer)
	{
		if(order->autoTalk == AutoTalkMode::Yes)
		{
			order->autoTalk = AutoTalkMode::Wait;
			order->timer = 1.f;
		}
		else
		{
			talkPlayer->StartDialog(this, order->autoTalkDialog, order->autoTalkQuest);
			OrderNext();
		}
	}
	else if(order->autoTalk == AutoTalkMode::Wait)
	{
		order->autoTalk = AutoTalkMode::Yes;
		order->timer = AUTO_TALK_WAIT;
	}

	nearUnits.clear();
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
			bonus = (float)data->spellPower / 20;
		else
			bonus = float(Get(AttributeId::INT) - 25 + Get(SkillId::MYSTIC_MAGIC) + CalculateMagicPower() * 10) / 20;
	}
	else if(IsSet(ability.flags, Ability::Strength))
		bonus = float(Get(AttributeId::STR));
	else
		bonus = float(CalculateMagicPower()) / 10 + level;
	return bonus * ability.dmgBonus + ability.dmg;
}

//=================================================================================================
void Unit::CastSpell()
{
	Ability& ability = *act.cast.ability;

	Mesh::Point* point = meshInst->mesh->GetPoint(NAMES::pointCast);
	Vec3 coord;

	if(point)
	{
		meshInst->SetupBones();
		Matrix m = point->mat * meshInst->matBones[point->bone] * (Matrix::Scale(data->scale) * Matrix::RotationY(rot) * Matrix::Translation(pos));
		coord = Vec3::TransformZero(m);
	}
	else
		coord = GetHeadPoint();

	float dmg = GetAbilityPower(ability);

	switch(ability.type)
	{
	case Ability::Ball:
	case Ability::Point:
		{
			int count = 1;
			if(IsSet(ability.flags, Ability::Triple))
				count = 3;

			float expectedRot = Clip(-Vec3::Angle2d(coord, targetPos) + PI / 2);
			float currentRot = Clip(rot + PI);
			AdjustAngle(currentRot, expectedRot, ToRadians(10.f));

			for(int i = 0; i < count; ++i)
			{
				Bullet* bullet = new Bullet;
				locPart->lvlPart->bullets.push_back(bullet);

				bullet->Register();
				bullet->isArrow = false;
				bullet->level = level + CalculateMagicPower();
				bullet->backstab = 0.25f;
				bullet->pos = coord;
				bullet->attack = dmg;
				bullet->rot = Vec3(0, Clip(currentRot + (IsPlayer() ? Random(-0.025f, 0.025f) : Random(-0.05f, 0.05f))), 0);
				bullet->mesh = ability.mesh;
				bullet->tex = ability.tex;
				bullet->texSize = ability.size;
				bullet->speed = ability.speed;
				bullet->timer = ability.range / (ability.speed - 1);
				bullet->owner = this;
				bullet->trail = nullptr;
				bullet->pe = nullptr;
				bullet->ability = &ability;
				bullet->startPos = bullet->pos;

				// ustal z jak¹ si³¹ rzuciæ kul¹
				if(ability.type == Ability::Ball)
				{
					float dist = Vec3::Distance2d(pos, targetPos);
					float t = dist / ability.speed;
					float h = (targetPos.y + (IsPlayer() ? Random(-0.25f, 0.25f) : Random(-0.5f, 0.5f))) - bullet->pos.y;
					bullet->yspeed = h / t + (10.f * t) / 2;
				}
				else if(ability.type == Ability::Point)
				{
					float dist = Vec3::Distance2d(pos, targetPos);
					float t = dist / ability.speed;
					float h = (targetPos.y + (IsPlayer() ? Random(-0.25f, 0.25f) : Random(-0.5f, 0.5f))) - bullet->pos.y;
					bullet->yspeed = h / t;
				}

				if(ability.texParticle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = ability.texParticle;
					pe->emissionInterval = 0.1f;
					pe->life = -1;
					pe->particleLife = 0.5f;
					pe->emissions = -1;
					pe->spawn = Int2(3, 4);
					pe->maxParticles = 50;
					pe->pos = bullet->pos;
					pe->speedMin = Vec3(-1, -1, -1);
					pe->speedMax = Vec3(1, 1, 1);
					pe->posMin = Vec3(-ability.size, -ability.size, -ability.size);
					pe->posMax = Vec3(ability.size, ability.size, ability.size);
					pe->size = Vec2(ability.sizeParticle, 0.f);
					pe->alpha = Vec2(1.f, 0.f);
					pe->mode = 1;
					pe->Init();
					locPart->lvlPart->pes.push_back(pe);
					bullet->pe = pe;
				}

				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::CREATE_SPELL_BALL);
					c << ability.hash
						<< bullet->id
						<< id
						<< bullet->startPos
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
			e->locPart = locPart;
			e->hitted.push_back(this);
			e->dmg = dmg;
			e->owner = this;
			e->ability = &ability;
			e->startPos = pos;

			Vec3 hitpoint;
			Unit* hitted;

			targetPos.y += Random(-0.5f, 0.5f);
			Vec3 dir = targetPos - coord;
			dir.Normalize();
			Vec3 target = coord + dir * ability.range;

			if(gameLevel->RayTest(coord, target, this, hitpoint, hitted))
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

			locPart->lvlPart->electros.push_back(e);

			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::CREATE_ELECTRO);
				c.extraId = e->id;
				c.pos = e->lines[0].from;
				memcpy(c.f, &e->lines[0].to, sizeof(Vec3));
			}
		}
		else if(ability.effect == Ability::Drain)
		{
			Vec3 hitpoint;
			Unit* hitted;

			targetPos.y += Random(-0.5f, 0.5f);

			if(gameLevel->RayTest(coord, targetPos, this, hitpoint, hitted) && hitted)
			{
				// trafiono w cel
				if(!IsSet(hitted->data->flags2, F2_BLOODLESS) && !IsFriend(*hitted, true))
				{
					Drain& drain = Add1(locPart->lvlPart->drains);
					drain.target = this;

					hitted->GiveDmg(dmg, this, nullptr, DMG_MAGICAL);

					drain.pe = locPart->lvlPart->pes.back();
					drain.t = 0.f;
					drain.pe->manualDelete = 1;
					drain.pe->speedMin = Vec3(-3, 0, -3);
					drain.pe->speedMax = Vec3(3, 3, 3);

					dmg *= hitted->CalculateMagicResistance();
					hp += dmg;
					if(hp > hpmax)
						hp = hpmax;

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
						c.unit = this;

						NetChange& c2 = Net::PushChange(NetChange::CREATE_DRAIN);
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
				for(Unit* u : locPart->units)
				{
					if(u->liveState == DEAD
						&& !IsEnemy(*u)
						&& IsSet(u->data->flags, F_UNDEAD)
						&& Vec3::Distance(targetPos, u->pos) < 0.5f)
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
			if(!target || target->locPart != locPart || Vec3::Distance(pos, target->pos) > ability.range * 1.5f)
				break;

			if(ability.effect == Ability::Raise)
			{
				// raise unit
				target->Standup();
				target->hp = target->hpmax;
				if(Net::IsServer())
				{
					NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
					c.unit = target;
				}

				// particle effect
				Vec2 bounds(target->GetUnitRadius(), 0);
				Vec3 pos = target->GetLootCenter();
				pos.y += 0.5f;
				gameLevel->CreateSpellParticleEffect(locPart, &ability, pos, bounds);
			}
			else if(ability.effect == Ability::Heal)
			{
				// heal target
				if(!IsSet(target->data->flags, F_UNDEAD) && !IsSet(target->data->flags2, F2_CONSTRUCT) && target->hp != target->hpmax)
				{
					target->hp = Min(target->hp + dmg, target->hpmax);
					if(Net::IsServer())
					{
						NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
						c.unit = target;
					}
				}

				// particle effect
				Vec2 bounds(target->GetUnitRadius(), target->GetUnitHeight());
				Vec3 pos;
				if(target->liveState == Unit::FALL || target->liveState == Unit::DEAD)
					pos = target->GetLootCenter();
				else
				{
					pos = target->pos;
					pos.y += bounds.y / 2;
				}
				gameLevel->CreateSpellParticleEffect(locPart, &ability, pos, bounds);
			}
		}
		break;
	case Ability::Summon:
		{
			// despawn old
			Unit* existingUnit = gameLevel->FindUnit([&](Unit* u) { return u->summoner == this; });
			if(existingUnit)
			{
				team->RemoveMember(existingUnit);
				gameLevel->RemoveUnit(existingUnit);
			}

			// spawn new
			Unit* newUnit = gameLevel->SpawnUnitNearLocation(*locPart, targetPos, *ability.unit, nullptr, level);
			if(newUnit)
			{
				newUnit->summoner = this;
				newUnit->inArena = inArena;
				if(newUnit->inArena != -1)
					game->arena->units.push_back(newUnit);
				team->AddMember(newUnit, HeroType::Visitor);
				newUnit->order->unit = this; // follow summoner
				gameLevel->SpawnUnitEffect(*newUnit);
			}
		}
		break;
	case Ability::Aggro:
		for(Unit* u : locPart->units)
		{
			if(u->toRemove || this == u || !u->IsStanding() || u->IsPlayer() || !IsFriend(*u) || u->ai->state == AIController::Fighting
				|| u->ai->alertTarget || u->dontAttack)
				continue;

			if(Vec3::Distance(pos, u->pos) <= ability.range && gameLevel->CanSee(*this, *u))
			{
				u->ai->alertTarget = ai->target;
				u->ai->alertTargetPos = ai->targetLastPos;
			}
		}
		break;
	case Ability::SummonAway:
		for(int i = 0; i < ability.count; ++i)
		{
			float angle = Random(MAX_ANGLE);
			Vec3 targetPos = pos + Vec3(sin(angle) * ability.range, 0, cos(angle) * ability.range);
			Unit* newUnit = gameLevel->SpawnUnitNearLocation(*locPart, targetPos, *ability.unit, nullptr, level);
			if(newUnit)
			{
				newUnit->inArena = inArena;
				if(newUnit->inArena != -1)
					game->arena->units.push_back(newUnit);
				gameLevel->SpawnUnitEffect(*newUnit);
				newUnit->ai->alertTarget = ai->target;
				newUnit->ai->alertTargetPos = ai->targetLastPos;
			}
		}
		break;
	case Ability::Charge:
		action = A_DASH;
		act.dash.ability = &ability;
		act.dash.rot = Clip(rot + PI);
		act.dash.hit = UnitList::Get();
		act.dash.hit->Clear();
		timer = 1.f;
		speed = ability.range / timer;
		break;
	case Ability::Trap:
		{
			// despawn old
			gameLevel->RemoveOldTrap(ability.trap, this, ability.count);

			// spawn new
			Trap* trap = gameLevel->CreateTrap(targetPos, ability.trap->type);
			trap->owner = this;
			trap->attack = ability.dmg + ability.dmgBonus * (level + CalculateMagicPower());

			// particle effect
			if(ability.texParticle)
				gameLevel->CreateSpellParticleEffect(locPart, &ability, targetPos, Vec2::Zero);
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
	if(ability.soundCast)
	{
		soundMgr->PlaySound3d(ability.soundCast, coord, ability.soundCastDist);
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::SPELL_SOUND);
			c.extraId = 0;
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
			talkTimer += dt;
			meshInst->needUpdate = true;
		}

		if(Net::IsLocal())
		{
			// hurt sound timer since last hit, timer since last stun (to prevent stunlock)
			hurtTimer -= dt;
			lastBash -= dt;

			if(moved)
			{
				// unstuck units after being force moved (by bull charge)
				static vector<Unit*> targets;
				targets.clear();
				float t;
				bool inDash = false;
				gameLevel->ContactTest(cobj, [&](btCollisionObject* obj, bool first)
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
							inDash = true;
						targets.push_back(target);
						return true;
					}
				}, true);

				if(!inDash)
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
						gameLevel->LineTest(cobj->getCollisionShape(), GetPhysicsPos(), center, [&](btCollisionObject* obj, bool)
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
	if(animation != currentAnimation)
	{
		changed = true;
		switch(animation)
		{
		case ANI_WALK:
			meshInst->Play(NAMES::aniMove, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				meshInst->groups[0].speed = GetWalkSpeed() / data->walkSpeed;
			break;
		case ANI_WALK_BACK:
			meshInst->Play(NAMES::aniMove, PLAY_BACK | PLAY_PRIO1, 0);
			if(!Net::IsClient())
				meshInst->groups[0].speed = GetWalkSpeed() / data->walkSpeed;
			break;
		case ANI_RUN:
			meshInst->Play(NAMES::aniRun, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				meshInst->groups[0].speed = GetRunSpeed() / data->runSpeed;
			break;
		case ANI_LEFT:
			meshInst->Play(NAMES::aniLeft, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				meshInst->groups[0].speed = GetRotationSpeed() / data->rotSpeed;
			break;
		case ANI_RIGHT:
			meshInst->Play(NAMES::aniRight, PLAY_PRIO1, 0);
			if(!Net::IsClient())
				meshInst->groups[0].speed = GetRotationSpeed() / data->rotSpeed;
			break;
		case ANI_STAND:
			meshInst->Play(NAMES::aniStand, PLAY_PRIO1, 0);
			break;
		case ANI_BATTLE:
			meshInst->Play(NAMES::aniBattle, PLAY_PRIO1, 0);
			break;
		case ANI_BATTLE_BOW:
			meshInst->Play(NAMES::aniBattleBow, PLAY_PRIO1, 0);
			break;
		case ANI_DIE:
			meshInst->Play(NAMES::aniDie, PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
			break;
		case ANI_PLAY:
			break;
		case ANI_IDLE:
			break;
		case ANI_KNEELS:
			meshInst->Play("kleka", PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
			break;
		default:
			assert(0);
			break;
		}
		currentAnimation = animation;
	}

	// aktualizuj animacjê
	meshInst->Update(dt);

	// koniec animacji idle
	if(animation == ANI_IDLE && meshInst->IsEnded())
	{
		meshInst->Play(NAMES::aniStand, PLAY_PRIO1, 0);
		animation = ANI_STAND;
	}

	// zmieñ stan z umiera na umar³ i stwórz krew (chyba ¿e tylko upad³)
	if(!IsStanding())
	{
		// move corpse that thanks to animation is now not lootable
		if(Net::IsLocal() && (Any(liveState, DYING, FALLING) || action == A_POSITION_CORPSE))
		{
			Vec3 center = GetLootCenter();
			gameLevel->globalCol.clear();
			Level::IgnoreObjects ignore{};
			ignore.ignoreUnits = true;
			ignore.ignoreDoors = true;
			gameLevel->GatherCollisionObjects(*locPart, gameLevel->globalCol, center, 0.25f, &ignore);
			if(gameLevel->Collide(gameLevel->globalCol, center, 0.25f))
			{
				Vec3 dir = pos - center;
				dir.y = 0;
				pos += dir * dt * 2;
				visualPos = pos;
				moved = true;
				action = A_POSITION_CORPSE;
				changed = true;
			}
			else
				action = A_NONE;
		}

		if(Any(liveState, DYING, FALLING) && meshInst->IsEnded())
		{
			if(liveState == DYING)
			{
				liveState = DEAD;
				gameLevel->CreateBlood(*locPart, *this);
				if(summoner && Net::IsLocal())
				{
					team->RemoveMember(this);
					action = A_DESPAWN;
					timer = Random(2.5f, 5.f);
					summoner = nullptr;
				}
			}
			else
				liveState = FALL;
		}

		if(action != A_POSITION && action != A_DESPAWN)
		{
			UpdateStaminaAction();
			return;
		}
	}

	const int groupIndex = meshInst->mesh->head.nGroups - 1;

	// aktualizuj akcjê
	switch(action)
	{
	case A_NONE:
		break;
	case A_TAKE_WEAPON:
		if(weaponState == WeaponState::Taking)
		{
			if(animationState == AS_TAKE_WEAPON_START && (meshInst->GetProgress(1) >= data->frames->t[F_TAKE_WEAPON] || meshInst->IsEnded(1)))
				animationState = AS_TAKE_WEAPON_MOVED;
			if(meshInst->IsEnded(1))
			{
				weaponState = WeaponState::Taken;
				if(usable)
				{
					action = A_USE_USABLE;
					animationState = AS_USE_USABLE_USING;
				}
				else
					action = A_NONE;
				meshInst->Deactivate(1);
			}
		}
		else
		{
			// chowanie broni
			if(animationState == AS_TAKE_WEAPON_START && (meshInst->GetProgress(1) <= data->frames->t[F_TAKE_WEAPON] || meshInst->IsEnded(1)))
				animationState = AS_TAKE_WEAPON_MOVED;
			if(weaponTaken != W_NONE && (animationState == AS_TAKE_WEAPON_MOVED || meshInst->IsEnded(1)))
			{
				meshInst->Play(GetTakeWeaponAnimation(weaponTaken == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
				weaponState = WeaponState::Taking;
				weaponHiding = W_NONE;
				animationState = AS_TAKE_WEAPON_START;
				if(IsPlayer())
					player->lastWeapon = weaponTaken;

				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::TAKE_WEAPON);
					c.unit = this;
					c.id = 0;
				}
			}
			else if(meshInst->IsEnded(1))
			{
				weaponState = WeaponState::Hidden;
				weaponHiding = W_NONE;
				action = A_NONE;
				meshInst->Deactivate(1);

				if(IsLocalPlayer())
				{
					switch(player->nextAction)
					{
					// unequip item
					case NA_REMOVE:
						if(slots[player->nextActionData.slot])
							gameGui->inventory->invMine->RemoveSlotItem(player->nextActionData.slot);
						break;
					// equip item after unequiping old one
					case NA_EQUIP:
					case NA_EQUIP_DRAW:
						{
							int index = player->GetNextActionItemIndex();
							if(index != -1)
							{
								gameGui->inventory->invMine->EquipSlotItem(index);
								if(player->nextAction == NA_EQUIP_DRAW)
								{
									switch(player->nextActionData.item->type)
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
						if(slots[player->nextActionData.slot])
							gameGui->inventory->invMine->DropSlotItem(player->nextActionData.slot);
						break;
					// use consumable
					case NA_CONSUME:
						{
							int index = player->GetNextActionItemIndex();
							if(index != -1)
								gameGui->inventory->invMine->ConsumeItem(index);
						}
						break;
					// use usable
					case NA_USE:
						if(!player->nextActionData.usable->user)
							player->UseUsable(player->nextActionData.usable, true);
						break;
					// sell equipped item
					case NA_SELL:
						if(slots[player->nextActionData.slot])
							gameGui->inventory->invTradeMine->SellSlotItem(player->nextActionData.slot);
						break;
					// put equipped item in container
					case NA_PUT:
						if(slots[player->nextActionData.slot])
							gameGui->inventory->invTradeMine->PutSlotItem(player->nextActionData.slot);
						break;
					// give equipped item
					case NA_GIVE:
						if(slots[player->nextActionData.slot])
							gameGui->inventory->invTradeMine->GiveSlotItem(player->nextActionData.slot);
						break;
					}

					player->ClearNextAction();

					if(action == A_NONE && usable && !usable->container)
					{
						action = A_USE_USABLE;
						animationState = AS_USE_USABLE_USING;
					}
				}
				else if(Net::IsLocal() && !fakeUnit && IsAI() && ai->potion != -1)
				{
					ConsumeItem(ai->potion);
					ai->potion = -1;
				}
			}
		}
		break;
	case A_SHOOT:
		staminaTimer = STAMINA_RESTORE_TIMER;
		if(!meshInst)
		{
			// fix na skutek, nie na przyczynê ;(
			game->ReportError(4, Format("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", GetName(), liveState, action, animation,
				currentAnimation, animationState));
			goto endShooting;
		}
		if(fakeUnit)
		{
			if(meshInst->IsEnded(1))
			{
				meshInst->Deactivate(1);
				action = A_NONE;
				gameLevel->FreeBowInstance(bowInstance);
				break;
			}
		}
		else if(animationState == AS_SHOOT_PREPARE)
		{
			if(meshInst->GetProgress(1) > 20.f / 40)
				meshInst->groups[1].time = 20.f / 40 * meshInst->groups[1].anim->length;
		}
		else if(animationState == AS_SHOOT_CAN)
		{
			if(Net::IsLocal() && meshInst->GetProgress(1) > 20.f / 40)
			{
				Bullet* bullet = new Bullet;
				locPart->lvlPart->bullets.push_back(bullet);

				meshInst->SetupBones();

				Mesh::Point* point = meshInst->mesh->GetPoint(NAMES::pointWeapon);
				assert(point);

				Matrix m2 = point->mat * meshInst->matBones[point->bone] * (Matrix::Scale(data->scale) * Matrix::RotationY(rot) * Matrix::Translation(pos));

				bullet->Register();
				bullet->isArrow = true;
				bullet->attack = CalculateAttack(&GetBow());
				bullet->level = level;
				bullet->backstab = GetBackstabMod(&GetBow());
				bullet->rot = Vec3(PI / 2, rot + PI, 0);
				bullet->pos = Vec3::TransformZero(m2);
				bullet->mesh = gameRes->aArrow;
				bullet->speed = GetArrowSpeed();
				bullet->timer = ARROW_TIMER;
				bullet->owner = this;
				bullet->pe = nullptr;
				bullet->ability = act.shoot.ability;
				bullet->tex = nullptr;
				bullet->poisonAttack = 0.f;
				bullet->startPos = bullet->pos;

				if(bullet->ability && bullet->ability->mesh)
					bullet->mesh = bullet->ability->mesh;

				float dist = Vec3::Distance2d(bullet->pos, targetPos);
				float t = dist / bullet->speed;
				float h = targetPos.y - bullet->pos.y;
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
				if(bullet->ability)
					sk += 10;

				if(sk < 50)
				{
					int chance;
					Vec2 variation;
					if(sk < 10)
					{
						chance = 100;
						variation.x = PI / 16;
						variation.y = 5.f;
					}
					else if(sk < 20)
					{
						chance = 80;
						variation.x = PI / 20;
						variation.y = 4.f;
					}
					else if(sk < 30)
					{
						chance = 60;
						variation.x = PI / 26;
						variation.y = 3.f;
					}
					else if(sk < 40)
					{
						chance = 40;
						variation.x = PI / 34;
						variation.y = 2.f;
					}
					else
					{
						chance = 20;
						variation.x = PI / 48;
						variation.y = 1.f;
					}

					if(Rand() % 100 < chance)
						bullet->rot.y += RandomNormalized(variation.x);
					if(Rand() % 100 < chance)
						bullet->yspeed += RandomNormalized(variation.y);
				}

				bullet->rot.y = Clip(bullet->rot.y);

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				if(bullet->ability)
				{
					tpe->color1 = bullet->ability->color;
					tpe->color2 = tpe->color1;
					tpe->color2.w = 0;
				}
				else
				{
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
				}
				tpe->Init(50);
				locPart->lvlPart->tpes.push_back(tpe);
				bullet->trail = tpe;

				act.shoot.ability = nullptr;

				soundMgr->PlaySound3d(gameRes->sBow[Rand() % 2], bullet->pos, SHOOT_SOUND_DIST);

				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::SHOOT_ARROW);
					c << bullet->id
						<< id
						<< bullet->startPos
						<< bullet->rot.x
						<< bullet->rot.y
						<< bullet->speed
						<< bullet->yspeed
						<< (bullet->ability ? bullet->ability->hash : 0);
				}
			}
			if(meshInst->GetProgress(1) > 20.f / 40)
				animationState = AS_SHOOT_SHOT;
		}
		else if(meshInst->GetProgress(1) > 35.f / 40)
		{
			animationState = AS_SHOOT_FINISHED;
			if(meshInst->IsEnded(1))
			{
			endShooting:
				meshInst->Deactivate(1);
				action = A_NONE;
				gameLevel->FreeBowInstance(bowInstance);
				if(Net::IsLocal() && IsAI())
				{
					float v = 1.f - float(Get(SkillId::BOW)) / 100;
					ai->nextAttack = Random(v / 2, v);
				}
				break;
			}
		}
		bowInstance->groups[0].time = min(meshInst->groups[1].time, bowInstance->groups[0].anim->length);
		bowInstance->needUpdate = true;
		break;
	case A_ATTACK:
		staminaTimer = STAMINA_RESTORE_TIMER;
		if(fakeUnit)
		{
			if(meshInst->IsEnded(1))
			{
				animation = ANI_BATTLE;
				currentAnimation = ANI_STAND;
				action = A_NONE;
			}
		}
		else if(animationState == AS_ATTACK_PREPARE)
		{
			float t = GetAttackFrame(0);
			float p = meshInst->GetProgress(groupIndex);
			if(p > t)
			{
				if(Net::IsLocal() && IsAI())
				{
					float speed = (1.f + GetAttackSpeed()) * GetStaminaAttackSpeedMod();
					meshInst->groups[groupIndex].speed = speed;
					act.attack.power = 2.f;
					animationState = AS_ATTACK_CAN_HIT;
					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::ATTACK);
						c.unit = this;
						c.id = AID_Attack;
						c.f[1] = speed;
					}
				}
				else
					meshInst->groups[groupIndex].time = t * meshInst->groups[groupIndex].anim->length;
			}
			else if(IsPlayer() && Net::IsLocal())
			{
				const Weapon& weapon = GetWeapon();
				float dif = p - timer;
				float staminaToRemoveTotal = weapon.GetInfo().stamina / 2 * GetStaminaMod(weapon);
				float staminaUsed = dif / t * staminaToRemoveTotal;
				timer = p;
				RemoveStamina(staminaUsed);
			}
		}
		else
		{
			if(animationState == AS_ATTACK_CAN_HIT && meshInst->GetProgress(groupIndex) > GetAttackFrame(0))
			{
				if(Net::IsLocal() && act.attack.hitted != 2 && meshInst->GetProgress(groupIndex) >= GetAttackFrame(1))
				{
					if(DoAttack())
						act.attack.hitted = 2;
				}
				if(meshInst->GetProgress(groupIndex) >= GetAttackFrame(2) || meshInst->IsEnded(groupIndex))
				{
					// koniec mo¿liwego ataku
					animationState = AS_ATTACK_FINISHED;
					meshInst->groups[groupIndex].speed = 1.f;
					act.attack.run = false;
				}
			}
			if(animationState == AS_ATTACK_FINISHED && meshInst->IsEnded(groupIndex))
			{
				if(groupIndex == 0)
				{
					animation = ANI_BATTLE;
					currentAnimation = ANI_STAND;
				}
				else
					meshInst->Deactivate(1);
				action = A_NONE;
				if(Net::IsLocal() && IsAI())
				{
					float v = 1.f - float(Get(SkillId::ONE_HANDED_WEAPON)) / 100;
					ai->nextAttack = Random(v / 2, v);
				}
			}
		}
		break;
	case A_BLOCK:
		break;
	case A_BASH:
		staminaTimer = STAMINA_RESTORE_TIMER;
		if(animationState == AS_BASH_ANIMATION)
		{
			if(meshInst->GetProgress(1) >= data->frames->t[F_BASH])
				animationState = AS_BASH_CAN_HIT;
		}
		if(Net::IsLocal() && animationState == AS_BASH_CAN_HIT)
		{
			if(DoShieldSmash())
				animationState = AS_BASH_HITTED;
		}
		if(meshInst->IsEnded(1))
		{
			action = A_NONE;
			meshInst->Deactivate(1);
		}
		break;
	case A_DRINK:
		{
			float p = meshInst->GetProgress(1);
			if(p >= 28.f / 52.f && animationState == AS_DRINK_START)
			{
				PlaySound(gameRes->sGulp, DRINK_SOUND_DIST);
				animationState = AS_DRINK_EFFECT;
				if(Net::IsLocal())
					ApplyConsumableEffect(usedItem->ToConsumable());
			}
			if(p >= 49.f / 52.f && animationState == AS_DRINK_EFFECT)
			{
				animationState = AS_DRINK_EFFECT;
				usedItem = nullptr;
			}
			if(meshInst->IsEnded(1))
			{
				if(usable)
				{
					animationState = AS_USE_USABLE_USING;
					action = A_USE_USABLE;
				}
				else
					action = A_NONE;
				meshInst->Deactivate(1);
			}
		}
		break;
	case A_EAT:
		{
			float p = meshInst->GetProgress(1);
			if(p >= 32.f / 70 && animationState == AS_EAT_START)
			{
				animationState = AS_EAT_SOUND;
				PlaySound(gameRes->sEat, EAT_SOUND_DIST);
			}
			if(p >= 48.f / 70 && animationState == AS_EAT_SOUND)
			{
				animationState = AS_EAT_EFFECT;
				if(Net::IsLocal())
					ApplyConsumableEffect(usedItem->ToConsumable());
			}
			if(p >= 60.f / 70 && animationState == AS_EAT_EFFECT)
			{
				animationState = AS_EAT_END;
				usedItem = nullptr;
			}
			if(meshInst->IsEnded(1))
			{
				if(usable)
				{
					animationState = AS_USE_USABLE_USING;
					action = A_USE_USABLE;
				}
				else
					action = A_NONE;
				meshInst->Deactivate(1);
			}
		}
		break;
	case A_PAIN:
		if(meshInst->mesh->head.nGroups == 2)
		{
			if(meshInst->IsEnded(1))
			{
				action = A_NONE;
				meshInst->Deactivate(1);
			}
		}
		else if(meshInst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_BATTLE;
		}
		break;
	case A_CAST:
		if(animationState != AS_CAST_KNEEL)
		{
			if(Net::IsLocal())
			{
				if(IsOtherPlayer()
					? animationState == AS_CAST_TRIGGER
					: (animationState == AS_CAST_ANIMATION && meshInst->GetProgress(groupIndex) >= data->frames->t[F_CAST]))
				{
					animationState = AS_CAST_CASTED;
					CastSpell();
				}
			}
			else if(IsLocalPlayer() && animationState == AS_CAST_ANIMATION && meshInst->GetProgress(groupIndex) >= data->frames->t[F_CAST])
			{
				animationState = AS_CAST_CASTED;
				NetChange& c = Net::PushChange(NetChange::CAST_SPELL);
				c.pos = targetPos;
			}
			if(meshInst->IsEnded(groupIndex))
			{
				action = A_NONE;
				if(groupIndex == 1)
					meshInst->Deactivate(1);
				else
					animation = ANI_BATTLE;
				if(Net::IsLocal() && IsAI())
					ai->nextAttack = Random(0.25f, 0.75f);
			}
		}
		else
		{
			timer -= dt;
			if(timer <= 0.f)
			{
				action = A_NONE;
				animation = ANI_STAND;
				CastSpell();
			}
		}
		break;
	case A_ANIMATION:
		if(meshInst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_STAND;
			currentAnimation = (Animation)-1;
		}
		break;
	case A_USE_USABLE:
		{
			bool allowMove = true;
			if(Net::IsServer())
			{
				if(IsOtherPlayer())
					allowMove = false;
			}
			else if(Net::IsClient())
			{
				if(!IsPlayer() || !player->isLocal)
					allowMove = false;
			}
			if(animationState == AS_USE_USABLE_MOVE_TO_ENDPOINT)
			{
				timer += dt;
				if(allowMove && timer >= 0.5f)
				{
					action = A_NONE;
					visualPos = pos = targetPos;
					changed = true;
					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::USE_USABLE);
						c.unit = this;
						c.id = usable->id;
						c.count = USE_USABLE_END;
					}
					if(Net::IsLocal())
						UseUsable(nullptr);
					break;
				}

				if(allowMove)
				{
					// przesuñ postaæ
					visualPos = pos = Vec3::Lerp(targetPos2, targetPos, timer * 2);

					// obrót
					float targetRot = Vec3::LookAtAngle(targetPos, usable->pos);
					float dif = AngleDiff(rot, targetRot);
					if(NotZero(dif))
					{
						const float rotSpeed = GetRotationSpeed() * 2 * dt;
						const float arc = ShortestArc(rot, targetRot);

						if(dif <= rotSpeed)
							rot = targetRot;
						else
							rot = Clip(rot + Sign(arc) * rotSpeed);
					}

					changed = true;
				}
			}
			else
			{
				BaseUsable& bu = *usable->base;

				if(animationState > AS_USE_USABLE_MOVE_TO_OBJECT)
				{
					// odtwarzanie dŸwiêku
					if(bu.sound)
					{
						if(meshInst->GetProgress() >= bu.soundTimer)
						{
							if(animationState == AS_USE_USABLE_USING)
							{
								animationState = AS_USE_USABLE_USING_SOUND;
								soundMgr->PlaySound3d(bu.sound, GetCenter(), Usable::SOUND_DIST);
								if(Net::IsServer())
								{
									NetChange& c = Net::PushChange(NetChange::USABLE_SOUND);
									c.unit = this;
								}
							}
						}
						else if(animationState == AS_USE_USABLE_USING_SOUND)
							animationState = AS_USE_USABLE_USING;
					}
				}
				else if(Net::IsLocal() || IsLocalPlayer())
				{
					// ustal docelowy obrót postaci
					float targetRot;
					if(bu.limitRot == 0)
						targetRot = rot;
					else if(bu.limitRot == 1)
					{
						float rot1 = Clip(act.useUsable.rot + PI / 2),
							dif1 = AngleDiff(rot1, usable->rot),
							rot2 = Clip(usable->rot + PI),
							dif2 = AngleDiff(rot1, rot2);

						if(dif1 < dif2)
							targetRot = usable->rot;
						else
							targetRot = rot2;
					}
					else if(bu.limitRot == 2)
						targetRot = usable->rot;
					else if(bu.limitRot == 3)
					{
						float rot1 = Clip(act.useUsable.rot + PI),
							dif1 = AngleDiff(rot1, usable->rot),
							rot2 = Clip(usable->rot + PI),
							dif2 = AngleDiff(rot1, rot2);

						if(dif1 < dif2)
							targetRot = usable->rot;
						else
							targetRot = rot2;
					}
					else
						targetRot = usable->rot + PI;
					targetRot = Clip(targetRot);

					// obrót w strone obiektu
					const float dif = AngleDiff(rot, targetRot);
					const float rotSpeed = GetRotationSpeed() * 2;
					if(allowMove && NotZero(dif))
					{
						const float rotSpeedDt = rotSpeed * dt;
						if(dif <= rotSpeedDt)
							rot = targetRot;
						else
						{
							const float arc = ShortestArc(rot, targetRot);
							rot = Clip(rot + Sign(arc) * rotSpeedDt);
						}
					}

					// czy musi siê obracaæ zanim zacznie siê przesuwaæ?
					if(dif < rotSpeed * 0.5f)
					{
						timer += dt;
						if(timer >= 0.5f)
						{
							timer = 0.5f;
							animationState = AS_USE_USABLE_USING;
							if(IsLocalPlayer() && IsSet(usable->base->useFlags, BaseUsable::ALCHEMY))
								gameGui->craft->Show();
						}

						// przesuñ postaæ i fizykê
						if(allowMove)
						{
							visualPos = pos = Vec3::Lerp(targetPos, targetPos2, timer * 2);
							changed = true;
							gameLevel->globalCol.clear();
							float myRadius = GetUnitRadius();
							bool ok = true;
							for(vector<Unit*>::iterator it2 = locPart->units.begin(), end2 = locPart->units.end(); it2 != end2; ++it2)
							{
								if(this == *it2 || !(*it2)->IsStanding())
									continue;

								float radius = (*it2)->GetUnitRadius();
								if(Distance((*it2)->pos.x, (*it2)->pos.z, pos.x, pos.z) <= radius + myRadius)
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
		if(animationState == AS_POSITION_HURT)
		{
			// obs³uga animacji cierpienia
			if(meshInst->mesh->head.nGroups == 2)
			{
				if(meshInst->IsEnded(1) || timer >= 0.5f)
				{
					meshInst->Deactivate(1);
					animationState = AS_POSITION_HURT_END;
				}
			}
			else if(meshInst->IsEnded() || timer >= 0.5f)
			{
				animation = ANI_BATTLE;
				animationState = AS_POSITION_HURT_END;
			}
		}
		if(timer >= 0.5f)
		{
			action = A_NONE;
			visualPos = pos = targetPos;

			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::USE_USABLE);
				c.unit = this;
				c.id = usable->id;
				c.count = USE_USABLE_END;
			}

			if(Net::IsLocal())
				UseUsable(nullptr);
		}
		else
			visualPos = pos = Vec3::Lerp(targetPos2, targetPos, timer * 2);
		changed = true;
		break;
	case A_PICKUP:
		if(meshInst->IsEnded())
		{
			action = A_NONE;
			animation = ANI_STAND;
			currentAnimation = (Animation)-1;
		}
		break;
	case A_DASH:
		// update unit dash/bull charge
		{
			float dtLeft = min(dt, timer);
			float t;
			const float eps = 0.05f;
			float len = speed * dtLeft;
			Vec3 dir(sin(act.dash.rot) * (len + eps), 0, cos(act.dash.rot) * (len + eps));
			Vec3 dirNormal = dir.Normalized();
			bool ok = true;
			Vec3 from = GetPhysicsPos();

			if(!IsSet(act.dash.ability->flags, Ability::IgnoreUnits))
			{
				// dash
				gameLevel->LineTest(cobj->getCollisionShape(), from, dir, [&](btCollisionObject* obj, bool)
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
				gameLevel->LineTest(cobj->getCollisionShape(), from, dir, [&](btCollisionObject* obj, bool first)
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
					float moveAngle = Angle(0, 0, dir.x, dir.z);
					Vec3 dirLeft(sin(act.dash.rot + PI / 2) * len, 0, cos(act.dash.rot + PI / 2) * len);
					Vec3 dirRight(sin(act.dash.rot - PI / 2) * len, 0, cos(act.dash.rot - PI / 2) * len);
					for(Unit* unit : targets)
					{
						// deal damage/stun
						bool moveForward = true;
						if(!unit->IsFriend(*this, true))
						{
							if(!act.dash.hit->IsInside(unit))
							{
								float attack = GetAbilityPower(*act.dash.ability);
								float def = unit->CalculateDefense();
								float dmg = CombatHelper::CalculateDamage(attack, def);
								unit->PlayHitSound(MAT_IRON, MAT_SPECIAL_UNIT, unit->GetCenter(), true);
								if(unit->IsPlayer())
									unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, level);
								unit->GiveDmg(dmg, this);
								if(IsPlayer())
									player->Train(TrainWhat::BullsCharge, 0.f, unit->level);
								if(!unit->IsAlive())
									continue;
								else
									act.dash.hit->Add(unit);
							}

							if(act.dash.ability->effect == Ability::Stun)
							{
								Effect e;
								e.effect = EffectId::Stun;
								e.source = EffectSource::Temporary;
								e.sourceId = -1;
								e.value = -1;
								e.power = 0;
								e.time = act.dash.ability->time;
								unit->AddEffect(e);
							}
						}
						else
							moveForward = false;

						auto unitClbk = [unit](btCollisionObject* obj, bool)
						{
							int flags = obj->getCollisionFlags();
							if(IsSet(flags, CG_TERRAIN | CG_UNIT))
								return LT_IGNORE;
							return LT_COLLIDE;
						};

						// try to push forward
						if(moveForward)
						{
							Vec3 moveDir = unit->pos - pos;
							moveDir.y = 0;
							moveDir.Normalize();
							moveDir += dirNormal;
							moveDir.Normalize();
							moveDir *= len;
							float t;
							gameLevel->LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), moveDir, unitClbk, t);
							if(t == 1.f)
							{
								unit->moved = true;
								unit->pos += moveDir;
								unit->Moved(false, true);
								continue;
							}
						}

						// try to push, left or right
						float angle = Angle(pos.x, pos.z, unit->pos.x, unit->pos.z);
						float bestDir = ShortestArc(moveAngle, angle);
						bool innerOk = false;
						for(int i = 0; i < 2; ++i)
						{
							float t;
							Vec3& actualDir = (bestDir < 0 ? dirLeft : dirRight);
							gameLevel->LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), actualDir, unitClbk, t);
							if(t == 1.f)
							{
								innerOk = true;
								unit->moved = true;
								unit->pos += actualDir;
								unit->Moved(false, true);
								break;
							}
						}
						if(!innerOk)
							ok = false;
					}
				}
			}

			// move if there is free space
			float actualLen = (len + eps) * t - eps;
			if(actualLen > 0 && ok)
			{
				pos += Vec3(sin(act.dash.rot), 0, cos(act.dash.rot)) * actualLen;
				Moved(false, true);
			}

			// end dash
			timer -= dt;
			if(timer <= 0 || t < 1.f || !ok)
			{
				if(act.dash.ability->effect == Ability::Stun)
					meshInst->Deactivate(1);
				if(act.dash.hit)
					act.dash.hit->Free();
				action = A_NONE;
				if(Net::IsLocal() || IsLocalPlayer())
					meshInst->groups[0].speed = GetRunSpeed() / data->runSpeed;
			}
		}
		break;
	case A_DESPAWN:
		timer -= dt;
		if(timer <= 0.f)
			gameLevel->RemoveUnit(this);
		break;
	case A_PREPARE:
		assert(Net::IsClient());
		break;
	case A_STAND_UP:
		if(animation == ANI_PLAY)
		{
			if(meshInst->IsEnded())
			{
				action = A_NONE;
				animation = ANI_STAND;
				currentAnimation = (Animation)-1;
			}
		}
		else if(!meshInst->IsBlending())
			action = A_NONE;
		break;
	case A_USE_ITEM:
		if(meshInst->IsEnded(1))
		{
			if(Net::IsLocal() && IsPlayer())
			{
				// magic scroll effect
				gameGui->messages->AddGameMsg3(player, GMS_TOO_COMPLICATED);
				Effect e;
				e.effect = EffectId::Stun;
				e.source = EffectSource::Temporary;
				e.sourceId = -1;
				e.value = -1;
				e.power = 0;
				e.time = 2.5f;
				AddEffect(e);
				RemoveItem(usedItem, 1u);
				usedItem = nullptr;
			}
			soundMgr->PlaySound3d(gameRes->sZap, GetCenter(), MAGIC_SCROLL_SOUND_DIST);
			action = A_NONE;
			animation = ANI_STAND;
			currentAnimation = (Animation)-1;
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
	if(gameLevel->location->outside)
	{
		if(locPart->partType == LocationPart::Type::Outside)
		{
			if(gameLevel->terrain->IsInside(pos))
			{
				gameLevel->terrain->SetY(pos);
				if(warped)
					return;
				if(IsPlayer() && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
				{
					bool allowExit = false;

					if(gameLevel->cityCtx && IsSet(gameLevel->cityCtx->flags, City::HaveExit))
					{
						for(vector<EntryPoint>::const_iterator it = gameLevel->cityCtx->entryPoints.begin(), end = gameLevel->cityCtx->entryPoints.end(); it != end; ++it)
						{
							if(it->exitRegion.IsInside(pos))
							{
								if(!team->IsLeader())
									gameGui->messages->AddGameMsg3(GMS_NOT_LEADER);
								else
								{
									if(Net::IsLocal())
									{
										CanLeaveLocationResult result = gameLevel->CanLeaveLocation(*this);
										if(result == CanLeaveLocationResult::Yes)
										{
											allowExit = true;
											world->SetTravelDir(pos);
										}
										else
											gameGui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										net->OnLeaveLocation(ENTER_FROM_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(gameLevel->locationIndex != questMgr->questSecret->where2
						&& (pos.x < 33.f || pos.x > 256.f - 33.f || pos.z < 33.f || pos.z > 256.f - 33.f))
					{
						if(!team->IsLeader())
							gameGui->messages->AddGameMsg3(GMS_NOT_LEADER);
						else if(Net::IsClient())
							net->OnLeaveLocation(ENTER_FROM_OUTSIDE);
						else
						{
							CanLeaveLocationResult result = gameLevel->CanLeaveLocation(*this);
							if(result == CanLeaveLocationResult::Yes)
							{
								allowExit = true;
								world->SetTravelDir(pos);
							}
							else
								gameGui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
					}

					if(allowExit)
					{
						game->fallbackType = FALLBACK::EXIT;
						game->fallbackTimer = -1.f;
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

			if(IsPlayer() && gameLevel->location->type == L_CITY && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
			{
				for(InsideBuilding* insideBuilding : gameLevel->cityCtx->insideBuildings)
				{
					if(insideBuilding->canEnter && insideBuilding->enterRegion.IsInside(pos))
					{
						if(Net::IsLocal())
						{
							// wejdŸ do budynku
							game->fallbackType = FALLBACK::ENTER;
							game->fallbackTimer = -1.f;
							game->fallbackValue = insideBuilding->partId;
							game->fallbackValue2 = -1;
							frozen = FROZEN::YES;
						}
						else
						{
							// info do serwera
							game->fallbackType = FALLBACK::WAIT_FOR_WARP;
							game->fallbackTimer = -1.f;
							frozen = FROZEN::YES;
							NetChange& c = Net::PushChange(NetChange::ENTER_BUILDING);
							c.id = insideBuilding->partId;
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
			InsideBuilding& building = *static_cast<InsideBuilding*>(locPart);

			if(IsPlayer() && building.exitRegion.IsInside(pos) && player->WantExitLevel() && frozen == FROZEN::NO && !dash)
			{
				if(Net::IsLocal())
				{
					// opuœæ budynek
					game->fallbackType = FALLBACK::ENTER;
					game->fallbackTimer = -1.f;
					game->fallbackValue = -1;
					game->fallbackValue2 = -1;
					frozen = FROZEN::YES;
				}
				else
				{
					// info do serwera
					game->fallbackType = FALLBACK::WAIT_FOR_WARP;
					game->fallbackTimer = -1.f;
					frozen = FROZEN::YES;
					Net::PushChange(NetChange::EXIT_BUILDING);
				}
			}
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)gameLevel->location;
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
		Portal* portal = gameLevel->location->portal;
		int index = 0;
		while(portal)
		{
			if(portal->targetLoc != -1 && Vec3::Distance2d(pos, portal->pos) < 2.f)
			{
				if(CircleToRotatedRectangle(pos.x, pos.z, GetUnitRadius(), portal->pos.x, portal->pos.z, 0.67f, 0.1f, portal->rot))
				{
					if(team->IsLeader())
					{
						if(Net::IsLocal())
						{
							CanLeaveLocationResult result = gameLevel->CanLeaveLocation(*this);
							if(result == CanLeaveLocationResult::Yes)
							{
								game->fallbackType = FALLBACK::USE_PORTAL;
								game->fallbackTimer = -1.f;
								game->fallbackValue = index;
								for(Unit& unit : team->members)
									unit.frozen = FROZEN::YES;
								if(Net::IsOnline())
									Net::PushChange(NetChange::LEAVE_LOCATION);
							}
							else
								gameGui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							net->OnLeaveLocation(ENTER_FROM_PORTAL + index);
					}
					else
						gameGui->messages->AddGameMsg3(GMS_NOT_LEADER);

					break;
				}
			}
			portal = portal->nextPortal;
			++index;
		}
	}

	if(Net::IsLocal() || !IsLocalPlayer() || net->interpolateTimer <= 0.f)
	{
		visualPos = pos;
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
				CanLeaveLocationResult result = gameLevel->CanLeaveLocation(*this);
				if(result == CanLeaveLocationResult::Yes)
				{
					game->fallbackType = FALLBACK::CHANGE_LEVEL;
					game->fallbackTimer = -1.f;
					game->fallbackValue = isPrev ? -1 : +1;
					for(Unit& unit : team->members)
						unit.frozen = FROZEN::YES;
					if(Net::IsOnline())
						Net::PushChange(NetChange::LEAVE_LOCATION);
				}
				else
					gameGui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
			}
			else
				net->OnLeaveLocation(isPrev ? ENTER_FROM_PREV_LEVEL : ENTER_FROM_NEXT_LEVEL);
		}
		else
			gameGui->messages->AddGameMsg3(GMS_NOT_LEADER);
	}
}

//=================================================================================================
void Unit::ChangeBase(UnitData* ud, bool updateItems)
{
	assert(ud);

	if(data == ud)
		return;

	data = ud;
	game->PreloadUnit(this);
	level = ud->level.Clamp(level);
	stats = data->GetStats(level);
	CalculateStats();

	if(updateItems)
	{
		ud->itemScript->Parse(*this);
		for(const Item* item : slots)
		{
			if(item)
				gameRes->PreloadItem(item);
		}
		for(ItemSlot& slot : items)
			gameRes->PreloadItem(slot.item);
		if(IsTeamMember())
			MakeItemsTeam(false);
		UpdateInventory();
	}

	if(IsHero() && IsSet(ud->flags2, F2_MELEE))
		hero->melee = true;

	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::CHANGE_UNIT_BASE);
		c.unit = this;
	}
}

//=================================================================================================
void Unit::MoveToLocation(LocationPart* newLocPart, const Vec3& newPos)
{
	assert(newLocPart && !IsTeamMember());

	if(newLocPart == locPart)
		return;

	const bool isActive = meshInst != nullptr;
	const bool activate = newLocPart->IsActive();
	RemoveElement(locPart->units, this);
	newLocPart->units.push_back(this);
	locPart = newLocPart;
	pos = newPos;
	visualPos = newPos;

	if(isActive != activate)
	{
		if(activate)
		{
			CreateMesh(CREATE_MESH::NORMAL);
			CreatePhysics();

			ai = new AIController;
			ai->Init(this);
			game->ais.push_back(ai);

			if(gameLevel->ready)
			{
				// preload items
				if(data->itemScript)
				{
					for(const Item* item : slots)
					{
						if(item)
							gameRes->PreloadItem(item);
					}
					for(ItemSlot& slot : items)
						gameRes->PreloadItem(slot.item);
				}
				if(data->trader)
				{
					for(ItemSlot& slot : stock->items)
						gameRes->PreloadItem(slot.item);
				}

				if(Net::IsServer())
				{
					NetChange& c = Net::PushChange(NetChange::SPAWN_UNIT);
					c.unit = this;
				}
			}
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
			delete meshInst;
			meshInst = nullptr;
			delete ai;
			ai = nullptr;
			EndEffects();
		}
	}
}

//=================================================================================================
void Unit::MoveOffscreen()
{
	assert(!IsTeamMember());

	if(meshInst != nullptr)
		game->RemoveUnit(this);
	else
		RemoveElement(locPart->units, this);

	OffscreenLocation* offscreen = world->GetOffscreenLocation();
	offscreen->units.push_back(this);
	locPart = offscreen;
}

//=================================================================================================
void Unit::Kill()
{
	assert(!gameLevel->ready); // TODO
	liveState = DEAD;
	if(data->mesh->IsLoaded())
	{
		animation = currentAnimation = ANI_DIE;
		SetAnimationAtEnd(NAMES::aniDie);
		gameLevel->CreateBlood(*gameLevel->lvl, *this, true);
	}
	else
		gameLevel->bloodToSpawn.push_back(this);
	hp = 0.f;
	++gameStats->totalKills;
	UpdatePhysics();
	if(eventHandler)
		eventHandler->HandleUnitEvent(UnitEventHandler::DIE, this);
}

//=================================================================================================
void Unit::GiveDmg(float dmg, Unit* giver, const Vec3* hitpoint, int dmgFlags)
{
	// blood particles
	if(!IsSet(dmgFlags, DMG_NO_BLOOD))
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = gameRes->tBlood[data->blood];
		pe->emissionInterval = 0.f;
		pe->life = 5.f;
		pe->particleLife = 0.5f;
		pe->emissions = 1;
		pe->spawn = Int2(10, 15);
		pe->maxParticles = 15;
		if(hitpoint)
			pe->pos = *hitpoint;
		else
		{
			pe->pos = pos;
			pe->pos.y += GetUnitHeight() * 2.f / 3;
		}
		pe->speedMin = Vec3(-1, 0, -1);
		pe->speedMax = Vec3(1, 1, 1);
		pe->posMin = Vec3(-0.1f, -0.1f, -0.1f);
		pe->posMax = Vec3(0.1f, 0.1f, 0.1f);
		pe->size = Vec2(0.3f, 0.f);
		pe->alpha = Vec2(0.9f, 0.f);
		pe->mode = 0;
		pe->Init();
		locPart->lvlPart->pes.push_back(pe);

		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::SPAWN_BLOOD);
			c.id = data->blood;
			c.pos = pe->pos;
		}
	}

	// apply magic resistance
	if(IsSet(dmgFlags, DMG_MAGICAL))
		dmg *= CalculateMagicResistance();

	if(giver && giver->IsPlayer())
	{
		// update player damage done
		giver->player->dmgDone += (int)dmg;
		giver->player->statFlags |= STAT_DMG_DONE;
	}

	if(IsPlayer())
	{
		// update player damage taken
		player->dmgTaken += (int)dmg;
		player->statFlags |= STAT_DMG_TAKEN;

		// train endurance
		player->Train(TrainWhat::TakeDamage, min(dmg, hp) / hpmax, (giver ? giver->level : -1));

		// red screen
		player->lastDmg += dmg;
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
		if(hurtTimer <= 0.f && data->sounds->Have(SOUND_PAIN))
		{
			game->PlayAttachedSound(*this, data->sounds->Random(SOUND_PAIN), PAIN_SOUND_DIST);
			hurtTimer = Random(1.f, 1.5f);
			if(IsSet(dmgFlags, DMG_NO_BLOOD))
				hurtTimer += 1.f;
			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::UNIT_SOUND);
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
			NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
			c.unit = this;
		}
	}
}

//=================================================================================================
void Unit::AttackReaction(Unit& attacker)
{
	if(attacker.IsPlayer() && IsAI() && inArena == -1 && !attackTeam)
	{
		if(data->group == G_CITIZENS)
		{
			if(!team->isBandit)
			{
				if(dontAttack && IsSet(data->flags, F_PEACEFUL))
				{
					if(ai->state == AIController::Idle)
					{
						ai->state = AIController::Escape;
						ai->timer = Random(2.f, 4.f);
						ai->target = attacker;
						ai->targetLastPos = attacker.pos;
						ai->st.escape.room = nullptr;
						ai->ignore = 0.f;
						ai->inCombat = false;
					}
				}
				else
					team->SetBandit(true);
			}
		}
		else if(data->group == G_CRAZIES)
		{
			if(!team->craziesAttack)
			{
				team->craziesAttack = true;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(dontAttack && !IsSet(data->flags, F_PEACEFUL))
		{
			for(vector<Unit*>::iterator it = gameLevel->localPart->units.begin(), end = gameLevel->localPart->units.end(); it != end; ++it)
			{
				if((*it)->dontAttack && !IsSet((*it)->data->flags, F_PEACEFUL))
				{
					(*it)->dontAttack = false;
					(*it)->ai->changeAiMode = true;
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

	if(!locPart->CheckForHit(*this, hitted, hitpoint))
		return false;

	if(!hitted)
		return true;

	float power;
	if(data->frames->extra)
		power = 1.f;
	else
		power = data->frames->attackPower[act.attack.index];
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

	if(!locPart->CheckForHit(*this, hitted, *mesh->FindPoint("hit"), meshInst->mesh->GetPoint(NAMES::pointShield), hitpoint))
		return false;

	if(!hitted)
		return true;

	if(!IsSet(hitted->data->flags, F_DONT_SUFFER) && hitted->lastBash <= 0.f)
	{
		hitted->lastBash = 1.f + float(hitted->Get(AttributeId::END)) / 50.f;

		hitted->BreakAction();

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animationState = AS_POSITION_HURT;

		if(hitted->meshInst->mesh->head.nGroups == 2)
			hitted->meshInst->Play(NAMES::aniHurt, PLAY_PRIO1 | PLAY_ONCE, 1);
		else
		{
			hitted->meshInst->Play(NAMES::aniHurt, PLAY_PRIO3 | PLAY_ONCE, 0);
			hitted->animation = ANI_PLAY;
		}

		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::STUNNED);
			c.unit = hitted;
		}
	}

	DoGenericAttack(*hitted, hitpoint, CalculateAttack(&GetShield()), DMG_BLUNT, true);

	return true;
}

//=================================================================================================
void Unit::DoGenericAttack(Unit& hitted, const Vec3& hitpoint, float attack, int dmgType, bool bash)
{
	// calculate modifiers
	int mod = CombatHelper::CalculateModifier(dmgType, hitted.data->flags);
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
	float angleDif = AngleDiff(Clip(rot + PI), hitted.rot);
	float backstabMod = GetBackstabMod(bash ? slots[SLOT_SHIELD] : slots[SLOT_WEAPON]);
	if(IsSet(hitted.data->flags2, F2_BACKSTAB_RES))
		backstabMod /= 2;
	m += angleDif / PI * backstabMod;

	// apply modifiers
	attack *= m;

	// blocking
	if(hitted.IsBlocking() && angleDif < PI / 2)
	{
		// play sound
		MATERIAL_TYPE hittedMat = hitted.GetShield().material;
		MATERIAL_TYPE weaponMat = (!bash ? GetWeaponMaterial() : GetShield().material);
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::HIT_SOUND);
			c.pos = hitpoint;
			c.id = weaponMat;
			c.count = hittedMat;
		}
		soundMgr->PlaySound3d(gameRes->GetMaterialSound(weaponMat, hittedMat), hitpoint, HIT_SOUND_DIST);

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
					hitted.animationState = AS_POSITION_HURT;

				if(hitted.meshInst->mesh->head.nGroups == 2)
					hitted.meshInst->Play(NAMES::aniHurt, PLAY_PRIO1 | PLAY_ONCE, 1);
				else
				{
					hitted.meshInst->Play(NAMES::aniHurt, PLAY_PRIO3 | PLAY_ONCE, 0);
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
	hitted.PlayHitSound(!bash ? GetWeaponMaterial() : GetShield().material, MAT_SPECIAL_UNIT, hitpoint, dmg > 0.f);

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
		float poisonRes = hitted.GetPoisonResistance();
		if(poisonRes > 0.f)
		{
			Effect e;
			e.effect = EffectId::Poison;
			e.source = EffectSource::Temporary;
			e.sourceId = -1;
			e.value = -1;
			e.power = dmg / 10 * poisonRes;
			e.time = 5.f;
			hitted.AddEffect(e);
		}
	}
}

//=================================================================================================
void Unit::DoRangedAttack(bool prepare, bool notify, float _speed)
{
	const float speed = _speed > 0.f ? _speed : GetBowAttackSpeed();
	meshInst->Play(NAMES::aniShoot, PLAY_PRIO1 | PLAY_ONCE, 1);
	meshInst->groups[1].speed = speed;
	action = A_SHOOT;
	act.shoot.ability = nullptr;
	animationState = prepare ? AS_SHOOT_PREPARE : AS_SHOOT_CAN;
	if(!bowInstance)
		bowInstance = gameLevel->GetBowInstance(GetBow().mesh);
	bowInstance->Play(&bowInstance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
	bowInstance->groups[0].speed = speed;

	if(!fakeUnit && Net::IsLocal())
	{
		RemoveStamina(Unit::STAMINA_BOW_ATTACK);

		if(Net::IsServer() && notify)
		{
			NetChange& c = Net::PushChange(NetChange::ATTACK);
			c.unit = this;
			c.id = prepare ? AID_StartShoot : AID_Shoot;
			c.f[1] = speed;
		}
	}
}

//=================================================================================================
void Unit::AlertAllies(Unit* target)
{
	for(Unit* u : locPart->units)
	{
		if(u->toRemove || this == u || !u->IsStanding() || u->IsPlayer() || !IsFriend(*u) || u->ai->state == AIController::Fighting
			|| u->ai->alertTarget || u->dontAttack)
			continue;

		if(Vec3::Distance(pos, u->pos) <= ALERT_RANGE && gameLevel->CanSee(*this, *u))
		{
			u->ai->alertTarget = target;
			u->ai->alertTargetPos = target->pos;
		}
	}
}

//=================================================================================================
void Unit::FireEvent(ScriptEvent& e)
{
	if(events.empty())
		return;

	for(Event& event : events)
	{
		if(event.type == e.type)
			event.quest->FireEvent(e);
	}
}

//=================================================================================================
bool Unit::HaveEventHandler(EventType eventType) const
{
	if(!events.empty())
	{
		for(const Event& event : events)
		{
			if(event.type == eventType)
				return true;
		}
	}

	return false;
}
