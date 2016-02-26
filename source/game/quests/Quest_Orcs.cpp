#include "Pch.h"
#include "Base.h"
#include "Quest_Orcs.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
DialogEntry orcs_guard[] = {
	TALK(462),
	IF_QUEST_PROGRESS(Quest_Orcs::Progress::None),
		TALK(463),
		TALK(464),
		TALK(465),
		SET_QUEST_PROGRESS(Quest_Orcs::Progress::TalkedWithGuard),
	END_IF,
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry orcs_captain[] = {
	IF_QUEST_PROGRESS_RANGE(Quest_Orcs::Progress::None, Quest_Orcs::Progress::NotAccepted),
		TALK(466),
		TALK(467),
		CHOICE(468),
			SET_QUEST_PROGRESS(Quest_Orcs::Progress::Started),
			TALK2(469),
			TALK(470),
			END,
		END_CHOICE,
		CHOICE(471),
			SET_QUEST_PROGRESS(Quest_Orcs::Progress::NotAccepted),
			TALK(472),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		IF_QUEST_PROGRESS(Quest_Orcs::Progress::Started),
			TALK2(473),
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Orcs::Progress::ClearedLocation),
			TALK(474),
			SET_QUEST_PROGRESS(Quest_Orcs::Progress::Finished),
			TALK(475),
			IF_QUEST_SPECIAL("q_orkowie_dolaczyl"),
				TALK(476),
			END_IF,
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Orcs::Progress::Finished),
			TALK2(477),
			END,
		END_IF,
	END_IF,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Orcs::Start()
{
	quest_id = Q_ORCS;
	type = Type::Unique;
}

