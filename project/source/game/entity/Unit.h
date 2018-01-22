#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"
#include "UnitData.h"
#include "Class.h"
#include "MeshInstance.h"
#include "HumanData.h"
#include "HeroData.h"
#include "PlayerController.h"
#include "Usable.h"
#include "Effect.h"
#include "Buff.h"
#include "StatsX.h"

//-----------------------------------------------------------------------------
struct PlayerController;
struct AIController;
struct Usable;
struct EntityInterpolator;
struct UnitEventHandler;
struct SpeechBubble;
struct GameDialog;

//-----------------------------------------------------------------------------
enum Animation
{
	ANI_NONE = -1,
	ANI_WALK,
	ANI_WALK_TYL,
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
	// etapy: 0, 1
	A_TAKE_WEAPON,
	// etapy: 0 - naci¹ga, 1 - naci¹ga ale mo¿e strzelaæ, 2 - po strzale, 3 - wyj¹³ now¹ strza³ê
	A_SHOOT,
	// etapy: 0 - przygotowanie, 1 - mo¿e atakowaæ, 2 - po trafieniu
	A_ATTACK,
	A_BLOCK,
	A_BASH,
	A_DRINK, // picie napoju (0 - zaczyna piæ, 1-efekt wypicia, 2-schowa³ przedmiot)
	A_EAT, // jedzenie (0 - zaczyna jeœæ, 1-odtwarza dŸwiêk, 2-efekt, 3-chowa)
	A_PAIN,
	A_CAST,
	A_ANIMATION, // animacja bez obiektu (np drapani siê, rozgl¹danie)
	A_ANIMATION2, // u¿ywanie obiektu (0-podchodzi, 1-u¿ywa, 2-u¿ywa dŸwiêk, 3-odchodzi)
	A_POSITION, // u¿ywa³ czegoœ ale dosta³ basha lub umar³, trzeba go przesun¹æ w normalne miejsce
	A_PICKUP, // póki co dzia³a jak animacja, potem doda siê punkt podnoszenia
	A_DASH,
	A_DESPAWN,
	A_USE_ITEM
};

//-----------------------------------------------------------------------------
enum AnimationState
{
	AS_NONE = 0,

	AS_ANIMATION2_MOVE_TO_OBJECT = 0,
	AS_ANIMATION2_USING = 1,
	AS_ANIMATION2_USING_SOUND = 2,
	AS_ANIMATION2_MOVE_TO_ENDPOINT = 3
};

inline bool IsBlocking(ACTION a)
{
	return a == A_ANIMATION || a == A_PICKUP || a == A_DASH;
}

//-----------------------------------------------------------------------------
enum WeaponState
{
	WS_HIDDEN,
	WS_HIDING,
	WS_TAKING,
	WS_TAKEN
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
// jednostka w grze
struct Unit
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
	static int netid_counter;

	int netid;
	UnitData* data;
	MeshInstance* mesh_inst;
	Animation animation, current_animation;
	Human* human_data;
	LiveState live_state;
	Vec3 pos; // pozycja postaci
	Vec3 visual_pos; // graficzna pozycja postaci, u¿ywana w MP
	Vec3 prev_pos, target_pos, target_pos2;
	float rot, prev_speed, hp, hpmax, stamina, stamina_max, speed, hurt_timer, talk_timer, timer, use_rot, attack_power, last_bash, alcohol, raise_timer;
	int animation_state, level, gold, attack_id, refid, in_building, in_arena, quest_refid;
	FROZEN frozen;
	ACTION action;
	WeaponType weapon_taken, weapon_hiding;
	WeaponState weapon_state;
	MeshInstance* bow_instance;
	PlayerController* player;
	const Item* used_item;
	bool used_item_is_team;
private:
	EffectVector effects;
public:
	bool hitted, invisible, talking, run_attack, to_remove, temporary, changed, dont_attack, assist, attack_team, fake_unit, moved;
	AIController* ai;
	btCollisionObject* cobj;
	Usable* usable;
	HeroData* hero;
	UnitEventHandler* event_handler;
	SpeechBubble* bubble;
	Unit* look_target, *guard_target, *summoner;
	int ai_mode; // u klienta w MP (0x01-dont_attack, 0x02-assist, 0x04-not_idle)
	enum Busy
	{
		Busy_No,
		Busy_Yes,
		Busy_Talking,
		Busy_Looted,
		Busy_Trading,
		Busy_Tournament
	} busy; // nie zapisywane, powinno byæ Busy_No
	EntityInterpolator* interp;
	StatsX* statsx;
	AutoTalkMode auto_talk;
	float auto_talk_timer;
	GameDialog* auto_talk_dialog;
	StaminaAction stamina_action;
	float stamina_timer;

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	Unit() : mesh_inst(nullptr), hero(nullptr), ai(nullptr), player(nullptr), cobj(nullptr), interp(nullptr), bow_instance(nullptr), fake_unit(false),
		human_data(nullptr), stamina_action(SA_RESTORE_MORE), summoner(nullptr), moved(false) {}
	~Unit();

