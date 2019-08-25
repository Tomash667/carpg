#include "Pch.h"
#include "GameCore.h"
#include "Quest_BanditsCollectToll.h"
#include "Game.h"
#include "Journal.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "City.h"
#include "SoundManager.h"
#include "GameFile.h"
#include "World.h"
#include "Team.h"

//=================================================================================================
void Quest_BanditsCollectToll::Start()
{
	type = Q_BANDITS_COLLECT_TOLL;
	category = QuestCategory::Captain;
	start_loc = world->GetCurrentLocationIndex();
	other_loc = world->GetRandomSettlementIndex(start_loc);
}

//=================================================================================================
GameDialog* Quest_BanditsCollectToll::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_bandits_collect_toll_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_bandits_collect_toll_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_bandits_collect_toll_end");
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
			OnStart(game->txQuest[51]);

			Location& sl = GetStartLocation();
			Location& ol = *world->GetLocation(other_loc);

			Encounter* e = world->AddEncounter(enc);
			e->dialog = GameDialog::TryGet("q_bandits_collect_toll_talk");
			e->dont_attack = true;
			e->group = UnitGroup::Get("bandits");
			e->pos = (sl.pos + ol.pos) / 2;
			e->quest = this;
			e->chance = 50;
			e->text = game->txQuest[52];
			e->timed = true;
			e->range = 64;
			e->location_event_handler = this;
			e->st = 6;

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[53], sl.name.c_str(), ol.name.c_str(), GetLocationDirName(sl.pos, ol.pos)));
		}
		break;
	case Progress::Timout:
		// player failed to kill bandits in time
		{
			state = Quest::Failed;
			RemoveEncounter();
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
			OnUpdate(game->txQuest[54]);
		}
		break;
	case Progress::KilledBandits:
		// player killed bandits
		{
			OnUpdate(game->txQuest[55]);
			RemoveEncounter();
		}
		break;
	case Progress::Finished:
		// player talked with captain after killing bandits, end of quest
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[56]);
			team->AddReward(1500, 4500);
			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			world->AddNews(game->txQuest[278]);
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
		return world->GetLocation(other_loc)->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(GetStartLocation().pos, world->GetLocation(other_loc)->pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_BanditsCollectToll::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 15;
}

//=================================================================================================
bool Quest_BanditsCollectToll::OnTimeout(TimeoutType ttype)
{
	OnUpdate(game->txQuest[277]);
	return true;
}

//=================================================================================================
bool Quest_BanditsCollectToll::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "pay_500") == 0)
	{
		ctx.pc->unit->ModGold(-500);
		ctx.talker->gold += 500;
	}
	else
		assert(0);
	return false;
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
	return strcmp(topic, "road_bandits") == 0 && prog == Progress::KilledBandits && world->GetCurrentLocationIndex() == start_loc;
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
		Location& ol = *world->GetLocation(other_loc);

		Encounter* e = world->RecreateEncounter(enc);
		e->dialog = GameDialog::TryGet("q_bandits_collect_toll_talk");
		e->dont_attack = true;
		e->group = UnitGroup::Get("bandits");
		e->pos = (sl.pos + ol.pos) / 2;
		e->quest = this;
		e->chance = 50;
		e->text = game->txQuest[52];
		e->timed = true;
		e->range = 64;
		e->location_event_handler = this;
		e->st = 6;
	}

	return true;
}
