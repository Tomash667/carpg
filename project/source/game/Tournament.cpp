// obs³uga zawodów na arenie
#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Quest_Mages.h"
#include "City.h"
#include "GameGui.h"
#include "AIController.h"
#include "Team.h"

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
	tournament_other_fighter = nullptr;
	city_ctx->FindInsideBuilding(BuildingGroup::BG_ARENA, tournament_arena);
}

//=================================================================================================
bool Game::IfUnitJoinTournament(Unit& u)
{
	if(u.summoner)
		return false;
	if(u.IsStanding() && u.IsHero() && u.frozen == FROZEN::NO)
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
	Vec3 pos = city_ctx->FindBuilding(BuildingGroup::BG_ARENA)->walk_pt;
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
			BreakUnitAction(u, BREAK_ACTION_MODE::NORMAL, true);
			u.in_building = -1;
			WarpNearLocation(local_ctx, u, pos, 12.f, false);
			local_ctx.units->push_back(&u);
			it = inn->units.erase(it);
			end = inn->units.end();
		}
		else
			++it;
	}

	// generate heroes
	int count = Random(6, 9);
	for(int i = 0; i < count; ++i)
	{
		Unit* u = SpawnUnitNearLocation(local_ctx, pos, *GetRandomHeroData(), nullptr, Random(5, 20), 12.f);
		if(u)
		{
			u->temporary = true;
			if(Net::IsOnline())
				Net_SpawnUnit(u);
		}
	}

	tournament_generated = true;
}

