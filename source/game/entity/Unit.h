#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"
#include "UnitData.h"
#include "Class.h"
#include "Animesh.h"
#include "HumanData.h"
#include "HeroData.h"
#include "PlayerController.h"
#include "Useable.h"
#include "Effect.h"

//-----------------------------------------------------------------------------
struct PlayerController;
struct AIController;
struct Useable;
struct EntityInterpolator;
struct UnitEventHandler;
struct SpeechBubble;
struct GameDialog;

//-----------------------------------------------------------------------------
enum Animation
{
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
	// etapy: 0 - naci�ga, 1 - naci�ga ale mo�e strzela�, 2 - po strzale, 3 - wyj�� now� strza��
	A_SHOOT,
	// etapy: 0 - przygotowanie, 1 - mo�e atakowa�, 2 - po trafieniu
	A_ATTACK,
	A_BLOCK,
	A_BASH,
	A_DRINK, // picie napoju (0 - zaczyna pi�, 1-efekt wypicia, 2-schowa� przedmiot)
	A_EAT, // jedzenie (0 - zaczyna je��, 1-odtwarza d�wi�k, 2-efekt, 3-chowa)
	A_PAIN,
	A_CAST,
	A_ANIMATION, // animacja bez obiektu (np drapani si�, rozgl�danie)
	A_ANIMATION2, // u�ywanie obiektu (0-podchodzi, 1-u�ywa, 2-u�ywa d�wi�k, 3-odchodzi)
	A_POSITION, // u�ywa� czego� ale dosta� basha lub umar�, trzeba go przesun�� w normalne miejsce
	//A_PAROWANIE
	A_PICKUP, // p�ki co dzia�a jak animacja, potem doda si� punkt podnoszenia
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
	return a == A_ANIMATION || a == A_PICKUP;
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
struct Effect
{
	ConsumeEffect effect;
	float time, power;
};

//-----------------------------------------------------------------------------
// MP obs�uguje max 8 buff�w
enum BUFF_FLAGS
{
	BUFF_REGENERATION = 1<<0,
	BUFF_NATURAL = 1<<1,
	BUFF_FOOD = 1<<2,
	BUFF_ALCOHOL = 1<<3,
	BUFF_POISON = 1<<4,
	BUFF_ANTIMAGIC = 1<<5,
};

//-----------------------------------------------------------------------------
/*struct Effect2
{
	EffectType type;
	int a, b;
};*/

enum class AutoTalkMode
{
	No,
	Yes,
	Wait,
	Leader
};

//-----------------------------------------------------------------------------
// jednostka w grze
struct Unit
{
	enum Type
	{
		ANIMAL,
		HUMANOID,
		HUMAN
	};

	enum LiveState
	{
		ALIVE,
		FALLING,
		FALL,
		DYING,
		DEAD,
		LIVESTATE_MAX
	};

	static const int MIN_SIZE = 36;
	static const float AUTO_TALK_WAIT;

	AnimeshInstance* ani;
	Animation animation, current_animation;
	Human* human_data;
	LiveState live_state;
	VEC3 pos; // pozycja postaci
	VEC3 visual_pos; // graficzna pozycja postaci, u�ywana w MP
	VEC3 prev_pos, target_pos, target_pos2;
	float rot, prev_speed, hp, hpmax, speed, hurt_timer, talk_timer, timer, use_rot, attack_power, last_bash, alcohol, raise_timer;
	Type type;
	int animation_state, level, gold, attack_id, refid, in_building, frozen, in_arena, quest_refid;
	ACTION action;
	WeaponType weapon_taken, weapon_hiding;
	WeaponState weapon_state;
	AnimeshInstance* bow_instance;
	UnitData* data;
	PlayerController* player;
	const Item* used_item;
	bool used_item_is_team;
	vector<Effect> effects;
	bool hitted, invisible, talking, run_attack, to_remove, temporary, changed, dont_attack, assist, attack_team, fake_unit;
	AIController* ai;
	btCollisionObject* cobj;
	static vector<Unit*> refid_table;
	static vector<std::pair<Unit**, int>> refid_request;
	Useable* useable;
	HeroData* hero;
	UnitEventHandler* event_handler;
	SpeechBubble* bubble;
	Unit* look_target, *guard_target;
	int netid;
	int ai_mode; // u klienta w MP (0x01-dont_attack, 0x02-assist, 0x04-not_idle)
	enum Busy
	{
		Busy_No,
		Busy_Yes,
		Busy_Talking,
		Busy_Looted,
		Busy_Trading,
		Busy_Tournament
	} busy; // nie zapisywane, powinno by� Busy_No
	EntityInterpolator* interp;
	UnitStats stats, unmod_stats;
	//vector<Effect2> effects2;
	AutoTalkMode auto_talk;
	float auto_talk_timer;
	GameDialog* auto_talk_dialog;

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	Unit() : ani(nullptr), hero(nullptr), ai(nullptr), player(nullptr), cobj(nullptr), interp(nullptr), bow_instance(nullptr), fake_unit(false),
		human_data(nullptr) {}
	~Unit();

