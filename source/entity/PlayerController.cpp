#include "Pch.h"
#include "PlayerController.h"

#include "Ability.h"
#include "AbilityPanel.h"
#include "AIController.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "BookPanel.h"
#include "Class.h"
#include "Door.h"
#include "FOV.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GroundItem.h"
#include "InsideBuilding.h"
#include "Inventory.h"
#include "Level.h"
#include "LevelGui.h"
#include "Messenger.h"
#include "PhysicCallbacks.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Scripted.h"
#include "Quest_Tutorial.h"
#include "ScriptException.h"
#include "Stock.h"
#include "Team.h"
#include "UnitList.h"
#include "World.h"

#include <ParticleSystem.h>
#include <SoundManager.h>

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
	if(dialogCtx && !dialogCtx->isLocal)
		delete dialogCtx;
}

//=================================================================================================
void PlayerController::Init(Unit& _unit, CreatedCharacter* cc)
{
	// to prevent sending MP message set temporary as fake unit
	_unit.fakeUnit = true;

	unit = &_unit;
	unit->player = this;
	moveTick = 0.f;
	lastWeapon = W_NONE;
	nextAction = NA_NONE;
	lastDmgPoison = lastDmg = dmgc = poisonDmgc = 0.f;
	idleTimer = Random(1.f, 2.f);
	credit = 0;
	onCredit = false;
	action = PlayerAction::None;
	freeDays = 0;
	recalculateLevel = false;
	splitGold = 0.f;

	if(cc)
	{
		godmode = false;
		nocd = false;
		noclip = false;
		invisible = false;
		alwaysRun = true;
		kills = 0;
		dmgDone = 0;
		dmgTaken = 0;
		knocks = 0;
		arenaFights = 0;
		learningPoints = 0;
		exp = 0;
		expLevel = 0;
		expNeed = GetExpNeed();

		// stats
		unit->stats->Set(unit->data->GetStatProfile());
		for(int i = 0; i < (int)SkillId::MAX; ++i)
		{
			if(cc->s[i].add)
				unit->stats->skill[i] += Skill::TAG_BONUS;
			skill[i].points = 0;
			skill[i].train = 0;
			skill[i].trainPart = 0;
			skill[i].apt = unit->stats->skill[i] / 5;
		}
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
		{
			attrib[i].points = 0;
			attrib[i].train = 0;
			attrib[i].trainPart = 0;
			attrib[i].apt = (unit->stats->attrib[i] - 50) / 5;
		}

		// apply perks
		PerkContext ctx(this, true);
		perks = cc->takenPerks;
		for(TakenPerk& tp : perks)
			tp.Apply(ctx);

		// inventory
		unit->data->itemScript->Parse(*unit);
		cc->GetStartingItems(unit->GetEquippedItems());
		if(HavePerk(Perk::Get("alchemistApprentice")))
			Stock::Get("alchemistApprentice")->Parse(unit->items);
		unit->MakeItemsTeam(false);
		unit->RecalculateWeight();
		if(HavePerk(Perk::Get("poor")))
			unit->gold = ::Random(0, 1);
		else
			unit->gold += unit->GetBase(SkillId::PERSUASION);

		unit->CalculateStats();
		unit->CalculateLoad();
		RecalculateLevel();
		unit->hp = unit->hpmax = unit->CalculateMaxHp();
		SetRequiredPoints();
		if(!questMgr->questTutorial->inTutorial)
			AddAbility(unit->GetClass()->ability);
		InitShortcuts();

		// starting known recipes
		int skill = unit->GetBase(SkillId::ALCHEMY);
		if(skill > 0)
		{
			for(Recipe* recipe : Recipe::items)
			{
				if(recipe->autolearn && skill >= recipe->skill)
					AddRecipe(recipe);
			}
		}
	}

	_unit.fakeUnit = false;
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
	if(!abilities.empty())
	{
		shortcuts[2].type = Shortcut::TYPE_ABILITY;
		shortcuts[2].ability = abilities[0].ability;
	}
	shortcuts[3].type = Shortcut::TYPE_SPECIAL;
	shortcuts[3].value = Shortcut::SPECIAL_HEALTH_POTION;
}

//=================================================================================================
void PlayerController::Train(SkillId skill, float points)
{
	assert(Net::IsLocal());
	int s = (int)skill;
	StatData& stat = this->skill[s];
	points += stat.trainPart;
	int intPoints = (int)points;
	stat.trainPart = points - intPoints;
	stat.points += intPoints;

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
		recalculateLevel = true;
		unit->Set(skill, value);
		gameGui->messages->AddFormattedMessage(this, GMS_GAIN_SKILL, s, gained);
		if(!isLocal)
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::STAT_CHANGED);
			c.id = (int)ChangedStatType::SKILL;
			c.a = s;
			c.count = value;
		}
	}
}

//=================================================================================================
void PlayerController::Train(AttributeId attrib, float points)
{
	assert(Net::IsLocal());
	int a = (int)attrib;
	StatData& stat = this->attrib[a];
	points += stat.trainPart;
	int intPoints = (int)points;
	stat.trainPart = points - intPoints;
	stat.points += intPoints;

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
		recalculateLevel = true;
		unit->Set(attrib, value);
		gameGui->messages->AddFormattedMessage(this, GMS_GAIN_ATTRIBUTE, a, gained);
		if(!isLocal)
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::STAT_CHANGED);
			c.id = (int)ChangedStatType::ATTRIBUTE;
			c.a = a;
			c.count = value;
		}
	}
}

//=================================================================================================
void PlayerController::TrainMove(float dist)
{
	moveTick += dist;
	if(moveTick >= 100.f)
	{
		float r = floor(moveTick / 100);
		moveTick -= r * 100;
		Train(TrainWhat::Move, r, 0);
	}
}

//=================================================================================================
void PlayerController::Rest(int days, bool resting, bool travel)
{
	// update effects that work for days, end other
	float naturalMod;
	float prevHp = unit->hp,
		prevMp = unit->mp,
		prevStamina = unit->stamina;
	unit->EndEffects(days, &naturalMod);

	// regenerate hp
	if(Net::IsLocal() && unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(AttributeId::END);
		if(resting)
			heal *= 2;
		heal *= naturalMod * days;
		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;

		Train(TrainWhat::NaturalHealing, heal / unit->hpmax, 0);
	}

	// send update
	if(Net::IsServer() && !travel)
	{
		if(unit->hp != prevHp)
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
			c.unit = unit;
		}

		if(unit->mp != prevMp)
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_MP);
			c.unit = unit;
		}

		if(unit->stamina != prevStamina)
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_STAMINA);
			c.unit = unit;
		}

		if(!isLocal)
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::ON_REST);
			c.count = days;
		}
	}

	// reset last damage
	lastDmg = 0;
	lastDmgPoison = 0;
	dmgc = 0;
	poisonDmgc = 0;

	// reset abilities cooldown
	RefreshCooldown();
}

//=================================================================================================
void PlayerController::Save(GameWriter& f)
{
	if(recalculateLevel)
	{
		RecalculateLevel();
		recalculateLevel = false;
	}

	f << name;
	f << moveTick;
	f << lastDmg;
	f << lastDmgPoison;
	f << dmgc;
	f << poisonDmgc;
	f << idleTimer;
	for(StatData& stat : attrib)
	{
		f << stat.points;
		f << stat.train;
		f << stat.apt;
		f << stat.trainPart;
	}
	for(StatData& stat : skill)
	{
		f << stat.points;
		f << stat.train;
		f << stat.apt;
		f << stat.trainPart;
	}
	f << actionKey;
	NextAction savedNextAction = nextAction;
	if(Any(savedNextAction, NA_SELL, NA_PUT, NA_GIVE))
		savedNextAction = NA_NONE; // inventory is closed, don't save this next action
	f << savedNextAction;
	switch(savedNextAction)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f << nextActionData.slot;
		break;
	case NA_EQUIP:
	case NA_CONSUME:
	case NA_EQUIP_DRAW:
		f << nextActionData.index;
		f << nextActionData.item->id;
		break;
	case NA_USE:
		f << nextActionData.usable->id;
		break;
	default:
		assert(0);
		break;
	}
	f << lastWeapon;
	f << credit;
	f << godmode;
	f << nocd;
	f << noclip;
	f << invisible;
	f << id;
	f << freeDays;
	f << kills;
	f << knocks;
	f << dmgDone;
	f << dmgTaken;
	f << arenaFights;
	f << learningPoints;
	f << (byte)perks.size();
	for(TakenPerk& tp : perks)
	{
		f << tp.perk->hash;
		f << tp.value;
	}
	f << (byte)abilities.size();
	for(PlayerAbility& ab : abilities)
	{
		f << ab.ability->hash;
		f << ab.cooldown;
		f << ab.recharge;
		f << (byte)ab.charges;
	}
	f << recipes.size();
	for(MemorizedRecipe& mr : recipes)
	{
		f << mr.recipe->hash;
	}
	f << splitGold;
	f << alwaysRun;
	f << exp;
	f << expLevel;
	f << lastRing;
	for(Shortcut& shortcut : shortcuts)
	{
		f << shortcut.type;
		switch(shortcut.type)
		{
		case Shortcut::TYPE_SPECIAL:
			f << shortcut.value;
			break;
		case Shortcut::TYPE_ITEM:
			f << shortcut.item->id;
			break;
		case Shortcut::TYPE_ABILITY:
			f << shortcut.ability->hash;
			break;
		}
	}
}

