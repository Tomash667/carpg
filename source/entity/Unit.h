#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"
#include "UnitData.h"
#include "Class.h"
#include "MeshInstance.h"
#include "HumanData.h"
#include "Hero.h"
#include "PlayerController.h"
#include "Usable.h"
#include "Effect.h"
#include "Buff.h"
#include "Event.h"

//-----------------------------------------------------------------------------
enum Animation
{
	ANI_NONE = -1,
	ANI_WALK,
	ANI_WALK_BACK,
	ANI_RUN,
	ANI_LEFT,
	ANI_RIGHT,
	ANI_STAND,
	ANI_BATTLE,
	ANI_BATTLE_BOW,
	ANI_DIE,
	ANI_PLAY,
	ANI_KNEELS,
	ANI_IDLE,
	ANI_MAX
};

//-----------------------------------------------------------------------------
enum ACTION
{
	A_NONE,
	A_TAKE_WEAPON,
	A_SHOOT,
	A_ATTACK,
	A_BLOCK,
	A_BASH,
	A_DRINK,
	A_EAT,
	A_PAIN,
	A_CAST,
	A_ANIMATION, // animation without object (looking around, scratching)
	A_USE_USABLE,
	A_POSITION, // was using object but was stunned or died, need to move unit to valid place
	A_PICKUP,
	A_DASH,
	A_DESPAWN,
	A_PREPARE, // mp client want to use object, waiting for response
	A_STAND_UP,
	A_USE_ITEM,
	A_POSITION_CORPSE, // move corpse that thanks to animation is now not lootable
};

//-----------------------------------------------------------------------------
enum AnimationState
{
	AS_NONE = 0,

	AS_ATTACK_PREPARE = 0,
	AS_ATTACK_CAN_HIT = 1,
	AS_ATTACK_FINISHED = 2,

	AS_BASH_ANIMATION = 0,
	AS_BASH_CAN_HIT = 1,
	AS_BASH_HITTED = 2,

	AS_CAST_ANIMATION = 0,
	AS_CAST_CASTED = 1,
	AS_CAST_TRIGGER = 2,
	AS_CAST_KNEEL = 3,

	AS_DRINK_START = 0,
	AS_DRINK_EFFECT = 1,
	AS_DRINK_END = 2,

	AS_EAT_START = 0,
	AS_EAT_SOUND = 1,
	AS_EAT_EFFECT = 2,
	AS_EAT_END = 3,

	AS_POSITION_NORMAL = 0,
	AS_POSITION_HURT = 1,
	AS_POSITION_HURT_END = 2,

	AS_SHOOT_PREPARE = 0,
	AS_SHOOT_CAN = 1,
	AS_SHOOT_SHOT = 2,
	AS_SHOOT_FINISHED = 3,

	AS_TAKE_WEAPON_START = 0,
	AS_TAKE_WEAPON_MOVED = 1,

	AS_USE_USABLE_MOVE_TO_OBJECT = 0,
	AS_USE_USABLE_USING = 1,
	AS_USE_USABLE_USING_SOUND = 2,
	AS_USE_USABLE_MOVE_TO_ENDPOINT = 3
};

inline bool IsBlocking(ACTION a)
{
	return a == A_ANIMATION || a == A_PICKUP || a == A_DASH || a == A_STAND_UP;
}

//-----------------------------------------------------------------------------
enum class WeaponState
{
	Hidden,
	Hiding,
	Taking,
	Taken
};

//-----------------------------------------------------------------------------
enum class AutoTalkMode
{
	No,
	Yes,
	Wait,
	Leader
};

//-----------------------------------------------------------------------------
// Frozen state - used to disable unit movement
// used for warping between locations, entering buildings
enum class FROZEN
{
	NO, // not frozen
	ROTATE, // can't move but can rotate (used after warp to arena combat)
	YES, // can't move/rotate
	YES_NO_ANIM, // can't move/rotate but don't change animation to standing (used by cheat 'warp')
};

//-----------------------------------------------------------------------------
enum UnitOrder
{
	ORDER_NONE,
	ORDER_WANDER,
	ORDER_WAIT,
	ORDER_FOLLOW,
	ORDER_LEAVE,
	ORDER_MOVE,
	ORDER_LOOK_AT,
	ORDER_ESCAPE_TO, // escape from enemies moving toward point
	ORDER_ESCAPE_TO_UNIT, // escape from enemies moving toward target
	ORDER_GOTO_INN,
	ORDER_GUARD, // stays near target, remove dontAttack when target dontAttack is removed
	ORDER_AUTO_TALK,
	ORDER_MAX
};

