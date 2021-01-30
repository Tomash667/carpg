#pragma once

//-----------------------------------------------------------------------------
#include "GameCommon.h"

//-----------------------------------------------------------------------------
struct TeamInfo
{
	int players;
	int npcs;
	int heroes;
	int sane_heroes;
	int insane_heroes;
	int free_members;
	int summons;
};

//-----------------------------------------------------------------------------
class Team
{
public:
	struct TeamShareItem
	{
		Unit* from, *to;
		const Item* item;
		int index, priority;
		float value;
		bool is_team;
	};

	struct Investment
	{
		int questId, gold, days;
		string name;
	};

	void AddMember(Unit* unit, HeroType type);
	void RemoveMember(Unit* unit);
	Unit* FindActiveTeamMember(int id);
	bool FindItemInTeam(const Item* item, int quest_id, Unit** unit_result, int* i_index, bool check_npc = true);
	Unit* FindTeamMember(cstring id);
	uint GetActiveNpcCount();
	uint GetActiveTeamSize() { return active_members.size(); }
	Unit* GetLeader() const { return leader; }
	void GetLeaderRequest(Entity<Unit>* unit) { leader_requests.push_back(unit); }
	uint GetMaxSize() { return 8u; }
	uint GetNpcCount();
	Vec2 GetShare();
	Vec2 GetShare(int pc, int npc);
	Unit* GetRandomSaneHero();
	void GetTeamInfo(TeamInfo& info);
	uint GetPlayersCount();
	uint GetTeamSize() { return members.size(); }
	bool HaveActiveNpc();
	bool HaveQuestItem(const Item* item, int quest_id = -1) { return FindItemInTeam(item, quest_id, nullptr, nullptr, true); }
	bool HaveItem(const Item* item) { return FindItemInTeam(item, -1, nullptr, nullptr, true); }
	bool HaveNpc();
	bool HaveOtherActiveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveOtherPlayer();
	bool HaveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveClass(Class* clas) const;
	bool IsAnyoneAlive();
	bool IsBandit() const { return is_bandit; }
	bool IsLeader() const { return my_id == leader_id; }
	bool IsLeader(const Unit& unit) const { return &unit == GetLeader(); }
	bool IsLeader(const Unit* unit) const { assert(unit); return unit == GetLeader(); }
	bool IsTeamMember(Unit& unit);
	bool IsTeamNotBusy();
	void Clear(bool newGame);
	void Save(GameWriter& f);
	void SaveOnWorldmap(GameWriter& f);
	void Load(GameReader& f);
	void Update(int days, UpdateMode mode);
	void CheckTeamItemShares();
	void UpdateTeamItemShares();
	void TeamShareGiveItemCredit(DialogContext& ctx);
	void TeamShareSellItem(DialogContext& ctx);
	void TeamShareDecline(DialogContext& ctx);
	void BuyTeamItems();
	void CheckCredit(bool require_update = false, bool ignore = false);
	bool RemoveQuestItem(const Item* item, int quest_id = -1);
	Unit* FindPlayerTradingWithUnit(Unit& u);
	void AddLearningPoint(int count = 1);
	void AddExp(int exp, rvector<Unit>* units = nullptr);
	void AddExpS(int exp) { AddExp(exp); }
	void AddGold(int count, rvector<Unit>* to = nullptr, bool show = false, bool is_quest = false);
	void AddGoldS(int count) { AddGold(count, nullptr, true); }
	void AddReward(int gold, int exp = 0);
	void OnTravel(float dist);
	void CalculatePlayersLevel();
	uint RemoveItem(const Item* item, uint count);
	void SetBandit(bool is_bandit);
	Unit* GetNearestTeamMember(const Vec3& pos, float* dist = nullptr);
	bool IsAnyoneTalking() const;
	void Warp(const Vec3& pos, const Vec3& look_at);
	int GetStPoints() const;
	bool PersuasionCheck(int level);
	const vector<Investment>& GetInvestments() const { return investments; }
	bool HaveInvestment(int questId) const;
	void AddInvestment(cstring name, int questId, int gold, int days = 0);
	void UpdateInvestment(int questId, int gold);
	void WriteInvestments(BitStreamWriter& f);
	void ReadInvestments(BitStreamReader& f);

	rvector<Unit> members; // all team members
	rvector<Unit> active_members; // team members that get gold (without quest units)
	Unit* leader;
	int my_id, leader_id, players_level, free_recruits;
	bool crazies_attack, // team attacked by crazies on current level
		is_bandit, // attacked npc, now npc's are aggresive
		anyone_talking;

private:
	struct CheckResult
	{
		Entity<Unit> unit;
		int tries;
		bool success;
	};

	bool CheckTeamShareItem(TeamShareItem& tsi);
	void CheckUnitOverload(Unit& unit);

	//------------------
	// temporary - not saved
	vector<Entity<Unit>*> leader_requests;
	vector<int> pot_have;
	// team shares
	vector<TeamShareItem> team_shares;
	int team_share_id;
	// items to buy
	struct ItemToBuy
	{
		const Item* item;
		float ai_value;
		float priority;
	};
	vector<ItemToBuy> to_buy, to_buy2;
	//
	vector<CheckResult> checkResults;
	vector<Investment> investments;
};
