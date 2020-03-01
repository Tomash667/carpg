#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"

//-----------------------------------------------------------------------------
class Quest_Artifacts;
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
		int quest_id;
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
	void AddQuestItemRequest(const Item** item, cstring name, int quest_id, vector<ItemSlot>* items, Unit* unit = nullptr);
	void Reset();
	void Clear();
	void Update(int days);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	Quest* FindQuest(int location, QuestCategory category);
	Quest* FindQuest(int id, bool active = true);
	Quest* FindAnyQuest(int id);
	Quest* FindAnyQuest(QuestScheme* scheme);
	Quest* FindQuest(QUEST_TYPE type);
	Quest* FindUnacceptedQuest(int location, QuestCategory category);
	Quest* FindUnacceptedQuest(int id);
	Quest* FindQuestS(const string& quest_id);
	const Item* FindQuestItem(cstring name, int quest_id);
	void EndUniqueQuest() { ++unique_quests_completed; }
	bool SetForcedQuest(const string& name);
	int GetForcedQuest() const { return force; }
	const vector<QuestInfo>& GetQuestInfos() const { return infos; }
	void RegisterSpecialHandler(QuestHandler* handler, cstring msg) { special_handlers[msg] = handler; }
	void RegisterSpecialIfHandler(QuestHandler* handler, cstring msg) { special_if_handlers[msg] = handler; }
	void RegisterFormatString(QuestHandler* handler, cstring msg) { format_str_handlers[msg] = handler; }
	bool HandleSpecial(DialogContext& ctx, cstring msg, bool& result);
	bool HandleSpecialIf(DialogContext& ctx, cstring msg, bool& result);
	bool HandleFormatString(const string& str, cstring& result);
	const Item* FindQuestItemClient(cstring item_id, int quest_id) const;
	void AddScriptedQuest(QuestScheme* scheme);
	QuestInfo* FindQuest(const string& id);
	void AddQuestRequest(int id, Quest** quest, delegate<void()> callback = nullptr) { quest_requests.push_back({ id, quest, callback }); }
	void AddQuestItem(Item* item) { quest_items.push_back(item); }
	bool HaveQuestRumors() const { return !quest_rumors.empty(); }
	int AddQuestRumor(cstring str);
	void AddQuestRumor(int id, cstring str) { quest_rumors.push_back(pair<int, string>(id, str)); }
	bool RemoveQuestRumor(int id);
	string GetRandomQuestRumor();
	void GenerateQuestUnits(bool on_enter);
	void UpdateQuests(int days);
	void RemoveQuestUnits(bool on_leave);
	void HandleQuestEvent(Quest_Event* event);
	void UpdateQuestsLocal(float dt);
	void ProcessQuestRequests();
	void UpgradeQuests();

	vector<Quest*> unaccepted_quests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> quests_timeout;
	vector<Quest*> quests_timeout2;
	vector<Item*> quest_items;
	Quest_Sawmill* quest_sawmill;
	Quest_Mine* quest_mine;
	Quest_Bandits* quest_bandits;
	Quest_Mages* quest_mages;
	Quest_Mages2* quest_mages2;
	Quest_Orcs* quest_orcs;
	Quest_Orcs2* quest_orcs2;
	Quest_Goblins* quest_goblins;
	Quest_Evil* quest_evil;
	Quest_Crazies* quest_crazies;
	Quest_Contest* quest_contest;
	Quest_Secret* quest_secret;
	Quest_Tournament* quest_tournament;
	Quest_Tutorial* quest_tutorial;
	Quest_Artifacts* quest_artifacts;
	int quest_counter;
	int unique_quests, unique_quests_completed;
	bool unique_completed_show;
	cstring txRumorQ[9];

private:
	void LoadQuests(GameReader& f, vector<Quest*>& quests);
	QuestInfo* GetRandomQuest(QuestCategory category);

	vector<QuestInfo> infos;
	vector<QuestItemRequest*> quest_item_requests;
	vector<QuestRequest> quest_requests;
	vector<Quest*> upgrade_quests;
	int force;
	QuestList* quests_mayor, *quests_captain, *quests_random;
	std::map<string, QuestHandler*> special_handlers, special_if_handlers, format_str_handlers;
	string tmp_str;
	vector<pair<int, string>> quest_rumors;
};