//-----------------------------------------------------------------------------
struct TraderStock
{
	vector<ItemSlot> items;
	int date;
};

//-----------------------------------------------------------------------------
enum MoveType
{
	MOVE_RUN,
	MOVE_WALK,
	MOVE_RUN_WHEN_NEAR_TEAM
};

//-----------------------------------------------------------------------------
struct UnitOrderEntry : public ObjectPoolProxy<UnitOrderEntry>
{
	UnitOrder order;
	Entity<Unit> unit;
	float timer;
	union
	{
		struct
		{
			Vec3 pos;
			MoveType moveType;
			float range;
		};
		struct
		{
			AutoTalkMode autoTalk;
			GameDialog* autoTalkDialog;
			Quest* autoTalkQuest;
		};
	};
	UnitOrderEntry* next;

	UnitOrderEntry() : next(nullptr) {}
	void OnFree()
	{
		if(next)
		{
			next->Free();
			next = nullptr;
		}
	}
	UnitOrderEntry* WithTimer(float timer);
	UnitOrderEntry* WithMoveType(MoveType moveType);
	UnitOrderEntry* WithRange(float range);
	UnitOrderEntry* ThenWander();
	UnitOrderEntry* ThenWait();
	UnitOrderEntry* ThenFollow(Unit* target);
	UnitOrderEntry* ThenLeave();
	UnitOrderEntry* ThenMove(const Vec3& pos);
	UnitOrderEntry* ThenLookAt(const Vec3& pos);
	UnitOrderEntry* ThenEscapeTo(const Vec3& pos);
	UnitOrderEntry* ThenEscapeToUnit(Unit* target);
	UnitOrderEntry* ThenGoToInn();
	UnitOrderEntry* ThenGuard(Unit* target);
	UnitOrderEntry* ThenAutoTalk(bool leader, GameDialog* dialog, Quest* quest);

private:
	UnitOrderEntry* NextOrder();
};

//-----------------------------------------------------------------------------
struct Unit : public EntityType<Unit>
{
	enum LiveState
	{
		ALIVE,
		FALLING,
		FALL,
		DYING,
		DEAD,
		LIVESTATE_MAX
	};

	enum StaminaAction
	{
		SA_DONT_RESTORE,
		SA_RESTORE_SLOW,
		SA_RESTORE,
		SA_RESTORE_MORE
	};

	enum LoadState
	{
		LS_NONE, // < 25%
		LS_LIGHT, // < 50%
		LS_MEDIUM, // < 75%
		LS_HEAVY, // < 100%
		LS_OVERLOADED, // < 200%
		LS_MAX_OVERLOADED // >= 200%
	};

	// used in mp by clients
	enum AiMode
	{
		AI_MODE_DONT_ATTACK = 1 << 0,
		AI_MODE_ASSIST = 1 << 1,
		AI_MODE_IDLE = 1 << 3,
		AI_MODE_ATTACK_TEAM = 1 << 4
	};

	enum class CREATE_MESH
	{
		NORMAL,
		ON_WORLDMAP,
		PRELOAD,
		AFTER_PRELOAD,
		LOAD
	};

	static const int MIN_SIZE = 36;
	static const float AUTO_TALK_WAIT;
	static const float STAMINA_BOW_ATTACK;
	static const float STAMINA_BASH_ATTACK;
	static const float STAMINA_UNARMED_ATTACK;
	static const float STAMINA_RESTORE_TIMER;
	static const float EAT_SOUND_DIST;
	static const float DRINK_SOUND_DIST;
	static const float ATTACK_SOUND_DIST;
	static const float TALK_SOUND_DIST;
	static const float ALERT_SOUND_DIST;
	static const float PAIN_SOUND_DIST;
	static const float DIE_SOUND_DIST;
	static const float YELL_SOUND_DIST;
	static const float COUGHS_SOUND_DIST;

