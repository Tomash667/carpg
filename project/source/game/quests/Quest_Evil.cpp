#include "Pch.h"
#include "GameCore.h"
#include "Quest_Evil.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "AIController.h"
#include "Team.h"
#include "Portal.h"
#include "World.h"
#include "Level.h"
#include "Pathfinding.h"
#include "ResourceManager.h"
#include "ParticleSystem.h"

//=================================================================================================
void Quest_Evil::Init()
{
	QM.RegisterSpecialIfHandler(this, "q_zlo_kapitan");
	QM.RegisterSpecialIfHandler(this, "q_zlo_burmistrz");
}

//=================================================================================================
void Quest_Evil::Start()
{
	type = QuestType::Unique;
	quest_id = Q_EVIL;
	mage_loc = -1;
	closed = 0;
	changed = false;
	for(int i = 0; i < 3; ++i)
	{
		loc[i].state = Loc::State::None;
		loc[i].target_loc = -1;
	}
	told_about_boss = false;
	evil_state = State::None;
	cleric = nullptr;
}

//=================================================================================================
GameDialog* Quest_Evil::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = game->current_dialog->talker->data->id;

	if(id == "q_zlo_kaplan")
		return FindDialog("q_evil_cleric");
	else if(id == "q_zlo_mag")
		return FindDialog("q_evil_mage");
	else if(id == "q_zlo_boss")
		return FindDialog("q_evil_boss");
	else if(id == "guard_captain")
		return FindDialog("q_evil_captain");
	else if(id == "mayor" || id == "soltys")
		return FindDialog("q_evil_mayor");
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Evil::SetProgress(int prog2)
{
	bool apply = true;

	switch(prog2)
	{
	case Progress::NotAccepted:
		// nie zaakceptowano
		{
			if(!quest_manager.RemoveQuestRumor(P_ZLO))
				game->game_gui->journal->AddRumor(Format(game->txQuest[232], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		// zaakceptowano
		break;
	case Progress::Talked:
		// zaakceptowano
		{
			OnStart(game->txQuest[233]);
			// usuñ plotkê
			quest_manager.RemoveQuestRumor(P_ZLO);
			// lokacja
			Location& target = *W.CreateLocation(L_DUNGEON, W.GetWorldPos(), 128.f, OLD_TEMPLE, SG_NONE, false, 1);
			target.SetKnown();
			target.st = 8;
			target.active_quest = this;
			target.dont_clean = true;
			target_loc = target.index;
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
			at_level = 0;
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[234], GetStartLocationName(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::AltarEvent:
		// zdarzenie
		{
			OnUpdate(game->txQuest[236]);
			W.AddNews(Format(game->txQuest[237], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedAboutBook:
		// powiedzia³ o ksiêdze
		{
			mage_loc = W.GetRandomSettlementIndex(start_loc);
			Location& mage = *W.GetLocation(mage_loc);
			OnUpdate(Format(game->txQuest[238], mage.name.c_str(), GetLocationDirName(GetStartLocation().pos, mage.pos)));
			evil_state = State::GenerateMage;
		}
		break;
	case Progress::MageToldAboutStolenBook:
		// mag powiedzia³ ¿e mu zabrali ksiêge
		{
			OnUpdate(game->txQuest[239]);
			W.AddNews(Format(game->txQuest[240], W.GetLocation(mage_loc)->name.c_str()));
			game->current_dialog->talker->temporary = true;
		}
		break;
	case Progress::TalkedWithCaptain:
		// pogadano z kapitanem
		{
			OnUpdate(Format(game->txQuest[241], LocationHelper::IsCity(W.GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::TalkedWithMayor:
		// pogadano z burmistrzem
		{
			OnUpdate(Format(game->txQuest[242], LocationHelper::IsCity(W.GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::GotBook:
		// dosta³eœ ksi¹¿ke
		{
			OnUpdate(game->txQuest[243]);
			const Item* item = Item::Get("q_zlo_ksiega");
			game->current_dialog->pc->unit->AddItem2(item, 1u, 1u);
		}
		break;
	case Progress::GivenBook:
		// da³eœ ksi¹¿kê jozanowi
		{
			// dodaj lokacje
			const struct
			{
				LOCATION type;
				int target;
				SPAWN_GROUP spawn;
				int st;
			} l_info[3] = {
				L_DUNGEON, OLD_TEMPLE, SG_EVIL, 15,
				L_DUNGEON, NECROMANCER_BASE, SG_NECROMANCERS, 14,
				L_CRYPT, MONSTER_CRYPT, SG_UNDEAD, 13
			};

			cstring new_msgs[4];
			new_msgs[0] = game->txQuest[245];

			for(int i = 0; i < 3; ++i)
			{
				Int2 levels = g_base_locations[l_info[i].target].levels;
				Location& target = *W.CreateLocation(l_info[i].type, Vec2(0, 0), -128.f, l_info[i].target, l_info[i].spawn, true,
					Random(max(levels.x, 2), max(levels.y, 2)));
				target.st = l_info[i].st;
				target.SetKnown();
				target.active_quest = this;
				loc[i].target_loc = target.index;
				loc[i].near_loc = W.GetNearestSettlement(target.pos);
				loc[i].at_level = target.GetLastLevel();
				loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
				new_msgs[i + 1] = Format(game->txQuest[247], W.GetLocation(loc[i].target_loc)->name.c_str(),
					GetLocationDirName(W.GetLocation(loc[i].near_loc)->pos, W.GetLocation(loc[i].target_loc)->pos),
					W.GetLocation(loc[i].near_loc)->name.c_str());
				W.AddNews(Format(game->txQuest[246], W.GetLocation(loc[i].target_loc)->name.c_str()));
			}

			OnUpdate({ new_msgs[0], new_msgs[1], new_msgs[2], new_msgs[3] });

			next_event = &loc[0];
			loc[0].next_event = &loc[1];
			loc[1].next_event = &loc[2];

			// dodaj jozana do dru¿yny
			Unit& u = *game->current_dialog->talker;
			const Item* item = Item::Get("q_zlo_ksiega");
			u.AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			Team.AddTeamMember(&u, true);

			evil_state = State::ClosingPortals;
		}
		break;
	case Progress::PortalClosed:
		// u¿ywane tylko do czyszczenia flagi changed
		apply = false;
		changed = false;
		break;
	case Progress::AllPortalsClosed:
		// zamkniêto wszystkie portale
		{
			changed = false;
			done = false;
			unit_to_spawn = UnitData::Get("q_zlo_boss");
			spawn_unit_room = RoomTarget::Treasury;
			unit_event_handler = this;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
			at_level = 0;
			Location& target = GetTargetLocation();
			target.st = 15;
			target.spawn = SG_EVIL;
			target.reset = true;
			evil_state = State::KillBoss;
			OnUpdate(Format(game->txQuest[248], GetTargetLocationName()));
			for(int i = 0; i < 3; ++i)
				W.GetLocation(loc[i].target_loc)->active_quest = nullptr;
		}
		break;
	case Progress::KilledBoss:
		// zabito bossa
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[249]);
			// przywróæ stary o³tarz
			Location& target = GetTargetLocation();
			target.active_quest = nullptr;
			target.dont_clean = false;
			BaseObject* base_obj = BaseObject::Get("bloody_altar");
			Object* obj = L.local_ctx.FindObject(base_obj);
			obj->base = BaseObject::Get("altar");
			obj->mesh = obj->base->mesh;
			ResourceManager::Get<Mesh>().Load(obj->mesh);
			// usuñ cz¹steczki
			float best_dist = 999.f;
			ParticleEmitter* pe = nullptr;
			for(vector<ParticleEmitter*>::iterator it = L.local_ctx.pes->begin(), end = L.local_ctx.pes->end(); it != end; ++it)
			{
				if((*it)->tex == game->tKrew[BLOOD_RED])
				{
					float dist = Vec3::Distance((*it)->pos, obj->pos);
					if(dist < best_dist)
					{
						best_dist = dist;
						pe = *it;
					}
				}
			}
			assert(pe);
			pe->destroy = true;
			// gadanie przez jozana
			Unit* unit = Team.FindTeamMember("q_zlo_kaplan");
			if(unit)
				unit->StartAutoTalk();

			quest_manager.EndUniqueQuest();
			evil_state = State::ClericWantTalk;
			W.AddNews(game->txQuest[250]);

			if(Net::IsOnline())
				Net::PushChange(NetChange::CLEAN_ALTAR);
		}
		break;
	case Progress::Finished:
		// pogadano z jozanem
		{
			evil_state = State::ClericLeaving;
			// usuñ jozana z dru¿yny
			Unit& u = *game->current_dialog->talker;
			Team.RemoveTeamMember(&u);
			u.hero->mode = HeroData::Leave;
		}
		break;
	}

	if(apply)
		prog = prog2;
}

//=================================================================================================
cstring Quest_Evil::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "mage_loc")
		return W.GetLocation(mage_loc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, W.GetLocation(mage_loc)->pos);
	else if(str == "burmistrza")
		return LocationHelper::IsCity(W.GetCurrentLocation()) ? game->txForMayor : game->txForSoltys;
	else if(str == "burmistrzem")
		return LocationHelper::IsCity(W.GetCurrentLocation()) ? game->txQuest[251] : game->txQuest[252];
	else if(str == "t1")
		return W.GetLocation(loc[0].target_loc)->name.c_str();
	else if(str == "t2")
		return W.GetLocation(loc[1].target_loc)->name.c_str();
	else if(str == "t3")
		return W.GetLocation(loc[2].target_loc)->name.c_str();
	else if(str == "t1n")
		return W.GetLocation(loc[0].near_loc)->name.c_str();
	else if(str == "t2n")
		return W.GetLocation(loc[1].near_loc)->name.c_str();
	else if(str == "t3n")
		return W.GetLocation(loc[2].near_loc)->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(W.GetLocation(loc[0].near_loc)->pos, W.GetLocation(loc[0].target_loc)->pos);
	else if(str == "t2d")
		return GetLocationDirName(W.GetLocation(loc[1].near_loc)->pos, W.GetLocation(loc[1].target_loc)->pos);
	else if(str == "t3d")
		return GetLocationDirName(W.GetLocation(loc[2].near_loc)->pos, W.GetLocation(loc[2].target_loc)->pos);
	else if(str == "close_dir")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::State::PortalClosed)
			{
				float dist = Vec2::Distance(W.GetWorldPos(), W.GetLocation(loc[i].target_loc)->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = i;
				}
			}
		}
		Loc& l = loc[best_index];
		return GetLocationDirName(W.GetWorldPos(), W.GetLocation(l.target_loc)->pos);
	}
	else if(str == "close_loc")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::State::PortalClosed)
			{
				float dist = Vec2::Distance(W.GetWorldPos(), W.GetLocation(loc[i].target_loc)->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = loc[i].target_loc;
				}
			}
		}
		return W.GetLocation(best_index)->name.c_str();
	}
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Evil::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "zlo") == 0;
}

//=================================================================================================
bool Quest_Evil::IfQuestEvent() const
{
	return changed;
}

//=================================================================================================
bool Quest_Evil::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_zlo_clear1") == 0)
		return evil_state == State::ClosingPortals && closed == 1;
	else if(strcmp(msg, "q_zlo_clear2") == 0)
		return evil_state == State::ClosingPortals && closed == 2;
	else if(strcmp(msg, "q_zlo_tutaj") == 0)
	{
		if(prog == Progress::GivenBook)
		{
			int d = GetLocId(W.GetCurrentLocationIndex());
			if(d != -1)
			{
				if(loc[d].state != 3)
					return true;
			}
		}
		else if(prog == Progress::AllPortalsClosed)
		{
			if(W.GetCurrentLocationIndex() == target_loc)
				return true;
		}
		return false;
	}
	else if(strcmp(msg, "q_zlo_kapitan") == 0)
	{
		if(W.GetCurrentLocationIndex() == mage_loc
			&& evil_state >= State::GeneratedMage
			&& evil_state < State::ClosingPortals
			&& InRange((Progress)prog, Progress::MageToldAboutStolenBook, Progress::TalkedWithMayor))
			return true;
	}
	else if(strcmp(msg, "q_zlo_burmistrz") == 0)
	{
		if(W.GetCurrentLocationIndex() == mage_loc
			&& evil_state >= State::GeneratedMage
			&& evil_state < State::ClosingPortals
			&& prog == Progress::TalkedWithCaptain)
			return true;
	}
	return false;
}

//=================================================================================================
void Quest_Evil::HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit)
{
	assert(unit);
	if(event_type == UnitEventHandler::DIE && prog == Progress::AllPortalsClosed)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Evil::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << mage_loc;
	for(int i = 0; i < 3; ++i)
	{
		f << loc[i].target_loc;
		f << loc[i].done;
		f << loc[i].at_level;
		f << loc[i].near_loc;
		f << loc[i].state;
		f << loc[i].pos;
	}
	f << closed;
	f << changed;
	f << told_about_boss;
	f << evil_state;
	f << pos;
	f << timer;
	f << cleric;
}

//=================================================================================================
bool Quest_Evil::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> mage_loc;
	for(int i = 0; i < 3; ++i)
	{
		f >> loc[i].target_loc;
		f >> loc[i].done;
		f >> loc[i].at_level;
		f >> loc[i].near_loc;
		f >> loc[i].state;
		f >> loc[i].pos;
		loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
	}
	f >> closed;
	f >> changed;
	f >> told_about_boss;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> evil_state;
		f >> pos;
		f >> timer;
		f >> cleric;
	}

	next_event = &loc[0];
	loc[0].next_event = &loc[1];
	loc[1].next_event = &loc[2];

	if(!done)
	{
		if(prog == Progress::Talked)
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
		else if(prog == Progress::AllPortalsClosed)
		{
			unit_to_spawn = UnitData::Get("q_zlo_boss");
			spawn_unit_room = RoomTarget::Treasury;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
		}
	}

	unit_event_handler = this;

	return true;
}

//=================================================================================================
void Quest_Evil::LoadOld(GameReader& f)
{
	int old_refid, city, where, where2;

	f >> evil_state;
	f >> old_refid;
	f >> city;
	f >> where;
	f >> where2;
	f >> pos;
	f >> timer;
	f >> cleric;
}

//=================================================================================================
void Quest_Evil::GenerateBloodyAltar()
{
	InsideLocation* inside = (InsideLocation*)W.GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();

	// szukaj zwyk³ego o³tarza blisko œrodka
	Vec3 center(float(lvl.w + 1), 0, float(lvl.h + 1));
	float best_dist = 999.f;
	BaseObject* base_obj = BaseObject::Get("altar");
	Object* obj = nullptr;
	for(auto o : lvl.objects)
	{
		if(o->base == base_obj)
		{
			float dist = Vec3::Distance(o->pos, center);
			if(dist < best_dist)
			{
				best_dist = dist;
				obj = o;
			}
		}
	}
	assert(obj);

	// zmieñ typ obiektu
	obj->base = BaseObject::Get("bloody_altar");
	obj->mesh = obj->base->mesh;

	// dodaj cz¹steczki
	ParticleEmitter* pe = new ParticleEmitter;
	pe->alpha = 0.8f;
	pe->emision_interval = 0.1f;
	pe->emisions = -1;
	pe->life = -1;
	pe->max_particles = 50;
	pe->op_alpha = POP_LINEAR_SHRINK;
	pe->op_size = POP_LINEAR_SHRINK;
	pe->particle_life = 0.5f;
	pe->pos = obj->pos;
	pe->pos.y += obj->base->centery;
	pe->pos_min = Vec3(0, 0, 0);
	pe->pos_max = Vec3(0, 0, 0);
	pe->spawn_min = 1;
	pe->spawn_max = 3;
	pe->speed_min = Vec3(-1, 4, -1);
	pe->speed_max = Vec3(1, 6, 1);
	pe->mode = 0;
	pe->tex = game->tKrew[BLOOD_RED];
	pe->size = 0.5f;
	pe->Init();
	L.local_ctx.pes->push_back(pe);

	// dodaj krew
	vector<Int2> path;
	game->pathfinding->FindPath(L.local_ctx, lvl.staircase_up, pos_to_pt(obj->pos), path);
	for(vector<Int2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood& b = Add1(L.local_ctx.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f))
				+ Vec3(2.f*it->x + 1 + (float(it->x) - (it - 1)->x) / 2, 0, 2.f*it->y + 1 + (float(it->y) - (it - 1)->y) / 2);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = Vec3(0, 1, 0);
		}
		{
			Blood& b = Add1(L.local_ctx.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f)) + Vec3(2.f*it->x + 1, 0, 2.f*it->y + 1);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = Vec3(0, 1, 0);
		}
	}

	// ustaw pokój na specjalny ¿eby nie by³o tam wrogów
	lvl.GetNearestRoom(obj->pos)->target = RoomTarget::Treasury;

	evil_state = Quest_Evil::State::SpawnedAltar;
	pos = obj->pos;

	if(game->devmode)
		Info("Generated bloody altar (%g,%g).", obj->pos.x, obj->pos.z);
}

