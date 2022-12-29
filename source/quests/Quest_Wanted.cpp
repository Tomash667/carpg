#include "Pch.h"
#include "Quest_Wanted.h"

#include "City.h"
#include "DialogContext.h"
#include "Journal.h"
#include "LocationContext.h"
#include "NameHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
bool Quest_Wanted::OnTimeout(TimeoutType ttype)
{
	if(targetUnit)
	{
		if(state == Quest::Failed)
			static_cast<City*>(startLoc)->questCaptain = CityQuestState::Failed;
		if(!targetUnit->hero->teamMember)
		{
			// not a team member, remove
			ForLocation(inLocation)->RemoveUnit(targetUnit);
		}
		else
			targetUnit->eventHandler = nullptr;
		targetUnit = nullptr;
	}

	OnUpdate(questMgr->txQuest[267]);

	return true;
}

//=================================================================================================
void Quest_Wanted::HandleUnitEvent(UnitEventHandler::TYPE eventType, Unit* unit)
{
	switch(eventType)
	{
	case UnitEventHandler::SPAWN:
		unit->hero->name = unitName;
		targetLoc->activeQuest = nullptr;
		targetUnit = unit;
		inLocation = world->GetCurrentLocationIndex();
		break;
	case UnitEventHandler::DIE:
		if(!unit->hero->teamMember)
		{
			SetProgress(Progress::Killed);
			targetUnit = nullptr;
		}
		break;
	/*case UnitEventHandler::RECRUIT:
		// target recruited, add note to journal
		if(prog != Progress::Recruited)
			SetProgress(Progress::Recruited);
		break;
	case UnitEventHandler::KICK:
		// kicked from team, can be killed now, don't dissapear
		unit->temporary = false;
		inLocation = world->GetCurrentLocationIndex();
		break;*/
	case UnitEventHandler::LEAVE:
		if(state == Quest::Failed)
			static_cast<City*>(startLoc)->questCaptain = CityQuestState::Failed;
		targetUnit = nullptr;
		break;
	}
}

//=================================================================================================
Quest::LoadResult Quest_Wanted::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> level;
	f >> crazy;
	if(LOAD_VERSION >= V_0_12)
		clas = Class::TryGet(f.ReadString1());
	else
	{
		int oldClass;
		f >> oldClass;
		clas = old::ConvertOldClass(oldClass);
	}
	f >> unitName;
	f >> targetUnit;
	f >> inLocation;

	if(!done)
	{
		unitToSpawn = crazy ? clas->crazy : clas->hero;
		unitDontAttack = true;
		unitEventHandler = this;
		sendSpawnEvent = true;
		unitSpawnLevel = level;
	}

	if(prog >= Progress::Started)
	{
		Item::Get("wanted_letter")->CreateCopy(letter);
		letter.id = "$wanted_letter";
		letter.name = questMgr->txQuest[258];
		letter.questId = id;
		letter.desc = Format(questMgr->txQuest[259], level * 100, unitName.c_str());
	}

	return LoadResult::Ok;
}