	float CalculateAttack() const;
	float CalculateAttack(const Item* weapon) const;
	float CalculateBlock(const Item* shield = nullptr) const;
	float CalculateDefense() const { return CalculateDefense(nullptr, nullptr); }
	float CalculateDefense(const Item* armor, const Item* shield) const;
	// czy ¿yje i nie le¿y na ziemi
	bool IsStanding() const { return live_state == ALIVE; }
	// czy ¿yje
	bool IsAlive() const { return live_state < DYING; }
	void RecalculateWeight();
	// konsumuje przedmiot (zwraca 0-u¿yto ostatni, 1-u¿yto nie ostatni, 2-chowa broñ, 3-zajêty)
	int ConsumeItem(int index);
	// u¿ywa przedmiotu, nie mo¿e nic robiæ w tej chwili i musi mieæ schowan¹ broñ
	void ConsumeItem(const Consumable& item, bool force = false, bool send = true);
	void ConsumeItemAnim(const Consumable& cons);
	void HideWeapon();
	void TakeWeapon(WeaponType type);
	// dodaj efekt zjadanego przedmiotu
	void ApplyConsumableEffect(const Consumable& item);
	float GetSphereRadius() const
	{
		float radius = data->mesh->head.radius;
		if(data->type == UNIT_TYPE::HUMAN)
			radius *= ((human_data->height - 1)*0.2f + 1.f);
		return radius;
	}
	float GetUnitRadius() const
	{
		if(data->type == UNIT_TYPE::HUMAN)
			return 0.3f * ((human_data->height - 1)*0.2f + 1.f);
		else
			return data->width;
	}
	Vec3 GetColliderPos() const
	{
		if(action != A_ANIMATION2)
			return pos;
		else if(animation_state == AS_ANIMATION2_MOVE_TO_ENDPOINT)
			return target_pos;
		else
			return target_pos2;
	}
	float GetUnitHeight() const
	{
		if(data->type == UNIT_TYPE::HUMAN)
			return 1.73f * ((human_data->height - 1)*0.2f + 1.f);
		else
			return data->mesh->head.bbox.SizeY();
	}
	Vec3 GetHeadPoint() const
	{
		Vec3 pt = visual_pos;
		pt.y += GetUnitHeight() * 1.1f;
		return pt;
	}
	Vec3 GetHeadSoundPos() const
	{
		Vec3 pt = visual_pos;
		pt.y += GetUnitHeight() * 0.9f;
		return pt;
	}
	Vec3 GetUnitTextPos() const
	{
		Vec3 pt;
		if(IsStanding())
		{
			pt = visual_pos;
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
	float CalculateMaxStamina() const;
	float GetHpp() const { return hp / hpmax; }
	float GetStaminap() const { return stamina / stamina_max; }
	void GetBox(Box& box) const;
	int GetDmgType() const;
	bool IsNotFighting() const
	{
		if(data->type == UNIT_TYPE::ANIMAL)
			return false;
		else
			return (weapon_state == WS_HIDDEN);
	}
	Vec3 GetLootCenter() const;

	float CalculateWeaponPros(const Weapon& weapon) const;
	bool IsBetterWeapon(const Weapon& weapon) const;
	bool IsBetterWeapon(const Weapon& weapon, int* value) const;
	bool IsBetterArmor(const Armor& armor) const;
	bool IsBetterArmor(const Armor& armor, int* value) const;
	bool IsBetterItem(const Item* item) const;
	bool IsPlayer() const
	{
		return (player != nullptr);
	}
	bool IsAI() const
	{
		return !IsPlayer();
	}
	float GetRotationSpeed() const
	{
		return data->rot_speed * GetMobilityMod(false);
	}
	float GetWalkSpeed() const
	{
		return data->walk_speed * GetMobilityMod(false);
	}
	float GetRunSpeed() const
	{
		return data->run_speed * GetMobilityMod(true);
	}
	bool CanRun() const
	{
		if(IS_SET(data->flags, F_SLOW) || action == A_BLOCK || action == A_BASH || (action == A_ATTACK && !run_attack) || action == A_SHOOT || action == A_USE_ITEM)
			return false;
		else
			return !IsOverloaded();
	}
	void RecalculateHp();
	void RecalculateStamina();
	bool CanBlock() const
	{
		return weapon_state == WS_TAKEN && weapon_taken == W_ONE_HANDED && HaveShield();
	}

	WeaponType GetHoldWeapon() const
	{
		switch(weapon_state)
		{
		case WS_TAKEN:
		case WS_TAKING:
			return weapon_taken;
		case WS_HIDING:
			return weapon_hiding;
		case WS_HIDDEN:
			return W_NONE;
		default:
			assert(0);
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
	MATERIAL_TYPE GetBodyMaterial() const
	{
		if(HaveArmor())
			return GetArmor().material;
		else
			return data->mat;
	}
	float GetAttackFrame(int frame) const;
	int GetRandomAttack() const;
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	Vec3 GetCenter() const
	{
		Vec3 pt = pos;
		pt.y += GetUnitHeight() / 2;
		return pt;
	}
	int FindHealingPotion() const;
	float GetAttackRange() const
	{
		return data->attack_range;
	}
	void ReequipItems();
	bool CanUseWeapons() const
	{
		return data->type >= UNIT_TYPE::HUMANOID;
	}
	bool CanUseArmor() const
	{
		return data->type == UNIT_TYPE::HUMAN;
	}


	void RemoveQuestItem(int quest_refid);
	bool HaveItem(const Item* item);
	float GetAttackSpeed(const Weapon* weapon = nullptr) const;
	float GetAttackSpeedModFromStrength(const Weapon& wep) const
	{
		int str = Get(Attribute::STR);
		if(str >= wep.req_str)
			return 0.f;
		else if(str * 2 <= wep.req_str)
			return 0.5f;
		else
			return 0.5f * float(wep.req_str - str) / (wep.req_str / 2);
	}
	float GetPowerAttackSpeed() const
	{
		if(HaveWeapon())
			return GetWeapon().GetInfo().power_speed * GetAttackSpeed();
		else
			return 0.33f;
	}
	float GetBowAttackSpeed() const;
	float GetAttackSpeedModFromStrength(const Bow& b) const
	{
		int str = Get(Attribute::STR);
		if(str >= b.req_str)
			return 0.f;
		else if(str * 2 <= b.req_str)
			return 0.75f;
		else
			return 0.75f * float(b.req_str - str) / (b.req_str / 2);
	}
	bool IsHero() const
	{
		return hero != nullptr;
	}
	bool IsFollower() const
	{
		return hero && hero->team_member;
	}
	bool IsFollowingTeamMember() const
	{
		return IsFollower() && hero->mode == HeroData::Follow;
	}
	int GetClass() const
	{
		if(IsPlayer())
			return player->clas;
		else if(IsHero())
			return hero->clas;
		else
			return Class::INVALID;
	}
	bool CanFollow() const
	{
		return IsHero() && hero->mode == HeroData::Follow && in_arena == -1 && frozen == FROZEN::NO;
	}
	bool IsTeamMember() const
	{
		if(IsPlayer())
			return true;
		else if(IsHero())
			return hero->team_member;
		else
			return false;
	}
	void MakeItemsTeam(bool team);
	void Heal(float heal)
	{
		hp += heal;
		if(hp > hpmax)
			hp = hpmax;
	}
	void NaturalHealing(int days)
	{
		Heal(0.15f * Get(Attribute::END) * days);
	}
	void HealPoison();
	// szuka przedmiotu w ekwipunku, zwraca i_index (INVALID_IINDEX jeœli nie ma takiego przedmiotu)
	static const int INVALID_IINDEX = (-SLOT_INVALID - 1);
	int FindItem(const Item* item, int quest_refid = -1) const;
	int FindQuestItem(int quest_refid) const;
	void RemoveItem(int iindex, bool active_location = true);
	int CountItem(const Item* item);
	//float CalculateBowAttackSpeed();
	cstring GetName() const
	{
		if(IsPlayer())
			return player->name.c_str();
		else if(IsHero() && hero->know_name)
			return hero->name.c_str();
		else
			return data->name.c_str();
	}
	void ClearInventory();
	bool PreferMelee()
	{
		if(IsFollower())
			return hero->melee;
		else
			return IS_SET(data->flags2, F2_MELEE);
	}
	bool IsImmortal() const
	{
		if(IS_SET(data->flags, F_IMMORTAL))
			return true;
		else if(IsPlayer())
			return player->godmode;
		else
			return false;
	}
	cstring GetRealName() const
	{
		if(IsPlayer())
			return player->name.c_str();
		else if(IsHero())
			return hero->name.c_str();
		else
			return data->name.c_str();
	}

	// szybkoœæ blokowania aktualnie u¿ywanej tarczy (im mniejsza tym lepiej)
	float GetBlockSpeed() const;

	//-----------------------------------------------------------------------------
	// EKWIPUNEK
	//-----------------------------------------------------------------------------
	const Item* slots[SLOT_MAX];
	vector<ItemSlot> items;
	int weight, weight_max;
	//-----------------------------------------------------------------------------
	bool HaveWeapon() const { return slots[SLOT_WEAPON] != nullptr; }
	bool HaveBow() const { return slots[SLOT_BOW] != nullptr; }
	bool HaveShield() const { return slots[SLOT_SHIELD] != nullptr; }
	bool HaveArmor() const { return slots[SLOT_ARMOR] != nullptr; }
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
	// wyrzuca przedmiot o podanym indeksie, zwraca czy to by³ ostatni
	bool DropItem(int index);
	// wyrzuca za³o¿ony przedmiot
	void DropItem(ITEM_SLOT slot);
	// wyrzuca kilka przedmiotów o podanym indeksie, zwraca czy to by³ ostatni (count=0 oznacza wszystko)
	bool DropItems(int index, uint count);
	// dodaje przedmiot do ekwipunku, zwraca czy siê zestackowa³
	bool AddItem(const Item* item, uint count, uint team_count);
	bool AddItem(const Item* item, uint count = 1, bool is_team = true)
	{
		return AddItem(item, count, is_team ? count : 0);
	}
	// dodaje przedmiot i zak³ada jeœli nie ma takiego typu, przedmiot jest dru¿ynowy
	void AddItemAndEquipIfNone(const Item* item, uint count = 1);
	// zwraca udŸwig postaci (0-brak obci¹¿enia, 1-maksymalne, >1 przeci¹¿ony)
	float GetLoad() const { return float(weight) / weight_max; }
	void CalculateLoad();
	bool IsOverloaded() const
	{
		return weight >= weight_max;
	}
	bool IsMaxOverloaded() const
	{
		return weight >= weight_max * 2;
	}
	LoadState GetLoadState() const
	{
		if(weight < weight_max / 4)
			return LS_NONE;
		else if(weight < weight_max / 2)
			return LS_LIGHT;
		else if(weight < weight_max * 3 / 4)
			return LS_MEDIUM;
		else if(weight < weight_max)
			return LS_HEAVY;
		else if(weight < weight_max * 2)
			return LS_OVERLOADED;
		else
			return LS_MAX_OVERLOADED;
	}
	LoadState GetArmorLoadState(const Item* armor) const;
	// zwraca wagê ekwipunku w kg
	float GetWeight() const
	{
		return float(weight) / 10;
	}
	// zwraca maksymalny udŸwig w kg
	float GetWeightMax() const
	{
		return float(weight_max) / 10;
	}
	bool CanTake(const Item* item, uint count = 1) const
	{
		assert(item && count);
		return weight + item->weight*(int)count <= weight_max;
	}
	const Item* GetIIndexItem(int i_index) const;

	Mesh::Animation* GetTakeWeaponAnimation(bool melee) const;

	bool CanDoWhileUsing() const
	{
		return action == A_ANIMATION2 && animation_state == AS_ANIMATION2_USING && IS_SET(usable->base->use_flags, BaseUsable::ALLOW_USE);
	}

	int GetBuffs() const;

	// nie sprawdza czy stoi/¿yje/czy chce gadaæ - tylko akcjê
	bool CanTalk() const
	{
		if(action == A_EAT || action == A_DRINK || auto_talk == AutoTalkMode::Leader)
			return false;
		else
			return true;
	}

	float CalculateMobility(const Armor* armor = nullptr) const;
	float GetMobilityMod(bool run) const;

	//int Get(SubSkill s) const;

	Skill GetBestWeaponSkill() const;
	Skill GetBestArmorSkill() const;

	void ApplyHumanData(HumanData& hd)
	{
		hd.Set(*human_data);
		human_data->ApplyScale(data->mesh);
	}

	int ItemsToSellWeight() const;

	void SetAnimationAtEnd(cstring anim_name = nullptr);

	float GetArrowSpeed() const
	{
		float s = (float)GetBow().speed;
		s *= 1.f + float(Get(Skill::BOW)) / 666;
		return s;
	}

	void StartAutoTalk(bool leader = false, GameDialog* dialog = nullptr);

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
	void RemoveStamina(float value);

	void CreateMesh(CREATE_MESH mode);

	void ApplyStun(float length);

	Skill GetWeaponSkill() const
	{
		if(HaveWeapon())
			return GetWeapon().GetInfo().skill;
		else
			return Skill::UNARMED;
	}

	enum PassTimeType
	{
		REST,
		TRAVEL,
		OTHER
	};
	void PassTime(int days, PassTimeType type);

	//==============================================
	// SKILLS & STATS
	//==============================================
	// Get stat
	int Get(Attribute a, StatState& state) const;
	int Get(Attribute a) const
	{
		StatState state;
		return Get(a, state);
	}
	int Get(Skill s, StatState& state) const;
	int Get(Skill s) const
	{
		StatState state;
		return Get(s, state);
	}

	// Get unmodified stats
	int GetBase(Attribute a) const
	{
		return statsx->Get(a);
	}
	int GetBase(Skill s) const
	{
		return statsx->Get(s);
	}

	// Get stat aptitude
	int GetAptitude(Attribute a) const
	{
		return statsx->attrib_apt[(int)a];
	}
	int GetAptitude(Skill s) const;
	float GetAptitudeMod(Attribute a) const
	{
		int apt = GetAptitude(a);
		return StatsX::GetModFromApt(apt);
	}
	float GetAptitudeMod(Skill s) const
	{
		int apt = GetAptitude(s);
		return StatsX::GetModFromApt(apt);
	}

	// Set stat
	void Set(Attribute a, int value)
	{
		statsx->Set(a, value);
		ApplyStat(a);
	}
	void Set(Skill s, int value)
	{
		statsx->Set(s, value);
		ApplyStat(s);
	}

	// Set base stat
	void SetBase(Attribute attrib, int value, bool startup, bool mod);
	void SetBase(Skill skill, int value, bool startup, bool mod);

	// Apply other stats based on skill/attribute (hp/atk/def)
	void ApplyStat(Attribute a);
	void ApplyStat(Skill s);

	// Calculate hp/stamina
	void CalculateStats(bool initial = false);

	// Various other stats
	float GetNaturalHealingMod() { return GetEffectModMultiply(EffectType::NaturalHealingMod); }
	float CalculateMagicResistance() const;
	int CalculateMagicPower() const;
	float GetPowerAttackMod() const;
	bool HavePoisonResistance() const;
	int GetCriticalChance(const Item* item, bool backstab, float ratio) const;
	float GetCriticalDamage(const Item* item) const;

	// Perk functions
	bool HavePerk(Perk perk, int value = -1) const;
	int GetPerkIndex(Perk perk);
	void AddPerk(Perk perk, int value);
	void RemovePerk(int index);

	//==============================================
	// ACTIVE EFFECTS
	//==============================================
	bool HaveEffect(EffectType effect) const;
	bool HaveEffect(int netid) const;
	float GetEffectSum(EffectType effect) const;
	float GetEffectSum(EffectType effect, int value, StatState& state) const;
	bool GetEffectModMultiply(EffectType effect, float& value) const;
	float GetEffectModMultiply(EffectType effect) const { float value = 1.f; GetEffectModMultiply(effect, value); return value; }
	Effect* FindEffect(EffectType effect);
	bool FindEffect(EffectType effect, float* value);
	void RemovePerkEffects(Perk perk);
	void RemoveEffect(const Effect* effect, bool notify);
	void AddEffect(Effect* e, bool update = false);
	void AddOrUpdateEffect(Effect* e);
	void RemoveEffect(EffectType effect);
	bool RemoveEffect(int netid);
	void RemoveEffects(EffectType effect, EffectSource source, Perk source_id);
	void RemoveEffects(bool notify = true);
	const EffectVector& GetEffects() const { return effects; }
	void WriteEffects(BitStream& stream);
	bool ReadEffects(BitStream& stream);
	void AddObservableEffect(EffectType effect, int netid, float time, bool update);
	bool RemoveObservableEffect(int netid);
	void WriteObservableEffects(BitStream& stream);
	bool ReadObservableEffects(BitStream& stream);
	void UpdateEffects(float dt);
	void EffectStatUpdate(const Effect& e);
	void EndEffects();
	void EndLongEffects(int days);

	//-----------------------------------------------------------------------------
	static vector<Unit*> refid_table;
	static vector<std::pair<Unit**, int>> refid_request;

	static Unit* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}
	static void AddRequest(Unit** unit, int refid)
	{
		assert(unit && refid != -1);
		refid_request.push_back(std::pair<Unit**, int>(unit, refid));
	}
	static void AddRefid(Unit* unit)
	{
		assert(unit);
		unit->refid = (int)refid_table.size();
		refid_table.push_back(unit);
	}
};

//-----------------------------------------------------------------------------
struct NAMES
{
	// punkty
	static cstring point_weapon;
	static cstring point_hidden_weapon;
	static cstring point_shield;
	static cstring point_shield_hidden;
	static cstring point_bow;
	static cstring point_cast;
	static cstring points[];
	static uint n_points;

	// animacje
	static cstring ani_take_weapon;
	static cstring ani_take_weapon_no_shield;
	static cstring ani_take_bow;
	static cstring ani_move;
	static cstring ani_run;
	static cstring ani_left;
	static cstring ani_right;
	static cstring ani_stand;
	static cstring ani_battle;
	static cstring ani_battle_bow;
	static cstring ani_die;
	static cstring ani_hurt;
	static cstring ani_shoot;
	static cstring ani_block;
	static cstring ani_bash;
	static cstring ani_base[];
	static cstring ani_humanoid[];
	static cstring ani_attacks[];
	static uint n_ani_base;
	static uint n_ani_humanoid;
	static int max_attacks;
};
