#include "Pch.h"
#include "Quest_Tournament.h"
#include "GameFile.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Mages.h"
#include "City.h"
#include "BuildingGroup.h"
#include "AIController.h"
#include "Team.h"
#include "Language.h"
#include "GameGui.h"
#include "LevelGui.h"
#include "Game.h"
#include "GameMessages.h"
#include "Arena.h"
#include "PlayerInfo.h"
#include "ScriptManager.h"

//=================================================================================================
void Quest_Tournament::InitOnce()
{
	quest_mgr->RegisterSpecialHandler(this, "ironfist_start");
	quest_mgr->RegisterSpecialHandler(this, "ironfist_join");
	quest_mgr->RegisterSpecialHandler(this, "ironfist_train");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_can_start");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_done");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_here");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_preparing");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_started");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_joined");
	quest_mgr->RegisterSpecialIfHandler(this, "ironfist_winner");
	quest_mgr->RegisterFormatString(this, "ironfist_city");
}

//=================================================================================================
void Quest_Tournament::LoadLanguage()
{
	LoadArray(txTour, "tour");
	LoadArray(txAiJoinTour, "aiJoinTour");
}

//=================================================================================================
void Quest_Tournament::Init()
{
	year = 0;
	city_year = world->GetYear();
	city = world->GetRandomCityIndex();
	state = TOURNAMENT_NOT_DONE;
	units.clear();
	winner = nullptr;
	generated = false;
}

//=================================================================================================
void Quest_Tournament::Clear()
{
	winner = nullptr;
}

//=================================================================================================
void Quest_Tournament::Save(GameWriter& f)
{
	f << year;
	f << city;
	f << city_year;
	f << winner;
	f << generated;
}

//=================================================================================================
void Quest_Tournament::Load(GameReader& f)
{
	f >> year;
	f >> city;
	f >> city_year;
	f >> winner;
	f >> generated;
	state = TOURNAMENT_NOT_DONE;
	units.clear();
	if(generated)
		master = game_level->local_area->FindUnit(UnitData::Get("arena_master"));
	else
		master = nullptr;
}

//=================================================================================================
bool Quest_Tournament::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "ironfist_start") == 0)
	{
		StartTournament(ctx.talker);
		units.push_back(ctx.pc->unit);
		ctx.pc->unit->ModGold(-100);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "ironfist_join") == 0)
	{
		units.push_back(ctx.pc->unit);
		ctx.pc->unit->ModGold(-100);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "ironfist_train") == 0)
	{
		winner = nullptr;
		ctx.pc->unit->frozen = FROZEN::YES;
		if(ctx.is_local)
		{
			// local fallback
			game->fallback_type = FALLBACK::TRAIN;
			game->fallback_t = -1.f;
			game->fallback_1 = 2;
			game->fallback_2 = 0;
		}
		else
		{
			// send info about training
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = 2;
			c.count = 0;
		}
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Tournament::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "ironfist_can_start") == 0)
	{
		return state == TOURNAMENT_NOT_DONE
			&& city == world->GetCurrentLocationIndex()
			&& world->GetDay() == 6
			&& world->GetMonth() == 2
			&& year != world->GetYear();
	}
	else if(strcmp(msg, "ironfist_done") == 0)
		return year == world->GetYear();
	else if(strcmp(msg, "ironfist_here") == 0)
		return city == world->GetCurrentLocationIndex();
	else if(strcmp(msg, "ironfist_preparing") == 0)
		return state == TOURNAMENT_STARTING;
	else if(strcmp(msg, "ironfist_started") == 0)
	{
		if(state == TOURNAMENT_IN_PROGRESS)
		{
			// winner can stop dialog and talk
			if(winner == ctx.pc->unit && state2 == 2 && state3 == 1)
			{
				master->look_target = nullptr;
				state = TOURNAMENT_NOT_DONE;
			}
			else
				return true;
		}
		return false;
	}
	else if(strcmp(msg, "ironfist_joined") == 0)
	{
		for(Unit* u : units)
		{
			if(ctx.pc->unit == u)
				return true;
		}
		return false;
	}
	else if(strcmp(msg, "ironfist_winner") == 0)
		return winner == ctx.pc->unit;
	assert(0);
	return false;
}

//=================================================================================================
cstring Quest_Tournament::FormatString(const string& str)
{
	if(str == "ironfist_city")
		return world->GetLocation(city)->name.c_str();
	return nullptr;
}

