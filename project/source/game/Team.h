#pragma once

struct Item;
struct Unit;

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

class TeamSingleton
{
public:
	struct TeamShareItem
	{
		Unit* from, *to;
		const Item* item;
		int index, value, priority;
		bool is_team;
	};

	void AddTeamMember(Unit* unit, bool free);
	void RemoveTeamMember(Unit* unit);
	Unit* FindActiveTeamMember(int netid);
	bool FindItemInTeam(const Item* item, int refid, Unit** unit_result, int* i_index, bool check_npc = true);
	Unit* FindTeamMember(cstring id);
	uint GetActiveNpcCount();
	uint GetActiveTeamSize() { return active_members.size(); }
	Unit* GetLeader() { return leader; }
	uint GetMaxSize() { return 8u; }
	uint GetNpcCount();
	Vec2 GetShare();
	Vec2 GetShare(int pc, int npc);
	Unit* GetRandomSaneHero();
	void GetTeamInfo(TeamInfo& info);
	uint GetPlayersCount();
	uint GetTeamSize() { return members.size(); }
	bool HaveActiveNpc();
	bool HaveQuestItem(const Item* item, int refid = -1) { return FindItemInTeam(item, refid, nullptr, nullptr, true); }
	bool HaveNpc();
	bool HaveOtherActiveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveOtherPlayer();
	bool HaveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool IsAnyoneAlive();
	bool IsLeader(const Unit& unit) { return &unit == GetLeader(); }
	bool IsLeader(const Unit* unit) { assert(unit); return unit == GetLeader(); }
	bool IsTeamMember(Unit& unit);
	bool IsTeamNotBusy();
	void Load(GameReader& f);
	void Reset();
	void ClearOnNewGameOrLoad();
	void Save(GameWriter& f);
	void SaveOnWorldmap(GameWriter& f);
	void Update(int days, bool travel);
	void CheckTeamItemShares();
	void UpdateTeamItemShares();
	void TeamShareGiveItemCredit(DialogContext& ctx);
	void TeamShareSellItem(DialogContext& ctx);
	void TeamShareDecline(DialogContext& ctx);
	void BuyTeamItems();
	void ValidateTeamItems();
	void CheckCredit(bool require_update = false, bool ignore = false);

	vector<Unit*> members; // all team members
	vector<Unit*> active_members; // team members that get gold (without quest units)
	Unit* leader;
	bool crazies_attack, // team attacked by crazies on current level
		free_recruit, // first hero joins for free if playing alone
		is_bandit; // attacked npc, now npc's are aggresive

private:
	bool CheckTeamShareItem(TeamShareItem& tsi);
	void CheckUnitOverload(Unit& unit);

	// team shares, not saved
	vector<TeamShareItem> team_shares;
	int team_share_id;
};

extern TeamSingleton Team;
