#include "Pch.h"
#include "GameCore.h"
#include "Quest_Evil.h"
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
#include "SoundManager.h"
#include "LocationGeneratorFactory.h"
#include "DungeonGenerator.h"
#include "GameResources.h"

//=================================================================================================
void Quest_Evil::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_zlo_kapitan");
	quest_mgr->RegisterSpecialIfHandler(this, "q_zlo_burmistrz");
}

//=================================================================================================
void Quest_Evil::Start()
{
	category = QuestCategory::Unique;
	type = Q_EVIL;
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
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[8], GetStartLocationName()));
}

//=================================================================================================
GameDialog* Quest_Evil::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;

	if(id == "q_zlo_kaplan")
		return GameDialog::TryGet("q_evil_cleric");
	else if(id == "q_zlo_mag")
		return GameDialog::TryGet("q_evil_mage");
	else if(id == "q_zlo_boss")
		return GameDialog::TryGet("q_evil_boss");
	else if(id == "guard_captain")
		return GameDialog::TryGet("q_evil_captain");
	else if(id == "mayor" || id == "soltys")
		return GameDialog::TryGet("q_evil_mayor");
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
			if(!quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(game->txQuest[232], GetStartLocationName()));
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
			quest_mgr->RemoveQuestRumor(id);
			// lokacja
			Location& target = *world->CreateLocation(L_DUNGEON, world->GetWorldPos(), 128.f, OLD_TEMPLE, UnitGroup::empty, false, 1);
			target.SetKnown();
			target.st = 8;
			target.active_quest = this;
			target.dont_clean = true;
			target_loc = target.index;
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
			at_level = 0;
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[234], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::AltarEvent:
		// zdarzenie
		{
			OnUpdate(game->txQuest[236]);
			world->AddNews(Format(game->txQuest[237], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedAboutBook:
		// powiedzia³ o ksiêdze
		{
			mage_loc = world->GetRandomSettlementIndex(start_loc);
			Location& mage = *world->GetLocation(mage_loc);
			OnUpdate(Format(game->txQuest[238], mage.name.c_str(), GetLocationDirName(GetStartLocation().pos, mage.pos)));
			evil_state = State::GenerateMage;
			team->AddExp(7500);
		}
		break;
	case Progress::MageToldAboutStolenBook:
		// mag powiedzia³ ¿e mu zabrali ksiêge
		{
			OnUpdate(game->txQuest[239]);
			world->AddNews(Format(game->txQuest[240], world->GetLocation(mage_loc)->name.c_str()));
			DialogContext::current->talker->temporary = true;
		}
		break;
	case Progress::TalkedWithCaptain:
		// pogadano z kapitanem
		{
			OnUpdate(Format(game->txQuest[241], LocationHelper::IsCity(world->GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::TalkedWithMayor:
		// pogadano z burmistrzem
		{
			OnUpdate(Format(game->txQuest[242], LocationHelper::IsCity(world->GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::GotBook:
		// dosta³eœ ksi¹¿ke
		{
			OnUpdate(game->txQuest[243]);
			const Item* item = Item::Get("q_zlo_ksiega");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
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
				cstring group;
				int st;
			} l_info[3] = {
				L_DUNGEON, OLD_TEMPLE, "evil", 16,
				L_DUNGEON, NECROMANCER_BASE, "necromancers", 15,
				L_DUNGEON, MONSTER_CRYPT, "undead", 13
			};

			cstring new_msgs[4];
			new_msgs[0] = game->txQuest[245];

			for(int i = 0; i < 3; ++i)
			{
				Int2 levels = g_base_locations[l_info[i].target].levels;
				Location& target = *world->CreateLocation(l_info[i].type, Vec2(0, 0), -128.f, l_info[i].target, UnitGroup::Get(l_info[i].group), true,
					Random(max(levels.x, 2), max(levels.y, 2)));
				target.st = l_info[i].st;
				target.SetKnown();
				target.active_quest = this;
				loc[i].target_loc = target.index;
				loc[i].near_loc = world->GetNearestSettlement(target.pos);
				loc[i].at_level = target.GetLastLevel();
				loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
				new_msgs[i + 1] = Format(game->txQuest[247], world->GetLocation(loc[i].target_loc)->name.c_str(),
					GetLocationDirName(world->GetLocation(loc[i].near_loc)->pos, world->GetLocation(loc[i].target_loc)->pos),
					world->GetLocation(loc[i].near_loc)->name.c_str());
				world->AddNews(Format(game->txQuest[246], world->GetLocation(loc[i].target_loc)->name.c_str()));
			}

			OnUpdate({ new_msgs[0], new_msgs[1], new_msgs[2], new_msgs[3] });

			next_event = &loc[0];
			loc[0].next_event = &loc[1];
			loc[1].next_event = &loc[2];

			// dodaj jozana do dru¿yny
			Unit& u = *DialogContext::current->talker;
			const Item* item = Item::Get("q_zlo_ksiega");
			u.AddItem(item, 1, true);
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			team->AddTeamMember(&u, HeroType::Free);

			evil_state = State::ClosingPortals;
		}
		break;
	case Progress::PortalClosed:
		// u¿ywane tylko do czyszczenia flagi changed
		apply = false;
		changed = false;
		team->AddExp(10000);
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
			target.group = UnitGroup::Get("evil");
			target.reset = true;
			evil_state = State::KillBoss;
			OnUpdate(Format(game->txQuest[248], GetTargetLocationName()));
			for(int i = 0; i < 3; ++i)
				world->GetLocation(loc[i].target_loc)->active_quest = nullptr;
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
			Object* obj = game_level->local_area->FindObject(base_obj);
			obj->base = BaseObject::Get("altar");
			obj->mesh = obj->base->mesh;
			res_mgr->Load(obj->mesh);
			// usuñ cz¹steczki
			float best_dist = 999.f;
			ParticleEmitter* best_pe = nullptr;
			for(ParticleEmitter* pe : game_level->local_area->tmp->pes)
			{
				if(pe->tex == game_res->tBlood[BLOOD_RED])
				{
					float dist = Vec3::Distance(pe->pos, obj->pos);
					if(dist < best_dist)
					{
						best_dist = dist;
						best_pe = pe;
					}
				}
			}
			assert(best_pe);
			best_pe->destroy = true;
			// gadanie przez jozana
			Unit* unit = team->FindTeamMember("q_zlo_kaplan");
			if(unit)
				unit->OrderAutoTalk();

			quest_mgr->EndUniqueQuest();
			team->AddExp(30000);
			evil_state = State::ClericWantTalk;
			world->AddNews(game->txQuest[250]);
			team->AddLearningPoint();

			if(Net::IsOnline())
				Net::PushChange(NetChange::CLEAN_ALTAR);
		}
		break;
	case Progress::Finished:
		// pogadano z jozanem
		{
			evil_state = State::ClericLeaving;
			// usuñ jozana z dru¿yny
			Unit& u = *DialogContext::current->talker;
			team->RemoveTeamMember(&u);
			u.OrderLeave();
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
		return world->GetLocation(mage_loc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, world->GetLocation(mage_loc)->pos);
	else if(str == "t1")
		return world->GetLocation(loc[0].target_loc)->name.c_str();
	else if(str == "t2")
		return world->GetLocation(loc[1].target_loc)->name.c_str();
	else if(str == "t3")
		return world->GetLocation(loc[2].target_loc)->name.c_str();
	else if(str == "t1n")
		return world->GetLocation(loc[0].near_loc)->name.c_str();
	else if(str == "t2n")
		return world->GetLocation(loc[1].near_loc)->name.c_str();
	else if(str == "t3n")
		return world->GetLocation(loc[2].near_loc)->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(world->GetLocation(loc[0].near_loc)->pos, world->GetLocation(loc[0].target_loc)->pos);
	else if(str == "t2d")
		return GetLocationDirName(world->GetLocation(loc[1].near_loc)->pos, world->GetLocation(loc[1].target_loc)->pos);
	else if(str == "t3d")
		return GetLocationDirName(world->GetLocation(loc[2].near_loc)->pos, world->GetLocation(loc[2].target_loc)->pos);
	else if(str == "close_dir")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), world->GetLocation(loc[i].target_loc)->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = i;
				}
			}
		}
		Loc& l = loc[best_index];
		return GetLocationDirName(world->GetWorldPos(), world->GetLocation(l.target_loc)->pos);
	}
	else if(str == "close_loc")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), world->GetLocation(loc[i].target_loc)->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = loc[i].target_loc;
				}
			}
		}
		return world->GetLocation(best_index)->name.c_str();
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
			int d = GetLocId(world->GetCurrentLocationIndex());
			if(d != -1)
			{
				if(loc[d].state != 3)
					return true;
			}
		}
		else if(prog == Progress::AllPortalsClosed)
		{
			if(world->GetCurrentLocationIndex() == target_loc)
				return true;
		}
		return false;
	}
	else if(strcmp(msg, "q_zlo_kapitan") == 0)
	{
		return world->GetCurrentLocationIndex() == mage_loc
			&& evil_state >= State::GeneratedMage
			&& evil_state < State::ClosingPortals
			&& InRange((Progress)prog, Progress::MageToldAboutStolenBook, Progress::TalkedWithMayor);
	}
	else if(strcmp(msg, "q_zlo_burmistrz") == 0)
	{
		return world->GetCurrentLocationIndex() == mage_loc
			&& evil_state >= State::GeneratedMage
			&& evil_state < State::ClosingPortals
			&& prog == Progress::TalkedWithCaptain;
	}
	assert(0);
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
	f >> evil_state;
	f >> pos;
	f >> timer;
	f >> cleric;

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
void Quest_Evil::GenerateBloodyAltar()
{
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
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
	pe->tex = game_res->tBlood[BLOOD_RED];
	pe->size = 0.5f;
	pe->Init();
	lvl.tmp->pes.push_back(pe);

	// dodaj krew
	vector<Int2> path;
	pathfinding->FindPath(lvl, lvl.staircase_up, PosToPt(obj->pos), path);
	for(vector<Int2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood& b = Add1(lvl.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f))
				+ Vec3(2.f*it->x + 1 + (float(it->x) - (it - 1)->x) / 2, 0, 2.f*it->y + 1 + (float(it->y) - (it - 1)->y) / 2);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.scale = 1.f;
			b.pos.y = 0.05f;
			b.normal = Vec3(0, 1, 0);
		}
		{
			Blood& b = Add1(lvl.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f)) + Vec3(2.f*it->x + 1, 0, 2.f*it->y + 1);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.scale = 1.f;
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
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Vec3 center(float(lvl.w), 0, float(lvl.h));

	static vector<pair<Room*, float>> good_pts;
	for(Room* room : lvl.rooms)
	{
		if(room->target == RoomTarget::None && room->size.x > 2 && room->size.y > 2)
		{
			float dist = Vec3::Distance2d(room->Center(), center);
			good_pts.push_back({ room, dist });
		}
	}
	std::sort(good_pts.begin(), good_pts.end(),
		[](const pair<Room*, float>& p1, const pair<Room*, float>& p2) { return p1.second > p2.second; });

	Room* room;
	while(true)
	{
		room = good_pts.back().first;
		good_pts.pop_back();

		game_level->global_col.clear();
		game_level->GatherCollisionObjects(lvl, game_level->global_col, room->Center(), 2.f);
		if(game_level->global_col.empty())
			break;

		if(good_pts.empty())
			throw "No free space to generate portal!";
	}
	good_pts.clear();

	Vec3 portal_pos = room->Center();
	room->target = RoomTarget::PortalCreate;
	float rot = PI * Random(0, 3);
	game_level->SpawnObjectEntity(lvl, BaseObject::Get("portal"), portal_pos, rot);
	inside->portal = new Portal;
	inside->portal->target_loc = -1;
	inside->portal->next_portal = nullptr;
	inside->portal->rot = rot;
	inside->portal->pos = portal_pos;
	inside->portal->at_level = game_level->dungeon_level;

	int d = GetLocId(world->GetCurrentLocationIndex());
	loc[d].pos = portal_pos;
	loc[d].state = Quest_Evil::Loc::State::None;

	if(game->devmode)
		Info("Generated portal (%g,%g).", portal_pos.x, portal_pos.z);
}

