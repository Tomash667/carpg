#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"
#include "SaveState.h"
// to delete
#include "Game.h"

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
#include "Quest_RetrivePackage.h"
#include "Quest_Sawmill.h"
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Wanted.h"

//-----------------------------------------------------------------------------
QuestManager QM;

//=================================================================================================
// Save quests data
void QuestManager::Save(GameWriter& f)
{
	// accepted quests
	f << quests.size();
	for(Quest* quest : quests)
		quest->Save(f.file);

	// unaccepted quests
	f << unaccepted_quests.size();
	for(Quest* quest : unaccepted_quests)
		quest->Save(f.file);

	// quest timeouts
	f << quests_timeout.size();
	for(Quest_Dungeon* q : quests_timeout)
		f << q->refid;
	f << quests_timeout2.size();
	for(Quest* q : quests_timeout2)
		f << q->refid;

	// other quest variables
	f << quest_counter;
	f << unique_quests_completed;
	f << unique_completed_shown;
	f << unique_completed_can_show;
}

//=================================================================================================
// Load quests data
void QuestManager::Load(GameReader& f)
{
	// quests
	LoadQuests(f, quests);
	LoadQuests(f, unaccepted_quests);

	// quests timeouts
	quests_timeout.resize(f.Read<uint>());
	for(Quest_Dungeon*& q : quests_timeout)
		q = (Quest_Dungeon*)FindQuest(f.Read<uint>(), false);
	if(LOAD_VERSION >= V_0_4)
	{
		quests_timeout2.resize(f.Read<uint>());
		for(Quest*& q : quests_timeout2)
			q = FindQuest(f.Read<uint>(), false);
	}

	// old timed units (now removed)
	if(LOAD_VERSION > V_0_2 && LOAD_VERSION < V_0_4)
	{
		uint count;
		f >> count;
		f.Skip(sizeof(int) * 3 * count);
	}

	// other quest variables
	f >> quest_counter;
	if(LOAD_VERSION >= V_CURRENT)
	{
		f >> unique_quests_completed;
		f >> unique_completed_shown;
		f >> unique_completed_can_show;
	}
	else
	{
		f >> unique_quests_completed;
		f >> unique_completed_shown;
		unique_completed_can_show = false;
		if(LOAD_VERSION == V_0_2)
			unique_completed_shown = false;
	}
}

//=================================================================================================
// Load list of quests
void QuestManager::LoadQuests(GameReader& f, vector<Quest*>& v_quests)
{
	uint count;
	f >> count;
	v_quests.resize(count);
	for(uint i = 0; i<count; ++i)
	{
		QUEST quest_id;
		f >> quest_id;
		if(LOAD_VERSION == V_0_2)
		{
			if(quest_id > Q_EVIL)
				quest_id = (QUEST)(quest_id - 1);
		}

		Quest* quest = CreateQuest(quest_id);
		quest->quest_id = quest_id;
		quest->quest_index = i;
		quest->Load(f.file);
		v_quests[i] = quest;
	}
}

//=================================================================================================
// Write quests data for mp client
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
// Read quests data by mp client
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
// Reset quests data
void QuestManager::Reset()
{
	quest_counter = 0;
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	quests_timeout.clear();
	quests_timeout2.clear();
	DeleteElements(client_quest_items);

	unique_quests_completed = 0;
	unique_completed_shown = false;
	unique_completed_can_show = false;
}

//=================================================================================================
// Create quest and call start
Quest* QuestManager::CreateQuest(QUEST quest_id)
{
	Quest* quest = CreateQuestInstance(quest_id);
	quest->refid = quest_counter++;
	unaccepted_quests.push_back(quest);
	quest->Start();
	return quest;
}

