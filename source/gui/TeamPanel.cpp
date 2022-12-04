#include "Pch.h"
#include "TeamPanel.h"

#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "Language.h"
#include "Level.h"
#include "MpBox.h"
#include "PlayerInfo.h"
#include "Team.h"
#include "Unit.h"

#include <GetNumberDialog.h>
#include <ResourceManager.h>
#include <SoundManager.h>

//-----------------------------------------------------------------------------
enum ButtonId
{
	Bt_FastTravel,
	Bt_GiveGold,
	Bt_PayCredit,
	Bt_Leader,
	Bt_Kick
};

enum ButtonEvent
{
	BtEvent_FastTravel = GuiEvent_Custom,
	BtEvent_GiveGold,
	BtEvent_PayCredit,
	BtEvent_Leader,
	BtEvent_Kick
};

enum TooltipGroup
{
	G_NONE = -1,
	G_LEADER,
	G_UNCONSCIOUS,
	G_CLASS,
	G_BUTTON,
	G_READY,
	G_WAITING
};

//=================================================================================================
TeamPanel::TeamPanel()
{
	visible = false;

	bt[Bt_FastTravel].id = BtEvent_FastTravel;
	bt[Bt_GiveGold].id = BtEvent_GiveGold;
	bt[Bt_PayCredit].id = BtEvent_PayCredit;
	bt[Bt_Leader].id = BtEvent_Leader;
	bt[Bt_Kick].id = BtEvent_Kick;
	for(int i = 0; i < 5; ++i)
		bt[i].parent = this;

	UpdateButtons();
	tooltip.Init(TooltipController::Callback(this, &TeamPanel::GetTooltip));
}

//=================================================================================================
void TeamPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("TeamPanel");

	bt[Bt_FastTravel].text = s.Get("fastTravel");
	bt[Bt_GiveGold].text = s.Get("giveGold");
	bt[Bt_PayCredit].text = s.Get("payCredit");
	bt[Bt_Leader].text = s.Get("changeLeader");
	bt[Bt_Kick].text = s.Get("kick");

	txTeam = s.Get("team");
	txCharInTeam = s.Get("charInTeam");
	txPing = s.Get("charPing");
	txDays = s.Get("charDays");
	txPickCharacter = s.Get("pickCharacter");
	txNoCredit = s.Get("noCredit");
	txPayCreditAmount = s.Get("payCreditAmount");
	txNotEnoughGold = s.Get("notEnoughGold");
	txPaidCredit = s.Get("paidCredit");
	txPaidCreditPart = s.Get("paidCreditPart");
	txGiveGoldSelf = s.Get("giveGoldSelf");
	txGiveGoldAmount = s.Get("giveGoldAmount");
	txOnlyPcLeader = s.Get("onlyPcLeader");
	txAlreadyLeader = s.Get("alreadyLeader");
	txYouAreLeader = s.Get("youAreLeader");
	txCantChangeLeader = s.Get("cantChangeLeader");
	txPcAlreadyLeader = s.Get("pcAlreadyLeader");
	txPcIsLeader = s.Get("pcIsLeader");
	txCantKickMyself = s.Get("cantKickMyself");
	txCantKickAi = s.Get("cantKickAi");
	txReallyKick = s.Get("reallyKick");
	txAlreadyLeft = s.Get("alreadyLeft");
	txGiveGoldRefuse = s.Get("giveGoldRefuse");
	txLeader = s.Get("leader");
	txUnconscious = s.Get("unconscious");
	txFastTravelTooltip = s.Get("fastTravelTooltip");
	txChangeLeaderTooltip = s.Get("changeLeaderTooltip");
	txKickTooltip = s.Get("kickTooltip");
	txFastTravel = s.Get("fastTravel");
	txCancelFastTravel = s.Get("cancelFastTravel");
	txWaiting = s.Get("waiting");
	txReady = s.Get("ready");
}