//=================================================================================================
void PlayerController::Load(GameReader& f)
{
	f >> name;
	f >> moveTick;
	f >> lastDmg;
	f >> lastDmgPoison;
	f >> dmgc;
	f >> poisonDmgc;
	f >> idleTimer;
	for(StatData& stat : attrib)
	{
		f >> stat.points;
		f >> stat.train;
		f >> stat.apt;
		f >> stat.trainPart;
		if(LOAD_VERSION < V_0_14)
			f.Skip<bool>(); // old blocked
	}
	for(StatData& stat : skill)
	{
		f >> stat.points;
		f >> stat.train;
		f >> stat.apt;
		f >> stat.trainPart;
	}
	SetRequiredPoints();
	f >> actionKey;
	f >> nextAction;
	switch(nextAction)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f >> nextActionData.slot;
		break;
	case NA_EQUIP:
	case NA_CONSUME:
	case NA_EQUIP_DRAW:
		f >> nextActionData.index;
		nextActionData.item = Item::Get(f.ReadString1());
		break;
	case NA_USE:
		Usable::AddRequest(&nextActionData.usable, f.Read<int>());
		break;
	case NA_SELL:
	case NA_PUT:
	case NA_GIVE:
	default:
		assert(0);
		nextAction = NA_NONE;
		break;
	}
	f >> lastWeapon;
	f >> credit;
	f >> godmode;
	if(LOAD_VERSION >= V_0_18)
		f >> nocd;
	else
		nocd = false;
	f >> noclip;
	f >> invisible;
	f >> id;
	f >> freeDays;
	f >> kills;
	f >> knocks;
	f >> dmgDone;
	f >> dmgTaken;
	f >> arenaFights;

	// perk points
	f >> learningPoints;

	// perks
	byte count;
	f >> count;
	perks.resize(count);
	if(LOAD_VERSION >= V_0_14)
	{
		for(TakenPerk& tp : perks)
		{
			tp.perk = Perk::Get(f.Read<int>());
			f >> tp.value;
		}
	}
	else
	{
		for(TakenPerk& tp : perks)
		{
			old::Perk perk = (old::Perk)f.Read<byte>();
			tp.perk = old::Convert(perk);
			f >> tp.value;
		}
	}

	// abilities
	if(LOAD_VERSION >= V_0_13)
	{
		abilities.resize(f.Read<byte>());
		for(PlayerAbility& ab : abilities)
		{
			ab.ability = Ability::Get(f.Read<int>());
			f >> ab.cooldown;
			f >> ab.recharge;
			f.ReadCasted<byte>(ab.charges);
		}
	}
	else
	{
		PlayerAbility& ab = Add1(abilities);
		ab.ability = unit->GetClass()->ability;
		f >> ab.cooldown;
		f >> ab.recharge;
		f.ReadCasted<byte>(ab.charges);
	}

	if(LOAD_VERSION < V_0_17 && unit->action == A_DASH && unit->act.dash.ability->RequireList())
	{
		unit->act.dash.hit = UnitList::Get();
		unit->act.dash.hit->Load(f);
	}

	// recipes
	if(LOAD_VERSION >= V_0_15)
	{
		uint count;
		f >> count;
		recipes.resize(count);
		for(MemorizedRecipe& mr : recipes)
			mr.recipe = Recipe::Get(f.Read<int>());
	}
	else
	{
		// Learn recipes based on player skill for saves before recipe learning
		int skill = unit->GetBase(SkillId::ALCHEMY);
		if(skill >= 1)
		{
			for(Recipe* recipe : Recipe::items)
			{
				if(recipe->autolearn && skill >= recipe->skill)
					AddRecipe(recipe);
			}
		}
	}

	f >> splitGold;
	f >> alwaysRun;
	f >> exp;
	f >> expLevel;
	expNeed = GetExpNeed();
	f >> lastRing;

	for(Shortcut& shortcut : shortcuts)
	{
		f >> shortcut.type;
		switch(shortcut.type)
		{
		case Shortcut::TYPE_SPECIAL:
			f >> shortcut.value;
			if(shortcut.value == Shortcut::SPECIAL_ABILITY_OLD)
			{
				shortcut.type = Shortcut::TYPE_ABILITY;
				shortcut.ability = abilities[0].ability;
			}
			break;
		case Shortcut::TYPE_ITEM:
			shortcut.item = Item::Get(f.ReadString1());
			break;
		case Shortcut::TYPE_ABILITY:
			shortcut.ability = Ability::Get(f.Read<int>());
			break;
		}
		shortcut.trigger = false;
	}

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
		float eLevel, ePrio = -1;
		switch(entry.type)
		{
		case Class::LevelEntry::T_ATTRIBUTE:
			{
				float value = (float)stats.attrib[entry.value];
				eLevel = (value - 25) / 5 * entry.priority;
				ePrio = entry.priority;
			}
			break;
		case Class::LevelEntry::T_SKILL:
			{
				float value = (float)stats.skill[entry.value];
				eLevel = value / 5 * entry.priority;
				ePrio = entry.priority;
			}
			break;
		case Class::LevelEntry::T_SKILL_SPECIAL:
			if(entry.value == Class::LevelEntry::S_WEAPON)
			{
				SkillId skill = WeaponTypeInfo::info[unit->GetBestWeaponType()].skill;
				float value = (float)stats.skill[(int)skill];
				eLevel = value / 5 * entry.priority;
				ePrio = entry.priority;
			}
			else if(entry.value == Class::LevelEntry::S_ARMOR)
			{
				SkillId skill = GetArmorTypeSkill(unit->GetBestArmorType());
				float value = (float)stats.skill[(int)skill];
				eLevel = value / 5 * entry.priority;
				ePrio = entry.priority;
			}
			break;
		}

		if(ePrio > 0)
		{
			if(entry.required)
			{
				level += eLevel;
				prio += ePrio;
			}
			else
				values.push_back({ eLevel, ePrio });
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
		if(Net::IsLocal() && gameLevel->ready)
		{
			team->CalculatePlayersLevel();
			if(playerInfo && !IsLocal())
				playerInfo->updateFlags |= PlayerInfo::UF_LEVEL;
		}
	}
}

const float levelMod[21] = {
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

inline float GetLevelMod(int myLevel, int targetLevel)
{
	return levelMod[Clamp(targetLevel - myLevel + 10, 0, 20)];
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
			const float cPoints = 50.f;
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();

			int skill = unit->GetBase(info.skill);
			if(skill < 25)
				TrainMod2(info.skill, cPoints);
			else if(skill < 50)
				TrainMod2(info.skill, cPoints / 2);

			skill = unit->GetBase(SkillId::ONE_HANDED_WEAPON);
			if(skill < 25)
				TrainMod2(SkillId::ONE_HANDED_WEAPON, cPoints);
			else if(skill < 50)
				TrainMod2(SkillId::ONE_HANDED_WEAPON, cPoints / 2);

			int str = unit->Get(AttributeId::STR);
			if(weapon.reqStr > str)
				TrainMod(AttributeId::STR, cPoints);
			else if(weapon.reqStr + 10 > str)
				TrainMod(AttributeId::STR, cPoints / 2);

			TrainMod(AttributeId::STR, cPoints * info.str2dmg);
			TrainMod(AttributeId::DEX, cPoints * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackNoDamage:
		{
			const float cPoints = 200.f * GetLevelMod(unit->level, level);
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod2(SkillId::ONE_HANDED_WEAPON, cPoints);
			TrainMod2(info.skill, cPoints);
			TrainMod(AttributeId::STR, cPoints * info.str2dmg);
			TrainMod(AttributeId::DEX, cPoints * info.dex2dmg);
		}
		break;
	case TrainWhat::AttackHit:
		{
			const float cPoints = (200.f + 2300.f * value) * GetLevelMod(unit->level, level);
			const Weapon& weapon = unit->GetWeapon();
			const WeaponTypeInfo& info = weapon.GetInfo();
			TrainMod2(SkillId::ONE_HANDED_WEAPON, cPoints);
			TrainMod2(info.skill, cPoints);
			TrainMod(AttributeId::STR, cPoints * info.str2dmg);
			TrainMod(AttributeId::DEX, cPoints * info.dex2dmg);
		}
		break;
	case TrainWhat::BlockBullet:
	case TrainWhat::BlockAttack:
		TrainMod(SkillId::SHIELD, value * 3000 * GetLevelMod(unit->level, level));
		break;
	case TrainWhat::BashStart:
		{
			int str = unit->Get(AttributeId::STR);
			if(unit->GetShield().reqStr > str)
				TrainMod(AttributeId::STR, 50.f);
			else if(unit->GetShield().reqStr + 10 > str)
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
			if(unit->GetBow().reqStr > str)
				TrainMod(AttributeId::STR, 50.f);
			else if(unit->GetBow().reqStr + 10 > str)
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
				if(armor.reqStr > str)
					TrainMod(AttributeId::STR, 250.f);
				else if(armor.reqStr + 10 > str)
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
		TrainMod(SkillId::PERSUASION, value);
		break;
	case TrainWhat::Persuade:
		if(value == 0 || value == 100)
			TrainMod(SkillId::PERSUASION, 30);
		else
			TrainMod(SkillId::PERSUASION, 30.f * (101 - value));
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
	case TrainWhat::CastCleric:
		TrainMod(SkillId::GODS_MAGIC, value);
		break;
	case TrainWhat::CastMage:
		TrainMod(SkillId::MYSTIC_MAGIC, value);
		break;
	case TrainWhat::Craft:
		TrainMod(SkillId::ALCHEMY, value);
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
void PlayerController::Train(bool isSkill, int id, TrainMode mode)
{
	StatData* stat;
	int value;
	if(isSkill)
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

		if(isSkill)
		{
			stat->next = GetRequiredSkillPoints(value);
			unit->Set((SkillId)id, value);
		}
		else
		{
			stat->next = GetRequiredAttributePoints(value);
			unit->Set((AttributeId)id, value);
		}

		gameGui->messages->AddFormattedMessage(this, isSkill ? GMS_GAIN_SKILL : GMS_GAIN_ATTRIBUTE, id, count);

		if(!IsLocal())
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::STAT_CHANGED);
			c.id = int(isSkill ? ChangedStatType::SKILL : ChangedStatType::ATTRIBUTE);
			c.a = id;
			c.count = value;
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
		if(isSkill)
			TrainMod2((SkillId)id, pts);
		else
			TrainMod((AttributeId)id, pts);
	}
}

//=================================================================================================
void PlayerController::WriteStart(BitStreamWriter& f) const
{
	f << invisible;
	f << nocd;
	f << noclip;
	f << godmode;
	f << alwaysRun;
	for(const Shortcut& shortcut : shortcuts)
	{
		f << shortcut.type;
		switch(shortcut.type)
		{
		case Shortcut::TYPE_SPECIAL:
			f << shortcut.value;
			break;
		case Shortcut::TYPE_ITEM:
			f << shortcut.item->id;
			break;
		case Shortcut::TYPE_ABILITY:
			f << shortcut.ability->hash;
			break;
		}
	}
}

//=================================================================================================
// Used to send per-player data in WritePlayerData
void PlayerController::Write(BitStreamWriter& f) const
{
	f << kills;
	f << dmgDone;
	f << dmgTaken;
	f << knocks;
	f << arenaFights;
	f << learningPoints;

	// perks
	f.WriteCasted<byte>(perks.size());
	for(const TakenPerk& tp : perks)
	{
		f << tp.perk->hash;
		f << tp.value;
	}

	// abilities
	f.WriteCasted<byte>(abilities.size());
	for(const PlayerAbility& ab : abilities)
	{
		f << ab.ability->hash;
		f << ab.cooldown;
		f << ab.recharge;
		f.WriteCasted<byte>(ab.charges);
	}

	// recipes
	f << recipes.size();
	for(const MemorizedRecipe& mr : recipes)
		f << mr.recipe->hash;

	// next action
	f.WriteCasted<byte>(nextAction);
	switch(nextAction)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f.WriteCasted<byte>(nextActionData.slot);
		break;
	case NA_EQUIP:
	case NA_CONSUME:
	case NA_EQUIP_DRAW:
		f << GetNextActionItemIndex();
		break;
	case NA_USE:
		f << nextActionData.usable->id;
		break;
	default:
		assert(0);
		break;
	}

	f << lastRing;
}

//=================================================================================================
void PlayerController::ReadStart(BitStreamReader& f)
{
	f >> invisible;
	f >> nocd;
	f >> noclip;
	f >> godmode;
	f >> alwaysRun;
	for(Shortcut& shortcut : shortcuts)
	{
		f >> shortcut.type;
		switch(shortcut.type)
		{
		case Shortcut::TYPE_SPECIAL:
			f >> shortcut.value;
			break;
		case Shortcut::TYPE_ITEM:
			shortcut.item = Item::Get(f.ReadString1());
			break;
		case Shortcut::TYPE_ABILITY:
			shortcut.ability = Ability::Get(f.Read<int>());
			break;
		}
		shortcut.trigger = false;
	}
}

//=================================================================================================
// Used to read per-player data in ReadPlayerData
bool PlayerController::Read(BitStreamReader& f)
{
	byte count;
	f >> kills;
	f >> dmgDone;
	f >> dmgTaken;
	f >> knocks;
	f >> arenaFights;
	f >> learningPoints;

	// perks
	f >> count;
	if(!f || !f.Ensure(5 * count))
		return false;
	perks.resize(count);
	for(TakenPerk& tp : perks)
	{
		tp.perk = Perk::Get(f.Read<int>());
		f >> tp.value;
	}

	// abilities
	f >> count;
	if(!f || !f.Ensure(11 * count))
		return false;
	abilities.resize(count);
	for(PlayerAbility& ab : abilities)
	{
		ab.ability = Ability::Get(f.Read<int>());
		f >> ab.cooldown;
		f >> ab.recharge;
		f.ReadCasted<byte>(ab.charges);
	}
	gameGui->ability->Refresh();

	// recipes
	uint iCount;
	f >> iCount;
	if(!f || !f.Ensure(sizeof(MemorizedRecipe) * iCount))
		return false;
	recipes.resize(iCount);
	for(MemorizedRecipe& mr : recipes)
		mr.recipe = Recipe::Get(f.Read<int>());

	// next action
	f.ReadCasted<byte>(nextAction);
	if(!f)
		return false;
	switch(nextAction)
	{
	case NA_NONE:
		break;
	case NA_REMOVE:
	case NA_DROP:
		f.ReadCasted<byte>(nextActionData.slot);
		if(!f)
			return false;
		break;
	case NA_EQUIP:
	case NA_CONSUME:
	case NA_EQUIP_DRAW:
		{
			f >> nextActionData.index;
			if(!f)
				return false;
			if(nextActionData.index == -1)
				nextAction = NA_NONE;
			else
				nextActionData.item = unit->items[nextActionData.index].item;
		}
		break;
	case NA_USE:
		{
			int index = f.Read<int>();
			if(!f)
				return false;
			nextActionData.usable = gameLevel->FindUsable(index);
		}
		break;
	default:
		assert(0);
		break;
	}

	f >> lastRing;
	return true;
}

//=================================================================================================
bool PlayerController::IsLeader() const
{
	return team->IsLeader(unit);
}

//=================================================================================================
bool PlayerController::HaveAbility(Ability* ability) const
{
	for(const PlayerAbility& ab : abilities)
	{
		if(ab.ability == ability)
			return true;
	}
	return false;
}

//=================================================================================================
bool PlayerController::AddAbility(Ability* ability)
{
	if(HaveAbility(ability))
		return false;

	PlayerAbility ab;
	ab.ability = ability;
	ab.cooldown = 0;
	ab.recharge = 0;
	ab.charges = ability->charges;
	abilities.push_back(ab);

	if(!unit->fakeUnit)
	{
		if(!IsLocal())
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::ADD_ABILITY);
			c.ability = ability;
		}
		else
			gameGui->ability->Refresh();
	}

	return true;
}

