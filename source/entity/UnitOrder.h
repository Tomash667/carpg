#pragma once

//-----------------------------------------------------------------------------
enum UnitOrder
{
	ORDER_NONE,
	ORDER_WANDER, // for heroes, they wander freely around city
	ORDER_WAIT, // for heroes, stay close to current position
	ORDER_FOLLOW, // for heroes, follow team leader
	ORDER_LEAVE, // go to nearest location exit and leave
	ORDER_MOVE, // move to position
	ORDER_LOOK_AT, // look at position
	ORDER_ESCAPE_TO, // unit runs toward position and ignore enemies
	ORDER_ESCAPE_TO_UNIT, // unit runs toward other unit and ignore enemies
	ORDER_GOTO, // go to building
	ORDER_GUARD, // stay close to another unit and remove dontAttack flag when target is attacked
	ORDER_AUTO_TALK, // talk with nearest player or leader
	ORDER_ATTACK_OBJECT, // attack destroyable object
	ORDER_MAX
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
		int insideBuilding;
		Entity<Usable> usable;
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
	UnitOrderEntry* ThenGoTo(CityBuilding* building);
	UnitOrderEntry* ThenGuard(Unit* target);
	UnitOrderEntry* ThenAutoTalk(bool leader, GameDialog* dialog, Quest* quest);
	UnitOrderEntry* ThenAttackObject(Usable* usable);

private:
	UnitOrderEntry* NextOrder();
};
