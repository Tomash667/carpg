#include "Pch.h"
#include "Quest_Evil.h"

#include "AIController.h"
#include "DungeonGenerator.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "InsideLocation.h"
#include "Journal.h"
#include "Level.h"
#include "LocationGeneratorFactory.h"
#include "LocationHelper.h"
#include "Pathfinding.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SoundManager.h>

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
	startLoc = world->GetRandomSettlement(quest_mgr->GetUsedCities());
	mage_loc = -1;
	closed = 0;
	changed = false;
	for(int i = 0; i < 3; ++i)
	{
		loc[i].state = Loc::State::None;
		loc[i].targetLoc = nullptr;
	}
	told_about_boss = false;
	evil_state = State::None;
	cleric = nullptr;
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[8], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Evil' - %s.", GetStartLocationName());
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
		{
			if(!quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(game->txQuest[232], GetStartLocationName()));
		}
		break;
	case Progress::Talked:
		{
			OnStart(game->txQuest[233]);
			quest_mgr->RemoveQuestRumor(id);
			// add location
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 128.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, OLD_TEMPLE, 1);
			target.group = UnitGroup::empty;
			target.SetKnown();
			target.st = 8;
			target.active_quest = this;
			target.dont_clean = true;
			targetLoc = &target;
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
			at_level = 0;
			// add journal entries
			msgs.push_back(Format(game->txQuest[234], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::AltarEvent:
		{
			OnUpdate(game->txQuest[236]);
			world->AddNews(Format(game->txQuest[237], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedAboutBook:
		{
			Location* mage = world->GetRandomSettlement(startLoc);
			mage_loc = mage->index;
			OnUpdate(Format(game->txQuest[238], mage->name.c_str(), GetLocationDirName(startLoc->pos, mage->pos)));
			evil_state = State::GenerateMage;
			team->AddExp(7500);
		}
		break;
	case Progress::MageToldAboutStolenBook:
		{
			OnUpdate(game->txQuest[239]);
			world->AddNews(Format(game->txQuest[240], world->GetLocation(mage_loc)->name.c_str()));
			DialogContext::current->talker->temporary = true;
		}
		break;
	case Progress::TalkedWithCaptain:
		{
			OnUpdate(Format(game->txQuest[241], LocationHelper::IsCity(world->GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::TalkedWithMayor:
		{
			OnUpdate(Format(game->txQuest[242], LocationHelper::IsCity(world->GetCurrentLocation()) ? game->txForMayor : game->txForSoltys));
		}
		break;
	case Progress::GotBook:
		{
			OnUpdate(game->txQuest[243]);
			const Item* item = Item::Get("q_zlo_ksiega");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
		}
		break;
	case Progress::GivenBook:
		{
			// spawn locations
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
				Location& target = *world->CreateLocation(l_info[i].type, world->GetRandomPlace(), l_info[i].target,
					Random(max(levels.x, 2), max(levels.y, 2)));
				target.group = UnitGroup::Get(l_info[i].group);
				target.st = l_info[i].st;
				target.SetKnown();
				target.active_quest = this;
				loc[i].targetLoc = &target;
				loc[i].near_loc = world->GetNearestSettlement(target.pos)->index;
				loc[i].at_level = target.GetLastLevel();
				loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
				new_msgs[i + 1] = Format(game->txQuest[247], target.name.c_str(),
					GetLocationDirName(world->GetLocation(loc[i].near_loc)->pos, target.pos),
					world->GetLocation(loc[i].near_loc)->name.c_str());
				world->AddNews(Format(game->txQuest[246], target.name.c_str()));
			}

			OnUpdate({ new_msgs[0], new_msgs[1], new_msgs[2], new_msgs[3] });

			next_event = &loc[0];
			loc[0].next_event = &loc[1];
			loc[1].next_event = &loc[2];

			// add cleric to team
			Unit& u = *DialogContext::current->talker;
			const Item* item = Item::Get("q_zlo_ksiega");
			u.AddItem(item, 1, true);
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			team->AddMember(&u, HeroType::Free);

			evil_state = State::ClosingPortals;
		}
		break;
	case Progress::PortalClosed:
		apply = false;
		changed = false;
		team->AddExp(10000);
		break;
	case Progress::AllPortalsClosed:
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
			targetLoc->st = 15;
			targetLoc->group = UnitGroup::Get("evil");
			targetLoc->reset = true;
			evil_state = State::KillBoss;
			OnUpdate(Format(game->txQuest[248], GetTargetLocationName()));
			for(int i = 0; i < 3; ++i)
				loc[i].targetLoc->active_quest = nullptr;
		}
		break;
	case Progress::KilledBoss:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[249]);
			// restore old altar
			targetLoc->active_quest = nullptr;
			targetLoc->dont_clean = false;
			BaseObject* base_obj = BaseObject::Get("bloody_altar");
			Object* obj = game_level->local_area->FindObject(base_obj);
			obj->base = BaseObject::Get("altar");
			obj->mesh = obj->base->mesh;
			res_mgr->Load(obj->mesh);
			// remove particles
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
			// talking
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
		{
			evil_state = State::ClericLeaving;
			// remove cleric from team
			Unit& u = *DialogContext::current->talker;
			team->RemoveMember(&u);
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
		return GetLocationDirName(startLoc->pos, world->GetLocation(mage_loc)->pos);
	else if(str == "t1")
		return loc[0].targetLoc->name.c_str();
	else if(str == "t2")
		return loc[1].targetLoc->name.c_str();
	else if(str == "t3")
		return loc[2].targetLoc->name.c_str();
	else if(str == "t1n")
		return world->GetLocation(loc[0].near_loc)->name.c_str();
	else if(str == "t2n")
		return world->GetLocation(loc[1].near_loc)->name.c_str();
	else if(str == "t3n")
		return world->GetLocation(loc[2].near_loc)->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(world->GetLocation(loc[0].near_loc)->pos, loc[0].targetLoc->pos);
	else if(str == "t2d")
		return GetLocationDirName(world->GetLocation(loc[1].near_loc)->pos, loc[1].targetLoc->pos);
	else if(str == "t3d")
		return GetLocationDirName(world->GetLocation(loc[2].near_loc)->pos, loc[2].targetLoc->pos);
	else if(str == "close_dir")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), loc[i].targetLoc->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = i;
				}
			}
		}
		Loc& l = loc[best_index];
		return GetLocationDirName(world->GetWorldPos(), l.targetLoc->pos);
	}
	else if(str == "close_loc")
	{
		float best_dist = 999.f;
		Location* bestLoc = nullptr;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), loc[i].targetLoc->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					bestLoc = loc[i].targetLoc;
				}
			}
		}
		return bestLoc->name.c_str();
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
			int d = GetLocId(world->GetCurrentLocation());
			if(d != -1)
			{
				if(loc[d].state != 3)
					return true;
			}
		}
		else if(prog == Progress::AllPortalsClosed)
		{
			if(world->GetCurrentLocation() == targetLoc)
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
		f << loc[i].targetLoc;
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
Quest::LoadResult Quest_Evil::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> mage_loc;
	for(int i = 0; i < 3; ++i)
	{
		f >> loc[i].targetLoc;
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

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Evil::GenerateBloodyAltar()
{
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();

	// find altar need center
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

	// change object type
	obj->base = BaseObject::Get("bloody_altar");
	obj->mesh = obj->base->mesh;

	// add particles
	ParticleEmitter* pe = new ParticleEmitter;
	pe->alpha = 0.8f;
	pe->emission_interval = 0.1f;
	pe->emissions = -1;
	pe->life = -1;
	pe->max_particles = 50;
	pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
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

	// add blood
	vector<Int2> path;
	pathfinding->FindPath(lvl, lvl.prevEntryPt, PosToPt(obj->pos), path);
	for(vector<Int2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood* blood = new Blood;
			blood->pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f))
				+ Vec3(2.f*it->x + 1 + (float(it->x) - (it - 1)->x) / 2, 0, 2.f*it->y + 1 + (float(it->y) - (it - 1)->y) / 2);
			blood->type = BLOOD_RED;
			blood->rot = Random(MAX_ANGLE);
			blood->size = 1.f;
			blood->scale = 1.f;
			blood->pos.y = 0.05f;
			blood->normal = Vec3(0, 1, 0);
			lvl.bloods.push_back(blood);
		}
		{
			Blood* blood = new Blood;
			blood->pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f)) + Vec3(2.f*it->x + 1, 0, 2.f*it->y + 1);
			blood->type = BLOOD_RED;
			blood->rot = Random(MAX_ANGLE);
			blood->size = 1.f;
			blood->scale = 1.f;
			blood->pos.y = 0.05f;
			blood->normal = Vec3(0, 1, 0);
			lvl.bloods.push_back(blood);
		}
	}

	// set special room type so no enemies will spawn there
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

	int d = GetLocId(world->GetCurrentLocation());
	loc[d].pos = portal_pos;
	loc[d].state = Quest_Evil::Loc::State::None;

	if(game->devmode)
		Info("Generated portal (%g,%g).", portal_pos.x, portal_pos.z);
}

