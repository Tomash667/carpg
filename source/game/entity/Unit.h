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

//-----------------------------------------------------------------------------
struct PlayerController;
struct AIController;
struct Useable;
struct EntityInterpolator;
struct UnitEventHandler;
struct SpeechBubble;

//-----------------------------------------------------------------------------
enum ANIMACJA
{
	ANI_IDZIE,
	ANI_IDZIE_TYL,
	ANI_BIEGNIE,
	ANI_W_LEWO,
	ANI_W_PRAWO,
	ANI_STOI,
	ANI_BOJOWA,
	ANI_BOJOWA_LUK,
	ANI_UMIERA,
	ANI_ODTWORZ,
	ANI_KLEKA,
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

inline bool IsBlocking(ACTION a)
{
	return a == A_ANIMATION || a == A_PICKUP;
}

//-----------------------------------------------------------------------------
enum WYJETA_BRON
{
	BRON_SCHOWANA,
	BRON_CHOWA,
	BRON_WYJMUJE,
	BRON_WYJETA
};

//-----------------------------------------------------------------------------
struct Effect
{
	ConsumeEffect effect;
	float time, power;
};

//-----------------------------------------------------------------------------
struct UnitEventHandler
{
	enum TYPE
	{
		LEAVE,
		DIE,
		FALL,
		SPAWN
	};

	virtual void HandleUnitEvent(TYPE type, Unit* unit) = 0;
	virtual int GetUnitEventHandlerQuestRefid() = 0;
};

//-----------------------------------------------------------------------------
enum STAT_STATE
{
	SS_NORMAL,
	SS_INCRASED,
	SS_DECRASED,
	SS_BOTH
};

//-----------------------------------------------------------------------------
// MP obs�uguje max 8 buff�w
enum BUFF_FLAGS
{
	BUFF_REGENERATION = 1<<0,
	BUFF_NATURAL = 1<<1,
	BUFF_FOOD = 1<<2,
	BUFF_ALCOHOL = 1<<3,
	BUFF_POISON = 1<<4
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
		DEAD
	};

	AnimeshInstance* ani;
	ANIMACJA animacja, animacja2;
	Human* human_data;
	LiveState live_state;
	VEC3 pos; // pozycja postaci
	VEC3 visual_pos; // graficzna pozycja postaci, u�ywana w MP
	VEC3 prev_pos, target_pos, target_pos2;
	float rot, prev_speed, hp, hpmax, speed, hurt_timer, talk_timer, timer, use_rot, attack_power, last_bash, auto_talk_timer, alcohol, raise_timer;
	Type type;
	int etap_animacji, level, attrib[(int)Attribute::MAX], skill[S_MAX], gold, attack_id, refid, in_building, frozen, in_arena, quest_refid, auto_talk; // 0-nie, 1-czekaj, 2-tak
	ACTION action;
	BRON wyjeta, chowana;
	WYJETA_BRON stan_broni;
	AnimeshInstance* bow_instance;
	UnitData* data;
	PlayerController* player;
	const Item* used_item;
	bool used_item_is_team;
	vector<Effect> effects;
	bool trafil, invisible, talking, atak_w_biegu, to_remove, temporary, changed, dont_attack, assist, attack_team;
	AIController* ai;
	btCollisionObject* cobj;
	static vector<Unit*> refid_table;
	static vector<std::pair<Unit**, int> > refid_request;
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

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	Unit() : ani(NULL), hero(NULL), ai(NULL), player(NULL), cobj(NULL), interp(NULL), bow_instance(NULL) {}
	~Unit();

 	float CalculateArmorDefense(const Armor* armor=NULL);
	float CalculateDexterityDefense(const Armor* armor=NULL);
	float CalculateBaseDefense();
// 	float CalculateArmor(float& def_natural, float& def_dex, float& def_armor);

