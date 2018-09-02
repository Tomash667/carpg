#include "Pch.h"
#include "GameCore.h"
#include "Quest_DeliverParcel.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "GameGui.h"
#include "GameFile.h"
#include "World.h"

//=================================================================================================
void Quest_DeliverParcel::Start()
{
	start_loc = W.current_location_index;
	end_loc = W.GetRandomSettlementIndex(start_loc);
	quest_id = Q_DELIVER_PARCEL;
	type = QuestType::Mayor;
}

//=================================================================================================
GameDialog* Quest_DeliverParcel::GetDialog(int dialog_type)
{
	switch(dialog_type)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_deliver_parcel_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_deliver_parcel_timeout");
	case QUEST_DIALOG_NEXT:
		return FindDialog("q_deliver_parcel_give");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_DeliverParcel::SetProgress(int prog2)
{
	bool apply = true;

	switch(prog2)
	{
	case Progress::Started:
		// give parcel to player
		{
			Location& loc = *W.GetLocation(end_loc);
			const Item* base_item = Item::Get("parcel");
			game->PreloadItem(base_item);
			CreateItemCopy(parcel, base_item);
			parcel.id = "$parcel";
			parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			parcel.refid = refid;
			game->current_dialog->pc->unit->AddItem(&parcel, 1, true);
			start_time = W.GetWorldtime();
			state = Quest::Started;
			name = game->txQuest[9];

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc2 = GetStartLocation();
			msgs.push_back(Format(game->txQuest[3], LocationHelper::IsCity(loc2) ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[10], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str(),
				GetLocationDirName(loc2.pos, loc.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Rand() % 4 != 0)
			{
				Encounter* e = W.AddEncounter(enc);
				e->pos = (loc.pos + loc2.pos) / 2;
				e->range = 64;
				e->chance = 45;
				e->dont_attack = true;
				e->dialog = FindDialog("q_deliver_parcel_bandits");
				e->group = SG_BANDITS;
				e->text = game->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->location_event_handler = nullptr;
			}

			if(Net::IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&parcel, base_item);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &parcel, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::DeliverAfterTime:
		// player failed to deliver parcel in time, but gain some gold anyway
		{
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::Failed;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(125);

			msgs.push_back(game->txQuest[12]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			RemoveEncounter();

			if(Net::IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case Progress::Timeout:
		// player failed to deliver parcel in time
		{
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::Failed;

			msgs.push_back(game->txQuest[13]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			RemoveEncounter();

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// parcel delivered, end of quest
		{
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::None;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(250);

			RemoveEncounter();

			msgs.push_back(game->txQuest[14]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			if(Net::IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case Progress::AttackedBandits:
		// don't giver parcel to bandits, get attacked
		{
			RemoveEncounter();
			apply = false;

			msgs.push_back(game->txQuest[15]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::ParcelGivenToBandits:
		// give parcel to bandits
		{
			RemoveEncounter();

			game->current_dialog->talker->AddItem(&parcel, 1, true);
			game->RemoveQuestItem(&parcel, refid);

			msgs.push_back(game->txQuest[16]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NoParcelAttackedBandits:
		// no parcel, attacked by bandits
		apply = false;
		RemoveEncounter();
		break;
	}

	if(apply)
		prog = prog2;
}

//=================================================================================================
cstring Quest_DeliverParcel::FormatString(const string& str)
{
	Location& loc = *W.GetLocation(end_loc);
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, loc.pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 15;
}

//=================================================================================================
bool Quest_DeliverParcel::OnTimeout(TimeoutType ttype)
{
	if(ttype == TIMEOUT2)
	{
		msgs.push_back(game->txQuest[277]);
		game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
		game->AddGameMsg3(GMS_JOURNAL_UPDATED);
	}

	return true;
}

//=================================================================================================
bool Quest_DeliverParcel::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_deliver_parcel_after") == 0)
		return W.GetWorldtime() - start_time < 30 && Rand() % 2 == 0;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IfHaveQuestItem() const
{
	if(W.current_location_index == W.GetEncounterLocationIndex() && prog == Progress::Started)
		return true;
	return W.current_location_index == end_loc && (prog == Progress::Started || prog == Progress::ParcelGivenToBandits);
}

//=================================================================================================
const Item* Quest_DeliverParcel::GetQuestItem()
{
	return &parcel;
}

//=================================================================================================
void Quest_DeliverParcel::Save(GameWriter& f)
{
	Quest_Encounter::Save(f);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
		f << end_loc;
}

//=================================================================================================
bool Quest_DeliverParcel::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
	{
		f >> end_loc;

		Location& loc = *W.GetLocation(end_loc);
		const Item* base_item = Item::Get("parcel");
		CreateItemCopy(parcel, base_item);
		parcel.id = "$parcel";
		parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.refid = refid;
		if(game->mp_load)
			game->Net_RegisterItem(&parcel, base_item);
	}

	if(enc != -1)
	{
		Location& loc = *W.GetLocation(end_loc);
		Location& loc2 = GetStartLocation();
		Encounter* e = W.RecreateEncounter(enc);
		e->pos = (loc.pos + loc2.pos) / 2;
		e->range = 64;
		e->chance = 45;
		e->dont_attack = true;
		e->dialog = FindDialog("q_deliver_parcel_bandits");
		e->group = SG_BANDITS;
		e->text = game->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->location_event_handler = nullptr;
	}

	return true;
}
