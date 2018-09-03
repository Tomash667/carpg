#include "Pch.h"
#include "GameCore.h"
#include "Quest_RescueCaptive.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "City.h"
#include "GameGui.h"
#include "AIController.h"
#include "SaveState.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_RescueCaptive::Start()
{
	quest_id = Q_RESCUE_CAPTIVE;
	type = QuestType::Captain;
	start_loc = W.current_location_index;
	group = GetRandomGroup();
}

//=================================================================================================
GameDialog* Quest_RescueCaptive::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_rescue_captive_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_rescue_captive_timeout");
	case QUEST_DIALOG_NEXT:
		if(game->current_dialog->talker->data->id == "captive")
			return FindDialog("q_rescue_captive_talk");
		else
			return FindDialog("q_rescue_captive_end");
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
			target_loc = W.GetRandomSpawnLocation(W.GetLocation(start_loc)->pos, group);

			Location& loc = GetStartLocation();
			Location& loc2 = GetTargetLocation();
			loc2.SetKnown();

			loc2.active_quest = this;
			unit_to_spawn = UnitData::Get("captive");
			unit_dont_attack = true;
			at_level = loc2.GetRandomLevel();
			unit_event_handler = this;
			send_spawn_event = true;

			start_time = W.GetWorldtime();
			state = Quest::Started;
			name = game->txQuest[28];
			captive = nullptr;

			msgs.push_back(Format(game->txQuest[29], loc.name.c_str(), W.GetDate()));

			cstring co;
			switch(group)
			{
			case SG_BANDITS:
			default:
				co = game->txQuest[30];
				break;
			case SG_ORCS:
				co = game->txQuest[31];
				break;
			case SG_GOBLINS:
				co = game->txQuest[32];
				break;
			}

			if(loc2.type == L_CAMP)
			{
				game->target_loc_is_camp = true;
				msgs.push_back(Format(game->txQuest[33], loc.name.c_str(), co, GetLocationDirName(loc.pos, loc2.pos)));
			}
			else
			{
				game->target_loc_is_camp = false;
				msgs.push_back(Format(game->txQuest[34], loc.name.c_str(), co, loc2.name.c_str(), GetLocationDirName(loc.pos, loc2.pos)));
			}

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			quest_manager.quests_timeout.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::FoundCaptive:
		// found captive
		{
			msgs.push_back(game->txQuest[35]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
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

			msgs.push_back(game->txQuest[36]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Timeout:
		// player failed to rescue captive in time
		{
			state = Quest::Failed;

			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);

			msgs.push_back(game->txQuest[37]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// captive returned to captain, end of quest
		{
			state = Quest::Completed;
			game->AddReward(1000);

			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			game->RemoveTeamMember(captive);

			game->RemoveUnit(captive);
			captive->event_handler = nullptr;
			msgs.push_back(Format(game->txQuest[38], GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);

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

			msgs.push_back(game->txQuest[39]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
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

			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);

			msgs.push_back(game->txQuest[40]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::ReportEscape:
		// inform captain about escape of captive, end of quest
		{
			state = Quest::Completed;
			game->AddReward(250);
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			msgs.push_back(Format(game->txQuest[41], GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::CaptiveLeftInCity:
		// captive was left in city
		{
			if(captive->hero->team_member)
				game->RemoveTeamMember(captive);
			captive->dont_attack = false;
			captive->ai->goto_inn = true;
			captive->ai->timer = 0.f;
			captive->temporary = true;
			captive->event_handler = nullptr;
			captive = nullptr;

			msgs.push_back(Format(game->txQuest[42], game->city_ctx->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RescueCaptive::FormatString(const string& str)
{
	if(str == "i_bandyci")
	{
		switch(group)
		{
		case SG_BANDITS:
			return game->txQuest[43];
		case SG_ORCS:
			return game->txQuest[44];
		case SG_GOBLINS:
			return game->txQuest[45];
		default:
			assert(0);
			return game->txQuest[46];
		}
	}
	else if(str == "ci_bandyci")
	{
		switch(group)
		{
		case SG_BANDITS:
			return game->txQuest[47];
		case SG_ORCS:
			return game->txQuest[48];
		case SG_GOBLINS:
			return game->txQuest[49];
		default:
			assert(0);
			return game->txQuest[50];
		}
	}
	else if(str == "locname")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_RescueCaptive::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_RescueCaptive::OnTimeout(TimeoutType ttype)
{
	if(prog < FoundCaptive)
	{
		if(captive)
		{
			captive->event_handler = nullptr;
			game->RemoveUnit(game->ForLevel(target_loc, at_level), captive);
			captive = nullptr;
		}

		msgs.push_back(game->txQuest[277]);
		game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
		game->AddGameMsg3(GMS_JOURNAL_UPDATED);
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
	if(W.current_location_index == start_loc)
	{
		if(prog == Progress::CaptiveDie || prog == Progress::CaptiveEscape || prog == Progress::CaptiveLeftInCity)
			return true;
		else if(prog == Progress::FoundCaptive && Team.IsTeamMember(*captive))
			return true;
		else
			return false;
	}
	else if(W.current_location_index == target_loc && prog == Progress::Started)
		return true;
	else
		return false;
}

//=================================================================================================
void Quest_RescueCaptive::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << group;
	f << captive;
}

//=================================================================================================
bool Quest_RescueCaptive::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(LOAD_VERSION >= V_0_4)
		f >> group;
	else
	{
		if(prog != Progress::None)
			f >> group;
		else
			group = GetRandomGroup();
	}
	f >> captive;

	unit_event_handler = this;

	if(!done)
	{
		unit_to_spawn = UnitData::Get("captive");
		unit_dont_attack = true;
	}

	return true;
}

//=================================================================================================
SPAWN_GROUP Quest_RescueCaptive::GetRandomGroup() const
{
	switch(Rand() % 4)
	{
	default:
	case 0:
	case 1:
		return SG_BANDITS;
	case 2:
		return SG_ORCS;
		break;
	case 3:
		return SG_GOBLINS;
	}
}
