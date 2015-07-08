#include "Pch.h"
#include "Base.h"
#include "Quest_Orcs.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"

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
			IF_SPECIAL("q_orkowie_dolaczyl"),
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

	if(strcmp(game->current_dialog->talker->data->id, "q_orkowie_straznik") == 0)
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
			// dodaj plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[189], game->locations[start_loc]->name.c_str());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
			game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
		}
		break;
	case Progress::NotAccepted:
		{
			// dodaj plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[190], game->locations[start_loc]->name.c_str());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
			// usuñ stra¿nika
			if(game->orkowie_straznik)
			{
				game->orkowie_straznik->auto_talk = 0;
				game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
				game->orkowie_straznik->temporary = true;
				game->orkowie_straznik = NULL;
			}
		}
		break;
	case Progress::Started:
		{
			// usuñ plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
			}
			// usuñ stra¿nika
			if(game->orkowie_straznik)
			{
				game->orkowie_straznik->auto_talk = false;
				game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
				game->orkowie_straznik->temporary = true;
				game->orkowie_straznik = NULL;
			}
			// generuj lokacje
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
			game->orkowie_gdzie = target_loc;
			unit_to_spawn = FindUnitData("q_orkowie_gorush");
			unit_to_spawn2 = FindUnitData("orc_chief_q");
			unit_spawn_level2 = -3;
			spawn_unit_room = POKOJ_CEL_WIEZIENIE;
			game->orkowie_stan = Game::OS_ZAAKCEPTOWANO;
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
			if(game->orkowie_stan == Game::OS_ORK_DOLACZYL)
			{
				game->orkowie_stan = Game::OS_UKONCZONO_DOLACZYL;
				// odliczanie
				game->orkowie_dni = random(60,90);
				GetTargetLocation().active_quest = NULL;
			}
			else
				game->orkowie_stan = Game::OS_UKONCZONO;
			game->AddReward(2500);
			msgs.push_back(game->txQuest[195]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

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
		return NULL;
	}
}

//=================================================================================================
bool Quest_Orcs::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "orkowie") == 0;
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

	WriteFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, NULL);
	WriteFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, NULL);
}

//=================================================================================================
void Quest_Orcs::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, NULL);
	ReadFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, NULL);

	location_event_handler = this;
	whole_location_event_handler = true;

	if(!done)
	{
		item_to_give[0] = FindItem("q_orkowie_klucz");
		spawn_item = Quest_Event::Item_GiveSpawned2;
		unit_to_spawn = FindUnitData("q_orkowie_gorush");
		unit_to_spawn2 = FindUnitData("orc_chief_q");
		unit_spawn_level2 = -3;
		spawn_unit_room = POKOJ_CEL_WIEZIENIE;
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
				IF_SPECIAL("q_orkowie_woj"),
					TALK(502),
					SET_QUEST_PROGRESS(Quest_Orcs2::Progress::SelectWarrior),
				ELSE,
					IF_SPECIAL("q_orkowie_lowca"),
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
			IF_SPECIAL("q_orkowie_na_miejscu"),
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
}

//=================================================================================================
DialogEntry* Quest_Orcs2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

