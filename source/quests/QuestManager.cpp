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

	infos.push_back(QuestInfo(Q_MINE, QuestType::Unique, "mine"));
	infos.push_back(QuestInfo(Q_SAWMILL, QuestType::Unique, "sawmill"));
	infos.push_back(QuestInfo(Q_BANDITS, QuestType::Unique, "bandits"));
	infos.push_back(QuestInfo(Q_MAGES, QuestType::Unique, "mages"));
	infos.push_back(QuestInfo(Q_MAGES2, QuestType::Unique, "mages2"));
	infos.push_back(QuestInfo(Q_ORCS, QuestType::Unique, "orcs"));
	infos.push_back(QuestInfo(Q_ORCS2, QuestType::Unique, "orcs2"));
	infos.push_back(QuestInfo(Q_GOBLINS, QuestType::Unique, "goblins"));
	infos.push_back(QuestInfo(Q_EVIL, QuestType::Unique, "evil"));
	infos.push_back(QuestInfo(Q_DELIVER_LETTER, QuestType::Mayor, "deliver_letter"));
	infos.push_back(QuestInfo(Q_DELIVER_PARCEL, QuestType::Mayor, "deliver_parcel"));
	infos.push_back(QuestInfo(Q_SPREAD_NEWS, QuestType::Mayor, "spread_news"));
	infos.push_back(QuestInfo(Q_RESCUE_CAPTIVE, QuestType::Captain, "rescue_captive"));
	infos.push_back(QuestInfo(Q_BANDITS_COLLECT_TOLL, QuestType::Captain, "bandits_collect_toll"));
	infos.push_back(QuestInfo(Q_CAMP_NEAR_CITY, QuestType::Captain, "camp_near_city"));
	infos.push_back(QuestInfo(Q_RETRIEVE_PACKAGE, QuestType::Mayor, "retrieve_package"));
	infos.push_back(QuestInfo(Q_KILL_ANIMALS, QuestType::Captain, "kill_animals"));
	infos.push_back(QuestInfo(Q_LOST_ARTIFACT, QuestType::Random, "lost_artifact"));
	infos.push_back(QuestInfo(Q_STOLEN_ARTIFACT, QuestType::Random, "stolen_artifact"));
	infos.push_back(QuestInfo(Q_FIND_ARTIFACT, QuestType::Random, "find_artifact"));
	infos.push_back(QuestInfo(Q_CRAZIES, QuestType::Unique, "crazies"));
	infos.push_back(QuestInfo(Q_WANTED, QuestType::Captain, "wanted"));
	infos.push_back({ Q_ARTIFACTS, QuestType::Unique, "artifacts" });

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
		if(scheme->type == QuestType::Unique)
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
	quest_tournament->Clear();
}

//=================================================================================================
void QuestManager::InitQuests(bool devmode)
{
	vector<int> used;

	// goblins
	quest_goblins = new Quest_Goblins;
	quest_goblins->Init();
	quest_goblins->start_loc = world->GetRandomSettlementIndex(used, 1);
	quest_goblins->refid = quest_counter++;
	quest_goblins->Start();
	unaccepted_quests.push_back(quest_goblins);
	used.push_back(quest_goblins->start_loc);

	// bandits
	quest_bandits = new Quest_Bandits;
	quest_bandits->Init();
	quest_bandits->start_loc = world->GetRandomSettlementIndex(used, 1);
	quest_bandits->refid = quest_counter++;
	quest_bandits->Start();
	unaccepted_quests.push_back(quest_bandits);
	used.push_back(quest_bandits->start_loc);

	// sawmill
	quest_sawmill = new Quest_Sawmill;
	quest_sawmill->start_loc = world->GetRandomSettlementIndex(used);
	quest_sawmill->refid = quest_counter++;
	quest_sawmill->Start();
	unaccepted_quests.push_back(quest_sawmill);
	used.push_back(quest_sawmill->start_loc);

	// mine
	quest_mine = new Quest_Mine;
	quest_mine->start_loc = world->GetRandomSettlementIndex(used);
	quest_mine->target_loc = world->GetClosestLocation(L_CAVE, world->GetLocation(quest_mine->start_loc)->pos);
	quest_mine->refid = quest_counter++;
	quest_mine->Start();
	unaccepted_quests.push_back(quest_mine);
	used.push_back(quest_mine->start_loc);

	// mages
	quest_mages = new Quest_Mages;
	quest_mages->start_loc = world->GetRandomSettlementIndex(used);
	quest_mages->refid = quest_counter++;
	quest_mages->Start();
	unaccepted_quests.push_back(quest_mages);
	used.push_back(quest_mages->start_loc);

	// mages2
	quest_mages2 = new Quest_Mages2;
	quest_mages2->Init();
	quest_mages2->refid = quest_counter++;
	quest_mages2->Start();
	unaccepted_quests.push_back(quest_mages2);

	// orcs
	quest_orcs = new Quest_Orcs;
	quest_orcs->Init();
	quest_orcs->start_loc = world->GetRandomSettlementIndex(used);
	quest_orcs->refid = quest_counter++;
	quest_orcs->Start();
	unaccepted_quests.push_back(quest_orcs);
	used.push_back(quest_orcs->start_loc);

	// orcs2
	quest_orcs2 = new Quest_Orcs2;
	quest_orcs2->Init();
	quest_orcs2->refid = quest_counter++;
	quest_orcs2->Start();
	unaccepted_quests.push_back(quest_orcs2);

	// evil
	quest_evil = new Quest_Evil;
	quest_evil->Init();
	quest_evil->start_loc = world->GetRandomSettlementIndex(used);
	quest_evil->refid = quest_counter++;
	quest_evil->Start();
	unaccepted_quests.push_back(quest_evil);
	used.push_back(quest_evil->start_loc);

	// crazies
	quest_crazies = new Quest_Crazies;
	quest_crazies->Init();
	quest_crazies->refid = quest_counter++;
	quest_crazies->Start();
	unaccepted_quests.push_back(quest_crazies);

	// artifacts
	quest_artifacts = new Quest_Artifacts;
	quest_artifacts->refid = quest_counter++;
	quest_artifacts->Start();
	unaccepted_quests.push_back(quest_artifacts);

	// pseudo quests
	quest_contest->Init();
	quest_secret->Init();
	quest_tournament->Init();

	if(devmode)
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
		if(scheme->type != QuestType::Unique)
			continue;
		Quest_Scripted* quest = new Quest_Scripted;
		quest->Init(scheme);
		unaccepted_quests.push_back(quest);
		quest->refid = quest_counter++;
		quest->Start();
	}
}

