#include "Pch.h"
#include "Base.h"
#include "BitStreamFunc.h"
#include "QuestManager.h"
#include "SaveState.h"

#include "Quest_Bandits.h"
#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
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
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Wanted.h"

QuestManager Singleton<QuestManager>::instance;

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
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Quest* QuestManager::GetMayorQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 12)
		{
		case 0:
		case 1:
		case 2:
			return new Quest_DeliverLetter;
		case 3:
		case 4:
		case 5:
			return new Quest_DeliverParcel;
		case 6:
		case 7:
			return new Quest_SpreadNews;
			break;
		case 8:
		case 9:
			return new Quest_RetrievePackage;
		case 10:
		case 11:
		default:
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_DeliverLetter;
		case 2:
			return new Quest_DeliverParcel;
		case 3:
			return new Quest_SpreadNews;
		case 4:
			return new Quest_RetrievePackage;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 11)
		{
		case 0:
		case 1:
			return new Quest_RescueCaptive;
		case 2:
		case 3:
			return new Quest_BanditsCollectToll;
		case 4:
		case 5:
			return new Quest_CampNearCity;
		case 6:
		case 7:
			return new Quest_KillAnimals;
		case 8:
		case 9:
			return new Quest_Wanted;
		case 10:
		default:
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_RescueCaptive;
		case 2:
			return new Quest_BanditsCollectToll;
		case 3:
			return new Quest_CampNearCity;
		case 4:
			return new Quest_KillAnimals;
		case 5:
			return new Quest_Wanted;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 3)
		{
		case 0:
		default:
			return new Quest_FindArtifact;
		case 1:
			return new Quest_LostArtifact;
		case 2:
			return new Quest_StolenArtifact;
		}
	}
	else
	{
		switch(force)
		{
		case 1:
		default:
			return new Quest_FindArtifact;
		case 2:
			return new Quest_LostArtifact;
		case 3:
			return new Quest_StolenArtifact;
		}
	}
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
void QuestManager::Cleanup()
{
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_item_requests);
}

//=================================================================================================
void QuestManager::Write(BitStream& stream)
{
	stream.WriteCasted<word>(quests.size());
	for(Quest* quest : quests)
	{
		stream.Write(quest->refid);
		stream.WriteCasted<byte>(quest->state);
		WriteString1(stream, quest->name);
		WriteStringArray<byte, word>(stream, quest->msgs);
	}
}

//=================================================================================================
bool QuestManager::Read(BitStream& stream)
{
	const int QUEST_MIN_SIZE = sizeof(int) + sizeof(byte) * 3;
	word quest_count;
	if(!stream.Read(quest_count)
		|| !EnsureSize(stream, QUEST_MIN_SIZE * quest_count))
	{
		ERROR("Read world: Broken packet for quests.");
		return false;
	}
	quests.resize(quest_count);

	int index = 0;
	for(Quest*& quest : quests)
	{
		quest = new PlaceholderQuest;
		quest->quest_index = index;
		if(!stream.Read(quest->refid) ||
			!stream.ReadCasted<byte>(quest->state) ||
			!ReadString1(stream, quest->name) ||
			!ReadStringArray<byte, word>(stream, quest->msgs))
		{
			ERROR(Format("Read world: Broken packet for quest %d.", index));
			return false;
		}
		++index;
	}

	return true;
}

//=================================================================================================
void QuestManager::Save(HANDLE file)
{
	FileWriter f(file);

	f << quests.size();
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		(*it)->Save(file);

	f << unaccepted_quests.size();
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
		(*it)->Save(file);

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
}

//=================================================================================================
void QuestManager::Load(HANDLE file)
{
	FileReader f(file);

	LoadQuests(file, quests);
	LoadQuests(file, unaccepted_quests);

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
}

//=================================================================================================
void QuestManager::LoadQuests(HANDLE file, vector<Quest*>& quests)
{
	DWORD tmp;

	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	quests.resize(count);
	for(uint i = 0; i<count; ++i)
	{
		QUEST quest_type;
		ReadFile(file, &quest_type, sizeof(quest_type), &tmp, nullptr);

		Quest* quest = CreateQuest(quest_type);
		quest->quest_id = quest_type;
		quest->quest_index = i;
		quest->Load(file);
		quests[i] = quest;
	}
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