//=================================================================================================
void Quest_Evil::WarpEvilBossToAltar()
{
	LevelArea& area = *game_level->local_area;

	// find bossa
	Unit* u = area.FindUnit(UnitData::Get("q_zlo_boss"));
	assert(u);

	// find blood altar
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
int Quest_Evil::GetLocId(Location* location)
{
	for(int i = 0; i < 3; ++i)
	{
		if(loc[i].targetLoc == location)
			return i;
	}
	return -1;
}

//=================================================================================================
void Quest_Evil::Update(float dt)
{
	if(evil_state == State::SpawnedAltar && game_level->location == targetLoc)
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
		int d = GetLocId(game_level->location);
		if(d != -1)
		{
			Loc& l = loc[d];
			if(l.state != 3)
			{
				Unit* u = cleric;

				if(u->ai->state == AIController::Idle)
				{
					// check if is near portal
					float dist = Vec3::Distance2d(u->pos, l.pos);
					if(dist < 5.f)
					{
						// move towards
						u->ai->st.idle.action = AIController::Idle_Move;
						u->ai->st.idle.pos = l.pos;
						u->ai->timer = 1.f;
						if(u->GetOrder() != ORDER_WAIT)
							u->OrderWait();

						// close portal
						if(dist < 2.f)
						{
							timer -= dt;
							if(timer <= 0.f)
							{
								l.state = Loc::State::PortalClosed;
								u->OrderFollow(team->GetLeader());
								u->ai->st.idle.action = AIController::Idle_None;
								OnUpdate(Format(game->txPortalClosed, game_level->location->name.c_str()));
								u->OrderAutoTalk();
								changed = true;
								++closed;
								game_level->local_area->tmp->scene->Remove(game_level->location->portal->node);
								game_level->location->portal->node->Free();
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
