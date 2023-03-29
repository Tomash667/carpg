#include "Pch.h"
#include "Quest_Tournament.h"

#include "AIController.h"
#include "Arena.h"
#include "BuildingGroup.h"
#include "City.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "Language.h"
#include "Level.h"
#include "LevelGui.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Mages.h"
#include "ScriptManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Tournament::InitOnce()
{
	questMgr->RegisterSpecialHandler(this, "ironfist_start");
	questMgr->RegisterSpecialHandler(this, "ironfist_join");
	questMgr->RegisterSpecialHandler(this, "ironfist_train");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_can_start");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_done");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_here");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_preparing");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_started");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_joined");
	questMgr->RegisterSpecialIfHandler(this, "ironfist_winner");
	questMgr->RegisterFormatString(this, "ironfist_city");
}

//=================================================================================================
void Quest_Tournament::LoadLanguage()
{
	StrArray(txTour, "tour");
	StrArray(txAiJoinTour, "aiJoinTour");
}

//=================================================================================================
void Quest_Tournament::Init()
{
	year = 0;
	cityYear = world->GetDateValue().year;
	city = world->GetRandomCity()->index;
	state = TOURNAMENT_NOT_DONE;
	units.clear();
	winner = nullptr;
	generated = false;

	if(game->devmode)
		Info("Tournament - %s.", world->GetLocation(GetCity())->name.c_str());
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
	f << cityYear;
	f << winner;
	f << generated;
}

//=================================================================================================
void Quest_Tournament::Load(GameReader& f)
{
	f >> year;
	f >> city;
	f >> cityYear;
	f >> winner;
	f >> generated;
	state = TOURNAMENT_NOT_DONE;
	units.clear();
	if(generated)
		master = gameLevel->localPart->FindUnit(UnitData::Get("arenaMaster"));
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
		ctx.pc->leavingEvent = false;
	}
	else if(strcmp(msg, "ironfist_join") == 0)
	{
		units.push_back(ctx.pc->unit);
		ctx.pc->unit->ModGold(-100);
		ctx.pc->leavingEvent = false;
	}
	else if(strcmp(msg, "ironfist_train") == 0)
	{
		winner = nullptr;
		ctx.pc->unit->frozen = FROZEN::YES;
		if(ctx.isLocal)
		{
			// local fallback
			game->fallbackType = FALLBACK::TRAIN;
			game->fallbackTimer = -1.f;
			game->fallbackValue = 2;
			game->fallbackValue2 = 0;
		}
		else
		{
			// send info about training
			NetChangePlayer& c = ctx.pc->playerInfo->PushChange(NetChangePlayer::TRAIN);
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
		const Date& date = world->GetDateValue();
		return state == TOURNAMENT_NOT_DONE
			&& city == world->GetCurrentLocationIndex()
			&& date.day == 6
			&& date.month == 2
			&& date.year != year;
	}
	else if(strcmp(msg, "ironfist_done") == 0)
		return year == world->GetDateValue().year;
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
				master->lookTarget = nullptr;
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
void Quest_Tournament::OnProgress()
{
	const Date& date = world->GetDateValue();
	if(date.year != cityYear)
	{
		cityYear = date.year;
		city = world->GetRandomCity(world->GetLocation(city))->index;
		master = nullptr;
	}
	if(date.day == 6 && date.month == 2 && gameLevel->cityCtx && IsSet(gameLevel->cityCtx->flags, City::HaveArena) && world->GetCurrentLocationIndex() == city && !generated)
		GenerateUnits();
	if(date.month > 2 || (date.month == 2 && date.day > 6))
		year = date.year;
}

//=================================================================================================
UnitData& Quest_Tournament::GetRandomHeroData()
{
	static UnitGroup* group = UnitGroup::Get("tournament");
	return *group->GetRandomUnit();
}

//=================================================================================================
void Quest_Tournament::StartTournament(Unit* arenaMaster)
{
	year = world->GetDateValue().year;
	state = TOURNAMENT_STARTING;
	timer = 0.f;
	state2 = 0;
	master = arenaMaster;
	units.clear();
	winner = nullptr;
	pairs.clear();
	skippedCount = nullptr;
	otherFighter = nullptr;
	arena = gameLevel->cityCtx->FindInsideBuilding(BuildingGroup::BG_ARENA);
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
		else if(IsSet(u.data->flags3, F3_DRUNK_MAGE) && questMgr->questMages2->magesState >= Quest_Mages2::State::MageCured)
		{
			// go back to inn
			if(!u.IsTeamMember())
				u.OrderGoToInn();
			return true;
		}
	}
	return false;
}

