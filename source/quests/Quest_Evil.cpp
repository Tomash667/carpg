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
#include "LevelPart.h"
#include "LocationGeneratorFactory.h"
#include "LocationHelper.h"
#include "ParticleEffect.h"
#include "Pathfinding.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <SoundManager.h>

//=================================================================================================
void Quest_Evil::Init()
{
	questMgr->RegisterSpecialIfHandler(this, "q_zlo_kapitan");
	questMgr->RegisterSpecialIfHandler(this, "q_zlo_burmistrz");
}

//=================================================================================================
void Quest_Evil::Start()
{
	category = QuestCategory::Unique;
	type = Q_EVIL;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities());
	mageLoc = -1;
	closed = 0;
	changed = false;
	for(int i = 0; i < 3; ++i)
	{
		loc[i].state = Loc::State::None;
		loc[i].targetLoc = nullptr;
	}
	toldAboutBoss = false;
	evilState = State::None;
	cleric = nullptr;
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[8], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Evil' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Evil::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;

	if(id == "qEvilCleric")
		return GameDialog::TryGet("qEvilCleric");
	else if(id == "qEvilMage")
		return GameDialog::TryGet("qEvilMage");
	else if(id == "qEvilBoss")
		return GameDialog::TryGet("qEvilBoss");
	else if(id == "guardCaptain")
		return GameDialog::TryGet("qEvilCaptain");
	else if(id == "mayor" || id == "soltys")
		return GameDialog::TryGet("qEvilMayor");
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
			if(!questMgr->RemoveQuestRumor(id))
				gameGui->journal->AddRumor(Format(questMgr->txQuest[232], GetStartLocationName()));
		}
		break;
	case Progress::Talked:
		{
			OnStart(questMgr->txQuest[233]);
			questMgr->RemoveQuestRumor(id);
			// add location
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 128.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, OLD_TEMPLE, 1);
			target.group = UnitGroup::empty;
			target.SetKnown();
			target.st = 8;
			target.activeQuest = this;
			target.dontClean = true;
			targetLoc = &target;
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
			atLevel = 0;
			// add journal entries
			msgs.push_back(Format(questMgr->txQuest[234], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::AltarEvent:
		{
			OnUpdate(questMgr->txQuest[236]);
			world->AddNews(Format(questMgr->txQuest[237], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedAboutBook:
		{
			Location* mage = world->GetRandomSettlement(startLoc);
			mageLoc = mage->index;
			OnUpdate(Format(questMgr->txQuest[238], mage->name.c_str(), GetLocationDirName(startLoc->pos, mage->pos)));
			evilState = State::GenerateMage;
			team->AddExp(7500);
		}
		break;
	case Progress::MageToldAboutStolenBook:
		{
			OnUpdate(questMgr->txQuest[239]);
			world->AddNews(Format(questMgr->txQuest[240], world->GetLocation(mageLoc)->name.c_str()));
			DialogContext::current->talker->temporary = true;
		}
		break;
	case Progress::TalkedWithCaptain:
		{
			OnUpdate(Format(questMgr->txQuest[241], LocationHelper::IsCity(world->GetCurrentLocation()) ? questMgr->txForMayor : questMgr->txForSoltys));
		}
		break;
	case Progress::TalkedWithMayor:
		{
			OnUpdate(Format(questMgr->txQuest[242], LocationHelper::IsCity(world->GetCurrentLocation()) ? questMgr->txForMayor : questMgr->txForSoltys));
		}
		break;
	case Progress::GotBook:
		{
			OnUpdate(questMgr->txQuest[243]);
			const Item* item = Item::Get("qEvilBook");
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
			} locInfo[3] = {
				L_DUNGEON, OLD_TEMPLE, "evil", 16,
				L_DUNGEON, NECROMANCER_BASE, "necromancers", 15,
				L_DUNGEON, MONSTER_CRYPT, "undead", 13
			};

			cstring newMsgs[4];
			newMsgs[0] = questMgr->txQuest[245];

			for(int i = 0; i < 3; ++i)
			{
				Int2 levels = gBaseLocations[locInfo[i].target].levels;
				Location& target = *world->CreateLocation(locInfo[i].type, world->GetRandomPlace(), locInfo[i].target,
					Random(max(levels.x, 2), max(levels.y, 2)));
				target.group = UnitGroup::Get(locInfo[i].group);
				target.st = locInfo[i].st;
				target.SetKnown();
				target.activeQuest = this;
				loc[i].targetLoc = &target;
				loc[i].nearLoc = world->GetNearestSettlement(target.pos)->index;
				loc[i].atLevel = target.GetLastLevel();
				loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
				newMsgs[i + 1] = Format(questMgr->txQuest[247], target.name.c_str(),
					GetLocationDirName(world->GetLocation(loc[i].nearLoc)->pos, target.pos),
					world->GetLocation(loc[i].nearLoc)->name.c_str());
				world->AddNews(Format(questMgr->txQuest[246], target.name.c_str()));
			}

			OnUpdate({ newMsgs[0], newMsgs[1], newMsgs[2], newMsgs[3] });

			nextEvent = &loc[0];
			loc[0].nextEvent = &loc[1];
			loc[1].nextEvent = &loc[2];

			// add cleric to team
			Unit& u = *DialogContext::current->talker;
			const Item* item = Item::Get("qEvilBook");
			u.AddItem(item, 1, true);
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			team->AddMember(&u, HeroType::Free);

			evilState = State::ClosingPortals;
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
			unitToSpawn = UnitData::Get("qEvilBoss");
			unitSpawnRoom = RoomTarget::Treasury;
			unitEventHandler = this;
			unitDontAttack = true;
			unitAutoTalk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
			atLevel = 0;
			targetLoc->st = 15;
			targetLoc->group = UnitGroup::Get("evil");
			targetLoc->reset = true;
			evilState = State::KillBoss;
			OnUpdate(Format(questMgr->txQuest[248], GetTargetLocationName()));
			for(int i = 0; i < 3; ++i)
				loc[i].targetLoc->activeQuest = nullptr;
		}
		break;
	case Progress::KilledBoss:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[249]);
			// restore old altar
			targetLoc->activeQuest = nullptr;
			targetLoc->dontClean = false;
			BaseObject* baseObj = BaseObject::Get("bloodyAltar");
			Object* obj = gameLevel->localPart->FindObject(baseObj);
			obj->base = BaseObject::Get("altar");
			obj->mesh = obj->base->mesh;
			resMgr->Load(obj->mesh);
			// remove particles
			float bestDist = 999.f;
			ParticleEmitter* bestPe = nullptr;
			for(ParticleEmitter* pe : gameLevel->localPart->lvlPart->pes)
			{
				if(pe->tex == gameRes->tBlood[BLOOD_RED])
				{
					float dist = Vec3::Distance(pe->pos, obj->pos);
					if(dist < bestDist)
					{
						bestDist = dist;
						bestPe = pe;
					}
				}
			}
			assert(bestPe);
			bestPe->destroy = true;
			// talking
			Unit* unit = team->FindTeamMember("qEvilCleric");
			if(unit)
				unit->OrderAutoTalk();

			questMgr->EndUniqueQuest();
			team->AddExp(30000);
			evilState = State::ClericWantTalk;
			world->AddNews(questMgr->txQuest[250]);
			team->AddLearningPoint();

			if(Net::IsOnline())
				Net::PushChange(NetChange::CLEAN_ALTAR);
		}
		break;
	case Progress::Finished:
		{
			evilState = State::ClericLeaving;
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
		return world->GetLocation(mageLoc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(startLoc->pos, world->GetLocation(mageLoc)->pos);
	else if(str == "t1")
		return loc[0].targetLoc->name.c_str();
	else if(str == "t2")
		return loc[1].targetLoc->name.c_str();
	else if(str == "t3")
		return loc[2].targetLoc->name.c_str();
	else if(str == "t1n")
		return world->GetLocation(loc[0].nearLoc)->name.c_str();
	else if(str == "t2n")
		return world->GetLocation(loc[1].nearLoc)->name.c_str();
	else if(str == "t3n")
		return world->GetLocation(loc[2].nearLoc)->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(world->GetLocation(loc[0].nearLoc)->pos, loc[0].targetLoc->pos);
	else if(str == "t2d")
		return GetLocationDirName(world->GetLocation(loc[1].nearLoc)->pos, loc[1].targetLoc->pos);
	else if(str == "t3d")
		return GetLocationDirName(world->GetLocation(loc[2].nearLoc)->pos, loc[2].targetLoc->pos);
	else if(str == "close_dir")
	{
		float bestDist = 999.f;
		int bestIndex = -1;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), loc[i].targetLoc->pos);
				if(dist < bestDist)
				{
					bestDist = dist;
					bestIndex = i;
				}
			}
		}
		Loc& l = loc[bestIndex];
		return GetLocationDirName(world->GetWorldPos(), l.targetLoc->pos);
	}
	else if(str == "close_loc")
	{
		float bestDist = 999.f;
		Location* bestLoc = nullptr;
		for(int i = 0; i < 3; ++i)
		{
			if(loc[i].state != Loc::PortalClosed)
			{
				float dist = Vec2::Distance(world->GetWorldPos(), loc[i].targetLoc->pos);
				if(dist < bestDist)
				{
					bestDist = dist;
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
		return evilState == State::ClosingPortals && closed == 1;
	else if(strcmp(msg, "q_zlo_clear2") == 0)
		return evilState == State::ClosingPortals && closed == 2;
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
		return world->GetCurrentLocationIndex() == mageLoc
			&& evilState >= State::GeneratedMage
			&& evilState < State::ClosingPortals
			&& InRange((Progress)prog, Progress::MageToldAboutStolenBook, Progress::TalkedWithMayor);
	}
	else if(strcmp(msg, "q_zlo_burmistrz") == 0)
	{
		return world->GetCurrentLocationIndex() == mageLoc
			&& evilState >= State::GeneratedMage
			&& evilState < State::ClosingPortals
			&& prog == Progress::TalkedWithCaptain;
	}
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Evil::HandleUnitEvent(UnitEventHandler::TYPE eventType, Unit* unit)
{
	assert(unit);
	if(eventType == UnitEventHandler::DIE && prog == Progress::AllPortalsClosed)
	{
		SetProgress(Progress::KilledBoss);
		unit->eventHandler = nullptr;
	}
}

//=================================================================================================
void Quest_Evil::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << mageLoc;
	for(int i = 0; i < 3; ++i)
	{
		f << loc[i].targetLoc;
		f << loc[i].done;
		f << loc[i].atLevel;
		f << loc[i].nearLoc;
		f << loc[i].state;
		f << loc[i].pos;
	}
	f << closed;
	f << changed;
	f << toldAboutBoss;
	f << evilState;
	f << pos;
	f << timer;
	f << cleric;
}

//=================================================================================================
Quest::LoadResult Quest_Evil::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> mageLoc;
	for(int i = 0; i < 3; ++i)
	{
		f >> loc[i].targetLoc;
		f >> loc[i].done;
		f >> loc[i].atLevel;
		f >> loc[i].nearLoc;
		f >> loc[i].state;
		f >> loc[i].pos;
		loc[i].callback = VoidDelegate(this, &Quest_Evil::GeneratePortal);
	}
	f >> closed;
	f >> changed;
	f >> toldAboutBoss;
	f >> evilState;
	f >> pos;
	f >> timer;
	f >> cleric;

	nextEvent = &loc[0];
	loc[0].nextEvent = &loc[1];
	loc[1].nextEvent = &loc[2];

	if(!done)
	{
		if(prog == Progress::Talked)
			callback = VoidDelegate(this, &Quest_Evil::GenerateBloodyAltar);
		else if(prog == Progress::AllPortalsClosed)
		{
			unitToSpawn = UnitData::Get("qEvilBoss");
			unitSpawnRoom = RoomTarget::Treasury;
			unitDontAttack = true;
			unitAutoTalk = true;
			callback = VoidDelegate(this, &Quest_Evil::WarpEvilBossToAltar);
		}
	}

	unitEventHandler = this;

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Evil::GenerateBloodyAltar()
{
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();

	// find altar need center
	Vec3 center(float(lvl.w + 1), 0, float(lvl.h + 1));
	float bestDist = 999.f;
	BaseObject* baseObj = BaseObject::Get("altar");
	Object* obj = nullptr;
	for(auto o : lvl.objects)
	{
		if(o->base == baseObj)
		{
			float dist = Vec3::Distance(o->pos, center);
			if(dist < bestDist)
			{
				bestDist = dist;
				obj = o;
			}
		}
	}
	assert(obj);

	// change object type
	obj->base = BaseObject::Get("bloodyAltar");
	obj->mesh = obj->base->mesh;

	// add particles
	if(obj->base->effect)
	{
		ParticleEmitter* pe = new ParticleEmitter;
		obj->base->effect->Apply(pe);
		pe->pos = obj->pos;
		pe->pos.y += obj->base->centery;
		pe->Init();
		lvl.lvlPart->pes.push_back(pe);
	}

	// add blood
	vector<Int2> path;
	pathfinding->FindPath(lvl, lvl.prevEntryPt, PosToPt(obj->pos), path);
	for(vector<Int2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood& b = Add1(lvl.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f))
				+ Vec3(2.f * it->x + 1 + (float(it->x) - (it - 1)->x) / 2, 0, 2.f * it->y + 1 + (float(it->y) - (it - 1)->y) / 2);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.scale = 1.f;
			b.pos.y = 0.05f;
			b.normal = Vec3(0, 1, 0);
		}
		{
			Blood& b = Add1(lvl.bloods);
			b.pos = Vec3::Random(Vec3(-0.5f, 0.05f, -0.5f), Vec3(0.5f, 0.05f, 0.5f)) + Vec3(2.f * it->x + 1, 0, 2.f * it->y + 1);
			b.type = BLOOD_RED;
			b.rot = Random(MAX_ANGLE);
			b.size = 1.f;
			b.scale = 1.f;
			b.pos.y = 0.05f;
			b.normal = Vec3(0, 1, 0);
		}
	}

	// set special room type so no enemies will spawn there
	lvl.GetNearestRoom(obj->pos)->target = RoomTarget::Treasury;

	evilState = Quest_Evil::State::SpawnedAltar;
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

	static vector<pair<Room*, float>> goodPts;
	for(Room* room : lvl.rooms)
	{
		if(room->target == RoomTarget::None && room->size.x > 2 && room->size.y > 2)
		{
			float dist = Vec3::Distance2d(room->Center(), center);
			goodPts.push_back({ room, dist });
		}
	}
	std::sort(goodPts.begin(), goodPts.end(),
		[](const pair<Room*, float>& p1, const pair<Room*, float>& p2) { return p1.second > p2.second; });

	Room* room;
	while(true)
	{
		room = goodPts.back().first;
		goodPts.pop_back();

		gameLevel->globalCol.clear();
		gameLevel->GatherCollisionObjects(lvl, gameLevel->globalCol, room->Center(), 2.f);
		if(gameLevel->globalCol.empty())
			break;

		if(goodPts.empty())
			throw "No free space to generate portal!";
	}
	goodPts.clear();

	Vec3 portalPos = room->Center();
	room->target = RoomTarget::PortalCreate;
	float rot = PI * Random(0, 3);
	gameLevel->SpawnObjectEntity(lvl, BaseObject::Get("portal"), portalPos, rot);
	inside->portal = new Portal;
	inside->portal->targetLoc = -1;
	inside->portal->nextPortal = nullptr;
	inside->portal->rot = rot;
	inside->portal->pos = portalPos;
	inside->portal->atLevel = gameLevel->dungeonLevel;

	int d = GetLocId(world->GetCurrentLocation());
	loc[d].pos = portalPos;
	loc[d].state = Quest_Evil::Loc::State::None;

	if(game->devmode)
		Info("Generated portal (%g,%g).", portalPos.x, portalPos.z);
}

