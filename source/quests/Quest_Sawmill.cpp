#include "Pch.h"
#include "Quest_Sawmill.h"

#include "Game.h"
#include "Journal.h"
#include "Level.h"
#include "Object.h"
#include "OutsideLocation.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

#include <Terrain.h>

const int PAYMENT = 500;

//=================================================================================================
void Quest_Sawmill::Start()
{
	type = Q_SAWMILL;
	category = QuestCategory::Unique;
	sawmill_state = State::None;
	build_state = BuildState::None;
	days = 0;
	startLoc = world->GetRandomSettlement(quest_mgr->GetUsedCities());
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[0], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Sawmill' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Sawmill::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(DialogContext::current->talker->data->id == "artur_drwal")
			return GameDialog::TryGet("q_sawmill_talk");
		else
			return GameDialog::TryGet("q_sawmill_messenger");
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Sawmill::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(quest_mgr->txQuest[124]);

			location_event_handler = this;

			targetLoc = world->GetClosestLocation(L_OUTSIDE, startLoc->pos, FOREST);
			at_level = 0;
			targetLoc->active_quest = this;
			targetLoc->SetKnown();
			if(targetLoc->state >= LS_ENTERED)
				targetLoc->reset = true;
			targetLoc->st = 8;

			msgs.push_back(Format(quest_mgr->txQuest[125], startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(quest_mgr->txQuest[126], targetLoc->name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		{
			OnUpdate(Format(quest_mgr->txQuest[127], GetTargetLocationName()));
			team->AddExp(3000);
		}
		break;
	case Progress::Talked:
		{
			days = 0;
			sawmill_state = State::InBuild;
			quest_mgr->RemoveQuestRumor(id);
			OnUpdate(quest_mgr->txQuest[128]);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			sawmill_state = State::Working;

			OnUpdate(quest_mgr->txQuest[129]);
			team->AddReward(PAYMENT);
			team->AddInvestment(quest_mgr->txQuest[124], id, PAYMENT);
			quest_mgr->EndUniqueQuest();
			world->AddNews(Format(quest_mgr->txQuest[130], targetLoc->name.c_str()));
			targetLoc->SetImage(LI_SAWMILL);
			targetLoc->SetNamePrefix(quest_mgr->txQuest[124]);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Sawmill::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "payment")
		return Format("%d", PAYMENT);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Sawmill::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "tartak") == 0;
}

//=================================================================================================
bool Quest_Sawmill::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "czy_tartak") == 0)
		return world->GetCurrentLocation() == targetLoc;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Sawmill::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::Started && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::ClearedLocation);
	return false;
}

//=================================================================================================
void Quest_Sawmill::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << sawmill_state;
	f << build_state;
	f << days;
	f << messenger;
	if(sawmill_state != State::None && build_state != BuildState::Finished)
		f << hd_lumberjack;
}

//=================================================================================================
Quest::LoadResult Quest_Sawmill::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	location_event_handler = this;

	f >> sawmill_state;
	f >> build_state;
	f >> days;
	f >> messenger;
	if(sawmill_state != State::None && build_state != BuildState::Finished)
		f >> hd_lumberjack;

	if(LOAD_VERSION < V_0_18 && sawmill_state == State::Working)
		team->AddInvestment(quest_mgr->txQuest[124], id, PAYMENT, days);

	return LoadResult::Ok;
}

//=================================================================================================
cstring sawmill_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"torch",
	"torch_off",
	"firewood"
};
const uint n_sawmill_objs = countof(sawmill_objs);
BaseObject* sawmill_objs_ptrs[n_sawmill_objs];

