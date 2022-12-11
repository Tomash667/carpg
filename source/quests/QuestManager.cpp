#include "Pch.h"
#include "QuestManager.h"

#include "BitStreamFunc.h"
#include "Camp.h"
#include "City.h"
#include "Content.h"
#include "Game.h"
#include "InsideLocation.h"
#include "Language.h"
#include "Level.h"
#include "Net.h"
#include "QuestList.h"
#include "QuestScheme.h"
#include "Quest_Artifacts.h"
#include "Quest_Bandits.h"
#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
#include "Quest_Contest.h"
#include "Quest_Crazies.h"
#include "Quest_DeliverLetter.h"
#include "Quest_DeliverParcel.h"
#include "Quest_DireWolf.h"
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
#include "Team.h"
#include "World.h"

QuestManager* questMgr;

//=================================================================================================
QuestManager::QuestManager() : questContest(nullptr), questSecret(nullptr), questTournament(nullptr), questTutorial(nullptr)
{
}

//=================================================================================================
QuestManager::~QuestManager()
{
	delete questContest;
	delete questSecret;
	delete questTournament;
	delete questTutorial;
}

//=================================================================================================
void QuestManager::Init()
{
	force = Q_FORCE_DISABLED;

	infos.push_back(QuestInfo(Q_GOBLINS, QuestCategory::Unique, "goblins"));
	infos.push_back(QuestInfo(Q_BANDITS, QuestCategory::Unique, "bandits"));
	infos.push_back(QuestInfo(Q_MINE, QuestCategory::Unique, "mine"));
	infos.push_back(QuestInfo(Q_SAWMILL, QuestCategory::Unique, "sawmill"));
	infos.push_back(QuestInfo(Q_MAGES, QuestCategory::Unique, "mages"));
	infos.push_back(QuestInfo(Q_MAGES2, QuestCategory::Unique, "mages2"));
	infos.push_back(QuestInfo(Q_ORCS, QuestCategory::Unique, "orcs"));
	infos.push_back(QuestInfo(Q_ORCS2, QuestCategory::Unique, "orcs2"));
	infos.push_back(QuestInfo(Q_EVIL, QuestCategory::Unique, "evil"));
	infos.push_back(QuestInfo(Q_DELIVER_PARCEL, QuestCategory::Mayor, "deliver_parcel"));
	infos.push_back(QuestInfo(Q_SPREAD_NEWS, QuestCategory::Mayor, "spread_news"));
	infos.push_back(QuestInfo(Q_RESCUE_CAPTIVE, QuestCategory::Captain, "rescue_captive"));
	infos.push_back(QuestInfo(Q_RETRIEVE_PACKAGE, QuestCategory::Mayor, "retrieve_package"));
	infos.push_back(QuestInfo(Q_KILL_ANIMALS, QuestCategory::Captain, "kill_animals"));
	infos.push_back(QuestInfo(Q_CRAZIES, QuestCategory::Unique, "crazies"));
	infos.push_back(QuestInfo(Q_WANTED, QuestCategory::Captain, "wanted"));
	infos.push_back(QuestInfo(Q_DIRE_WOLF, QuestCategory::Unique, "dire_wolf"));

	// create pseudo quests
	questContest = new Quest_Contest;
	questContest->InitOnce();
	questSecret = new Quest_Secret;
	questSecret->InitOnce();
	questTournament = new Quest_Tournament;
	questTournament->InitOnce();
	questTutorial = new Quest_Tutorial;
}

//=================================================================================================
void QuestManager::InitLists()
{
	questsMayor = QuestList::TryGet("mayor");
	questsCaptain = QuestList::TryGet("captain");
	questsRandom = QuestList::TryGet("random");
	assert(questsMayor && questsCaptain && questsRandom);

	uniqueQuests = 8;
	for(QuestScheme* scheme : QuestScheme::schemes)
	{
		if(scheme->category == QuestCategory::Unique && !IsSet(scheme->flags, QuestScheme::DONT_COUNT))
			++uniqueQuests;
	}
}

//=================================================================================================
void QuestManager::LoadLanguage()
{
	StrArray(txQuest, "quest");
	StrArray(txRumorQ, "rumorQ");
	txForMayor = Str("forMayor");
	txForSoltys = Str("forSoltys");
	questContest->LoadLanguage();
	questSecret->LoadLanguage();
	questTournament->LoadLanguage();
	questTutorial->LoadLanguage();
}

//=================================================================================================
void QuestManager::Clear()
{
	DeleteElements(quests);
	DeleteElements(unacceptedQuests);
	DeleteElements(questItemRequests);
	DeleteElements(questItems);
	if(questTournament)
		questTournament->Clear();
}