//=================================================================================================
bool PlayerController::RemoveAbility(Ability* ability)
{
	for(auto it = abilities.begin(), end = abilities.end(); it != end; ++it)
	{
		if(it->ability == ability)
		{
			abilities.erase(it);
			if(!unit->fakeUnit)
			{
				if(IsLocal())
				{
					gameGui->ability->Refresh();
					if(data.abilityReady == ability)
						data.abilityReady = nullptr;
					int index = 0;
					for(Shortcut& shortcut : shortcuts)
					{
						if(shortcut.type == Shortcut::TYPE_ABILITY && shortcut.ability == ability)
						{
							shortcut.type = Shortcut::TYPE_NONE;
							if(Net::IsClient())
							{
								NetChange& c = Net::PushChange(NetChange::SET_SHORTCUT);
								c.id = index;
							}
						}
						++index;
					}
				}
				else
				{
					NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::REMOVE_ABILITY);
					c.ability = ability;
				}
			}
			return true;
		}
	}
	return false;
}

//=================================================================================================
PlayerAbility* PlayerController::GetAbility(Ability* ability)
{
	assert(ability);
	for(PlayerAbility& pa : abilities)
	{
		if(pa.ability == ability)
			return &pa;
	}
	return nullptr;
}

//=================================================================================================
const PlayerAbility* PlayerController::GetAbility(Ability* ability) const
{
	assert(ability);
	for(const PlayerAbility& pa : abilities)
	{
		if(pa.ability == ability)
			return &pa;
	}
	return nullptr;
}

//=================================================================================================
PlayerController::CanUseAbilityResult PlayerController::CanUseAbility(Ability* ability, bool prepare) const
{
	assert(ability);
	if(questMgr->questTutorial->inTutorial)
		return CanUseAbilityResult::No;

	const PlayerAbility* ab = GetAbility(ability);
	if(ab && (ab->charges == 0 || ab->cooldown > 0))
		return CanUseAbilityResult::No;

	if(!CanUseAbilityPreview(ability))
		return CanUseAbilityResult::No;

	if(IsSet(ability->flags, Ability::Mage))
	{
		if(!unit->HaveWeapon() || !IsSet(unit->GetWeapon().flags, ITEM_MAGE))
			return CanUseAbilityResult::NeedWand;
		if((unit->weaponTaken != W_ONE_HANDED || unit->weaponState != WeaponState::Taken) && Any(unit->action, A_NONE, A_TAKE_WEAPON))
			return CanUseAbilityResult::TakeWand;
	}

	if(ability->type == Ability::RangedAttack)
	{
		if(!unit->HaveBow())
			return CanUseAbilityResult::NeedBow;
		if((unit->weaponTaken != W_BOW || unit->weaponState != WeaponState::Taken) && Any(unit->action, A_NONE, A_TAKE_WEAPON))
			return CanUseAbilityResult::TakeBow;
	}

	if(unit->action != A_NONE)
	{
		if(prepare)
		{
			if(ability->type == Ability::RangedAttack)
			{
				if(unit->action != A_SHOOT)
					return CanUseAbilityResult::No;
			}
			else if(IsSet(ability->flags, Ability::UseCast))
			{
				if(unit->action != A_CAST)
					return CanUseAbilityResult::No;
			}
			else
			{
				if(!Any(unit->action, A_ATTACK, A_BLOCK, A_BASH))
					return CanUseAbilityResult::No;
			}
		}
		else
			return CanUseAbilityResult::No;
	}

	return CanUseAbilityResult::Yes;
}

//=================================================================================================
bool PlayerController::CanUseAbilityPreview(Ability* ability) const
{
	if(ability->mana > unit->mp || ability->stamina > unit->stamina)
		return false;
	if(ability->type == Ability::Charge)
	{
		if(!unit->CanMove() || unit->IsOverloaded() || unit->GetEffectMax(EffectId::SlowMove) >= 0.5f)
			return false;
	}
	return true;
}

//=================================================================================================
bool PlayerController::CanUseAbilityCheck() const
{
	CanUseAbilityResult result = CanUseAbility(data.abilityReady, true);
	int check; // 1 ok, 0 wait, -1 cancel
	switch(result)
	{
	case CanUseAbilityResult::Yes:
		check = 1;
		break;
	case CanUseAbilityResult::TakeWand:
		if(unit->action == A_TAKE_WEAPON && unit->weaponTaken == W_ONE_HANDED)
			check = 0;
		else
			check = -1;
		break;
	case CanUseAbilityResult::TakeBow:
		if(unit->action == A_TAKE_WEAPON && unit->weaponTaken == W_BOW)
			check = 0;
		else
			check = -1;
		break;
	default:
		check = -1;
		break;
	}

	if(check == -1)
	{
		data.abilityReady = nullptr;
		soundMgr->PlaySound2d(gameRes->sCancel);
	}

	return check == 1;
}

//=================================================================================================
void PlayerController::RefreshCooldown()
{
	for(PlayerAbility& ab : abilities)
	{
		ab.cooldown = 0;
		ab.recharge = 0;
		ab.charges = ab.ability->charges;
	}
}

//=================================================================================================
int PlayerController::GetNextActionItemIndex() const
{
	return FindItemIndex(unit->items, nextActionData.index, nextActionData.item, false);
}

//=================================================================================================
void PlayerController::PayCredit(int count)
{
	rvector<Unit> units;
	for(Unit& u : team->activeMembers)
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

	if(freeDays >= count)
		freeDays -= count;
	else
	{
		count -= freeDays;
		freeDays = 0;

		for(PlayerInfo& info : net->players)
		{
			if(info.left == PlayerInfo::LEFT_NO && info.pc != this)
				info.pc->freeDays += count;
		}

		world->Update(count, UM_NORMAL);
	}

	Net::PushChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
void PlayerController::StartDialog(Unit* talker, GameDialog* dialog, Quest* quest)
{
	assert(talker);

	DialogContext& ctx = *dialogCtx;
	assert(!ctx.dialogMode);

	if(!isLocal)
	{
		NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::START_DIALOG);
		c.id = talker->id;
	}

	ctx.StartDialog(talker, dialog, quest);
}

