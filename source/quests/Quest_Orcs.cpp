#include "Pch.h"
#include "Quest_Orcs.h"

#include "AIController.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "Level.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Orcs::Init()
{
	questMgr->RegisterSpecialIfHandler(this, "q_orkowie_to_miasto");
}

//=================================================================================================
void Quest_Orcs::Start()
{
	type = Q_ORCS;
	category = QuestCategory::Unique;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities());
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[6], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Orcs' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Orcs::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(DialogContext::current->talker->data->id == "q_orkowie_straznik")
		return GameDialog::TryGet("q_orcs_guard");
	else
		return GameDialog::TryGet("q_orcs_captain");
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
			if(questMgr->RemoveQuestRumor(id))
				gameGui->journal->AddRumor(Format(questMgr->txQuest[189], GetStartLocationName()));
			questMgr->questOrcs2->orcsState = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::NotAccepted:
		{
			if(questMgr->RemoveQuestRumor(id))
				gameGui->journal->AddRumor(Format(questMgr->txQuest[190], GetStartLocationName()));
			// mark guard to remove
			Unit*& u = questMgr->questOrcs2->guard;
			if(u)
			{
				u->temporary = true;
				u = nullptr;
			}
			questMgr->questOrcs2->orcsState = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::Started:
		{
			OnStart(questMgr->txQuest[191]);
			// remove rumor from pool
			questMgr->RemoveQuestRumor(id);
			// mark guard to remove
			Unit*& u = questMgr->questOrcs2->guard;
			if(u)
			{
				u->temporary = true;
				u = nullptr;
			}
			// generate location
			const Vec2 pos = world->FindPlace(startLoc->pos, 64.f);
			Location& tl = *world->CreateLocation(L_DUNGEON, pos, HUMAN_FORT);
			tl.group = UnitGroup::Get("orcs");
			tl.SetKnown();
			tl.st = 8;
			tl.activeQuest = this;
			targetLoc = &tl;
			locationEventHandler = this;
			atLevel = tl.GetLastLevel();
			dungeon_levels = atLevel + 1;
			wholeLocationEventHandler = true;
			itemToGive[0] = Item::Get("q_orkowie_klucz");
			spawnItem = Quest_Event::Item_GiveSpawned2;
			unitToSpawn = UnitData::Get("q_orkowie_gorush");
			unitSpawnRoom = RoomTarget::Prison;
			unitDontAttack = true;
			unitToSpawn2 = UnitGroup::Get("orcs")->GetLeader(10);
			unitSpawnLevel2 = -3;
			questMgr->questOrcs2->orcsState = Quest_Orcs2::State::Accepted;
			// add journal entry
			msgs.push_back(Format(questMgr->txQuest[192], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[193], GetStartLocationName(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		{
			OnUpdate(Format(questMgr->txQuest[194], GetTargetLocationName(), GetStartLocationName()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			team->AddReward(4000, 12000);
			OnUpdate(questMgr->txQuest[195]);
			world->AddNews(Format(questMgr->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

			if(questMgr->questOrcs2->orcsState == Quest_Orcs2::State::OrcJoined)
			{
				questMgr->questOrcs2->orcsState = Quest_Orcs2::State::CompletedJoined;
				questMgr->questOrcs2->days = Random(30, 60);
				targetLoc->activeQuest = nullptr;
				targetLoc = nullptr;
			}
			else
				questMgr->questOrcs2->orcsState = Quest_Orcs2::State::Completed;
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
		return LocationHelper::IsCity(startLoc) ? questMgr->txQuest[72] : questMgr->txQuest[73];
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
bool Quest_Orcs::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_dolaczyl") == 0)
		return questMgr->questOrcs2->orcsState == Quest_Orcs2::State::OrcJoined || questMgr->questOrcs2->orcsState == Quest_Orcs2::State::CompletedJoined;
	else if(strcmp(msg, "q_orkowie_to_miasto") == 0)
		return world->GetCurrentLocation() == startLoc;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Orcs::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started)
		SetProgress(Progress::ClearedLocation);
	return false;
}

//=================================================================================================
void Quest_Orcs::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << dungeon_levels;
}

//=================================================================================================
Quest::LoadResult Quest_Orcs::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> dungeon_levels;
	if(LOAD_VERSION < V_0_16)
		f.Skip<int>(); // old levels_cleared

	locationEventHandler = this;
	wholeLocationEventHandler = true;

	if(!done)
	{
		itemToGive[0] = Item::Get("q_orkowie_klucz");
		spawnItem = Quest_Event::Item_GiveSpawned2;
		unitToSpawn = UnitData::Get("q_orkowie_gorush");
		unitToSpawn2 = UnitGroup::Get("orcs")->GetLeader(10);
		unitSpawnLevel2 = -3;
		unitSpawnRoom = RoomTarget::Prison;
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Orcs2::Init()
{
	questMgr->RegisterSpecialIfHandler(this, "q_orkowie_zaakceptowano");
	questMgr->RegisterSpecialIfHandler(this, "q_orkowie_nie_ukonczono");
}

//=================================================================================================
void Quest_Orcs2::Start()
{
	type = Q_ORCS2;
	category = QuestCategory::Unique;
	startLoc = nullptr;
	near_loc = -1;
	talked = Talked::No;
	orcsState = State::None;
	guard = nullptr;
	orc = nullptr;
	orcClass = OrcClass::None;
}

//=================================================================================================
GameDialog* Quest_Orcs2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;

	if(id == "q_orkowie_slaby")
		return GameDialog::TryGet("q_orcs2_weak_orc");
	else if(id == "q_orkowie_kowal")
		return GameDialog::TryGet("q_orcs2_blacksmith");
	else if(id == "q_orkowie_gorush" || id == "q_orkowie_gorush_woj" || id == "q_orkowie_gorush_lowca" || id == "q_orkowie_gorush_szaman")
		return GameDialog::TryGet("q_orcs2_gorush");
	else
		return GameDialog::TryGet("q_orcs2_orc");
}

//=================================================================================================
void WarpToThroneOrcBoss()
{
	LocationPart& locPart = *gameLevel->localPart;

	// search for boss
	UnitData* ud = UnitData::Get("q_orkowie_boss");
	Unit* u = nullptr;
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// search for throne
	Usable* use = locPart.FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp
	gameLevel->WarpUnit(*u, use->pos);
}

//=================================================================================================
void Quest_Orcs2::SetProgress(int prog2)
{
	bool apply = true;

	switch(prog2)
	{
	case Progress::TalkedOrc:
		{
			orc = DialogContext::current->talker;
			orc->RevealName(true);
		}
		break;
	case Progress::NotJoined:
		break;
	case Progress::Joined:
		// add quest
		{
			OnStart(questMgr->txQuest[214]);
			msgs.push_back(Format(questMgr->txQuest[170], world->GetDate()));
			msgs.push_back(questMgr->txQuest[197]);
			if(orcsState == Quest_Orcs2::State::Accepted)
				orcsState = Quest_Orcs2::State::OrcJoined;
			else
			{
				orcsState = Quest_Orcs2::State::CompletedJoined;
				days = Random(30, 60);
				questMgr->questOrcs->targetLoc->activeQuest = nullptr;
				questMgr->questOrcs->targetLoc = nullptr;
			}
			// join team
			DialogContext::current->talker->dontAttack = false;
			team->AddMember(DialogContext::current->talker, HeroType::Free);
			if(team->freeRecruits > 0)
				--team->freeRecruits;
		}
		break;
	case Progress::TalkedAboutCamp:
		{
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 256.f);
			targetLoc = world->CreateCamp(pos, UnitGroup::Get("orcs"));
			targetLoc->state = LS_HIDDEN;
			targetLoc->st = 11;
			targetLoc->activeQuest = this;
			near_loc = world->GetNearestSettlement(targetLoc->pos)->index;
			OnUpdate(questMgr->txQuest[198]);
			orcsState = Quest_Orcs2::State::ToldAboutCamp;
		}
		break;
	case Progress::TalkedWhereIsCamp:
		{
			if(prog == Progress::TalkedWhereIsCamp)
				break;
			Location& nearl = *world->GetLocation(near_loc);
			targetLoc->SetKnown();
			done = false;
			locationEventHandler = this;
			OnUpdate(Format(questMgr->txQuest[199], GetLocationDirName(nearl.pos, targetLoc->pos), nearl.name.c_str()));
		}
		break;
	case Progress::ClearedCamp:
		{
			orc->OrderAutoTalk();
			world->AddNews(questMgr->txQuest[200]);
			team->AddExp(14000);
		}
		break;
	case Progress::TalkedAfterClearingCamp:
		{
			orcsState = Quest_Orcs2::State::CampCleared;
			days = Random(25, 50);
			targetLoc->activeQuest = nullptr;
			targetLoc = nullptr;
			OnUpdate(questMgr->txQuest[201]);
		}
		break;
	case Progress::SelectWarrior:
		apply = false;
		ChangeClass(OrcClass::Warrior);
		break;
	case Progress::SelectHunter:
		apply = false;
		ChangeClass(OrcClass::Hunter);
		break;
	case Progress::SelectShaman:
		apply = false;
		ChangeClass(OrcClass::Shaman);
		break;
	case Progress::SelectRandom:
		{
			Class* player_class = DialogContext::current->pc->unit->GetClass();
			OrcClass clas;
			if(player_class->id == "warrior")
			{
				if(Rand() % 2 == 0)
					clas = OrcClass::Hunter;
				else
					clas = OrcClass::Shaman;
			}
			else if(player_class->id == "hunter")
			{
				if(Rand() % 2 == 0)
					clas = OrcClass::Warrior;
				else
					clas = OrcClass::Shaman;
			}
			else
			{
				switch(Rand() % 3)
				{
				case 0:
					clas = OrcClass::Warrior;
					break;
				case 1:
					clas = OrcClass::Hunter;
					break;
				case 2:
					clas = OrcClass::Shaman;
					break;
				}
			}

			apply = false;
			ChangeClass(clas);
		}
		break;
	case Progress::TalkedAboutBase:
		{
			done = false;
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 256.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, THRONE_FORT);
			target.group = UnitGroup::Get("orcs");
			target.st = 15;
			target.activeQuest = this;
			target.state = LS_HIDDEN;
			targetLoc = &target;
			OnUpdate(questMgr->txQuest[202]);
			orcsState = State::ToldAboutBase;
		}
		break;
	case Progress::TalkedWhereIsBase:
		{
			targetLoc->SetKnown();
			unitToSpawn = UnitData::Get("q_orkowie_boss");
			unitSpawnRoom = RoomTarget::Throne;
			callback = WarpToThroneOrcBoss;
			atLevel = targetLoc->GetLastLevel();
			locationEventHandler = nullptr;
			unitEventHandler = this;
			near_loc = world->GetNearestSettlement(targetLoc->pos)->index;
			Location& nearl = *world->GetLocation(near_loc);
			OnUpdate(Format(questMgr->txQuest[203], GetLocationDirName(nearl.pos, targetLoc->pos), nearl.name.c_str(), targetLoc->name.c_str()));
			done = false;
			orcsState = State::GenerateOrcs;
		}
		break;
	case Progress::KilledBoss:
		{
			orc->OrderAutoTalk();
			OnUpdate(questMgr->txQuest[204]);
			world->AddNews(questMgr->txQuest[205]);
			team->AddLearningPoint();
		}
		break;
	case Progress::Finished:
		{
			LocationPart& locPart = *gameLevel->localPart;
			state = Quest::Completed;
			team->AddReward(Random(9000, 11000), 25000);
			OnUpdate(questMgr->txQuest[206]);
			questMgr->EndUniqueQuest();

			// gorush - move to throne
			team->RemoveMember(orc);
			Usable* throne = locPart.FindUsable(BaseUsable::Get("throne"));
			assert(throne);
			if(throne)
			{
				orc->ai->st.idle.action = AIController::Idle_WalkUse;
				orc->ai->st.idle.usable = throne;
				orc->ai->timer = 9999.f;
			}
			orc = nullptr;

			// convert orcs to friendly version
			UnitData* ud[12] = {
				UnitData::Get("orc"), UnitData::Get("q_orkowie_orc"),
				UnitData::Get("orc_fighter"), UnitData::Get("q_orkowie_orc_fighter"),
				UnitData::Get("orc_warius"), UnitData::Get("q_orkowie_orc_warius"),
				UnitData::Get("orc_hunter"), UnitData::Get("q_orkowie_orc_hunter"),
				UnitData::Get("orc_shaman"), UnitData::Get("q_orkowie_orc_shaman"),
				UnitData::Get("orc_chief"), UnitData::Get("q_orkowie_orc_chief")
			};
			UnitData* ud_weak_orc = UnitData::Get("q_orkowie_slaby");

			for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsAlive())
				{
					if(u.data == ud_weak_orc)
					{
						u.dontAttack = false;
						u.ai->changeAiMode = true;
					}
					else
					{
						for(int i = 0; i < 6; ++i)
						{
							if(u.data == ud[i * 2])
							{
								u.data = ud[i * 2 + 1];
								u.ai->target = nullptr;
								u.ai->alertTarget = nullptr;
								u.ai->state = AIController::Idle;
								u.ai->changeAiMode = true;
								if(Net::IsOnline())
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::CHANGE_UNIT_BASE;
									c.unit = &u;
								}
								break;
							}
						}
					}
				}
			}

			// assumes that player is talking at last dungeon level, hopefuly player won't run away before talking
			MultiInsideLocation* multi = (MultiInsideLocation*)world->GetCurrentLocation();
			for(vector<InsideLocationLevel*>::iterator it = multi->levels.begin(), end = multi->levels.end() - 1; it != end; ++it)
			{
				for(Unit* unit : (*it)->units)
				{
					if(unit->IsAlive())
					{
						for(int i = 0; i < 5; ++i)
						{
							if(unit->data == ud[i * 2])
							{
								unit->data = ud[i * 2 + 1];
								break;
							}
						}
					}
				}
			}

			// clear corpses after leaving dungeon
			orcsState = State::ClearDungeon;
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
		return world->GetLocation(near_loc)->name.c_str();
	else if(str == "close_dir")
		return GetLocationDirName(world->GetLocation(near_loc)->pos, targetLoc->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(world->GetWorldPos(), targetLoc->pos);
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
	return Any(orcsState, State::CompletedJoined, State::CampCleared, State::PickedClass) && days <= 0;
}

//=================================================================================================
bool Quest_Orcs2::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_woj") == 0)
		return orcClass == OrcClass::Warrior;
	else if(strcmp(msg, "q_orkowie_lowca") == 0)
		return orcClass == OrcClass::Hunter;
	else if(strcmp(msg, "q_orkowie_na_miejscu") == 0)
		return world->GetCurrentLocation() == targetLoc;
	else if(strcmp(msg, "q_orkowie_zaakceptowano") == 0)
		return orcsState >= State::Accepted;
	else if(strcmp(msg, "q_orkowie_nie_ukonczono") == 0)
		return orcsState < State::Completed;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Orcs2::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::TalkedWhereIsCamp)
	{
		SetProgress(Progress::ClearedCamp);
		return true;
	}
	return false;
}

//=================================================================================================
void Quest_Orcs2::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == Progress::TalkedWhereIsBase)
	{
		SetProgress(Progress::KilledBoss);
		unit->eventHandler = nullptr;
	}
}

//=================================================================================================
void Quest_Orcs2::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << near_loc;
	f << talked;
	f << orcsState;
	f << days;
	f << guard;
	f << orc;
	f << orcClass;
}

