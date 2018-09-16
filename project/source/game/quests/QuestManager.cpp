#include "Pch.h"
#include "GameCore.h"
#include "BitStreamFunc.h"
#include "QuestManager.h"
#include "SaveState.h"
#include "GameFile.h"
#include "World.h"
#include "Content.h"

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
#include "Quest_Main.h"
#include "Quest_Mine.h"
#include "Quest_Orcs.h"
#include "Quest_RescueCaptive.h"
#include "Quest_RetrievePackage.h"
#include "Quest_Sawmill.h"
#include "Quest_Secret.h"
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "Quest_Wanted.h"

//-----------------------------------------------------------------------------
QuestManager QM;

//=================================================================================================
void QuestManager::InitOnce()
{
	force = Q_FORCE_DISABLED;

	infos.push_back({ Q_MINE, QuestType::Unique, "mine" });
	infos.push_back({ Q_SAWMILL, QuestType::Unique, "sawmill" });
	infos.push_back({ Q_BANDITS, QuestType::Unique, "bandits" });
	infos.push_back({ Q_MAGES, QuestType::Unique, "mages" });
	infos.push_back({ Q_MAGES2, QuestType::Unique, "mages2" });
	infos.push_back({ Q_ORCS, QuestType::Unique, "orcs" });
	infos.push_back({ Q_ORCS2, QuestType::Unique, "orcs2" });
	infos.push_back({ Q_GOBLINS, QuestType::Unique, "goblins" });
	infos.push_back({ Q_EVIL, QuestType::Unique, "evil" });
	infos.push_back({ Q_DELIVER_LETTER, QuestType::Mayor, "deliver_letter" });
	infos.push_back({ Q_DELIVER_PARCEL, QuestType::Mayor, "deliver_parcel" });
	infos.push_back({ Q_SPREAD_NEWS, QuestType::Mayor, "spread_news" });
	infos.push_back({ Q_RESCUE_CAPTIVE, QuestType::Captain, "rescue_captive" });
	infos.push_back({ Q_BANDITS_COLLECT_TOLL, QuestType::Captain, "bandits_collect_toll" });
	infos.push_back({ Q_CAMP_NEAR_CITY, QuestType::Captain, "camp_near_city" });
	infos.push_back({ Q_RETRIEVE_PACKAGE, QuestType::Mayor, "retrieve_package" });
	infos.push_back({ Q_KILL_ANIMALS, QuestType::Captain, "kill_animals" });
	infos.push_back({ Q_LOST_ARTIFACT, QuestType::Random, "lost_artifact" });
	infos.push_back({ Q_STOLEN_ARTIFACT, QuestType::Random, "stolen_artifact" });
	infos.push_back({ Q_FIND_ARTIFACT, QuestType::Random, "find_artifact" });
	infos.push_back({ Q_CRAZIES, QuestType::Unique, "crazies" });
	infos.push_back({ Q_WANTED, QuestType::Captain, "wanted" });
	infos.push_back({ Q_MAIN, QuestType::Unique, "main" });

	// create pseudo quests
	quest_contest = new Quest_Contest;
	quest_contest->InitOnce();
	quest_secret = new Quest_Secret;
	quest_secret->InitOnce();
	quest_tournament = new Quest_Tournament;
	quest_tournament->InitOnce();
	quest_tutorial = new Quest_Tutorial;
	quest_tutorial->InitOnce();
}

//=================================================================================================
void QuestManager::Cleanup()
{
	delete quest_contest;
	delete quest_secret;
	delete quest_tournament;
	delete quest_tutorial;
}

//=================================================================================================
void QuestManager::Clear()
{
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_item_requests);
}

