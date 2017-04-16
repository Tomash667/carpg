#include "Pch.h"
#include "Base.h"
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

//=================================================================================================
void Quest_Evil::Start()
{
	type = QuestType::Unique;
	quest_id = Q_EVIL;
	mage_loc = -1;
	closed = 0;
	changed = false;
	for(int i=0; i<3; ++i)
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
			// dodaj plotk�
			if(!quest_manager.RemoveQuestRumor(P_ZLO))
			{
				cstring text = Format(game->txQuest[232], GetStartLocationName());
				game->rumors.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->rumors.size())-1;
				}
			}
		}
		break;
	case Progress::Started:
		// zaakceptowano
		break;
	case Progress::Talked:
		// zaakceptowano
		{
			name = game->txQuest[233];
			state = Quest::Started;
			// usu� plotk�
			quest_manager.RemoveQuestRumor(P_ZLO);
			// lokacja
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 128.f, OLD_TEMPLE, SG_BRAK, false, 1);
			Location& target = GetTargetLocation();
			bool now_known = false;
			if(target.state == LS_UNKNOWN)
			{
				target.state = LS_KNOWN;
				now_known = true;
			}
			target.st = 8;
			target.active_quest = this;
			target.dont_clean = true;
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
			at_level = 0;
			// questowe rzeczy
			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[234], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);			

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::AltarEvent:
		// zdarzenie
		{
			msgs.push_back(game->txQuest[236]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[237], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedAboutBook:
		// powiedzia� o ksi�dze
		{
			mage_loc = game->GetRandomSettlement(start_loc);
			Location& mage = *game->locations[mage_loc];
			msgs.push_back(Format(game->txQuest[238], mage.name.c_str(), GetLocationDirName(GetStartLocation().pos, mage.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			evil_state = State::GenerateMage;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::MageToldAboutStolenBook:
		// mag powiedzia� �e mu zabrali ksi�ge
		{
			msgs.push_back(game->txQuest[239]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[240], game->locations[mage_loc]->name.c_str()));
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithCaptain:
		// pogadano z kapitanem
		{
			msgs.push_back(Format(game->txQuest[241], LocationHelper::IsCity(game->location) ? game->txForMayor : game->txForSoltys));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithMayor:
		// pogadano z burmistrzem
		{
			msgs.push_back(Format(game->txQuest[242], LocationHelper::IsCity(game->location) ? game->txForMayor : game->txForSoltys));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GotBook:
		// dosta�e� ksi��ke
		{
			msgs.push_back(game->txQuest[243]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			const Item* item = FindItem("q_zlo_ksiega");
			game->current_dialog->pc->unit->AddItem(item, 1, true);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::GivenBook:
		// da�e� ksi��k� jozanowi
		{
			// dodaj lokacje
			const struct
			{
				LOCATION type;
				int target;
				SPAWN_GROUP spawn;
				int st;
			} l_info[3] = {
				L_DUNGEON, OLD_TEMPLE, SG_ZLO, 15,
				L_DUNGEON, NECROMANCER_BASE, SG_NEKRO, 14,
				L_CRYPT, MONSTER_CRYPT, SG_NIEUMARLI, 13
			};

			msgs.push_back(game->txQuest[245]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			for(int i=0; i<3; ++i)
			{
				INT2 levels = g_base_locations[l_info[i].target].levels;
				loc[i].target_loc = game->CreateLocation(l_info[i].type, VEC2(0,0), -128.f, l_info[i].target, l_info[i].spawn, true,
					random2(max(levels.x, 2), max(levels.y, 2)));
				Location& target = *game->locations[loc[i].target_loc];
				target.st = l_info[i].st;
				target.state = LS_KNOWN;
				target.active_quest = this;
				loc[i].near_loc = game->GetNearestSettlement(target.pos);
				loc[i].at_level = target.GetLastLevel();
				loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
				msgs.push_back(Format(game->txQuest[247], game->locations[loc[i].target_loc]->name.c_str(),
					GetLocationDirName(game->locations[loc[i].near_loc]->pos, game->locations[loc[i].target_loc]->pos),
					game->locations[loc[i].near_loc]->name.c_str()));
				game->AddNews(Format(game->txQuest[246],
					game->locations[loc[i].target_loc]->name.c_str()));
			}

			next_event = &loc[0];
			loc[0].next_event = &loc[1];
			loc[1].next_event = &loc[2];

			// dodaj jozana do dru�yny
			Unit& u = *game->current_dialog->talker;
			const Item* item = FindItem("q_zlo_ksiega");
			u.AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->AddTeamMember(&u, true);

			evil_state = State::ClosingPortals;

			if(game->IsOnline())
			{
				game->Net_UpdateQuestMulti(refid, 4);
				for(int i=0; i<3; ++i)
					game->Net_ChangeLocationState(loc[i].target_loc, false);
			}
		}
		break;
	case Progress::PortalClosed:
		// u�ywane tylko do czyszczenia flagi changed
		apply = false;
		changed = false;
		break;
	case Progress::AllPortalsClosed:
		// zamkni�to wszystkie portale
		{
			changed = false;
			done = false;
			unit_to_spawn = FindUnitData("q_zlo_boss");
			spawn_unit_room = RoomTarget::Treasury;
			unit_event_handler = this;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
			at_level = 0;
			Location& target = *game->locations[target_loc];
			target.st = 15;
			target.spawn = SG_ZLO;
			target.reset = true;
			evil_state = State::KillBoss;
			msgs.push_back(Format(game->txQuest[248], GetTargetLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			for(int i=0; i<3; ++i)
				game->locations[loc[i].target_loc]->active_quest = nullptr;

			if(game->IsOnline())
				game->Net_UpdateQuestMulti(refid, 2);
		}
		break;
	case Progress::KilledBoss:
		// zabito bossa
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[249]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// przywr�� stary o�tarz
			Location& target = GetTargetLocation();
			target.active_quest = nullptr;
			target.dont_clean = false;
			Obj* o = FindObject("bloody_altar");
			int index = 0;
			for(vector<Object>::iterator it = game->local_ctx.objects->begin(), end = game->local_ctx.objects->end(); it != end; ++it, ++index)
			{
				if(it->base == o)
					break;
			}
			Object& obj = game->local_ctx.objects->at(index);
			obj.base = FindObject("altar");
			obj.mesh = obj.base->mesh;
			// usu� cz�steczki
			float best_dist = 999.f;
			ParticleEmitter* pe = nullptr;
			for(vector<ParticleEmitter*>::iterator it = game->local_ctx.pes->begin(), end = game->local_ctx.pes->end(); it != end; ++it)
			{
				if((*it)->tex == game->tKrew[BLOOD_RED])
				{
					float dist = distance((*it)->pos, obj.pos);
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
			game->AddNews(game->txQuest[250]);

			if(game->IsOnline())
			{
				game->PushNetChange(NetChange::CLEAN_ALTAR);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	case Progress::Finished:
		// pogadano z jozanem
		{
			evil_state = State::ClericLeaving;
			// usu� jozana z dru�yny
			Unit& u = *game->current_dialog->talker;
			game->RemoveTeamMember(&u);
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
		return game->locations[mage_loc]->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[mage_loc]->pos);
	else if(str == "burmistrza")
		return LocationHelper::IsCity(game->location) ? game->txForMayor : game->txForSoltys;
	else if(str == "burmistrzem")
		return LocationHelper::IsCity(game->location) ? game->txQuest[251] : game->txQuest[252];
	else if(str == "t1")
		return game->locations[loc[0].target_loc]->name.c_str();
	else if(str == "t2")
		return game->locations[loc[1].target_loc]->name.c_str();
	else if(str == "t3")
		return game->locations[loc[2].target_loc]->name.c_str();
	else if(str == "t1n")
		return game->locations[loc[0].near_loc]->name.c_str();
	else if(str == "t2n")
		return game->locations[loc[1].near_loc]->name.c_str();
	else if(str == "t3n")
		return game->locations[loc[2].near_loc]->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(game->locations[loc[0].near_loc]->pos, game->locations[loc[0].target_loc]->pos);
	else if(str == "t2d")
		return GetLocationDirName(game->locations[loc[1].near_loc]->pos, game->locations[loc[1].target_loc]->pos);
	else if(str == "t3d")
		return GetLocationDirName(game->locations[loc[2].near_loc]->pos, game->locations[loc[2].target_loc]->pos);
	else if(str == "close_dir")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i=0; i<3; ++i)
		{
			if(loc[i].state != Loc::State::PortalClosed)
			{
				float dist = distance(game->world_pos, game->locations[loc[i].target_loc]->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = i;
				}
			}
		}
		Loc& l = loc[best_index];
		return GetLocationDirName(game->world_pos, game->locations[l.target_loc]->pos);
	}
	else if(str == "close_loc")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i=0; i<3; ++i)
		{
			if(loc[i].state != Loc::State::PortalClosed)
			{
				float dist = distance(game->world_pos, game->locations[loc[i].target_loc]->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = loc[i].target_loc;
				}
			}
		}
		return game->locations[best_index]->name.c_str();
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
bool Quest_Evil::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_zlo_clear1") == 0)
		return evil_state == State::ClosingPortals && closed == 1;
	else if(strcmp(msg, "q_zlo_clear2") == 0)
		return evil_state == State::ClosingPortals && closed == 2;
	else if(strcmp(msg, "q_zlo_tutaj") == 0)
	{
		if(prog == Progress::GivenBook)
		{
			int d = GetLocId(game->current_location);
			if(d != -1)
			{
				if(loc[d].state != 3)
					return true;
			}
		}
		else if(prog == Progress::AllPortalsClosed)
		{
			if(game->current_location == target_loc)
				return true;
		}
		return false;
	}
	else
	{
		assert(0);
		return false;
	}
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
void Quest_Evil::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

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
void Quest_Evil::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

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
			unit_to_spawn = FindUnitData("q_zlo_boss");
			spawn_unit_room = RoomTarget::Treasury;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
		}
	}

	unit_event_handler = this;
}

//=================================================================================================
void Quest_Evil::LoadOld(HANDLE file)
{
	GameReader f(file);
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
	InsideLocation* inside = (InsideLocation*)game->location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	// szukaj zwyk�ego o�tarza blisko �rodka
	VEC3 center(float(lvl.w+1), 0, float(lvl.h+1));
	int best_index = -1, index = 0;
	float best_dist = 999.f;
	Obj* oltarz = FindObject("altar");
	for(vector<Object>::iterator it = lvl.objects.begin(), end = lvl.objects.end(); it != end; ++it, ++index)
	{
		if(it->base == oltarz)
		{
			float dist = distance(it->pos, center);
			if(dist < best_dist)
			{
				best_dist = dist;
				best_index = index;
			}
		}
	}
	assert(best_index != -1);

	// zmie� typ obiektu
	Object& o = lvl.objects[best_index];
	o.base = FindObject("bloody_altar");
	o.mesh = o.base->mesh;

	// dodaj cz�steczki
	ParticleEmitter* pe = new ParticleEmitter;
	pe->alpha = 0.8f;
	pe->emision_interval = 0.1f;
	pe->emisions = -1;
	pe->life = -1;
	pe->max_particles = 50;
	pe->op_alpha = POP_LINEAR_SHRINK;
	pe->op_size = POP_LINEAR_SHRINK;
	pe->particle_life = 0.5f;
	pe->pos = o.pos;
	pe->pos.y += o.base->centery;
	pe->pos_min = VEC3(0, 0, 0);
	pe->pos_max = VEC3(0, 0, 0);
	pe->spawn_min = 1;
	pe->spawn_max = 3;
	pe->speed_min = VEC3(-1, 4, -1);
	pe->speed_max = VEC3(1, 6, 1);
	pe->mode = 0;
	pe->tex = game->tKrew[BLOOD_RED];
	pe->size = 0.5f;
	pe->Init();
	game->local_ctx.pes->push_back(pe);

	// dodaj krew
	vector<INT2> path;
	game->FindPath(game->local_ctx, lvl.staircase_up, pos_to_pt(o.pos), path);
	for(vector<INT2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood& b = Add1(game->local_ctx.bloods);
			b.pos = random(VEC3(-0.5f, 0.05f, -0.5f), VEC3(0.5f, 0.05f, 0.5f))
				+ VEC3(2.f*it->x+1+(float(it->x)-(it-1)->x)/2, 0, 2.f*it->y+1+(float(it->y)-(it-1)->y)/2);
			b.type = BLOOD_RED;
			b.rot = random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = VEC3(0, 1, 0);
		}
		{
			Blood& b = Add1(game->local_ctx.bloods);
			b.pos = random(VEC3(-0.5f, 0.05f, -0.5f), VEC3(0.5f, 0.05f, 0.5f))+VEC3(2.f*it->x+1, 0, 2.f*it->y+1);
			b.type = BLOOD_RED;
			b.rot = random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = VEC3(0, 1, 0);
		}
	}

	// ustaw pok�j na specjalny �eby nie by�o tam wrog�w
	lvl.GetNearestRoom(o.pos)->target = RoomTarget::Treasury;

	evil_state = Quest_Evil::State::SpawnedAltar;
	pos = o.pos;

	if(game->devmode)
		LOG(Format("Generated bloody altar (%g,%g).", o.pos.x, o.pos.z));
}

//=================================================================================================
void Quest_Evil::GeneratePortal()
{
	InsideLocation* inside = (InsideLocation*)game->location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	VEC3 srodek(float(lvl.w), 0, float(lvl.h));

	// szukaj pokoju
	static vector<std::pair<int, float> > dobre;
	int index = 0;
	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it, ++index)
	{
		if(it->target == RoomTarget::None && it->size.x > 2 && it->size.y > 2)
		{
			float dist = distance2d(it->Center(), srodek);
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

		game->global_col.clear();
		game->GatherCollisionObjects(game->local_ctx, game->global_col, r.Center(), 2.f);
		if(game->global_col.empty())
			break;

		if(dobre.empty())
			throw "No free space to generate portal!";
	}

	dobre.clear();

	Room& r = lvl.rooms[id];
	VEC3 portal_pos = r.Center();
	r.target = RoomTarget::PortalCreate;
	float rot = PI*random(0, 3);
	game->SpawnObject(game->local_ctx, FindObject("portal"), portal_pos, rot);
	inside->portal = new Portal;
	inside->portal->target_loc = -1;
	inside->portal->next_portal = nullptr;
	inside->portal->rot = rot;
	inside->portal->pos = portal_pos;
	inside->portal->at_level = game->dungeon_level;

	int d = GetLocId(game->current_location);
	loc[d].pos = portal_pos;
	loc[d].state = Quest_Evil::Loc::State::None;

	if(game->devmode)
		LOG(Format("Generated portal (%g,%g).", portal_pos.x, portal_pos.z));
}

//=================================================================================================
void Quest_Evil::WarpEvilBossToAltar()
{
	// znajd� bossa
	Unit* u = game->FindUnitByIdLocal("q_zlo_boss");
	assert(u);

	// znajd� krwawy o�tarz
	Object* o = game->FindObjectByIdLocal("bloody_altar");
	assert(o);

	if(u && o)
	{
		VEC3 warp_pos = o->pos;
		warp_pos -= VEC3(sin(o->rot.y)*1.5f, 0, cos(o->rot.y)*1.5f);
		game->WarpUnit(*u, warp_pos);
		u->ai->start_pos = u->pos;
	}
}

//=================================================================================================
int Quest_Evil::GetLocId(int location_id)
{
	for(int i = 0; i<3; ++i)
	{
		if(loc[i].target_loc == location_id)
			return i;
	}
	return -1;
}