	LocationPart* locPart;
	UnitData* data;
	PlayerController* player;
	AIController* ai;
	Hero* hero;
	Human* humanData;
	MeshInstance* meshInst;
	Animation animation, currentAnimation;
	LiveState liveState;
	Vec3 pos, visualPos, prevPos, targetPos, targetPos2;
	float rot, prevSpeed, hp, hpmax, mp, mpmax, stamina, staminaMax, speed, hurtTimer, talkTimer, timer, lastBash, alcohol, raiseTimer, staminaTimer;
	int refs, animationState, level, gold, inArena, questId, aiMode;
	FROZEN frozen;
	ACTION action;
	union ActionData
	{
		ActionData() {}
		struct AttackAction
		{
			int index;
			float power;
			bool run, hitted;
		} attack;
		struct CastAction
		{
			Ability* ability;
			Entity<Unit> target;
		} cast;
		struct DashAction
		{
			Ability* ability;
			float rot;
			UnitList* hit;
		} dash;
		struct ShootAction
		{
			Ability* ability;
		} shoot;
		struct UseUsableAction
		{
			float rot;
		} useUsable;
	} act;
	WeaponType weaponTaken, weaponHiding;
	WeaponState weaponState;
	MeshInstance* bowInstance;
	const Item* usedItem;
	vector<Effect> effects;
	bool talking, toRemove, temporary, changed, dontAttack, assist, attackTeam, fakeUnit, moved, mark, running, usedItemIsTeam;
	btCollisionObject* cobj;
	Usable* usable;
	UnitEventHandler* eventHandler;
	SpeechBubble* bubble;
	Entity<Unit> summoner, lookTarget;
	enum Busy
	{
		Busy_No,
		Busy_Yes,
		Busy_Talking,
		Busy_Looted,
		Busy_Trading,
		Busy_Tournament
	} busy; // not saved, should be Busy_No at saving
	EntityInterpolator* interp;
	UnitStats* stats;
	StaminaAction staminaAction;
	TraderStock* stock;
	vector<QuestDialog> dialogs;
	vector<Event> events;
	UnitOrderEntry* order;

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	Unit() : meshInst(nullptr), hero(nullptr), ai(nullptr), player(nullptr), cobj(nullptr), interp(nullptr), bowInstance(nullptr), fakeUnit(false),
		humanData(nullptr), staminaAction(SA_RESTORE_MORE), moved(false), refs(1), stock(nullptr), stats(nullptr), mark(false), order(nullptr) {}
	~Unit();

	void AddRef() { ++refs; }
	void Release();
	void Init(UnitData& base, int level = -2);

	float CalculateAttack() const;
	float CalculateAttack(const Item* weapon) const;
	float CalculateBlock(const Item* shield = nullptr) const;
	float CalculateDefense(const Item* armor = nullptr) const;
	bool IsStanding() const { return liveState == ALIVE; }
	bool IsAlive() const { return liveState < DYING; }
	bool IsIdle() const;
	bool IsAssist() const;
	bool IsDontAttack() const;
	bool WantAttackTeam() const;
	byte GetAiMode() const;
	void RecalculateWeight();
	// konsumuje przedmiot (zwraca 0-u¿yto ostatni, 1-u¿yto nie ostatni, 2-chowa broñ, 3-zajêty)
	int ConsumeItem(int index);
	// u¿ywa przedmiotu, nie mo¿e nic robiæ w tej chwili i musi mieæ schowan¹ broñ
	void ConsumeItem(const Consumable& item, bool force = false, bool send = true);
	void ConsumeItemAnim(const Consumable& cons);
	void ConsumeItemS(const Item* item);
	void HideWeapon() { SetWeaponState(false, W_NONE, true); }
	void TakeWeapon(WeaponType type);
	float GetSphereRadius() const
	{
		float radius = data->mesh->head.radius * data->scale;
		if(data->type == UNIT_TYPE::HUMAN)
			radius *= ((humanData->height - 1) * 0.2f + 1.f);
		return radius;
	}
	float GetUnitRadius() const
	{
		if(data->type == UNIT_TYPE::HUMAN)
			return 0.3f * ((humanData->height - 1) * 0.2f + 1.f);
		else
			return data->width;
	}
	Vec3 GetColliderPos() const
	{
		if(action != A_USE_USABLE)
			return pos;
		else if(animationState == AS_USE_USABLE_MOVE_TO_ENDPOINT)
			return targetPos;
		else
			return targetPos2;
	}
	float GetUnitHeight() const
	{
		if(data->type == UNIT_TYPE::HUMAN)
			return 1.73f * data->scale * ((humanData->height - 1) * 0.2f + 1.f);
		else
			return data->scale * data->mesh->head.bbox.SizeY();
	}
	Vec3 GetPhysicsPos() const
	{
		Vec3 p = pos;
		p.y += max(MIN_H, GetUnitHeight()) * 0.5f + 0.1f;
		return p;
	}
	Vec3 GetHeadPoint() const
	{
		Vec3 pt = visualPos;
		pt.y += GetUnitHeight() * 1.1f;
		return pt;
	}
	Vec3 GetHeadSoundPos() const
	{
		Vec3 pt = visualPos;
		pt.y += GetUnitHeight() * 0.9f;
		return pt;
	}
	Vec3 GetUnitTextPos() const
	{
		Vec3 pt;
		if(IsStanding())
		{
			pt = visualPos;
			pt.y += GetUnitHeight();
		}
		else
		{
			pt = GetLootCenter();
			pt.y += 0.5f;
		}
		return pt;
	}
	Vec3 GetEyePos() const;
	float CalculateMaxHp() const;
	float CalculateMaxMp() const;
	float GetMpRegen() const;
	float CalculateMaxStamina() const;
	float GetHpp() const { return hp / hpmax; }
	float GetMpp() const { return mp / mpmax; }
	float GetStaminap() const { return stamina / staminaMax; }
	void GetBox(Box& box) const;
	int GetDmgType() const;
	bool IsNotFighting() const
	{
		if(data->type == UNIT_TYPE::ANIMAL)
			return false;
		else
			return weaponState == WeaponState::Hidden;
	}
	Vec3 GetLootCenter() const;