//=================================================================================================
void Quest_Tournament::Progress()
{
	int current_year = world->GetYear();
	int month = world->GetMonth();
	int day = world->GetDay();
	if(current_year != city_year)
	{
		city_year = current_year;
		city = world->GetRandomCityIndex(city);
		master = nullptr;
	}
	if(day == 6 && month == 2 && game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveArena) && world->GetCurrentLocationIndex() == city && !generated)
		GenerateUnits();
	if(month > 2 || (month == 2 && day > 6))
		year = current_year;
}

//=================================================================================================
UnitData& Quest_Tournament::GetRandomHeroData()
{
	static UnitGroup* group = UnitGroup::Get("tournament");
	return *group->GetRandomUnit();
}

//=================================================================================================
void Quest_Tournament::StartTournament(Unit* arena_master)
{
	year = world->GetYear();
	state = TOURNAMENT_STARTING;
	timer = 0.f;
	state2 = 0;
	master = arena_master;
	units.clear();
	winner = nullptr;
	pairs.clear();
	skipped_unit = nullptr;
	other_fighter = nullptr;
	arena = game_level->city_ctx->FindInsideBuilding(BuildingGroup::BG_ARENA);
}

//=================================================================================================
bool Quest_Tournament::ShouldJoin(Unit& u)
{
	if(u.summoner)
		return false;
	if(u.IsStanding() && u.IsHero() && u.frozen == FROZEN::NO)
	{
		if(IsSet(u.data->flags2, F2_TOURNAMENT))
			return true;
		else if(IsSet(u.data->flags3, F3_DRUNK_MAGE) && quest_mgr->quest_mages2->mages_state >= Quest_Mages2::State::MageCured)
		{
			if(!u.IsTeamMember())
			{
				// po wszystkim wr�� do karczmy
				u.OrderGoToInn();
			}
			return true;
		}
	}
	return false;
}

//=================================================================================================
void Quest_Tournament::GenerateUnits()
{
	LevelArea& area = *game_level->city_ctx;
	Vec3 pos = game_level->city_ctx->FindBuilding(BuildingGroup::BG_ARENA)->walk_pt;
	master = area.FindUnit(UnitData::Get("arena_master"));

	// warp heroes in front of arena
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(ShouldJoin(u) && !u.IsFollowingTeamMember())
		{
			u.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, true);
			game_level->WarpNearLocation(area, u, pos, 12.f, false);
		}
	}
	InsideBuilding* inn = game_level->city_ctx->FindInn();
	for(vector<Unit*>::iterator it = inn->units.begin(), end = inn->units.end(); it != end;)
	{
		Unit& u = **it;
		if(ShouldJoin(u) && !u.IsFollowingTeamMember())
		{
			u.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, true);
			u.area = &area;
			game_level->WarpNearLocation(area, u, pos, 12.f, false);
			area.units.push_back(&u);
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
		Unit* u = game_level->SpawnUnitNearLocation(area, pos, GetRandomHeroData(), nullptr, Random(5, 20), 12.f);
		if(u)
			u->temporary = true;
	}

	generated = true;
}