//=================================================================================================
Quest* QuestManager::CreateQuest(QUEST quest_id)
{
	switch(quest_id)
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
	if(info->id == Q_SCRIPTED)
	{
		Quest_Scripted* quest = new Quest_Scripted;
		quest->Init(info->scheme);
		return quest;
	}
	else
		return CreateQuest(info->id);
}

//=================================================================================================
QuestInfo* QuestManager::GetRandomQuest(QuestType type)
{
	if(force == Q_FORCE_NONE)
	{
		if(type == QuestType::Random)
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
		if(info.type != type)
		{
			Warn("Can't force quest '%s', invalid quest type.", info.name);
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
	switch(type)
	{
	case QuestType::Mayor:
		list = quests_mayor;
		break;
	case QuestType::Captain:
		list = quests_captain;
		break;
	case QuestType::Random:
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
	return CreateQuest(GetRandomQuest(QuestType::Mayor));
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest()
{
	return CreateQuest(GetRandomQuest(QuestType::Captain));
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest()
{
	return CreateQuest(GetRandomQuest(QuestType::Random));
}

//=================================================================================================
void QuestManager::AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit)
{
	assert(item && name && quest_refid != -1);
	QuestItemRequest* q = new QuestItemRequest;
	q->item = item;
	q->name = name;
	q->quest_refid = quest_refid;
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
		f << quest->refid;
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
		f << c.item2->refid;
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
		f >> quest->refid;
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
		f >> item->refid;
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
		f << q->refid;

	f << quests_timeout2.size();
	for(Quest* q : quests_timeout2)
		f << q->refid;

	f << quest_items.size();
	for(Item* item : quest_items)
	{
		f << item->id;
		f << item->refid;
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
	LoadQuests(f, quests);
	LoadQuests(f, unaccepted_quests);


	// get quest pointers
	quest_sawmill = static_cast<Quest_Sawmill*>(FindQuestById(Q_SAWMILL));
	quest_mine = static_cast<Quest_Mine*>(FindQuestById(Q_MINE));
	quest_bandits = static_cast<Quest_Bandits*>(FindQuestById(Q_BANDITS));
	quest_bandits->Init();
	quest_goblins = static_cast<Quest_Goblins*>(FindQuestById(Q_GOBLINS));
	quest_goblins->Init();
	quest_mages = static_cast<Quest_Mages*>(FindQuestById(Q_MAGES));
	quest_mages2 = static_cast<Quest_Mages2*>(FindQuestById(Q_MAGES2));
	quest_orcs = static_cast<Quest_Orcs*>(FindQuestById(Q_ORCS));
	quest_orcs->Init();
	quest_orcs2 = static_cast<Quest_Orcs2*>(FindQuestById(Q_ORCS2));
	quest_orcs2->Init();
	quest_evil = static_cast<Quest_Evil*>(FindQuestById(Q_EVIL));
	quest_evil->Init();
	quest_crazies = static_cast<Quest_Crazies*>(FindQuestById(Q_CRAZIES));
	quest_crazies->Init();
	quest_artifacts = static_cast<Quest_Artifacts*>(FindQuestById(Q_ARTIFACTS));
	if(LOAD_VERSION < V_0_8 && !quest_mages2)
	{
		quest_mages2 = new Quest_Mages2;
		quest_mages2->refid = quest_counter++;
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
			f >> item->refid;
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
			QUEST id;
			switch((old::QUEST_RUMOR)i)
			{
			case old::R_SAWMILL:
				id = Q_SAWMILL;
				text = Format(txRumorQ[0], world->GetLocation(quest_sawmill->start_loc)->name.c_str());
				break;
			case old::R_MINE:
				id = Q_MINE;
				text = Format(txRumorQ[1], world->GetLocation(quest_mine->start_loc)->name.c_str());
				break;
			case old::R_CONTEST:
				id = Q_FORCE_NONE;
				text = txRumorQ[2];
				break;
			case old::R_BANDITS:
				id = Q_BANDITS;
				text = Format(txRumorQ[3], world->GetLocation(quest_bandits->start_loc)->name.c_str());
				break;
			case old::R_MAGES:
				id = Q_MAGES;
				text = Format(txRumorQ[4], world->GetLocation(quest_mages->start_loc)->name.c_str());
				break;
			case old::R_MAGES2:
				id = Q_MAGES2;
				text = txRumorQ[5];
				break;
			case old::R_ORCS:
				id = Q_ORCS;
				text = Format(txRumorQ[6], world->GetLocation(quest_orcs->start_loc)->name.c_str());
				break;
			case old::R_GOBLINS:
				id = Q_GOBLINS;
				text = Format(txRumorQ[7], world->GetLocation(quest_goblins->start_loc)->name.c_str());
				break;
			case old::R_EVIL:
				id = Q_EVIL;
				text = Format(txRumorQ[8], world->GetLocation(quest_evil->start_loc)->name.c_str());
				break;
			}
			if(id == Q_FORCE_NONE)
			{
				quest_contest->rumor = quest_counter++;
				quest_rumors.push_back(pair<int, string>(quest_contest->rumor, text));
			}
			else
			{
				Quest* quest = FindQuestById(id);
				quest_rumors.push_back(pair<int, string>(quest->refid, text));
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
		*request.quest = FindAnyQuest(request.refid);
		if(request.callback)
			request.callback();
	}
	quest_requests.clear();

	// process quest item requests
	for(vector<QuestItemRequest*>::iterator it = quest_item_requests.begin(), end = quest_item_requests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		*qir->item = FindQuestItem(qir->name.c_str(), qir->quest_refid);
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
		QUEST quest_type;
		f >> quest_type;

		Quest* quest = CreateQuest(quest_type);
		quest->quest_id = quest_type;
		quest->quest_index = quest_index;
		if(!quest->Load(f))
		{
			delete quest;
			continue;
		}

		quests[quest_index] = quest;
		++quest_index;
	}

	while(!quests.empty() && !quests.back())
		quests.pop_back();
}

//=================================================================================================
Quest* QuestManager::FindQuest(int refid, bool active)
{
	for(Quest* quest : quests)
	{
		if((!active || quest->IsActive()) && quest->refid == refid)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuest(int loc, QuestType type)
{
	for(Quest* quest : quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindAnyQuest(int refid)
{
	for(Quest* quest : quests)
	{
		if(quest->refid == refid)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->refid == refid)
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
		if(quest->quest_id == Q_SCRIPTED && static_cast<Quest_Scripted*>(quest)->GetScheme() == scheme)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->quest_id == Q_SCRIPTED && static_cast<Quest_Scripted*>(quest)->GetScheme() == scheme)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuestById(QUEST quest_id)
{
	for(Quest* quest : quests)
	{
		if(quest->quest_id == quest_id)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->quest_id == quest_id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int loc, QuestType type)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int refid)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->refid == refid)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
const Item* QuestManager::FindQuestItem(cstring name, int refid)
{
	for(Item* item : quest_items)
	{
		if(item->refid == refid && item->id == name)
			return item;
	}

	for(Quest* quest : quests)
	{
		if(refid == quest->refid)
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
const Item* QuestManager::FindQuestItemClient(cstring id, int refid) const
{
	assert(id);

	for(Item* item : quest_items)
	{
		if(item->id == id && (refid == -1 || item->IsQuest(refid)))
			return item;
	}

	return nullptr;
}

//=================================================================================================
void QuestManager::AddScriptedQuest(QuestScheme* scheme)
{
	assert(scheme);
	infos.push_back(QuestInfo(Q_SCRIPTED, scheme->type, scheme->id.c_str(), scheme));
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
	int refid = quest_counter++;
	quest_rumors.push_back(pair<int, string>(refid, str));
	return refid;
}

//=================================================================================================
bool QuestManager::RemoveQuestRumor(int refid)
{
	for(uint i = 0; i < quest_rumors.size(); ++i)
	{
		if(quest_rumors[i].first == refid)
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