//=================================================================================================
DialogEntry* Quest_Orcs::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(game->current_dialog->talker->data->id == "q_orkowie_straznik")
		return orcs_guard;
	else
		return orcs_captain;
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
			if(!game->quest_rumor[P_ORKOWIE])
			{
				game->quest_rumor[P_ORKOWIE] = true;
				--game->quest_rumor_counter;
				cstring text = Format(game->txQuest[189], game->locations[start_loc]->name.c_str());
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
			game->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::NotAccepted:
		{
			// add gossip
			if(!game->quest_rumor[P_ORKOWIE])
			{
				game->quest_rumor[P_ORKOWIE] = true;
				--game->quest_rumor_counter;
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
				u->auto_talk = 0;
				u->temporary = true;
				u = nullptr;
			}
			game->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::Started:
		{
			// remove rumor from pool
			if(!game->quest_rumor[P_ORKOWIE])
			{
				game->quest_rumor[P_ORKOWIE] = true;
				--game->quest_rumor_counter;
			}
			// mark guard to remove
			Unit*& u = game->quest_orcs2->guard;
			if(u)
			{
				u->auto_talk = 0;
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
			quest_index = game->quests.size();
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
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
		return GetStartLocation().type == L_CITY ? game->txQuest[72] : game->txQuest[73];
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

//-----------------------------------------------------------------------------
DialogEntry orcs2_gorush[] = {
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::Finished),
		TALK(478),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::KilledBoss),
		TALK(479),
		TALK(480),
		TALK(481),
		TALK(482),
		TALK(483),
		TALK(484),
		SET_QUEST_PROGRESS(Quest_Orcs2::Progress::Finished),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::ClearedCamp),
		TALK(485),
		TALK(486),
		TALK(487),
		TALK(488),
		SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedAfterClearingCamp),
		END,
	END_IF,
	IF_QUEST_EVENT,
		IF_QUEST_PROGRESS(Quest_Orcs2::Progress::ChangedClass),
			TALK(489),
			TALK(490),
			TALK(491),
			SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedAboutBase),
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedAfterClearingCamp),
			TALK(492),
			TALK(493),
			TALK(494),
			CHOICE(495),
				TALK(496),
				SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectWarrior),
				END,
			END_CHOICE,
			CHOICE(497),
				TALK(498),
				SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectHunter),
				END,
			END_CHOICE,
			CHOICE(499),
				TALK(500),
				SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectShaman),
				END,
			END_CHOICE,
			CHOICE(501),
				SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectRandom),
				IF_QUEST_SPECIAL("q_orkowie_woj"),
					TALK(502),
					SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectWarrior),
				ELSE,
					IF_QUEST_SPECIAL("q_orkowie_lowca"),
						TALK(503),
						SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectHunter),
					ELSE,
						TALK(504),
						SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectShaman),
					END_IF,
				END_IF,
				END,
			END_CHOICE,
			SHOW_CHOICES,
		END_IF,
		TALK(505),
		TALK(506),
		TALK(507),
		TALK(508),
		TALK(509),
		SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedAboutCamp),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Orcs2::Progress::None, Quest_Orcs2::Progress::TalkedOrc),
		SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedOrc),
		TALK2(510),
		TALK(511),
		TALK(512),
		CHOICE(513),
			TALK(514),
			SET_QUEST_PROGRESS(Quest_Orcs2::Progress::Joined),
			END,
		END_CHOICE,
		CHOICE(515),
			TALK(516),
			SET_QUEST_PROGRESS(Quest_Orcs2::Progress::NotJoined),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::NotJoined),
		TALK(517),
		TALK(518),
		CHOICE(519),
			TALK(520),
			SET_QUEST_PROGRESS(Quest_Orcs2::Progress::Joined),
			END,
		END_CHOICE,
		CHOICE(521),
			TALK(522),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	RANDOM_TEXT(3),
		TALK(523),
		TALK(524),
		TALK(525),
	IF_QUEST_PROGRESS_RANGE(Quest_Orcs2::Progress::TalkedAboutCamp, Quest_Orcs2::Progress::TalkedWhereIsCamp),
		CHOICE(526),
			IF_QUEST_SPECIAL("q_orkowie_na_miejscu"),
				TALK(527),
			ELSE,
				TALK2(528),
				SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedWhereIsCamp),
			END_IF,
			END,
		END_CHOICE,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedAboutBase),
		CHOICE(529),
			TALK2(530),
			TALK(531),
			TALK(532),
			SET_QUEST_PROGRESS(Quest_Orcs2::Progress::TalkedWhereIsBase),
			END,
		END_CHOICE,
	END_IF,
	CHOICE(533),
		TALK(534),
		TALK(535),
		TALK(536),
		TALK(537),
		TALK(538),
		TALK(539),
		TALK(540),
		TALK(541),
		TALK(542),
		END,
	END_CHOICE,
	CHOICE(543),
		TALK(544),
		TALK(545),
		END,
	END_CHOICE,
	CHOICE(546),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry orcs2_weak_orc[] = {
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::Finished),
		TALK(547),
	ELSE,
		RANDOM_TEXT(2),
			TALK(548),
			TALK(549),
	END_IF,
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry orcs2_blacksmith[] = {
	IF_QUEST_PROGRESS(Quest_Orcs2::Progress::Finished),
		TALK(550),
		CHOICE(551),
			TRADE,
			END,
		END_CHOICE,
		CHOICE(552),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		TALK(553),
		END,
	END_IF,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry orcs2_orc[] = {
	RANDOM_TEXT(3),
		TALK(554),
		TALK(555),
		TALK(556),
	END,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Orcs2::Start()
{
	quest_id = Q_ORCS2;
	type = Type::Unique;
	start_loc = -1;
	near_loc = -1;
	talked = Talked::No;
	orcs_state = State::None;
	guard = nullptr;
	orc = nullptr;
	orc_class = OrcClass::None;
}

//=================================================================================================
DialogEntry* Quest_Orcs2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

#define TALKER(x) (strcmp(game->current_dialog->talker->data->id, x) == 0)

	const string& id = game->current_dialog->talker->data->id;

	if(id == "q_orkowie_slaby")
		return orcs2_weak_orc;
	else if(id == "q_orkowie_kowal")
		return orcs2_blacksmith;
	else if(id == "q_orkowie_gorush" || id == "q_orkowie_gorush_woj" || id == "q_orkowie_gorush_lowca" || id == "q_orkowie_gorush_szaman")
		return orcs2_gorush;
	else
		return orcs2_orc;
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
			quest_index = game->quests.size();
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
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
			game->free_recruit = false;

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
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
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
			orc->auto_talk = 1;
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
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
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
			orc->auto_talk = 1;
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
			game->EndUniqueQuest();
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

	if(LOAD_VERSION != V_0_2)
		f >> talked;
	else
		talked = Talked::No;

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
	int city, refid, refid2, where;
	GameReader f(file);

	f >> orcs_state;
	f >> city;
	f >> refid;
	f >> refid2;
	f >> days;
	f >> where;
	f >> guard;
	f >> orc;
	f >> orc_class;
	game->LoadStock(file, wares);
}

//=================================================================================================
void Quest_Orcs2::ChangeClass(OrcClass orc_class)
{
	cstring nazwa, udi;
	Class clas;

	switch(orc_class)
	{
	case OrcClass::Warrior:
	default:
		nazwa = game->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		clas = Class::WARRIOR;
		break;
	case OrcClass::Hunter:
		nazwa = game->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		clas = Class::HUNTER;
		break;
	case OrcClass::Shaman:
		nazwa = game->txQuest[209];
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
	game->ParseItemScript(*orc, ud->items, ud->new_items);
	orc->MakeItemsTeam(false);
	game->UpdateUnitInventory(*orc);

	msgs.push_back(Format(game->txQuest[210], nazwa));
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
