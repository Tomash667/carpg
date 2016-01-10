#include "Pch.h"
#include "Base.h"
#include "TeamPanel.h"
#include "Unit.h"
#include "Game.h"
#include "Language.h"
#include "GetNumberDialog.h"

//-----------------------------------------------------------------------------
enum ButtonId
{
	Bt_GiveGold = GuiEvent_Custom,
	Bt_PayCredit,
	Bt_Leader,
	Bt_Kick
};

//-----------------------------------------------------------------------------
TEX TeamPanel::tKorona, TeamPanel::tCzaszka;

//=================================================================================================
TeamPanel::TeamPanel() : game(Game::Get())
{
	visible = false;

	bt[0].text = Str("giveGold");
	bt[0].id = Bt_GiveGold;
	bt[0].parent = this;
	bt[1].text = Str("payCredit");
	bt[1].id = Bt_PayCredit;
	bt[1].parent = this;
	bt[2].text = Str("changeLeader");
	bt[2].id = Bt_Leader;
	bt[2].parent = this;
	bt[3].text = Str("kick");
	bt[3].id = Bt_Kick;
	bt[3].parent = this;

	UpdateButtons();

	txTeam = Str("team");
	txCharInTeam = Str("charInTeam");
	txPing = Str("charPing");
	txDays = Str("charDays");
	txPickCharacter = Str("pickCharacter");
	txNoCredit = Str("noCredit");
	txPayCreditAmount = Str("payCreditAmount");
	txNotEnoughGold = Str("notEnoughGold");
	txPaidCredit = Str("paidCredit");
	txPaidCreditPart = Str("paidCreditPart");
	txGiveGoldSelf = Str("giveGoldSelf");
	txGiveGoldAmount = Str("giveGoldAmount");
	txOnlyPcLeader = Str("onlyPcLeader");
	txAlreadyLeader = Str("alreadyLeader");
	txYouAreLeader = Str("youAreLeader");
	txCantChangeLeader = Str("cantChangeLeader");
	txPcAlreadyLeader = Str("pcAlreadyLeader");
	txPcIsLeader = Str("pcIsLeader");
	txCantKickMyself = Str("cantKickMyself");
	txCantKickAi = Str("cantKickAi");
	txReallyKick = Str("reallyKick");
	txAlreadyLeft = Str("alreadyLeft");
	txCAlreadyLeft = Str("cAlreadyLeft");
}

//=================================================================================================
inline int& GetCredit(Unit& u)
{
	if(u.IsPlayer())
		return u.player->credit;
	else
	{
		assert(u.IsFollower());
		return u.hero->credit;
	}
}

//=================================================================================================
void TeamPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	RECT rect = {
		pos.x+8,
		pos.y+8,
		pos.x+size.x-16,
		pos.y+size.y-16
	};
	GUI.DrawText(GUI.fBig, txTeam, DT_TOP|DT_CENTER, BLACK, rect);

	INT2 offset = global_pos+INT2(8,40-scrollbar.offset);
	rect.left = global_pos.x+8;
	rect.right = global_pos.x+size.x-44;
	rect.top = global_pos.y+40;
	rect.bottom = global_pos.y+size.y-8-48;

	int pc_share = game.GetPCShare();
	LocalString s;

	if(!picking)
		picked = -1;

	int n = 0;
	int hitbox_counter = 0;
	hitboxes.clear();
	MATRIX mat;
	for(vector<Unit*>::iterator it = game.team.begin(), end = game.team.end(); it != end; ++it, ++n)
	{
		Unit* u = *it;

		if(!u->IsHero() || !IS_SET(u->data->flags2, F2_NO_CLASS))
		{
			TEX t = g_classes[(int)u->GetClass()].icon;
			INT2 img_size;
			VEC2 scale;
			Control::ResizeImage(t, INT2(32, 32), img_size, scale);
			D3DXMatrixTransformation2D(&mat, nullptr, 0.f, &scale, nullptr, 0.f, &VEC2((float)offset.x, (float)offset.y));
			GUI.DrawSprite2(t, &mat, nullptr, &rect, WHITE);
		}
		if(u == game.leader)
			GUI.DrawSprite(tKorona, INT2(offset.x+32,offset.y), WHITE, &rect);
		if(!u->IsStanding())
			GUI.DrawSprite(tCzaszka, INT2(offset.x+64,offset.y), WHITE, &rect);

		RECT r2 = {
			offset.x+96, 
			offset.y,
			offset.x+1000,
			offset.y+32
		};
		s = "$h+";
		s += Format(txCharInTeam, u->GetName(), u->IsPlayer() ? pc_share : (u->hero->free ? 0 : 10), GetCredit(*u));
		if(u->IsPlayer() && game.IsOnline())
		{
			if(game.IsServer())
			{
				if(u != game.pc->unit)
				{
					PlayerInfo& info = game.GetPlayerInfo(u->player);
					s += Format(txPing, game.peer->GetAveragePing(info.adr));
				}
			}
			else if(u == game.pc->unit)
				s += Format(txPing, game.peer->GetAveragePing(game.server));
			s += Format(txDays, (*it)->player->free_days);
		}
		s += ")$h-";
		if(!GUI.DrawText(GUI.default_font, s->c_str(), DT_VCENTER|DT_SINGLELINE, (n == picked ? WHITE : BLACK), r2, &rect, &hitboxes, &hitbox_counter))
			break;

		offset.y += 32;
	}

	scrollbar.Draw();

	int ile = (game.IsOnline() ? 4 : 2);
	for(int i=0; i<ile; ++i)
		bt[i].Draw();

	DrawBox();
}