	float CalculateWeaponPros(const Weapon& weapon) const;
	bool IsBetterWeapon(const Weapon& weapon, int* value = nullptr, int* prevValue = nullptr) const;
	bool IsBetterArmor(const Armor& armor, int* value = nullptr, int* prevValue = nullptr) const;
	bool IsBetterItem(const Item* item, int* value = nullptr, int* prevValue = nullptr, ITEM_SLOT* targetSlot = nullptr) const;
	float GetItemAiValue(const Item* item) const;
	bool IsPlayer() const { return (player != nullptr); }
	bool IsLocalPlayer() const { return IsPlayer() && player->IsLocal(); }
	bool IsOtherPlayer() const { return IsPlayer() && !player->IsLocal(); }
	bool IsAI() const { return !IsPlayer(); }
	float GetRotationSpeed() const
	{
		return data->rotSpeed * GetMobilityMod(false);
	}
	float GetWalkSpeed() const
	{
		return data->walkSpeed * GetMobilityMod(false) * (1.f - GetEffectMax(EffectId::SlowMove));
	}
	float GetRunSpeed() const
	{
		return data->runSpeed * GetMobilityMod(true) * (1.f - GetEffectMax(EffectId::SlowMove));
	}
	bool CanMove() const { return !HaveEffect(EffectId::Rooted); }
	bool CanRun() const;
	void RecalculateHp();
	void RecalculateMp();
	void RecalculateStamina();
	bool CanBlock() const
	{
		return weaponState == WeaponState::Taken && weaponTaken == W_ONE_HANDED && HaveShield();
	}

