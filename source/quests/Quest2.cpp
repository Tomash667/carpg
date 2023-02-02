#include "Pch.h"
#include "Quest2.h"

#include "City.h"
#include "DialogContext.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "ScriptManager.h"
#include "World.h"

#include <angelscript.h>

//=================================================================================================
Quest2::Quest2() : scheme(nullptr), timeoutDays(-1)
{
}

//=================================================================================================
// Remove event ptr, must be called when removing real event
void Quest2::RemoveEventPtr(const EventPtr& event)
{
	LoopAndRemove(events, [&](EventPtr& e)
	{
		return event == e;
	});
}

//=================================================================================================
void Quest2::RemoveDialogPtr(Unit* unit)
{
	LoopAndRemove(unitDialogs, [=](Unit* u)
	{
		return unit == u;
	});
}

//=================================================================================================
GameDialog* Quest2::GetDialog(Cstring name)
{
	return scheme->GetDialog(name.s);
}

//=================================================================================================
GameDialog* Quest2::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_START)
		return scheme->GetDialog("start");
	else
	{
		assert(0); // not implemented
		return nullptr;
	}
}

//=================================================================================================
cstring Quest2::FormatString(const string& str)
{
	if(str == "date")
		return world->GetDate();
	else if(str == "name")
	{
		Unit* talker = DialogContext::current->talker;
		assert(talker->IsHero());
		return talker->hero->name.c_str();
	}
	else if(str == "playerName")
		return DialogContext::current->pc->name.c_str();
	else
	{
		assert(0);
		return Format("!!!%s!!!", str.c_str());
	}
}

//=================================================================================================
void Quest2::LoadQuest2(GameReader& f, cstring schemeId)
{
	assert(schemeId);

	Quest::Load(f);

	QuestScheme* scheme = questMgr->FindQuestInfo(schemeId)->scheme;
	SetScheme(scheme);
	if(IsActive())
	{
		f >> timeoutDays;
		LoadDetails(f);
	}
}

//=================================================================================================
void Quest2::Cleanup()
{
	if(timeoutDays != -1)
	{
		RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
		timeoutDays = -1;
	}

	for(EventPtr& e : events)
	{
		switch(e.source)
		{
		case EventPtr::LOCATION:
			e.location->RemoveEventHandler(this, EVENT_ANY, true);
			break;
		case EventPtr::UNIT:
			e.unit->RemoveEventHandler(this, EVENT_ANY, true);
			break;
		case EventPtr::USABLE:
			e.usable->RemoveEventHandler(this, EVENT_ANY, true);
			break;
		}
	}
	events.clear();

	for(Unit* unit : unitDialogs)
		unit->RemoveDialog(this, true);
	unitDialogs.clear();

	world->RemoveEncounter(this);
}

//=================================================================================================
void Quest2::SetTimeout(int days)
{
	assert(Any(state, Quest::Hidden, Quest::Started));
	assert(timeoutDays == -1);
	assert(days > 0);
	timeoutDays = days;
	startTime = world->GetWorldtime();
	questMgr->questTimeouts2.push_back(this);
}

//=================================================================================================
void Quest2::AddTimer(int days)
{
	questMgr->AddTimer(this, days);
}

//=================================================================================================
int Quest2::GetTimeout() const
{
	int days = world->GetWorldtime() - startTime;
	if(days >= timeoutDays)
		return 0;
	return timeoutDays - days;
}

//=================================================================================================
bool Quest2::IsTimedout() const
{
	if(timeoutDays == -1)
		return false;
	return world->GetWorldtime() - startTime >= timeoutDays;
}

//=================================================================================================
bool Quest2::OnTimeout(TimeoutType ttype)
{
	ScriptEvent event(EVENT_TIMEOUT);
	timeoutDays = -1;
	FireEvent(event);
	return true;
}