	float CalculateAttack() const;
	float CalculateAttack(const Item* weapon) const;
	float CalculateBlock(const Item* shield) const;
	float CalculateDefense() const;
	float CalculateDefense(const Item* armor) const;
	float CalculateDexterity() const;
	float CalculateDexterity(const Armor& armor) const;
	// czy �yje i nie le�y na ziemi
	inline bool IsStanding() const { return live_state == ALIVE; }
	// czy �yje
	inline bool IsAlive() const { return live_state < DYING; }
	void RecalculateWeight();
	// konsumuje przedmiot (zwraca 0-u�yto ostatni, 1-u�yto nie ostatni, 2-chowa bro�, 3-zaj�ty)
	int ConsumeItem(int index);
	// u�ywa przedmiotu, nie mo�e nic robi� w tej chwili i musi mie� schowan� bro�
	void ConsumeItem(const Consumeable& item, bool force=false, bool send=true);
	void HideWeapon();
	void TakeWeapon(BRON type);
	// dodaj efekt zjadanego przedmiotu
	void ApplyConsumeableEffect(const Consumeable& item);
	// aktualizuj efekty
	void UpdateEffects(float dt);
	// zako�cz tymczasowe efekty po opuszczeniu lokacji
	void EndEffects(int days=0, int* best_nat=NULL);
	inline float GetSphereRadius() const
	{
		float radius = ani->ani->head.radius;
		if(type == HUMAN)
			radius *= ((human_data->height-1)*0.2f+1.f);
		return radius;
	}
	inline float GetUnitRadius() const
	{
		if(type == HUMAN)
			return 0.3f * ((human_data->height-1)*0.2f+1.f);
		else
			return data->width;
	}
	inline VEC3 GetColliderPos() const
	{
		if(action != A_ANIMATION2)
			return pos;
		else if(etap_animacji == 3)
			return target_pos;
		else
			return target_pos2;
	}
	inline float GetUnitHeight() const
	{
		if(type == HUMAN)
			return 1.73f * ((human_data->height-1)*0.2f+1.f);
		else
			return ani->ani->head.bbox.SizeY();
	}
	inline VEC3 GetHeadPoint() const
	{
		VEC3 pt = visual_pos;
		pt.y += GetUnitHeight() * 1.1f;
		return pt;
	}
	inline VEC3 GetHeadSoundPos() const
	{
		VEC3 pt = visual_pos;
		pt.y += GetUnitHeight() * 0.9f;
		return pt;
	}
	inline VEC3 GetUnitTextPos() const
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
	void CalculateLevel();
	float CalculateMaxHp() const;
	inline float GetHpp() const { return hp/hpmax; }
	void GetBox(BOX& box) const;
	int GetDmgType() const;
	inline bool IsNotFighting() const
	{
		if(type == ANIMAL)
			return false;
		else
			return (stan_broni == BRON_SCHOWANA);
	}
	VEC3 GetLootCenter() const;

	
	float CalculateWeaponPros(const Weapon& weapon) const;
	bool IsBetterWeapon(const Weapon& weapon) const;
	bool IsBetterWeapon(const Weapon& weapon, int* value) const;
	bool IsBetterArmor(const Armor& armor) const;
	bool IsBetterArmor(const Armor& armor, int* value) const;
	bool IsBetterItem(const Item* item) const;
	inline bool IsPlayer() const
	{
		return (player != NULL);
	}
	inline bool IsAI() const
	{
		return !IsPlayer();
	}
	inline float GetRotationSpeed() const
	{
		return data->rot_speed * (0.6f + CalculateDexterity() / 150) * GetWalkLoad() * GetArmorMovement();
	}
	inline float GetWalkSpeed() const
	{
		return data->walk_speed * (0.6f + CalculateDexterity() / 150) * GetWalkLoad() * GetArmorMovement();
	}
	inline float GetRunSpeed() const
	{
		return data->run_speed * (0.6f + CalculateDexterity() / 150) * GetRunLoad() * GetArmorMovement();
	}
	inline bool CanRun() const
	{
		if(IS_SET(data->flagi, F_POWOLNY) || action == A_BLOCK || action == A_BASH || (action == A_ATTACK && !atak_w_biegu) || action == A_SHOOT)
			return false;
		else
			return !IsOverloaded();
	}
	void RecalculateHp();
	inline bool CanBlock() const
	{
		return stan_broni == BRON_WYJETA && wyjeta == B_JEDNORECZNA && HaveShield();
	}
	float CalculateShieldAttack() const;
	
