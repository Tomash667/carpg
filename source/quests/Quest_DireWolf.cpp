#include "Pch.h"
#include "Quest_DireWolf.h"

#include "Level.h"
#include "OutsideLocation.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_DireWolf::Start()
{
	category = QuestCategory::Unique;
	type = Q_DIRE_WOLF;

	startLoc = world->GetLocationByType(L_OUTSIDE, HUNTERS_CAMP);
	startLoc->AddEventHandler(this, EVENT_ENTER);
	forest = nullptr;
}

//=================================================================================================
void Quest_DireWolf::SetProgress(int p)
{
	switch(p)
	{
	case Started:
		forest = world->GetClosestLocation(L_OUTSIDE, startLoc->pos, FOREST);
		forest->AddEventHandler(this, EVENT_ENTER);
		forest->SetKnown();
		forest->activeQuest = this;

		OnStart(GetText(0));
		msgs.push_back(GetText(1));
		msgs.push_back(GetText(2));
		break;
	case Killed:
		forest->activeQuest = nullptr;
		OnUpdate(GetText(3));
		break;
	case Complete:
		SetState(State::Completed);
		team->AddReward(5000, 15000);
		OnUpdate(GetText(4));
		break;
	}

	prog = p;
}

//=================================================================================================
void Quest_DireWolf::FireEvent(ScriptEvent& event)
{
	if(event.type == EVENT_ENTER)
	{
		if(prog == None)
		{
			// add dialog to hunters leader
			UnitData* ud = UnitData::Get("hunter_leader");
			Unit* unit = gameLevel->FindUnit(ud);
			unit->AddDialog(this, GetDialog("hunter"));
		}
		else
		{
			// spawn dire wolf
			UnitData* ud = UnitData::Get("dire_wolf");
			Unit* unit = gameLevel->SpawnUnitNearLocation(*gameLevel->localPart, Vec3(128, 0, 128), *ud);
			unit->AddEventHandler(this, EVENT_DIE);
		}
	}
	else if(event.type == EVENT_DIE)
		SetProgress(Killed);
	RemoveEvent(event);
}

//=================================================================================================
void Quest_DireWolf::SaveDetails(GameWriter& f)
{
	f << forest;
}

//=================================================================================================
void Quest_DireWolf::LoadDetails(GameReader& f)
{
	f >> forest;
}
