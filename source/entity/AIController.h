#pragma once

//-----------------------------------------------------------------------------
#include "Const.h"

//-----------------------------------------------------------------------------
struct ObjP
{
	Vec3 pos;
	float rot;
	Object* ptr;
};

//-----------------------------------------------------------------------------
struct RegionTarget
{
	LocationPart* locPart;
	Vec3 pos;
	bool exit;
};

//-----------------------------------------------------------------------------
enum class HavePotion
{
	No,
	Check,
	Yes
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
		Wait,
		Aggro,
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
	trzeba znalezc nowa sciezke (pfState == PFS_NOT_USING lub
		targetTile != startTile && pfState == PFS_WALKING && lastPfCheck < 0 lub
		dist(startTile, targetTile) > 1
	1. szuka globalna sciezke
		a. znaleziono (pfState = PFS_GLOBAL_DONE)
		b. jesli jest zablokowana to idzie lokalnie (pfState = PFS_GLOBAL_NOT_USED) lub
			jesli jest zadaleko to idzie w kierunku (pfState = PFS_MANUAL_WALK)
	2. szukanie lokalnej sciezki
		a. jest
			ustaw pfState = PFS_WALKING
		b. nie ma lub pfState == PFS_GLOBAL_NOT_USED
			idz w kierunku pfState = PFS_MANUAL_WALK
		c. zablokowana droga
			pfState = PFS_LOCAL_TRY_WALK
			pfLocalTry++;
			w nastêpnym obiektu idŸ do kolejnego
			jeœli pfLocalTry == 4 lub dystans do nastepnego punktu jest za daleki to pfState = PFS_RETRY i dodanie blokady
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

	Unit* unit;
	State state;
	union StateData
	{
		StateData() {}
		struct CastState
		{
			Ability* ability;
		} cast;
		struct EscapeState
		{
			Room* room;
		} escape;
		struct IdleState
		{
			IdleState() {}
			IdleAction action;
			union
			{
				float rot;
				Vec3 pos;
				Entity<Unit> unit;
				Usable* usable;
				ObjP obj;
				RegionTarget region;
			};
		} idle;
		struct SearchState
		{
			Room* room;
		} search;
	} st;
	Entity<Unit> target, alertTarget;
	Vec3 targetLastPos, alertTargetPos, startPos;
	bool inCombat, cityWander;
	float nextAttack, timer, ignore, morale, cooldown[MAX_ABILITIES], startRot, locTimer, scanTimer;
	HavePotion havePotion, haveMpPotion;
	int potion; // miksturka do u¿ycia po schowaniu broni
	bool changeAiMode; // tymczasowe u serwera

	// pathfinding
	vector<Int2> pfPath, pfLocalPath;
	PathFindingState pfState;
	Int2 pfTargetTile, pfLocalTargetTile;
	int pfLocalTry;
	float pfTimer;

	void Init(Unit* unit);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void LoadIdleAction(GameReader& f, StateData::IdleState& idle, bool apply);
	bool CheckPotion(bool inCombat = true);
	void Reset();
	float GetMorale() const;
	bool CanWander() const;
	Vec3 PredictTargetPos(const Unit& target, float bulletSpeed) const;
	void Shout();
	void HitReaction(const Vec3& pos);
	void DoAttack(Unit* target, bool running = false);
};

//-----------------------------------------------------------------------------
extern cstring strAiState[AIController::State_Max];
extern cstring strAiIdle[AIController::Idle_Max];
