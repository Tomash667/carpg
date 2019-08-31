#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "Scrollbar.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class TeamPanel : public GamePanel
{
public:
	TeamPanel();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show();
	void Hide();
	void Changed();

private:
	void UpdateButtons();
	void GiveGold(Unit* target);
	void ChangeLeader(Unit* target);
	void Kick(Unit* target);
	void OnPayCredit(int id);
	void OnGiveGold(int id);
	void OnKick(int id);

	Scrollbar scrollbar;
	Button bt[4];
	cstring txTeam, txCharInTeam, txPing, txDays, txPickCharacter, txNoCredit, txPayCreditAmount, txNotEnoughGold, txPaidCredit, txPaidCreditPart,
		txGiveGoldSelf, txGiveGoldAmount, txOnlyPcLeader, txAlreadyLeader, txYouAreLeader, txCantChangeLeader, txPcAlreadyLeader, txPcIsLeader,
		txCantKickMyself, txCantKickAi, txReallyKick, txAlreadyLeft, txGiveGoldRefuse;
	int counter, mode, picked;
	TexturePtr tCrown, tSkull;
	vector<Hitbox> hitboxes;
	Entity<Unit> target_unit;
	bool picking;
};
