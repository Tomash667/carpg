#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
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
struct QuestItemRequest
{
	const Item** item;
	string name;
	int quest_refid;
	vector<ItemSlot>* items;
	Unit* unit;
};

//-----------------------------------------------------------------------------
struct QuestInfo
{
	QUEST id;
	QuestType type;
	cstring name;
};

//-----------------------------------------------------------------------------
class QuestManager : public GameComponent
{
public:
	void InitOnce() override;
	void LoadLanguage() override;
	void Cleanup() override;
	void InitQuests(bool devmode);
	Quest* CreateQuest(QUEST quest_id);
	Quest* GetMayorQuest();
	Quest* GetCaptainQuest();
	Quest* GetAdventurerQuest();
	void AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit = nullptr);
	void Reset();
	void Clear();
	void Update(int days);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	Quest* FindQuest(int location, QuestType type);
	Quest* FindQuest(int refid, bool active = true);
	Quest* FindQuestById(QUEST quest_id);
	Quest* FindUnacceptedQuest(int location, QuestType type);
	Quest* FindUnacceptedQuest(int refid);
	const Item* FindQuestItem(cstring name, int refid);
	void EndUniqueQuest();
	bool RemoveQuestRumor(PLOTKA_QUESTOWA rumor_id);
	bool SetForcedQuest(const string& name);
	QUEST GetForcedQuest() const { return force; }
	const vector<QuestInfo>& GetQuestInfos() const { return infos; }
	void RegisterSpecialHandler(QuestHandler* handler, cstring msg) { special_handlers[msg] = handler; }
	void RegisterSpecialIfHandler(QuestHandler* handler, cstring msg) { special_if_handlers[msg] = handler; }
	void RegisterFormatString(QuestHandler* handler, cstring msg) { format_str_handlers[msg] = handler; }
	bool HandleSpecial(DialogContext& ctx, cstring msg, bool& result) { return HandleSpecialGeneric(special_handlers, ctx, msg, result); }
	bool HandleSpecialIf(DialogContext& ctx, cstring msg, bool& result) { return HandleSpecialGeneric(special_if_handlers, ctx, msg, result); }
	bool HandleFormatString(const string& str, cstring& result);

	vector<Quest*> unaccepted_quests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> quests_timeout;
	vector<Quest*> quests_timeout2;
	vector<QuestItemRequest*> quest_item_requests;
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
	int quest_counter;
	int unique_quests_completed;
	bool unique_completed_show;
	int quest_rumor_counter;
	bool quest_rumor[P_MAX];

private:
	void LoadQuests(GameReader& f, vector<Quest*>& quests);
	QUEST GetRandomQuest(QuestType type);
	bool HandleSpecialGeneric(std::map<string, QuestHandler*>& handlers, DialogContext& ctx, cstring msg, bool& result);

	vector<QuestInfo> infos;
	QUEST force;
	std::map<string, QuestHandler*> special_handlers, special_if_handlers, format_str_handlers;
	string tmp_str;
};
extern QuestManager QM;