//=================================================================================================
void TeamPanel::LoadData()
{
	tCrown = resMgr->Load<Texture>("korona.png");
	tSkull = resMgr->Load<Texture>("czaszka.png");
	tFastTravelWait = resMgr->Load<Texture>("fast_travel_wait.png");
	tFastTravelOk = resMgr->Load<Texture>("fast_travel_ok.png");
}

//=================================================================================================
void TeamPanel::Draw()
{
	GamePanel::Draw();

	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	gui->DrawText(GameGui::fontBig, txTeam, DTF_TOP | DTF_CENTER, Color::Black, rect);

	Int2 offset = globalPos + Int2(15, 48 - scrollbar.offset);
	rect = Rect::Create(Int2(globalPos.x + 15, globalPos.y + 40), Int2(size.x - 30 - 16 - 5, size.y - 96));

	Vec2 share = team->GetShare();
	int pcShare = (int)round(share.x * 100);
	int npcShare = (int)round(share.y * 100);
	LocalString s;

	if(!picking)
		picked = -1;

	int n = 0;
	int hitboxCounter = 0;
	hitboxes.clear();
	for(Unit& unit : team->members)
	{
		Class* clas = unit.GetClass();
		if(clas)
		{
			Texture* t = clas->icon;
			Int2 imgSize, newSize(32, 32);
			Vec2 scale;
			t->ResizeImage(newSize, imgSize, scale);
			const Vec2 pos((float)offset.x, (float)offset.y);
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSprite2(t, mat, nullptr, &rect, Color::White);
		}
		if(&unit == team->leader)
			gui->DrawSprite(tCrown, Int2(offset.x + 32, offset.y), Color::White, &rect);
		else if(unit.IsPlayer() && net->IsFastTravel())
		{
			TexturePtr image = unit.player->player_info->fast_travel ? tFastTravelOk : tFastTravelWait;
			gui->DrawSprite(image, Int2(offset.x + 32, offset.y), Color::White, &rect);
		}
		if(!unit.IsAlive())
			gui->DrawSprite(tSkull, Int2(offset.x + 64, offset.y), Color::White, &rect);

		Rect r2 = {
			offset.x + 96,
			offset.y,
			offset.x + 1000,
			offset.y + 32
		};
		s = "$h+";
		s += Format(txCharInTeam, unit.GetName(), unit.IsPlayer() ? pcShare : (unit.hero->type == HeroType::Normal ? npcShare : 0), unit.GetCredit());
		if(unit.IsPlayer() && Net::IsOnline())
		{
			if(Net::IsServer())
			{
				if(&unit != game->pc->unit)
					s += Format(txPing, net->peer->GetAveragePing(unit.player->player_info->adr));
			}
			else if(&unit == game->pc->unit)
				s += Format(txPing, net->peer->GetAveragePing(net->server));
			s += Format(txDays, unit.player->free_days);
		}
		s += ")$h-";
		if(!gui->DrawText(GameGui::font, s->c_str(), DTF_VCENTER | DTF_SINGLELINE | DTF_PARSE_SPECIAL, (n == picked ? Color::White : Color::Black), r2, &rect, &hitboxes, &hitboxCounter))
			break;

		offset.y += 32;
		++n;
	}

	scrollbar.Draw();

	const int count = Net::IsOnline() ? 5 : 3;
	for(int i = 0; i < count; ++i)
		bt[i].Draw();

	DrawBox();

	tooltip.Draw();
}