void Quest_Sawmill::GenerateSawmill(bool in_progress)
{
	OutsideLocation& outside = *(OutsideLocation*)game_level->location;
	DeleteElements(outside.units);
	outside.bloods.clear();

	// smooth terrain
	float* h = outside.h;
	const int _s = outside.size + 1;
	vector<Int2> tiles;
	float height = 0.f;
	for(int y = 64 - 6; y < 64 + 6; ++y)
	{
		for(int x = 64 - 6; x < 64 + 6; ++x)
		{
			if(Vec2::Distance(Vec2(2.f*x + 1.f, 2.f*y + 1.f), Vec2(128, 128)) < 8.f)
			{
				height += h[x + y * _s];
				tiles.push_back(Int2(x, y));
			}
		}
	}
	height /= tiles.size();
	for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		h[it->x + it->y*_s] = height;
	game_level->terrain->Rebuild(true);

	// remove objects
	DeleteElements(outside.objects, [](const Object* obj) { return Vec3::Distance2d(obj->pos, Vec3(128, 0, 128)) < 16.f; });

	if(!sawmill_objs_ptrs[0])
	{
		for(uint i = 0; i < n_sawmill_objs; ++i)
			sawmill_objs_ptrs[i] = BaseObject::Get(sawmill_objs[i]);
	}

	UnitData& ud = *UnitData::Get("artur_drwal");
	UnitData& ud2 = *UnitData::Get("drwal");

	if(in_progress)
	{
		// arthur
		Unit* u = game_level->SpawnUnitNearLocation(outside, Vec3(128, 0, 128), ud, nullptr, -2);
		u->hero->know_name = true;
		u->ApplyHumanData(hd_lumberjack);

		// spawn objects
		for(int i = 0; i < 25; ++i)
		{
			Vec2 pt = Vec2::Random(Vec2(128 - 16, 128 - 16), Vec2(128 + 16, 128 + 16));
			BaseObject* obj = sawmill_objs_ptrs[Rand() % n_sawmill_objs];
			game_level->SpawnObjectNearLocation(outside, obj, pt, Random(MAX_ANGLE), 2.f);
		}

		// spawn other woodcutters
		int count = Random(5, 10);
		for(int i = 0; i < count; ++i)
			game_level->SpawnUnitNearLocation(outside, Vec3::Random(Vec3(128 - 16, 0, 128 - 16), Vec3(128 + 16, 0, 128 + 16)), ud2, nullptr, -2);

		build_state = BuildState::InProgress;
	}
	else
	{
		// building
		Vec3 spawn_pt;
		float rot = PI / 2 * (Rand() % 4);
		game_level->SpawnObjectEntity(outside, BaseObject::Get("tartak"), Vec3(128, height, 128), rot, 1.f, 0, &spawn_pt);

		// arthur
		Unit* u = game_level->SpawnUnitNearLocation(outside, spawn_pt, ud, nullptr, -2);
		u->rot = rot;
		u->hero->know_name = true;
		u->ApplyHumanData(hd_lumberjack);

		// spawn objects
		for(int i = 0; i < 25; ++i)
		{
			Vec2 pt = Vec2::Random(Vec2(128 - 16, 128 - 16), Vec2(128 + 16, 128 + 16));
			BaseObject* obj = sawmill_objs_ptrs[Rand() % n_sawmill_objs];
			game_level->SpawnObjectNearLocation(outside, obj, pt, Random(MAX_ANGLE), 2.f);
		}

		// spawn other woodcutters
		int count = Random(5, 10);
		for(int i = 0; i < count; ++i)
			game_level->SpawnUnitNearLocation(outside, Vec3::Random(Vec3(128 - 16, 0, 128 - 16), Vec3(128 + 16, 0, 128 + 16)), ud2, nullptr, -2);

		build_state = BuildState::Finished;
	}
}

//=================================================================================================
void Quest_Sawmill::OnProgress(int d)
{
	if(sawmill_state == State::InBuild)
	{
		days += d;
		if(days >= 30 && game_level->city_ctx && game->game_state == GS_LEVEL)
		{
			days = 29;
			Unit* u = game_level->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("poslaniec_tartak"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				messenger = u;
				u->OrderAutoTalk(true);
			}
		}
	}
}
