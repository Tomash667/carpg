#include "Pch.h"
#include "GameCore.h"
#include "Quest_Sawmill.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"
#include "QuestManager.h"
#include "World.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "Terrain.h"
#include "Team.h"

//=================================================================================================
void Quest_Sawmill::Start()
{
	quest_id = Q_SAWMILL;
	type = QuestType::Unique;
	sawmill_state = State::None;
	build_state = BuildState::None;
	days = 0;
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
	case Progress::NotAccepted:
		// nie zaakceptowano
		break;
	case Progress::Started:
		// zakceptowano
		{
			OnStart(game->txQuest[124]);

			location_event_handler = this;

			Location& sl = GetStartLocation();
			target_loc = W.GetClosestLocation(L_FOREST, sl.pos);
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			tl.SetKnown();
			if(tl.state >= LS_ENTERED)
				tl.reset = true;
			tl.st = 8;

			msgs.push_back(Format(game->txQuest[125], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[126], tl.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		// oczyszczono
		{
			OnUpdate(Format(game->txQuest[127], GetTargetLocationName()));
			Team.AddExp(3000);
		}
		break;
	case Progress::Talked:
		// poinformowano
		{
			days = 0;
			sawmill_state = State::InBuild;

			quest_manager.RemoveQuestRumor(R_SAWMILL);

			OnUpdate(game->txQuest[128]);
		}
		break;
	case Progress::Finished:
		// pierwsza kasa
		{
			state = Quest::Completed;
			sawmill_state = State::Working;
			days = 0;

			OnUpdate(game->txQuest[129]);
			game->AddReward(PAYMENT);
			quest_manager.EndUniqueQuest();
			Location& target = GetTargetLocation();
			W.AddNews(Format(game->txQuest[130], target.name.c_str()));
			target.SetImage(LI_SAWMILL);
			target.SetNamePrefix(game->txQuest[124]);
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
		return W.GetCurrentLocationIndex() == target_loc;
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
bool Quest_Sawmill::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	location_event_handler = this;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> sawmill_state;
		f >> build_state;
		f >> days;
		f >> messenger;
		if(sawmill_state != State::None && build_state != BuildState::Finished)
			f >> hd_lumberjack;
	}

	return true;
}

//=================================================================================================
void Quest_Sawmill::LoadOld(GameReader& f)
{
	int city, forest;

	f >> city;
	f >> sawmill_state;
	f >> build_state;
	f >> days;
	f >> refid;
	f >> forest;
	f >> messenger;
	if(sawmill_state != State::None && build_state != BuildState::InProgress)
		f >> hd_lumberjack;
	else if(sawmill_state != State::None && build_state == BuildState::InProgress)
	{
		// fix for missing human data
		Unit* u = ForLocation(target_loc)->FindUnit(UnitData::Get("artur_drwal"));
		if(u)
			hd_lumberjack.Get(*u->human_data);
		else
			hd_lumberjack.Random();
	}
}

//=================================================================================================
cstring tartak_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"torch",
	"torch_off",
	"firewood"
};
const uint n_tartak_objs = countof(tartak_objs);
BaseObject* tartak_objs_ptrs[n_tartak_objs];

void Quest_Sawmill::GenerateSawmill(bool in_progress)
{
	for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
		delete *it;
	L.local_ctx.units->clear();
	L.local_ctx.bloods->clear();

	// wyrównaj teren
	OutsideLocation* outside = (OutsideLocation*)L.location;
	float* h = outside->h;
	const int _s = outside->size + 1;
	vector<Int2> tiles;
	float wys = 0.f;
	for(int y = 64 - 6; y < 64 + 6; ++y)
	{
		for(int x = 64 - 6; x < 64 + 6; ++x)
		{
			if(Vec2::Distance(Vec2(2.f*x + 1.f, 2.f*y + 1.f), Vec2(128, 128)) < 8.f)
			{
				wys += h[x + y * _s];
				tiles.push_back(Int2(x, y));
			}
		}
	}
	wys /= tiles.size();
	for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		h[it->x + it->y*_s] = wys;
	L.terrain->Rebuild(true);

	// usuñ obiekty
	LoopAndRemove(*L.local_ctx.objects, [](const Object* obj)
	{
		if(Vec3::Distance2d(obj->pos, Vec3(128, 0, 128)) < 16.f)
		{
			delete obj;
			return true;
		}
		return false;
	});

	if(!tartak_objs_ptrs[0])
	{
		for(uint i = 0; i < n_tartak_objs; ++i)
			tartak_objs_ptrs[i] = BaseObject::Get(tartak_objs[i]);
	}

	UnitData& ud = *UnitData::Get("artur_drwal");
	UnitData& ud2 = *UnitData::Get("drwal");

	if(in_progress)
	{
		// artur drwal
		Unit* u = L.SpawnUnitNearLocation(L.local_ctx, Vec3(128, 0, 128), ud, nullptr, -2);
		assert(u);
		u->rot = Random(MAX_ANGLE);
		u->hero->name = game->txArthur;
		u->hero->know_name = true;
		u->ApplyHumanData(hd_lumberjack);

		// generuj obiekty
		for(int i = 0; i < 25; ++i)
		{
			Vec2 pt = Vec2::Random(Vec2(128 - 16, 128 - 16), Vec2(128 + 16, 128 + 16));
			BaseObject* obj = tartak_objs_ptrs[Rand() % n_tartak_objs];
			L.SpawnObjectNearLocation(L.local_ctx, obj, pt, Random(MAX_ANGLE), 2.f);
		}

		// generuj innych drwali
		int count = Random(5, 10);
		for(int i = 0; i < count; ++i)
		{
			Unit* u = L.SpawnUnitNearLocation(L.local_ctx, Vec3::Random(Vec3(128 - 16, 0, 128 - 16), Vec3(128 + 16, 0, 128 + 16)), ud2, nullptr, -2);
			if(u)
				u->rot = Random(MAX_ANGLE);
		}

		build_state = BuildState::InProgress;
	}
	else
	{
		// budynek
		Vec3 spawn_pt;
		float rot = PI / 2 * (Rand() % 4);
		L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("tartak"), Vec3(128, wys, 128), rot, 1.f, 0, &spawn_pt);

		// artur drwal
		Unit* u = L.SpawnUnitNearLocation(L.local_ctx, spawn_pt, ud, nullptr, -2);
		assert(u);
		u->rot = rot;
		u->hero->name = game->txArthur;
		u->hero->know_name = true;
		u->ApplyHumanData(hd_lumberjack);

		// obiekty
		for(int i = 0; i < 25; ++i)
		{
			Vec2 pt = Vec2::Random(Vec2(128 - 16, 128 - 16), Vec2(128 + 16, 128 + 16));
			BaseObject* obj = tartak_objs_ptrs[Rand() % n_tartak_objs];
			L.SpawnObjectNearLocation(L.local_ctx, obj, pt, Random(MAX_ANGLE), 2.f);
		}

		// inni drwale
		int count = Random(5, 10);
		for(int i = 0; i < count; ++i)
		{
			Unit* u = L.SpawnUnitNearLocation(L.local_ctx, Vec3::Random(Vec3(128 - 16, 0, 128 - 16), Vec3(128 + 16, 0, 128 + 16)), ud2, nullptr, -2);
			if(u)
				u->rot = Random(MAX_ANGLE);
		}

		build_state = BuildState::Finished;
	}
}