//=================================================================================================
void QuestManager::InitQuests(bool devmode)
{
	vector<int> used;

	// goblins
	quest_goblins = new Quest_Goblins;
	quest_goblins->Init();
	quest_goblins->start_loc = W.GetRandomSettlementIndex(used, 1);
	quest_goblins->refid = quest_counter++;
	quest_goblins->Start();
	unaccepted_quests.push_back(quest_goblins);
	used.push_back(quest_goblins->start_loc);

	// bandits
	quest_bandits = new Quest_Bandits;
	quest_bandits->Init();
	quest_bandits->start_loc = W.GetRandomSettlementIndex(used, 1);
	quest_bandits->refid = quest_counter++;
	quest_bandits->Start();
	unaccepted_quests.push_back(quest_bandits);
	used.push_back(quest_bandits->start_loc);

	// sawmill
	quest_sawmill = new Quest_Sawmill;
	quest_sawmill->start_loc = W.GetRandomSettlementIndex(used);
	quest_sawmill->refid = quest_counter++;
	quest_sawmill->Start();
	unaccepted_quests.push_back(quest_sawmill);
	used.push_back(quest_sawmill->start_loc);

	// mine
	quest_mine = new Quest_Mine;
	quest_mine->start_loc = W.GetRandomSettlementIndex(used);
	quest_mine->target_loc = W.GetClosestLocation(L_CAVE, W.GetLocation(quest_mine->start_loc)->pos);
	quest_mine->refid = quest_counter++;
	quest_mine->Start();
	unaccepted_quests.push_back(quest_mine);
	used.push_back(quest_mine->start_loc);

	// mages
	quest_mages = new Quest_Mages;
	quest_mages->start_loc = W.GetRandomSettlementIndex(used);
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
	quest_rumor[P_MAGOWIE2] = true;
	--quest_rumor_counter;

	// orcs
	quest_orcs = new Quest_Orcs;
	quest_orcs->Init();
	quest_orcs->start_loc = W.GetRandomSettlementIndex(used);
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
	quest_evil->start_loc = W.GetRandomSettlementIndex(used);
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

	// pseudo quests
	quest_contest->Init();
	quest_secret->Init();
	quest_tournament->Init();

	if(devmode)
	{
		Info("Quest 'Sawmill' - %s.", W.GetLocation(quest_sawmill->start_loc)->name.c_str());
		Info("Quest 'Mine' - %s, %s.", W.GetLocation(quest_mine->start_loc)->name.c_str(), W.GetLocation(quest_mine->target_loc)->name.c_str());
		Info("Quest 'Bandits' - %s.", W.GetLocation(quest_bandits->start_loc)->name.c_str());
		Info("Quest 'Mages' - %s.", W.GetLocation(quest_mages->start_loc)->name.c_str());
		Info("Quest 'Orcs' - %s.", W.GetLocation(quest_orcs->start_loc)->name.c_str());
		Info("Quest 'Goblins' - %s.", W.GetLocation(quest_goblins->start_loc)->name.c_str());
		Info("Quest 'Evil' - %s.", W.GetLocation(quest_evil->start_loc)->name.c_str());
		Info("Tournament - %s.", W.GetLocation(quest_tournament->GetCity())->name.c_str());
		Info("Contest - %s.", W.GetLocation(quest_contest->where)->name.c_str());
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
	case Q_MAIN:
		return new Quest_Main;
	case Q_FORCE_NONE:
		return nullptr;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
QUEST QuestManager::GetRandomQuest(QuestType type)
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
			return Q_FORCE_NONE;
		}
	}

	if(force != Q_FORCE_DISABLED)
	{
		auto& info = infos[force];
		if(info.type != type)
		{
			Warn("Can't force quest '%s', invalid quest type.", info.name);
			force = Q_FORCE_DISABLED;
		}
		else
		{
			Info("Forcing '%s' quest.", info.name);
			force = Q_FORCE_DISABLED;
			return info.id;
		}
	}

	switch(type)
	{
	case QuestType::Mayor:
		switch(Rand() % 12)
		{
		case 0:
		case 1:
		case 2:
			return Q_DELIVER_LETTER;
		case 3:
		case 4:
		case 5:
			return Q_DELIVER_PARCEL;
		case 6:
		case 7:
			return Q_SPREAD_NEWS;
		case 8:
		case 9:
			return Q_RETRIEVE_PACKAGE;
		case 10:
		case 11:
		default:
			return Q_FORCE_NONE;
		}
	case QuestType::Captain:
		switch(Rand() % 11)
		{
		case 0:
		case 1:
			return Q_RESCUE_CAPTIVE;
		case 2:
		case 3:
			return Q_BANDITS_COLLECT_TOLL;
		case 4:
		case 5:
			return Q_CAMP_NEAR_CITY;
		case 6:
		case 7:
			return Q_KILL_ANIMALS;
		case 8:
		case 9:
			return Q_WANTED;
		case 10:
		default:
			return Q_FORCE_NONE;
		}
	case QuestType::Random:
		switch(Rand() % 3)
		{
		case 0:
		default:
			return Q_FIND_ARTIFACT;
		case 1:
			return Q_LOST_ARTIFACT;
		case 2:
			return Q_STOLEN_ARTIFACT;
		}
	default:
		assert(0);
		return Q_FORCE_NONE;
	}
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
	DeleteElements(quest_item_requests);
	quests_timeout.clear();
	quests_timeout2.clear();
	quest_counter = 0;
	unique_quests_completed = 0;
	unique_completed_show = false;
	quest_rumor_counter = P_MAX;
	for(int i = 0; i < P_MAX; ++i)
		quest_rumor[i] = false;
}

