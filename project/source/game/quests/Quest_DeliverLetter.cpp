#include "Pch.h"
#include "GameCore.h"
#include "Quest_DeliverLetter.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "GameGui.h"
#include "GameFile.h"
#include "World.h"

//=================================================================================================
void Quest_DeliverLetter::Start()
{
	start_loc = W.current_location_index;
	end_loc = W.GetRandomSettlementIndex(start_loc);
	quest_id = Q_DELIVER_LETTER;
	type = QuestType::Mayor;
}

//=================================================================================================
GameDialog* Quest_DeliverLetter::GetDialog(int dialog_type)
{
	switch(dialog_type)
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
			Location& loc = *W.GetLocation(end_loc);
			const Item* base_item = Item::Get("letter");
			game->PreloadItem(base_item);
			CreateItemCopy(letter, base_item);
			letter.id = "$letter";
			letter.name = Format(game->txQuest[0], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			letter.refid = refid;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);
			start_time = W.GetWorldtime();
			state = Quest::Started;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc2 = GetStartLocation();
			name = game->txQuest[2];
			msgs.push_back(Format(game->txQuest[3], LocationHelper::IsCity(loc2) ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[4], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str(),
				kierunek_nazwa[GetLocationDir(loc2.pos, loc.pos)]));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
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
			((City&)GetStartLocation())->quest_mayor = CityQuestState::Failed;
			if(W.current_location_index == end_loc)
			{
				game->current_dialog->pc->unit->RemoveQuestItem(refid);
				removed_item = true;
			}

			msgs.push_back(game->txQuest[5]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
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
			Location& loc = *W.GetLocation(end_loc);
			letter.name = Format(game->txQuest[1], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			msgs.push_back(game->txQuest[6]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
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

			((City&)GetStartLocation()).quest_mayor = CityQuestState::None;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			msgs.push_back(game->txQuest[7]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			if(Net::IsOnline())
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
	Location& loc = *W.GetLocation(end_loc);
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return kierunek_nazwa[GetLocationDir(GetStartLocation().pos, loc.pos)];
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverLetter::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 30;
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
		return W.current_location_index == end_loc;
	else
		return W.current_location_index == start_loc && prog == Progress::GotResponse;
}

//=================================================================================================
const Item* Quest_DeliverLetter::GetQuestItem()
{
	return &letter;
}

//=================================================================================================
void Quest_DeliverLetter::Save(GameWriter& f)
{
	Quest::Save(f);

	if(prog < Progress::Finished)
		f << end_loc;
}

//=================================================================================================
bool Quest_DeliverLetter::Load(GameReader& f)
{
	Quest::Load(f);

	if(prog < Progress::Finished)
	{
		f >> end_loc;

		Location& loc = *W.GetLocation(end_loc);

		const Item* base_item = Item::Get("letter");
		CreateItemCopy(letter, base_item);
		letter.id = "$letter";
		letter.name = Format(game->txQuest[prog == Progress::GotResponse ? 1 : 0], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys,
			loc.name.c_str());
		letter.refid = refid;
		if(game->mp_load)
			game->Net_RegisterItem(&letter, base_item);
	}

	return true;
}
