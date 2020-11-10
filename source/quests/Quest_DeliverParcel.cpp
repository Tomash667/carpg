#include "Pch.h"
#include "Quest_DeliverParcel.h"

#include "DialogContext.h"
#include "Encounter.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_DeliverParcel::Start()
{
	startLoc = world->GetCurrentLocation();
	end_loc = world->GetRandomSettlement(startLoc)->index;
	type = Q_DELIVER_PARCEL;
	category = QuestCategory::Mayor;
}

//=================================================================================================
GameDialog* Quest_DeliverParcel::GetDialog(int dialog_type)
{
	switch(dialog_type)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_deliver_parcel_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_deliver_parcel_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_deliver_parcel_give");
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
			OnStart(quest_mgr->txQuest[9]);
			quest_mgr->quests_timeout2.push_back(this);

			Location& loc = *world->GetLocation(end_loc);
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$parcel";
			parcel.name = Format(quest_mgr->txQuest[8], LocationHelper::IsCity(loc) ? quest_mgr->txForMayor : quest_mgr->txForSoltys, loc.name.c_str());
			parcel.quest_id = id;
			DialogContext::current->pc->unit->AddItem2(&parcel, 1u, 1u);

			msgs.push_back(Format(quest_mgr->txQuest[3], LocationHelper::IsCity(startLoc) ? quest_mgr->txForMayor : quest_mgr->txForSoltys, startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(quest_mgr->txQuest[10], LocationHelper::IsCity(loc) ? quest_mgr->txForMayor : quest_mgr->txForSoltys, loc.name.c_str(),
				GetLocationDirName(startLoc->pos, loc.pos)));

			if(Rand() % 4 != 0)
			{
				Encounter* e = world->AddEncounter(enc);
				e->pos = (loc.pos + startLoc->pos) / 2;
				e->range = 64;
				e->chance = 45;
				e->dont_attack = true;
				e->dialog = GameDialog::TryGet("q_deliver_parcel_bandits");
				e->group = UnitGroup::Get("bandits");
				e->text = quest_mgr->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->location_event_handler = nullptr;
				e->st = 6;
			}
		}
		break;
	case Progress::DeliverAfterTime:
		// player failed to deliver parcel in time, but gain some gold anyway
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->quest_mayor = CityQuestState::Failed;

			DialogContext::current->pc->unit->RemoveQuestItem(id);
			team->AddReward(300, 2000);

			OnUpdate(quest_mgr->txQuest[12]);
			RemoveElementTry(quest_mgr->quests_timeout2, static_cast<Quest*>(this));
			RemoveEncounter();
		}
		break;
	case Progress::Timeout:
		// player failed to deliver parcel in time
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->quest_mayor = CityQuestState::Failed;

			OnUpdate(quest_mgr->txQuest[13]);
			RemoveElementTry(quest_mgr->quests_timeout2, static_cast<Quest*>(this));
			RemoveEncounter();
		}
		break;
	case Progress::Finished:
		// parcel delivered, end of quest
		{
			state = Quest::Completed;
			static_cast<City*>(startLoc)->quest_mayor = CityQuestState::None;

			DialogContext::current->pc->unit->RemoveQuestItem(id);
			team->AddReward(750, 3000);

			RemoveEncounter();

			OnUpdate(quest_mgr->txQuest[14]);
			RemoveElementTry(quest_mgr->quests_timeout2, static_cast<Quest*>(this));
		}
		break;
	case Progress::AttackedBandits:
		// don't giver parcel to bandits, get attacked
		{
			RemoveEncounter();
			apply = false;

			OnUpdate(quest_mgr->txQuest[15]);
		}
		break;
	case Progress::ParcelGivenToBandits:
		// give parcel to bandits
		{
			RemoveEncounter();

			DialogContext::current->talker->AddItem(&parcel, 1, true);
			team->RemoveQuestItem(&parcel, id);

			OnUpdate(quest_mgr->txQuest[16]);
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
	Location& loc = *world->GetLocation(end_loc);
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? quest_mgr->txForMayor : quest_mgr->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(startLoc->pos, loc.pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 15;
}

//=================================================================================================
bool Quest_DeliverParcel::OnTimeout(TimeoutType ttype)
{
	if(ttype == TIMEOUT2)
		OnUpdate(quest_mgr->txQuest[267]);
	return true;
}

//=================================================================================================
bool Quest_DeliverParcel::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_deliver_parcel_after") == 0)
		return world->GetWorldtime() - start_time < 30 && Rand() % 2 == 0;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_DeliverParcel::IfHaveQuestItem() const
{
	if(world->GetCurrentLocation() == world->GetEncounterLocation() && prog == Progress::Started)
		return true;
	return world->GetCurrentLocationIndex() == end_loc && (prog == Progress::Started || prog == Progress::ParcelGivenToBandits);
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
Quest::LoadResult Quest_DeliverParcel::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
	{
		f >> end_loc;
		if(prog >= Progress::Started)
		{
			Location& loc = *world->GetLocation(end_loc);
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$parcel";
			parcel.name = Format(quest_mgr->txQuest[8], LocationHelper::IsCity(loc) ? quest_mgr->txForMayor : quest_mgr->txForSoltys, loc.name.c_str());
			parcel.quest_id = id;
		}
	}

	if(enc != -1)
	{
		Location& loc = *world->GetLocation(end_loc);
		Encounter* e = world->RecreateEncounter(enc);
		e->pos = (loc.pos + startLoc->pos) / 2;
		e->range = 64;
		e->chance = 45;
		e->dont_attack = true;
		e->dialog = GameDialog::TryGet("q_deliver_parcel_bandits");
		e->group = UnitGroup::Get("bandits");
		e->text = quest_mgr->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->location_event_handler = nullptr;
		e->st = 6;
	}

	return LoadResult::Ok;
}
