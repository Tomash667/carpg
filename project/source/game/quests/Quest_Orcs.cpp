#include "Pch.h"
#include "GameCore.h"
#include "Quest_Orcs.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "MultiInsideLocation.h"
#include "GlobalGui.h"
#include "AIController.h"
#include "Team.h"
#include "World.h"
#include "Level.h"
#include "ItemHelper.h"

//=================================================================================================
void Quest_Orcs::Init()
{
	QM.RegisterSpecialIfHandler(this, "q_orkowie_to_miasto");
}

//=================================================================================================
void Quest_Orcs::Start()
{
	quest_id = Q_ORCS;
	type = QuestType::Unique;
	QM.AddQuestRumor(refid, Format(QM.txRumorQ[6], GetStartLocationName()));
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
			if(QM.RemoveQuestRumor(refid))
				game->gui->journal->AddRumor(Format(game->txQuest[189], GetStartLocationName()));
			QM.quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::NotAccepted:
		{
			if(QM.RemoveQuestRumor(refid))
				game->gui->journal->AddRumor(Format(game->txQuest[190], GetStartLocationName()));
			// mark guard to remove
			Unit*& u = QM.quest_orcs2->guard;
			if(u)
			{
				u->auto_talk = AutoTalkMode::No;
				u->temporary = true;
				u = nullptr;
			}
			QM.quest_orcs2->orcs_state = Quest_Orcs2::State::GuardTalked;
		}
		break;
	case Progress::Started:
		{
			OnStart(game->txQuest[191]);
			// remove rumor from pool
			QM.RemoveQuestRumor(refid);
			// mark guard to remove
			Unit*& u = QM.quest_orcs2->guard;
			if(u)
			{
				u->auto_talk = AutoTalkMode::No;
				u->temporary = true;
				u = nullptr;
			}
			// generate location
			Location& tl = *W.CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, HUMAN_FORT, SG_ORCS, false);
			tl.SetKnown();
			tl.st = 8;
			tl.active_quest = this;
			target_loc = tl.index;
			location_event_handler = this;
			at_level = tl.GetLastLevel();
			dungeon_levels = at_level + 1;
			levels_cleared = 0;
			whole_location_event_handler = true;
			item_to_give[0] = Item::Get("q_orkowie_klucz");
			spawn_item = Quest_Event::Item_GiveSpawned2;
			unit_to_spawn = UnitData::Get("q_orkowie_gorush");
			spawn_unit_room = RoomTarget::Prison;
			unit_dont_attack = true;
			unit_to_spawn2 = g_spawn_groups[SG_ORCS].GetSpawnLeader(10);
			unit_spawn_level2 = -3;
			QM.quest_orcs2->orcs_state = Quest_Orcs2::State::Accepted;
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[192], GetStartLocationName(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[193], GetStartLocationName(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		// oczyszczono lokacjê
		{
			OnUpdate(Format(game->txQuest[194], GetTargetLocationName(), GetStartLocationName()));
		}
		break;
	case Progress::Finished:
		// ukoñczono - nagroda
		{
			state = Quest::Completed;

			Team.AddReward(4000, 12000);
			OnUpdate(game->txQuest[195]);
			W.AddNews(Format(game->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

			if(QM.quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			{
				QM.quest_orcs2->orcs_state = Quest_Orcs2::State::CompletedJoined;
				QM.quest_orcs2->days = Random(30, 60);
				GetTargetLocation().active_quest = nullptr;
				target_loc = -1;
			}
			else
				QM.quest_orcs2->orcs_state = Quest_Orcs2::State::Completed;
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
		return QM.quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined || QM.quest_orcs2->orcs_state == Quest_Orcs2::State::CompletedJoined;
	else if(strcmp(msg, "q_orkowie_to_miasto") == 0)
		return W.GetCurrentLocationIndex() == start_loc;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Orcs::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started)
	{
		levels_cleared |= (1 << L.dungeon_level);
		if(CountBits(levels_cleared) == dungeon_levels)
			SetProgress(Progress::ClearedLocation);
	}
	return false;
}

//=================================================================================================
void Quest_Orcs::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << dungeon_levels;
	f << levels_cleared;
}

//=================================================================================================
bool Quest_Orcs::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> dungeon_levels;
	f >> levels_cleared;

	location_event_handler = this;
	whole_location_event_handler = true;

	if(!done)
	{
		item_to_give[0] = Item::Get("q_orkowie_klucz");
		spawn_item = Quest_Event::Item_GiveSpawned2;
		unit_to_spawn = UnitData::Get("q_orkowie_gorush");
		unit_to_spawn2 = g_spawn_groups[SG_ORCS].GetSpawnLeader(10);
		unit_spawn_level2 = -3;
		spawn_unit_room = RoomTarget::Prison;
	}

	return true;
}


//=================================================================================================
void Quest_Orcs2::Init()
{
	QM.RegisterSpecialIfHandler(this, "q_orkowie_zaakceptowano");
	QM.RegisterSpecialIfHandler(this, "q_orkowie_nie_ukonczono");
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
	// szukaj orka
	UnitData* ud = UnitData::Get("q_orkowie_boss");
	Unit* u = nullptr;
	for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Usable* use = L.local_ctx.FindUsable("throne");
	assert(use);

	// przenieœ
	L.WarpUnit(*u, use->pos);
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
			orc = DialogContext::current->talker;
			orc->RevealName(true);
		}
		break;
	case Progress::NotJoined:
		break;
	case Progress::Joined:
		// dodaj questa
		{
			OnStart(game->txQuest[214]);
			msgs.push_back(Format(game->txQuest[170], W.GetDate()));
			msgs.push_back(game->txQuest[197]);
			// ustaw stan
			if(orcs_state == Quest_Orcs2::State::Accepted)
				orcs_state = Quest_Orcs2::State::OrcJoined;
			else
			{
				orcs_state = Quest_Orcs2::State::CompletedJoined;
				days = Random(30, 60);
				QM.quest_orcs->GetTargetLocation().active_quest = nullptr;
				QM.quest_orcs->target_loc = -1;
			}
			// do³¹cz do dru¿yny
			DialogContext::current->talker->dont_attack = false;
			Team.AddTeamMember(DialogContext::current->talker, true);
			if(Team.free_recruits > 0)
				--Team.free_recruits;
		}
		break;
	case Progress::TalkedAboutCamp:
		// powiedzia³ o obozie
		{
			target_loc = W.CreateCamp(W.GetWorldPos(), SG_ORCS, 256.f, false);
			Location& target = GetTargetLocation();
			target.state = LS_HIDDEN;
			target.st = 11;
			target.active_quest = this;
			near_loc = W.GetNearestSettlement(target.pos);
			OnUpdate(game->txQuest[198]);
			orcs_state = Quest_Orcs2::State::ToldAboutCamp;
		}
		break;
	case Progress::TalkedWhereIsCamp:
		// powiedzia³ gdzie obóz
		{
			if(prog == Progress::TalkedWhereIsCamp)
				break;
			Location& target = GetTargetLocation();
			Location& nearl = *W.GetLocation(near_loc);
			target.SetKnown();
			done = false;
			location_event_handler = this;
			OnUpdate(Format(game->txQuest[199], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str()));
		}
		break;
	case Progress::ClearedCamp:
		// oczyszczono obóz orków
		{
			orc->StartAutoTalk();
			W.AddNews(game->txQuest[200]);
			Team.AddExp(14000);
		}
		break;
	case Progress::TalkedAfterClearingCamp:
		// pogada³ po oczyszczeniu
		{
			orcs_state = Quest_Orcs2::State::CampCleared;
			days = Random(25, 50);
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			OnUpdate(game->txQuest[201]);
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
			if(DialogContext::current->pc->unit->GetClass() == Class::WARRIOR)
			{
				if(Rand() % 2 == 0)
					clas = OrcClass::Hunter;
				else
					clas = OrcClass::Shaman;
			}
			else if(DialogContext::current->pc->unit->GetClass() == Class::HUNTER)
			{
				if(Rand() % 2 == 0)
					clas = OrcClass::Warrior;
				else
					clas = OrcClass::Shaman;
			}
			else
			{
				int co = Rand() % 3;
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
			done = false;
			Location& target = *W.CreateLocation(L_DUNGEON, W.GetWorldPos(), 256.f, THRONE_FORT, SG_ORCS, false);
			target.st = 15;
			target.active_quest = this;
			target.state = LS_HIDDEN;
			target_loc = target.index;
			OnUpdate(game->txQuest[202]);
			orcs_state = State::ToldAboutBase;
		}
		break;
	case Progress::TalkedWhereIsBase:
		// powiedzia³ gdzie baza
		{
			Location& target = GetTargetLocation();
			target.SetKnown();
			unit_to_spawn = UnitData::Get("q_orkowie_boss");
			spawn_unit_room = RoomTarget::Throne;
			callback = WarpToThroneOrcBoss;
			at_level = target.GetLastLevel();
			location_event_handler = nullptr;
			unit_event_handler = this;
			near_loc = W.GetNearestSettlement(target.pos);
			Location& nearl = *W.GetLocation(near_loc);
			OnUpdate(Format(game->txQuest[203], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str(), target.name.c_str()));
			done = false;
			orcs_state = State::GenerateOrcs;
		}
		break;
	case Progress::KilledBoss:
		// zabito bossa
		{
			orc->StartAutoTalk();
			OnUpdate(game->txQuest[204]);
			W.AddNews(game->txQuest[205]);
			Team.AddLearningPoint();
		}
		break;
	case Progress::Finished:
		// pogadano z gorushem
		{
			state = Quest::Completed;
			Team.AddReward(Random(9000, 11000), 25000);
			OnUpdate(game->txQuest[206]);
			QM.EndUniqueQuest();
			// gorush
			Team.RemoveTeamMember(orc);
			Usable* tron = L.local_ctx.FindUsable("throne");
			assert(tron);
			if(tron)
			{
				orc->ai->idle_action = AIController::Idle_WalkUse;
				orc->ai->idle_data.usable = tron;
				orc->ai->timer = 9999.f;
			}
			orc = nullptr;
			// orki
			UnitData* ud[12] = {
				UnitData::Get("orc"), UnitData::Get("q_orkowie_orc"),
				UnitData::Get("orc_fighter"), UnitData::Get("q_orkowie_orc_fighter"),
				UnitData::Get("orc_warius"), UnitData::Get("q_orkowie_orc_warius"),
				UnitData::Get("orc_hunter"), UnitData::Get("q_orkowie_orc_hunter"),
				UnitData::Get("orc_shaman"), UnitData::Get("q_orkowie_orc_shaman"),
				UnitData::Get("orc_chief"), UnitData::Get("q_orkowie_orc_chief")
			};
			UnitData* ud_slaby = UnitData::Get("q_orkowie_slaby");

			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
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
			// zak³ada ¿e gadamy na ostatnim levelu, mam nadzieje ¿e gracz z tamt¹d nie spierdoli przed pogadaniem :3
			MultiInsideLocation* multi = (MultiInsideLocation*)W.GetCurrentLocation();
			for(vector<InsideLocationLevel>::iterator it = multi->levels.begin(), end = multi->levels.end() - 1; it != end; ++it)
			{
				for(vector<Unit*>::iterator it2 = it->units.begin(), end2 = it->units.end(); it2 != end2; ++it2)
				{
					Unit& u = **it2;
					if(u.IsAlive())
					{
						for(int i = 0; i < 5; ++i)
						{
							if(u.data == ud[i * 2])
							{
								u.data = ud[i * 2 + 1];
								break;
							}
						}
					}
				}
			}
			// usuñ zw³oki po opuszczeniu lokacji
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
		return W.GetLocation(near_loc)->name.c_str();
	else if(str == "close_dir")
		return GetLocationDirName(W.GetLocation(near_loc)->pos, GetTargetLocation().pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(W.GetWorldPos(), GetTargetLocation().pos);
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
		return W.GetCurrentLocationIndex() == target_loc;
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
bool Quest_Orcs2::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> near_loc;
	f >> talked;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> orcs_state;
		f >> days;
		f >> guard;
		f >> orc;
		f >> orc_class;
		if(LOAD_VERSION < V_0_8)
			ItemHelper::SkipStock(f);
	}

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

	return true;
}

//=================================================================================================
void Quest_Orcs2::LoadOld(GameReader& f)
{
	int city, old_refid, old_refid2, where;

	f >> orcs_state;
	f >> city;
	f >> old_refid;
	f >> old_refid2;
	f >> days;
	f >> where;
	f >> guard;
	f >> orc;
	f >> orc_class;
	ItemHelper::SkipStock(f);
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
	orc->data = ud;
	orc->level = ud->level.x;
	orc->stats = orc->data->GetStats(orc->level);
	orc->CalculateStats();
	game->ParseItemScript(*orc, ud->item_script);
	for(const Item* item : orc->slots)
	{
		if(item)
			game->PreloadItem(item);
	}
	for(ItemSlot& slot : orc->items)
		game->PreloadItem(slot.item);
	orc->MakeItemsTeam(false);
	orc->UpdateInventory();

	OnUpdate(Format(game->txQuest[210], class_name));

	prog = Progress::ChangedClass;
	orcs_state = State::PickedClass;
	days = Random(30, 60);

	if(orc->GetClass() == Class::WARRIOR)
		orc->hero->melee = true;

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_UNIT_BASE;
		c.unit = orc;
	}
}