//=================================================================================================
void TeamPanel::Update(float dt)
{
	GamePanel::Update(dt);

	if(focus)
	{
		if(Key.Focus() && IsInside(GUI.cursor_pos))
			scrollbar.ApplyMouseWheel();
		
		scrollbar.Update(dt);

		if(picking && Key.Focus())
		{
			picked = -1;
			GUI.Intersect(hitboxes, GUI.cursor_pos, &picked);

			if(Key.Pressed(VK_LBUTTON))
			{
				picking = false;
				if(picked >= 0)
				{
					target = game.team[picked];
					switch(mode)
					{
					case Bt_GiveGold:
						GiveGold();
						break;
					case Bt_Kick:
						Kick();
						break;
					case Bt_Leader:
						ChangeLeader();
						break;
					}
				}
			}
			else if(Key.Pressed(VK_RBUTTON))
				picking = false;
		}
	}	

	int ile = (game.IsOnline() ? 4 : 2);
	for(int i=0; i<ile; ++i)
	{
		bt[i].mouse_focus = focus;
		bt[i].Update(dt);
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void TeamPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	switch(e)
	{
	case GuiEvent_Moved:
		scrollbar.global_pos = global_pos + scrollbar.pos;
		UpdateButtons();
		break;
	case GuiEvent_Resize:
		{
			int s = 32*game.team.size();
			scrollbar.total = s;
			scrollbar.part = min(s, scrollbar.size.y);
			scrollbar.pos = INT2(size.x-28, 48);
			scrollbar.global_pos = global_pos + scrollbar.pos;
			scrollbar.size = INT2(16, size.y-60-48);
			if(scrollbar.offset+scrollbar.part > scrollbar.total)
				scrollbar.offset = float(scrollbar.total-scrollbar.part);
			if(scrollbar.offset < 0)
				scrollbar.offset = 0;
			UpdateButtons();
		}
		break;
	case GuiEvent_LostFocus:
		scrollbar.LostFocus();
		break;
	case GuiEvent_Show:
		bt[2].state = ((game.IsServer() || game.IsLeader()) ? Button::NONE : Button::DISABLED);
		bt[3].state = (game.IsServer() ? Button::NONE : Button::DISABLED);
		picking = false;
		Changed();
		UpdateButtons();
		break;
	case Bt_GiveGold:
	case Bt_Leader:
	case Bt_Kick:
		game.AddGameMsg2(txPickCharacter, 1.5f, GMS_PICK_CHARACTER);
		picking = true;
		picked = -1;
		mode = e;
		break;
	case Bt_PayCredit:
		if(game.pc->credit == 0)
			SimpleDialog(txNoCredit);
		else
		{
			counter = min(game.pc->credit, game.pc->unit->gold);
			GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &TeamPanel::OnPayCredit), Format(txPayCreditAmount, game.pc->credit), 0, counter, &counter);
		}
		break;
	}
}

//=================================================================================================
void TeamPanel::Changed()
{
	int s = 32*game.team.size();
	scrollbar.total = s;
	scrollbar.part = min(s, scrollbar.size.y);
	if(scrollbar.offset+scrollbar.part > scrollbar.total)
		scrollbar.offset = float(scrollbar.total-scrollbar.part);
	if(scrollbar.offset < 0)
		scrollbar.offset = 0;
}

//=================================================================================================
void TeamPanel::UpdateButtons()
{
	if(game.IsOnline())
	{
		int s = (size.x-16-12)/4;

		bt[0].size.x = s;
		bt[0].size.y = 48;
		bt[0].pos = INT2(8,size.y-58);
		bt[0].global_pos = bt[0].pos + global_pos;

		bt[1].size.x = s;
		bt[1].size.y = 48;
		bt[1].pos = INT2(8+s+4,size.y-58);
		bt[1].global_pos = bt[1].pos + global_pos;

		bt[2].size.x = s;
		bt[2].size.y = 48;
		bt[2].pos = INT2(8+s*2+8,size.y-58);
		bt[2].global_pos = bt[2].pos + global_pos;

		bt[3].size.x = s;
		bt[3].size.y = 48;
		bt[3].pos = INT2(8+s*3+12,size.y-58);
		bt[3].global_pos = bt[3].pos + global_pos;
	}
	else
	{
		int s = (size.x-16-6)/2;

		bt[0].size.x = s;
		bt[0].size.y = 48;
		bt[0].pos = INT2(8,size.y-58);
		bt[0].global_pos = bt[0].pos + global_pos;

		bt[1].size.x = s;
		bt[1].size.y = 48;
		bt[1].pos = INT2(8+s+4,size.y-58);
		bt[1].global_pos = bt[1].pos + global_pos;
	}
}