	float CalculateArmorDefense(const Armor* armor=nullptr);
	float CalculateDexterityDefense(const Armor* armor=nullptr);
	float CalculateBaseDefense() const;
// 	float CalculateArmor(float& def_natural, float& def_dex, float& def_armor);

	float CalculateAttack() const;
	float CalculateAttack(const Item* weapon) const;
	float CalculateBlock(const Item* shield) const;
	float CalculateDefense() const;
	float CalculateDefense(const Item* armor) const;
	// czy �yje i nie le�y na ziemi
	bool IsStanding() const { return live_state == ALIVE; }
	// czy �yje
	bool IsAlive() const { return live_state < DYING; }
	void RecalculateWeight();
	// konsumuje przedmiot (zwraca 0-u�yto ostatni, 1-u�yto nie ostatni, 2-chowa bro�, 3-zaj�ty)
	int ConsumeItem(int index);
	// u�ywa przedmiotu, nie mo�e nic robi� w tej chwili i musi mie� schowan� bro�
	void ConsumeItem(const Consumable& item, bool force=false, bool send=true);
	void HideWeapon();
	void TakeWeapon(WeaponType type);
	// dodaj efekt zjadanego przedmiotu
	void ApplyConsumableEffect(const Consumable& item);
	// aktualizuj efekty
	void UpdateEffects(float dt);
	// zako�cz tymczasowe efekty po opuszczeniu lokacji
	void EndEffects(int days=0, int* best_nat=nullptr);
	float GetSphereRadius() const
	{
		float radius = ani->ani->head.radius;
		if(type == HUMAN)
			radius *= ((human_data->height-1)*0.2f+1.f);
		return radius;
	}
	float GetUnitRadius() const
	{
		if(type == HUMAN)
			return 0.3f * ((human_data->height-1)*0.2f+1.f);
		else
			return data->width;
	}
	VEC3 GetColliderPos() const
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
		if(type == HUMAN)
			return 1.73f * ((human_data->height-1)*0.2f+1.f);
		else
			return ani->ani->head.bbox.SizeY();
	}
	VEC3 GetHeadPoint() const
	{
		VEC3 pt = visual_pos;
		pt.y += GetUnitHeight() * 1.1f;
		return pt;
	}
	VEC3 GetHeadSoundPos() const
	{
		VEC3 pt = visual_pos;
		pt.y += GetUnitHeight() * 0.9f;
		return pt;
	}
	VEC3 GetUnitTextPos() const
	{
		VEC3 pt;
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
	VEC3 GetEyePos() const;
	float CalculateMaxHp() const;
	float GetHpp() const { return hp/hpmax; }
	void GetBox(BOX& box) const;
	int GetDmgType() const;
	bool IsNotFighting() const
	{
		if(type == ANIMAL)
			return false;
		else
			return (weapon_state == WS_HIDDEN);
	}
	VEC3 GetLootCenter() const;

	
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
		return data->rot_speed * (0.6f + 1.f/150*CalculateMobility()) * GetWalkLoad() * GetArmorMovement();
	}
	float GetWalkSpeed() const
	{
		return data->walk_speed * (0.6f + 1.f / 150 * CalculateMobility()) * GetWalkLoad() * GetArmorMovement();
	}
	float GetRunSpeed() const
	{
		return data->run_speed * (0.6f + 1.f / 150 * CalculateMobility()) * GetRunLoad() * GetArmorMovement();
	}
	bool CanRun() const
	{
		if(IS_SET(data->flags, F_SLOW) || action == A_BLOCK || action == A_BASH || (action == A_ATTACK && !run_attack) || action == A_SHOOT)
			return false;
		else
			return !IsOverloaded();
	}
	void RecalculateHp();
	bool CanBlock() const
	{
		return weapon_state == WS_TAKEN && weapon_taken == W_ONE_HANDED && HaveShield();
	}
	float CalculateShieldAttack() const;
	
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
	float GetArmorMovement() const
	{
		if(!HaveArmor())
			return 1.f;
		else
		{
			switch(GetArmor().skill)
			{
			case Skill::LIGHT_ARMOR:
			default:
				return 1.f + 1.f / 300 * Get(Skill::LIGHT_ARMOR);
			case Skill::MEDIUM_ARMOR:
				return 1.f + 1.f / 450 * Get(Skill::MEDIUM_ARMOR);
			case Skill::HEAVY_ARMOR:
				return 1.f + 1.f / 600 * Get(Skill::HEAVY_ARMOR);
			}
		}
	}
	VEC3 GetFrontPos() const
	{
		return VEC3(
			pos.x + sin(rot+PI) * 2,
			pos.y,
			pos.z + cos(rot+PI) * 2);
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
	bool FindEffect(ConsumeEffect effect, float* value);
	VEC3 GetCenter() const
	{
		VEC3 pt = pos;
		pt.y += GetUnitHeight()/2;
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
		return type >= HUMANOID;
	}
	bool CanUseArmor() const
	{
		return type == HUMAN;
	}
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
	void RemoveQuestItem(int quest_refid);
	bool HaveItem(const Item* item);
	float GetAttackSpeed(const Weapon* weapon=nullptr) const;
	float GetAttackSpeedModFromStrength(const Weapon& wep) const
	{
		int str = Get(Attribute::STR);
		if(str >= wep.req_str)
			return 0.f;
		else if(str * 2 <= wep.req_str)
			return 0.75f;
		else
			return 0.75f * float(wep.req_str - str) / (wep.req_str / 2);
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
		else if(str*2 <= b.req_str)
			return 0.75f;
		else
			return 0.75f * float(b.req_str-str)/(b.req_str/2);
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
	Class GetClass() const
	{
		if(IsPlayer())
			return player->clas;
		else if(IsHero())
			return hero->clas;
		else
		{
			assert(0);
			return Class::WARRIOR;
		}
	}
	bool CanFollow() const
	{
		return IsHero() && hero->mode == HeroData::Follow && in_arena == -1 && frozen == 0;
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
	void RemovePoison();
	// szuka przedmiotu w ekwipunku, zwraca i_index (INVALID_IINDEX je�li nie ma takiego przedmiotu)
#define INVALID_IINDEX (-SLOT_INVALID-1)
	int FindItem(const Item* item, int quest_refid=-1) const;
	int FindQuestItem(int quest_refid) const;
	void RemoveItem(int iindex, bool active_location=true);
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

	// szybko�� blokowania aktualnie u�ywanej tarczy (im mniejsza tym lepiej)
	float GetBlockSpeed() const;

	float CalculateWeaponBlock() const;
	float CalculateMagicResistance() const;
	int CalculateMagicPower() const;
	bool HaveEffect(ConsumeEffect effect) const;
	
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
	// wyrzuca przedmiot o podanym indeksie, zwraca czy to by� ostatni
	bool DropItem(int index);
	// wyrzuca za�o�ony przedmiot
	void DropItem(ITEM_SLOT slot);
	// wyrzuca kilka przedmiot�w o podanym indeksie, zwraca czy to by� ostatni (count=0 oznacza wszystko)
	bool DropItems(int index, uint count);
	// dodaje przedmiot do ekwipunku, zwraca czy si� zestackowa�
	bool AddItem(const Item* item, uint count, uint team_count);
	bool AddItem(const Item* item, uint count=1, bool is_team=true)
	{
		return AddItem(item, count, is_team ? count : 0);
	}
	// dodaje przedmiot i zak�ada je�li nie ma takiego typu, przedmiot jest dru�ynowy
	void AddItemAndEquipIfNone(const Item* item, uint count=1);
	// zwraca ud�wig postaci (0-brak obci��enia, 1-maksymalne, >1 przeci��ony)
	float GetLoad() const { return float(weight)/weight_max; }
	void CalculateLoad() { weight_max = Get(Attribute::STR)*15; }
	bool IsOverloaded() const
	{
		return weight > weight_max;
	}
	bool IsMaxOverloaded() const
	{
		return weight > weight_max*2;
	}
	int GetLoadState() const
	{
		if(weight < weight/4)
			return 0;
		else if(weight < weight/2)
			return 1;
		else if(weight < weight*3/2)
			return 2;
		else if(weight < weight_max)
			return 3;
		else if(weight < weight_max*2)
			return 4;
		else
			return 5;
	}
	float GetRunLoad() const
	{
		switch(GetLoadState())
		{
		case 0:
			return 1.f;
		case 1:
			return lerp(1.f, 0.95f, float(weight-weight/4)/(weight/2-weight/4));
		case 2:
			return lerp(0.95f, 0.85f, float(weight-weight/2)/(weight*3/2-weight/2));
		case 3:
			return lerp(0.85f, 0.7f, float(weight-weight*3/2)/(weight_max-weight*3/2));
		case 4:
		case 5:
			return 0.f;
		default:
			assert(0);
			return 0.f;
		}
	}
	float GetWalkLoad() const
	{
		switch(GetLoadState())
		{
		case 0:
		case 1:
		case 2:
			return 1.f;
		case 3:
			return lerp(1.f, 0.9f, float(weight-weight*3/2)/(weight_max-weight*3/2));
		case 4:
			return lerp(0.9f, 0.f, float(weight-weight_max)/weight_max);
		case 5:
			return 0.f;
		default:
			assert(0);
			return 0.f;
		}
	}
	float GetAttackSpeedModFromLoad() const
	{
		switch(GetLoadState())
		{
		case 0:
		case 1:
		case 2:
			return 0.f;
		case 3:
			return lerp(0.f, 0.1f, float(weight-weight*3/2)/(weight_max-weight*3/2));
		case 4:
			return lerp(0.1f, 0.25f, float(weight-weight_max)/weight_max);
		case 5:
			return 0.25f;
		default:
			assert(0);
			return 0.25f;
		}
	}
	// zwraca wag� ekwipunku w kg
	float GetWeight() const
	{
		return float(weight)/10;
	}
	// zwraca maksymalny ud�wig w kg
	float GetWeightMax() const
	{
		return float(weight_max)/10;
	}
	bool CanTake(const Item* item, uint count=1) const
	{
		assert(item && count);
		return weight + item->weight*(int)count <= weight_max;
	}
	const Item* GetIIndexItem( int i_index ) const;

	Animesh::Animation* GetTakeWeaponAnimation(bool melee) const;

	bool CanDoWhileUsing() const
	{
		return action == A_ANIMATION2 && animation_state == AS_ANIMATION2_USING && g_base_usables[useable->type].allow_use;
	}

	int GetBuffs() const;

	// nie sprawdza czy stoi/�yje/czy chce gada� - tylko akcj�
	bool CanTalk() const
	{
		if(action == A_EAT || action == A_DRINK || auto_talk == AutoTalkMode::Leader)
			return false;
		else
			return true;
	}

	int CalculateLevel();
	int CalculateLevel(Class clas);

	int Get(Attribute a) const
	{
		return stats.attrib[(int)a];
	}

	int Get(Skill s) const
	{
		return stats.skill[(int)s];
	}

	// change unmod stat
	void Set(Attribute a, int value)
	{
		//int dif = value - unmod_stats.attrib[(int)a];
		unmod_stats.attrib[(int)a] = value;
		RecalculateStat(a, true);
	}
	void Set(Skill s, int value)
	{
		//int dif = value - unmod_stats.skill[(int)s];
		unmod_stats.skill[(int)s] = value;
		RecalculateStat(s, true);
	}

	int GetUnmod(Attribute a) const
	{
		return unmod_stats.attrib[(int)a];
	}
	int GetUnmod(Skill s) const
	{
		return unmod_stats.skill[(int)s];
	}

	void CalculateStats();

	int GetEffectModifier(EffectType type, int id, StatState* state) const;

	int CalculateMobility() const;
	int CalculateMobility(const Armor& armor) const;

	int Get(SubSkill s) const;

	Skill GetBestWeaponSkill() const;
	Skill GetBestArmorSkill() const;

	void RecalculateStat(Attribute a, bool apply);
	void RecalculateStat(Skill s, bool apply);
	void ApplyStat(Attribute a, int old, bool calculate_skill);

	void ApplyHumanData(HumanData& hd)
	{
		hd.Set(*human_data);
		human_data->ApplyScale(ani->ani);
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