	inline BRON GetHoldWeapon() const
	{
		switch(stan_broni)
		{
		case BRON_WYJETA:
		case BRON_WYJMUJE:
			return wyjeta;
		case BRON_CHOWA:
			return chowana;
		case BRON_SCHOWANA:
			return B_BRAK;
		default:
			assert(0);
			return B_BRAK;
		}
	}
	inline bool IsHoldingMeeleWeapon() const
	{
		return GetHoldWeapon() == B_JEDNORECZNA;
	}
	inline bool IsHoldingBow() const
	{
		return GetHoldWeapon() == B_LUK;
	}
	inline float GetArmorMovement() const
	{
		if(!HaveArmor())
			return 1.f;
		else
		{
			if(GetArmor().IsHeavy())
				return 1.f + float(skill[S_HEAVY_ARMOR])/600;
			else
				return 1.f + float(skill[S_LIGHT_ARMOR])/300;
		}
	}
	inline VEC3 GetFrontPos() const
	{
		return VEC3(
			pos.x + sin(rot+PI) * 2,
			pos.y,
			pos.z + cos(rot+PI) * 2);
	}
	inline MATERIAL_TYPE GetWeaponMaterial() const
	{
		if(HaveWeapon())
			return GetWeapon().material;
		else
			return data->mat;
	}
	inline MATERIAL_TYPE GetBodyMaterial() const
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
	inline VEC3 GetCenter() const
	{
		VEC3 pt = pos;
		pt.y += GetUnitHeight()/2;
		return pt;
	}
	int FindHealingPotion() const;
	inline float GetAttackRange() const
	{
		return data->attack_range;
	}
	void ReequipItems();
	inline bool CanUseWeapons() const
	{
		return type >= HUMANOID;
	}
	inline bool CanUseArmor() const
	{
		return type == HUMAN;
	}
	static inline Unit* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return NULL;
		else
			return refid_table[_refid];
	}
	static inline void AddRequest(Unit** unit, int refid)
	{
		assert(unit && refid != -1);
		refid_request.push_back(std::pair<Unit**, int>(unit, refid));
	}
	static inline void AddRefid(Unit* unit)
	{
		assert(unit);
		unit->refid = (int)refid_table.size();
		refid_table.push_back(unit);
	}
	void RemoveQuestItem(int refid);
	bool HaveItem(const Item* item);
	float GetLevel(TrainWhat src);
	float GetAttackSpeed(const Weapon* weapon=NULL) const;
	inline float GetAttackSpeedModFromStrength(const Weapon& wep) const
	{
		int str = attrib[(int)Attribute::STR];
		if(str >= wep.sila)
			return 0.f;
		else if(str * 2 <= wep.sila)
			return 0.75f;
		else
			return 0.75f * float(wep.sila - str) / (wep.sila / 2);
	}
	inline float GetPowerAttackSpeed() const
	{
		if(HaveWeapon())
			return GetWeapon().GetInfo().power_speed * GetAttackSpeed();
		else
			return 0.33f;
	}
	float GetBowAttackSpeed() const;
	inline float GetAttackSpeedModFromStrength(const Bow& b) const
	{
		int str = attrib[(int)Attribute::STR];
		if(str >=b.sila)
			return 0.f;
		else if(str*2 <= b.sila)
			return 0.75f;
		else
			return 0.75f * float(b.sila-str)/(b.sila/2);
	}
	inline bool IsHero() const
	{
		return hero != NULL;
	}
	inline bool IsFollower() const
	{
		return hero && hero->team_member;
	}
	inline bool IsFollowingTeamMember() const
	{
		return IsFollower() && hero->mode == HeroData::Follow;
	}
	inline CLASS GetClass() const
	{
		if(IsPlayer())
			return player->clas;
		else if(IsHero())
			return hero->clas;
		else
		{
			assert(0);
			return WARRIOR;
		}
	}
	bool CanFollow() const
	{
		return IsHero() && hero->mode == HeroData::Follow && in_arena == -1 && frozen == 0;
	}
	inline bool IsTeamMember() const
	{
		if(IsPlayer())
			return true;
		else if(IsHero())
			return hero->team_member;
		else
			return false;
	}
	void MakeItemsTeam(bool team);
	inline void Heal(float heal)
	{
		hp += heal;
		if(hp > hpmax)
			hp = hpmax;
	}
	inline void NaturalHealing(int days)
	{
		Heal(0.15f * attrib[(int)Attribute::CON] * days);
	}
	void HealPoison();
	void RemovePoison();
	// szuka przedmiotu w ekwipunku, zwraca i_index (INVALID_IINDEX je�li nie ma takiego przedmiotu)
