#include "Pch.h"
#include "GameCore.h"
#include "BitStreamFunc.h"
#include "QuestManager.h"
#include "SaveState.h"
#include "GameFile.h"
#include "World.h"
#include "Content.h"
#include "Net.h"
#include "QuestScheme.h"
#include "QuestList.h"
#include "Language.h"
#include "Game.h"
#include "Level.h"
#include "Team.h"
#include "City.h"
#include "InsideLocation.h"

#include "Quest_Artifacts.h"
#include "Quest_Bandits.h"
#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
#include "Quest_Contest.h"
#include "Quest_Crazies.h"
#include "Quest_DeliverLetter.h"
#include "Quest_DeliverParcel.h"
#include "Quest_Evil.h"
#include "Quest_FindArtifact.h"
#include "Quest_Goblins.h"
#include "Quest_KillAnimals.h"
#include "Quest_LostArtifact.h"
#include "Quest_Mages.h"
#include "Quest_Mine.h"
#include "Quest_Orcs.h"
#include "Quest_RescueCaptive.h"
#include "Quest_RetrievePackage.h"
#include "Quest_Sawmill.h"
#include "Quest_Scripted.h"
#include "Quest_Secret.h"
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "Quest_Wanted.h"

QuestManager* global::quest_mgr;

//=================================================================================================
QuestManager::QuestManager() : quest_contest(nullptr), quest_secret(nullptr), quest_tournament(nullptr), quest_tutorial(nullptr)
{
}

//=================================================================================================
QuestManager::~QuestManager()
{
	delete quest_contest;
	delete quest_secret;
	delete quest_tournament;
	delete quest_tutorial;
}

//=================================================================================================
void QuestManager::Init()
{
	force = Q_FORCE_DISABLED;

	infos.push_back(QuestInfo(Q_MINE, QuestCategory::Unique, "mine"));
	infos.push_back(QuestInfo(Q_SAWMILL, QuestCategory::Unique, "sawmill"));
	infos.push_back(QuestInfo(Q_BANDITS, QuestCategory::Unique, "bandits"));
	infos.push_back(QuestInfo(Q_MAGES, QuestCategory::Unique, "mages"));
	infos.push_back(QuestInfo(Q_MAGES2, QuestCategory::Unique, "mages2"));
	infos.push_back(QuestInfo(Q_ORCS, QuestCategory::Unique, "orcs"));
	infos.push_back(QuestInfo(Q_ORCS2, QuestCategory::Unique, "orcs2"));
	infos.push_back(QuestInfo(Q_GOBLINS, QuestCategory::Unique, "goblins"));
	infos.push_back(QuestInfo(Q_EVIL, QuestCategory::Unique, "evil"));
	infos.push_back(QuestInfo(Q_DELIVER_LETTER, QuestCategory::Mayor, "deliver_letter"));
	infos.push_back(QuestInfo(Q_DELIVER_PARCEL, QuestCategory::Mayor, "deliver_parcel"));
	infos.push_back(QuestInfo(Q_SPREAD_NEWS, QuestCategory::Mayor, "spread_news"));
	infos.push_back(QuestInfo(Q_RESCUE_CAPTIVE, QuestCategory::Captain, "rescue_captive"));
	infos.push_back(QuestInfo(Q_CAMP_NEAR_CITY, QuestCategory::Captain, "camp_near_city"));
	infos.push_back(QuestInfo(Q_RETRIEVE_PACKAGE, QuestCategory::Mayor, "retrieve_package"));
	infos.push_back(QuestInfo(Q_KILL_ANIMALS, QuestCategory::Captain, "kill_animals"));
	infos.push_back(QuestInfo(Q_LOST_ARTIFACT, QuestCategory::Random, "lost_artifact"));
	infos.push_back(QuestInfo(Q_STOLEN_ARTIFACT, QuestCategory::Random, "stolen_artifact"));
	infos.push_back(QuestInfo(Q_FIND_ARTIFACT, QuestCategory::Random, "find_artifact"));
	infos.push_back(QuestInfo(Q_CRAZIES, QuestCategory::Unique, "crazies"));
	infos.push_back(QuestInfo(Q_WANTED, QuestCategory::Captain, "wanted"));
	infos.push_back({ Q_ARTIFACTS, QuestCategory::Unique, "artifacts" });

	// create pseudo quests
	quest_contest = new Quest_Contest;
	quest_contest->InitOnce();
	quest_secret = new Quest_Secret;
	quest_secret->InitOnce();
	quest_tournament = new Quest_Tournament;
	quest_tournament->InitOnce();
	quest_tutorial = new Quest_Tutorial;
}

//=================================================================================================
void QuestManager::InitLists()
{
	quests_mayor = QuestList::TryGet("mayor");
	quests_captain = QuestList::TryGet("captain");
	quests_random = QuestList::TryGet("random");
	assert(quests_mayor && quests_captain && quests_random);

	unique_quests = 8;
	for(QuestScheme* scheme : QuestScheme::schemes)
	{
		if(scheme->category == QuestCategory::Unique)
			++unique_quests;
	}
}

//=================================================================================================
void QuestManager::LoadLanguage()
{
	LoadArray(txRumorQ, "rumorQ");
	quest_contest->LoadLanguage();
	quest_secret->LoadLanguage();
	quest_tournament->LoadLanguage();
	quest_tutorial->LoadLanguage();
}

//=================================================================================================
void QuestManager::Clear()
{
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_item_requests);
	DeleteElements(quest_items);
	if(quest_tournament)
		quest_tournament->Clear();
}

