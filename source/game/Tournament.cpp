// obs³uga zawodów na arenie
#include "Pch.h"
#include "Game.h"
#include "Quest_Mages.h"

//=================================================================================================
void Game::StartTournament(Unit* arena_master)
{
	tournament_year = year;
	tournament_state = TOURNAMENT_STARTING;
	tournament_timer = 0.f;
	tournament_state2 = 0;
	tournament_master = arena_master;
	tournament_units.clear();
	tournament_generated = false;
	tournament_winner = nullptr;
	tournament_pairs.clear();
	tournament_skipped_unit = nullptr;
}

//=================================================================================================
bool Game::IfUnitJoinTournament(Unit& u)
{
	if(u.IsStanding() && u.IsHero() && u.frozen == 0)
	{
		if(IS_SET(u.data->flags2, F2_TOURNAMENT))
			return true;
		else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE) && quest_mages2->mages_state >= Quest_Mages2::State::MageCured)
		{
			if(!u.IsTeamMember())
			{
				// po wszystkim wróæ do karczmy
				u.ai->goto_inn = true;
			}
			return true;
		}
	}
	return false;
}

//=================================================================================================
void Game::GenerateTournamentUnits()
{
	VEC3 pos = city_ctx->FindBuilding(B_ARENA)->walk_pt;
	tournament_master = FindUnitByIdLocal("arena_master");

	// przenieœ herosów przed arenê
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(IfUnitJoinTournament(u) && !u.IsFollowingTeamMember())
			WarpNearLocation(local_ctx, u, pos, 12.f, false);
	}
	InsideBuilding* inn = city_ctx->FindInn();
	for(vector<Unit*>::iterator it = inn->units.begin(), end = inn->units.end(); it != end;)
	{
		Unit& u = **it;
		if(IfUnitJoinTournament(u) && !u.IsFollowingTeamMember())
		{
			BreakAction(u, false, true);
			u.in_building = -1;
			WarpNearLocation(local_ctx, u, pos, 12.f, false);
			local_ctx.units->push_back(&u);
			it = inn->units.erase(it);
			end = inn->units.end();
		}
		else
			++it;
	}

	// generuj herosów
	int ile = random(6,9);
	for(int i=0; i<ile; ++i)
	{
		Unit* u = SpawnUnitNearLocation(local_ctx, pos, *GetRandomHeroData(), nullptr, random(5,20), 12.f);
		if(u)
		{
			u->temporary = true;
			if(IsOnline())
				Net_SpawnUnit(u);
		}
	}

	tournament_generated = true;
}

