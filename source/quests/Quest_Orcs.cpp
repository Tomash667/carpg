#include "Pch.h"
#include "Quest_Orcs.h"

#include "AIController.h"
#include "Game.h"
#include "GameFile.h"
#include "GameGui.h"
#include "GameResources.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "Level.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "QuestManager.h"
#include "SaveState.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Orcs::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_orkowie_to_miasto");
}

//=================================================================================================
void Quest_Orcs::Start()
{
	type = Q_ORCS;
	category = QuestCategory::Unique;
	vector<int>& used = quest_mgr->GetUsedCities();
	start_loc = world->GetRandomSettlementIndex(used);
	used.push_back(start_loc);
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[6], GetStartLocationName()));

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
			if(quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(game->txQuest[189], GetStartLocationName()));
			quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::NotAccepted:
		{
			if(quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(game->txQuest[190], GetStartLocationName()));
			// mark guard to remove
			Unit*& u = quest_mgr->quest_orcs2->guard;
			if(u)
			{
				u->temporary = true;
				u = nullptr;
			}
			quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::Started:
		{
			OnStart(game->txQuest[191]);
			// remove rumor from pool
			quest_mgr->RemoveQuestRumor(id);
			// mark guard to remove
			Unit*& u = quest_mgr->quest_orcs2->guard;
			if(u)
			{
				u->temporary = true;
				u = nullptr;
			}
			// generate location
			const Vec2 pos = world->FindPlace(GetStartLocation().pos, 64.f);
			Location& tl = *world->CreateLocation(L_DUNGEON, pos, HUMAN_FORT);
			tl.group = UnitGroup::Get("orcs");
			tl.SetKnown();
			tl.st = 8;
			tl.active_quest = this;
			target_loc = tl.index;
			location_event_handler = this;
			at_level = tl.GetLastLevel();
			dungeon_levels = at_level + 1;
			whole_location_event_handler = true;
			item_to_give[0] = Item::Get("q_orkowie_klucz");
			spawn_item = Quest_Event::Item_GiveSpawned2;
			unit_to_spawn = UnitData::Get("q_orkowie_gorush");
			spawn_unit_room = RoomTarget::Prison;
			unit_dont_attack = true;
			unit_to_spawn2 = UnitGroup::Get("orcs")->GetLeader(10);
			unit_spawn_level2 = -3;
			quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::Accepted;
			// add journal entry
			msgs.push_back(Format(game->txQuest[192], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[193], GetStartLocationName(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		{
			OnUpdate(Format(game->txQuest[194], GetTargetLocationName(), GetStartLocationName()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			team->AddReward(4000, 12000);
			OnUpdate(game->txQuest[195]);
			world->AddNews(Format(game->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

			if(quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			{
				quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::CompletedJoined;
				quest_mgr->quest_orcs2->days = Random(30, 60);
				GetTargetLocation().active_quest = nullptr;
				target_loc = -1;
			}
			else
				quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::Completed;
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
bool Quest_Orcs::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_dolaczyl") == 0)
		return quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined || quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::CompletedJoined;
	else if(strcmp(msg, "q_orkowie_to_miasto") == 0)
		return world->GetCurrentLocationIndex() == start_loc;
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

	location_event_handler = this;
	whole_location_event_handler = true;

	if(!done)
	{
		item_to_give[0] = Item::Get("q_orkowie_klucz");
		spawn_item = Quest_Event::Item_GiveSpawned2;
		unit_to_spawn = UnitData::Get("q_orkowie_gorush");
		unit_to_spawn2 = UnitGroup::Get("orcs")->GetLeader(10);
		unit_spawn_level2 = -3;
		spawn_unit_room = RoomTarget::Prison;
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Orcs2::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_orkowie_zaakceptowano");
	quest_mgr->RegisterSpecialIfHandler(this, "q_orkowie_nie_ukonczono");
}

//=================================================================================================
void Quest_Orcs2::Start()
{
	type = Q_ORCS2;
	category = QuestCategory::Unique;
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
	LevelArea& area = *game_level->local_area;

	// search for boss
	UnitData* ud = UnitData::Get("q_orkowie_boss");
	Unit* u = nullptr;
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// search for throne
	Usable* use = area.FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp
	game_level->WarpUnit(*u, use->pos);
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
			OnStart(game->txQuest[214]);
			msgs.push_back(Format(game->txQuest[170], world->GetDate()));
			msgs.push_back(game->txQuest[197]);
			if(orcs_state == Quest_Orcs2::State::Accepted)
				orcs_state = Quest_Orcs2::State::OrcJoined;
			else
			{
				orcs_state = Quest_Orcs2::State::CompletedJoined;
				days = Random(30, 60);
				quest_mgr->quest_orcs->GetTargetLocation().active_quest = nullptr;
				quest_mgr->quest_orcs->target_loc = -1;
			}
			// join team
			DialogContext::current->talker->dont_attack = false;
			team->AddMember(DialogContext::current->talker, HeroType::Free);
			if(team->free_recruits > 0)
				--team->free_recruits;
		}
		break;
	case Progress::TalkedAboutCamp:
		{
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 256.f);
			target_loc = world->CreateCamp(pos, UnitGroup::Get("orcs"));
			Location& target = GetTargetLocation();
			target.state = LS_HIDDEN;
			target.st = 11;
			target.active_quest = this;
			near_loc = world->GetNearestSettlement(target.pos);
			OnUpdate(game->txQuest[198]);
			orcs_state = Quest_Orcs2::State::ToldAboutCamp;
		}
		break;
	case Progress::TalkedWhereIsCamp:
		{
			if(prog == Progress::TalkedWhereIsCamp)
				break;
			Location& target = GetTargetLocation();
			Location& nearl = *world->GetLocation(near_loc);
			target.SetKnown();
			done = false;
			location_event_handler = this;
			OnUpdate(Format(game->txQuest[199], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str()));
		}
		break;
	case Progress::ClearedCamp:
		{
			orc->OrderAutoTalk();
			world->AddNews(game->txQuest[200]);
			team->AddExp(14000);
		}
		break;
	case Progress::TalkedAfterClearingCamp:
		{
			orcs_state = Quest_Orcs2::State::CampCleared;
			days = Random(25, 50);
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			OnUpdate(game->txQuest[201]);
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
			target.active_quest = this;
			target.state = LS_HIDDEN;
			target_loc = target.index;
			OnUpdate(game->txQuest[202]);
			orcs_state = State::ToldAboutBase;
		}
		break;
	case Progress::TalkedWhereIsBase:
		{
			Location& target = GetTargetLocation();
			target.SetKnown();
			unit_to_spawn = UnitData::Get("q_orkowie_boss");
			spawn_unit_room = RoomTarget::Throne;
			callback = WarpToThroneOrcBoss;
			at_level = target.GetLastLevel();
			location_event_handler = nullptr;
			unit_event_handler = this;
			near_loc = world->GetNearestSettlement(target.pos);
			Location& nearl = *world->GetLocation(near_loc);
			OnUpdate(Format(game->txQuest[203], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str(), target.name.c_str()));
			done = false;
			orcs_state = State::GenerateOrcs;
		}
		break;
	case Progress::KilledBoss:
		{
			orc->OrderAutoTalk();
			OnUpdate(game->txQuest[204]);
			world->AddNews(game->txQuest[205]);
			team->AddLearningPoint();
		}
		break;
	case Progress::Finished:
		{
			LevelArea& area = *game_level->local_area;
			state = Quest::Completed;
			team->AddReward(Random(9000, 11000), 25000);
			OnUpdate(game->txQuest[206]);
			quest_mgr->EndUniqueQuest();

			// gorush - move to throne
			team->RemoveMember(orc);
			Usable* throne = area.FindUsable(BaseUsable::Get("throne"));
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

			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsAlive())
				{
					if(u.data == ud_weak_orc)
					{
						u.dont_attack = false;
						u.ai->change_ai_mode = true;
					}
					else
					{
						for(int i = 0; i < 6; ++i)
						{
							if(u.data == ud[i * 2])
							{
								u.data = ud[i * 2 + 1];
								u.ai->target = nullptr;
								u.ai->alert_target = nullptr;
								u.ai->state = AIController::Idle;
								u.ai->change_ai_mode = true;
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
			orcs_state = State::ClearDungeon;
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
		return GetLocationDirName(world->GetLocation(near_loc)->pos, GetTargetLocation().pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(world->GetWorldPos(), GetTargetLocation().pos);
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
	return Any(orcs_state, State::CompletedJoined, State::CampCleared, State::PickedClass) && days <= 0;
}

//=================================================================================================
bool Quest_Orcs2::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_orkowie_woj") == 0)
		return orc_class == OrcClass::Warrior;
	else if(strcmp(msg, "q_orkowie_lowca") == 0)
		return orc_class == OrcClass::Hunter;
	else if(strcmp(msg, "q_orkowie_na_miejscu") == 0)
		return world->GetCurrentLocationIndex() == target_loc;
	else if(strcmp(msg, "q_orkowie_zaakceptowano") == 0)
		return orcs_state >= State::Accepted;
	else if(strcmp(msg, "q_orkowie_nie_ukonczono") == 0)
		return orcs_state < State::Completed;
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
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Orcs2::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << near_loc;
	f << talked;
	f << orcs_state;
	f << days;
	f << guard;
	f << orc;
	f << orc_class;
}

//=================================================================================================
Quest::LoadResult Quest_Orcs2::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> near_loc;
	f >> talked;
	f >> orcs_state;
	f >> days;
	f >> guard;
	f >> orc;
	f >> orc_class;
	if(LOAD_VERSION < V_0_8)
		ItemHelper::SkipStock(f);

	if(!done)
	{
		if(prog == Progress::TalkedWhereIsCamp)
			location_event_handler = this;
		else if(prog == Progress::TalkedWhereIsBase)
		{
			unit_to_spawn = UnitData::Get("q_orkowie_boss");
			spawn_unit_room = RoomTarget::Throne;
			location_event_handler = nullptr;
			unit_event_handler = this;
			callback = WarpToThroneOrcBoss;
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Orcs2::ChangeClass(OrcClass new_orc_class)
{
	cstring class_name, udi;

	orc_class = new_orc_class;

	switch(new_orc_class)
	{
	case OrcClass::Warrior:
	default:
		class_name = game->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		break;
	case OrcClass::Hunter:
		class_name = game->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		break;
	case OrcClass::Shaman:
		class_name = game->txQuest[209];
		udi = "q_orkowie_gorush_szaman";
		break;
	}

	UnitData* ud = UnitData::Get(udi);
	orc->ChangeBase(ud, true);

	OnUpdate(Format(game->txQuest[210], class_name));

	prog = Progress::ChangedClass;
	orcs_state = State::PickedClass;
	days = Random(30, 60);
}

//=================================================================================================
void Quest_Orcs2::OnProgress(int d)
{
	if(Any(orcs_state, State::CompletedJoined, State::CampCleared, State::PickedClass))
	{
		days -= d;
		if(days <= 0)
			orc->OrderAutoTalk();
	}
}