	WeaponType GetHoldWeapon() const
	{
		switch(weaponState)
		{
		case WeaponState::Taken:
		case WeaponState::Taking:
			return weaponTaken;
		case WeaponState::Hiding:
			return weaponHiding;
		case WeaponState::Hidden:
		default:
			return W_NONE;
		}
	}
	bool IsHoldingMeeleWeapon() const
	{
		return GetHoldWeapon() == W_ONE_HANDED;
	}
	bool IsHoldingBow() const
	{
		return GetHoldWeapon() == W_BOW;
	}
	Vec3 GetFrontPos() const
	{
		return Vec3(
			pos.x + sin(rot + PI) * 2,
			pos.y,
			pos.z + cos(rot + PI) * 2);
	}
	MATERIAL_TYPE GetWeaponMaterial() const
	{
		if(HaveWeapon())
			return GetWeapon().material;
		else
			return data->mat;
	}
	float GetAttackFrame(int frame) const;
	int GetRandomAttack() const;
	void Save(GameWriter& f);
	void SaveStock(GameWriter& f);
	void Load(GameReader& f);
	void LoadStock(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);
	Vec3 GetCenter() const
	{
		Vec3 pt = pos;
		pt.y += GetUnitHeight() / 2;
		return pt;
	}
	int FindHealingPotion() const;
	int FindManaPotion() const;
	float GetAttackRange() const
	{
		return data->attackRange;
	}
	void ReequipItems();
private:
	void ReequipItemsInternal();
public:
	bool CanUseWeapons() const
	{
		return data->type >= UNIT_TYPE::HUMANOID;
	}
	bool CanUseArmor() const
	{
		return data->type == UNIT_TYPE::HUMAN;
	}
	bool HaveQuestItem(int questId);
	void RemoveQuestItem(int questId);
	void RemoveQuestItemS(Quest* quest);
	bool HaveItem(const Item* item, bool owned = false) const;
	bool HaveItemEquipped(const Item* item) const;
	bool SlotRequireHideWeapon(ITEM_SLOT slot) const;
	float GetAttackSpeed(const Weapon* weapon = nullptr) const;
	float GetAttackSpeedModFromStrength(const Weapon& wep) const
	{
		int str = Get(AttributeId::STR);
		if(str >= wep.reqStr)
			return 0.f;
		else if(str * 2 <= wep.reqStr)
			return 0.5f;
		else
			return 0.5f * float(wep.reqStr - str) / (wep.reqStr / 2);
	}
	float GetPowerAttackSpeed() const
	{
		if(HaveWeapon())
			return GetWeapon().GetInfo().powerSpeed * GetAttackSpeed();
		else
			return 0.33f;
	}
	float GetBowAttackSpeed() const;
	float GetAttackSpeedModFromStrength(const Bow& b) const
	{
		int str = Get(AttributeId::STR);
		if(str >= b.reqStr)
			return 0.f;
		else if(str * 2 <= b.reqStr)
			return 0.75f;
		else
			return 0.75f * float(b.reqStr - str) / (b.reqStr / 2);
	}
	bool IsHero() const { return hero != nullptr; }
	bool IsFollower() const { return hero && hero->teamMember; }
	bool IsFollowing(Unit* u) const { return GetOrder() == ORDER_FOLLOW && order->unit == u; }
	bool IsFollowingTeamMember() const { return IsFollower() && GetOrder() == ORDER_FOLLOW; }
	Class* GetClass() const { return data->clas; }
	const string& GetClassId() const { return data->clas->id; }
	bool IsUsingMp() const
	{
		if(data->clas)
			return IsSet(data->clas->flags, Class::F_MP_BAR);
		return IsSet(data->flags2, F2_MP_BAR);
	}
	bool CanFollowWarp() const { return IsHero() && GetOrder() == ORDER_FOLLOW && inArena == -1 && frozen == FROZEN::NO; }
	bool IsTeamMember() const
	{
		if(IsPlayer())
			return true;
		else if(IsHero())
			return hero->teamMember;
		else
			return false;
	}
	void MakeItemsTeam(bool isTeam);
	void Heal(float heal)
	{
		hp += heal;
		if(hp > hpmax)
			hp = hpmax;
	}
	void NaturalHealing(int days)
	{
		Heal(0.15f * Get(AttributeId::END) * days);
	}
	void HealPoison();
	// szuka przedmiotu w ekwipunku, zwraca iIndex (INVALID_IINDEX jeœli nie ma takiego przedmiotu)
	static const int INVALID_IINDEX = (-SLOT_INVALID - 1);
	int FindItem(const Item* item, int questId = -1) const;
	int FindItem(delegate<bool(const ItemSlot& slot)> callback) const;
	int FindQuestItem(int questId) const;
	bool FindQuestItem(cstring id, Quest** quest, int* iIndex, bool notActive = false, int questId = -1);
	void EquipItem(int index);
	void EquipItem(const Item* item);
	void RemoveItem(int iindex, bool activeLocation = true);
	uint RemoveItem(int iIndex, uint count);
	uint RemoveItem(const Item* item, uint count);
	uint RemoveItemS(const string& item_id, uint count);
	void RemoveEquippedItem(ITEM_SLOT slot);
	void RemoveAllEquippedItems();
	int CountItem(const Item* item);
	const string& GetNameS() const
	{
		if(IsPlayer())
			return player->name;
		else if(IsHero() && hero->knowName)
			return hero->name;
		else
			return data->name;
	}
	cstring GetName() const { return GetNameS().c_str(); }
	void SetName(const string& name);
	void ClearInventory();
	bool PreferMelee()
	{
		if(IsFollower())
			return hero->melee;
		else
			return IsSet(data->flags2, F2_MELEE);
	}
	bool IsImmortal() const
	{
		if(IsSet(data->flags, F_IMMORTAL))
			return true;
		else if(IsPlayer())
			return player->godmode;
		else
			return false;
	}
	const string& GetRealNameS() const
	{
		if(IsPlayer())
			return player->name;
		else if(IsHero() && !hero->name.empty())
			return hero->name;
		else
			return data->name;
	}
	cstring GetRealName() const
	{
		return GetRealNameS().c_str();
	}
	void RevealName(bool setName);
	bool GetKnownName() const;
	void SetKnownName(bool known);

	float GetBlockSpeed() const;
	float CalculateMagicResistance() const;
	float GetPoisonResistance() const;
	int CalculateMagicPower() const { return (int)GetEffectSum(EffectId::MagicPower); }
	float GetBackstabMod(const Item* item) const;

