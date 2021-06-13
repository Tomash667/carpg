#include "Pch.h"
#include "Quest_KillAnimals.h"

#include "City.h"
#include "DialogContext.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_KillAnimals::Start()
{
	type = Q_KILL_ANIMALS;
	category = QuestCategory::Captain;
	startLoc = world->GetCurrentLocation();
}

//=================================================================================================
void Quest_KillAnimals::SetProgress(int p)
{
	prog = p;
	switch(prog)
	{
	case Started:
		// player accepted quest
		{
			if(Rand() % 2 == 0)
				targetLoc = world->GetClosestLocation(L_CAVE, startLoc->pos);
			else
				targetLoc = world->GetClosestLocation(L_OUTSIDE, startLoc->pos, { FOREST, HILLS });
			targetLoc->AddEventHandler(this, EVENT_CLEARED);
			targetLoc->active_quest = this;
			targetLoc->SetKnown();
			if(targetLoc->st < 5)
				targetLoc->st = 5;
			st = targetLoc->st;

			OnStart(GetText(0));
			msgs.push_back(GetText(1));
			msgs.push_back(GetText(2));

			DialogContext::current->talker->AddDialog(this, GetDialog("captain"));
			SetTimeout(30);
		}
		break;
	case ClearedLocation:
		// player cleared location from animals
		{
			targetLoc->active_quest = nullptr;
			OnUpdate(GetText(3));
		}
		break;
	case Finished:
		// player talked with captain, end of quest
		{
			SetState(State::Completed);
			int reward = GetReward();
			team->AddReward(2500, reward * 3);
			OnUpdate(GetText(4));
		}
		break;
	case Timeout:
		// player failed to clear location in time
		{
			SetState(State::Failed);
			OnUpdate(GetText(6));
		}
		break;
	case OnTimeout:
		{
			targetLoc->RemoveEventHandler(this, EVENT_CLEARED);
			targetLoc->active_quest = nullptr;
			OnUpdate(GetText(5));
		}
		break;
	}
}

//=================================================================================================
void Quest_KillAnimals::FireEvent(ScriptEvent& event)
{
	if(event.type == EVENT_CLEARED)
		SetProgress(ClearedLocation);
	else if(event.type == EVENT_TIMEOUT)
	{
		if(prog == Started)
			SetProgress(OnTimeout);
	}
}

//=================================================================================================
cstring Quest_KillAnimals::FormatString(const string& str)
{
	if(str == "reward")
		return Format("%d", GetReward());
	else
		return Quest2::FormatString(str);
}

//=================================================================================================
void Quest_KillAnimals::SaveDetails(GameWriter& f)
{
	f << targetLoc;
	f << st;
}

//=================================================================================================
Quest::LoadResult Quest_KillAnimals::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_17)
		Quest2::Load(f);
	else
	{
		Quest::Load(f);
		f >> targetLoc;
		f.Skip<bool>(); // done
		f.Skip<int>(); // at_level
		SetScheme(quest_mgr->FindQuestInfo(type)->scheme);

		if(LOAD_VERSION >= V_0_9)
			f >> st;
		else if(targetLoc)
			st = targetLoc->st;
		else
			st = 5;

		if(IsActive())
		{
			if(prog == Started)
			{
				if(timeout)
					prog = OnTimeout;
				else
				{
					targetLoc->AddEventHandler(this, EVENT_CLEARED);
					SetTimeout(30 + start_time - world->GetWorldtime());
				}
			}
			LocationHelper::GetCaptain(startLoc)->AddDialog(this, GetDialog("captain"));
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_KillAnimals::LoadDetails(GameReader& f)
{
	f >> targetLoc;
	f >> st;
}

//=================================================================================================
int Quest_KillAnimals::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 10), Int2(2500, 4000));
}