//=================================================================================================
void Game::UpdateTournament(float dt)
{
	if(tournament_state == TOURNAMENT_STARTING)
	{
		if(tournament_master->busy == Unit::Busy_No)
			tournament_timer += dt;

		// do³¹czanie cz³onków dru¿yny
		const VEC3& walk_pt = city_ctx->FindBuilding(B_ARENA)->walk_pt; 
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.busy == Unit::Busy_No && distance2d(u.pos, tournament_master->pos) <= 16.f && !u.dont_attack && IfUnitJoinTournament(u))
			{
				u.busy = Unit::Busy_Tournament;
				u.ai->idle_action = AIController::Idle_Move;
				u.ai->idle_data.pos.Build(walk_pt);
				u.ai->timer = random(5.f,10.f);

				UnitTalk(**it, random_string(txAiJoinTour));
			}
		}

		if(tournament_state2 == 0)
		{
			if(tournament_timer >= 5.f)
			{
				tournament_state2 = 1;
				TournamentTalk(txTour[0]);
			}
		}
		else if(tournament_state2 == 1)
		{
			if(tournament_timer >= 30.f)
			{
				tournament_state2 = 2;
				TournamentTalk(txTour[1]);
			}
		}
		else if(tournament_state2 == 2)
		{
			if(tournament_timer >= 60.f)
			{
				// zbierz npc
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
				{
					Unit& u = **it;
					if(distance2d(u.pos, tournament_master->pos) < 64.f && IfUnitJoinTournament(u))
					{
						tournament_units.push_back(&u);
						u.busy = Unit::Busy_Tournament;
					}
				}

				city_ctx->FindInsideBuilding(B_ARENA, tournament_arena);
				tournament_state2 = 3;
				tournament_round = 0;
				tournament_master->busy = Unit::Busy_Yes;
				TournamentTalk(txTour[2]);
				tournament_skipped_unit = nullptr;
				StartTournamentRound();
			}
		}
		else if(tournament_state2 == 3)
		{
			if(!tournament_master->talking)
			{
				if(!tournament_pairs.empty())
				{
					tournament_state2 = 4;
					TournamentTalk(Format(txTour[3], tournament_pairs.size()*2 + (tournament_skipped_unit ? 1 : 0)));
					SpawnArenaViewers(5);
				}
				else
				{
					tournament_state2 = 5;
					TournamentTalk(txTour[22]);
				}
			}
		}
		else if(tournament_state2 == 4)
		{
			if(!tournament_master->talking)
			{
				tournament_state = TOURNAMENT_IN_PROGRESS;
				tournament_state2 = 0;
				tournament_state3 = 0;
			}
		}
		else
		{
			if(!tournament_master->talking)
			{
				tournament_state = TOURNAMENT_NOT_DONE;
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
				{
					Unit& u = **it;
					if(u.busy == Unit::Busy_Tournament)
						u.busy = Unit::Busy_No;
				}
				tournament_master->busy = Unit::Busy_No;
			}
		}
	}
	else
	{
		if(tournament_state2 == 0)
		{
			// gadanie przed walk¹
			if(!tournament_master->talking)
			{
				cstring text;
				if(tournament_state3 == 0)
					text = Format(txTour[4], tournament_round+1);
				else if(tournament_state3 <= (int)tournament_pairs.size())
				{
					std::pair<Unit*, Unit*>& p = tournament_pairs[tournament_state3-1];
					text = Format(txTour[5], p.first->GetRealName(), p.second->GetRealName());
				}
				else if(tournament_state3 == (int)tournament_pairs.size()+1)
				{
					if(tournament_skipped_unit)
						text = Format(txTour[6], tournament_skipped_unit->GetRealName());
					else
						text = txTour[tournament_round == 0 ? 7 : 8];
				}
				else
					text = txTour[tournament_round == 0 ? 7 : 8];

				TournamentTalk(text);

				++tournament_state3;
				bool koniec = false;
				if(tournament_state3 == (int)tournament_pairs.size()+1)
				{
					if(!tournament_skipped_unit)
						koniec = true;
				}
				else if(tournament_state3 == (int)tournament_pairs.size()+2)
					koniec = true;

				if(koniec)
				{
					tournament_state2 = 1;
					tournament_state3 = 0;
					std::reverse(tournament_pairs.begin(), tournament_pairs.end());
				}
			}
		}
		else if(tournament_state2 == 1)
		{
			if(tournament_state3 == 0)
			{
				// gadanie przed walk¹
				if(!tournament_master->talking)
				{
					std::pair<Unit*, Unit*>& p = tournament_pairs.back();
					if(p.first && p.second)
						TournamentTalk(Format(txTour[9], p.first->GetRealName(), p.second->GetRealName()));
					else
						TournamentTalk(txTour[10]);
					tournament_state3 = 1;
				}
			}
			else if(tournament_state3 == 1)
			{
				// sprawdza czy s¹ w pobli¿u, rozpoczyna walkê lub mówi ¿e ich nie ma na arenie
				if(!tournament_master->talking)
				{
					std::pair<Unit*, Unit*> p = tournament_pairs.back();
					tournament_pairs.pop_back();
					if(!p.first || !p.second)
					{
						if(p.first)
							tournament_units.push_back(p.first);
						else if(p.second)
							tournament_units.push_back(p.second);
					}
					else if(!p.first->IsStanding() || p.first->frozen != 0 || !(distance2d(p.first->pos, tournament_master->pos) <= 64.f || p.first->in_building == tournament_arena))
					{
						tournament_state3 = 2;
						tournament_other_fighter = p.second;
						TournamentTalk(Format(txTour[11], p.first->GetRealName()));
					}
					else if(!p.second->IsStanding() || p.second->frozen != 0 || !(distance2d(p.second->pos, tournament_master->pos) <= 64.f || p.second->in_building == tournament_arena))
					{
						tournament_state3 = 3;
						tournament_units.push_back(p.first);
						TournamentTalk(Format(txTour[12], p.second->GetRealName(), p.first->GetRealName()));
					}
					else
					{
						// walka
						tournament_master->busy = Unit::Busy_No;
						tournament_state3 = 4;
						arena_free = false;
						arena_tryb = Arena_Zawody;
						arena_etap = Arena_OdliczanieDoPrzeniesienia;
						arena_t = 0.f;
						at_arena.clear();
						at_arena.push_back(p.first);
						at_arena.push_back(p.second);

						p.first->busy = Unit::Busy_No;
						p.first->frozen = 2;
						if(p.first->IsPlayer())
						{
							p.first->player->arena_fights++;
							if(IsOnline())
								p.first->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.first->player == pc)
							{
								fallback_co = FALLBACK_ARENA;
								fallback_t = -1.f;
							}
							else
							{
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::ENTER_ARENA;
								c.pc = p.first->player;
								GetPlayerInfo(c.pc).NeedUpdate();
							}
						}

						p.second->busy = Unit::Busy_No;
						p.second->frozen = 2;
						if(p.second->IsPlayer())
						{
							p.second->player->arena_fights++;
							if(IsOnline())
								p.second->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.second->player == pc)
							{
								fallback_co = FALLBACK_ARENA;
								fallback_t = -1.f;
							}
							else
							{
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::ENTER_ARENA;
								c.pc = p.second->player;
								GetPlayerInfo(c.pc).NeedUpdate();
							}
						}
					}
				}
			}
			else if(tournament_state3 == 2)
			{
				// sprawdŸ czy drugi zawodnik przyszed³
				if(!tournament_master->talking)
				{
					if(tournament_other_fighter)
					{
						if(!tournament_other_fighter->IsStanding() || tournament_other_fighter->frozen != 0 ||
							!(distance2d(tournament_other_fighter->pos, tournament_master->pos) <= 64.f || tournament_other_fighter->in_building == tournament_arena))
						{
							TournamentTalk(Format(txTour[13], tournament_other_fighter->GetRealName()));
						}
						else
						{
							tournament_units.push_back(tournament_other_fighter);
							TournamentTalk(Format(txTour[14], tournament_other_fighter->GetRealName()));
						}
					}

					tournament_state3 = 3;
				}
			}
			else if(tournament_state3 == 3)
			{
				// jeden lub obaj zawodnicy nie przyszli
				// lub pogada³ po walce
				if(!tournament_master->talking)
				{
					if(tournament_pairs.empty())
					{
						// nie ma ju¿ walk w tej rundzie
						if(tournament_units.size() <= 1 && !tournament_skipped_unit)
						{
							// jest zwyciêzca / lub nie ma nikogo
							if(!tournament_units.empty())
								tournament_winner = tournament_units.back();
							else
								tournament_winner = tournament_skipped_unit;

							if(!tournament_winner)
							{
								TournamentTalk(txTour[15]);
								tournament_state3 = 1;
								AddNews(txTour[17]);
								tournament_state = TOURNAMENT_NOT_DONE;
								tournament_master->busy = Unit::Busy_No;
							}
							else
							{
								TournamentTalk(Format(txTour[16], tournament_winner->GetRealName()));
								tournament_state3 = 0;
								AddNews(Format(txTour[18], tournament_winner->GetRealName()));
							}

							tournament_units.clear();
							tournament_state2 = 2;

							RemoveArenaViewers();
							arena_viewers.clear();
						}
						else
						{
							// kolejna runda
							++tournament_round;
							StartTournamentRound();
							tournament_state2 = 0;
							tournament_state3 = 0;
						}
					}
					else
					{
						// kolejna walka
						tournament_state3 = 0;
					}
				}
			}
			else if(tournament_state3 == 4)
			{
				// trwa walka, obs³ugiwane przez kod areny
			}
			else if(tournament_state3 == 5)
			{
				// po walce
				if(!tournament_master->talking && tournament_master->busy == Unit::Busy_No)
				{
					// daj miksturki lecznicze
					static const Item* p1 = FindItem("p_hp");
					static const Item* p2 = FindItem("p_hp2");
					static const Item* p3 = FindItem("p_hp3");
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						Unit& u = **it;
						float mhp = u.hpmax - u.hp;
						int given_items = 0;
						if(mhp > 0.f && u.IsAI())
							u.ai->have_potion = 2;
						while(mhp >= 600.f)
						{
							u.AddItem(p3, 1, false);
							mhp -= 600.f;
							++given_items;
						}
						if(mhp >= 400.f)
						{
							u.AddItem(p2, 1, false);
							mhp -= 400.f;
							++given_items;
						}
						while(mhp > 0.f)
						{
							u.AddItem(p1, 1, false);
							mhp -= 200.f;
							++given_items;
						}
						if(u.IsPlayer() && given_items)
						{
							if(u.player == pc)
							{
								if(given_items == 1)
									AddGameMsg3(GMS_ADDED_ITEM);
								else
									AddGameMsg(Format(txGmsAddedItems, given_items), 3.f);
							}
							else
							{
								if(given_items == 1)
									Net_AddedItemMsg(u.player);
								else
								{
									NetChangePlayer& c = Add1(net_changes_player);
									c.type = NetChangePlayer::ADDED_ITEM_MSG;
									c.ile = given_items;
									c.pc = u.player;
									GetPlayerInfo(c.pc).NeedUpdate();
								}
							}
						}
					}

					// zwyciêzca
					Unit* wygrany = at_arena[arena_wynik];
					wygrany->busy = Unit::Busy_Tournament;
					tournament_units.push_back(wygrany);
					TournamentTalk(Format(txTour[rand2()%2 == 0 ? 19 : 20], wygrany->GetRealName()));
					tournament_state3 = 3;
					at_arena.clear();
				}
			}
		}
		else if(tournament_state2 == 2)
		{
			if(!tournament_master->talking)
			{
				if(tournament_state3 == 0)
				{
					const int NAGRODA = 5000;

					tournament_state3 = 1;
					tournament_master->look_target = tournament_winner;
					tournament_winner->gold += NAGRODA;
					if(tournament_winner->IsHero())
					{
						tournament_winner->look_target = tournament_master;
						tournament_winner->hero->LevelUp();
					}
					else
					{
						tournament_winner->busy = Unit::Busy_No;
						if(tournament_winner->player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::GOLD_MSG;
							c.id = 1;
							c.ile = NAGRODA;
							c.pc = tournament_winner->player;
							GetPlayerInfo(c.pc).NeedUpdateAndGold();
						}
						else
							AddGameMsg(Format(txGoldPlus, NAGRODA), 3.f);
					}
					TournamentTalk(Format(txTour[21], tournament_winner->GetRealName()));

					tournament_master->busy = Unit::Busy_No;
					tournament_master->ai->idle_action = AIController::Idle_None;
				}
				else if(tournament_state3 == 1)
				{
					// koniec zawodów
					if(tournament_winner && tournament_winner->IsHero())
					{
						tournament_winner->look_target = nullptr;
						tournament_winner->ai->idle_action = AIController::Idle_None;
						tournament_winner->busy = Unit::Busy_No;
					}
					tournament_master->look_target = nullptr;
					tournament_state = TOURNAMENT_NOT_DONE;
				}
			}
		}
	}
}

