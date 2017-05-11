// kontroler ai
#pragma once

//-----------------------------------------------------------------------------
struct Unit;
struct Room;
struct Useable;
struct Object;

//-----------------------------------------------------------------------------
struct ObjP
{
	VEC3P pos;
	float rot;
	Object* ptr;
};

//-----------------------------------------------------------------------------
struct AreaTarget
{
	VEC3P pos;
	int id;
};

//-----------------------------------------------------------------------------
struct AIController
{
	enum State
	{
		Idle,
		SeenEnemy,
		Fighting,
		SearchEnemy,
		Dodge,
		Escape,
		Block,
		Cast,
		State_Max
	};

	enum IdleAction
	{
		Idle_None,
		Idle_Animation,
		Idle_Rot,
		Idle_Move,
		Idle_Look,
		Idle_Chat,
		Idle_Use,
		Idle_WalkTo,
		Idle_WalkUse,
		Idle_TrainCombat,
		Idle_TrainBow,
		Idle_MoveRegion,
		Idle_RunRegion,
		Idle_Pray,
		Idle_WalkNearUnit,
		Idle_WalkUseEat,
		Idle_MoveAway,
		Idle_Max
	};

	/*
	trzeba znalezc nowa sciezke (pf_state == PFS_NOT_USING lub
		target_tile != start_tile && pf_state == PFS_WALKING && last_pf_check < 0 lub
		dist(start_tile, target_tile) > 1
	1. szuka globalna sciezke
		a. znaleziono (pf_state = PFS_GLOBAL_DONE)
		b. jesli jest zablokowana to idzie lokalnie (pf_state = PFS_GLOBAL_NOT_USED) lub
			jesli jest zadaleko to idzie w kierunku (pf_state = PFS_MANUAL_WALK)
	2. szukanie lokalnej sciezki
		a. jest
			ustaw pf_state = PFS_WALKING
		b. nie ma lub pf_state == PFS_GLOBAL_NOT_USED
			idz w kierunku pf_state = PFS_MANUAL_WALK
		c. zablokowana droga
			pf_state = PFS_LOCAL_TRY_WALK
			pf_local_try++;
			w nastêpnym obiektu idŸ do kolejnego
			jeœli pf_local_try == 4 lub dystans do nastepnego punktu jest za daleki to pf_state = PFS_RETRY i dodanie blokady
	*/
	enum PathFindingState
	{
		PFS_NOT_USING,
		PFS_GLOBAL_DONE,
		PFS_GLOBAL_NOT_USED,
		PFS_MANUAL_WALK,
		PFS_WALKING,
		PFS_LOCAL_TRY_WALK,
		PFS_WALKING_LOCAL
	};

	Unit* unit, *target, *alert_target, *cast_target;
	State state;
	VEC3 target_last_pos, alert_target_pos, start_pos;
	bool in_combat, city_wander, goto_inn;
	float next_attack, timer, ignore, morale, cooldown[3], last_scan, start_rot, loc_timer, shoot_yspeed;
	Room* escape_room;
	int have_potion; // czy ai ma miksturke lecznicz¹: 0 - nie ma, 1 - trzeba sprawdziæ, 2 - ma
	int potion; // miksturka do u¿ycia po schowaniu broni
	IdleAction idle_action;
	union IdleData
	{
		float rot;
		VEC3P pos;
		Unit* unit;
		Useable* useable;
		ObjP obj;
		AreaTarget area;
	} idle_data;
	bool change_ai_mode; // tymczasowe u serwera

	// pathfinding
	vector<INT2> pf_path, pf_local_path;
	PathFindingState pf_state;
	INT2 pf_target_tile, pf_local_target_tile;
	int pf_local_try;
	float pf_timer;

	void Init(Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	bool CheckPotion(bool in_combat=true);
	void Reset();
};

//-----------------------------------------------------------------------------
extern cstring str_ai_state[AIController::State_Max];
extern cstring str_ai_idle[AIController::Idle_Max];