//=================================================================================================
void TeamPanel::OnPayCredit(int id)
{
	if(id != BUTTON_OK)
		return;

	if(game.pc->credit == 0)
		SimpleDialog(txNoCredit);
	else if(counter > game.pc->unit->gold)
		SimpleDialog(txNotEnoughGold);
	else
	{
		int ile = min(counter, game.pc->credit);
		if(game.pc->credit == ile)
			SimpleDialog(txPaidCredit);
		else
			SimpleDialog(Format(txPaidCreditPart, ile, game.pc->credit-ile));
		game.pc->unit->gold -= ile;
		if(game.sound_volume)
			game.PlaySound2d(game.sCoins);
		if(game.IsLocal())
			game.PayCredit(game.pc, ile);
		else
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::PAY_CREDIT;
			c.id = ile;
		}
	}
}

//=================================================================================================
void TeamPanel::GiveGold()
{
	if(target == game.pc->unit)
		SimpleDialog(txGiveGoldSelf);
	else
	{
		counter = 0;
		GetNumberDialog::Show(this, DialogEvent(this, &TeamPanel::OnGiveGold), Format(txGiveGoldAmount, target->GetName()), 0, game.pc->unit->gold, &counter);
	}
}

//=================================================================================================
void TeamPanel::ChangeLeader()
{
	if(target->IsAI())
		SimpleDialog(txOnlyPcLeader);
	else if(target == game.pc->unit)
	{
		if(game.IsLeader())
			SimpleDialog(txAlreadyLeader);
		else if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::CHANGE_LEADER;
			c.id = game.my_id;

			game.leader_id = game.my_id;
			game.leader =game. pc->unit;

			game.AddMultiMsg(txYouAreLeader);
		}
		else
			SimpleDialog(txCantChangeLeader);
	}
	else if(game.IsLeader(target))
		SimpleDialog(Format(txPcAlreadyLeader, target->GetName()));
	else if(game.IsLeader() || game.IsServer())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::CHANGE_LEADER;
		c.id = target->player->id;

		if(game.IsServer())
		{
			game.leader_id = c.id;
			game.leader = target;

			game.AddMultiMsg(Format(txPcIsLeader, target->GetName()));
		}
	}
	else
		SimpleDialog(txCantChangeLeader);
}

//=================================================================================================
void TeamPanel::Kick()
{
	if(target == game.pc->unit)
		SimpleDialog(txCantKickMyself);
	else if(target->IsAI())
		SimpleDialog(txCantKickAi);
	else
	{
		DialogInfo info;
		info.event = DialogEvent(this, &TeamPanel::OnKick);
		info.name = "kick";
		info.order = ORDER_TOP;
		info.parent = this;
		info.pause = false;
		info.text = Format(txReallyKick, target->GetName());
		info.type = DIALOG_YESNO;
		GUI.ShowDialog(info);
	}
}

//=================================================================================================
void TeamPanel::OnGiveGold(int id)
{
	if(id != BUTTON_OK || counter == 0)
		return;

	if(!game.IsTeamMember(*target))
		SimpleDialog(Format(txCAlreadyLeft, target->GetName()));
	else if(counter > game.pc->unit->gold)
		SimpleDialog(txNotEnoughGold);
	else
	{
		game.pc->unit->gold -= counter;
		if(game.sound_volume)
			game.PlaySound2d(game.sCoins);
		if(game.IsLocal())
		{
			target->gold += counter;
			if(target->IsPlayer() && target->player != game.pc)
			{
				NetChangePlayer& c = Add1(game.net_changes_player);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.pc = target->player;
				c.id = game.pc->id;
				c.ile = counter;
				game.GetPlayerInfo(target->player).NeedUpdateAndGold();
			}
		}
		else
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = target->netid;
			c.ile = counter;
		}
	}
}

//=================================================================================================
void TeamPanel::OnKick(int id)
{
	if(id == BUTTON_NO)
		return;

	if(!game.IsTeamMember(*target))
		SimpleDialog(txAlreadyLeft);
	else
	{
		int index = game.GetPlayerIndex(target->player->id);
		if(index != -1)
			game.KickPlayer(index);
	}
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