	//-----------------------------------------------------------------------------
	// EFFECTS
	//-----------------------------------------------------------------------------
	void AddEffect(Effect& e, bool send = true);
	void ApplyConsumableEffect(const Consumable& item);
	void UpdateEffects(float dt);
	void EndEffects(int days = 0, float* outNaturalMod = nullptr);
	Effect* FindEffect(EffectId effect);
	bool FindEffect(EffectId effect, float* value);
	void RemoveEffect(EffectId effect);
	void RemoveEffects(bool send = true);
	uint RemoveEffects(EffectId effect, EffectSource source, int sourceId, int value);
	float GetEffectSum(EffectId effect) const;
	float GetEffectMul(EffectId effect) const;
	float GetEffectMulInv(EffectId effect) const;
	float GetEffectMax(EffectId effect) const;
	bool HaveEffect(EffectId effect, int value = -1) const;
	void OnAddRemoveEffect(Effect& e);
	void ApplyItemEffects(const Item* item, ITEM_SLOT slot);
	void RemoveItemEffects(const Item* item, ITEM_SLOT slot);

	//-----------------------------------------------------------------------------
	// EQUIPMENT
	//-----------------------------------------------------------------------------
private:
	array<const Item*, SLOT_MAX> slots;
public:
	vector<ItemSlot> items;
	int weight, weightMax;
	//-----------------------------------------------------------------------------
	int GetGold() const { return gold; }
	void ModGold(int goldChange) { SetGold(gold + goldChange); }
	void SetGold(int gold);
	array<const Item*, SLOT_MAX>& GetEquippedItems() { return slots; }
	const Item* GetEquippedItem(ITEM_SLOT slot) const { return slots[slot]; }
	bool HaveEquippedItem(ITEM_SLOT slot) const { return slots[slot] != nullptr; }
	bool HaveWeapon() const { return slots[SLOT_WEAPON] != nullptr; }
	bool HaveBow() const { return slots[SLOT_BOW] != nullptr; }
	bool HaveShield() const { return slots[SLOT_SHIELD] != nullptr; }
	bool HaveArmor() const { return slots[SLOT_ARMOR] != nullptr; }
	bool HaveAmulet() const { return slots[SLOT_AMULET] != nullptr; }
	bool CanWear(const Item* item) const;
	bool WantItem(const Item* item) const;
	const Weapon& GetWeapon() const
	{
		assert(HaveWeapon());
		return slots[SLOT_WEAPON]->ToWeapon();
	}
	const Bow& GetBow() const
	{
		assert(HaveBow());
		return slots[SLOT_BOW]->ToBow();
	}
	const Shield& GetShield() const
	{
		assert(HaveShield());
		return slots[SLOT_SHIELD]->ToShield();
	}
	const Armor& GetArmor() const
	{
		assert(HaveArmor());
		return slots[SLOT_ARMOR]->ToArmor();
	}
	const Item& GetAmulet() const
	{
		assert(HaveAmulet());
		return *slots[SLOT_AMULET];
	}
	void ReplaceItem(ITEM_SLOT slot, const Item* item);
	void ReplaceItems(array<const Item*, SLOT_MAX>& items);
	// drop item by index, return if that was last item from stack (count=0 drops all)
	bool DropItem(int index, uint count = 1);
	// wyrzuca za³o¿ony przedmiot
	void DropItem(ITEM_SLOT slot);
	// dodaje przedmiot do ekwipunku, zwraca czy siê zestackowa³
	bool AddItem(const Item* item, uint count, uint teamCount);
	// add item and show game message, send net notification, calls preload, refresh inventory if open
	void AddItem2(const Item* item, uint count, uint teamCount, bool showMsg = true, bool notify = true);
	void AddItemS(const Item* item, uint count) { AddItem2(item, count, 0u); }
	void AddTeamItemS(const Item* item, uint count) { AddItem2(item, count, count); }
	// dodaje przedmiot i zak³ada jeœli nie ma takiego typu, przedmiot jest dru¿ynowy
	void AddItemAndEquipIfNone(const Item* item, uint count = 1);
	// zwraca udŸwig postaci (0-brak obci¹¿enia, 1-maksymalne, >1 przeci¹¿ony)
	float GetLoad() const { return float(weight) / weightMax; }
	void CalculateLoad();
	bool IsOverloaded() const
	{
		return weight > weightMax;
	}
	bool IsMaxOverloaded() const
	{
		return weight > weightMax * 2;
	}
	LoadState GetLoadState() const
	{
		if(weight <= weightMax / 4)
			return LS_NONE;
		else if(weight <= weightMax / 2)
			return LS_LIGHT;
		else if(weight <= weightMax * 3 / 4)
			return LS_MEDIUM;
		else if(weight <= weightMax)
			return LS_HEAVY;
		else if(weight <= weightMax * 2)
			return LS_OVERLOADED;
		else
			return LS_MAX_OVERLOADED;
	}
	LoadState GetArmorLoadState(const Item* armor) const;
	float GetWeight() const { return float(weight) / 10; }
	float GetWeightMax() const { return float(weightMax) / 10; }
	bool CanTake(const Item* item, uint count = 1) const
	{
		assert(item && count);
		return weight + item->weight * (int)count <= weightMax;
	}
	const Item* GetIIndexItem(int iIndex) const;