//=================================================================================================
void Game::StartTournamentRound()
{
	std::random_shuffle(tournament_units.begin(), tournament_units.end(), myrand);
	tournament_pairs.clear();

	Unit* first = tournament_skipped_unit;
	for(vector<Unit*>::iterator it = tournament_units.begin(), end = tournament_units.end(); it != end; ++it)
	{
		if(!first)
			first = *it;
		else
		{
			tournament_pairs.push_back(std::pair<Unit*, Unit*>(first, *it));
			first = nullptr;
		}
	}

	tournament_skipped_unit = first;
	tournament_units.clear();
}

//=================================================================================================
void Game::TournamentTalk(cstring text)
{
	UnitTalk(*tournament_master, text);
	game_gui->AddSpeechBubble(VEC3_x0y(GetArena()->exit_area.Midpoint(), 1.5f), text);
}

//=================================================================================================
void Game::TournamentTrain(Unit& u)
{
	tournament_master = nullptr;
	Train(u, false, (int)Attribute::STR);
	Train(u, false, (int)Attribute::END);
	Train(u, false, (int)Attribute::DEX);
	if(u.HaveWeapon())
	{
		Train(u, true, (int)Skill::ONE_HANDED_WEAPON);
		Train(u, true, (int)u.GetWeapon().GetInfo().skill);
	}
	if(u.HaveBow())
		Train(u, true, (int)Skill::BOW);
	if(u.HaveShield())
		Train(u, true, (int)Skill::SHIELD);
	if(u.HaveArmor())
		Train(u, true, (int)u.GetArmor().skill);
}