//=================================================================================================
void TeamPanel::Update(float dt)
{
	GamePanel::Update(dt);

	int group = G_NONE;
	int index = 0;

	if(focus)
	{
		if(input->Focus() && IsInside(gui->cursorPos))
			scrollbar.ApplyMouseWheel();

		scrollbar.mouseFocus = mouseFocus;
		scrollbar.Update(dt);

		if(picking && input->Focus())
		{
			picked = -1;
			gui->Intersect(hitboxes, gui->cursorPos, &picked);

			if(input->Pressed(Key::LeftButton))
			{
				picking = false;
				if(picked >= 0)
				{
					Unit* target = team->members.ptrs[picked];
					switch(mode)
					{
					case BtEvent_GiveGold:
						GiveGold(target);
						break;
					case BtEvent_Kick:
						Kick(target);
						break;
					case BtEvent_Leader:
						ChangeLeader(target);
						break;
					}
				}
			}
			else if(input->Pressed(Key::RightButton))
				picking = false;
		}

		// handle icon tooltips
		Int2 offset = globalPos + Int2(8, 40 - scrollbar.offset);
		for(Unit& unit : team->members)
		{
			if(Class* clas = unit.GetClass(); clas && Rect::IsInside(gui->cursorPos, offset.x, offset.y, offset.x + 32, offset.y + 32))
			{
				group = G_CLASS;
				break;
			}
			if(Rect::IsInside(gui->cursorPos, offset.x + 32, offset.y, offset.x + 64, offset.y + 32))
			{
				if(&unit == team->leader)
				{
					group = G_LEADER;
					break;
				}
				else if(unit.IsPlayer() && net->IsFastTravel())
				{
					group = unit.player->player_info->fast_travel ? G_READY : G_WAITING;
					break;
				}
			}
			if(!unit.IsAlive() && Rect::IsInside(gui->cursorPos, offset.x + 64, offset.y, offset.x + 96, offset.y + 32))
			{
				group = G_UNCONSCIOUS;
				break;
			}

			offset.y += 32;
			++index;
		}
	}

	// enable change leader button if player is leader
	if(Net::IsClient())
	{
		bool wasLeader = bt[Bt_Leader].state != Button::DISABLED;
		bool isLeader = team->IsLeader();
		if(wasLeader != isLeader)
			bt[Bt_Leader].state = (isLeader ? Button::NONE : Button::DISABLED);
	}

	// enable fast travel button
	bool allowFastTravelPrev = bt[Bt_FastTravel].state != Button::DISABLED;
	if(net->IsFastTravel() && team->IsLeader())
	{
		bt[Bt_FastTravel].text = txCancelFastTravel;
		if(!allowFastTravelPrev)
			bt[Bt_FastTravel].state = Button::NONE;
	}
	else
	{
		bt[Bt_FastTravel].text = txFastTravel;
		bool allowFastTravel = team->IsLeader() && gameLevel->CanFastTravel();
		if(allowFastTravel != allowFastTravelPrev)
			bt[Bt_FastTravel].state = (allowFastTravel ? Button::NONE : Button::DISABLED);
	}

	const int count = Net::IsOnline() ? 5 : 3;
	for(int i = 0; i < count; ++i)
	{
		bt[i].mouseFocus = focus;
		bt[i].Update(dt);
		if(bt[i].state == Button::DISABLED && bt[i].IsInside(gui->cursorPos))
		{
			group = G_BUTTON;
			index = i;
		}
	}

	tooltip.UpdateTooltip(dt, group, index);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Hide();
}

