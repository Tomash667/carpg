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
void Quest_Wanted::Start()
{
	startLoc = world->GetCurrentLocation();
	type = Q_WANTED;
	category = QuestCategory::Captain;
	level = Random(5, 15);
	crazy = (Rand() % 5 == 0);
	clas = crazy ? Class::GetRandomCrazy() : Class::GetRandomHero(true);
	targetUnit = nullptr;
	inLocation = -1;
}

//=================================================================================================
GameDialog* Quest_Wanted::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("qWantedStart");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("qWantedTimeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("qWantedEnd");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Wanted::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(questMgr->txQuest[257]);
			questMgr->questTimeouts.push_back(this);

			NameHelper::GenerateHeroName(clas, crazy, unitName);
			targetLoc = world->GetRandomFreeSettlement(startLoc);
			// if there is no free city he will talk about some random city but there won't be there...
			if(!targetLoc)
				targetLoc = world->GetRandomSettlement(startLoc);
			if(!targetLoc->activeQuest)
			{
				targetLoc->activeQuest = this;
				unitToSpawn = crazy ? clas->crazy : clas->hero;
				unitDontAttack = true;
				unitEventHandler = this;
				sendSpawnEvent = true;
				unitSpawnLevel = level;
			}

			// add letter
			Item::Get("wantedPoster")->CreateCopy(letter);
			letter.id = "$wantedPoster";
			letter.name = questMgr->txQuest[258];
			letter.questId = id;
			letter.desc = Format(questMgr->txQuest[259], level * 100, unitName.c_str());
			DialogContext::current->pc->unit->AddItem2(&letter, 1u, 1u);

			// add journal entry
			msgs.push_back(Format(questMgr->txQuest[29], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[260], level * 100, unitName.c_str(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::Timeout:
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->questCaptain = CityQuestState::Failed;

			if(targetLoc->activeQuest == this)
				targetLoc->activeQuest = nullptr;

			OnUpdate(Format(questMgr->txQuest[261], unitName.c_str()));

			done = false;
		}
		break;
	case Progress::Killed:
		{
			state = Quest::Started; // if recruited that will change it to in progress
			OnUpdate(Format(questMgr->txQuest[262], unitName.c_str()));
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			static_cast<City*>(startLoc)->questCaptain = CityQuestState::None;

			team->AddReward(level * 100, level * 250);

			OnUpdate(Format(questMgr->txQuest[263], unitName.c_str()));
		}
		break;
	case Progress::Recruited:
		{
			state = Quest::Failed;
			OnUpdate(Format(questMgr->txQuest[266], targetUnit->GetName()));
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Wanted::FormatString(const string& str)
{
	if(str == "reward")
		return Format("%d", level * 100);
	else if(str == "name")
		return unitName.c_str();
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "player")
		return DialogContext::current->pc->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Wanted::IsTimedout() const
{
	return world->GetWorldtime() - startTime >= 30;
}

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
bool Quest_Wanted::IfHaveQuestItem() const
{
	return DialogContext::current->talker == targetUnit;
}

//=================================================================================================
bool Quest_Wanted::IfNeedTalk(cstring topic) const
{
	return prog == Progress::Killed && strcmp(topic, "wanted") == 0 && world->GetCurrentLocation() == startLoc;
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
	case UnitEventHandler::RECRUIT:
		// target recruited, add note to journal
		if(prog != Progress::Recruited)
			SetProgress(Progress::Recruited);
		break;
	case UnitEventHandler::KICK:
		// kicked from team, can be killed now, don't dissapear
		unit->temporary = false;
		inLocation = world->GetCurrentLocationIndex();
		break;
	case UnitEventHandler::LEAVE:
		if(state == Quest::Failed)
			static_cast<City*>(startLoc)->questCaptain = CityQuestState::Failed;
		targetUnit = nullptr;
		break;
	}
}

//=================================================================================================
void Quest_Wanted::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << level;
	f << crazy;
	f << clas->id;
	f << unitName;
	f << targetUnit;
	f << inLocation;
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
		Item::Get("wantedPoster")->CreateCopy(letter);
		letter.id = "$wantedPoster";
		letter.name = questMgr->txQuest[258];
		letter.questId = id;
		letter.desc = Format(questMgr->txQuest[259], level * 100, unitName.c_str());
	}

	return LoadResult::Ok;
}