//=================================================================================================
bool PlayerController::HavePerk(Perk* perk, int value)
{
	assert(perk);
	for(TakenPerk& tp : perks)
	{
		if(tp.perk == perk && tp.value == value)
			return true;
	}
	return false;
}

//=================================================================================================
bool PlayerController::HavePerkS(const string& perkId)
{
	Perk* perk = Perk::Get(perkId);
	if(!perk)
		throw ScriptException("Invalid perk '%s'.", perkId.c_str());
	return HavePerk(perk);
}

//=================================================================================================
bool PlayerController::AddPerk(Perk* perk, int value)
{
	assert(perk);
	if(HavePerk(perk, value))
		return false;
	TakenPerk tp(perk, value);
	perks.push_back(tp);
	PerkContext ctx(this, false);
	tp.Apply(ctx);
	if(Net::IsServer() && !IsLocal())
	{
		NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::ADD_PERK);
		c.id = perk->hash;
		c.count = value;
	}
	return true;
}

//=================================================================================================
bool PlayerController::RemovePerk(Perk* perk, int value)
{
	assert(perk);
	for(auto it = perks.begin(), end = perks.end(); it != end; ++it)
	{
		TakenPerk& tp = *it;
		if(tp.perk == perk && tp.value == value)
		{
			TakenPerk tpCopy = tp;
			perks.erase(it);
			PerkContext ctx(this, false);
			tpCopy.Remove(ctx);
			if(Net::IsServer() && !IsLocal())
			{
				NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::REMOVE_PERK);
				c.id = perk->hash;
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
	learningPoints += count;
	gameGui->messages->AddFormattedMessage(this, GMS_GAIN_LEARNING_POINTS, -1, count);
	if(!isLocal)
		playerInfo->updateFlags |= PlayerInfo::UF_LEARNING_POINTS;
}

//=================================================================================================
void PlayerController::AddExp(int xp)
{
	exp += xp;
	int points = 0;
	while(exp >= expNeed)
	{
		++points;
		++expLevel;
		exp -= expNeed;
		expNeed = GetExpNeed();
	}
	if(points > 0)
		AddLearningPoint(points);
}

//=================================================================================================
int PlayerController::GetExpNeed() const
{
	int expPerLevel = 1000 - (unit->GetBase(AttributeId::INT) - 50);
	return expPerLevel * (expLevel + 1);
}

//=================================================================================================
int PlayerController::GetAptitude(SkillId s)
{
	int apt = skill[(int)s].apt;
	SkillType skillType = Skill::skills[(int)s].type;
	if(skillType != SkillType::NONE)
	{
		int value = unit->GetBase(s);
		// cross training bonus
		if(skillType == SkillType::WEAPON)
		{
			SkillId best = WeaponTypeInfo::info[unit->GetBestWeaponType()].skill;
			if(best != s && value - 1 > unit->GetBase(best))
				apt += 5;
		}
		else if(skillType == SkillType::ARMOR)
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
			NetChange& c = Net::PushChange(NetChange::UNIT_SOUND);
			c.unit = unit;
			c.id = SOUND_SEE_ENEMY;
		}
	}
	unit->Talk(RandomString(game->txYell), 0);

	LocationPart& locPart = *unit->locPart;
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(u2.IsAI() && u2.IsStanding() && !unit->IsEnemy(u2) && !unit->IsFriend(u2) && u2.busy == Unit::Busy_No && u2.frozen == FROZEN::NO && !u2.usable
			&& u2.ai->state == AIController::Idle && !IsSet(u2.data->flags, F_AI_STAY)
			&& Any(u2.ai->st.idle.action, AIController::Idle_None, AIController::Idle_Animation, AIController::Idle_Rot, AIController::Idle_Look))
		{
			u2.ai->st.idle.action = AIController::Idle_MoveAway;
			u2.ai->st.idle.unit = unit;
			u2.ai->timer = Random(3.f, 5.f);
		}
	}
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
		NetChange& c = Net::PushChange(NetChange::SET_SHORTCUT);
		c.id = index;
	}
}

//=================================================================================================
void PlayerController::CheckObjectDistance(const Vec3& pos, void* ptr, float& bestDist, BeforePlayer type)
{
	assert(ptr);

	if(pos.y < unit->pos.y - 0.5f || pos.y > unit->pos.y + 2.f)
		return;

	float dist = Vec3::Distance2d(unit->pos, pos);
	if(dist < PICKUP_RANGE && dist < bestDist)
	{
		float angle = AngleDiff(Clip(unit->rot + PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos)));
		assert(angle >= 0.f);
		if(angle < PI / 4)
		{
			if(type == BP_CHEST)
			{
				Chest* chest = static_cast<Chest*>(ptr);
				if(AngleDiff(Clip(chest->rot - PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos))) > PI / 2)
					return;
			}
			else if(type == BP_USABLE)
			{
				Usable* use = static_cast<Usable*>(ptr);
				auto& bu = *use->base;
				if(IsSet(bu.useFlags, BaseUsable::CONTAINER))
				{
					float allowedDif;
					switch(bu.limitRot)
					{
					default:
					case 0:
						allowedDif = PI * 2;
						break;
					case 1:
						allowedDif = PI / 2;
						break;
					case 2:
						allowedDif = PI / 4;
						break;
					}
					if(AngleDiff(Clip(use->rot - PI / 2), Clip(-Vec3::Angle2d(unit->pos, pos))) > allowedDif)
						return;
				}
			}
			dist += angle;
			if(dist < bestDist && gameLevel->CanSee(*unit->locPart, unit->pos, pos, type == BP_DOOR, ptr))
			{
				bestDist = dist;
				data.beforePlayerPtr.any = ptr;
				data.beforePlayer = type;
			}
		}
	}
}

//=================================================================================================
void PlayerController::UseUsable(Usable& usable, bool afterAction)
{
	Unit& u = *unit;
	BaseUsable& base = *usable.base;

	bool ok = true;
	if(base.item)
	{
		if(!u.HaveItem(base.item) && u.GetEquippedItem(SLOT_WEAPON) != base.item)
		{
			gameGui->messages->AddGameMsg2(Format(game->txNeedItem, base.item->name.c_str()), 2.f);
			ok = false;
		}
		else if(unit->weaponState != WeaponState::Hidden && (base.item != &unit->GetWeapon() || unit->HaveShield()))
		{
			if(afterAction)
				return;
			u.HideWeapon();
			nextAction = NA_USE;
			nextActionData.usable = &usable;
			if(Net::IsClient())
				Net::PushChange(NetChange::SET_NEXT_ACTION);
			ok = false;
		}
		else
			u.usedItem = base.item;
	}

	if(!ok)
		return;

	if(Net::IsLocal())
	{
		u.UseUsable(&usable);
		data.beforePlayer = BP_NONE;

		if(IsSet(base.useFlags, BaseUsable::CONTAINER))
		{
			// loot container
			gameGui->inventory->StartTrade2(I_LOOT_CONTAINER, &usable);
		}
		else
		{
			u.action = A_USE_USABLE;
			u.animationState = AS_USE_USABLE_MOVE_TO_OBJECT;
			u.animation = ANI_PLAY;
			u.meshInst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
			u.targetPos = u.pos;
			u.targetPos2 = usable.pos;
			if(usable.base->limitRot == 4)
				u.targetPos2 -= Vec3(sin(usable.rot) * 1.5f, 0, cos(usable.rot) * 1.5f);
			u.timer = 0.f;
			u.act.useUsable.rot = Vec3::LookAtAngle(u.pos, u.usable->pos);
		}

		if(Net::IsOnline())
		{
			NetChange& c = Net::PushChange(NetChange::USE_USABLE);
			c.unit = &u;
			c.id = u.usable->id;
			c.count = USE_USABLE_START;
		}
	}
	else
	{
		NetChange& c = Net::PushChange(NetChange::USE_USABLE);
		c.id = usable.id;
		c.count = USE_USABLE_START;

		if(IsSet(base.useFlags, BaseUsable::CONTAINER))
		{
			action = PlayerAction::LootContainer;
			actionUsable = &usable;
			chestTrade = &actionUsable->container->items;
		}

		unit->action = A_PREPARE;
	}
}

//=================================================================================================
void PlayerController::UseAbility(Ability* ability, bool fromServer, const Vec3* pos, Unit* target)
{
	if(isLocal && fromServer)
		return;

	if(!fromServer && !nocd)
	{
		PlayerAbility* ab = GetAbility(ability);
		if(ab)
		{
			--ab->charges;
			ab->cooldown = ability->cooldown.x;
			if(ab->recharge == 0)
				ab->recharge = ability->recharge;
		}

		if(ability->mana > 0.f)
			unit->RemoveMana(ability->mana);
		if(ability->stamina > 0.f)
			unit->RemoveStamina(ability->stamina);
	}

	if(IsSet(ability->flags, Ability::UseCast))
	{
		// cast animation
		unit->action = A_CAST;
		unit->animationState = AS_CAST_ANIMATION;
		unit->act.cast.ability = ability;
		unit->meshInst->Play(NAMES::aniCast, PLAY_ONCE | PLAY_PRIO1, 1);
		if(isLocal)
		{
			unit->targetPos = data.abilityPoint;
			unit->act.cast.target = data.abilityTarget;
		}
		else if(Net::IsServer())
		{
			unit->targetPos = *pos;
			unit->act.cast.target = target;
		}
	}
	else if(IsSet(ability->flags, Ability::UseKneel))
	{
		// kneel animation
		unit->action = A_CAST;
		unit->animation = ANI_KNEELS;
		unit->animationState = AS_CAST_KNEEL;
		unit->act.cast.ability = ability;
		unit->timer = ability->castTime;
		if(isLocal)
		{
			unit->targetPos = data.abilityPoint;
			unit->act.cast.target = data.abilityTarget;
		}
		else if(Net::IsServer())
		{
			unit->targetPos = *pos;
			unit->act.cast.target = target;
		}
	}
	else if(ability->type == Ability::RangedAttack)
	{
		float speed = -1.f;
		if(fromServer)
			speed = pos->x;
		actionKey = data.wastedKey;
		data.wastedKey = Key::None;
		unit->DoRangedAttack(true, false, speed);
		unit->act.shoot.ability = ability;
	}
	else if(ability->soundCast)
		game->PlayAttachedSound(*unit, ability->soundCast, ability->soundCastDist);

	if(ability->type == Ability::Charge)
	{
		const bool dash = !IsSet(ability->flags, Ability::IgnoreUnits);
		if(dash && Net::IsLocal())
			Train(TrainWhat::Dash, 0.f, 0);
		unit->action = A_DASH;
		unit->act.dash.ability = ability;
		if(Net::IsLocal() || !fromServer)
			unit->act.dash.rot = Clip(data.abilityRot + unit->rot + PI);
		unit->animation = ANI_RUN;
		unit->currentAnimation = ANI_RUN;
		unit->meshInst->Play(NAMES::aniRun, PLAY_PRIO1 | PLAY_NO_BLEND, 0);

		if(dash)
		{
			unit->timer = 0.33f;
			unit->speed = ability->range / 0.33f;
			unit->meshInst->GetGroup(0).speed = 3.f;
		}
		else
		{
			unit->timer = 0.5f;
			unit->speed = ability->range / 0.5f;
			unit->meshInst->GetGroup(0).speed = 2.5f;
			unit->meshInst->GetGroup(1).blendMax = 0.1f;
			unit->meshInst->Play("charge", PLAY_PRIO1, 1);
		}

		if(ability->RequireList())
		{
			unit->act.dash.hit = UnitList::Get();
			unit->act.dash.hit->Clear();
		}
		else
			unit->act.dash.hit = nullptr;
	}

	if(Net::IsOnline())
	{
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::PLAYER_ABILITY);
			c.unit = unit;
			c.ability = ability;
			c.extraFloat = unit->meshInst->GetGroup(1).speed;
		}
		else if(!fromServer)
		{
			NetChange& c = Net::PushChange(NetChange::PLAYER_ABILITY);
			c.unit = data.abilityTarget;
			c.pos = data.abilityPoint;
			c.ability = ability;
		}
	}

	if(isLocal)
		data.abilityReady = nullptr;
}

