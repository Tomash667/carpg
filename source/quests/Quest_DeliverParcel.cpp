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
	endLoc = world->GetRandomSettlement(startLoc)->index;
	type = Q_DELIVER_PARCEL;
	category = QuestCategory::Mayor;
}

//=================================================================================================
GameDialog* Quest_DeliverParcel::GetDialog(int dialogType)
{
	switch(dialogType)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("qDeliverParcelStart");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("qDeliverParcelTimeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("qDeliverParcelGive");
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
			OnStart(questMgr->txQuest[9]);
			questMgr->questTimeouts2.push_back(this);

			Location& loc = *world->GetLocation(endLoc);
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$parcel";
			parcel.name = Format(questMgr->txQuest[8], LocationHelper::IsCity(loc) ? questMgr->txForMayor : questMgr->txForSoltys, loc.name.c_str());
			parcel.questId = id;
			DialogContext::current->pc->unit->AddItem2(&parcel, 1u, 1u);

			msgs.push_back(Format(questMgr->txQuest[3], LocationHelper::IsCity(startLoc) ? questMgr->txForMayor : questMgr->txForSoltys, startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[10], LocationHelper::IsCity(loc) ? questMgr->txForMayor : questMgr->txForSoltys, loc.name.c_str(),
				GetLocationDirName(startLoc->pos, loc.pos)));

			if(Rand() % 4 != 0)
			{
				Encounter* e = world->AddEncounter(enc);
				e->pos = (loc.pos + startLoc->pos) / 2;
				e->range = 64;
				e->chance = 45;
				e->dontAttack = true;
				e->dialog = GameDialog::TryGet("qDeliverParcelBandits");
				e->group = UnitGroup::Get("bandits");
				e->text = questMgr->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->locationEventHandler = nullptr;
				e->st = 6;
			}
		}
		break;
	case Progress::DeliverAfterTime:
		// player failed to deliver parcel in time, but gain some gold anyway
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::Failed;

			DialogContext::current->pc->unit->RemoveQuestItem(id);
			team->AddReward(300, 2000);

			OnUpdate(questMgr->txQuest[12]);
			RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
			RemoveEncounter();
		}
		break;
	case Progress::Timeout:
		// player failed to deliver parcel in time
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::Failed;

			OnUpdate(questMgr->txQuest[13]);
			RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
			RemoveEncounter();
		}
		break;
	case Progress::Finished:
		// parcel delivered, end of quest
		{
			state = Quest::Completed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::None;

			DialogContext::current->pc->unit->RemoveQuestItem(id);
			team->AddReward(750, 3000);

			RemoveEncounter();

			OnUpdate(questMgr->txQuest[14]);
			RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
		}
		break;
	case Progress::AttackedBandits:
		// don't giver parcel to bandits, get attacked
		{
			RemoveEncounter();
			apply = false;

			OnUpdate(questMgr->txQuest[15]);
		}
		break;
	case Progress::ParcelGivenToBandits:
		// give parcel to bandits
		{
			RemoveEncounter();

			DialogContext::current->talker->AddItem(&parcel, 1, true);
			team->RemoveQuestItem(&parcel, id);

			OnUpdate(questMgr->txQuest[16]);
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
	Location& loc = *world->GetLocation(endLoc);
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? questMgr->txForMayor : questMgr->txForSoltys);
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
	return world->GetWorldtime() - startTime >= 15;
}

//=================================================================================================
bool Quest_DeliverParcel::OnTimeout(TimeoutType ttype)
{
	if(ttype == TIMEOUT2)
		OnUpdate(questMgr->txQuest[267]);
	return true;
}

//=================================================================================================
bool Quest_DeliverParcel::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_deliver_parcel_after") == 0)
		return world->GetWorldtime() - startTime < 30 && Rand() % 2 == 0;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_DeliverParcel::IfHaveQuestItem() const
{
	if(world->GetCurrentLocation() == world->GetEncounterLocation() && prog == Progress::Started)
		return true;
	return world->GetCurrentLocationIndex() == endLoc && (prog == Progress::Started || prog == Progress::ParcelGivenToBandits);
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
		f << endLoc;
}

//=================================================================================================
Quest::LoadResult Quest_DeliverParcel::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
	{
		f >> endLoc;
		if(prog >= Progress::Started)
		{
			Location& loc = *world->GetLocation(endLoc);
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$parcel";
			parcel.name = Format(questMgr->txQuest[8], LocationHelper::IsCity(loc) ? questMgr->txForMayor : questMgr->txForSoltys, loc.name.c_str());
			parcel.questId = id;
		}
	}

	if(enc != -1)
	{
		Location& loc = *world->GetLocation(endLoc);
		Encounter* e = world->RecreateEncounter(enc);
		e->pos = (loc.pos + startLoc->pos) / 2;
		e->range = 64;
		e->chance = 45;
		e->dontAttack = true;
		e->dialog = GameDialog::TryGet("qDeliverParcelBandits");
		e->group = UnitGroup::Get("bandits");
		e->text = questMgr->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->locationEventHandler = nullptr;
		e->st = 6;
	}

	return LoadResult::Ok;
}