//=================================================================================================
void TeamPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	switch(e)
	{
	case GuiEvent_Moved:
		scrollbar.globalPos = globalPos + scrollbar.pos;
		UpdateButtons();
		break;
	case GuiEvent_Resize:
		{
			int s = 32 * team->GetTeamSize();
			scrollbar.pos = Int2(size.x - 16 - 15, 48);
			scrollbar.globalPos = globalPos + scrollbar.pos;
			scrollbar.size = Int2(16, size.y - 70 - 48);
			scrollbar.total = s;
			scrollbar.part = min(s, scrollbar.size.y);
			if(scrollbar.offset + scrollbar.part > scrollbar.total)
				scrollbar.offset = float(scrollbar.total - scrollbar.part);
			if(scrollbar.offset < 0)
				scrollbar.offset = 0;
			UpdateButtons();
		}
		break;
	case GuiEvent_LostFocus:
		scrollbar.LostFocus();
		break;
	case GuiEvent_Show:
		bt[Bt_Leader].state = ((Net::IsServer() || team->IsLeader()) ? Button::NONE : Button::DISABLED);
		bt[Bt_Kick].state = (Net::IsServer() ? Button::NONE : Button::DISABLED);
		picking = false;
		Changed();
		UpdateButtons();
		tooltip.Clear();
		break;
	case BtEvent_FastTravel:
		if(net->IsFastTravel())
			net->CancelFastTravel(FAST_TRAVEL_CANCEL, team->myId);
		else if(Net::IsSingleplayer() || (Net::IsServer() && !team->HaveOtherPlayer()))
			game->ExitToMap();
		else
			net->StartFastTravel(0);
		break;
	case BtEvent_GiveGold:
	case BtEvent_Leader:
	case BtEvent_Kick:
		gameGui->messages->AddGameMsg2(txPickCharacter, 1.5f, GMS_PICK_CHARACTER);
		picking = true;
		picked = -1;
		mode = e;
		break;
	case BtEvent_PayCredit:
		if(game->pc->credit == 0)
			SimpleDialog(txNoCredit);
		else
		{
			counter = min(game->pc->credit, game->pc->unit->gold);
			GetNumberDialog::Show(this, delegate<void(int)>(this, &TeamPanel::OnPayCredit), Format(txPayCreditAmount, game->pc->credit), 1, counter, &counter);
		}
		break;
	}
}

//=================================================================================================
void TeamPanel::Changed()
{
	int s = 32 * team->GetTeamSize();
	scrollbar.total = s;
	scrollbar.part = min(s, scrollbar.size.y);
	if(scrollbar.offset + scrollbar.part > scrollbar.total)
		scrollbar.offset = float(scrollbar.total - scrollbar.part);
	if(scrollbar.offset < 0)
		scrollbar.offset = 0;
}

//=================================================================================================
void TeamPanel::UpdateButtons()
{
	const int count = Net::IsOnline() ? 5 : 3;
	const int s = (size.x - 30 - 4 * (count - 1)) / count;

	for(int i = 0; i < count; ++i)
	{
		bt[i].size.x = s;
		bt[i].size.y = 48;
		bt[i].pos = Int2(15 + (s + 4) * i, size.y - 48 - 15);
		bt[i].globalPos = bt[i].pos + globalPos;
	}
}

//=================================================================================================
void TeamPanel::OnPayCredit(int id)
{
	if(id != BUTTON_OK)
		return;

	if(game->pc->credit == 0)
		SimpleDialog(txNoCredit);
	else if(counter > game->pc->unit->gold)
		SimpleDialog(txNotEnoughGold);
	else
	{
		int count = min(counter, game->pc->credit);
		if(game->pc->credit == count)
			SimpleDialog(txPaidCredit);
		else
			SimpleDialog(Format(txPaidCreditPart, count, game->pc->credit - count));
		game->pc->unit->gold -= count;
		soundMgr->PlaySound2d(gameRes->sCoins);
		if(Net::IsLocal())
			game->pc->PayCredit(count);
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PAY_CREDIT;
			c.id = count;
		}
	}
}

//=================================================================================================
void TeamPanel::GiveGold(Unit* target)
{
	if(target == game->pc->unit)
		SimpleDialog(txGiveGoldSelf);
	else if(target->hero && target->hero->type != HeroType::Normal)
		SimpleDialog(Format(txGiveGoldRefuse, target->GetRealName()));
	else
	{
		targetUnit = target;
		counter = 1;
		GetNumberDialog::Show(this, DialogEvent(this, &TeamPanel::OnGiveGold), Format(txGiveGoldAmount, target->GetName()), 1, game->pc->unit->gold, &counter);
	}
}