//=================================================================================================
Quest::LoadResult Quest_Orcs2::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> near_loc;
	f >> talked;
	f >> orcsState;
	f >> days;
	f >> guard;
	f >> orc;
	f >> orcClass;

	if(!done)
	{
		if(prog == Progress::TalkedWhereIsCamp)
			locationEventHandler = this;
		else if(prog == Progress::TalkedWhereIsBase)
		{
			unitToSpawn = UnitData::Get("q_orkowie_boss");
			unitSpawnRoom = RoomTarget::Throne;
			locationEventHandler = nullptr;
			unitEventHandler = this;
			callback = WarpToThroneOrcBoss;
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Orcs2::ChangeClass(OrcClass newOrcClass)
{
	cstring class_name, udi;

	orcClass = newOrcClass;

	switch(newOrcClass)
	{
	case OrcClass::Warrior:
	default:
		class_name = questMgr->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		break;
	case OrcClass::Hunter:
		class_name = questMgr->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		break;
	case OrcClass::Shaman:
		class_name = questMgr->txQuest[209];
		udi = "q_orkowie_gorush_szaman";
		break;
	}

	UnitData* ud = UnitData::Get(udi);
	orc->ChangeBase(ud, true);

	OnUpdate(Format(questMgr->txQuest[210], class_name));

	prog = Progress::ChangedClass;
	orcsState = State::PickedClass;
	days = Random(30, 60);
}

//=================================================================================================
void Quest_Orcs2::OnProgress(int d)
{
	if(Any(orcsState, State::CompletedJoined, State::CampCleared, State::PickedClass))
	{
		days -= d;
		if(days <= 0)
			orc->OrderAutoTalk();
	}
}
