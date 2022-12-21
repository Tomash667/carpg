#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"

//-----------------------------------------------------------------------------
class Quest_Bandits;
class Quest_Contest;
class Quest_Crazies;
class Quest_Evil;
class Quest_Goblins;
class Quest_Mages;
class Quest_Mages2;
class Quest_Mine;
class Quest_Orcs;
class Quest_Orcs2;
class Quest_Sawmill;
class Quest_Secret;
class Quest_Tournament;
class Quest_Tutorial;

//-----------------------------------------------------------------------------
struct QuestInfo
{
	QUEST_TYPE type;
	QuestCategory category;
	cstring name;
	QuestScheme* scheme;

	QuestInfo(QUEST_TYPE type, QuestCategory category, cstring name, QuestScheme* scheme = nullptr)
		: type(type), category(category), name(name), scheme(scheme) {}
};

//-----------------------------------------------------------------------------
class QuestManager
{
	struct QuestRequest
	{
		int id;
		Quest** quest;
		delegate<void()> callback;
	};

	struct QuestItemRequest
	{
		const Item** item;
		string name;
		int questId;
		vector<ItemSlot>* items;
		Unit* unit;
	};

public:
	QuestManager();
	~QuestManager();
	void Init();
	void InitLists();
	void LoadLanguage();
	void InitQuests();
	Quest* CreateQuest(QUEST_TYPE type);
	Quest* CreateQuest(QuestInfo* info);
	Quest* GetMayorQuest();
	Quest* GetCaptainQuest();
	Quest* GetAdventurerQuest();
	void AddQuestItemRequest(const Item** item, cstring name, int questId, vector<ItemSlot>* items, Unit* unit = nullptr);
	void Reset();
	void Clear();
	void Update(int days);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	Quest* FindQuest(Location* location, QuestCategory category);
	Quest* FindQuest(int id, bool active = true);
	Quest* FindAnyQuest(int id);
	Quest* FindAnyQuest(QuestScheme* scheme);
	Quest* FindQuest(QUEST_TYPE type);
	Quest* FindUnacceptedQuest(Location* location, QuestCategory category);
	Quest* FindUnacceptedQuest(int id);
	Quest* FindQuestS(const string& questId);
	const Item* FindQuestItem(cstring name, int questId);
	void EndUniqueQuest() { ++uniqueQuestsCompleted; }
	bool SetForcedQuest(const string& name);
	int GetForcedQuest() const { return force; }
	const vector<QuestInfo>& GetQuestInfos() const { return infos; }
	void RegisterSpecialHandler(QuestHandler* handler, cstring msg) { specialHandlers[msg] = handler; }
	void RegisterSpecialIfHandler(QuestHandler* handler, cstring msg) { specialIfHandlers[msg] = handler; }
	void RegisterFormatString(QuestHandler* handler, cstring msg) { formatStrHandlers[msg] = handler; }
	bool HandleSpecial(DialogContext& ctx, cstring msg, bool& result);
	bool HandleSpecialIf(DialogContext& ctx, cstring msg, bool& result);
	bool HandleFormatString(const string& str, cstring& result);
	const Item* FindQuestItemClient(cstring itemId, int questId) const;
	void AddScriptedQuest(QuestScheme* scheme);
	QuestInfo* FindQuestInfo(QUEST_TYPE type);
	QuestInfo* FindQuestInfo(const string& id);
	void AddQuestRequest(int id, Quest** quest, delegate<void()> callback = nullptr) { questRequests.push_back({ id, quest, callback }); }
	void AddQuestItem(Item* item) { questItems.push_back(item); }
	bool HaveQuestRumors() const { return !questRumors.empty(); }
	int AddQuestRumor(cstring str);
	void AddQuestRumor(int id, cstring str) { questRumors.push_back(pair<int, string>(id, str)); }
	bool RemoveQuestRumor(int id);
	string GetRandomQuestRumor();
	void GenerateQuestUnits(bool onEnter);
	void RemoveQuestUnits(bool onLeave);
	void HandleQuestEvent(Quest_Event* event);
	void UpdateQuestsLocal(float dt);
	void ProcessQuestRequests();
	void UpgradeQuests();
	vector<Location*>& GetUsedCities() { return used; }
	void AddItemEventHandler(Quest2* quest, const Item* item) { itemEventHandlers.push_back(std::make_pair(quest, item)); }
	void RemoveItemEventHandler(Quest2* quest, const Item* item);
	void CheckItemEventHandler(Unit* unit, const Item* item);

	vector<Quest*> unacceptedQuests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> questTimeouts;
	vector<Quest*> questTimeouts2;
	vector<Item*> questItems;
	Quest_Sawmill* questSawmill;
	Quest_Mine* questMine;
	Quest_Bandits* questBandits;
	Quest_Mages* questMages;
	Quest_Mages2* questMages2;
	Quest_Orcs* questOrcs;
	Quest_Orcs2* questOrcs2;
	Quest_Goblins* questGoblins;
	Quest_Evil* questEvil;
	Quest_Crazies* questCrazies;
	Quest_Contest* questContest;
	Quest_Secret* questSecret;
	Quest_Tournament* questTournament;
	Quest_Tutorial* questTutorial;
	int uniqueQuests, uniqueQuestsCompleted;
	bool uniqueCompletedShow;
	cstring txQuest[268], txForMayor, txForSoltys, txRumorQ[9];

private:
	void LoadQuests(GameReader& f, vector<Quest*>& quests);
	QuestInfo* GetRandomQuest(QuestCategory category);

	vector<QuestInfo> infos;
	vector<QuestItemRequest*> questItemRequests;
	vector<QuestRequest> questRequests;
	vector<Quest*> upgradeQuests;
	int questCounter, force;
	QuestList* questsMayor, *questsCaptain, *questsRandom;
	std::map<string, QuestHandler*> specialHandlers, specialIfHandlers, formatStrHandlers;
	string tmpStr;
	vector<pair<int, string>> questRumors;
	vector<Location*> used;
	vector<pair<Quest2*, const Item*>> itemEventHandlers;
};