//=================================================================================================
void TeamPanel::ChangeLeader(Unit* target)
{
	if(target->IsAI())
		SimpleDialog(txOnlyPcLeader);
	else if(target == game->pc->unit)
	{
		if(team->IsLeader())
			SimpleDialog(txAlreadyLeader);
		else if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_LEADER;
			c.id = team->myId;

			team->leaderId = team->myId;
			team->leader = game->pc->unit;

			gameGui->mpBox->Add(txYouAreLeader);
		}
		else
			SimpleDialog(txCantChangeLeader);
	}
	else if(team->IsLeader(target))
		SimpleDialog(Format(txPcAlreadyLeader, target->GetName()));
	else if(team->IsLeader() || Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_LEADER;
		c.id = target->player->id;

		if(Net::IsServer())
		{
			team->leaderId = c.id;
			team->leader = target;

			gameGui->mpBox->Add(Format(txPcIsLeader, target->GetName()));
		}
	}
	else
		SimpleDialog(txCantChangeLeader);
}

//=================================================================================================
void TeamPanel::Kick(Unit* target)
{
	if(target == game->pc->unit)
		SimpleDialog(txCantKickMyself);
	else if(target->IsAI())
		SimpleDialog(txCantKickAi);
	else
	{
		targetUnit = target;
		DialogInfo info;
		info.event = DialogEvent(this, &TeamPanel::OnKick);
		info.name = "kick";
		info.order = DialogOrder::Top;
		info.parent = this;
		info.pause = false;
		info.text = Format(txReallyKick, target->GetName());
		info.type = DIALOG_YESNO;
		gui->ShowDialog(info);
	}
}

//=================================================================================================
void TeamPanel::OnGiveGold(int id)
{
	if(id != BUTTON_OK || counter == 0)
		return;

	Unit* target = targetUnit;
	if(!target || !team->IsTeamMember(*target))
		SimpleDialog(txAlreadyLeft);
	else if(counter > game->pc->unit->gold)
		SimpleDialog(txNotEnoughGold);
	else
	{
		game->pc->unit->gold -= counter;
		soundMgr->PlaySound2d(gameRes->sCoins);
		if(Net::IsLocal())
		{
			target->gold += counter;
			if(target->IsPlayer() && target->player != game->pc)
			{
				NetChangePlayer& c = Add1(target->player->player_info->changes);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.id = game->pc->id;
				c.count = counter;
				target->player->player_info->UpdateGold();
			}
		}
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = target->id;
			c.count = counter;
		}
	}
}

//=================================================================================================
void TeamPanel::OnKick(int id)
{
	if(id == BUTTON_NO)
		return;

	Unit* target = targetUnit;
	if(!target || !team->IsTeamMember(*target))
		SimpleDialog(txAlreadyLeft);
	else
		net->KickPlayer(*target->player->player_info);
}

//=================================================================================================
void TeamPanel::Show()
{
	visible = true;
	Event(GuiEvent_Show);
	GainFocus();
}

//=================================================================================================
void TeamPanel::Hide()
{
	LostFocus();
	visible = false;
}

//=================================================================================================
void TeamPanel::GetTooltip(TooltipController*, int group, int id, bool refresh)
{
	if(group == G_NONE)
	{
		tooltip.anything = false;
		return;
	}

	tooltip.anything = true;
	tooltip.bigText.clear();
	tooltip.smallText.clear();
	tooltip.img = nullptr;

	switch(group)
	{
	case G_LEADER:
		tooltip.text = txLeader;
		break;
	case G_UNCONSCIOUS:
		tooltip.text = txUnconscious;
		break;
	case G_CLASS:
		tooltip.text = team->members[id].GetClass()->name;
		break;
	case G_BUTTON:
		switch(id)
		{
		case Bt_FastTravel:
			tooltip.text = txFastTravelTooltip;
			break;
		case Bt_Leader:
			tooltip.text = txChangeLeaderTooltip;
			break;
		case Bt_Kick:
			tooltip.text = txKickTooltip;
			break;
		}
		break;
	case G_WAITING:
		tooltip.text = txWaiting;
		break;
	case G_READY:
		tooltip.text = txReady;
		break;
	}
}