//=================================================================================================
void QuestManager::InitQuests()
{
	vector<int> used;

	// goblins
	quest_goblins = new Quest_Goblins;
	quest_goblins->Init();
	quest_goblins->start_loc = world->GetRandomSettlementIndex(used, CITY);
	quest_goblins->id = quest_counter++;
	quest_goblins->Start();
	unaccepted_quests.push_back(quest_goblins);
	used.push_back(quest_goblins->start_loc);

	// bandits
	quest_bandits = new Quest_Bandits;
	quest_bandits->Init();
	quest_bandits->start_loc = world->GetRandomSettlementIndex(used, CITY);
	quest_bandits->id = quest_counter++;
	quest_bandits->Start();
	unaccepted_quests.push_back(quest_bandits);
	used.push_back(quest_bandits->start_loc);

	// sawmill
	quest_sawmill = new Quest_Sawmill;
	quest_sawmill->start_loc = world->GetRandomSettlementIndex(used);
	quest_sawmill->id = quest_counter++;
	quest_sawmill->Start();
	unaccepted_quests.push_back(quest_sawmill);
	used.push_back(quest_sawmill->start_loc);

	// mine
	quest_mine = new Quest_Mine;
	quest_mine->start_loc = world->GetRandomSettlementIndex(used);
	quest_mine->target_loc = world->GetClosestLocation(L_CAVE, world->GetLocation(quest_mine->start_loc)->pos);
	quest_mine->id = quest_counter++;
	quest_mine->Start();
	unaccepted_quests.push_back(quest_mine);
	used.push_back(quest_mine->start_loc);

	// mages
	quest_mages = new Quest_Mages;
	quest_mages->start_loc = world->GetRandomSettlementIndex(used);
	quest_mages->id = quest_counter++;
	quest_mages->Start();
	unaccepted_quests.push_back(quest_mages);
	used.push_back(quest_mages->start_loc);

	// mages2
	quest_mages2 = new Quest_Mages2;
	quest_mages2->Init();
	quest_mages2->id = quest_counter++;
	quest_mages2->Start();
	unaccepted_quests.push_back(quest_mages2);

	// orcs
	quest_orcs = new Quest_Orcs;
	quest_orcs->Init();
	quest_orcs->start_loc = world->GetRandomSettlementIndex(used);
	quest_orcs->id = quest_counter++;
	quest_orcs->Start();
	unaccepted_quests.push_back(quest_orcs);
	used.push_back(quest_orcs->start_loc);

	// orcs2
	quest_orcs2 = new Quest_Orcs2;
	quest_orcs2->Init();
	quest_orcs2->id = quest_counter++;
	quest_orcs2->Start();
	unaccepted_quests.push_back(quest_orcs2);

	// evil
	quest_evil = new Quest_Evil;
	quest_evil->Init();
	quest_evil->start_loc = world->GetRandomSettlementIndex(used);
	quest_evil->id = quest_counter++;
	quest_evil->Start();
	unaccepted_quests.push_back(quest_evil);
	used.push_back(quest_evil->start_loc);

	// crazies
	quest_crazies = new Quest_Crazies;
	quest_crazies->Init();
	quest_crazies->id = quest_counter++;
	quest_crazies->Start();
	unaccepted_quests.push_back(quest_crazies);

	// artifacts
	quest_artifacts = new Quest_Artifacts;
	quest_artifacts->id = quest_counter++;
	quest_artifacts->Start();
	unaccepted_quests.push_back(quest_artifacts);

	// pseudo quests
	quest_contest->Init();
	quest_secret->Init();
	quest_tournament->Init();

	if(game->devmode)
	{
		Info("Quest 'Sawmill' - %s.", world->GetLocation(quest_sawmill->start_loc)->name.c_str());
		Info("Quest 'Mine' - %s, %s.", world->GetLocation(quest_mine->start_loc)->name.c_str(), world->GetLocation(quest_mine->target_loc)->name.c_str());
		Info("Quest 'Bandits' - %s.", world->GetLocation(quest_bandits->start_loc)->name.c_str());
		Info("Quest 'Mages' - %s.", world->GetLocation(quest_mages->start_loc)->name.c_str());
		Info("Quest 'Orcs' - %s.", world->GetLocation(quest_orcs->start_loc)->name.c_str());
		Info("Quest 'Goblins' - %s.", world->GetLocation(quest_goblins->start_loc)->name.c_str());
		Info("Quest 'Evil' - %s.", world->GetLocation(quest_evil->start_loc)->name.c_str());
		Info("Tournament - %s.", world->GetLocation(quest_tournament->GetCity())->name.c_str());
		Info("Contest - %s.", world->GetLocation(quest_contest->where)->name.c_str());
		Info("Gladiator armor - %s.", world->GetLocation(quest_artifacts->target_loc)->name.c_str());
	}

	// init scripted quests
	for(QuestScheme* scheme : QuestScheme::schemes)
	{
		if(scheme->category != QuestCategory::Unique)
			continue;
		Quest_Scripted* quest = new Quest_Scripted;
		quest->Init(scheme);
		unaccepted_quests.push_back(quest);
		quest->id = quest_counter++;
		quest->Start();
	}
}

