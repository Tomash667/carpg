#include "Pch.h"
#include "UnitOrder.h"

#include "Unit.h"

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::NextOrder()
{
	next = UnitOrderEntry::Get();
	next->timer = -1.f;
	return next;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::WithTimer(float timer)
{
	this->timer = timer;
	return this;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::WithMoveType(MoveType moveType)
{
	this->moveType = moveType;
	return this;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::WithRange(float range)
{
	this->range = range;
	return this;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenWander()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_WANDER;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenWait()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_WAIT;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenFollow(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_FOLLOW;
	o->unit = target;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenLeave()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_LEAVE;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenMove(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_MOVE;
	o->pos = pos;
	o->moveType = MOVE_RUN;
	o->range = 0.1f;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenLookAt(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_LOOK_AT;
	o->pos = pos;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenEscapeTo(const Vec3& pos)
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_ESCAPE_TO;
	o->pos = pos;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenEscapeToUnit(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_ESCAPE_TO_UNIT;
	o->unit = target;
	o->pos = target->pos;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenGoToInn()
{
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_GOTO_INN;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenGuard(Unit* target)
{
	assert(target);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_GUARD;
	o->unit = target;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenAutoTalk(bool leader, GameDialog* dialog, Quest* quest)
{
	if(quest)
		assert(dialog);

	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_AUTO_TALK;
	if(!leader)
	{
		o->autoTalk = AutoTalkMode::Yes;
		o->timer = Unit::AUTO_TALK_WAIT;
	}
	else
	{
		o->autoTalk = AutoTalkMode::Leader;
		o->timer = 0.f;
	}
	o->autoTalkDialog = dialog;
	o->autoTalkQuest = quest;
	return o;
}

//=================================================================================================
UnitOrderEntry* UnitOrderEntry::ThenAttackObject(Usable* usable)
{
	assert(usable);
	UnitOrderEntry* o = NextOrder();
	o->order = ORDER_ATTACK_OBJECT;
	o->usable = usable;
	return o;
}
