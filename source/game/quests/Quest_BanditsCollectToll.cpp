#include "Pch.h"
#include "Base.h"
#include "Quest_BanditsCollectToll.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"

//=================================================================================================
void Quest_BanditsCollectToll::Start()
{
	quest_id = Q_BANDITS_COLLECT_TOLL;
	type = Type::Captain;
	start_loc = game->current_location;
	other_loc = game->GetRandomCityLocation(start_loc);
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
			name = game->txQuest[51];

			Location& sl = *game->locations[start_loc];
			Location& ol = *game->locations[other_loc];

			Encounter* e = game->AddEncounter(enc);
			e->dialog = FindDialog("q_bandits_collect_toll_talk");
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->pos = (sl.pos + ol.pos)/2;
			e->quest = this;
			e->szansa = 50;
			e->text = game->txQuest[52];
			e->timed = true;
			e->zasieg = 64;
			e->location_event_handler = this;

			QM.AcceptQuest(this);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[53], sl.name.c_str(), ol.name.c_str(), GetLocationDirName(sl.pos, ol.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::Timout:
		// player failed to kill bandits in time
		{
			state = Quest::Failed;
			RemoveEncounter();
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::Failed;
			msgs.push_back(game->txQuest[54]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
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

			if(game->IsOnline())
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
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			game->AddNews(game->txQuest[278]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_BanditsCollectToll::FormatString(const string& str)
{
	if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else if(str == "other_loc")
		return game->locations[other_loc]->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[other_loc]->pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_BanditsCollectToll::IsTimedout() const
{
	return game->worldtime - start_time > 15;
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
		ctx.talker->gold += 500;
		ctx.pc->unit->gold -= 500;
		if(game->sound_volume)
			game->PlaySound2d(game->sCoins);
		if(!ctx.is_local)
			game->GetPlayerInfo(ctx.pc->id).UpdateGold();
	}
	else
	{
		assert(0);
	}
}

//=================================================================================================
bool Quest_BanditsCollectToll::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "have_500") == 0)
		return (ctx.pc->unit->gold >= 500);
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_BanditsCollectToll::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started)
		SetProgress(Progress::KilledBandits);
}

//=================================================================================================
bool Quest_BanditsCollectToll::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "road_bandits") == 0 && prog == Progress::KilledBandits);
}

//=================================================================================================
void Quest_BanditsCollectToll::Save(HANDLE file)
{
	Quest_Encounter::Save(file);

	WriteFile(file, &other_loc, sizeof(other_loc), &tmp, nullptr);
}

//=================================================================================================
void Quest_BanditsCollectToll::Load(HANDLE file)
{
	Quest_Encounter::Load(file);

	ReadFile(file, &other_loc, sizeof(other_loc), &tmp, nullptr);

	if(enc != -1)
	{
		Location& sl = *game->locations[start_loc];
		Location& ol = *game->locations[other_loc];

		Encounter* e = game->RecreateEncounter(enc);
		e->dialog = FindDialog("q_bandits_collect_toll_talk");
		e->dont_attack = true;
		e->grupa = SG_BANDYCI;
		e->pos = (sl.pos + ol.pos)/2;
		e->quest = this;
		e->szansa = 50;
		e->text = game->txQuest[52];
		e->timed = true;
		e->zasieg = 64;
		e->location_event_handler = this;
	}
}