//=================================================================================================
void Quest_Tournament::Update(float dt)
{
	if(state == TOURNAMENT_NOT_DONE)
		return;

	if(game->arena->free && !master->IsAlive())
	{
		Clean();
		return;
	}

	// info about getting out of range
	for(auto& unit : units)
		VerifyUnit(unit);
	for(auto& pair : pairs)
	{
		VerifyUnit(pair.first);
		VerifyUnit(pair.second);
	}
	VerifyUnit(skipped_unit);
	VerifyUnit(other_fighter);

	if(state == TOURNAMENT_STARTING)
	{
		if(master->busy == Unit::Busy_No && master->IsStanding() && master->ai->state == AIController::Idle)
			timer += dt;

		// team members joining
		const Vec3& walk_pt = game_level->city_ctx->FindBuilding(BuildingGroup::BG_ARENA)->walk_pt;
		for(Unit& unit : team->members)
		{
			if(unit.busy == Unit::Busy_No && Vec3::Distance2d(unit.pos, master->pos) <= 16.f && !unit.dont_attack && ShouldJoin(unit))
			{
				unit.busy = Unit::Busy_Tournament;
				unit.ai->st.idle.action = AIController::Idle_Move;
				unit.ai->st.idle.pos = walk_pt;
				unit.ai->timer = Random(5.f, 10.f);

				unit.Talk(RandomString(txAiJoinTour));
			}
		}

		if(state2 == 0)
		{
			if(timer >= 5.f)
			{
				// tell to join
				state2 = 1;
				Talk(txTour[0]);
			}
		}
		else if(state2 == 1)
		{
			if(timer >= 30.f)
			{
				// tell to join 2
				state2 = 2;
				Talk(txTour[1]);
			}
		}
		else if(state2 == 2)
		{
			if(timer >= 60.f)
			{
				// gather npc's
				for(vector<Unit*>::iterator it = game_level->local_area->units.begin(), end = game_level->local_area->units.end(); it != end; ++it)
				{
					Unit& u = **it;
					if(Vec3::Distance2d(u.pos, master->pos) < 64.f && ShouldJoin(u))
					{
						units.push_back(&u);
						u.busy = Unit::Busy_Tournament;
					}
				}

				// start tournament
				state2 = 3;
				round = 0;
				master->busy = Unit::Busy_Yes;
				Talk(txTour[2]);
				skipped_unit = nullptr;
				StartRound();
			}
		}
		else if(state2 == 3)
		{
			if(master->CanAct())
			{
				if(!pairs.empty())
				{
					// tell how many units joined
					state2 = 4;
					Talk(Format(txTour[3], pairs.size() * 2 + (skipped_unit ? 1 : 0)));
					game->arena->SpawnArenaViewers(5);
				}
				else
				{
					// not enough units
					state2 = 5;
					Talk(txTour[22]);
				}
			}
		}
		else if(state2 == 4)
		{
			if(master->CanAct())
			{
				state = TOURNAMENT_IN_PROGRESS;
				state2 = 0;
				state3 = 0;
			}
		}
		else
		{
			if(master->CanAct())
			{
				// not enough units, end tournament
				Clean();
			}
		}
	}
	else
	{
		if(state2 == 0)
		{
			// introduce all fight in this round
			if(master->CanAct())
			{
				cstring text;
				if(state3 == 0)
					text = Format(txTour[4], round + 1);
				else if(pairs.size() == 1u && !skipped_unit)
				{
					auto& p = pairs[0];
					text = Format(txTour[23], p.first->GetRealName(), p.second->GetRealName());
				}
				else if(state3 <= (int)pairs.size())
				{
					auto& p = pairs[state3 - 1];
					text = Format(txTour[5], p.first->GetRealName(), p.second->GetRealName());
				}
				else if(state3 == (int)pairs.size() + 1)
				{
					if(skipped_unit)
						text = Format(txTour[6], skipped_unit->GetRealName());
					else
						text = txTour[round == 0 ? 7 : 8];
				}
				else
					text = txTour[round == 0 ? 7 : 8];

				Talk(text);

				++state3;
				bool end = false;
				if(state3 == (int)pairs.size() + 1)
				{
					if(!skipped_unit)
						end = true;
				}
				else if(state3 == (int)pairs.size() + 2)
					end = true;

				if(end)
				{
					state2 = 1;
					state3 = 0;
					std::reverse(pairs.begin(), pairs.end());
				}
			}
		}
		else if(state2 == 1)
		{
			if(state3 == 0)
			{
				// tell next fight participants (skip talking at final round)
				if(pairs.size() == 1u && units.empty() && !skipped_unit)
					state3 = 1;
				else if(master->CanAct())
				{
					auto& p = pairs.back();
					Talk(Format(txTour[9], p.first->GetRealName(), p.second->GetRealName()));
					state3 = 1;
				}
			}
			else if(state3 == 1)
			{
				// check if units are ready to fight
				if(master->CanAct())
				{
					auto& p = pairs.back();
					if(p.first->to_remove || !p.first->IsStanding() || p.first->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.first->pos, master->pos) <= 64.f || p.first->area == arena))
					{
						// first unit left or can't fight, check other
						state3 = 2;
						other_fighter = p.second;
						Talk(Format(txTour[11], p.first->GetRealName()));
						if(!p.first->to_remove && p.first->IsPlayer())
							game_gui->messages->AddGameMsg3(p.first->player, GMS_LEFT_EVENT);
					}
					else if(p.second->to_remove || !p.second->IsStanding() || p.second->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.second->pos, master->pos) <= 64.f || p.second->area == arena))
					{
						// second unit left or can't fight, first automaticaly goes to next round
						state3 = 3;
						units.push_back(p.first);
						Talk(Format(txTour[12], p.second->GetRealName(), p.first->GetRealName()));
						if(!p.second->to_remove && p.second->IsPlayer())
							game_gui->messages->AddGameMsg3(p.second->player, GMS_LEFT_EVENT);
					}
					else
					{
						// fight
						master->busy = Unit::Busy_No;
						state3 = 4;
						game->arena->Start(Arena::TOURNAMENT);
						game->arena->units.push_back(p.first);
						game->arena->units.push_back(p.second);

						p.first->busy = Unit::Busy_No;
						p.first->frozen = FROZEN::YES;
						if(p.first->IsPlayer())
						{
							p.first->player->arena_fights++;
							p.first->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.first->player == game->pc)
							{
								game->fallback_type = FALLBACK::ARENA;
								game->fallback_t = -1.f;
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
							p.second->player->stat_flags |= STAT_ARENA_FIGHTS;
							if(p.second->player == game->pc)
							{
								game->fallback_type = FALLBACK::ARENA;
								game->fallback_t = -1.f;
							}
							else
							{
								NetChangePlayer& c = Add1(p.second->player->player_info->changes);
								c.type = NetChangePlayer::ENTER_ARENA;
							}
						}
					}
					pairs.pop_back();
				}
			}
			else if(state3 == 2)
			{
				// check if second unit is here
				if(master->CanAct())
				{
					if(other_fighter->to_remove || !other_fighter->IsStanding() || other_fighter->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(other_fighter->pos, master->pos) <= 64.f || other_fighter->area == arena))
					{
						// second unit left too
						Talk(Format(txTour[13], other_fighter->GetRealName()));
						if(!other_fighter->to_remove && other_fighter->IsPlayer())
							game_gui->messages->AddGameMsg3(other_fighter->player, GMS_LEFT_EVENT);
					}
					else
					{
						// second unit is here, go automaticaly to next round
						units.push_back(other_fighter);
						Talk(Format(txTour[14], other_fighter->GetRealName()));
					}
					other_fighter = nullptr;
					state3 = 3;
				}
			}
			else if(state3 == 3)
			{
				// one or more units left
				// or talking after combat
				if(master->CanAct())
				{
					if(pairs.empty())
					{
						// no more fights in this round
						if(units.size() <= 1 && !skipped_unit)
						{
							// is there a winner?
							if(!units.empty())
								winner = units.back();
							else
								winner = skipped_unit;

							if(!winner)
							{
								// no winner
								Talk(txTour[15]);
								state3 = 2;
							}
							else
							{
								// there is winner
								Talk(Format(txTour[16], winner->GetRealName()));
								state3 = 0;
								world->AddNews(Format(txTour[18], winner->GetRealName()));
							}

							units.clear();
							state2 = 2;

							game->arena->RemoveArenaViewers();
						}
						else
						{
							// next round
							++round;
							StartRound();
							state2 = 0;
							state3 = 0;
						}
					}
					else
					{
						// next fight
						state3 = 0;
					}
				}
			}
			else if(state3 == 4)
			{
				// fight is going on, handled by arena code
			}
			else if(state3 == 5)
			{
				// after combat
				if(master->CanAct() && master->busy == Unit::Busy_No)
				{
					// give healing potions
					static const Item* p1 = Item::Get("p_hp");
					static const Item* p2 = Item::Get("p_hp2");
					static const Item* p3 = Item::Get("p_hp3");
					static const float p1_power = p1->GetEffectPower(EffectId::Heal);
					static const float p2_power = p2->GetEffectPower(EffectId::Heal);
					static const float p3_power = p3->GetEffectPower(EffectId::Heal);
					for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
					{
						Unit& u = **it;
						float mhp = u.hpmax - u.hp;
						uint given_items = 0;
						if(mhp > 0.f && u.IsAI())
							u.ai->have_potion = HavePotion::Yes;
						if(mhp >= p3_power)
						{
							int count = (int)floor(mhp / p3_power);
							u.AddItem2(p3, count, 0, false);
							mhp -= p3_power * count;
							given_items += count;
						}
						if(mhp >= p2_power)
						{
							int count = (int)floor(mhp / p2_power);
							u.AddItem2(p2, count, 0, false);
							mhp -= p2_power * count;
							given_items += count;
						}
						if(mhp > 0.f)
						{
							int count = (int)ceil(mhp / p1_power);
							u.AddItem2(p1, count, 0, false);
							mhp -= p1_power * count;
							given_items += count;
						}
						if(u.IsPlayer() && given_items)
							game_gui->messages->AddItemMessage(u.player, given_items);
					}

					// winner goes to next round
					Unit* round_winner = game->arena->units[game->arena->result];
					round_winner->busy = Unit::Busy_Tournament;
					units.push_back(round_winner);
					Talk(Format(txTour[Rand() % 2 == 0 ? 19 : 20], round_winner->GetRealName()));
					state3 = 3;
					game->arena->units.clear();
				}
			}
		}
		else if(state2 == 2)
		{
			if(master->CanAct())
			{
				if(state3 == 0)
				{
					// give reward to winner
					const int REWARD = 5000;

					state3 = 1;
					master->look_target = winner;
					winner->ModGold(REWARD);
					if(winner->IsHero())
					{
						winner->look_target = master;
						winner->hero->AddExp(15000);
					}
					else
						winner->busy = Unit::Busy_No;

					Talk(Format(txTour[21], winner->GetRealName()));
					master->busy = Unit::Busy_No;
					master->ai->st.idle.action = AIController::Idle_None;
				}
				else if(state3 == 1)
				{
					// end of tournament
					if(winner && winner->IsHero())
					{
						winner->look_target = nullptr;
						winner->ai->st.idle.action = AIController::Idle_None;
						winner->busy = Unit::Busy_No;
					}
					master->look_target = nullptr;
					Clean();
				}
				else if(state3 == 2)
				{
					// no winner
					world->AddNews(txTour[17]);
					Clean();
				}
			}
		}
	}
}