//=================================================================================================
bool PlayerController::IsAbilityPrepared() const
{
	if(data.abilityReady)
		return true;
	if(unit->action == A_SHOOT)
		return unit->act.shoot.ability != nullptr;
	else if(unit->action == A_CAST)
		return IsClear(unit->act.cast.ability->flags, Ability::DefaultAttack);
	return false;
}

//=================================================================================================
bool PlayerController::AddRecipe(Recipe* recipe)
{
	if(HaveRecipe(recipe))
		return false;
	MemorizedRecipe mr;
	mr.recipe = recipe;
	recipes.push_back(mr);

	if(!unit->fakeUnit)
	{
		if(IsLocal())
			messenger->Post(Msg::LearnRecipe);
		else
		{
			NetChangePlayer& c = playerInfo->PushChange(NetChangePlayer::ADD_RECIPE);
			c.recipe = recipe;
		}
	}

	return true;
}

bool PlayerController::HaveRecipe(Recipe* recipe) const
{
	assert(recipe);
	for(const MemorizedRecipe& mr : recipes)
	{
		if(mr.recipe == recipe)
			return true;
	}
	return false;
}

//=================================================================================================
void PlayerController::Update(float dt)
{
	lastDmg = 0.f;
	if(Net::IsLocal())
		lastDmgPoison = 0.f;

	if(unit->IsAlive())
		data.grayout = max(data.grayout - dt, 0.f);
	else
		data.grayout = min(data.grayout + dt, 1.f);

	if(data.wastedKey != Key::None && input->Up(data.wastedKey))
		data.wastedKey = Key::None;

	Unit* lookTarget = unit->lookTarget;
	if(game->dialogContext.dialogMode || lookTarget || gameGui->inventory->mode > I_INVENTORY)
	{
		Vec3 pos;
		if(lookTarget)
		{
			pos = lookTarget->pos;
			unit->animation = ANI_STAND;
		}
		else if(gameGui->inventory->mode == I_LOOT_CHEST)
		{
			assert(action == PlayerAction::LootChest);
			pos = actionChest->pos;
			unit->animation = ANI_KNEELS;
		}
		else if(gameGui->inventory->mode == I_LOOT_BODY)
		{
			assert(action == PlayerAction::LootUnit);
			pos = actionUnit->GetLootCenter();
			unit->animation = ANI_KNEELS;
		}
		else if(gameGui->inventory->mode == I_LOOT_CONTAINER)
		{
			assert(action == PlayerAction::LootContainer);
			pos = actionUsable->pos;
			unit->animation = ANI_STAND;
		}
		else if(game->dialogContext.dialogMode)
		{
			pos = game->dialogContext.talker->pos;
			unit->animation = ANI_STAND;
		}
		else
		{
			assert(action == InventoryModeToActionRequired(gameGui->inventory->mode));
			pos = actionUnit->pos;
			unit->animation = ANI_STAND;
		}

		unit->RotateTo(pos, dt);
		if(gameGui->inventory->mode > I_INVENTORY)
		{
			if(gameGui->inventory->mode == I_LOOT_BODY)
			{
				if(actionUnit->IsAlive())
				{
					// looted corpse was resurected
					game->CloseInventory();
				}
			}
			else if(gameGui->inventory->mode == I_TRADE || gameGui->inventory->mode == I_SHARE || gameGui->inventory->mode == I_GIVE)
			{
				if(!actionUnit->IsStanding() || !actionUnit->IsIdle())
				{
					// trader was attacked/died
					game->CloseInventory();
				}
			}
		}
		else if(game->dialogContext.dialogMode && Net::IsLocal())
		{
			if(!game->dialogContext.talker->IsStanding() || !game->dialogContext.talker->IsIdle() || game->dialogContext.talker->toRemove
				|| game->dialogContext.talker->frozen != FROZEN::NO)
			{
				// talker waas removed/died/was attacked
				game->dialogContext.EndDialog();
			}
		}

		data.beforePlayer = BP_NONE;
		data.rotBuf = 0.f;
		data.autowalk = false;
		data.abilityReady = nullptr;
	}
	else if(!IsBlocking(unit->action) && !unit->HaveEffect(EffectId::Stun))
	{
		bool allowRot = true;
		if(unit->action == A_CAST && unit->act.cast.ability->type == Ability::Target && unit->act.cast.target != unit)
		{
			Vec3 pos;
			if(Unit* target = unit->act.cast.target)
				pos = target->pos;
			else
				pos = unit->targetPos;
			unit->RotateTo(pos, dt);
			allowRot = false;
		}
		UpdateMove(dt, allowRot);
	}
	else
	{
		data.beforePlayer = BP_NONE;
		data.rotBuf = 0.f;
		data.autowalk = false;
		data.abilityReady = nullptr;
	}

	UpdateCooldown(dt);
	ClearShortcuts();

	// last damage
	dmgc += lastDmg;
	dmgc *= (1.f - dt * 2);
	if(lastDmg == 0.f && dmgc < 0.1f)
		dmgc = 0.f;

	// poison damage
	poisonDmgc += (lastDmgPoison - poisonDmgc) * dt;
	if(lastDmgPoison == 0.f && poisonDmgc < 0.1f)
		poisonDmgc = 0.f;

	// building underground
	if(buildingUndergroundState)
	{
		if(buildingUndergroundValue < 1)
		{
			buildingUndergroundValue = min(buildingUndergroundValue + dt * 3, 1.f);
			static_cast<InsideBuilding*>(unit->locPart)->SetUndergroundValue(buildingUndergroundValue);
			game->SetMusic(MusicType::Dungeon);
		}
	}
	else
	{
		if(buildingUndergroundValue > 0)
		{
			buildingUndergroundValue = max(buildingUndergroundValue - dt * 3, 0.f);
			static_cast<InsideBuilding*>(unit->locPart)->SetUndergroundValue(buildingUndergroundValue);
			game->SetMusic(MusicType::City);
		}
	}

	if(recalculateLevel)
	{
		recalculateLevel = false;
		RecalculateLevel();
	}
}

