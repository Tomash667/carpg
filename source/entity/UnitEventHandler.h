#pragma once

//-----------------------------------------------------------------------------
struct UnitEventHandler
{
	enum TYPE
	{
		LEAVE,
		DIE,
		FALL,
		SPAWN,
		RECRUIT,
		KICK
	};

	virtual void HandleUnitEvent(TYPE eventType, Unit* unit) = 0;
	virtual int GetUnitEventHandlerQuestId() = 0;
};
