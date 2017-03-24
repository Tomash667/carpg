#include "Pch.h"
#include "Base.h"
#include "Quest_Orcs.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "MultiInsideLocation.h"
#include "GameGui.h"
#include "AIController.h"
#include "Team.h"

//=================================================================================================
void Quest_Orcs::Start()
{
	quest_id = Q_ORCS;
	type = QuestType::Unique;
}

//=================================================================================================
GameDialog* Quest_Orcs::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(game->current_dialog->talker->data->id == "q_orkowie_straznik")
		return FindDialog("q_orcs_guard");
	else
		return FindDialog("q_orcs_captain");
}

//=================================================================================================
void Quest_Orcs::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::TalkedWithGuard:
		{
			if(prog != Progress::None)
				return;
			// add gossip
			if(quest_manager.RemoveQuestRumor(P_ORKOWIE))
			{
				cstring text = Format(game->txQuest[189], game->locations[start_loc]->name.c_str());
				game->rumors.push_back(Format(game->game_gui->journal->txAddNote, game->day + 1, game->month + 1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->rumors.size()) - 1;
				}
			}
			game->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::NotAccepted:
		{
			// add gossip
			if(quest_manager.RemoveQuestRumor(P_ORKOWIE))
			{
				cstring text = Format(game->txQuest[190], game->locations[start_loc]->name.c_str());
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
			// mark guard to remove
			Unit*& u = game->quest_orcs2->guard;
			if(u)
			{
				u->auto_talk = AutoTalkMode::No;
				u->temporary = true;
				u = nullptr;
			}
			game->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::Started:
		{
			// remove rumor from pool
			quest_manager.RemoveQuestRumor(P_ORKOWIE);
			// mark guard to remove
			Unit*& u = game->quest_orcs2->guard;
			if(u)
			{
				u->auto_talk = AutoTalkMode::No;
				u->temporary = true;
				u = nullptr;
			}
			// generate location
			target_loc = game->CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, HUMAN_FORT, SG_ORKOWIE, false);
			Location& tl = GetTargetLocation();
			tl.state = LS_KNOWN;
			tl.st = 10;
			tl.active_quest = this;
			location_event_handler = this;
			at_level = tl.GetLastLevel();
			dungeon_levels = at_level+1;
			levels_cleared = 0;
			whole_location_event_handler = true;
			item_to_give[0] = FindItem("q_orkowie_klucz");
			spawn_item = Quest_Event::Item_GiveSpawned2;
			unit_to_spawn = FindUnitData("q_orkowie_gorush");
			unit_to_spawn2 = g_spawn_groups[SG_ORKOWIE].GetSpawnLeader();
			unit_spawn_level2 = -3;
			spawn_unit_room = RoomTarget::Prison;
			game->quest_orcs2->orcs_state = Quest_Orcs2::State::Accepted;
			// questowe rzeczy
			state = Quest::Started;
			name = game->txQuest[191];
			start_time = game->worldtime;
			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[192], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[193], GetStartLocationName(), GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::ClearedLocation:
		// oczyszczono lokacjê
		{
			msgs.push_back(Format(game->txQuest[194], GetTargetLocationName(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// ukoñczono - nagroda
		{
			state = Quest::Completed;

			game->AddReward(2500);
			msgs.push_back(game->txQuest[195]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

			if(game->quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			{
				game->quest_orcs2->orcs_state = Quest_Orcs2::State::CompletedJoined;
				game->quest_orcs2->days = random(30, 60);
				GetTargetLocation().active_quest = nullptr;
				target_loc = -1;
			}
			else
				game->quest_orcs2->orcs_state = Quest_Orcs2::State::Completed;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}

	prog = prog2;
}

//=================================================================================================
cstring Quest_Orcs::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "naszego_miasta")
		return LocationHelper::IsCity(GetStartLocation()) ? game->txQuest[72] : game->txQuest[73];
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Orcs::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "orkowie") == 0;
}

//=================================================================================================
bool Quest_Orcs::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_dolaczyl") == 0)
		return game->quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined || game->quest_orcs2->orcs_state == Quest_Orcs2::State::CompletedJoined;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Orcs::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started)
	{
		levels_cleared |= (1<<game->dungeon_level);
		if(count_bits(levels_cleared) == dungeon_levels)
			SetProgress(Progress::ClearedLocation);
	}
}

//=================================================================================================
void Quest_Orcs::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, nullptr);
	WriteFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, nullptr);
}