//=================================================================================================
void PlayerController::UpdateMove(float dt, bool allowRot)
{
	Unit& u = *unit;
	LocationPart& locPart = *u.locPart;

	// unit lying on ground
	if(!u.IsStanding())
	{
		data.autowalk = false;
		data.abilityReady = nullptr;
		data.rotBuf = 0.f;
		if(Net::IsLocal())
			u.TryStandup(dt);
		return;
	}

	// unit is frozen
	if(u.frozen >= FROZEN::YES)
	{
		data.autowalk = false;
		data.abilityReady = nullptr;
		data.rotBuf = 0.f;
		if(u.frozen == FROZEN::YES)
			u.animation = ANI_STAND;
		return;
	}

	// casting - can't move
	if(u.action == A_CAST && u.animationState == AS_CAST_KNEEL)
		return;

	// using usable
	if(u.usable)
	{
		if(u.action == A_USE_USABLE && Any(u.animationState, AS_USE_USABLE_USING, AS_USE_USABLE_USING_SOUND))
		{
			if(GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE) || GKey.KeyPressedReleaseAllowed(GK_USE))
				u.StopUsingUsable();
		}
		data.rotBuf = 0.f;
		data.abilityReady = nullptr;
	}

	u.prevPos = u.pos;
	u.changed = true;

	bool idle = true;
	int move = 0;

	if(!u.usable)
	{
		if(u.weaponTaken == W_NONE)
		{
			if(u.animation != ANI_IDLE)
				u.animation = ANI_STAND;
		}
		else if(u.weaponTaken == W_ONE_HANDED)
			u.animation = ANI_BATTLE;
		else if(u.weaponTaken == W_BOW)
			u.animation = ANI_BATTLE_BOW;

		if(GKey.KeyPressedReleaseAllowed(GK_TOGGLE_WALK))
		{
			alwaysRun = !alwaysRun;
			if(Net::IsClient())
			{
				NetChange& c = Net::PushChange(NetChange::CHANGE_ALWAYS_RUN);
				c.id = (alwaysRun ? 1 : 0);
			}
		}

		int rotate = 0;
		if(allowRot)
		{
			if(GKey.KeyDownAllowed(GK_ROTATE_LEFT))
				--rotate;
			if(GKey.KeyDownAllowed(GK_ROTATE_RIGHT))
				++rotate;
		}

		if(u.frozen == FROZEN::NO)
		{
			bool cancelAutowalk = (GKey.KeyPressedReleaseAllowed(GK_MOVE_FORWARD) || GKey.KeyDownAllowed(GK_MOVE_BACK));
			if(cancelAutowalk)
				data.autowalk = false;
			else if(GKey.KeyPressedReleaseAllowed(GK_AUTOWALK))
				data.autowalk = !data.autowalk;

			if(u.action == A_ATTACK && u.act.attack.run)
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

		if((GKey.allowInput == GameKeys::ALLOW_INPUT || GKey.allowInput == GameKeys::ALLOW_MOUSE) && !gameLevel->camera.freeRot && allowRot)
		{
			int div = (u.action == A_SHOOT ? 800 : 400);
			data.rotBuf *= (1.f - dt * 2);
			data.rotBuf += float(input->GetMouseDif().x) * game->settings.GetMouseSensitivity() / div;
			if(data.rotBuf > 0.1f)
				data.rotBuf = 0.1f;
			else if(data.rotBuf < -0.1f)
				data.rotBuf = -0.1f;
		}
		else
			data.rotBuf = 0.f;

		const bool rotating = (rotate != 0 || data.rotBuf != 0.f);
		if(move != 0 && !u.CanMove())
			move = 0;

		u.running = false;
		if(rotating || move)
		{
			// rotating by mouse don't affect idle timer
			if(move || rotate)
				idle = false;

			if(rotating)
			{
				const float rotSpeedDt = u.GetRotationSpeed() * dt;
				const float mouseRot = Clamp(data.rotBuf, -rotSpeedDt, rotSpeedDt);
				const float val = rotSpeedDt * rotate + mouseRot;

				data.rotBuf -= mouseRot;
				u.rot = Clip(u.rot + Clamp(val, -rotSpeedDt, rotSpeedDt));

				if(val > 0)
					u.animation = ANI_RIGHT;
				else if(val < 0)
					u.animation = ANI_LEFT;
			}

			if(move)
			{
				// set angle & speed of move
				float angle = u.rot;
				bool run = alwaysRun;
				if(u.action == A_ATTACK && u.act.attack.run)
					run = true;
				else
				{
					if(GKey.KeyDownAllowed(GK_WALK))
						run = !run;
					if(!u.CanRun())
						run = false;
				}

				float speedMod;
				switch(move)
				{
				case 10: // forward
					angle += PI;
					speedMod = 1.f;
					break;
				case -10: // backward
					run = false;
					speedMod = 0.8f;
					break;
				case -1: // left
					angle += PI / 2;
					speedMod = 0.75f;
					break;
				case 1: // right
					angle += PI * 3 / 2;
					speedMod = 0.75f;
					break;
				case 9: // forward left
					angle += PI * 3 / 4;
					speedMod = 0.9f;
					break;
				case 11: // forward right
					angle += PI * 5 / 4;
					speedMod = 0.9f;
					break;
				case -11: // backward left
					run = false;
					angle += PI / 4;
					speedMod = 0.9f;
					break;
				case -9: // backward right
					run = false;
					angle += PI * 7 / 4;
					speedMod = 0.9f;
					break;
				}

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

				if(u.action == A_SHOOT || u.action == A_CAST)
					speedMod /= 2;
				u.speed = (run ? u.GetRunSpeed() : u.GetWalkSpeed()) * speedMod;
				u.prevSpeed = Clamp((u.prevSpeed + (u.speed - u.prevSpeed) * dt * 3), 0.f, u.speed);
				float speed = u.prevSpeed * dt;

				u.prevPos = u.pos;

				const Vec3 dir(sin(angle) * speed, 0, cos(angle) * speed);
				Int2 prevTile(int(u.pos.x / 2), int(u.pos.z / 2));
				bool moved = false;

				if(noclip)
				{
					u.pos += dir;
					moved = true;
				}
				else if(gameLevel->CheckMove(u.pos, dir, u.GetRadius(), &u))
					moved = true;

				if(moved)
				{
					u.Moved();

					// train by moving
					if(Net::IsLocal())
						TrainMove(speed);

					if(!gameLevel->location->outside)
					{
						// revealing minimap
						Int2 newTile(int(u.pos.x / 2), int(u.pos.z / 2));
						if(newTile != prevTile)
							FOV::DungeonReveal(newTile, gameLevel->minimapReveal);
					}
					else
						CheckBuildingUnderground(false);
				}

				if(run && abs(u.speed - u.prevSpeed) < 0.25f && move >= 9)
					u.running = true;
			}
		}

		if(move == 0)
		{
			u.prevSpeed -= dt * 3;
			if(u.prevSpeed < 0)
				u.prevSpeed = 0;
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
						bool ignoreTeam = !team->HaveOtherActiveTeamMember();
						int iIndex = u.FindItem([=](const ItemSlot& slot)
						{
							return slot.item == item && (slot.teamCount == 0u || ignoreTeam);
						});
						if(iIndex != Unit::INVALID_IINDEX)
						{
							ITEM_SLOT slotType = ItemTypeToSlot(item->type);
							bool ok = true;
							if(u.SlotRequireHideWeapon(slotType))
							{
								u.HideWeapon();
								if(u.action == A_TAKE_WEAPON)
								{
									ok = false;
									nextAction = NA_EQUIP_DRAW;
									nextActionData.item = item;
									nextActionData.index = iIndex;
									if(Net::IsClient())
										Net::PushChange(NetChange::SET_NEXT_ACTION);
								}
							}

							if(ok)
							{
								gameGui->inventory->invMine->EquipSlotItem(iIndex);
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
					int iIndex = u.FindItem(item);
					if(iIndex != Unit::INVALID_IINDEX)
						gameGui->inventory->invMine->ConsumeItem(iIndex);
				}
				else if(item->type == IT_BOOK)
				{
					int iIndex = u.FindItem(item);
					if(iIndex != Unit::INVALID_IINDEX)
					{
						if(IsSet(item->flags, ITEM_MAGIC_SCROLL))
						{
							if(!u.usable) // can't use when sitting
								ReadBook(iIndex);
						}
						else
						{
							OpenPanel open = gameGui->levelGui->GetOpenPanel();
							if(open != OpenPanel::Inventory)
								gameGui->levelGui->ClosePanels();
							gameGui->book->Show((const Book*)item);
						}
					}
				}
			}
			break;
		}
	}

	if(u.action == A_NONE || u.action == A_TAKE_WEAPON || u.CanDoWhileUsing() || (u.action == A_ATTACK && u.animationState == AS_ATTACK_PREPARE)
		|| (u.action == A_SHOOT && u.animationState == AS_SHOOT_PREPARE))
	{
		if(GKey.KeyPressedReleaseAllowed(GK_TAKE_WEAPON))
		{
			if(Any(u.weaponState, WeaponState::Taken, WeaponState::Taking))
			{
				u.HideWeapon();
				idle = false;
			}
			else
			{
				// decide which weapon to take out
				WeaponType weapon = lastWeapon;
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
					gameGui->messages->AddGameMsg3(GMS_NEED_WEAPON);
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

		if(shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_HEALTH_POTION && u.hp < u.hpmax)
		{
			// drink healing potion
			int potionIndex = u.FindHealingPotion();
			if(potionIndex != -1)
			{
				idle = false;
				u.ConsumeItem(potionIndex);
			}
			else
				gameGui->messages->AddGameMsg3(GMS_NO_HEALTH_POTION);
		}
		else if(shortcut.type == Shortcut::TYPE_SPECIAL && shortcut.value == Shortcut::SPECIAL_MANA_POTION && u.mp < u.mpmax)
		{
			// drink mana potion
			int potionIndex = u.FindManaPotion();
			if(potionIndex != -1)
			{
				idle = false;
				u.ConsumeItem(potionIndex);
			}
			else
				gameGui->messages->AddGameMsg3(GMS_NO_MANA_POTION);
		}
	} // allowInput == ALLOW_INPUT || allowInput == ALLOW_KEYBOARD

	if(u.usable)
		return;

	// check what is in front of player
	data.beforePlayer = BP_NONE;
	float bestDist = 3.0f;

	// doors in front of player
	for(Door* door : locPart.doors)
	{
		if(Any(door->state, Door::Opened, Door::Closed))
			CheckObjectDistance(door->pos, door, bestDist, BP_DOOR);
	}

	if(!data.abilityReady && u.action != A_CAST)
	{
		// units in front of player
		for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
		{
			Unit& u2 = **it;
			if(&u == &u2 || u2.toRemove)
				continue;

			if(u2.IsStanding())
				CheckObjectDistance(u2.visualPos, &u2, bestDist, BP_UNIT);
			else if(u2.liveState == Unit::FALL || u2.liveState == Unit::DEAD)
				CheckObjectDistance(u2.GetLootCenter(), &u2, bestDist, BP_UNIT);
		}

		// chests in front of player
		for(Chest* chest : locPart.chests)
		{
			if(!chest->GetUser())
				CheckObjectDistance(chest->pos, chest, bestDist, BP_CHEST);
		}

		// ground items in front of player
		for(GroundItem* groundItem : locPart.GetGroundItems())
			CheckObjectDistance(groundItem->pos, groundItem, bestDist, BP_ITEM);

		// usable objects in front of player
		for(Usable* usable : locPart.usables)
		{
			if(!usable->user)
				CheckObjectDistance(usable->pos, usable, bestDist, BP_USABLE);
		}
	}

	// use something in front of player
	if(u.frozen == FROZEN::NO && data.beforePlayer != BP_NONE
		&& (GKey.KeyPressedReleaseAllowed(GK_USE) || (u.IsNotFighting() && GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE))))
	{
		idle = false;
		if(data.beforePlayer == BP_UNIT)
		{
			Unit* u2 = data.beforePlayerPtr.unit;

			// there is unit in front of player
			if(u2->liveState == Unit::DEAD || u2->liveState == Unit::FALL)
			{
				if(u.action != A_NONE)
				{
				}
				else if(u2->liveState == Unit::FALL)
				{
					// can't loot alive units that are just lying on ground
					gameGui->messages->AddGameMsg3(GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// can't loot allies
					gameGui->messages->AddGameMsg3(GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->inArena != -1)
					gameGui->messages->AddGameMsg3(GMS_DONT_LOOT_ARENA);
				else if(Net::IsLocal())
				{
					if(Net::IsOnline() && u2->busy == Unit::Busy_Looted)
					{
						// someone else is looting
						gameGui->messages->AddGameMsg3(GMS_IS_LOOTED);
					}
					else
					{
						// start looting corpse
						action = PlayerAction::LootUnit;
						actionUnit = u2;
						u2->busy = Unit::Busy_Looted;
						chestTrade = &u2->items;
						gameGui->inventory->StartTrade(I_LOOT_BODY, *u2);
					}
				}
				else
				{
					// send message to server about looting
					NetChange& c = Net::PushChange(NetChange::LOOT_UNIT);
					c.id = u2->id;
					action = PlayerAction::LootUnit;
					actionUnit = u2;
					chestTrade = &u2->items;
				}
			}
			else if(u2->IsAI() && u2->IsIdle() && u2->inArena == -1 && u2->data->dialog && !u.IsEnemy(*u2))
			{
				if(Net::IsLocal())
				{
					if(u2->busy != Unit::Busy_No || !u2->CanTalk(u))
					{
						// using is busy
						gameGui->messages->AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// start dialog
						game->dialogContext.StartDialog(u2);
						data.beforePlayer = BP_NONE;
					}
				}
				else
				{
					// send message to server about starting dialog
					NetChange& c = Net::PushChange(NetChange::TALK);
					c.id = u2->id;
					action = PlayerAction::Talk;
					actionUnit = u2;
					game->dialogContext.predialog.clear();
					game->dialogContext.predialogBubble = u2->bubble;
				}
			}
		}
		else if(data.beforePlayer == BP_CHEST)
		{
			if(u.action != A_NONE)
			{
			}
			else if(Net::IsLocal())
			{
				// start looting chest
				gameGui->inventory->StartTrade2(I_LOOT_CHEST, data.beforePlayerPtr.chest);
				data.beforePlayerPtr.chest->OpenClose(unit);
			}
			else
			{
				// send message to server about looting chest
				NetChange& c = Net::PushChange(NetChange::USE_CHEST);
				c.id = data.beforePlayerPtr.chest->id;
				action = PlayerAction::LootChest;
				actionChest = data.beforePlayerPtr.chest;
				chestTrade = &actionChest->items;
				u.action = A_PREPARE;
			}
		}
		else if(data.beforePlayer == BP_DOOR)
		{
			// opening/closing door
			Door* door = data.beforePlayerPtr.door;
			if(door->state == Door::Closed)
			{
				// open door
				if(door->locked == LOCK_NONE)
					door->Open();
				else
				{
					cstring key;
					switch(door->locked)
					{
					case LOCK_MINE:
						key = "qMineKey";
						break;
					case LOCK_ORCS:
						key = "qOrcsKey";
						break;
					case LOCK_UNLOCKABLE:
					default:
						key = nullptr;
						break;
					}

					Vec3 center = door->GetCenter();
					if(key && u.HaveItem(Item::Get(key)))
					{
						soundMgr->PlaySound3d(gameRes->sUnlock, center, Door::UNLOCK_SOUND_DIST);
						gameGui->messages->AddGameMsg3(GMS_UNLOCK_DOOR);
						door->Open();
					}
					else
					{
						gameGui->messages->AddGameMsg3(GMS_NEED_KEY);
						soundMgr->PlaySound3d(gameRes->sDoorClosed[Rand() % 2], center, Door::SOUND_DIST);
					}
				}
			}
			else
				door->Close();
		}
		else if(data.beforePlayer == BP_ITEM)
		{
			// pickup item
			GroundItem* groundItem = data.beforePlayerPtr.item;
			if(u.action == A_NONE)
			{
				bool upAnim = (groundItem->pos.y > u.pos.y + 0.5f);

				u.action = A_PICKUP;
				u.animation = ANI_PLAY;
				u.meshInst->Play(upAnim ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);

				if(Net::IsLocal())
				{
					u.AddItem2(groundItem->item, groundItem->count, groundItem->teamCount, false);

					if(groundItem->item->type == IT_GOLD)
						soundMgr->PlaySound2d(gameRes->sCoins);

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::PICKUP_ITEM);
						c.unit = unit;
						c.count = (upAnim ? 1 : 0);
					}

					locPart.RemoveGroundItem(groundItem, false);

					ScriptEvent event(EVENT_PICKUP);
					event.onPickup.unit = unit;
					event.onPickup.groundItem = groundItem;
					event.onPickup.item = groundItem->item;
					gameLevel->location->FireEvent(event);

					delete groundItem;
				}
				else
				{
					NetChange& c = Net::PushChange(NetChange::PICKUP_ITEM);
					c.id = groundItem->id;
				}
			}
		}
		else if(data.beforePlayer == BP_USABLE)
		{
			if(u.action == A_NONE && !IsSet(data.beforePlayerPtr.usable->base->useFlags, BaseUsable::DESTROYABLE))
				UseUsable(*data.beforePlayerPtr.usable, false);
		}
	}

	// attack
	if(u.weaponState == WeaponState::Taken)
	{
		idle = false;
		if(u.weaponTaken == W_ONE_HANDED)
		{
			if(u.action == A_ATTACK)
			{
				if(u.animationState == AS_ATTACK_PREPARE)
				{
					if(GKey.KeyUpAllowed(actionKey))
					{
						// release attack
						const float ratio = u.meshInst->GetGroup(1).time / u.GetAttackFrame(0);
						const float speed = (ratio + u.GetAttackSpeed()) * u.GetStaminaAttackSpeedMod();
						u.meshInst->GetGroup(1).speed = speed;
						u.animationState = AS_ATTACK_CAN_HIT;
						u.act.attack.power = ratio + 1.f;

						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_Attack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
							Train(TrainWhat::AttackStart, 0.f, 0);
					}
					else if(GKey.KeyPressedUpAllowed(GK_CANCEL_ATTACK))
					{
						u.meshInst->Deactivate(1);
						u.action = A_NONE;
						data.wastedKey = actionKey;

						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_Cancel;
							c.f[1] = 1.f;
						}
					}
				}
				else if(u.animationState == AS_ATTACK_FINISHED && !data.abilityReady)
				{
					Key k = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::Down);
					if(k == Key::None)
						k = GKey.KeyDoReturn(GK_SECONDARY_ATTACK, &Input::Down);
					if(k != Key::None)
					{
						// prepare next attack
						float speed = u.GetPowerAttackSpeed() * u.GetStaminaAttackSpeedMod();
						u.action = A_ATTACK;
						u.animationState = AS_ATTACK_PREPARE;
						u.act.attack.index = u.GetRandomAttack();
						u.act.attack.run = false;
						u.act.attack.hitted = 0;
						u.meshInst->Play(NAMES::aniAttacks[u.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
						u.meshInst->GetGroup(1).speed = speed;
						actionKey = k;
						u.timer = 0.f;

						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_PrepareAttack;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
						{
							const Weapon& weapon = u.GetWeapon();
							u.RemoveStamina(weapon.GetInfo().stamina * u.GetStaminaMod(weapon));
						}
					}
				}
			}
			else if(u.action == A_BLOCK)
			{
				if(GKey.KeyUpAllowed(actionKey))
				{
					// stop blocking
					u.action = A_NONE;
					u.meshInst->Deactivate(1);

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::ATTACK);
						c.unit = unit;
						c.id = AID_Cancel;
						c.f[1] = 1.f;
					}
				}
				else if(!u.meshInst->GetGroup(1).IsBlending() && u.HaveShield() && !data.abilityReady)
				{
					if(GKey.KeyPressedUpAllowed(GK_ATTACK_USE) || GKey.KeyPressedUpAllowed(GK_SECONDARY_ATTACK))
					{
						// shield bash
						const float speed = u.GetBashSpeed();
						u.action = A_BASH;
						u.animationState = AS_BASH_ANIMATION;
						u.meshInst->Play(NAMES::aniBash, PLAY_ONCE | PLAY_PRIO1, 1);
						u.meshInst->GetGroup(1).speed = speed;

						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_Bash;
							c.f[1] = speed;
						}

						if(Net::IsLocal())
						{
							Train(TrainWhat::BashStart, 0.f, 0);
							u.RemoveStamina(Unit::STAMINA_BASH_ATTACK * u.GetStaminaMod(u.GetShield()));
						}
					}
				}
			}
			else if(u.action == A_NONE && u.frozen == FROZEN::NO && !GKey.KeyDownAllowed(GK_BLOCK) && !data.abilityReady)
			{
				Key k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &Input::Down, data.wastedKey);
				bool secondary = false;
				if(k == Key::None)
				{
					k = GKey.KeyDoReturnIgnore(GK_SECONDARY_ATTACK, &Input::Down, data.wastedKey);
					secondary = true;
				}
				if(k != Key::None)
				{
					if(!secondary && IsSet(u.GetWeapon().flags, ITEM_WAND) && u.Get(SkillId::MYSTIC_MAGIC) > 0)
					{
						// cast magic bolt
						UseAbility(Ability::Get("magicBolt"), false);
					}
					else
					{
						u.action = A_ATTACK;
						u.act.attack.index = u.GetRandomAttack();
						u.act.attack.hitted = 0;
						u.meshInst->Play(NAMES::aniAttacks[u.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
						if(u.running)
						{
							// running attack
							float speed = u.GetAttackSpeed() * u.GetStaminaAttackSpeedMod();
							u.meshInst->GetGroup(1).speed = speed;
							u.animationState = AS_ATTACK_CAN_HIT;
							u.act.attack.run = true;
							u.act.attack.power = 1.5f;

							if(Net::IsOnline())
							{
								NetChange& c = Net::PushChange(NetChange::ATTACK);
								c.unit = unit;
								c.id = AID_RunningAttack;
								c.f[1] = speed;
							}

							if(Net::IsLocal())
							{
								Train(TrainWhat::AttackStart, 0.f, 0);
								const Weapon& weapon = u.GetWeapon();
								u.RemoveStamina(weapon.GetInfo().stamina * 1.5f * u.GetStaminaMod(weapon));
							}
						}
						else
						{
							// prepare attack
							float speed = u.GetPowerAttackSpeed() * u.GetStaminaAttackSpeedMod();
							u.meshInst->GetGroup(1).speed = speed;
							actionKey = k;
							u.animationState = AS_ATTACK_PREPARE;
							u.act.attack.run = false;
							u.timer = 0.f;

							if(Net::IsOnline())
							{
								NetChange& c = Net::PushChange(NetChange::ATTACK);
								c.unit = unit;
								c.id = AID_PrepareAttack;
								c.f[1] = speed;
							}

							if(Net::IsLocal())
							{
								const Weapon& weapon = u.GetWeapon();
								u.RemoveStamina(weapon.GetInfo().stamina * u.GetStaminaMod(weapon));
							}
						}
					}
				}
			}

			// no action or non-heavy attack
			if(u.frozen == FROZEN::NO && u.HaveShield() && !data.abilityReady && (u.action == A_NONE
				|| (u.action == A_ATTACK && !u.act.attack.run && !(u.animationState == AS_ATTACK_CAN_HIT && u.act.attack.power > 1.5f))))
			{
				Key k = GKey.KeyDoReturnIgnore(GK_BLOCK, &Input::Down, data.wastedKey);
				if(k != Key::None)
				{
					// start blocking
					const float blendMax = u.GetBlockSpeed();
					u.action = A_BLOCK;
					u.meshInst->Play(NAMES::aniBlock, PLAY_PRIO1 | PLAY_STOP_AT_END, 1);
					u.meshInst->GetGroup(1).blendMax = blendMax;
					actionKey = k;

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::ATTACK);
						c.unit = unit;
						c.id = AID_Block;
						c.f[1] = blendMax;
					}
				}
			}
		}
		else
		{
			// shoting from bow
			if(u.action == A_SHOOT)
			{
				if(u.animationState == AS_SHOOT_PREPARE)
				{
					if(GKey.KeyUpAllowed(actionKey))
					{
						u.animationState = AS_SHOOT_CAN;
						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_Shoot;
							c.f[1] = 1.f;
						}
					}
					else if(GKey.KeyPressedUpAllowed(GK_CANCEL_ATTACK))
					{
						u.meshInst->Deactivate(1);
						u.action = A_NONE;
						data.wastedKey = actionKey;
						gameLevel->FreeBowInstance(u.bowInstance);
						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::ATTACK);
							c.unit = unit;
							c.id = AID_Cancel;
							c.f[1] = 1.f;
						}
					}
				}
			}
			else if(u.frozen == FROZEN::NO && !data.abilityReady)
			{
				Key k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &Input::Down, data.wastedKey);
				if(k == Key::None)
					k = GKey.KeyDoReturnIgnore(GK_SECONDARY_ATTACK, &Input::Down, data.wastedKey);
				if(k != Key::None)
				{
					actionKey = k;
					u.DoRangedAttack(true);
				}
			}
		}
	}

	// ability
	if(u.frozen == FROZEN::NO && shortcut.type == Shortcut::TYPE_ABILITY && data.abilityReady != shortcut.ability)
	{
		PlayerController::CanUseAbilityResult result = CanUseAbility(shortcut.ability, true);
		if(Any(result,
			PlayerController::CanUseAbilityResult::Yes,
			PlayerController::CanUseAbilityResult::TakeWand,
			PlayerController::CanUseAbilityResult::TakeBow))
		{
			data.abilityReady = shortcut.ability;
			data.abilityOk = false;
			data.abilityRot = 0.f;
			data.abilityTarget = nullptr;
			if(result == PlayerController::CanUseAbilityResult::TakeWand)
				unit->TakeWeapon(W_ONE_HANDED);
			else if(result == PlayerController::CanUseAbilityResult::TakeBow)
				unit->TakeWeapon(W_BOW);
		}
		else
		{
			if(result == PlayerController::CanUseAbilityResult::NeedWand)
				gameGui->messages->AddGameMsg3(GMS_NEED_WAND);
			else if(result == PlayerController::CanUseAbilityResult::NeedBow)
				gameGui->messages->AddGameMsg3(GMS_NEED_BOW);
			soundMgr->PlaySound2d(gameRes->sCancel);
		}
	}
	else if(data.abilityReady)
	{
		Ability& ability = *data.abilityReady;
		if(ability.type == Ability::Charge && IsSet(ability.flags, Ability::PickDir))
		{
			// adjust action dir
			switch(move)
			{
			case 0: // none
			case 10: // forward
				data.abilityRot = 0.f;
				break;
			case -10: // backward
				data.abilityRot = PI;
				break;
			case -1: // left
				data.abilityRot = -PI / 2;
				break;
			case 1: // right
				data.abilityRot = PI / 2;
				break;
			case 9: // forward left
				data.abilityRot = -PI / 4;
				break;
			case 11: // forward right
				data.abilityRot = PI / 4;
				break;
			case -11: // backward left
				data.abilityRot = -PI * 3 / 4;
				break;
			case -9: // backward right
				data.abilityRot = PI * 3 / 4;
				break;
			}
		}

		data.wastedKey = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::Down);
		if(data.wastedKey != Key::None)
		{
			if(data.abilityOk && CanUseAbility(data.abilityReady, false) == CanUseAbilityResult::Yes)
				UseAbility(data.abilityReady, false);
			else
				data.wastedKey = Key::None;
		}
		else
		{
			data.wastedKey = GKey.KeyDoReturn(GK_BLOCK, &Input::PressedRelease);
			if(data.wastedKey != Key::None || GKey.KeyPressedUpAllowed(GK_CANCEL_ATTACK))
				data.abilityReady = nullptr;
		}
	}

	// idle animation
	if(idle && u.action == A_NONE)
	{
		idleTimer += dt;
		if(idleTimer >= 4.f)
		{
			if(u.animation == ANI_LEFT || u.animation == ANI_RIGHT)
				idleTimer = Random(2.f, 3.f);
			else
			{
				int id = Rand() % u.data->idles->anims.size();
				idleTimer = Random(0.f, 0.5f);
				u.meshInst->Play(u.data->idles->anims[id].c_str(), PLAY_ONCE, 0);
				u.animation = ANI_IDLE;

				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::IDLE);
					c.unit = unit;
					c.id = id;
				}
			}
		}
	}
	else
		idleTimer = Random(0.f, 0.5f);
}