//=================================================================================================
void Quest_Evil::WarpEvilBossToAltar()
{
	LevelArea& area = *game_level->local_area;

	// znajdŸ bossa
	Unit* u = area.FindUnit(UnitData::Get("q_zlo_boss"));
	assert(u);

	// znajdŸ krwawy o³tarz
	Object* o = area.FindObject(BaseObject::Get("bloody_altar"));
	assert(o);

	if(u && o)
	{
		Vec3 warp_pos = o->pos;
		warp_pos -= Vec3(sin(o->rot.y)*1.5f, 0, cos(o->rot.y)*1.5f);
		game_level->WarpUnit(*u, warp_pos);
		u->ai->start_pos = u->pos;

		for(int i = 0; i < 2; ++i)
		{
			Unit* u2 = game_level->SpawnUnitNearLocation(area, u->pos, *UnitData::Get("zombie_ancient"));
			if(u2)
			{
				u2->dont_attack = true;
				u2->OrderGuard(u);
			}
		}
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

//=================================================================================================
void Quest_Evil::Update(float dt)
{
	if(evil_state == State::SpawnedAltar && game_level->location_index == target_loc)
	{
		for(Unit& unit : team->members)
		{
			if(unit.IsStanding() && unit.IsPlayer() && Vec3::Distance(unit.pos, pos) < 5.f
				&& game_level->CanSee(*game_level->local_area, unit.pos, pos))
			{
				evil_state = State::Summoning;
				sound_mgr->PlaySound2d(game_res->sEvil);
				if(Net::IsOnline())
					Net::PushChange(NetChange::EVIL_SOUND);
				SetProgress(Progress::AltarEvent);
				// spawn undead
				InsideLocation* inside = (InsideLocation*)game_level->location;
				inside->group = UnitGroup::Get("undead");
				DungeonGenerator* gen = (DungeonGenerator*)game->loc_gen_factory->Get(game_level->location);
				gen->GenerateUnits();
				break;
			}
		}
	}
	else if(evil_state == State::ClosingPortals && !game_level->location->outside && game_level->location->GetLastLevel() == game_level->dungeon_level)
	{
		int d = GetLocId(game_level->location_index);
		if(d != -1)
		{
			Loc& l = loc[d];
			if(l.state != 3)
			{
				Unit* u = cleric;

				if(u->ai->state == AIController::Idle)
				{
					// sprawdŸ czy podszed³ do portalu
					float dist = Vec3::Distance2d(u->pos, l.pos);
					if(dist < 5.f)
					{
						// podejdŸ
						u->ai->idle_action = AIController::Idle_Move;
						u->ai->idle_data.pos = l.pos;
						u->ai->timer = 1.f;
						if(u->GetOrder() != ORDER_WAIT)
							u->OrderWait();

						// zamknij
						if(dist < 2.f)
						{
							timer -= dt;
							if(timer <= 0.f)
							{
								l.state = Loc::State::PortalClosed;
								u->OrderFollow(team->GetLeader());
								u->ai->idle_action = AIController::Idle_None;
								OnUpdate(Format(game->txPortalClosed, game_level->location->name.c_str()));
								u->OrderAutoTalk();
								changed = true;
								++closed;
								delete game_level->location->portal;
								game_level->location->portal = nullptr;
								world->AddNews(Format(game->txPortalClosedNews, game_level->location->name.c_str()));
								if(Net::IsOnline())
									Net::PushChange(NetChange::CLOSE_PORTAL);
							}
						}
						else
							timer = 1.5f;
					}
					else if(u->GetOrder() != ORDER_FOLLOW)
						u->OrderFollow(team->GetLeader());
				}
			}
		}
	}
}
