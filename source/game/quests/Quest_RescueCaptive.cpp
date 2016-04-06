#include "Pch.h"
#include "Base.h"
#include "Quest_RescueCaptive.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"

//=================================================================================================
void Quest_RescueCaptive::Start()
{
	quest_id = Q_RESCUE_CAPTIVE;
	type = Type::Captain;
	start_loc = game->current_location;
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
			target_loc = game->GetRandomSpawnLocation(game->locations[start_loc]->pos, group);

			Location& loc = *game->locations[start_loc];
			Location& loc2 = *game->locations[target_loc];
			bool now_known = false;
			if(loc2.state == LS_UNKNOWN)
			{
				loc2.state = LS_KNOWN;
				now_known = true;
			}

			loc2.active_quest = this;
			unit_to_spawn = FindUnitData("captive");
			unit_dont_attack = true;
			at_level = loc2.GetRandomLevel();
			unit_event_handler = this;
			send_spawn_event = true;

			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[28];
			captive = nullptr;

			msgs.push_back(Format(game->txQuest[29], loc.name.c_str(), game->day+1, game->month+1, game->year));

			cstring co;
			switch(group)
			{
			case SG_BANDYCI:
			default:
				co = game->txQuest[30];
				break;
			case SG_ORKOWIE:
				co = game->txQuest[31];
				break;
			case SG_GOBLINY:
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

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::FoundCaptive:
		// found captive
		{
			msgs.push_back(game->txQuest[35]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
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

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Timeout:
		// player failed to rescue captive in time
		{
			state = Quest::Failed;

			((City*)game->locations[start_loc])->quest_captain = CityQuestState::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			msgs.push_back(game->txQuest[37]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(captive)
			{
				captive->event_handler = nullptr;
				captive = nullptr;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// captive returned to captain, end of quest
		{
			state = Quest::Completed;
			game->AddReward(1000);

			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			game->RemoveTeamMember(captive);
			
			captive->to_remove = true;
			game->to_remove.push_back(captive);
			captive->event_handler = nullptr;
			msgs.push_back(Format(game->txQuest[38], game->locations[start_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_RemoveUnit(captive);
			}

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

			if(game->IsOnline())
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

			((City*)game->locations[start_loc])->quest_captain = CityQuestState::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			msgs.push_back(game->txQuest[40]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
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

			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			msgs.push_back(Format(game->txQuest[41], game->locations[start_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
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

			if(game->IsOnline())
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
		case SG_BANDYCI:
			return game->txQuest[43];
		case SG_ORKOWIE:
			return game->txQuest[44];
		case SG_GOBLINY:
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
		case SG_BANDYCI:
			return game->txQuest[47];
		case SG_ORKOWIE:
			return game->txQuest[48];
		case SG_GOBLINY:
			return game->txQuest[49];
		default:
			assert(0);
			return game->txQuest[50];
		}
	}
	else if(str == "locname")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_RescueCaptive::IsTimedout() const
{
	return game->worldtime - start_time > 30;
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
void Quest_RescueCaptive::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	assert(unit);

	switch(type)
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
	if(game->current_location == start_loc)
	{
		if(prog == Progress::CaptiveDie || prog == Progress::CaptiveEscape || prog == Progress::CaptiveLeftInCity)
			return true;
		else if(prog == Progress::FoundCaptive && game->IsTeamMember(*captive))
			return true;
		else
			return false;
	}
	else if(game->current_location == target_loc && prog == Progress::Started)
		return true;
	else
		return false;
}

//=================================================================================================
void Quest_RescueCaptive::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);
	f << group;
	f << captive;
}

//=================================================================================================
void Quest_RescueCaptive::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);
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
		unit_to_spawn = FindUnitData("captive");
		unit_dont_attack = true;
	}
}

//=================================================================================================
SPAWN_GROUP Quest_RescueCaptive::GetRandomGroup() const
{
	switch(rand2()%4)
	{
	default:
	case 0:
	case 1:
		return SG_BANDYCI;
	case 2:
		return SG_ORKOWIE;
		break;
	case 3:
		return SG_GOBLINY;
	}
}
