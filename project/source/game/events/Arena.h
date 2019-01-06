#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "QuestHandler.h"

//-----------------------------------------------------------------------------
class Arena : public GameComponent, public QuestHandler
{
public:
	enum Mode
	{
		NONE,
		FIGHT,
		PVP,
		TOURNAMENT
	};

	enum State
	{
		WAITING_TO_WARP,
		WAITING_TO_START,
		IN_PROGRESS,
		WAITING_TO_END,
		WAITING_TO_EXIT
	};

	struct PvpResponse
	{
		Unit* from, *to;
		float timer;
		bool ok;
	};

	struct Enemy
	{
		UnitData* unit;
		uint count;
		int level;
		bool side;
	};

	void InitOnce() override;
	void LoadLanguage() override;
	void Cleanup() override { delete this; }
	void Start(Mode mode);
	void Reset();
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void SpawnArenaViewers(int count);
	void Clean();
	void RemoveArenaViewers();
	void Verify();
	void UpdatePvpRequest(float dt);
	void StartArenaCombat(int level);
	void HandlePvpResponse(PlayerInfo& info, bool accepted);
	void StartPvp(PlayerController* player, Unit* unit);
	void AddReward(int gold, int exp);
	void Update(float dt);
	Unit* GetRandomArenaHero();
	void PvpEvent(int id);
	void ClosePvpDialog();
	void ShowPvpRequest(Unit* unit);
	void RewardExp(Unit* dead_unit);
	void SpawnUnit(const vector<Enemy>& units);

	Mode mode;
	State state;
	int difficulty; // 1-3
	int result; // winner index 0-player, 1-enemy (in pvp/tournament unit index)
	float timer;
	vector<Unit*> units, near_players, viewers;
	vector<string> near_players_str;
	Unit* fighter, *pvp_unit;
	PlayerController* pvp_player;
	PvpResponse pvp_response;
	DialogBox* dialog_pvp;
	bool free;

private:
	cstring txPvp, txPvpWith, txPvpTooFar, txArenaText[3], txArenaTextU[5];
};