//=================================================================================================
void PlayerController::UpdateCooldown(float dt)
{
	for(PlayerAbility& ab : abilities)
	{
		if(ab.cooldown != 0)
			ab.cooldown = Max(0.f, ab.cooldown - dt);
		if(ab.recharge != 0)
		{
			ab.recharge -= dt;
			if(ab.recharge < 0)
			{
				++ab.charges;
				if(ab.charges == ab.ability->charges)
					ab.recharge = 0;
				else
					ab.recharge += ab.ability->recharge;
			}
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
	if(nextAction != NA_NONE)
	{
		nextAction = NA_NONE;
		if(Net::IsClient())
			Net::PushChange(NetChange::SET_NEXT_ACTION);
	}
}

//=================================================================================================
Vec3 PlayerController::RaytestTarget(float range)
{
	RaytestWithIgnoredCallback clbk(unit, nullptr);
	Vec3 from = gameLevel->camera.from;
	Vec3 dir = (gameLevel->camera.to - from).Normalized();
	from += dir * gameLevel->camera.dist;
	Vec3 to = from + dir * range;
	phyWorld->rayTest(ToVector3(from), ToVector3(to), clbk);
	if(range < 10)
		data.rangeRatio = clbk.fraction * range / 10;
	else
		data.rangeRatio = clbk.fraction;
	return from + dir * range * clbk.fraction;
}

//=================================================================================================
bool PlayerController::ShouldUseRaytest() const
{
	if(action != PlayerAction::None)
		return false;
	return (unit->weaponState == WeaponState::Taken && unit->weaponTaken == W_BOW && Any(unit->action, A_NONE, A_SHOOT))
		|| (data.abilityReady && Any(data.abilityReady->type, Ability::Target, Ability::Point, Ability::Ray, Ability::Summon, Ability::Trap))
		|| (unit->action == A_CAST && Any(unit->act.cast.ability->type, Ability::Point, Ability::Ray, Ability::Summon, Ability::Trap))
		|| (unit->weaponState == WeaponState::Taken && unit->weaponTaken == W_ONE_HANDED
			&& IsSet(unit->GetWeapon().flags, ITEM_WAND) && unit->Get(SkillId::MYSTIC_MAGIC) > 0
			&& Any(unit->action, A_NONE, A_ATTACK, A_CAST, A_BLOCK, A_BASH));
}

//=================================================================================================
void PlayerController::ReadBook(int index)
{
	assert(index >= 0 && index < int(unit->items.size()));
	ItemSlot& slot = unit->items[index];
	assert(slot.item && slot.item->type == IT_BOOK);
	const Book& book = slot.item->ToBook();
	if(IsSet(book.flags, ITEM_MAGIC_SCROLL))
	{
		if(unit->usable) // can't use when sitting
			return;

		if(Net::IsLocal())
		{
			unit->action = A_USE_ITEM;
			unit->usedItem = slot.item;
			unit->meshInst->Play(NAMES::aniCast, PLAY_ONCE | PLAY_PRIO1, 1);
			if(Net::IsServer())
			{
				NetChange& c = Net::PushChange(NetChange::USE_ITEM);
				c.unit = unit;
			}
		}
		else
		{
			NetChange& c = Net::PushChange(NetChange::USE_ITEM);
			c.id = index;
			unit->action = A_PREPARE;
		}
	}
	else
	{
		bool useItemSent = false;
		if(!book.recipes.empty())
		{
			int skill = unit->GetBase(SkillId::ALCHEMY);
			bool anythingNew = false, anythingAllowed = false, anythingTooHard = false;
			for(Recipe* recipe : book.recipes)
			{
				if(!HaveRecipe(recipe))
				{
					anythingNew = true;
					if(skill >= recipe->skill)
						anythingAllowed = true;
					else
						anythingTooHard = true;
				}
			}

			if(!anythingNew)
				gameGui->messages->AddGameMsg3(GMS_ALREADY_LEARNED);
			else if(!anythingAllowed)
				gameGui->messages->AddGameMsg3(GMS_TOO_COMPLICATED);
			else
			{
				if(Net::IsLocal())
				{
					for(Recipe* recipe : book.recipes)
					{
						if(skill >= recipe->skill)
							AddRecipe(recipe);
					}
					if(IsSet(book.flags, ITEM_SINGLE_USE))
						unit->RemoveItem(index, 1u);
					if(anythingTooHard)
						gameGui->messages->AddGameMsg3(GMS_TOO_COMPLICATED);
				}
				else
				{
					NetChange& c = Net::PushChange(NetChange::USE_ITEM);
					c.id = index;
					unit->action = A_PREPARE;
					useItemSent = true;
				}
			}
		}

		if(!IsSet(book.flags, ITEM_SINGLE_USE))
		{
			if(Net::IsLocal())
				questMgr->CheckItemEventHandler(unit, &book);
			else if(!useItemSent)
			{
				NetChange& c = Net::PushChange(NetChange::USE_ITEM);
				c.id = index;
			}
			gameGui->book->Show(&book);
		}
	}
}

//=================================================================================================
void PlayerController::CheckBuildingUnderground(bool instant)
{
	if(unit->locPart->partType == LocationPart::Type::Building)
	{
		// check building underground
		InsideBuilding& insideBuilding = static_cast<InsideBuilding&>(*unit->locPart);
		if(insideBuilding.underground[0] < 900.f)
		{
			if(unit->pos.y < insideBuilding.underground[0])
			{
				buildingUndergroundState = true;
				if(instant)
				{
					buildingUndergroundValue = 1.f;
					insideBuilding.SetUndergroundValue(1.f);
				}
			}
			else if(unit->pos.y > insideBuilding.underground[1])
				buildingUndergroundState = false;
		}
	}
}