//=================================================================================================
void Quest_Orcs::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, nullptr);
	ReadFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, nullptr);

	location_event_handler = this;
	whole_location_event_handler = true;

	if(!done)
	{
		item_to_give[0] = FindItem("q_orkowie_klucz");
		spawn_item = Quest_Event::Item_GiveSpawned2;
		unit_to_spawn = FindUnitData("q_orkowie_gorush");
		unit_to_spawn2 = g_spawn_groups[SG_ORKOWIE].GetSpawnLeader();
		unit_spawn_level2 = -3;
		spawn_unit_room = RoomTarget::Prison;
	}
}

//=================================================================================================
void Quest_Orcs2::Start()
{
	quest_id = Q_ORCS2;
	type = QuestType::Unique;
	start_loc = -1;
	near_loc = -1;
	talked = Talked::No;
	orcs_state = State::None;
	guard = nullptr;
	orc = nullptr;
	orc_class = OrcClass::None;
}

//=================================================================================================
GameDialog* Quest_Orcs2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);
	
	const string& id = game->current_dialog->talker->data->id;

	if(id == "q_orkowie_slaby")
		return FindDialog("q_orcs2_weak_orc");
	else if(id == "q_orkowie_kowal")
		return FindDialog("q_orcs2_blacksmith");
	else if(id == "q_orkowie_gorush" || id == "q_orkowie_gorush_woj" || id == "q_orkowie_gorush_lowca" || id == "q_orkowie_gorush_szaman")
		return FindDialog("q_orcs2_gorush");
	else
		return FindDialog("q_orcs2_orc");
}

//=================================================================================================
void WarpToThroneOrcBoss()
{
	Game& game = Game::Get();

	// szukaj orka
	UnitData* ud = FindUnitData("q_orkowie_boss");
	Unit* u = nullptr;
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Useable* use = nullptr;
	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_THRONE)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// przenieœ
	game.WarpUnit(*u, use->pos);
}