	Mesh::Animation* GetTakeWeaponAnimation(bool melee) const;

	bool CanDoWhileUsing() const
	{
		return action == A_USE_USABLE && animationState == AS_USE_USABLE_USING && IsSet(usable->base->useFlags, BaseUsable::ALLOW_USE_ITEM);
	}

	int GetBuffs() const;

	bool CanTalk(Unit& unit) const;
	bool CanAct() const;

	int Get(AttributeId a, StatState* state = nullptr) const;
	int Get(SkillId s, StatState* state = nullptr, bool skill_bonus = true) const;
	int GetBase(AttributeId a) const { return stats->attrib[(int)a]; }
	int GetBase(SkillId s) const { return stats->skill[(int)s]; }
	void Set(AttributeId a, int value);
	void Set(SkillId s, int value);
	void ApplyStat(AttributeId a);
	void ApplyStat(SkillId s);
	void CalculateStats();
	float CalculateMobility(const Armor* armor = nullptr) const;
	float GetMobilityMod(bool run) const;

	WEAPON_TYPE GetBestWeaponType() const;
	ARMOR_TYPE GetBestArmorType() const;

	void ApplyHumanData(HumanData& hd)
	{
		hd.Set(*humanData);
		humanData->ApplyScale(meshInst);
	}

	int ItemsToSellWeight() const;

	void SetAnimationAtEnd(cstring animName = nullptr);

	float GetArrowSpeed() const
	{
		float s = (float)GetBow().speed;
		s *= 1.f + float(Get(SkillId::BOW)) / 666;
		return s;
	}

	void SetDontAttack(bool dontAttack);
	bool GetDontAttack() const { return dontAttack; }

	int& GetCredit()
	{
		if(IsPlayer())
			return player->credit;
		else
		{
			assert(IsFollower());
			return hero->credit;
		}
	}

	void UpdateStaminaAction();
	void RemoveMana(float value);
	void RemoveStamina(float value);
	void RemoveStaminaBlock(float value);
	float GetStaminaMod(const Item& item) const;

	void CreateMesh(CREATE_MESH mode);

	void UseUsable(Usable* usable);
	enum class BREAK_ACTION_MODE
	{
		NORMAL,
		FALL,
		INSTANT,
		ON_LEAVE
	};
	void BreakAction(BREAK_ACTION_MODE mode = BREAK_ACTION_MODE::NORMAL, bool notify = false, bool allowAnimation = false);
	void Fall();
	void TryStandup(float dt);
	void Standup(bool warp = true, bool leave = false);
	void Die(Unit* killer);
	void DropGold(int count);
	bool IsDrunkman() const;
	void PlaySound(Sound* sound, float range);
	void PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const Vec3& hitpoint, bool dmg);
	void CreatePhysics(bool position = false);
	void UpdatePhysics(const Vec3* pos = nullptr);
	Sound* GetSound(SOUND_ID sound_id) const;
	bool SetWeaponState(bool takes_out, WeaponType type, bool send);
	void SetWeaponStateInstant(WeaponState weaponState, WeaponType type);
	void SetTakeHideWeaponAnimationToEnd(bool hide, bool breakAction);
	void UpdateInventory(bool notify = true);
	bool IsEnemy(Unit& u, bool ignoreDontAttack = false) const;
	bool IsFriend(Unit& u, bool checkArenaAttack = false) const;
	Color GetRelationColor(Unit& u) const;
	bool IsInvisible() const { return IsPlayer() && player->invisible; }
	void RefreshStock();
	float GetMaxMorale() const { return IsSet(data->flags, F_COWARD) ? 5.f : 10.f; }
	void AddDialog(Quest2* quest, GameDialog* dialog, int priority = 0);
	void AddDialogS(Quest2* quest, const string& dialogId, int priority);
	void RemoveDialog(Quest2* quest, bool cleanup);
	void RemoveDialogS(Quest2* quest) { RemoveDialog(quest, false); }
	void AddEventHandler(Quest2* quest, EventType type);
	void RemoveEventHandler(Quest2* quest, EventType type, bool cleanup);
	void RemoveEventHandlerS(Quest2* quest, EventType type) { RemoveEventHandler(quest, type, false); }
	void RemoveAllEventHandlers();
	UnitOrder GetOrder() const
	{
		if(order)
			return order->order;
		return ORDER_NONE;
	}