//=================================================================================================
void Game::UpdateTournament(float dt)
{
	if(arena_free && !tournament_master->IsAlive())
	{
		CleanTournament();
		return;
	}

	// info about getting out of range
	for(auto& unit : tournament_units)
		VerifyTournamentUnit(unit);
	for(auto& pair : tournament_pairs)
	{
		VerifyTournamentUnit(pair.first);
		VerifyTournamentUnit(pair.second);
	}
	VerifyTournamentUnit(tournament_skipped_unit);
	VerifyTournamentUnit(tournament_other_fighter);

	if(tournament_state == TOURNAMENT_STARTING)
	{
		if(tournament_master->busy == Unit::Busy_No && tournament_master->IsStanding() && tournament_master->ai->state == AIController::Idle)
			tournament_timer += dt;

		// team members joining
		const Vec3& walk_pt = city_ctx->FindBuilding(BuildingGroup::BG_ARENA)->walk_pt;
		for(Unit* unit : Team.members)
		{
			if(unit->busy == Unit::Busy_No && Vec3::Distance2d(unit->pos, tournament_master->pos) <= 16.f && !unit->dont_attack && IfUnitJoinTournament(*unit))
			{
				unit->busy = Unit::Busy_Tournament;
				unit->ai->idle_action = AIController::Idle_Move;
				unit->ai->idle_data.pos = walk_pt;
				unit->ai->timer = Random(5.f, 10.f);

				UnitTalk(*unit, random_string(txAiJoinTour));
			}
		}

		if(tournament_state2 == 0)
		{
			if(tournament_timer >= 5.f)
			{
				// tell to join
				tournament_state2 = 1;
				TournamentTalk(txTour[0]);
			}
		}
		else if(tournament_state2 == 1)
		{
			if(tournament_timer >= 30.f)
			{
				// tell to join 2
				tournament_state2 = 2;
				TournamentTalk(txTour[1]);
			}
		}
		else if(tournament_state2 == 2)
		{
			if(tournament_timer >= 60.f)
			{
				// gather npc's
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
				{
					Unit& u = **it;
					if(Vec3::Distance2d(u.pos, tournament_master->pos) < 64.f && IfUnitJoinTournament(u))
					{
						tournament_units.push_back(&u);
						u.busy = Unit::Busy_Tournament;
					}
				}

				// start tournament
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
			if(tournament_master->CanAct())
			{
				if(!tournament_pairs.empty())
				{
					// tell how many units joined
					tournament_state2 = 4;
					TournamentTalk(Format(txTour[3], tournament_pairs.size() * 2 + (tournament_skipped_unit ? 1 : 0)));
					SpawnArenaViewers(5);
				}
				else
				{
					// not enought units
					tournament_state2 = 5;
					TournamentTalk(txTour[22]);
				}
			}
		}
		else if(tournament_state2 == 4)
		{
			if(tournament_master->CanAct())
			{
				tournament_state = TOURNAMENT_IN_PROGRESS;
				tournament_state2 = 0;
				tournament_state3 = 0;
			}
		}
		else
		{
			if(tournament_master->CanAct())
			{
				// not enough units, end tournament
				CleanTournament();
			}
		}
	}
	else
	{
		if(tournament_state2 == 0)
		{
			// introduce all fight in this round
			if(tournament_master->CanAct())
			{
				cstring text;
				if(tournament_state3 == 0)
					text = Format(txTour[4], tournament_round + 1);
				else if(tournament_state3 <= (int)tournament_pairs.size())
				{
					auto& p = tournament_pairs[tournament_state3 - 1];
					text = Format(txTour[5], p.first->GetRealName(), p.second->GetRealName());
				}
				else if(tournament_state3 == (int)tournament_pairs.size() + 1)
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
				bool end = false;
				if(tournament_state3 == (int)tournament_pairs.size() + 1)
				{
					if(!tournament_skipped_unit)
						end = true;
				}
				else if(tournament_state3 == (int)tournament_pairs.size() + 2)
					end = true;

				if(end)
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
				// tell next fight participants
				if(tournament_master->CanAct())
				{
					auto& p = tournament_pairs.back();
					TournamentTalk(Format(txTour[9], p.first->GetRealName(), p.second->GetRealName()));
					tournament_state3 = 1;
				}
			}
			else if(tournament_state3 == 1)
			{
				// check if units are ready to fight
				if(tournament_master->CanAct())
				{
					auto& p = tournament_pairs.back();
					if(p.first->to_remove || !p.first->IsStanding() || p.first->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.first->pos, tournament_master->pos) <= 64.f || p.first->in_building == tournament_arena))
					{
						// first unit left or can't fight, check other
						tournament_state3 = 2;
						tournament_other_fighter = p.second;
						TournamentTalk(Format(txTour[11], p.first->GetRealName()));
						if(!p.first->to_remove && p.first->IsPlayer())
							AddGameMsg3(p.first->player, GMS_LEFT_EVENT);
					}
					else if(p.second->to_remove || !p.second->IsStanding() || p.second->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.second->pos, tournament_master->pos) <= 64.f || p.second->in_building == tournament_arena))
					{
						// second unit left or can't fight, first automaticaly goes to next round
						tournament_state3 = 3;
						tournament_units.push_back(p.first);
						TournamentTalk(Format(txTour[12], p.second->GetRealName(), p.first->GetRealName()));
						if(!p.second->to_remove && p.second->IsPlayer())
							AddGameMsg3(p.second->player, GMS_LEFT_EVENT);
					}
					else
					{
						// fight
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
						p.first->frozen = FROZEN::YES;
						if(p.first->IsPlayer())
						{
							p.first->player->arena_fights++;
							if(Net::IsOnline())
								p.first->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.first->player == pc)
							{
								fallback_co = FALLBACK::ARENA;
								fallback_t = -1.f;
							}
							else
							{
								NetChangePlayer& c = Add1(p.first->player->player_info->changes);
								c.type = NetChangePlayer::ENTER_ARENA;
							}
						}

						p.second->busy = Unit::Busy_No;
						p.second->frozen = FROZEN::YES;
						if(p.second->IsPlayer())
						{
							p.second->player->arena_fights++;
							if(Net::IsOnline())
								p.second->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.second->player == pc)
							{
								fallback_co = FALLBACK::ARENA;
								fallback_t = -1.f;
							}
							else
							{
								NetChangePlayer& c = Add1(p.second->player->player_info->changes);
								c.type = NetChangePlayer::ENTER_ARENA;
							}
						}
					}
					tournament_pairs.pop_back();
				}
			}
			else if(tournament_state3 == 2)
			{
				// check if second unit is here
				if(tournament_master->CanAct())
				{
					if(tournament_other_fighter->to_remove || !tournament_other_fighter->IsStanding() || tournament_other_fighter->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(tournament_other_fighter->pos, tournament_master->pos) <= 64.f
							|| tournament_other_fighter->in_building == tournament_arena))
					{
						// second unit left too
						TournamentTalk(Format(txTour[13], tournament_other_fighter->GetRealName()));
						if(!tournament_other_fighter->to_remove && tournament_other_fighter->IsPlayer())
							AddGameMsg3(tournament_other_fighter->player, GMS_LEFT_EVENT);
					}
					else
					{
						// second unit is here, go automaticaly to next round
						tournament_units.push_back(tournament_other_fighter);
						TournamentTalk(Format(txTour[14], tournament_other_fighter->GetRealName()));
					}
					tournament_other_fighter = nullptr;
					tournament_state3 = 3;
				}
			}
			else if(tournament_state3 == 3)
			{
				// one or more units left
				// or talking after combat
				if(tournament_master->CanAct())
				{
					if(tournament_pairs.empty())
					{
						// no more fights in this round
						if(tournament_units.size() <= 1 && !tournament_skipped_unit)
						{
							// is there a winner?
							if(!tournament_units.empty())
								tournament_winner = tournament_units.back();
							else
								tournament_winner = tournament_skipped_unit;

							if(!tournament_winner)
							{
								// no winner
								TournamentTalk(txTour[15]);
								tournament_state3 = 2;
							}
							else
							{
								// there is winner
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
							// next round
							++tournament_round;
							StartTournamentRound();
							tournament_state2 = 0;
							tournament_state3 = 0;
						}
					}
					else
					{
						// next fight
						tournament_state3 = 0;
					}
				}
			}
			else if(tournament_state3 == 4)
			{
				// fight is going on, handled by arena code
			}
			else if(tournament_state3 == 5)
			{
				// after combat
				if(tournament_master->CanAct() && tournament_master->busy == Unit::Busy_No)
				{
					// give healing potions
					static const Item* p1 = Item::Get("p_hp");
					static const Item* p2 = Item::Get("p_hp2");
					static const Item* p3 = Item::Get("p_hp3");
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
									NetChangePlayer& c = Add1(u.player->player_info->changes);
									c.type = NetChangePlayer::ADDED_ITEMS_MSG;
									c.ile = given_items;
								}
							}
						}
					}

					// winner goes to next round
					Unit* winner = at_arena[arena_wynik];
					winner->busy = Unit::Busy_Tournament;
					tournament_units.push_back(winner);
					TournamentTalk(Format(txTour[Rand() % 2 == 0 ? 19 : 20], winner->GetRealName()));
					tournament_state3 = 3;
					at_arena.clear();
				}
			}
		}
		else if(tournament_state2 == 2)
		{
			if(tournament_master->CanAct())
			{
				if(tournament_state3 == 0)
				{
					// give reward to winner
					const int REWARD = 5000;

					tournament_state3 = 1;
					tournament_master->look_target = tournament_winner;
					tournament_winner->ModGold(REWARD);
					if(tournament_winner->IsHero())
					{
						tournament_winner->look_target = tournament_master;
						tournament_winner->hero->LevelUp();
					}
					else
						tournament_winner->busy = Unit::Busy_No;

					TournamentTalk(Format(txTour[21], tournament_winner->GetRealName()));
					tournament_master->busy = Unit::Busy_No;
					tournament_master->ai->idle_action = AIController::Idle_None;
				}
				else if(tournament_state3 == 1)
				{
					// end of tournament
					if(tournament_winner && tournament_winner->IsHero())
					{
						tournament_winner->look_target = nullptr;
						tournament_winner->ai->idle_action = AIController::Idle_None;
						tournament_winner->busy = Unit::Busy_No;
					}
					tournament_master->look_target = nullptr;
					CleanTournament();
				}
				else if(tournament_state3 == 2)
				{
					// no winner
					AddNews(txTour[17]);
					CleanTournament();
				}
			}
		}
	}
}

