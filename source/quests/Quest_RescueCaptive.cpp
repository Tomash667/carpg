#include "Pch.h"
#include "Quest_RescueCaptive.h"

#include "AIController.h"
#include "City.h"
#include "DialogContext.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "Level.h"
#include "LocationContext.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_RescueCaptive::Start()
{
	type = Q_RESCUE_CAPTIVE;
	category = QuestCategory::Captain;
	startLoc = world->GetCurrentLocation();
	switch(Rand() % 4)
	{
	case 0:
	case 1:
		group = UnitGroup::Get("bandits");
		break;
	case 2:
		group = UnitGroup::Get("orcs");
		break;
	case 3:
		group = UnitGroup::Get("goblins");
		break;
	}
}

//=================================================================================================
GameDialog* Quest_RescueCaptive::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_rescue_captive_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_rescue_captive_timeout");
	case QUEST_DIALOG_NEXT:
		if(DialogContext::current->talker->data->id == "captive")
			return GameDialog::TryGet("q_rescue_captive_talk");
		else
			return GameDialog::TryGet("q_rescue_captive_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_RescueCaptive::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// received quest
		{
			OnStart(questMgr->txQuest[28]);
			questMgr->questTimeouts.push_back(this);

			targetLoc = world->GetRandomSpawnLocation(startLoc->pos, group);
			targetLoc->SetKnown();
			targetLoc->active_quest = this;
			st = targetLoc->st;
			unitToSpawn = UnitData::Get("captive");
			unitDontAttack = true;
			atLevel = targetLoc->GetRandomLevel();
			unitEventHandler = this;
			sendSpawnEvent = true;
			captive = nullptr;

			msgs.push_back(Format(questMgr->txQuest[29], startLoc->name.c_str(), world->GetDate()));

			if(targetLoc->type == L_CAMP)
				msgs.push_back(Format(questMgr->txQuest[33], startLoc->name.c_str(), group->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
			else
				msgs.push_back(Format(questMgr->txQuest[34], startLoc->name.c_str(), group->name.c_str(), targetLoc->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
		}
		break;
	case Progress::FoundCaptive:
		// found captive
		{
			OnUpdate(questMgr->txQuest[35]);
			team->AddExp(2000);
		}
		break;
	case Progress::CaptiveDie:
		// captive died
		{
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			OnUpdate(questMgr->txQuest[36]);
		}
		break;
	case Progress::Timeout:
		// player failed to rescue captive in time
		{
			state = Quest::Failed;

			static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;
			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);

			OnUpdate(questMgr->txQuest[37]);
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}
		}
		break;
	case Progress::Finished:
		// captive returned to captain, end of quest
		{
			state = Quest::Completed;
			int reward = GetReward();
			team->AddReward(reward, reward * 3);

			static_cast<City*>(startLoc)->quest_captain = CityQuestState::None;
			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);
			team->RemoveMember(captive);

			gameLevel->RemoveUnit(captive);
			captive->event_handler = nullptr;
			OnUpdate(Format(questMgr->txQuest[38], GetStartLocationName()));

			captive = nullptr;
		}
		break;
	case Progress::CaptiveEscape:
		// captive escaped location without player
		{
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			OnUpdate(questMgr->txQuest[39]);
		}
		break;
	case Progress::ReportDeath:
		// inform captain about death of captive
		{
			state = Quest::Failed;
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;
			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);

			OnUpdate(questMgr->txQuest[40]);
		}
		break;
	case Progress::ReportEscape:
		// inform captain about escape of captive, end of quest
		{
			state = Quest::Completed;
			int reward = GetReward();
			team->AddReward(reward / 4, reward * 3 / 2);
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			static_cast<City*>(startLoc)->quest_captain = CityQuestState::None;
			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;

			OnUpdate(Format(questMgr->txQuest[41], GetStartLocationName()));
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);
		}
		break;
	case Progress::CaptiveLeftInCity:
		// captive was left in city
		{
			if(captive->hero->teamMember)
				team->RemoveMember(captive);
			captive->dont_attack = false;
			captive->OrderGoToInn();
			captive->temporary = true;
			captive->event_handler = nullptr;
			captive = nullptr;

			OnUpdate(Format(questMgr->txQuest[42], gameLevel->cityCtx->name.c_str()));
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RescueCaptive::FormatString(const string& str)
{
	if(str == "Goddamn_bandits")
		return Format("%s %s", questMgr->txQuest[group->gender ? 44 : 43], group->name.c_str());
	else if(str == "Those_bandits")
		return Format("%s %s", questMgr->txQuest[group->gender ? 46 : 45], group->name.c_str());
	else if(str == "locname")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(startLoc->pos, targetLoc->pos);
	else if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "reward")
		return Format("%d", GetReward());
	else if(str == "reward2")
		return Format("%d", GetReward() / 4);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_RescueCaptive::IsTimedout() const
{
	return world->GetWorldtime() - startTime >= 30;
}

//=================================================================================================
bool Quest_RescueCaptive::OnTimeout(TimeoutType ttype)
{
	if(prog < FoundCaptive)
	{
		if(captive)
		{
			captive->event_handler = nullptr;
			ForLocation(targetLoc, atLevel)->RemoveUnit(captive);
			captive = nullptr;
		}

		OnUpdate(questMgr->txQuest[267]);
	}

	return true;
}

//=================================================================================================
void Quest_RescueCaptive::HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit)
{
	assert(unit);

	switch(event_type)
	{
	case UnitEventHandler::DIE:
		SetProgress(Progress::CaptiveDie);
		break;
	case UnitEventHandler::LEAVE:
		SetProgress(Progress::CaptiveEscape);
		break;
	case UnitEventHandler::SPAWN:
		captive = unit;
		break;
	}
}

//=================================================================================================
bool Quest_RescueCaptive::IfNeedTalk(cstring topic) const
{
	if(strcmp(topic, "captive") != 0)
		return false;
	if(world->GetCurrentLocation() == startLoc)
	{
		if(prog == Progress::CaptiveDie || prog == Progress::CaptiveEscape || prog == Progress::CaptiveLeftInCity)
			return true;
		else if(prog == Progress::FoundCaptive && team->IsTeamMember(*captive))
			return true;
		else
			return false;
	}
	else if(world->GetCurrentLocation() == targetLoc && prog == Progress::Started)
		return true;
	else
		return false;
}

//=================================================================================================
bool Quest_RescueCaptive::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "is_camp") == 0)
		return targetLoc->type == L_CAMP;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_RescueCaptive::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << group;
	f << captive;
	f << st;
}

//=================================================================================================
Quest::LoadResult Quest_RescueCaptive::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> group;
	f >> captive;
	f >> st;

	unitEventHandler = this;

	if(!done)
	{
		unitToSpawn = UnitData::Get("captive");
		unitDontAttack = true;
	}

	return LoadResult::Ok;
}

//=================================================================================================
int Quest_RescueCaptive::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2000, 6000));
}