//=================================================================================================
void Quest_Tournament::VerifyUnit(Unit* unit)
{
	if(!unit || !unit->IsPlayer() || unit->to_remove)
		return;
	bool leaving_event = true;
	if(unit->area == arena)
		leaving_event = false;
	else if(unit->area->area_type == LevelArea::Type::Outside)
		leaving_event = Vec3::Distance2d(unit->pos, master->pos) > 16.f;

	if(leaving_event != unit->player->leaving_event)
	{
		unit->player->leaving_event = leaving_event;
		if(leaving_event)
			game_gui->messages->AddGameMsg3(GMS_GETTING_OUT_OF_RANGE);
	}
}

//=================================================================================================
void Quest_Tournament::StartRound()
{
	Shuffle(units.begin(), units.end());
	pairs.clear();

	Unit* first = skipped_unit;
	for(auto& unit : units)
	{
		if(!first)
			first = unit;
		else
		{
			pairs.push_back(pair<SmartPtr<Unit>, SmartPtr<Unit>>(first, unit));
			first = nullptr;
		}
	}

	skipped_unit = first;
	units.clear();
}

//=================================================================================================
void Quest_Tournament::Talk(cstring text)
{
	master->Talk(text);
	Vec3 pos = game_level->GetArena()->exit_region.Midpoint().XZ(1.5f);
	game_gui->level_gui->AddSpeechBubble(pos, text);
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK_POS;
		c.pos = pos;
		c.str = StringPool.Get();
		*c.str = text;
		net->net_strs.push_back(c.str);
	}
}

