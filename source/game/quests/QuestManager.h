#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"

//-----------------------------------------------------------------------------
struct Item;
struct ItemSlot;
struct Quest;
struct Quest_Dungeon;
struct Unit;

//-----------------------------------------------------------------------------
struct QuestItemRequest
{
	const Item** item;
	string name;
	int quest_refid;
	vector<ItemSlot>* items;
	Unit* unit;
};

//-----------------------------------------------------------------------------
class QuestManager : public Singleton<QuestManager>
{
public:
	Quest* CreateQuest(QUEST quest_id);
	Quest* GetMayorQuest(int force = -1);
	Quest* GetCaptainQuest(int force = -1);
	Quest* GetAdventurerQuest(int force = -1);
	void AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit = nullptr);
	void Reset();
	void Cleanup();
	void Write(BitStream& stream);
	bool Read(BitStream& stream);
	void Save(HANDLE file);
	void Load(HANDLE file);
	Quest* FindQuest(int location, QuestType type);
	Quest* FindQuest(int refid, bool active = true);
	Quest* FindQuestById(QUEST quest_id);
	Quest* FindUnacceptedQuest(int location, QuestType type);
	Quest* FindUnacceptedQuest(int refid);
	const Item* FindQuestItem(cstring name, int refid);
	void EndUniqueQuest();
	bool RemoveQuestRumor(PLOTKA_QUESTOWA rumor_id);

	vector<Quest*> unaccepted_quests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> quests_timeout;
	vector<Quest*> quests_timeout2;
	vector<QuestItemRequest*> quest_item_requests;
	int quest_counter;
	int unique_quests_completed;
	bool unique_completed_show;
	int quest_rumor_counter;
	bool quest_rumor[P_MAX];

private:
	void LoadQuests(HANDLE file, vector<Quest*>& quests);
};