#define INVALID_IINDEX (-SLOT_INVALID-1)
	int FindItem(const Item* item, int refid=-1) const;
	bool RemoveItem(const Item* item);
	int CountItem(const Item* item);
	//float CalculateBowAttackSpeed();
	inline cstring GetName() const
	{
		if(IsPlayer())
			return player->name.c_str();
		else if(IsHero() && hero->know_name)
			return hero->name.c_str();
		else
			return data->name;
	}
	void ClearInventory();
	inline bool PreferMelee()
	{
		if(IsFollower())
			return hero->melee;
		else
			return IS_SET(data->flagi2, F2_WALKA_WRECZ);
	}
	inline bool IsImmortal() const
	{
		if(IS_SET(data->flagi, F_NIESMIERTELNY))
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
			return data->name;
	}

	// szybko�� blokowania aktualnie u�ywanej tarczy (im mniejsza tym lepiej)
	float GetBlockSpeed() const;

	float CalculateWeaponBlock() const;
	float CalculateMagicResistance() const;
	int CalculateMagicPower() const;
	bool HaveEffect(ConsumeEffect effect) const;

	//-----------------------------------------------------------------------------
	// STATYSTYKI
	//-----------------------------------------------------------------------------
	inline int GetAttribute(int index) const { return attrib[index]; }
	inline int GetBaseAttribute(int index) const { return attrib[index]; }
	inline STAT_STATE GetAttributeState(int index) const { return SS_NORMAL; }
	inline int GetSkill(int index) const { return skill[index]; }
	inline int GetBaseSkill(int index) const { return skill[index]; }
	inline STAT_STATE GetSkillState(int index) const { return SS_NORMAL; }

	//-----------------------------------------------------------------------------
	// EKWIPUNEK
	//-----------------------------------------------------------------------------
	const Item* slots[SLOT_MAX];
	vector<ItemSlot> items;
	int weight, weight_max;
	//-----------------------------------------------------------------------------
	inline bool HaveWeapon() const { return slots[SLOT_WEAPON] != NULL; }
	inline bool HaveBow() const { return slots[SLOT_BOW] != NULL; }
	inline bool HaveShield() const { return slots[SLOT_SHIELD] != NULL; }
	inline bool HaveArmor() const { return slots[SLOT_ARMOR] != NULL; }
	inline const Weapon& GetWeapon() const
	{
		assert(HaveWeapon());
		return slots[SLOT_WEAPON]->ToWeapon();
	}
	inline const Bow& GetBow() const
	{
		assert(HaveBow());
		return slots[SLOT_BOW]->ToBow();
	}
	inline const Shield& GetShield() const
	{
		assert(HaveShield());
		return slots[SLOT_SHIELD]->ToShield();
	}
	inline const Armor& GetArmor() const
	{
		assert(HaveArmor());
		return slots[SLOT_ARMOR]->ToArmor();
	}
	inline const Armor* GetArmorPtr() const
	{
		if(HaveArmor())
			return slots[SLOT_ARMOR]->ToArmorPtr();
		else
			return NULL;
	}
	// wyrzuca przedmiot o podanym indeksie, zwraca czy to by� ostatni
	bool DropItem(int index);
	// wyrzuca za�o�ony przedmiot
	void DropItem(ITEM_SLOT slot);
	// wyrzuca kilka przedmiot�w o podanym indeksie, zwraca czy to by� ostatni (count=0 oznacza wszystko)
	bool DropItems(int index, uint count);
	// dodaje przedmiot do ekwipunku, zwraca czy si� zestackowa�
	bool AddItem(const Item* item, uint count, uint team_count);
	inline bool AddItem(const Item* item, uint count=1, bool is_team=true)
	{
		return AddItem(item, count, is_team ? count : 0);
	}
	bool AddItem(const Item*,bool); // ta funkcja jest poto �eby wykry� z�e wywo�anie AddItem
	// dodaje przedmiot i zak�ada je�li nie ma takiego typu, przedmiot jest dru�ynowy
	void AddItemAndEquipIfNone(const Item* item, uint count=1);
	// zwraca ud�wig postaci (0-brak obci��enia, 1-maksymalne, >1 przeci��ony)
	inline float GetLoad() const { return float(weight)/weight_max; }
	void CalculateLoad() { weight_max = attrib[(int)Attribute::STR]*15; }
	inline bool IsOverloaded() const
	{
		return weight > weight_max;
	}
	inline bool IsMaxOverloaded() const
	{
		return weight > weight_max*2;
	}
	inline int GetLoadState() const
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
	inline float GetRunLoad() const
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
	inline float GetWalkLoad() const
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
	inline float GetAttackSpeedModFromLoad() const
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
	inline float GetWeight() const
	{
		return float(weight)/10;
	}
	// zwraca maksymalny ud�wig w kg
	inline float GetWeightMax() const
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

	inline bool CanDoWhileUsing() const
	{
		return action == A_ANIMATION2 && etap_animacji == 1 && g_base_usables[useable->type].allow_use;
	}

	int GetBuffs() const;

	// nie sprawdza czy stoi/�yje/czy chce gada� - tylko akcj�
	inline bool CanTalk() const
	{
		if(action == A_EAT || action == A_DRINK)
			return false;
		else
			return true;
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