//=================================================================================================
void Quest_Tournament::Train(PlayerController& player)
{
	Unit& u = *player.unit;
	master = nullptr;
	player.Train(false, (int)AttributeId::STR);
	player.Train(false, (int)AttributeId::END);
	player.Train(false, (int)AttributeId::DEX);
	if(u.HaveWeapon())
	{
		player.Train(true, (int)SkillId::ONE_HANDED_WEAPON);
		player.Train(true, (int)u.GetWeapon().GetSkill());
	}
	if(u.HaveBow())
		player.Train(true, (int)SkillId::BOW);
	if(u.HaveShield())
		player.Train(true, (int)SkillId::SHIELD);
	if(u.HaveArmor())
		player.Train(true, (int)u.GetArmor().GetSkill());
	Var* var = script_mgr->GetVars(&u)->Get("ironfist_won");
	if(!var->IsBool(true))
	{
		u.player->AddLearningPoint();
		var->SetBool(true);
	}
}

//=================================================================================================
void Quest_Tournament::Clean()
{
	if(!game->arena->free)
		game->arena->Clean();

	for(auto& unit : units)
		unit->busy = Unit::Busy_No;
	units.clear();
	for(auto& p : pairs)
	{
		p.first->busy = Unit::Busy_No;
		p.second->busy = Unit::Busy_No;
	}
	pairs.clear();
	if(skipped_unit)
	{
		skipped_unit->busy = Unit::Busy_No;
		skipped_unit = nullptr;
	}
	master->busy = Unit::Busy_No;
	master = nullptr;

	state = TOURNAMENT_NOT_DONE;
	generated = false;
}
