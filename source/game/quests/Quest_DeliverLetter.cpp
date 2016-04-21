#include "Pch.h"
#include "Base.h"
#include "Quest_DeliverLetter.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"

//=================================================================================================
void Quest_DeliverLetter::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomCityLocation(start_loc);
	quest_id = Q_DELIVER_LETTER;
	type = Type::Mayor;
}

//=================================================================================================
GameDialog* Quest_DeliverLetter::GetDialog(int type)
{
	switch(type)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_deliver_letter_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_deliver_letter_timeout");
	case QUEST_DIALOG_NEXT:
		if(prog == Progress::Started)
			return FindDialog("q_deliver_letter_give");
		else
			return FindDialog("q_deliver_letter_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_DeliverLetter::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog)
	{
	case Progress::Started:
		// give letter to player
		{
			Location& loc = *game->locations[end_loc];
			const Item* base_item = FindItem("letter");
			CreateItemCopy(letter, base_item);
			letter.id = "$letter";
			letter.name = Format(game->txQuest[0], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			letter.refid = refid;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);

			QM.AcceptQuest(this, 2);

			Location& loc2 = *game->locations[start_loc];
			name = game->txQuest[2];
			msgs.push_back(Format(game->txQuest[3], loc2.type == L_CITY ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[4], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), kierunek_nazwa[GetLocationDir(loc2.pos, loc.pos)]));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&letter, base_item);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &letter, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::Timeout:
		// player failed to deliver letter in time
		{
			bool removed_item = false;

			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;
			if(game->current_location == end_loc)
			{
				game->current_dialog->pc->unit->RemoveQuestItem(refid);
				removed_item = true;
			}

			msgs.push_back(game->txQuest[5]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				if(removed_item && !game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	case Progress::GotResponse:
		// given letter, got response
		{
			Location& loc = *game->locations[end_loc];
			letter.name = Format(game->txQuest[1], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			msgs.push_back(game->txQuest[6]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_RenameItem(&letter);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	case Progress::Finished:
		// given response, end of quest
		{
			state = Quest::Completed;
			game->AddReward(100);

			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::None;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			msgs.push_back(game->txQuest[7]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			QM.RemoveTimeout(this, 2);

			if(game->IsOnline())
			{
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	}
}

//=================================================================================================
cstring Quest_DeliverLetter::FormatString(const string& str)
{
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (loc.type == L_CITY ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return kierunek_nazwa[GetLocationDir(game->locations[start_loc]->pos, loc.pos)];
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverLetter::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_DeliverLetter::OnTimeout(TimeoutType ttype)
{
	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_DeliverLetter::IfHaveQuestItem() const
{
	if(prog == Progress::Started)
		return game->current_location == end_loc;
	else
		return game->current_location == start_loc && prog == Progress::GotResponse;
}

//=================================================================================================
const Item* Quest_DeliverLetter::GetQuestItem()
{
	return &letter;
}

//=================================================================================================
void Quest_DeliverLetter::Save(HANDLE file)
{
	Quest::Save(file);

	if(prog < Progress::Finished)
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);
}

//=================================================================================================
void Quest_DeliverLetter::Load(HANDLE file)
{
	Quest::Load(file);

	if(prog < Progress::Finished)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);

		Location& loc = *game->locations[end_loc];

		const Item* base_item = FindItem("letter");
		CreateItemCopy(letter, base_item);
		letter.id = "$letter";
		letter.name = Format(game->txQuest[prog == Progress::GotResponse ? 1 : 0], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		letter.refid = refid;
		if(game->mp_load)
			game->Net_RegisterItem(&letter, base_item);
	}
}