//=================================================================================================
void Quest_Evil::WarpEvilBossToAltar()
{
	LocationPart& locPart = *gameLevel->localPart;

	// find boss
	Unit* u = locPart.FindUnit(UnitData::Get("qEvilBoss"));
	assert(u);

	// find blood altar
	Object* o = locPart.FindObject(BaseObject::Get("bloodyAltar"));
	assert(o);

	if(u && o)
	{
		Vec3 warpPos = o->pos;
		warpPos -= Vec3(sin(o->rot.y) * 1.5f, 0, cos(o->rot.y) * 1.5f);
		gameLevel->WarpUnit(*u, warpPos);
		u->ai->startPos = u->pos;

		for(int i = 0; i < 2; ++i)
		{
			Unit* u2 = gameLevel->SpawnUnitNearLocation(locPart, u->pos, *UnitData::Get("zombieAncient"));
			if(u2)
			{
				u2->dontAttack = true;
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
	if(evilState == State::SpawnedAltar && gameLevel->location == targetLoc)
	{
		for(Unit& unit : team->members)
		{
			if(unit.IsStanding() && unit.IsPlayer() && Vec3::Distance(unit.pos, pos) < 5.f
				&& gameLevel->CanSee(*gameLevel->localPart, unit.pos, pos))
			{
				evilState = State::Summoning;
				soundMgr->PlaySound2d(gameRes->sEvil);
				if(Net::IsOnline())
					Net::PushChange(NetChange::EVIL_SOUND);
				SetProgress(Progress::AltarEvent);
				// spawn undead
				InsideLocation* inside = (InsideLocation*)gameLevel->location;
				inside->group = UnitGroup::Get("undead");
				DungeonGenerator* gen = (DungeonGenerator*)game->locGenFactory->Get(gameLevel->location);
				gen->GenerateUnits();
				break;
			}
		}
	}
	else if(evilState == State::ClosingPortals && !gameLevel->location->outside && gameLevel->location->GetLastLevel() == gameLevel->dungeonLevel)
	{
		int d = GetLocId(gameLevel->location);
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
								OnUpdate(Format(game->txPortalClosed, gameLevel->location->name.c_str()));
								u->OrderAutoTalk();
								changed = true;
								++closed;
								delete gameLevel->location->portal;
								gameLevel->location->portal = nullptr;
								world->AddNews(Format(game->txPortalClosedNews, gameLevel->location->name.c_str()));
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
