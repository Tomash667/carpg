#include "Pch.h"
#include "GameCore.h"
#include "Quest_BanditsCollectToll.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "City.h"
#include "GameGui.h"
#include "SoundManager.h"
#include "GameFile.h"
#include "World.h"

//=================================================================================================
void Quest_BanditsCollectToll::Start()
{
	quest_id = Q_BANDITS_COLLECT_TOLL;
	type = QuestType::Captain;
	start_loc = W.current_location_index;
	other_loc = W.GetRandomSettlementIndex(start_loc);
}

//=================================================================================================
GameDialog* Quest_BanditsCollectToll::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_bandits_collect_toll_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_bandits_collect_toll_timeout");
	case QUEST_DIALOG_NEXT:
		return FindDialog("q_bandits_collect_toll_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_BanditsCollectToll::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// quest accepted
		{
			start_time = W.GetWorldtime();
			state = Quest::Started;
			name = game->txQuest[51];

			Location& sl = GetStartLocation();
			Location& ol = *W.GetLocation(other_loc);

			Encounter* e = W.AddEncounter(enc);
			e->dialog = FindDialog("q_bandits_collect_toll_talk");
			e->dont_attack = true;
			e->group = SG_BANDITS;
			e->pos = (sl.pos + ol.pos) / 2;
			e->quest = this;
			e->chance = 50;
			e->text = game->txQuest[52];
			e->timed = true;
			e->range = 64;
			e->location_event_handler = this;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[53], sl.name.c_str(), ol.name.c_str(), GetLocationDirName(sl.pos, ol.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::Timout:
		// player failed to kill bandits in time
		{
			state = Quest::Failed;
			RemoveEncounter();
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
			msgs.push_back(game->txQuest[54]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::KilledBandits:
		// player killed bandits
		{
			msgs.push_back(game->txQuest[55]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveEncounter();

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// player talked with captain after killing bandits, end of quest
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[56]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(400);
			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			W.AddNews(game->txQuest[278]);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_BanditsCollectToll::FormatString(const string& str)
{
	if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "other_loc")
		return W.GetLocation(other_loc)->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(GetStartLocation().pos, W.GetLocation(other_loc)->pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_BanditsCollectToll::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 15;
}

//=================================================================================================
bool Quest_BanditsCollectToll::OnTimeout(TimeoutType ttype)
{
	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
void Quest_BanditsCollectToll::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "pay_500") == 0)
	{
		ctx.pc->unit->ModGold(-500);
		ctx.talker->gold += 500;
	}
	else
	{
		assert(0);
	}
}

//=================================================================================================
bool Quest_BanditsCollectToll::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started)
		SetProgress(Progress::KilledBandits);
	return false;
}

//=================================================================================================
bool Quest_BanditsCollectToll::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "road_bandits") == 0 && prog == Progress::KilledBandits);
}

//=================================================================================================
void Quest_BanditsCollectToll::Save(GameWriter& f)
{
	Quest_Encounter::Save(f);

	f << other_loc;
}

//=================================================================================================
bool Quest_BanditsCollectToll::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	f >> other_loc;

	if(enc != -1)
	{
		Location& sl = GetStartLocation();
		Location& ol = *W.GetLocation(other_loc);

		Encounter* e = W.RecreateEncounter(enc);
		e->dialog = FindDialog("q_bandits_collect_toll_talk");
		e->dont_attack = true;
		e->group = SG_BANDITS;
		e->pos = (sl.pos + ol.pos) / 2;
		e->quest = this;
		e->chance = 50;
		e->text = game->txQuest[52];
		e->timed = true;
		e->range = 64;
		e->location_event_handler = this;
	}

	return true;
}