//=================================================================================================
// Create quest instance
Quest* QuestManager::CreateQuestInstance(QUEST quest_id)
{
	switch(quest_id)
	{
	case Q_DELIVER_LETTER:
		return new Quest_DeliverLetter;
	case Q_DELIVER_PARCEL:
		return new Quest_DeliverParcel;
	case Q_SPREAD_NEWS:
		return new Quest_SpreadNews;
	case Q_RETRIVE_PACKAGE:
		return new Quest_RetrivePackage;
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
// Get random mayor quest
Quest* QuestManager::GetMayorQuest(int force)
{
	QUEST quest_id;
	if(force == -1)
	{
		switch(rand2() % 12)
		{
		case 0:
		case 1:
		case 2:
			quest_id = Q_DELIVER_LETTER;
			break;
		case 3:
		case 4:
		case 5:
			quest_id = Q_DELIVER_PARCEL;
			break;
		case 6:
		case 7:
			quest_id = Q_SPREAD_NEWS;
			break;
		case 8:
		case 9:
			quest_id = Q_RETRIVE_PACKAGE;
			break;
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
			quest_id = Q_DELIVER_LETTER;
			break;
		case 2:
			quest_id = Q_DELIVER_PARCEL;
			break;
		case 3:
			quest_id = Q_SPREAD_NEWS;
			break;
		case 4:
			quest_id = Q_RETRIVE_PACKAGE;
			break;
		}
	}

	return CreateQuest(quest_id);
}

//=================================================================================================
// Get random captain quest
Quest* QuestManager::GetCaptainQuest(int force)
{
	QUEST quest_id;
	if(force == -1)
	{
		switch(rand2() % 11)
		{
		case 0:
		case 1:
			quest_id = Q_RESCUE_CAPTIVE;
			break;
		case 2:
		case 3:
			quest_id = Q_BANDITS_COLLECT_TOLL;
			break;
		case 4:
		case 5:
			quest_id = Q_CAMP_NEAR_CITY;
			break;
		case 6:
		case 7:
			quest_id = Q_KILL_ANIMALS;
			break;
		case 8:
		case 9:
			quest_id = Q_WANTED;
			break;
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
			quest_id = Q_RESCUE_CAPTIVE;
			break;
		case 2:
			quest_id = Q_BANDITS_COLLECT_TOLL;
			break;
		case 3:
			quest_id = Q_CAMP_NEAR_CITY;
			break;
		case 4:
			quest_id = Q_KILL_ANIMALS;
			break;
		case 5:
			quest_id = Q_WANTED;
			break;
		}
	}

	return CreateQuest(quest_id);
}

//=================================================================================================
// Get random adventurer quest
Quest* QuestManager::GetAdventurerQuest(int force)
{
	QUEST quest_id;
	if(force == -1)
	{
		switch(rand2() % 3)
		{
		case 0:
		default:
			quest_id = Q_FIND_ARTIFACT;
			break;
		case 1:
			quest_id = Q_LOST_ARTIFACT;
			break;
		case 2:
			quest_id = Q_STOLEN_ARTIFACT;
			break;
		}
	}
	else
	{
		switch(force)
		{
		case 1:
		default:
			quest_id = Q_FIND_ARTIFACT;
			break;
		case 2:
			quest_id = Q_LOST_ARTIFACT;
			break;
		case 3:
			quest_id = Q_STOLEN_ARTIFACT;
			break;
		}
	}

	return CreateQuest(quest_id);
}

//=================================================================================================
// Find quest that need talk for selected topic
Quest* QuestManager::FindNeedTalkQuest(cstring topic, bool active)
{
	assert(topic);

	if(active)
	{
		for(Quest* quest : quests)
		{
			if(quest->IsActive() && quest->IfNeedTalk(topic))
				return quest;
		}
	}
	else
	{
		for(Quest* quest : quests)
		{
			if(quest->IfNeedTalk(topic))
				return quest;
		}

		for(Quest* quest : unaccepted_quests)
		{
			if(quest->IfNeedTalk(topic))
				return quest;
		}
	}

	return nullptr;
}

//=================================================================================================
// Find quest by refid
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
// Find quest by location and type
Quest* QuestManager::FindQuest(int loc, Quest::Type type)
{
	for(Quest* quest : quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
// Find quest by id
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
// Find unaccepted quest by location and type
Quest* QuestManager::FindUnacceptedQuest(int loc, Quest::Type type)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
// Find unaccepted quest by refid
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
// Add placeholder quest
void QuestManager::AddPlaceholderQuest(Quest* quest)
{
	assert(quest);

	quest->state = Quest::Started;
	quest->quest_index = quests.size();
	quests.push_back(quest);
}

//=================================================================================================
// Accept quest (set state and add timeout if selected)
void QuestManager::AcceptQuest(Quest* quest, int timeout)
{
	assert(quest);
	assert(in_range(timeout, 0, 2));

	quest->start_time = Game::Get().worldtime;
	quest->state = Quest::Started;
	quest->quest_index = quests.size();
	quests.push_back(quest);
	RemoveElement(unaccepted_quests, quest);

	if(timeout == 1)
		quests_timeout.push_back(checked_cast<Quest_Dungeon*>(quest));
	else if(timeout == 2)
		quests_timeout2.push_back(quest);
}

//=================================================================================================
// Remove quest timeout
void QuestManager::RemoveTimeout(Quest* quest, int timeout)
{
	assert(quest);
	assert(timeout == 1 || timeout == 2);

	if(timeout == 1)
		RemoveElementTry(quests_timeout, checked_cast<Quest_Dungeon*>(quest));
	else
		RemoveElementTry(quests_timeout2, quest);
}

//=================================================================================================
// Update quest timeouts (call quest function and remove if camp)
void QuestManager::UpdateTimeouts(int days)
{
	Game& game = Game::Get();

	// call timeouts attached to locations, remove timedout camps
	LoopAndRemove(quests_timeout, [&game](Quest_Dungeon* quest)
	{
		if(!quest->IsTimedout())
			return false;

		int target_loc = quest->target_loc;
		Location* loc = game.locations[target_loc];
		bool in_camp = (loc->type == L_CAMP && (target_loc == game.picked_location || target_loc == game.current_location));

		// call timeout function
		if(!quest->timeout)
		{
			if(!quest->OnTimeout(in_camp ? TIMEOUT_CAMP : TIMEOUT_NORMAL))
				return false;
			quest->timeout = true;
		}

		// don't delete camp that team is going to or is open
		if(in_camp)
			return false;

		loc->active_quest = nullptr;

		// remove camp
		if(loc->type == L_CAMP)
		{
			// send message to players
			if(game.IsOnline())
			{
				NetChange& c = Add1(game.net_changes);
				c.type = NetChange::REMOVE_CAMP;
				c.id = target_loc;
			}

			// cleanup location
			quest->target_loc = -1;
			OutsideLocation* outside = (OutsideLocation*)loc;
			DeleteElements(outside->chests);
			DeleteElements(outside->items);
			DeleteElements(outside->units);
			delete loc;

			// remove location from list
			if(target_loc + 1 == game.locations.size())
				game.locations.pop_back();
			else
			{
				game.locations[target_loc] = nullptr;
				++game.empty_locations;
			}
		}

		return true;
	});

	// apply quest timeouts, not attached to location
	LoopAndRemove(quests_timeout2, [](Quest* quest)
	{
		if(quest->IsTimedout() && quest->OnTimeout(TIMEOUT2))
		{
			quest->timeout = true;
			return true;
		}
		else
			return false;
	});
}

//=================================================================================================
// Add quest item request on loading
void QuestManager::AddQuestItemRequest(const Item** item, int quest_refid, vector<ItemSlot>* items, Unit* unit)
{
	assert(item && quest_refid != -1);
	QuestItemRequest qir;
	qir.item = item;
	qir.quest_refid = quest_refid;
	qir.items = items;
	qir.unit = unit;
	quest_item_requests.push_back(qir);
}

//=================================================================================================
// Apply quest item requests on loading
void QuestManager::ApplyQuestItemRequests()
{
	for(QuestItemRequest& qir : quest_item_requests)
	{
		*qir.item = FindQuestItem(qir.quest_refid);
		if(qir.items)
		{
			// check is sort is required
			bool ok = true;
			for(ItemSlot& slot : *qir.items)
			{
				if(slot.item == QUEST_ITEM_PLACEHOLDER)
				{
					ok = false;
					break;
				}
			}
			
			// sort
			if(ok)
			{
				if(LOAD_VERSION < V_0_2_10)
					RemoveNullItems(*qir.items);
				SortItems(*qir.items);
				if(qir.unit && LOAD_VERSION < V_0_2_10)
					qir.unit->RecalculateWeight();
			}
		}
	}
	quest_item_requests.clear();
}

//=================================================================================================
// Find quest item by refid
const Item* QuestManager::FindQuestItem(int refid)
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
// Find client quest item by refid and name
Item* QuestManager::FindClientQuestItem(int refid, cstring id)
{
	assert(refid != -1 && id != nullptr);

	for(Item* item : client_quest_items)
	{
		if(item->refid == refid && item->id == id)
			return item;
	}

	return nullptr;
}

//=================================================================================================
inline bool ReadItemSimple(BitStream& stream, const Item*& item)
{
	if(!ReadString1(stream))
		return false;

	if(BUF[0] == '$')
		item = FindItem(BUF + 1);
	else
		item = FindItem(BUF);

	return (item != nullptr);
}

//=================================================================================================
// Read client quest items in mp loading
bool QuestManager::ReadClientQuestItems(BitStream& stream)
{
	const int QUEST_ITEM_MIN_SIZE = 7;
	word quest_items_count;
	if(!stream.Read(quest_items_count)
		|| !EnsureSize(stream, QUEST_ITEM_MIN_SIZE * quest_items_count))
	{
		ERROR("Read world: Broken packet for quest items.");
		return false;
	}

	client_quest_items.reserve(quest_items_count);
	for(word i = 0; i < quest_items_count; ++i)
	{
		const Item* base_item;
		if(!ReadItemSimple(stream, base_item))
		{
			ERROR(Format("Read world: Broken packet for quest item %u.", i));
			return false;
		}
		else
		{
			Item* item = CreateItemCopy(base_item);
			if(!ReadString1(stream, item->id)
				|| !ReadString1(stream, item->name)
				|| !ReadString1(stream, item->desc)
				|| !stream.Read(item->refid))
			{
				ERROR(Format("Read world: Broken packet for quest item %u (2).", i));
				delete item;
				return false;
			}
			else
				client_quest_items.push_back(item);
		}
	}

	return true;
}

//=================================================================================================
// Get quest entry for selected quest
const QuestEntry& QuestManager::GetQuestEntry(int index)
{
	static QuestEntry entry;
	assert(index < (int)quests.size());
	Quest* quest = quests[index];
	entry.state = quest->state;
	entry.name = quest->name.c_str();
	entry.msgs = &quest->msgs;
	return entry;
}