//=================================================================================================
void QuestManager::InitQuests()
{
	used.clear();

	// init quests
	for(QuestInfo& info : infos)
	{
		if(info.category == QuestCategory::Unique)
			CreateQuest(&info);
	}

	// set quest pointers
	questSawmill = static_cast<Quest_Sawmill*>(FindQuest(Q_SAWMILL));
	questMine = static_cast<Quest_Mine*>(FindQuest(Q_MINE));
	questBandits = static_cast<Quest_Bandits*>(FindQuest(Q_BANDITS));
	questGoblins = static_cast<Quest_Goblins*>(FindQuest(Q_GOBLINS));
	questMages = static_cast<Quest_Mages*>(FindQuest(Q_MAGES));
	questMages2 = static_cast<Quest_Mages2*>(FindQuest(Q_MAGES2));
	questOrcs = static_cast<Quest_Orcs*>(FindQuest(Q_ORCS));
	questOrcs2 = static_cast<Quest_Orcs2*>(FindQuest(Q_ORCS2));
	questEvil = static_cast<Quest_Evil*>(FindQuest(Q_EVIL));
	questCrazies = static_cast<Quest_Crazies*>(FindQuest(Q_CRAZIES));

	// pseudo quests
	questContest->Init();
	questSecret->Init();
	questTournament->Init();
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
	case Q_DIRE_WOLF:
		return new Quest_DireWolf;
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
	Quest* quest = CreateQuest(info->type);
	if(info->scheme)
		static_cast<Quest2*>(quest)->SetScheme(info->scheme);
	quest->Init();
	quest->id = questCounter++;
	quest->Start();
	unacceptedQuests.push_back(quest);
	return quest;
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
		list = questsMayor;
		break;
	case QuestCategory::Captain:
		list = questsCaptain;
		break;
	case QuestCategory::Random:
		list = questsRandom;
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
void QuestManager::AddQuestItemRequest(const Item** item, cstring name, int questId, vector<ItemSlot>* items, Unit* unit)
{
	assert(item && name && questId != -1);
	QuestItemRequest* q = new QuestItemRequest;
	q->item = item;
	q->name = name;
	q->questId = questId;
	q->items = items;
	q->unit = unit;
	questItemRequests.push_back(q);
}

//=================================================================================================
void QuestManager::Reset()
{
	force = Q_FORCE_DISABLED;
	DeleteElements(quests);
	DeleteElements(unacceptedQuests);
	questRequests.clear();
	DeleteElements(questItemRequests);
	questTimeouts.clear();
	questTimeouts2.clear();
	questCounter = 0;
	uniqueQuestsCompleted = 0;
	uniqueCompletedShow = false;
	questRumors.clear();
	itemEventHandlers.clear();
}

//=================================================================================================
void QuestManager::Update(int days)
{
	// mark quest locations as not quest / remove quest camps
	LoopAndRemove(questTimeouts, [this](Quest_Dungeon* quest)
	{
		if(!quest->IsTimedout())
			return false;

		Location* loc = quest->targetLoc;
		bool in_camp = false;

		if(loc->type == L_CAMP && (loc == world->GetTravelLocation() || loc == world->GetCurrentLocation()))
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

		loc->activeQuest = nullptr;

		if(loc->type == L_CAMP)
		{
			quest->targetLoc = nullptr;
			world->DeleteCamp(static_cast<Camp*>(loc));
		}

		return true;
	});

	// quest timeouts, not attached to location
	LoopAndRemove(questTimeouts2, [](Quest* quest)
	{
		if(quest->IsTimedout() && quest->OnTimeout(TIMEOUT2))
		{
			quest->timeout = true;
			return true;
		}
		return false;
	});

	// update contest
	const Date& date = world->GetDateValue();
	if(questContest->year != date.year)
	{
		questContest->year = date.year;
		questContest->where = world->GetRandomSettlement(world->GetLocation(questContest->where))->index;
	}

	if(!team->isBandit)
	{
		RemoveQuestUnits(false);

		questSawmill->OnProgress(days);
		questMine->OnProgress(days);
		questContest->OnProgress();
		questTournament->OnProgress();
		questMages2->OnProgress(days);
		questOrcs2->OnProgress(days);
		questGoblins->OnProgress(days);
		questCrazies->OnProgress(days);

		if(gameLevel->cityCtx)
			GenerateQuestUnits(false);
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
		f << c.baseItem->id;
		f << c.item2->id;
		f << c.item2->name;
		f << c.item2->desc;
		f << c.item2->questId;
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
		quest->questIndex = index;
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
	questItems.reserve(quest_items_count);
	for(word i = 0; i < quest_items_count; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read world: Broken packet for quest item %u.", i);
			return false;
		}

		const Item* baseItem;
		if(item_id[0] == '$')
			baseItem = Item::TryGet(item_id.c_str() + 1);
		else
			baseItem = Item::TryGet(item_id);
		if(!baseItem)
		{
			Error("Read world: Missing quest item '%s' (%u).", item_id.c_str(), i);
			return false;
		}

		Item* item = baseItem->CreateCopy();
		f >> item->id;
		f >> item->name;
		f >> item->desc;
		f >> item->questId;
		if(!f)
		{
			Error("Read world: Broken packet for quest item %u (2).", i);
			delete item;
			return false;
		}
		else
			questItems.push_back(item);
	}

	return true;
}

//=================================================================================================
void QuestManager::Save(GameWriter& f)
{
	f << quests.size();
	for(Quest* quest : quests)
		quest->Save(f);

	f << unacceptedQuests.size();
	for(Quest* quest : unacceptedQuests)
		quest->Save(f);

	f << questTimeouts.size();
	for(Quest_Dungeon* q : questTimeouts)
		f << q->id;

	f << questTimeouts2.size();
	for(Quest* q : questTimeouts2)
		f << q->id;

	f << questItems.size();
	for(Item* item : questItems)
	{
		f << item->id;
		f << item->questId;
		f << item->name;
	}

	f << itemEventHandlers.size();
	for(pair<Quest2*, const Item*>& p : itemEventHandlers)
	{
		f << p.first->id;
		f << p.second;
	}

	f << questCounter;
	f << uniqueQuestsCompleted;
	f << uniqueCompletedShow;
	f << questRumors.size();
	for(pair<int, string>& rumor : questRumors)
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

	questSecret->Save(f);
	questContest->Save(f);
	questTournament->Save(f);
}

//=================================================================================================
void QuestManager::Load(GameReader& f)
{
	upgradeQuests.clear();

	LoadQuests(f, quests);
	LoadQuests(f, unacceptedQuests);

	// get quest pointers
	questSawmill = static_cast<Quest_Sawmill*>(FindQuest(Q_SAWMILL));
	questMine = static_cast<Quest_Mine*>(FindQuest(Q_MINE));
	questBandits = static_cast<Quest_Bandits*>(FindQuest(Q_BANDITS));
	questBandits->Init();
	questGoblins = static_cast<Quest_Goblins*>(FindQuest(Q_GOBLINS));
	questGoblins->Init();
	questMages = static_cast<Quest_Mages*>(FindQuest(Q_MAGES));
	questMages2 = static_cast<Quest_Mages2*>(FindQuest(Q_MAGES2));
	questMages2->Init();
	questOrcs = static_cast<Quest_Orcs*>(FindQuest(Q_ORCS));
	questOrcs->Init();
	questOrcs2 = static_cast<Quest_Orcs2*>(FindQuest(Q_ORCS2));
	questOrcs2->Init();
	questEvil = static_cast<Quest_Evil*>(FindQuest(Q_EVIL));
	questEvil->Init();
	questCrazies = static_cast<Quest_Crazies*>(FindQuest(Q_CRAZIES));
	questCrazies->Init();

	// quest timeouts
	questTimeouts.resize(f.Read<uint>());
	for(Quest_Dungeon*& q : questTimeouts)
		q = static_cast<Quest_Dungeon*>(FindQuest(f.Read<int>(), false));
	questTimeouts2.resize(f.Read<uint>());
	for(Quest*& q : questTimeouts2)
		q = FindQuest(f.Read<int>(), false);

	// quest items
	questItems.resize(f.Read<uint>());
	for(Item*& item : questItems)
	{
		const string& id = f.ReadString1();
		Item* base = Item::Get(id.c_str() + 1);
		item = base->CreateCopy();
		item->id = id;
		f >> item->questId;
		f >> item->name;
	}

	// item event handlers
	if(LOAD_VERSION >= V_DEV)
	{
		itemEventHandlers.resize(f.Read<uint>());
		for(pair<Quest2*, const Item*>& p : itemEventHandlers)
		{
			p.first = static_cast<Quest2*>(FindQuest(f.Read<int>(), false));
			f >> p.second;
		}
	}
	else
		itemEventHandlers.clear();

	f >> questCounter;
	f >> uniqueQuestsCompleted;
	f >> uniqueCompletedShow;

	// quest rumors
	uint count;
	f >> count;
	questRumors.resize(count);
	for(pair<int, string>& rumor : questRumors)
	{
		f >> rumor.first;
		f >> rumor.second;
	}

	// force quest
	const string& force_id = f.ReadString1();
	if(force_id.empty())
		force = Q_FORCE_DISABLED;
	else
		SetForcedQuest(force_id);

	// load pseudo-quests
	questSecret->Load(f);
	questContest->Load(f);
	questTournament->Load(f);
}

//=================================================================================================
void QuestManager::LoadQuests(GameReader& f, vector<Quest*>& quests)
{
	uint count = f.Read<uint>();
	quests.resize(count, nullptr);

	uint questIndex = 0;
	for(uint i = 0; i < count; ++i)
	{
		QUEST_TYPE type;
		f >> type;

		Quest* quest = CreateQuest(type);
		quest->type = type;
		quest->questIndex = questIndex;
		Quest::LoadResult result = quest->Load(f);
		if(result == Quest::LoadResult::Remove)
		{
			delete quest;
			continue;
		}
		else if(result == Quest::LoadResult::Convert)
			upgradeQuests.push_back(quest);

		quests[questIndex] = quest;
		++questIndex;
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
Quest* QuestManager::FindQuest(Location* location, QuestCategory category)
{
	for(Quest* quest : quests)
	{
		if(quest->startLoc == location && quest->category == category)
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

	for(Quest* quest : unacceptedQuests)
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

	for(Quest* quest : unacceptedQuests)
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

	for(Quest* quest : unacceptedQuests)
	{
		if(quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(Location* location, QuestCategory category)
{
	for(Quest* quest : unacceptedQuests)
	{
		if(quest->startLoc == location && quest->category == category)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int id)
{
	for(Quest* quest : unacceptedQuests)
	{
		if(quest->id == id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuestS(const string& questId)
{
	QuestScheme* scheme = QuestScheme::TryGet(questId);
	if(!scheme)
		return nullptr;
	return FindAnyQuest(scheme);
}

//=================================================================================================
const Item* QuestManager::FindQuestItem(cstring item_id, int questId)
{
	for(Item* item : questItems)
	{
		if(item->questId == questId && item->id == item_id)
			return item;
	}

	for(Quest* quest : quests)
	{
		if(quest->id == questId)
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
		tmpStr = string(msg, slash - msg);
		it = specialHandlers.find(tmpStr);
	}
	else
		it = specialHandlers.find(msg);
	if(it == specialHandlers.end())
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
		tmpStr = string(msg, slash - msg);
		it = specialIfHandlers.find(tmpStr);
	}
	else
		it = specialIfHandlers.find(msg);
	if(it == specialIfHandlers.end())
		return false;
	result = it->second->SpecialIf(ctx, msg);
	return true;
}

//=================================================================================================
bool QuestManager::HandleFormatString(const string& str, cstring& result)
{
	auto it = formatStrHandlers.find(str);
	if(it == formatStrHandlers.end())
		return false;
	result = it->second->FormatString(str);
	assert(result);
	return true;
}

//=================================================================================================
const Item* QuestManager::FindQuestItemClient(cstring item_id, int questId) const
{
	assert(item_id);

	for(Item* item : questItems)
	{
		if(item->id == item_id && (questId == -1 || item->IsQuest(questId)))
			return item;
	}

	return nullptr;
}

//=================================================================================================
void QuestManager::AddScriptedQuest(QuestScheme* scheme)
{
	assert(scheme);
	if(IsSet(scheme->flags, QuestScheme::NOT_SCRIPTED))
	{
		QuestInfo* info = FindQuestInfo(scheme->id);
		info->scheme = scheme;
	}
	else
		infos.push_back(QuestInfo(Q_SCRIPTED, scheme->category, scheme->id.c_str(), scheme));
}

//=================================================================================================
QuestInfo* QuestManager::FindQuestInfo(QUEST_TYPE type)
{
	for(QuestInfo& info : infos)
	{
		if(type == info.type)
			return &info;
	}
	return nullptr;
}

//=================================================================================================
QuestInfo* QuestManager::FindQuestInfo(const string& id)
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
	int id = questCounter++;
	questRumors.push_back(pair<int, string>(id, str));
	return id;
}

//=================================================================================================
bool QuestManager::RemoveQuestRumor(int id)
{
	for(uint i = 0; i < questRumors.size(); ++i)
	{
		if(questRumors[i].first == id)
		{
			RemoveElementIndex(questRumors, i);
			return true;
		}
	}
	return false;
}

//=================================================================================================
string QuestManager::GetRandomQuestRumor()
{
	uint index = Rand() % questRumors.size();
	string str = questRumors[index].second;
	RemoveElementIndex(questRumors, index);
	return str;
}

//=================================================================================================
void QuestManager::GenerateQuestUnits(bool onEnter)
{
	if(onEnter)
	{
		if(questSawmill->sawmillState == Quest_Sawmill::State::None && gameLevel->location == questSawmill->startLoc)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("artur_drwal"), -2);
			assert(u);
			if(u)
			{
				questSawmill->sawmillState = Quest_Sawmill::State::GeneratedUnit;
				questSawmill->hdLumberjack.Get(*u->humanData);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->location == questMine->startLoc && questMine->mineState == Quest_Mine::State::None)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("inwestor"), -2);
			assert(u);
			if(u)
			{
				questMine->mineState = Quest_Mine::State::SpawnedInvestor;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->location == questBandits->startLoc && questBandits->banditsState == Quest_Bandits::State::None)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("mistrz_agentow"), -2);
			assert(u);
			if(u)
			{
				questBandits->banditsState = Quest_Bandits::State::GeneratedMaster;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->location == questMages->startLoc && questMages2->magesState == Quest_Mages2::State::None)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_magowie_uczony"), -2);
			assert(u);
			if(u)
			{
				questMages2->magesState = Quest_Mages2::State::GeneratedScholar;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->locationIndex == questMages2->mageLoc)
		{
			if(questMages2->magesState == Quest_Mages2::State::TalkedWithCaptain)
			{
				Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
				assert(u);
				if(u)
				{
					questMages2->magesState = Quest_Mages2::State::GeneratedOldMage;
					questMages2->goodMageName = u->hero->name;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
			else if(questMages2->magesState == Quest_Mages2::State::MageLeft)
			{
				Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
				assert(u);
				if(u)
				{
					questMages2->scholar = u;
					u->hero->knowName = true;
					u->hero->name = questMages2->goodMageName;
					u->ApplyHumanData(questMages2->hdMage);
					questMages2->magesState = Quest_Mages2::State::MageGeneratedInCity;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
		}

		if(gameLevel->location == questOrcs->startLoc && questOrcs2->orcsState == Quest_Orcs2::State::None)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_orkowie_straznik"));
			assert(u);
			if(u)
			{
				u->OrderAutoTalk();
				questOrcs2->orcsState = Quest_Orcs2::State::GeneratedGuard;
				questOrcs2->guard = u;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->location == questGoblins->startLoc && questGoblins->goblinsState == Quest_Goblins::State::None)
		{
			Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_gobliny_szlachcic"));
			assert(u);
			if(u)
			{
				questGoblins->nobleman = u;
				questGoblins->hdNobleman.Get(*u->humanData);
				questGoblins->goblinsState = Quest_Goblins::State::GeneratedNobleman;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(gameLevel->location == questEvil->startLoc && questEvil->evilState == Quest_Evil::State::None)
		{
			CityBuilding* b = gameLevel->cityCtx->FindBuilding(BuildingGroup::BG_INN);
			Unit* u = gameLevel->SpawnUnitNearLocation(*gameLevel->localPart, b->walkPt, *UnitData::Get("q_zlo_kaplan"), nullptr, 10);
			assert(u);
			if(u)
			{
				u->OrderAutoTalk();
				questEvil->cleric = u;
				questEvil->evilState = Quest_Evil::State::GeneratedCleric;
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(!team->isBandit)
		{
			// sawmill quest
			if(questSawmill->sawmillState == Quest_Sawmill::State::InBuild)
			{
				if(questSawmill->days >= 30 && gameLevel->cityCtx)
				{
					questSawmill->days = 29;
					Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("poslaniec_tartak"), &team->leader->pos, -2, 2.f);
					if(u)
					{
						questSawmill->messenger = u;
						u->OrderAutoTalk(true);
					}
				}
			}

			if(questMine->days >= questMine->daysRequired
				&& ((questMine->mineState2 == Quest_Mine::State2::InBuild && questMine->mineState == Quest_Mine::State::Shares) // inform player about building mine & give gold
				|| questMine->mineState2 == Quest_Mine::State2::Built // inform player about possible investment
				|| questMine->mineState2 == Quest_Mine::State2::InExpand // inform player about finished mine expanding
				|| questMine->mineState2 == Quest_Mine::State2::Expanded)) // inform player about finding portal
			{
				Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("poslaniec_kopalnia"), &team->leader->pos, -2, 2.f);
				if(u)
				{
					questMine->messenger = u;
					u->OrderAutoTalk(true);
				}
			}

			if(questEvil->evilState == Quest_Evil::State::GenerateMage && gameLevel->locationIndex == questEvil->mageLoc)
			{
				Unit* u = gameLevel->SpawnUnitInsideInn(*UnitData::Get("q_zlo_mag"), -2);
				assert(u);
				if(u)
				{
					questEvil->evilState = Quest_Evil::State::GeneratedMage;
					if(game->devmode)
						Info("Generated quest unit '%s'.", u->GetRealName());
				}
			}
		}

		const Date& date = world->GetDateValue();
		if(date.day == 6 && date.month == 2 && gameLevel->cityCtx && IsSet(gameLevel->cityCtx->flags, City::HaveArena)
			&& gameLevel->locationIndex == questTournament->GetCity() && !questTournament->IsGenerated())
			questTournament->GenerateUnits();
	}

	// spawn on enter or update
	if(!team->isBandit)
	{
		if(questGoblins->goblinsState == Quest_Goblins::State::Counting && questGoblins->days <= 0)
		{
			Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("q_gobliny_poslaniec"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				questGoblins->messenger = u;
				u->OrderAutoTalk(true);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}

		if(questGoblins->goblinsState == Quest_Goblins::State::NoblemanLeft && questGoblins->days <= 0)
		{
			Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("q_gobliny_mag"), &team->leader->pos, 5, 2.f);
			if(u)
			{
				questGoblins->messenger = u;
				questGoblins->goblinsState = Quest_Goblins::State::GeneratedMage;
				u->OrderAutoTalk(true);
				if(game->devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}
	}
}

//=================================================================================================
void QuestManager::RemoveQuestUnits(bool onLeave)
{
	if(gameLevel->cityCtx)
	{
		if(questSawmill->messenger)
		{
			gameLevel->RemoveUnit(UnitData::Get("poslaniec_tartak"), onLeave);
			questSawmill->messenger = nullptr;
		}

		if(questMine->messenger)
		{
			gameLevel->RemoveUnit(UnitData::Get("poslaniec_kopalnia"), onLeave);
			questMine->messenger = nullptr;
		}

		if(gameLevel->isOpen && gameLevel->location == questSawmill->startLoc && questSawmill->sawmillState == Quest_Sawmill::State::InBuild
			&& questSawmill->buildState == Quest_Sawmill::BuildState::None)
		{
			Unit* u = gameLevel->cityCtx->FindInn()->FindUnit(UnitData::Get("artur_drwal"));
			if(u && u->IsAlive())
			{
				questSawmill->buildState = Quest_Sawmill::BuildState::LumberjackLeft;
				gameLevel->RemoveUnit(u, !onLeave);
			}
		}

		if(questMages2->scholar && questMages2->magesState == Quest_Mages2::State::ScholarWaits)
		{
			gameLevel->RemoveUnit(UnitData::Get("q_magowie_uczony"), onLeave);
			questMages2->scholar = nullptr;
			questMages2->magesState = Quest_Mages2::State::Counting;
			questMages2->days = Random(15, 30);
		}

		if(questOrcs2->guard && questOrcs2->orcsState >= Quest_Orcs2::State::GuardTalked)
		{
			gameLevel->RemoveUnit(UnitData::Get("q_orkowie_straznik"), onLeave);
			questOrcs2->guard = nullptr;
		}
	}

	if(questBandits->banditsState == Quest_Bandits::State::AgentTalked)
	{
		questBandits->banditsState = Quest_Bandits::State::AgentLeft;
		questBandits->agent = nullptr;
	}

	if(questMages2->magesState == Quest_Mages2::State::MageLeaving)
	{
		questMages2->magesState = Quest_Mages2::State::MageLeft;
		questMages2->scholar = nullptr;
	}

	if(questGoblins->goblinsState == Quest_Goblins::State::MessengerTalked && questGoblins->messenger)
	{
		gameLevel->RemoveUnit(UnitData::Get("q_gobliny_poslaniec"), onLeave);
		questGoblins->messenger = nullptr;
	}

	if(questGoblins->goblinsState == Quest_Goblins::State::GivenBow && questGoblins->nobleman)
	{
		gameLevel->RemoveUnit(UnitData::Get("q_gobliny_szlachcic"), onLeave);
		questGoblins->nobleman = nullptr;
		questGoblins->goblinsState = Quest_Goblins::State::NoblemanLeft;
		questGoblins->days = Random(15, 30);
	}

	if(questGoblins->goblinsState == Quest_Goblins::State::MageTalked && questGoblins->messenger)
	{
		gameLevel->RemoveUnit(UnitData::Get("q_gobliny_mag"), onLeave);
		questGoblins->messenger = nullptr;
		questGoblins->goblinsState = Quest_Goblins::State::MageLeft;
	}

	if(questEvil->evilState == Quest_Evil::State::ClericLeaving)
	{
		questEvil->cleric = nullptr;
		questEvil->evilState = Quest_Evil::State::ClericLeft;
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
	if(gameLevel->localPart->partType == LocationPart::Type::Inside)
	{
		inside = static_cast<InsideLocation*>(gameLevel->location);
		lvl = &inside->GetLevelData();
	}

	// spawn unit
	if(event->unitToSpawn)
	{
		if(gameLevel->localPart->partType == LocationPart::Type::Outside)
		{
			if(gameLevel->location->type == L_CITY)
				spawned = gameLevel->SpawnUnitInsideInn(*event->unitToSpawn, event->unitSpawnLevel);
			else
			{
				Vec3 pos(0, 0, 0);
				int count = 0;
				for(Unit* unit : gameLevel->localPart->units)
				{
					pos += unit->pos;
					++count;
				}
				pos /= (float)count;
				spawned = gameLevel->SpawnUnitNearLocation(*gameLevel->localPart, pos, *event->unitToSpawn, nullptr, event->unitSpawnLevel);
			}
		}
		else
		{
			Room& room = lvl->GetRoom(event->unitSpawnRoom, inside->HaveNextEntry());
			spawned = gameLevel->SpawnUnitInsideRoomOrNear(room, *event->unitToSpawn, event->unitSpawnLevel);
		}
		if(!spawned)
			throw "Failed to spawn quest unit!";
		spawned->dontAttack = event->unitDontAttack;
		if(event->unitAutoTalk)
			spawned->OrderAutoTalk();
		spawned->eventHandler = event->unitEventHandler;
		if(spawned->eventHandler && event->sendSpawnEvent)
			spawned->eventHandler->HandleUnitEvent(UnitEventHandler::SPAWN, spawned);
		if(game->devmode)
			Info("Generated unit %s (%g,%g).", event->unitToSpawn->id.c_str(), spawned->pos.x, spawned->pos.z);

		// mark near units as guards if guarded (only in dungeon)
		if(IsSet(spawned->data->flags2, F2_GUARDED) && lvl)
		{
			Room& room = lvl->GetRoom(event->unitSpawnRoom, inside->HaveNextEntry());
			for(Unit* unit : gameLevel->localPart->units)
			{
				if(unit != spawned && unit->IsFriend(*spawned) && lvl->GetRoom(PosToPt(unit->pos)) == &room)
				{
					unit->dontAttack = spawned->dontAttack;
					unit->OrderGuard(spawned);
				}
			}
		}
	}

	// spawn second units (only in dungeon)
	if(event->unitToSpawn2 && lvl)
	{
		Room* room;
		if(spawned && event->spawnGuards)
			room = lvl->GetRoom(PosToPt(spawned->pos));
		else
			room = &lvl->GetRoom(event->unitSpawnRoom2, inside->HaveNextEntry());
		spawned2 = gameLevel->SpawnUnitInsideRoomOrNear(*room, *event->unitToSpawn2, event->unitSpawnLevel2);
		if(!spawned2)
			throw "Failed to spawn quest unit 2!";
		if(game->devmode)
			Info("Generated unit %s (%g,%g).", event->unitToSpawn2->id.c_str(), spawned2->pos.x, spawned2->pos.z);
		if(spawned && event->spawnGuards)
		{
			spawned2->dontAttack = spawned->dontAttack;
			spawned2->OrderGuard(spawned);
		}
	}

	// spawn item
	switch(event->spawnItem)
	{
	case Quest_Dungeon::Item_DontSpawn:
		break;
	case Quest_Dungeon::Item_GiveStrongest:
		{
			Unit* best = nullptr;
			for(Unit* unit : gameLevel->localPart->units)
			{
				if(unit->IsAlive() && unit->IsEnemy(*game->pc->unit) && (!best || unit->level > best->level))
					best = unit;
			}
			assert(best);
			if(best)
			{
				best->AddItem(event->itemToGive[0], 1, true);
				if(game->devmode)
					Info("Given item %s unit %s (%g,%g).", event->itemToGive[0]->id.c_str(), best->data->id.c_str(), best->pos.x, best->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned:
		assert(spawned);
		if(spawned)
		{
			spawned->AddItem(event->itemToGive[0], 1, true);
			if(game->devmode)
				Info("Given item %s unit %s (%g,%g).", event->itemToGive[0]->id.c_str(), spawned->data->id.c_str(), spawned->pos.x, spawned->pos.z);
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned2:
		assert(spawned2);
		if(spawned2)
		{
			spawned2->AddItem(event->itemToGive[0], 1, true);
			if(game->devmode)
				Info("Given item %s unit %s (%g,%g).", event->itemToGive[0]->id.c_str(), spawned2->data->id.c_str(), spawned2->pos.x, spawned2->pos.z);
		}
		break;
	case Quest_Dungeon::Item_OnGround:
		{
			GroundItem* item;
			if(lvl)
				item = gameLevel->SpawnGroundItemInsideAnyRoom(event->itemToGive[0]);
			else
				item = gameLevel->SpawnGroundItemInsideRadius(event->itemToGive[0], Vec2(128, 128), 10.f);
			if(game->devmode)
				Info("Generated item %s on ground (%g,%g).", event->itemToGive[0]->id.c_str(), item->pos.x, item->pos.z);
		}
		break;
	case Quest_Dungeon::Item_InTreasure:
		if(inside && Any(inside->target, HERO_CRYPT, MONSTER_CRYPT, LABYRINTH))
		{
			Chest* chest = gameLevel->GetTreasureChest();
			assert(chest);
			if(chest)
			{
				chest->AddItem(event->itemToGive[0]);
				if(game->devmode)
					Info("Generated item %s in treasure chest (%g,%g).", event->itemToGive[0]->id.c_str(), chest->pos.x, chest->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_InChest:
		{
			Chest* chest = gameLevel->localPart->GetRandomFarChest(gameLevel->GetSpawnPoint());
			assert(event->itemToGive[0]);
			if(game->devmode)
			{
				LocalString str = "Addded items (";
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->itemToGive[i])
						break;
					if(i > 0)
						str += ", ";
					chest->AddItem(event->itemToGive[i]);
					str += event->itemToGive[i]->id;
				}
				str += Format(") to chest (%g,%g).", chest->pos.x, chest->pos.z);
				Info(str.get_ref().c_str());
			}
			else
			{
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->itemToGive[i])
						break;
					chest->AddItem(event->itemToGive[i]);
				}
			}
			chest->handler = event->chestEventHandler;
		}
		break;
	}

	if(event->callback)
		event->callback();
}

//=================================================================================================
void QuestManager::UpdateQuestsLocal(float dt)
{
	questTournament->Update(dt);
	questContest->Update(dt);
	questBandits->Update(dt);
	questMages2->Update(dt);
	questEvil->Update(dt);
	questSecret->UpdateFight();
}

//=================================================================================================
void QuestManager::ProcessQuestRequests()
{
	// process quest requests
	for(QuestRequest& request : questRequests)
	{
		Quest* quest = FindAnyQuest(request.id);
		assert(quest);
		*request.quest = quest;
		if(request.callback)
			request.callback();
	}
	questRequests.clear();

	// process quest item requests
	for(vector<QuestItemRequest*>::iterator it = questItemRequests.begin(), end = questItemRequests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		const Item* item = FindQuestItem(qir->name.c_str(), qir->questId);
		assert(item);
		*qir->item = item;
		if(qir->items && content.requireUpdate)
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
			if(ok)
			{
				SortItems(*qir->items);
				if(qir->unit)
					qir->unit->RecalculateWeight();
			}
		}
		delete* it;
	}
	questItemRequests.clear();
}

//=================================================================================================
void QuestManager::UpgradeQuests()
{
	for(Quest* quest : upgradeQuests)
	{
		Quest_Scripted* quest2 = new Quest_Scripted;
		quest2->questIndex = quest->questIndex;
		quest2->Upgrade(quest);
		if(quest->state == Quest::Hidden)
			unacceptedQuests[quest->questIndex] = quest2;
		else
			quests[quest->questIndex] = quest2;
		RemoveElementTry(questTimeouts, reinterpret_cast<Quest_Dungeon*>(quest));
		RemoveElementTry(questTimeouts2, quest);
		delete quest;
	}
}

void QuestManager::RemoveItemEventHandler(Quest2* quest, const Item* item)
{
	assert(quest && item);

	int index = 0;
	for(pair<Quest2*, const Item*>& p : itemEventHandlers)
	{
		if(p.first == quest && p.second == item)
		{
			RemoveElementIndex(itemEventHandlers, index);
			return;
		}
		++index;
	}
}

void QuestManager::CheckItemEventHandler(Unit* unit, const Item* item)
{
	if(itemEventHandlers.empty())
		return;

	for(pair<Quest2*, const Item*>& p : itemEventHandlers)
	{
		if(p.second == item)
		{
			ScriptEvent e(EVENT_USE);
			e.unit = unit;
			e.item = item;
			p.first->FireEvent(e);
			break;
		}
	}
}
