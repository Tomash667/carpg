#pragma once

//-----------------------------------------------------------------------------
#include "GameCommon.h"

//-----------------------------------------------------------------------------
struct TeamInfo
{
	int players;
	int npcs;
	int heroes;
	int saneHeroes;
	int insaneHeroes;
	int freeMembers;
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
		bool isTeam;
	};

	struct Investment
	{
		int questId, gold, days;
		string name;
	};

	void AddMember(Unit* unit, HeroType type);
	void RemoveMember(Unit* unit);
	Unit* FindActiveTeamMember(int id);
	bool FindItemInTeam(const Item* item, int questId, Unit** unitResult, int* outIndex, bool checkNpc = true);
	Unit* FindTeamMember(cstring id);
	uint GetActiveNpcCount();
	uint GetActiveTeamSize() { return activeMembers.size(); }
	Unit* GetLeader() const { return leader; }
	uint GetMaxSize() { return 8u; }
	uint GetNpcCount();
	Vec2 GetShare();
	Vec2 GetShare(int pc, int npc);
	Unit* GetRandomSaneHero();
	void GetTeamInfo(TeamInfo& info);
	uint GetPlayersCount();
	uint GetTeamSize() { return members.size(); }
	bool HaveActiveNpc();
	bool HaveQuestItem(const Item* item, int questId = -1) { return FindItemInTeam(item, questId, nullptr, nullptr, true); }
	bool HaveItem(const Item* item) { return FindItemInTeam(item, -1, nullptr, nullptr, true); }
	bool HaveNpc();
	bool HaveOtherActiveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveOtherPlayer();
	bool HaveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveClass(Class* clas) const;
	bool IsAnyoneAlive();
	bool IsBandit() const { return isBandit; }
	bool IsLeader() const { return myId == leaderId; }
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
	void CheckCredit(bool requireUpdate = false, bool ignore = false);
	bool RemoveQuestItem(const Item* item, int questId = -1);
	Unit* FindPlayerTradingWithUnit(Unit& u);
	void AddLearningPoint(int count = 1);
	void AddExp(int exp, rvector<Unit>* units = nullptr);
	void AddExpS(int exp) { AddExp(exp); }
	void AddGold(int count, rvector<Unit>* to = nullptr, bool show = false, bool isQuest = false);
	void AddGoldS(int count) { AddGold(count, nullptr, true); }
	void AddReward(int gold, int exp = 0);
	void OnTravel(float dist);
	void CalculatePlayersLevel();
	uint RemoveItem(const Item* item, uint count);
	void SetBandit(bool isBandit);
	Unit* GetNearestTeamMember(const Vec3& pos, float* dist = nullptr);
	bool IsAnyoneTalking() const;
	void Warp(const Vec3& pos, const Vec3& lookAt);
	int GetStPoints() const;
	bool PersuasionCheck(int level);
	const vector<Investment>& GetInvestments() const { return investments; }
	bool HaveInvestment(int questId) const;
	void AddInvestment(cstring name, int questId, int gold, int days = 0);
	void UpdateInvestment(int questId, int gold);
	void WriteInvestments(BitStreamWriter& f);
	void ReadInvestments(BitStreamReader& f);
	void ShortRest();

	rvector<Unit> members; // all team members
	rvector<Unit> activeMembers; // team members that get gold (without quest units)
	Unit* leader;
	int myId, leaderId, playersLevel, freeRecruits;
	bool craziesAttack, // team attacked by crazies on current level
		isBandit, // attacked npc, now npc's are aggresive
		anyoneTalking;

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
	vector<int> potHave;
	// team shares
	vector<TeamShareItem> teamShares;
	int teamShareId;
	// items to buy
	struct ItemToBuy
	{
		const Item* item;
		float aiValue;
		float priority;
	};
	vector<ItemToBuy> toBuy, toBuy2;
	//
	vector<CheckResult> checkResults;
	vector<Investment> investments;
};