//=================================================================================================
void Game::VerifyTournamentUnit(Unit* unit)
{
	if(!unit || !unit->IsPlayer() || unit->to_remove)
		return;
	bool leaving_event = true;
	if(unit->in_building == tournament_arena)
		leaving_event = false;
	else if(unit->in_building == -1)
		leaving_event = Vec3::Distance2d(unit->pos, tournament_master->pos) > 16.f;

	if(leaving_event != unit->player->leaving_event)
	{
		unit->player->leaving_event = leaving_event;
		if(leaving_event)
			AddGameMsg3(GMS_GETTING_OUT_OF_RANGE);
	}
}

//=================================================================================================
void Game::StartTournamentRound()
{
	std::random_shuffle(tournament_units.begin(), tournament_units.end(), MyRand);
	tournament_pairs.clear();

	Unit* first = tournament_skipped_unit;
	for(auto& unit : tournament_units)
	{
		if(!first)
			first = unit;
		else
		{
			tournament_pairs.push_back(std::pair<SmartPtr<Unit>, SmartPtr<Unit>>(first, unit));
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
	Vec3 pos = GetArena()->exit_area.Midpoint().XZ(1.5f);
	game_gui->AddSpeechBubble(pos, text);
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK_POS;
		c.pos = pos;
		c.str = StringPool.Get();
		*c.str = text;
		net_talk.push_back(c.str);
	}
}

//=================================================================================================
void Game::TournamentTrain(Unit& u)
{
	tournament_master = nullptr;
	Train(u, false, (int)AttributeId::STR);
	Train(u, false, (int)AttributeId::END);
	Train(u, false, (int)AttributeId::DEX);
	if(u.HaveWeapon())
	{
		Train(u, true, (int)SkillId::ONE_HANDED_WEAPON);
		Train(u, true, (int)u.GetWeapon().GetInfo().skill);
	}
	if(u.HaveBow())
		Train(u, true, (int)SkillId::BOW);
	if(u.HaveShield())
		Train(u, true, (int)SkillId::SHIELD);
	if(u.HaveArmor())
		Train(u, true, (int)u.GetArmor().skill);
}

//=================================================================================================
void Game::CleanArena()
{
	InsideBuilding* arena = city_ctx->FindInsideBuilding(BuildingGroup::BG_ARENA);

	// wyrzuæ ludzi z areny
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.frozen = FROZEN::NO;
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

	for(auto& unit : tournament_units)
		unit->busy = Unit::Busy_No;
	tournament_units.clear();
	for(auto& p : tournament_pairs)
	{
		p.first->busy = Unit::Busy_No;
		p.second->busy = Unit::Busy_No;
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
