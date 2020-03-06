#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include <Scrollbar.h>
#include <Button.h>
#include <TooltipController.h>

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
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);

	TooltipController tooltip;
	Scrollbar scrollbar;
	Button bt[5];
	cstring txTeam, txCharInTeam, txPing, txDays, txPickCharacter, txNoCredit, txPayCreditAmount, txNotEnoughGold, txPaidCredit, txPaidCreditPart,
		txGiveGoldSelf, txGiveGoldAmount, txOnlyPcLeader, txAlreadyLeader, txYouAreLeader, txCantChangeLeader, txPcAlreadyLeader, txPcIsLeader,
		txCantKickMyself, txCantKickAi, txReallyKick, txAlreadyLeft, txGiveGoldRefuse, txLeader, txUnconscious, txFastTravelTooltip, txChangeLeaderTooltip,
		txKickTooltip, txFastTravel, txCancelFastTravel, txWaiting, txReady;
	int counter, mode, picked;
	TexturePtr tCrown, tSkull, tFastTravelWait, tFastTravelOk;
	vector<Hitbox> hitboxes;
	Entity<Unit> target_unit;
	bool picking;
};