//=================================================================================================
void Game::CleanArena()
{
	InsideBuilding* arena = city_ctx->FindInsideBuilding(B_ARENA);

	// wyrzuæ ludzi z areny
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.frozen = 0;
		u.in_arena = -1;
		u.in_building = -1;
		u.busy = Unit::Busy_No;
		if(u.hp <= 0.f)
		{
			u.HealPoison();
			u.live_state = Unit::ALIVE;
		}
		if(u.IsAI())
			u.ai->Reset();
		WarpUnit(u, arena->outside_spawn);
		u.rot = arena->outside_rot;
	}
	RemoveArenaViewers();
	at_arena.clear();
	arena_free = true;
	arena_tryb = Arena_Brak;
}

//=================================================================================================
void Game::CleanTournament()
{
	if(!arena_free)
		CleanArena();

	// zmieñ zajêtoœæ
	for(vector<Unit*>::iterator it = tournament_units.begin(), end = tournament_units.end(); it != end; ++it)
		(*it)->busy = Unit::Busy_No;
	tournament_units.clear();
	for(vector<std::pair<Unit*, Unit*> >::iterator it2 = tournament_pairs.begin(), end2 = tournament_pairs.end(); it2 != end2; ++it2)
	{
		it2->first->busy = Unit::Busy_No;
		it2->second->busy = Unit::Busy_No;
	}
	tournament_pairs.clear();
	if(tournament_skipped_unit)
	{
		tournament_skipped_unit->busy = Unit::Busy_No;
		tournament_skipped_unit = nullptr;
	}
	tournament_master->busy = Unit::Busy_No;
	tournament_master = nullptr;
	tournament_winner = nullptr;

	tournament_state = TOURNAMENT_NOT_DONE;
}