private:
	void OrderReset();
public:
	void OrderClear();
	void OrderNext();
	void OrderAttack();
	UnitOrderEntry* OrderWander();
	UnitOrderEntry* OrderWait();
	UnitOrderEntry* OrderFollow(Unit* target);
	UnitOrderEntry* OrderLeave();
	UnitOrderEntry* OrderMove(const Vec3& pos);
	UnitOrderEntry* OrderLookAt(const Vec3& pos);
	UnitOrderEntry* OrderEscapeTo(const Vec3& pos);
	UnitOrderEntry* OrderEscapeToUnit(Unit* unit);
	UnitOrderEntry* OrderGoToInn();
	UnitOrderEntry* OrderGuard(Unit* target);
	UnitOrderEntry* OrderAutoTalk(bool leader = false, GameDialog* dialog = nullptr, Quest* quest = nullptr);
	void Talk(cstring text, int playAnim = -1);
	void TalkS(const string& text, int playAnim = -1) { Talk(text.c_str(), playAnim); }
	bool IsBlocking() const { return action == A_BLOCK || (action == A_BASH && animationState == AS_BASH_ANIMATION); }
	float GetBlockMod() const { return action == A_BLOCK ? Max(0.5f, meshInst->groups[1].GetBlendT()) : 0.5f; }
	float GetStaminaAttackSpeedMod() const;
	float GetBashSpeed() const;
	void RotateTo(const Vec3& pos, float dt);
	void RotateTo(const Vec3& pos);
	void RotateTo(float rot);
	void StopUsingUsable(bool send = true);
	void CheckAutoTalk(float dt);
	float GetAbilityPower(Ability& ability) const;
	void CastSpell();
	void Update(float dt);
	void Moved(bool warped = false, bool dash = false);
	void MovedToEntry(EntryType type, const Int2& pt, GameDirection dir, bool canWarp, bool isPrev);
	void ChangeBase(UnitData* ud, bool updateItems = false);
	void MoveToLocation(LocationPart* locPart, const Vec3& pos);
	void MoveOffscreen();
	void Kill();
	enum DamageFlags
	{
		DMG_NO_BLOOD = 1 << 0,
		DMG_MAGICAL = 1 << 1
	};
	void GiveDmg(float dmg, Unit* giver = nullptr, const Vec3* hitpoint = nullptr, int dmgFlags = 0);
	void AttackReaction(Unit& attacker);
	bool DoAttack();
	bool DoShieldSmash();
	void DoGenericAttack(Unit& hitted, const Vec3& hitpoint, float attack, int dmgType, bool bash);
	void DoRangedAttack(bool prepare, bool notify = true, float speed = -1);
	void AlertAllies(Unit* target);
};

//-----------------------------------------------------------------------------
struct NAMES
{
	// points
	static cstring pointWeapon;
	static cstring pointHiddenWeapon;
	static cstring pointShield;
	static cstring pointShieldHidden;
	static cstring pointBow;
	static cstring pointCast;
	static cstring points[];
	static uint nPoints;

	// animations
	static cstring aniTakeWeapon;
	static cstring aniTakeWeaponNoShield;
	static cstring aniTakeBow;
	static cstring aniMove;
	static cstring aniRun;
	static cstring aniLeft;
	static cstring aniRight;
	static cstring aniStand;
	static cstring aniBattle;
	static cstring aniBattleBow;
	static cstring aniDie;
	static cstring aniHurt;
	static cstring aniShoot;
	static cstring aniBlock;
	static cstring aniBash;
	static cstring aniBase[];
	static cstring aniHumanoid[];
	static cstring aniAttacks[];
	static uint nAniBase;
	static uint nAniHumanoid;
	static int maxAttacks;
};

//-----------------------------------------------------------------------------
inline void operator << (GameWriter& f, Unit* u)
{
	int id = (u ? u->id : -1);
	f << id;
}
inline void operator << (GameWriter& f, const SmartPtr<Unit>& u)
{
	int id = (u ? u->id : -1);
	f << id;
}
inline void operator >> (GameReader& f, Unit*& unit)
{
	int id = f.Read<int>();
	unit = Unit::GetById(id);
}
inline void operator >> (GameReader& f, SmartPtr<Unit>& unit)
{
	int id = f.Read<int>();
	unit = Unit::GetById(id);
}