#define TALKER(x) (strcmp(game->current_dialog->talker->data->id, x) == 0)

	if(TALKER("q_orkowie_slaby"))
		return orcs2_weak_orc;
	else if(TALKER("q_orkowie_kowal"))
		return orcs2_blacksmith;
	else if(TALKER("q_orkowie_gorush") || TALKER("q_orkowie_gorush_woj") || TALKER("q_orkowie_gorush_lowca") || TALKER("q_orkowie_gorush_szaman"))
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
	Unit* u = NULL;
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
	Useable* use = NULL;
	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_TRON)
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
			game->orkowie_gorush = game->current_dialog->talker;
			game->current_dialog->talker->hero->know_name = true;
			game->current_dialog->talker->hero->name = game->txQuest[216];
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
			if(game->orkowie_stan == Game::OS_ZAAKCEPTOWANO)
				game->orkowie_stan = Game::OS_ORK_DOLACZYL;
			else
			{
				game->orkowie_stan = Game::OS_UKONCZONO_DOLACZYL;
				game->orkowie_dni = random(60,90);
				((Quest_Orcs*)game->FindQuest(game->orkowie_refid))->target_loc = NULL;
			}
			// do³¹cz do dru¿yny
			game->current_dialog->talker->hero->free = true;
			game->current_dialog->talker->hero->team_member = true;
			game->current_dialog->talker->hero->mode = HeroData::Follow;
			game->AddTeamMember(game->current_dialog->talker, false);
			game->free_recruit = false;

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RecruitNpc(game->current_dialog->talker);
			}
		}
		break;
	case Progress::TalkedAboutCamp:
		// powiedzia³ o obozie
		{
			target_loc = game->CreateCamp(game->world_pos, SG_ORKOWIE, 256.f, false);
			done = false;
			location_event_handler = this;
			Location& target = GetTargetLocation();
			target.state = LS_HIDDEN;
			target.st = 13;
			target.active_quest = this;
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
			msgs.push_back(game->txQuest[198]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->orkowie_stan = Game::OS_POWIEDZIAL_O_OBOZIE;

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
			game->orkowie_gorush->auto_talk = 1;
			game->AddNews(game->txQuest[200]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedAfterClearingCamp:
		// pogada³ po oczyszczeniu
		{
			game->orkowie_stan = Game::OS_OCZYSZCZONO;
			game->orkowie_dni = random(30,60);
			GetTargetLocation().active_quest = NULL;
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
		ChangeClass(Game::GORUSH_WOJ);
		break;
	case Progress::SelectHunter:
		// zostañ ³owc¹
		apply = false;
		ChangeClass(Game::GORUSH_LOWCA);
		break;
	case Progress::SelectShaman:
		// zostañ szamanem
		apply = false;
		ChangeClass(Game::GORUSH_SZAMAN);
		break;
	case Progress::SelectRandom:
		// losowo
		{
			Game::GorushKlasa klasa;
			if(game->current_dialog->pc->unit->player->clas == Class::WARRIOR)
			{
				if(rand2()%2 == 0)
					klasa = Game::GORUSH_LOWCA;
				else
					klasa = Game::GORUSH_SZAMAN;
			}
			else if(game->current_dialog->pc->unit->player->clas == Class::HUNTER)
			{
				if(rand2()%2 == 0)
					klasa = Game::GORUSH_WOJ;
				else
					klasa = Game::GORUSH_SZAMAN;
			}
			else
			{
				int co = rand2()%3;
				if(co == 0)
					klasa = Game::GORUSH_WOJ;
				else if(co == 1)
					klasa = Game::GORUSH_LOWCA;
				else
					klasa = Game::GORUSH_SZAMAN;
			}

			apply = false;
			ChangeClass(klasa);
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
			game->orkowie_stan = Game::OS_POWIEDZIAL_O_BAZIE;

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
			spawn_unit_room = POKOJ_CEL_TRON;
			callback = WarpToThroneOrcBoss;
			at_level = target.GetLastLevel();
			location_event_handler = NULL;
			unit_event_handler = this;
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
			Location& nearl = *game->locations[near_loc];
			msgs.push_back(Format(game->txQuest[203], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str(), target.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->orkowie_gdzie = target_loc;
			done = false;
			game->orkowie_stan = Game::OS_GENERUJ_ORKI;

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
			game->orkowie_gorush->auto_talk = 1;
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
			game->orkowie_gorush->hero->team_member = false;
			RemoveElementOrder(game->team, game->orkowie_gorush);
			game->orkowie_gorush->MakeItemsTeam(true);
			Useable* tron = game->FindUseableByIdLocal(U_TRON);
			assert(tron);
			if(tron)
			{
				game->orkowie_gorush->ai->idle_action = AIController::Idle_WalkUse;
				game->orkowie_gorush->ai->idle_data.useable = tron;
				game->orkowie_gorush->ai->timer = 9999.f;
			}
			game->orkowie_gorush = NULL;
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
								u.ai->target = NULL;
								u.ai->alert_target = NULL;
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
			game->orkowie_stan = Game::OS_WYCZYSC;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_KickNpc(game->current_dialog->talker);
			}
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
		return game->orkowie_gorush->hero->name.c_str();
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
		return NULL;
	}
}

//=================================================================================================
bool Quest_Orcs2::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "orkowie2") == 0;
}

//=================================================================================================
bool Quest_Orcs2::IfQuestEvent()
{
	return (game->orkowie_stan == Game::OS_UKONCZONO_DOLACZYL ||
		game->orkowie_stan == Game::OS_OCZYSZCZONO ||
		game->orkowie_stan == Game::OS_WYBRAL_KLASE)
		&& game->orkowie_dni <= 0;
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
		unit->event_handler = NULL;
	}
}

//=================================================================================================
void Quest_Orcs2::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &near_loc, sizeof(near_loc), &tmp, NULL);
	WriteFile(file, &talked, sizeof(talked), &tmp, NULL);
}

//=================================================================================================
void Quest_Orcs2::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &near_loc, sizeof(near_loc), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		talked = Talked::No;
	else
		ReadFile(file, &talked, sizeof(talked), &tmp, NULL);

	if(!done)
	{
		if(prog == Progress::TalkedAboutCamp)
			location_event_handler = this;
		else if(prog == Progress::TalkedWhereIsBase)
		{
			unit_to_spawn = FindUnitData("q_orkowie_boss");
			spawn_unit_room = POKOJ_CEL_TRON;
			location_event_handler = NULL;
			unit_event_handler = this;
			callback = WarpToThroneOrcBoss;
		}
	}
}

//=================================================================================================
void Quest_Orcs2::ChangeClass(int klasa)
{
	cstring nazwa, udi;
	Class clas;

	switch(klasa)
	{
	case Game::GORUSH_WOJ:
	default:
		nazwa = game->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		clas = Class::WARRIOR;
		break;
	case Game::GORUSH_LOWCA:
		nazwa = game->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		clas = Class::HUNTER;
		break;
	case Game::GORUSH_SZAMAN:
		nazwa = game->txQuest[209];
		udi = "q_orkowie_gorush_szaman";
		clas = Class::MAGE;
		break;
	}

	UnitData* ud = FindUnitData(udi);
	Unit* u = game->orkowie_gorush;
	u->hero->clas = clas;

	u->level = ud->level.x;
	u->data->GetStatProfile().Set(u->level, u->unmod_stats.attrib, u->unmod_stats.skill);
	u->CalculateStats();
	u->RecalculateHp();
	u->data = ud;
	game->ParseItemScript(*u, ud->items);
	u->MakeItemsTeam(false);
	game->UpdateUnitInventory(*u);

	msgs.push_back(Format(game->txQuest[210], nazwa));
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	prog = Progress::ChangedClass;
	game->orkowie_stan = Game::OS_WYBRAL_KLASE;
	game->orkowie_dni = random(60,90);

	if(clas == Class::WARRIOR)
		u->hero->melee = true;

	if(game->IsOnline())
		game->Net_UpdateQuest(refid);
}