//=================================================================================================
void Quest_Orcs2::SetProgress(int prog2)
{
	bool apply = true;

	switch(prog2)
	{
	case Progress::TalkedOrc:
		// zapisz gorusha
		{
			orc = game->current_dialog->talker;
			orc->hero->know_name = true;
			orc->hero->name = game->txQuest[216];
		}
		break;
	case Progress::NotJoined:
		break;
	case Progress::Joined:
		// dodaj questa
		{
			start_time = game->worldtime;
			name = game->txQuest[214];
			state = Quest::Started;
			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			msgs.push_back(game->txQuest[197]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);			
			// ustaw stan
			if(orcs_state == Quest_Orcs2::State::Accepted)
				orcs_state = Quest_Orcs2::State::OrcJoined;
			else
			{
				orcs_state = Quest_Orcs2::State::CompletedJoined;
				days = random(30, 60);
				game->quest_orcs->GetTargetLocation().active_quest = nullptr;
				game->quest_orcs->target_loc = -1;
			}
			// do³¹cz do dru¿yny
			game->AddTeamMember(game->current_dialog->talker, true);
			Team.free_recruit = false;

			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::TalkedAboutCamp:
		// powiedzia³ o obozie
		{
			target_loc = game->CreateCamp(game->world_pos, SG_ORKOWIE, 256.f, false);
			Location& target = GetTargetLocation();
			target.state = LS_HIDDEN;
			target.st = 13;
			target.active_quest = this;
			near_loc = game->GetNearestSettlement(target.pos);
			msgs.push_back(game->txQuest[198]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			orcs_state = Quest_Orcs2::State::ToldAboutCamp;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWhereIsCamp:
		// powiedzia³ gdzie obóz
		{
			if(prog == Progress::TalkedWhereIsCamp)
				break;
			Location& target = GetTargetLocation();
			Location& nearl = *game->locations[near_loc];
			target.state = LS_KNOWN;
			done = false;
			location_event_handler = this;
			msgs.push_back(Format(game->txQuest[199], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::ClearedCamp:
		// oczyszczono obóz orków
		{
			orc->StartAutoTalk();
			delete game->news.back();
			game->news.pop_back();
			game->AddNews(game->txQuest[200]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedAfterClearingCamp:
		// pogada³ po oczyszczeniu
		{
			orcs_state = Quest_Orcs2::State::CampCleared;
			days = random(25, 50);
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			msgs.push_back(game->txQuest[201]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::SelectWarrior:
		// zostañ wojownikiem
		apply = false;
		ChangeClass(OrcClass::Warrior);
		break;
	case Progress::SelectHunter:
		// zostañ ³owc¹
		apply = false;
		ChangeClass(OrcClass::Hunter);
		break;
	case Progress::SelectShaman:
		// zostañ szamanem
		apply = false;
		ChangeClass(OrcClass::Shaman);
		break;
	case Progress::SelectRandom:
		// losowo
		{
			OrcClass clas;
			if(game->current_dialog->pc->unit->player->clas == Class::WARRIOR)
			{
				if(rand2() % 2 == 0)
					clas = OrcClass::Hunter;
				else
					clas = OrcClass::Shaman;
			}
			else if(game->current_dialog->pc->unit->player->clas == Class::HUNTER)
			{
				if(rand2() % 2 == 0)
					clas = OrcClass::Warrior;
				else
					clas = OrcClass::Shaman;
			}
			else
			{
				int co = rand2()%3;
				if(co == 0)
					clas = OrcClass::Warrior;
				else if(co == 1)
					clas = OrcClass::Hunter;
				else
					clas = OrcClass::Shaman;
			}

			apply = false;
			ChangeClass(clas);
		}
		break;
	case Progress::TalkedAboutBase:
		// pogada³ o bazie
		{
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 256.f, THRONE_FORT, SG_ORKOWIE, false);
			Location& target = GetTargetLocation();
			done = false;
			target.st = 15;
			target.active_quest = this;
			target.state = LS_HIDDEN;
			msgs.push_back(game->txQuest[202]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			orcs_state = State::ToldAboutBase;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWhereIsBase:
		// powiedzia³ gdzie baza
		{
			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;
			unit_to_spawn = FindUnitData("q_orkowie_boss");
			spawn_unit_room = RoomTarget::Throne;
			callback = WarpToThroneOrcBoss;
			at_level = target.GetLastLevel();
			location_event_handler = nullptr;
			unit_event_handler = this;
			near_loc = game->GetNearestSettlement(target.pos);
			Location& nearl = *game->locations[near_loc];
			msgs.push_back(Format(game->txQuest[203], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str(), target.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			done = false;
			orcs_state = State::GenerateOrcs;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::KilledBoss:
		// zabito bossa
		{
			orc->StartAutoTalk();
			msgs.push_back(game->txQuest[204]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[205]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// pogadano z gorushem
		{
			state = Quest::Completed;
			game->AddReward(random(4000, 5000));
			msgs.push_back(game->txQuest[206]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			quest_manager.EndUniqueQuest();
			// gorush
			game->RemoveTeamMember(orc);
			Useable* tron = game->FindUseableByIdLocal(U_THRONE);
			assert(tron);
			if(tron)
			{
				orc->ai->idle_action = AIController::Idle_WalkUse;
				orc->ai->idle_data.useable = tron;
				orc->ai->timer = 9999.f;
			}
			orc = nullptr;
			// orki
			UnitData* ud[10] = {
				FindUnitData("orc"), FindUnitData("q_orkowie_orc"),
				FindUnitData("orc_fighter"), FindUnitData("q_orkowie_orc_fighter"),
				FindUnitData("orc_hunter"), FindUnitData("q_orkowie_orc_hunter"),
				FindUnitData("orc_shaman"), FindUnitData("q_orkowie_orc_shaman"),
				FindUnitData("orc_chief"), FindUnitData("q_orkowie_orc_chief")
			};
			UnitData* ud_slaby = FindUnitData("q_orkowie_slaby");

			for(vector<Unit*>::iterator it = game->local_ctx.units->begin(), end = game->local_ctx.units->end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsAlive())
				{
					if(u.data == ud_slaby)
					{
						// usuñ dont_attack, od tak :3
						u.dont_attack = false;
						u.ai->change_ai_mode = true;
					}
					else
					{
						for(int i=0; i<5; ++i)
						{
							if(u.data == ud[i*2])
							{
								u.data = ud[i*2+1];
								u.ai->target = nullptr;
								u.ai->alert_target = nullptr;
								u.ai->state = AIController::Idle;
								u.ai->change_ai_mode = true;
								if(game->IsOnline())
								{
									NetChange& c = Add1(game->net_changes);
									c.type = NetChange::CHANGE_UNIT_BASE;
									c.unit = &u;
								}
								break;
							}
						}
					}
				}
			}
			// zak³ada ¿e gadamy na ostatnim levelu, mam nadzieje ¿e gracz z tamt¹d nie spierdoli przed pogadaniem :3
			MultiInsideLocation* multi = (MultiInsideLocation*)game->location;
			for(vector<InsideLocationLevel>::iterator it = multi->levels.begin(), end = multi->levels.end()-1; it != end; ++it)
			{
				for(vector<Unit*>::iterator it2 = it->units.begin(), end2 = it->units.end(); it2 != end2; ++it2)
				{
					Unit& u = **it2;
					if(u.IsAlive())
					{
						for(int i=0; i<5; ++i)
						{
							if(u.data == ud[i*2])
							{
								u.data = ud[i*2+1];
								break;
							}
						}
					}
				}
			}
			// usuñ zw³oki po opuszczeniu lokacji
			orcs_state = State::ClearDungeon;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}

	if(apply)
		prog = prog2;
}

//=================================================================================================
cstring Quest_Orcs2::FormatString(const string& str)
{
	if(str == "name")
		return orc->hero->name.c_str();
	else if(str == "close")
		return game->locations[near_loc]->name.c_str();
	else if(str == "close_dir")
		return GetLocationDirName(game->locations[near_loc]->pos, GetTargetLocation().pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(game->world_pos, GetTargetLocation().pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Orcs2::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "orkowie2") == 0;
}

//=================================================================================================
bool Quest_Orcs2::IfQuestEvent() const
{
	return (In(orcs_state, { State::CompletedJoined, State::CampCleared, State::PickedClass }) && days <= 0);
}

//=================================================================================================
bool Quest_Orcs2::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_woj") == 0)
		return orc_class == OrcClass::Warrior;
	else if(strcmp(msg, "q_orkowie_lowca") == 0)
		return orc_class == OrcClass::Hunter;
	else if(strcmp(msg, "q_orkowie_na_miejscu") == 0)
		return game->current_location == target_loc;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Orcs2::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::TalkedWhereIsCamp)
		SetProgress(Progress::ClearedCamp);
}

//=================================================================================================
void Quest_Orcs2::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == Progress::TalkedWhereIsBase)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Orcs2::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << near_loc;
	f << talked;
	f << orcs_state;
	f << days;
	f << guard;
	f << orc;
	f << orc_class;
	game->SaveStock(file, wares);
}

//=================================================================================================
void Quest_Orcs2::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

	f >> near_loc;
	f >> talked;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> orcs_state;
		f >> days;
		f >> guard;
		f >> orc;
		f >> orc_class;
		game->LoadStock(file, wares);
	}

	if(!done)
	{
		if(prog == Progress::TalkedWhereIsCamp)
			location_event_handler = this;
		else if(prog == Progress::TalkedWhereIsBase)
		{
			unit_to_spawn = FindUnitData("q_orkowie_boss");
			spawn_unit_room = RoomTarget::Throne;
			location_event_handler = nullptr;
			unit_event_handler = this;
			callback = WarpToThroneOrcBoss;
		}
	}
}

//=================================================================================================
void Quest_Orcs2::LoadOld(HANDLE file)
{
	int city, old_refid, old_refid2, where;
	GameReader f(file);

	f >> orcs_state;
	f >> city;
	f >> old_refid;
	f >> old_refid2;
	f >> days;
	f >> where;
	f >> guard;
	f >> orc;
	f >> orc_class;
	game->LoadStock(file, wares);
}

//=================================================================================================
void Quest_Orcs2::ChangeClass(OrcClass new_orc_class)
{
	cstring class_name, udi;
	Class clas;

	orc_class = new_orc_class;

	switch(new_orc_class)
	{
	case OrcClass::Warrior:
	default:
		class_name = game->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		clas = Class::WARRIOR;
		break;
	case OrcClass::Hunter:
		class_name = game->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		clas = Class::HUNTER;
		break;
	case OrcClass::Shaman:
		class_name = game->txQuest[209];
		udi = "q_orkowie_gorush_szaman";
		clas = Class::MAGE;
		break;
	}

	UnitData* ud = FindUnitData(udi);
	orc->hero->clas = clas;

	orc->level = ud->level.x;
	orc->data->GetStatProfile().Set(orc->level, orc->unmod_stats.attrib, orc->unmod_stats.skill);
	orc->CalculateStats();
	orc->RecalculateHp();
	orc->data = ud;
	game->ParseItemScript(*orc, ud->items);
	orc->MakeItemsTeam(false);
	game->UpdateUnitInventory(*orc);

	msgs.push_back(Format(game->txQuest[210], class_name));
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	prog = Progress::ChangedClass;
	orcs_state = State::PickedClass;
	days = random(30, 60);

	if(clas == Class::WARRIOR)
		orc->hero->melee = true;

	if(game->IsOnline())
		game->Net_UpdateQuest(refid);
}
