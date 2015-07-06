#include "Pch.h"
#include "Base.h"
#include "Quest_DeliverParcel.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"

//-----------------------------------------------------------------------------
DialogEntry deliver_parcel_start[] = {
	TALK2(17),
	CHOICE(18),
		TALK2(19),
		TALK(20),
		TALK2(21),
		SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::Started),
		END,
	END_CHOICE,
	CHOICE(22),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry deliver_parcel_give[] = {
	TALK(23),
	IF_QUEST_TIMEOUT,
		IF_RAND(2),
			TALK(24),
			TALK(25),
			SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::DeliverAfterTime),
			END,
		ELSE,
			TALK(26),
			SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::Timeout),
			END,
		END_IF,
	END_IF,
	TALK(27),
	SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::Finished),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry deliver_parcel_timeout[] = {
	TALK2(28),
	SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::Timeout),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry deliver_parcel_bandits[] = {
	IF_QUEST_PROGRESS(Quest_DeliverParcel::Progress::Started),
		IF_HAVE_QUEST_ITEM("$parcel"),
			TALK2(29),
			TALK(30),
			CHOICE(31),
				TALK(32),
				SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::ParcelGivenToBandits),
				END,
			END_CHOICE,
			CHOICE(33),
				SPECIAL("attack"),
				SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::AttackedBandits),
				END,
			END_CHOICE,
			SHOW_CHOICES,
		ELSE,
			SET_QUEST_PROGRESS(Quest_DeliverParcel::Progress::NoParcelAttackedBandits),
			SPECIAL("attack"),
		END_IF,
	ELSE,
		TALK(34),
		END,
	END_IF,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_DeliverParcel::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomCityLocation(start_loc);
	quest_id = Q_DELIVER_PARCEL;
	type = Type::Mayor;
}

//=================================================================================================
DialogEntry* Quest_DeliverParcel::GetDialog(int type)
{
	switch(type)
	{
	case QUEST_DIALOG_START:
		return deliver_parcel_start;
	case QUEST_DIALOG_FAIL:
		return deliver_parcel_timeout;
	case QUEST_DIALOG_NEXT:
		return deliver_parcel_give;
	default:
		assert(0);
		return NULL;
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
			Location& loc = *game->locations[end_loc];
			parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			parcel.type = IT_OTHER;
			parcel.weight = 10;
			parcel.value = 0;
			parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			parcel.id = "$parcel";
			parcel.mesh = NULL;
			parcel.tex = game->tPaczka;
			parcel.refid = refid;
			parcel.other_type = OtherItems;
			game->current_dialog->pc->unit->AddItem(&parcel, 1, true);
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[9];

			quest_index = game->quests.size();
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			Location& loc2 = *game->locations[start_loc];
			msgs.push_back(Format(game->txQuest[3], loc2.type == L_CITY ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[10], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), GetLocationDirName(loc2.pos, loc.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(rand2()%4 != 0)
			{
				Encounter* e = game->AddEncounter(enc);
				e->pos = (loc.pos+loc2.pos)/2;
				e->zasieg = 64;
				e->szansa = 45;
				e->dont_attack = true;
				e->dialog = deliver_parcel_bandits;
				e->grupa = SG_BANDYCI;
				e->text = game->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->location_event_handler = NULL;
			}

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&parcel);
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
			((City*)game->locations[start_loc])->quest_burmistrz = CityQuestState::Failed;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(125);

			msgs.push_back(game->txQuest[12]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveEncounter();

			if(game->IsOnline())
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
			((City*)game->locations[start_loc])->quest_burmistrz = CityQuestState::Failed;

			msgs.push_back(game->txQuest[13]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveEncounter();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// parcel delivered, end of quest
		{
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_burmistrz = CityQuestState::None;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(250);

			RemoveEncounter();

			msgs.push_back(game->txQuest[14]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
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

			if(game->IsOnline())
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

			if(game->IsOnline())
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
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (loc.type == L_CITY ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, loc.pos);
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IsTimedout()
{
	return game->worldtime - start_time > 15;
}

//=================================================================================================
bool Quest_DeliverParcel::IfHaveQuestItem()
{
	if(game->current_location == Game::Get().encounter_loc && prog == Progress::Started)
		return true;
	return game->current_location == end_loc && (prog == Progress::Started || prog == Progress::ParcelGivenToBandits);
}

//=================================================================================================
const Item* Quest_DeliverParcel::GetQuestItem()
{
	return &parcel;
}

//=================================================================================================
void Quest_DeliverParcel::Save(HANDLE file)
{
	Quest_Encounter::Save(file);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);
}

//=================================================================================================
void Quest_DeliverParcel::Load(HANDLE file)
{
	Quest_Encounter::Load(file);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);

		Location& loc = *game->locations[end_loc];
		parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.type = IT_OTHER;
		parcel.weight = 10;
		parcel.value = 0;
		parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
		parcel.id = "$parcel";
		parcel.mesh = NULL;
		parcel.tex = game->tPaczka;
		parcel.refid = refid;
		parcel.other_type = OtherItems;

		if(game->mp_load)
			game->Net_RegisterItem(&parcel);
	}

	if(enc != -1)
	{
		Location& loc = *game->locations[end_loc];
		Location& loc2 = *game->locations[start_loc];
		Encounter* e = game->RecreateEncounter(enc);
		e->pos = (loc.pos+loc2.pos)/2;
		e->zasieg = 64;
		e->szansa = 45;
		e->dont_attack = true;
		e->dialog = deliver_parcel_bandits;
		e->grupa = SG_BANDYCI;
		e->text = game->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->location_event_handler = NULL;
	}
}