//=================================================================================================
void QuestManager::Update(int days)
{
	// mark quest locations as not quest / remove quest camps
	LoopAndRemove(quests_timeout, [this](Quest_Dungeon* quest)
	{
		if(!quest->IsTimedout())
			return false;

		Location* loc = W.GetLocation(quest->target_loc);
		bool in_camp = false;

		if(loc->type == L_CAMP && (quest->target_loc == W.GetTravelLocationIndex() || quest->target_loc == W.GetCurrentLocationIndex()))
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
			W.DeleteCamp((Camp*)loc);
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
	if(quest_contest->year != W.GetYear())
	{
		quest_contest->year = W.GetYear();
		quest_contest->where = W.GetRandomSettlementIndex(quest_contest->where);
	}
}

//=================================================================================================
void QuestManager::Write(BitStreamWriter& f)
{
	f.WriteCasted<word>(quests.size());
	for(Quest* quest : quests)
	{
		f << quest->refid;
		f.WriteCasted<byte>(quest->state);
		f << quest->name;
		f.WriteStringArray<byte, word>(quest->msgs);
	}
}

//=================================================================================================
bool QuestManager::Read(BitStreamReader& f)
{
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

	f << quest_counter;
	f << unique_quests_completed;
	f << unique_completed_show;
	f << quest_rumor_counter;
	f << quest_rumor;
	f << force;

	quest_contest->Save(f);
	quest_secret->Save(f);
	quest_tournament->Save(f);
}

//=================================================================================================
void QuestManager::Load(GameReader& f)
{
	LoadQuests(f, quests);
	LoadQuests(f, unaccepted_quests);

	quests_timeout.resize(f.Read<uint>());
	for(Quest_Dungeon*& q : quests_timeout)
		q = (Quest_Dungeon*)FindQuest(f.Read<uint>(), false);

	if(LOAD_VERSION >= V_0_4)
	{
		quests_timeout2.resize(f.Read<uint>());
		for(Quest*& q : quests_timeout2)
			q = FindQuest(f.Read<uint>(), false);
	}
	else
	{
		// old timed units (now removed)
		uint count;
		f >> count;
		f.Skip(sizeof(int) * 3 * count);
	}

	f >> quest_counter;
	f >> unique_quests_completed;
	f >> unique_completed_show;
	f >> quest_rumor_counter;
	f >> quest_rumor;

	if(LOAD_VERSION >= V_0_5)
		f >> force;
	else
		force = Q_FORCE_DISABLED;

	// get quest pointers
	quest_sawmill = (Quest_Sawmill*)FindQuestById(Q_SAWMILL);
	quest_mine = (Quest_Mine*)FindQuestById(Q_MINE);
	quest_bandits = (Quest_Bandits*)FindQuestById(Q_BANDITS);
	quest_bandits->Init();
	quest_goblins = (Quest_Goblins*)FindQuestById(Q_GOBLINS);
	quest_goblins->Init();
	quest_mages = (Quest_Mages*)FindQuestById(Q_MAGES);
	quest_mages2 = (Quest_Mages2*)FindQuestById(Q_MAGES2);
	quest_orcs = (Quest_Orcs*)FindQuestById(Q_ORCS);
	quest_orcs->Init();
	quest_orcs2 = (Quest_Orcs2*)FindQuestById(Q_ORCS2);
	quest_orcs2->Init();
	quest_evil = (Quest_Evil*)FindQuestById(Q_EVIL);
	quest_evil->Init();
	quest_crazies = (Quest_Crazies*)FindQuestById(Q_CRAZIES);
	quest_crazies->Init();

	if(LOAD_VERSION < V_FEATURE && !quest_mages2)
	{
		quest_mages2 = new Quest_Mages2;
		quest_mages2->refid = quest_counter++;
		quest_mages2->Start();
		unaccepted_quests.push_back(quest_mages2);
	}
	quest_mages2->Init();

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
			if(ok && (LOAD_VERSION < V_0_7_1 || content::require_update))
			{
				SortItems(*qir->items);
				if(qir->unit)
					qir->unit->RecalculateWeight();
			}
		}
		delete *it;
	}
	quest_item_requests.clear();

	// load quests old data (now are stored inside quest)
	if(LOAD_VERSION < V_0_4)
	{
		quest_sawmill->LoadOld(f);
		quest_mine->LoadOld(f);
		quest_bandits->LoadOld(f);
		quest_mages2->LoadOld(f);
		quest_orcs2->LoadOld(f);
		quest_goblins->LoadOld(f);
		quest_evil->LoadOld(f);
		quest_crazies->LoadOld(f);
	}

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
	for(Quest* quest : quests)
	{
		if(refid == quest->refid)
			return quest->GetQuestItem();
	}

	assert(0);
	return nullptr;
}

//=================================================================================================
void QuestManager::EndUniqueQuest()
{
	++unique_quests_completed;
}

//=================================================================================================
bool QuestManager::RemoveQuestRumor(PLOTKA_QUESTOWA rumor_id)
{
	if(!quest_rumor[rumor_id])
	{
		quest_rumor[rumor_id] = true;
		--quest_rumor_counter;
		return true;
	}
	else
		return false;
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

	for(auto& info : infos)
	{
		if(name == info.name)
		{
			force = info.id;
			return true;
		}
	}

	return false;
}

//=================================================================================================
bool QuestManager::HandleSpecialGeneric(std::map<string, QuestHandler*>& handlers, DialogContext& ctx, cstring msg, bool& result)
{
	std::map<string, QuestHandler*>::iterator it;
	cstring slash = strrchr(msg, '\\');
	if(slash)
	{
		tmp_str = string(msg, slash - msg);
		it = handlers.find(tmp_str); // FIXME verify
	}
	else
		it = handlers.find(msg);
	if(it == handlers.end())
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