//=================================================================================================
void Quest_Tournament::GenerateUnits()
{
	LocationPart& locPart = *gameLevel->cityCtx;
	Vec3 pos = gameLevel->cityCtx->FindBuilding(BuildingGroup::BG_ARENA)->walkPt;
	master = locPart.FindUnit(UnitData::Get("arenaMaster"));

	// warp heroes in front of arena
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(ShouldJoin(u) && !u.IsFollowingTeamMember())
		{
			u.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, true);
			gameLevel->WarpNearLocation(locPart, u, pos, 12.f, false);
		}
	}
	InsideBuilding* inn = gameLevel->cityCtx->FindInn();
	for(vector<Unit*>::iterator it = inn->units.begin(), end = inn->units.end(); it != end;)
	{
		Unit& u = **it;
		if(ShouldJoin(u) && !u.IsFollowingTeamMember())
		{
			u.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, true);
			u.locPart = &locPart;
			gameLevel->WarpNearLocation(locPart, u, pos, 12.f, false);
			locPart.units.push_back(&u);
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
		Unit* u = gameLevel->SpawnUnitNearLocation(locPart, pos, GetRandomHeroData(), nullptr, Random(5, 20), 12.f);
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
	VerifyUnit(skippedCount);
	VerifyUnit(otherFighter);

	if(state == TOURNAMENT_STARTING)
	{
		if(master->busy == Unit::Busy_No && master->IsStanding() && master->ai->state == AIController::Idle)
			timer += dt;

		// team members joining
		const Vec3& walkPt = gameLevel->cityCtx->FindBuilding(BuildingGroup::BG_ARENA)->walkPt;
		for(Unit& unit : team->members)
		{
			if(unit.busy == Unit::Busy_No && Vec3::Distance2d(unit.pos, master->pos) <= 16.f && !unit.dontAttack && ShouldJoin(unit))
			{
				unit.busy = Unit::Busy_Tournament;
				unit.ai->st.idle.action = AIController::Idle_Move;
				unit.ai->st.idle.pos = walkPt;
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
				for(vector<Unit*>::iterator it = gameLevel->localPart->units.begin(), end = gameLevel->localPart->units.end(); it != end; ++it)
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
				skippedCount = nullptr;
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
					Talk(Format(txTour[3], pairs.size() * 2 + (skippedCount ? 1 : 0)));
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
			// introduce all fights in this round
			if(master->CanAct())
			{
				cstring text;
				if(state3 == 0)
					text = Format(txTour[4], round + 1);
				else if(pairs.size() == 1u && !skippedCount)
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
					if(skippedCount)
						text = Format(txTour[6], skippedCount->GetRealName());
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
					if(!skippedCount)
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
				if(pairs.size() == 1u && units.empty() && !skippedCount)
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
					if(p.first->toRemove || !p.first->IsStanding() || p.first->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.first->pos, master->pos) <= 64.f || p.first->locPart == arena))
					{
						// first unit left or can't fight, check other
						state3 = 2;
						otherFighter = p.second;
						Talk(Format(txTour[11], p.first->GetRealName()));
						if(!p.first->toRemove && p.first->IsPlayer())
							gameGui->messages->AddGameMsg3(p.first->player, GMS_LEFT_EVENT);
					}
					else if(p.second->toRemove || !p.second->IsStanding() || p.second->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(p.second->pos, master->pos) <= 64.f || p.second->locPart == arena))
					{
						// second unit left or can't fight, first automaticaly goes to next round
						state3 = 3;
						units.push_back(p.first);
						Talk(Format(txTour[12], p.second->GetRealName(), p.first->GetRealName()));
						if(!p.second->toRemove && p.second->IsPlayer())
							gameGui->messages->AddGameMsg3(p.second->player, GMS_LEFT_EVENT);
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
							p.first->player->arenaFights++;
							p.first->player->statFlags |= STAT_ARENA_FIGHTS;
							if(p.first->player == game->pc)
							{
								game->fallbackType = FALLBACK::ARENA;
								game->fallbackTimer = -1.f;
							}
							else
								p.first->player->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);
						}

						p.second->busy = Unit::Busy_No;
						p.second->frozen = FROZEN::YES;
						if(p.second->IsPlayer())
						{
							p.second->player->arenaFights++;
							p.second->player->statFlags |= STAT_ARENA_FIGHTS;
							if(p.second->player == game->pc)
							{
								game->fallbackType = FALLBACK::ARENA;
								game->fallbackTimer = -1.f;
							}
							else
								p.second->player->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);
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
					if(otherFighter->toRemove || !otherFighter->IsStanding() || otherFighter->frozen != FROZEN::NO
						|| !(Vec3::Distance2d(otherFighter->pos, master->pos) <= 64.f || otherFighter->locPart == arena))
					{
						// second unit left too
						Talk(Format(txTour[13], otherFighter->GetRealName()));
						if(!otherFighter->toRemove && otherFighter->IsPlayer())
							gameGui->messages->AddGameMsg3(otherFighter->player, GMS_LEFT_EVENT);
					}
					else
					{
						// second unit is here, go automaticaly to next round
						units.push_back(otherFighter);
						Talk(Format(txTour[14], otherFighter->GetRealName()));
					}
					otherFighter = nullptr;
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
						if(units.size() <= 1 && !skippedCount)
						{
							// is there a winner?
							if(!units.empty())
								winner = units.back();
							else
								winner = skippedCount;

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
					static const Consumable* p1 = (Consumable*)Item::potionHealth;
					static const Consumable* p2 = (Consumable*)Item::potionHealth2;
					static const Consumable* p3 = (Consumable*)Item::potionHealth3;
					static const float p1Power = p1->GetEffectPower(EffectId::Heal);
					static const float p2Power = p2->GetEffectPower(EffectId::Heal);
					static const float p3Power = p3->GetEffectPower(EffectId::Heal);
					for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
					{
						Unit& u = **it;
						float mhp = u.hpmax - u.hp;
						uint givenItems = 0;
						if(mhp > 0.f && u.IsAI())
							u.ai->havePotion = HavePotion::Yes;
						if(mhp >= p3Power)
						{
							int count = (int)floor(mhp / p3Power);
							u.AddItem2(p3, count, 0, false);
							mhp -= p3Power * count;
							givenItems += count;
						}
						if(mhp >= p2Power)
						{
							int count = (int)floor(mhp / p2Power);
							u.AddItem2(p2, count, 0, false);
							mhp -= p2Power * count;
							givenItems += count;
						}
						if(mhp > 0.f)
						{
							int count = (int)ceil(mhp / p1Power);
							u.AddItem2(p1, count, 0, false);
							mhp -= p1Power * count;
							givenItems += count;
						}
						if(u.IsPlayer() && givenItems)
							gameGui->messages->AddItemMessage(u.player, givenItems);
					}

					// winner goes to next round
					Unit* roundWinner = game->arena->units[game->arena->result];
					roundWinner->busy = Unit::Busy_Tournament;
					units.push_back(roundWinner);
					Talk(Format(txTour[Rand() % 2 == 0 ? 19 : 20], roundWinner->GetRealName()));
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
					master->lookTarget = winner;
					winner->ModGold(REWARD);
					if(winner->IsHero())
					{
						winner->lookTarget = master;
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
						winner->lookTarget = nullptr;
						winner->ai->st.idle.action = AIController::Idle_None;
						winner->busy = Unit::Busy_No;
					}
					master->lookTarget = nullptr;
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
	if(!unit || !unit->IsPlayer() || unit->toRemove)
		return;
	bool leavingEvent = true;
	if(unit->locPart == arena)
		leavingEvent = false;
	else if(unit->locPart->partType == LocationPart::Type::Outside)
		leavingEvent = Vec3::Distance2d(unit->pos, master->pos) > 16.f;

	if(leavingEvent != unit->player->leavingEvent)
	{
		unit->player->leavingEvent = leavingEvent;
		if(leavingEvent)
			gameGui->messages->AddGameMsg3(GMS_GETTING_OUT_OF_RANGE);
	}
}

//=================================================================================================
void Quest_Tournament::StartRound()
{
	Shuffle(units.begin(), units.end());
	pairs.clear();

	Unit* first = skippedCount;
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

	skippedCount = first;
	units.clear();
}

//=================================================================================================
void Quest_Tournament::Talk(cstring text)
{
	master->Talk(text);
	Vec3 pos = gameLevel->GetArena()->exitRegion.Midpoint().XZ(1.5f);
	gameGui->levelGui->AddSpeechBubble(pos, text);
	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::TALK_POS);
		c.pos = pos;
		c.str = StringPool.Get();
		*c.str = text;
		net->netStrs.push_back(c.str);
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
	Var* var = scriptMgr->GetVars(&u)->Get("ironfist_won");
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
	if(skippedCount)
	{
		skippedCount->busy = Unit::Busy_No;
		skippedCount = nullptr;
	}
	master->busy = Unit::Busy_No;
	master = nullptr;

	state = TOURNAMENT_NOT_DONE;
	generated = false;
}