//=================================================================================================
void Quest_Evil::GeneratePortal()
{
	InsideLocation* inside = (InsideLocation*)W.GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Vec3 srodek(float(lvl.w), 0, float(lvl.h));

	// szukaj pokoju
	static vector<std::pair<int, float> > dobre;
	int index = 0;
	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it, ++index)
	{
		if(it->target == RoomTarget::None && it->size.x > 2 && it->size.y > 2)
		{
			float dist = Vec3::Distance2d(it->Center(), srodek);
			dobre.push_back(std::pair<int, float>(index, dist));
		}
	}
	std::sort(dobre.begin(), dobre.end(),
		[](const std::pair<int, float>& p1, const std::pair<int, float>& p2) { return p1.second > p2.second; });

	int id;

	while(true)
	{
		id = dobre.back().first;
		dobre.pop_back();
		Room& r = lvl.rooms[id];

		L.global_col.clear();
		L.GatherCollisionObjects(L.local_ctx, L.global_col, r.Center(), 2.f);
		if(L.global_col.empty())
			break;

		if(dobre.empty())
			throw "No free space to generate portal!";
	}

	dobre.clear();

	Room& r = lvl.rooms[id];
	Vec3 portal_pos = r.Center();
	r.target = RoomTarget::PortalCreate;
	float rot = PI*Random(0, 3);
	L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("portal"), portal_pos, rot);
	inside->portal = new Portal;
	inside->portal->target_loc = -1;
	inside->portal->next_portal = nullptr;
	inside->portal->rot = rot;
	inside->portal->pos = portal_pos;
	inside->portal->at_level = L.dungeon_level;

	int d = GetLocId(W.GetCurrentLocationIndex());
	loc[d].pos = portal_pos;
	loc[d].state = Quest_Evil::Loc::State::None;

	if(game->devmode)
		Info("Generated portal (%g,%g).", portal_pos.x, portal_pos.z);
}

//=================================================================================================
void Quest_Evil::WarpEvilBossToAltar()
{
	// znajdŸ bossa
	Unit* u = L.local_ctx.FindUnit("q_zlo_boss");
	assert(u);

	// znajdŸ krwawy o³tarz
	Object* o = L.local_ctx.FindObject("bloody_altar");
	assert(o);

	if(u && o)
	{
		Vec3 warp_pos = o->pos;
		warp_pos -= Vec3(sin(o->rot.y)*1.5f, 0, cos(o->rot.y)*1.5f);
		L.WarpUnit(*u, warp_pos);
		u->ai->start_pos = u->pos;
	}
}

//=================================================================================================
int Quest_Evil::GetLocId(int location_id)
{
	for(int i = 0; i < 3; ++i)
	{
		if(loc[i].target_loc == location_id)
			return i;
	}
	return -1;
}