//=================================================================================================
Quest* QuestManager::CreateQuest(QUEST_TYPE type)
{
	switch(type)
	{
	case Q_DELIVER_LETTER:
		return new Quest_DeliverLetter;
	case Q_DELIVER_PARCEL:
		return new Quest_DeliverParcel;
	case Q_SPREAD_NEWS:
		return new Quest_SpreadNews;
	case Q_RETRIEVE_PACKAGE:
		return new Quest_RetrievePackage;
	case Q_RESCUE_CAPTIVE:
		return new Quest_RescueCaptive;
	case Q_BANDITS_COLLECT_TOLL:
		return new Quest_BanditsCollectToll;
	case Q_CAMP_NEAR_CITY:
		return new Quest_CampNearCity;
	case Q_KILL_ANIMALS:
		return new Quest_KillAnimals;
	case Q_FIND_ARTIFACT:
		return new Quest_FindArtifact;
	case Q_LOST_ARTIFACT:
		return new Quest_LostArtifact;
	case Q_STOLEN_ARTIFACT:
		return new Quest_StolenArtifact;
	case Q_SAWMILL:
		return new Quest_Sawmill;
	case Q_MINE:
		return new Quest_Mine;
	case Q_BANDITS:
		return new Quest_Bandits;
	case Q_MAGES:
		return new Quest_Mages;
	case Q_MAGES2:
		return new Quest_Mages2;
	case Q_ORCS:
		return new Quest_Orcs;
	case Q_ORCS2:
		return new Quest_Orcs2;
	case Q_GOBLINS:
		return new Quest_Goblins;
	case Q_EVIL:
		return new Quest_Evil;
	case Q_CRAZIES:
		return new Quest_Crazies;
	case Q_WANTED:
		return new Quest_Wanted;
	case Q_ARTIFACTS:
		return new Quest_Artifacts;
	case Q_FORCE_NONE:
		return nullptr;
	case Q_SCRIPTED:
		return new Quest_Scripted;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Quest* QuestManager::CreateQuest(QuestInfo* info)
{
	if(!info)
		return nullptr;
	if(info->type == Q_SCRIPTED)
	{
		Quest_Scripted* quest = new Quest_Scripted;
		quest->Init(info->scheme);
		return quest;
	}
	else
		return CreateQuest(info->type);
}

//=================================================================================================
QuestInfo* QuestManager::GetRandomQuest(QuestCategory category)
{
	if(force == Q_FORCE_NONE)
	{
		if(category == QuestCategory::Random)
		{
			Warn("Can't force none random quest - unsupported.");
			force = Q_FORCE_DISABLED;
		}
		else
		{
			Info("Forcing NONE quest.");
			force = Q_FORCE_DISABLED;
			return nullptr;
		}
	}

	if(force != Q_FORCE_DISABLED)
	{
		QuestInfo& info = infos[force];
		if(info.category != category)
		{
			Warn("Can't force quest '%s', invalid quest category.", info.name);
			force = Q_FORCE_DISABLED;
		}
		else
		{
			Info("Forcing '%s' quest.", info.name);
			force = Q_FORCE_DISABLED;
			return &info;
		}
	}

	QuestList* list;
	switch(category)
	{
	case QuestCategory::Mayor:
		list = quests_mayor;
		break;
	case QuestCategory::Captain:
		list = quests_captain;
		break;
	case QuestCategory::Random:
		list = quests_random;
		break;
	default:
		assert(0);
		return nullptr;
	}

	return list->GetRandom();
}

//=================================================================================================
Quest* QuestManager::GetMayorQuest()
{
	return CreateQuest(GetRandomQuest(QuestCategory::Mayor));
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest()
{
	return CreateQuest(GetRandomQuest(QuestCategory::Captain));
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest()
{
	return CreateQuest(GetRandomQuest(QuestCategory::Random));
}

//=================================================================================================
void QuestManager::AddQuestItemRequest(const Item** item, cstring name, int quest_id, vector<ItemSlot>* items, Unit* unit)
{
	assert(item && name && quest_id != -1);
	QuestItemRequest* q = new QuestItemRequest;
	q->item = item;
	q->name = name;
	q->quest_id = quest_id;
	q->items = items;
	q->unit = unit;
	quest_item_requests.push_back(q);
}

//=================================================================================================
void QuestManager::Reset()
{
	force = Q_FORCE_DISABLED;
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	quest_requests.clear();
	DeleteElements(quest_item_requests);
	quests_timeout.clear();
	quests_timeout2.clear();
	quest_counter = 0;
	unique_quests_completed = 0;
	unique_completed_show = false;
	quest_rumors.clear();
}

//=================================================================================================
void QuestManager::Update(int days)
{
	// mark quest locations as not quest / remove quest camps
	LoopAndRemove(quests_timeout, [this](Quest_Dungeon* quest)
	{
		if(!quest->IsTimedout())
			return false;

		Location* loc = world->GetLocation(quest->target_loc);
		bool in_camp = false;

		if(loc->type == L_CAMP && (quest->target_loc == world->GetTravelLocationIndex() || quest->target_loc == world->GetCurrentLocationIndex()))
			in_camp = true;

		if(!quest->timeout)
		{
			bool ok = quest->OnTimeout(in_camp ? TIMEOUT_CAMP : TIMEOUT_NORMAL);
			if(ok)
				quest->timeout = true;
			else
				return false;
		}

		if(in_camp)
			return false;

		loc->active_quest = nullptr;

		if(loc->type == L_CAMP)
		{
			quest->target_loc = -1;
			world->DeleteCamp((Camp*)loc);
		}

		return true;
	});

	// quest timeouts, not attached to location
	LoopAndRemove(quests_timeout2, [](Quest* quest)
	{
		if(quest->IsTimedout() && quest->OnTimeout(TIMEOUT2))
		{
			quest->timeout = true;
			return true;
		}
		return false;
	});

	// update contest
	if(quest_contest->year != world->GetYear())
	{
		quest_contest->year = world->GetYear();
		quest_contest->where = world->GetRandomSettlementIndex(quest_contest->where);
	}
}

//=================================================================================================
void QuestManager::Write(BitStreamWriter& f)
{
	// quests
	f.WriteCasted<word>(quests.size());
	for(Quest* quest : quests)
	{
		f << quest->id;
		f.WriteCasted<byte>(quest->state);
		f << quest->name;
		f.WriteStringArray<byte, word>(quest->msgs);
	}

	// quest items
	// temporary fix for crash
	LoopAndRemove(Net::changes, [](NetChange& c)
	{
		if(c.type == NetChange::REGISTER_ITEM)
			return false;
		game->ReportError(13, Format("QuestManager write invalid change %d.", c.type));
		return true;
	});
	f.WriteCasted<word>(Net::changes.size());
	for(NetChange& c : Net::changes)
	{
		f << c.base_item->id;
		f << c.item2->id;
		f << c.item2->name;
		f << c.item2->desc;
		f << c.item2->quest_id;
	}
	Net::changes.clear();
}

//=================================================================================================
bool QuestManager::Read(BitStreamReader& f)
{
	// quests
	const int QUEST_MIN_SIZE = sizeof(int) + sizeof(byte) * 3;
	word quest_count;
	f >> quest_count;
	if(!f.Ensure(QUEST_MIN_SIZE * quest_count))
	{
		Error("Read world: Broken packet for quests.");
		return false;
	}
	quests.resize(quest_count);

	int index = 0;
	for(Quest*& quest : quests)
	{
		quest = new PlaceholderQuest;
		quest->quest_index = index;
		f >> quest->id;
		f.ReadCasted<byte>(quest->state);
		f >> quest->name;
		f.ReadStringArray<byte, word>(quest->msgs);
		if(!f)
		{
			Error("Read world: Broken packet for quest %d.", index);
			return false;
		}
		++index;
	}

	// quest items
	const int QUEST_ITEM_MIN_SIZE = 7;
	word quest_items_count;
	f >> quest_items_count;
	if(!f.Ensure(QUEST_ITEM_MIN_SIZE * quest_items_count))
	{
		Error("Read world: Broken packet for quest items.");
		return false;
	}
	quest_items.reserve(quest_items_count);
	for(word i = 0; i < quest_items_count; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read world: Broken packet for quest item %u.", i);
			return false;
		}

		const Item* base_item;
		if(item_id[0] == '$')
			base_item = Item::TryGet(item_id.c_str() + 1);
		else
			base_item = Item::TryGet(item_id);
		if(!base_item)
		{
			Error("Read world: Missing quest item '%s' (%u).", item_id.c_str(), i);
			return false;
		}

		Item* item = base_item->CreateCopy();
		f >> item->id;
		f >> item->name;
		f >> item->desc;
		f >> item->quest_id;
		if(!f)
		{
			Error("Read world: Broken packet for quest item %u (2).", i);
			delete item;
			return false;
		}
		else
			quest_items.push_back(item);
	}

	return true;
}

//=================================================================================================
void QuestManager::Save(GameWriter& f)
{
	f << quests.size();
	for(Quest* quest : quests)
		quest->Save(f);

	f << unaccepted_quests.size();
	for(Quest* quest : unaccepted_quests)
		quest->Save(f);

	f << quests_timeout.size();
	for(Quest_Dungeon* q : quests_timeout)
		f << q->id;

	f << quests_timeout2.size();
	for(Quest* q : quests_timeout2)
		f << q->id;

	f << quest_items.size();
	for(Item* item : quest_items)
	{
		f << item->id;
		f << item->quest_id;
		f << item->name;
	}

	f << quest_counter;
	f << unique_quests_completed;
	f << unique_completed_show;
	f << quest_rumors.size();
	for(pair<int, string>& rumor : quest_rumors)
	{
		f << rumor.first;
		f << rumor.second;
	}
	if(force == Q_FORCE_DISABLED)
		f.Write0();
	else if(force == Q_FORCE_NONE)
		f << "none";
	else
		f << infos[force].name;

	quest_secret->Save(f);
	quest_contest->Save(f);
	quest_tournament->Save(f);
}

//=================================================================================================
void QuestManager::Load(GameReader& f)
{
	upgrade_quests.clear();

	LoadQuests(f, quests);
	LoadQuests(f, unaccepted_quests);

	// get quest pointers
	quest_sawmill = static_cast<Quest_Sawmill*>(FindQuest(Q_SAWMILL));
	quest_mine = static_cast<Quest_Mine*>(FindQuest(Q_MINE));
	quest_bandits = static_cast<Quest_Bandits*>(FindQuest(Q_BANDITS));
	quest_bandits->Init();
	quest_goblins = static_cast<Quest_Goblins*>(FindQuest(Q_GOBLINS));
	quest_goblins->Init();
	quest_mages = static_cast<Quest_Mages*>(FindQuest(Q_MAGES));
	quest_mages2 = static_cast<Quest_Mages2*>(FindQuest(Q_MAGES2));
	quest_orcs = static_cast<Quest_Orcs*>(FindQuest(Q_ORCS));
	quest_orcs->Init();
	quest_orcs2 = static_cast<Quest_Orcs2*>(FindQuest(Q_ORCS2));
	quest_orcs2->Init();
	quest_evil = static_cast<Quest_Evil*>(FindQuest(Q_EVIL));
	quest_evil->Init();
	quest_crazies = static_cast<Quest_Crazies*>(FindQuest(Q_CRAZIES));
	quest_crazies->Init();
	quest_artifacts = static_cast<Quest_Artifacts*>(FindQuest(Q_ARTIFACTS));
	if(LOAD_VERSION < V_0_8 && !quest_mages2)
	{
		quest_mages2 = new Quest_Mages2;
		quest_mages2->id = quest_counter++;
		quest_mages2->Start();
		unaccepted_quests.push_back(quest_mages2);
	}
	quest_mages2->Init();

	// quest timeouts
	quests_timeout.resize(f.Read<uint>());
	for(Quest_Dungeon*& q : quests_timeout)
		q = static_cast<Quest_Dungeon*>(FindQuest(f.Read<uint>(), false));
	quests_timeout2.resize(f.Read<uint>());
	for(Quest*& q : quests_timeout2)
		q = FindQuest(f.Read<uint>(), false);

	// quest rumors
	if(LOAD_VERSION >= V_0_9)
	{
		quest_items.resize(f.Read<uint>());
		for(Item*& item : quest_items)
		{
			const string& id = f.ReadString1();
			Item* base = Item::Get(id.c_str() + 1);
			item = base->CreateCopy();
			item->id = id;
			f >> item->quest_id;
			f >> item->name;
		}
	}

	f >> quest_counter;
	f >> unique_quests_completed;
	f >> unique_completed_show;
	if(LOAD_VERSION >= V_0_10)
	{
		uint count;
		f >> count;
		quest_rumors.resize(count);
		for(pair<int, string>& rumor : quest_rumors)
		{
			f >> rumor.first;
			f >> rumor.second;
		}
	}
	else
	{
		f.Skip<int>(); // quest_rumor_counter
		bool rumors[old::R_MAX];
		f >> rumors;
		for(int i = 0; i < old::R_MAX; ++i)
		{
			if(rumors[i])
				continue;
			cstring text;
			QUEST_TYPE type;
			switch((old::QUEST_RUMOR)i)
			{
			case old::R_SAWMILL:
				type = Q_SAWMILL;
				text = Format(txRumorQ[0], world->GetLocation(quest_sawmill->start_loc)->name.c_str());
				break;
			case old::R_MINE:
				type = Q_MINE;
				text = Format(txRumorQ[1], world->GetLocation(quest_mine->start_loc)->name.c_str());
				break;
			case old::R_CONTEST:
				type = Q_FORCE_NONE;
				text = txRumorQ[2];
				break;
			case old::R_BANDITS:
				type = Q_BANDITS;
				text = Format(txRumorQ[3], world->GetLocation(quest_bandits->start_loc)->name.c_str());
				break;
			case old::R_MAGES:
				type = Q_MAGES;
				text = Format(txRumorQ[4], world->GetLocation(quest_mages->start_loc)->name.c_str());
				break;
			case old::R_MAGES2:
				type = Q_MAGES2;
				text = txRumorQ[5];
				break;
			case old::R_ORCS:
				type = Q_ORCS;
				text = Format(txRumorQ[6], world->GetLocation(quest_orcs->start_loc)->name.c_str());
				break;
			case old::R_GOBLINS:
				type = Q_GOBLINS;
				text = Format(txRumorQ[7], world->GetLocation(quest_goblins->start_loc)->name.c_str());
				break;
			case old::R_EVIL:
				type = Q_EVIL;
				text = Format(txRumorQ[8], world->GetLocation(quest_evil->start_loc)->name.c_str());
				break;
			}
			if(type == Q_FORCE_NONE)
			{
				quest_contest->rumor = quest_counter++;
				quest_rumors.push_back(pair<int, string>(quest_contest->rumor, text));
			}
			else
			{
				Quest* quest = FindQuest(type);
				quest_rumors.push_back(pair<int, string>(quest->id, text));
			}
		}
	}

	// force quest
	if(LOAD_VERSION >= V_0_9)
	{
		const string& force_id = f.ReadString1();
		if(force_id.empty())
			force = Q_FORCE_DISABLED;
		else
			SetForcedQuest(force_id);
	}
	else
		f >> force;

	// process quest requests
	for(QuestRequest& request : quest_requests)
	{
		*request.quest = FindAnyQuest(request.id);
		if(request.callback)
			request.callback();
	}
	quest_requests.clear();

	// process quest item requests
	for(vector<QuestItemRequest*>::iterator it = quest_item_requests.begin(), end = quest_item_requests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		*qir->item = FindQuestItem(qir->name.c_str(), qir->quest_id);
		if(qir->items)
		{
			bool ok = true;
			for(vector<ItemSlot>::iterator it2 = qir->items->begin(), end2 = qir->items->end(); it2 != end2; ++it2)
			{
				if(it2->item == QUEST_ITEM_PLACEHOLDER)
				{
					ok = false;
					break;
				}
			}
			if(ok && (LOAD_VERSION < V_0_7_1 || content.require_update))
			{
				SortItems(*qir->items);
				if(qir->unit)
					qir->unit->RecalculateWeight();
			}
		}
		delete *it;
	}
	quest_item_requests.clear();

	// load pseudo-quests
	quest_secret->Load(f);
	quest_contest->Load(f);
	quest_tournament->Load(f);
}

//=================================================================================================
void QuestManager::LoadQuests(GameReader& f, vector<Quest*>& quests)
{
	uint count = f.Read<uint>();
	quests.resize(count, nullptr);

	uint quest_index = 0;
	for(uint i = 0; i < count; ++i)
	{
		QUEST_TYPE type;
		f >> type;

		Quest* quest = CreateQuest(type);
		quest->type = type;
		quest->quest_index = quest_index;
		Quest::LoadResult result = quest->Load(f);
		if(result == Quest::LoadResult::Remove)
		{
			delete quest;
			continue;
		}
		else if(result == Quest::LoadResult::Convert)
			upgrade_quests.push_back(quest);

		quests[quest_index] = quest;
		++quest_index;
	}

	while(!quests.empty() && !quests.back())
		quests.pop_back();
}

//=================================================================================================
Quest* QuestManager::FindQuest(int id, bool active)
{
	for(Quest* quest : quests)
	{
		if((!active || quest->IsActive()) && quest->id == id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuest(int loc, QuestCategory category)
{
	for(Quest* quest : quests)
	{
		if(quest->start_loc == loc && quest->category == category)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindAnyQuest(int id)
{
	for(Quest* quest : quests)
	{
		if(quest->id == id)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->id == id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindAnyQuest(QuestScheme* scheme)
{
	assert(scheme);

	for(Quest* quest : quests)
	{
		if(quest->type == Q_SCRIPTED && static_cast<Quest_Scripted*>(quest)->GetScheme() == scheme)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->type == Q_SCRIPTED && static_cast<Quest_Scripted*>(quest)->GetScheme() == scheme)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuest(QUEST_TYPE type)
{
	for(Quest* quest : quests)
	{
		if(quest->type == type)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int loc, QuestCategory category)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->start_loc == loc && quest->category == category)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int id)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->id == id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
const Item* QuestManager::FindQuestItem(cstring item_id, int quest_id)
{
	for(Item* item : quest_items)
	{
		if(item->quest_id == quest_id && item->id == item_id)
			return item;
	}

	for(Quest* quest : quests)
	{
		if(quest->id == quest_id)
			return quest->GetQuestItem();
	}

	assert(0);
	return nullptr;
}

//=================================================================================================
bool QuestManager::SetForcedQuest(const string& name)
{
	if(name == "reset")
	{
		force = Q_FORCE_DISABLED;
		return true;
	}
	if(name == "none")
	{
		force = Q_FORCE_NONE;
		return true;
	}

	int index = 0;
	for(QuestInfo& info : infos)
	{
		if(name == info.name)
		{
			force = index;
			return true;
		}
		++index;
	}

	return false;
}

//=================================================================================================
bool QuestManager::HandleSpecial(DialogContext& ctx, cstring msg, bool& result)
{
	std::map<string, QuestHandler*>::iterator it;
	cstring slash = strrchr(msg, '/');
	if(slash)
	{
		tmp_str = string(msg, slash - msg);
		it = special_handlers.find(tmp_str);
	}
	else
		it = special_handlers.find(msg);
	if(it == special_handlers.end())
		return false;
	result = it->second->Special(ctx, msg);
	return true;
}

//=================================================================================================
bool QuestManager::HandleSpecialIf(DialogContext& ctx, cstring msg, bool& result)
{
	std::map<string, QuestHandler*>::iterator it;
	cstring slash = strrchr(msg, '/');
	if(slash)
	{
		tmp_str = string(msg, slash - msg);
		it = special_if_handlers.find(tmp_str);
	}
	else
		it = special_if_handlers.find(msg);
	if(it == special_if_handlers.end())
		return false;
	result = it->second->SpecialIf(ctx, msg);
	return true;
}

//=================================================================================================
bool QuestManager::HandleFormatString(const string& str, cstring& result)
{
	auto it = format_str_handlers.find(str);
	if(it == format_str_handlers.end())
		return false;
	result = it->second->FormatString(str);
	assert(result);
	return true;
}

//=================================================================================================
const Item* QuestManager::FindQuestItemClient(cstring item_id, int quest_id) const
{
	assert(item_id);

	for(Item* item : quest_items)
	{
		if(item->id == item_id && (quest_id == -1 || item->IsQuest(quest_id)))
			return item;
	}

	return nullptr;
}

//=================================================================================================
void QuestManager::AddScriptedQuest(QuestScheme* scheme)
{
	assert(scheme);
	infos.push_back(QuestInfo(Q_SCRIPTED, scheme->category, scheme->id.c_str(), scheme));
}

//=================================================================================================
QuestInfo* QuestManager::FindQuest(const string& id)
{
	for(QuestInfo& info : infos)
	{
		if(id == info.name)
			return &info;
	}
	return nullptr;
}

//=================================================================================================
int QuestManager::AddQuestRumor(cstring str)
{
	int id = quest_counter++;
	quest_rumors.push_back(pair<int, string>(id, str));
	return id;
}

//=================================================================================================
bool QuestManager::RemoveQuestRumor(int id)
{
	for(uint i = 0; i < quest_rumors.size(); ++i)
	{
		if(quest_rumors[i].first == id)
		{
			RemoveElementIndex(quest_rumors, i);
			return true;
		}
	}
	return false;
}

//=================================================================================================
string QuestManager::GetRandomQuestRumor()
{
	uint index = Rand() % quest_rumors.size();
	string str = quest_rumors[index].second;
	RemoveElementIndex(quest_rumors, index);
	return str;
}

//=================================================================================================
void QuestManager::GenerateQuestUnits(bool on_enter)
{
	if(on_enter)
	{
		if(quest_sawmill->sawmill_state == Quest_Sawmill::State::None && game_level->location_index == quest_sawmill->start_loc)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("artur_drwal"), -2);
			assert(u);
			if(u)
			{
				quest_sawmill->sawmill_state = Quest_Sawmill::State::GeneratedUnit;
				quest_sawmill->hd_lumberjack.Get(*u->human_data);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_mine->start_loc && quest_mine->mine_state == Quest_Mine::State::None)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("inwestor"), -2);
			assert(u);
			if(u)
			{
				quest_mine->mine_state = Quest_Mine::State::SpawnedInvestor;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_bandits->start_loc && quest_bandits->bandits_state == Quest_Bandits::State::None)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("mistrz_agentow"), -2);
			assert(u);
			if(u)
			{
				quest_bandits->bandits_state = Quest_Bandits::State::GeneratedMaster;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_mages->start_loc && quest_mages2->mages_state == Quest_Mages2::State::None)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_magowie_uczony"), -2);
			assert(u);
			if(u)
			{
				quest_mages2->mages_state = Quest_Mages2::State::GeneratedScholar;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_mages2->mage_loc)
		{
			if(quest_mages2->mages_state == Quest_Mages2::State::TalkedWithCaptain)
			{
				Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
				assert(u);
				if(u)
				{
					quest_mages2->mages_state = Quest_Mages2::State::GeneratedOldMage;
					quest_mages2->good_mage_name = u->hero->name;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
			else if(quest_mages2->mages_state == Quest_Mages2::State::MageLeft)
			{
				Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
				assert(u);
				if(u)
				{
					quest_mages2->scholar = u;
					u->hero->know_name = true;
					u->hero->name = quest_mages2->good_mage_name;
					u->ApplyHumanData(quest_mages2->hd_mage);
					quest_mages2->mages_state = Quest_Mages2::State::MageGeneratedInCity;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
		}

		if(game_level->location_index == quest_orcs->start_loc && quest_orcs2->orcs_state == Quest_Orcs2::State::None)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_orkowie_straznik"));
			assert(u);
			if(u)
			{
				u->OrderAutoTalk();
				quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedGuard;
				quest_orcs2->guard = u;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_goblins->start_loc && quest_goblins->goblins_state == Quest_Goblins::State::None)
		{
			Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_gobliny_szlachcic"));
			assert(u);
			if(u)
			{
				quest_goblins->nobleman = u;
				quest_goblins->hd_nobleman.Get(*u->human_data);
				quest_goblins->goblins_state = Quest_Goblins::State::GeneratedNobleman;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(game_level->location_index == quest_evil->start_loc && quest_evil->evil_state == Quest_Evil::State::None)
		{
			CityBuilding* b = game_level->city_ctx->FindBuilding(BuildingGroup::BG_INN);
			Unit* u = game_level->SpawnUnitNearLocation(*game_level->local_area, b->walk_pt, *UnitData::Get("q_zlo_kaplan"), nullptr, 10);
			assert(u);
			if(u)
			{
				u->OrderAutoTalk();
				quest_evil->cleric = u;
				quest_evil->evil_state = Quest_Evil::State::GeneratedCleric;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(!team->is_bandit)
		{
			// sawmill quest
			if(quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
			{
				if(quest_sawmill->days >= 30 && game_level->city_ctx)
				{
					quest_sawmill->days = 29;
					Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("poslaniec_tartak"), &team->leader->pos, -2, 2.f);
					if(u)
					{
						quest_sawmill->messenger = u;
						u->OrderAutoTalk(true);
					}
				}
			}
			else if(quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
			{
				int count = quest_sawmill->days / 30;
				if(count)
				{
					quest_sawmill->days -= count * 30;
					team->AddGold(count * Quest_Sawmill::PAYMENT, nullptr, true);
				}
			}

			if(quest_mine->days >= quest_mine->days_required
				&& ((quest_mine->mine_state2 == Quest_Mine::State2::InBuild && quest_mine->mine_state == Quest_Mine::State::Shares) // inform player about building mine & give gold
				|| quest_mine->mine_state2 == Quest_Mine::State2::Built // inform player about possible investment
				|| quest_mine->mine_state2 == Quest_Mine::State2::InExpand // inform player about finished mine expanding
				|| quest_mine->mine_state2 == Quest_Mine::State2::Expanded)) // inform player about finding portal
			{
				Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("poslaniec_kopalnia"), &team->leader->pos, -2, 2.f);
				if(u)
				{
					quest_mine->messenger = u;
					u->OrderAutoTalk(true);
				}
			}

			if(quest_evil->evil_state == Quest_Evil::State::GenerateMage && game_level->location_index == quest_evil->mage_loc)
			{
				Unit* u = game_level->SpawnUnitInsideInn(*UnitData::Get("q_zlo_mag"), -2);
				assert(u);
				if(u)
				{
					quest_evil->evil_state = Quest_Evil::State::GeneratedMage;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
		}

		if(world->GetDay() == 6 && world->GetMonth() == 2 && game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveArena)
			&& game_level->location_index == quest_tournament->GetCity() && !quest_tournament->IsGenerated())
			quest_tournament->GenerateUnits();
	}

	// spawn on enter or update
	if(!team->is_bandit)
	{
		if(quest_goblins->goblins_state == Quest_Goblins::State::Counting && quest_goblins->days <= 0)
		{
			Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("q_gobliny_poslaniec"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				quest_goblins->messenger = u;
				u->OrderAutoTalk(true);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft && quest_goblins->days <= 0)
		{
			Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("q_gobliny_mag"), &team->leader->pos, 5, 2.f);
			if(u)
			{
				quest_goblins->messenger = u;
				quest_goblins->goblins_state = Quest_Goblins::State::GeneratedMage;
				u->OrderAutoTalk(true);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}
	}
}

//=================================================================================================
void QuestManager::UpdateQuests(int days)
{
	if(team->is_bandit)
		return;

	RemoveQuestUnits(false);

	int income = 0;

	// sawmill
	if(quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
	{
		quest_sawmill->days += days;
		if(quest_sawmill->days >= 30 && game_level->city_ctx && game->game_state == GS_LEVEL)
		{
			quest_sawmill->days = 29;
			Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("poslaniec_tartak"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				quest_sawmill->messenger = u;
				u->OrderAutoTalk(true);
			}
		}
	}
	else if(quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		quest_sawmill->days += days;
		int count = quest_sawmill->days / 30;
		if(count)
		{
			quest_sawmill->days -= count * 30;
			income += count * Quest_Sawmill::PAYMENT;
		}
	}

	// mine
	if(quest_mine->mine_state2 == Quest_Mine::State2::InBuild)
	{
		quest_mine->days += days;
		if(quest_mine->days >= quest_mine->days_required)
		{
			if(quest_mine->mine_state == Quest_Mine::State::Shares)
			{
				// player invesetd in mine, inform him about finishing
				if(game_level->city_ctx && game->game_state == GS_LEVEL)
				{
					Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("poslaniec_kopalnia"), &team->leader->pos, -2, 2.f);
					if(u)
					{
						world->AddNews(Format(game->txMineBuilt, world->GetLocation(quest_mine->target_loc)->name.c_str()));
						quest_mine->messenger = u;
						u->OrderAutoTalk(true);
					}
				}
			}
			else
			{
				// player got gold, don't inform him
				world->AddNews(Format(game->txMineBuilt, world->GetLocation(quest_mine->target_loc)->name.c_str()));
				quest_mine->mine_state2 = Quest_Mine::State2::Built;
				quest_mine->days -= quest_mine->days_required;
				quest_mine->days_required = Random(60, 90);
				if(quest_mine->days >= quest_mine->days_required)
					quest_mine->days = quest_mine->days_required - 1;
			}
		}
	}
	else if(quest_mine->mine_state2 == Quest_Mine::State2::Built
		|| quest_mine->mine_state2 == Quest_Mine::State2::InExpand
		|| quest_mine->mine_state2 == Quest_Mine::State2::Expanded)
	{
		// mine is built/in expand/expanded
		// count time to news about expanding/finished expanding/found portal
		quest_mine->days += days;
		if(quest_mine->days >= quest_mine->days_required && game_level->city_ctx && game->game_state == GS_LEVEL)
		{
			Unit* u = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("poslaniec_kopalnia"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				quest_mine->messenger = u;
				u->OrderAutoTalk(true);
			}
		}
	}

	// give gold from mine
	income += quest_mine->GetIncome(days);

	if(income != 0)
		team->AddGold(income, nullptr, true);

	quest_contest->Progress();

	//----------------------------
	// mages
	if(quest_mages2->mages_state == Quest_Mages2::State::Counting)
	{
		quest_mages2->days -= days;
		if(quest_mages2->days <= 0)
		{
			// from now golem can be encountered on road
			quest_mages2->mages_state = Quest_Mages2::State::Encounter;
		}
	}

	// orcs
	if(Any(quest_orcs2->orcs_state, Quest_Orcs2::State::CompletedJoined, Quest_Orcs2::State::CampCleared, Quest_Orcs2::State::PickedClass))
	{
		quest_orcs2->days -= days;
		if(quest_orcs2->days <= 0)
			quest_orcs2->orc->OrderAutoTalk();
	}

	// goblins
	if(quest_goblins->goblins_state == Quest_Goblins::State::Counting || quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft)
		quest_goblins->days -= days;

	// crazies
	if(quest_crazies->crazies_state == Quest_Crazies::State::PickedStone)
		quest_crazies->days -= days;

	quest_tournament->Progress();

	if(game_level->city_ctx)
		GenerateQuestUnits(false);
}

//=================================================================================================
void QuestManager::RemoveQuestUnits(bool on_leave)
{
	if(game_level->city_ctx)
	{
		if(quest_sawmill->messenger)
		{
			game_level->RemoveUnit(UnitData::Get("poslaniec_tartak"), on_leave);
			quest_sawmill->messenger = nullptr;
		}

		if(quest_mine->messenger)
		{
			game_level->RemoveUnit(UnitData::Get("poslaniec_kopalnia"), on_leave);
			quest_mine->messenger = nullptr;
		}

		if(game_level->is_open && game_level->location_index == quest_sawmill->start_loc && quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
			&& quest_sawmill->build_state == Quest_Sawmill::BuildState::None)
		{
			Unit* u = game_level->city_ctx->FindInn()->FindUnit(UnitData::Get("artur_drwal"));
			if(u && u->IsAlive())
			{
				quest_sawmill->build_state = Quest_Sawmill::BuildState::LumberjackLeft;
				game_level->RemoveUnit(u, !on_leave);
			}
		}

		if(quest_mages2->scholar && quest_mages2->mages_state == Quest_Mages2::State::ScholarWaits)
		{
			game_level->RemoveUnit(UnitData::Get("q_magowie_uczony"), on_leave);
			quest_mages2->scholar = nullptr;
			quest_mages2->mages_state = Quest_Mages2::State::Counting;
			quest_mages2->days = Random(15, 30);
		}

		if(quest_orcs2->guard && quest_orcs2->orcs_state >= Quest_Orcs2::State::GuardTalked)
		{
			game_level->RemoveUnit(UnitData::Get("q_orkowie_straznik"), on_leave);
			quest_orcs2->guard = nullptr;
		}
	}

	if(quest_bandits->bandits_state == Quest_Bandits::State::AgentTalked)
	{
		quest_bandits->bandits_state = Quest_Bandits::State::AgentLeft;
		quest_bandits->agent = nullptr;
	}

	if(quest_mages2->mages_state == Quest_Mages2::State::MageLeaving)
	{
		quest_mages2->mages_state = Quest_Mages2::State::MageLeft;
		quest_mages2->scholar = nullptr;
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::MessengerTalked && quest_goblins->messenger)
	{
		game_level->RemoveUnit(UnitData::Get("q_gobliny_poslaniec"), on_leave);
		quest_goblins->messenger = nullptr;
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::GivenBow && quest_goblins->nobleman)
	{
		game_level->RemoveUnit(UnitData::Get("q_gobliny_szlachcic"), on_leave);
		quest_goblins->nobleman = nullptr;
		quest_goblins->goblins_state = Quest_Goblins::State::NoblemanLeft;
		quest_goblins->days = Random(15, 30);
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::MageTalked && quest_goblins->messenger)
	{
		game_level->RemoveUnit(UnitData::Get("q_gobliny_mag"), on_leave);
		quest_goblins->messenger = nullptr;
		quest_goblins->goblins_state = Quest_Goblins::State::MageLeft;
	}

	if(quest_evil->evil_state == Quest_Evil::State::ClericLeaving)
	{
		quest_evil->cleric = nullptr;
		quest_evil->evil_state = Quest_Evil::State::ClericLeft;
	}
}

//=================================================================================================
void QuestManager::HandleQuestEvent(Quest_Event* event)
{
	assert(event);

	event->done = true;

	Unit* spawned = nullptr, *spawned2 = nullptr;
	InsideLocationLevel* lvl = nullptr;
	InsideLocation* inside = nullptr;
	if(game_level->local_area->area_type == LevelArea::Type::Inside)
	{
		inside = (InsideLocation*)game_level->location;
		lvl = &inside->GetLevelData();
	}

	// spawn unit
	if(event->unit_to_spawn)
	{
		if(game_level->local_area->area_type == LevelArea::Type::Outside)
		{
			if(game_level->location->type == L_CITY)
				spawned = game_level->SpawnUnitInsideInn(*event->unit_to_spawn, event->unit_spawn_level);
			else
			{
				Vec3 pos(0, 0, 0);
				int count = 0;
				for(Unit* unit : game_level->local_area->units)
				{
					pos += unit->pos;
					++count;
				}
				pos /= (float)count;
				spawned = game_level->SpawnUnitNearLocation(*game_level->local_area, pos, *event->unit_to_spawn, nullptr, event->unit_spawn_level);
			}
		}
		else
		{
			Room& room = lvl->GetRoom(event->spawn_unit_room, inside->HaveDownStairs());
			spawned = game_level->SpawnUnitInsideRoomOrNear(*lvl, room, *event->unit_to_spawn, event->unit_spawn_level);
		}
		if(!spawned)
			throw "Failed to spawn quest unit!";
		spawned->dont_attack = event->unit_dont_attack;
		if(event->unit_auto_talk)
			spawned->OrderAutoTalk();
		spawned->event_handler = event->unit_event_handler;
		if(spawned->event_handler && event->send_spawn_event)
			spawned->event_handler->HandleUnitEvent(UnitEventHandler::SPAWN, spawned);
		if(game->devmode)
			Info("Generated unit %s (%g,%g).", event->unit_to_spawn->id.c_str(), spawned->pos.x, spawned->pos.z);

		// mark near units as guards if guarded (only in dungeon)
		if(IsSet(spawned->data->flags2, F2_GUARDED) && lvl)
		{
			Room& room = lvl->GetRoom(event->spawn_unit_room, inside->HaveDownStairs());
			for(Unit* unit : game_level->local_area->units)
			{
				if(unit != spawned && unit->IsFriend(*spawned) && lvl->GetRoom(PosToPt(unit->pos)) == &room)
				{
					unit->dont_attack = spawned->dont_attack;
					unit->OrderGuard(spawned);
				}
			}
		}
	}

	// spawn second units (only in dungeon)
	if(event->unit_to_spawn2 && lvl)
	{
		Room* room;
		if(spawned && event->spawn_2_guard_1)
			room = lvl->GetRoom(PosToPt(spawned->pos));
		else
			room = &lvl->GetRoom(event->spawn_unit_room2, inside->HaveDownStairs());
		spawned2 = game_level->SpawnUnitInsideRoomOrNear(*lvl, *room, *event->unit_to_spawn2, event->unit_spawn_level2);
		if(!spawned2)
			throw "Failed to spawn quest unit 2!";
		if(game->devmode)
			Info("Generated unit %s (%g,%g).", event->unit_to_spawn2->id.c_str(), spawned2->pos.x, spawned2->pos.z);
		if(spawned && event->spawn_2_guard_1)
		{
			spawned2->dont_attack = spawned->dont_attack;
			spawned2->OrderGuard(spawned);
		}
	}

	// spawn item
	switch(event->spawn_item)
	{
	case Quest_Dungeon::Item_DontSpawn:
		break;
	case Quest_Dungeon::Item_GiveStrongest:
		{
			Unit* best = nullptr;
			for(Unit* unit : game_level->local_area->units)
			{
				if(unit->IsAlive() && unit->IsEnemy(*game->pc->unit) && (!best || unit->level > best->level))
					best = unit;
			}
			assert(best);
			if(best)
			{
				best->AddItem(event->item_to_give[0], 1, true);
				if(game->devmode)
					Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), best->data->id.c_str(), best->pos.x, best->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned:
		assert(spawned);
		if(spawned)
		{
			spawned->AddItem(event->item_to_give[0], 1, true);
			if(game->devmode)
				Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned->data->id.c_str(), spawned->pos.x, spawned->pos.z);
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned2:
		assert(spawned2);
		if(spawned2)
		{
			spawned2->AddItem(event->item_to_give[0], 1, true);
			if(game->devmode)
				Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned2->data->id.c_str(), spawned2->pos.x, spawned2->pos.z);
		}
		break;
	case Quest_Dungeon::Item_OnGround:
		{
			GroundItem* item;
			if(lvl)
				item = game_level->SpawnGroundItemInsideAnyRoom(*lvl, event->item_to_give[0]);
			else
				item = game_level->SpawnGroundItemInsideRadius(event->item_to_give[0], Vec2(128, 128), 10.f);
			if(game->devmode)
				Info("Generated item %s on ground (%g,%g).", event->item_to_give[0]->id.c_str(), item->pos.x, item->pos.z);
		}
		break;
	case Quest_Dungeon::Item_InTreasure:
		if(inside && Any(inside->target, HERO_CRYPT, MONSTER_CRYPT, LABYRINTH))
		{
			Chest* chest = nullptr;

			if(inside->target == LABYRINTH)
				chest = RandomItem(lvl->chests);
			else
			{
				Room& room = *lvl->rooms[inside->special_room];
				LocalVector<Chest*> chests;
				for(Chest* chest2 : lvl->chests)
				{
					if(room.IsInside(chest2->pos))
						chests.push_back(chest2);
				}

				if(!chests.empty())
					chest = chests.RandomItem();
			}

			assert(chest);
			if(chest)
			{
				chest->AddItem(event->item_to_give[0]);
				if(game->devmode)
					Info("Generated item %s in treasure chest (%g,%g).", event->item_to_give[0]->id.c_str(), chest->pos.x, chest->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_InChest:
		{
			Chest* chest = game_level->local_area->GetRandomFarChest(game_level->GetSpawnPoint());
			assert(event->item_to_give[0]);
			if(game->devmode)
			{
				LocalString str = "Addded items (";
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->item_to_give[i])
						break;
					if(i > 0)
						str += ", ";
					chest->AddItem(event->item_to_give[i]);
					str += event->item_to_give[i]->id;
				}
				str += Format(") to chest (%g,%g).", chest->pos.x, chest->pos.z);
				Info(str.get_ref().c_str());
			}
			else
			{
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->item_to_give[i])
						break;
					chest->AddItem(event->item_to_give[i]);
				}
			}
			chest->handler = event->chest_event_handler;
		}
		break;
	}

	if(event->callback)
		event->callback();
}

//=================================================================================================
void QuestManager::UpdateQuestsLocal(float dt)
{
	quest_tournament->Update(dt);
	quest_contest->Update(dt);
	quest_bandits->Update(dt);
	quest_mages2->Update(dt);
	quest_evil->Update(dt);
	quest_secret->UpdateFight();
}

//=================================================================================================
void QuestManager::UpgradeQuests()
{
	for(Quest* quest : upgrade_quests)
	{
		Quest_Scripted* quest2 = new Quest_Scripted;
		quest2->quest_index = quest->quest_index;
		quest2->Upgrade(quest);
		if(quest->state == Quest::Hidden)
			unaccepted_quests[quest->quest_index] = quest2;
		else
			quests[quest->quest_index] = quest2;
		delete quest;
	}
}
