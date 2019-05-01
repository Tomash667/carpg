#include "Pch.h"
#include "GameCore.h"
#include "TeamPanel.h"
#include "Unit.h"
#include "Game.h"
#include "Language.h"
#include "GetNumberDialog.h"
#include "Team.h"
#include "SoundManager.h"
#include "GlobalGui.h"
#include "GameMessages.h"
#include "ResourceManager.h"
#include "PlayerInfo.h"

//-----------------------------------------------------------------------------
enum ButtonId
{
	Bt_GiveGold = GuiEvent_Custom,
	Bt_PayCredit,
	Bt_Leader,
	Bt_Kick
};

//=================================================================================================
TeamPanel::TeamPanel() : game(Game::Get())
{
	visible = false;

	bt[0].id = Bt_GiveGold;
	bt[0].parent = this;
	bt[1].id = Bt_PayCredit;
	bt[1].parent = this;
	bt[2].id = Bt_Leader;
	bt[2].parent = this;
	bt[3].id = Bt_Kick;
	bt[3].parent = this;

	UpdateButtons();
}

//=================================================================================================
void TeamPanel::LoadLanguage()
{
	bt[0].text = Str("giveGold");
	bt[1].text = Str("payCredit");
	bt[2].text = Str("changeLeader");
	bt[3].text = Str("kick");

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
void TeamPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	GUI.DrawText(GUI.fBig, txTeam, DTF_TOP | DTF_CENTER, Color::Black, rect);

	Int2 offset = global_pos + Int2(8, 40 - scrollbar.offset);
	rect = Rect::Create(Int2(global_pos.x + 8, global_pos.y + 40), Int2(size.x - 52, size.y - 96));

	Vec2 share = Team.GetShare();
	int pc_share = (int)round(share.x * 100);
	int npc_share = (int)round(share.y * 100);
	LocalString s;

	if(!picking)
		picked = -1;

	int n = 0;
	int hitbox_counter = 0;
	hitboxes.clear();
	Matrix mat;
	for(Unit* u : Team.members)
	{
		if(u->GetClass() != Class::INVALID)
		{
			TEX t = ClassInfo::classes[(int)u->GetClass()].icon;
			Int2 img_size;
			Vec2 scale;
			Control::ResizeImage(t, Int2(32, 32), img_size, scale);
			mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &Vec2((float)offset.x, (float)offset.y));
			GUI.DrawSprite2(t, mat, nullptr, &rect, Color::White);
		}
		if(u == Team.leader)
			GUI.DrawSprite(tKorona, Int2(offset.x + 32, offset.y), Color::White, &rect);
		if(!u->IsAlive())
			GUI.DrawSprite(tCzaszka, Int2(offset.x + 64, offset.y), Color::White, &rect);

		Rect r2 = {
			offset.x + 96,
			offset.y,
			offset.x + 1000,
			offset.y + 32
		};
		s = "$h+";
		s += Format(txCharInTeam, u->GetName(), u->IsPlayer() ? pc_share : (u->hero->free ? 0 : npc_share), u->GetCredit());
		if(u->IsPlayer() && Net::IsOnline())
		{
			if(Net::IsServer())
			{
				if(u != game.pc->unit)
					s += Format(txPing, N.peer->GetAveragePing(u->player->player_info->adr));
			}
			else if(u == game.pc->unit)
				s += Format(txPing, N.peer->GetAveragePing(N.server));
			s += Format(txDays, u->player->free_days);
		}
		s += ")$h-";
		if(!GUI.DrawText(GUI.default_font, s->c_str(), DTF_VCENTER | DTF_SINGLELINE | DTF_PARSE_SPECIAL, (n == picked ? Color::White : Color::Black), r2, &rect, &hitboxes, &hitbox_counter))
			break;

		offset.y += 32;
		++n;
	}

	scrollbar.Draw();

	int count = (Net::IsOnline() ? 4 : 2);
	for(int i = 0; i < count; ++i)
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

		scrollbar.mouse_focus = mouse_focus;
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
					target = Team.members[picked];
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

	// enable change leader button if player is leader
	if(Net::IsClient())
	{
		bool was_leader = bt[2].state != Button::DISABLED;
		bool is_leader = Team.IsLeader();
		if(was_leader != is_leader)
			bt[2].state = (is_leader ? Button::NONE : Button::DISABLED);
	}

	int count = (Net::IsOnline() ? 4 : 2);
	for(int i = 0; i < count; ++i)
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
			int s = 32 * Team.GetTeamSize();
			scrollbar.total = s;
			scrollbar.part = min(s, scrollbar.size.y);
			scrollbar.pos = Int2(size.x - 28, 48);
			scrollbar.global_pos = global_pos + scrollbar.pos;
			scrollbar.size = Int2(16, size.y - 60 - 48);
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
		bt[2].state = ((Net::IsServer() || Team.IsLeader()) ? Button::NONE : Button::DISABLED);
		bt[3].state = (Net::IsServer() ? Button::NONE : Button::DISABLED);
		picking = false;
		Changed();
		UpdateButtons();
		break;
	case Bt_GiveGold:
	case Bt_Leader:
	case Bt_Kick:
		game.gui->messages->AddGameMsg2(txPickCharacter, 1.5f, GMS_PICK_CHARACTER);
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
			GetNumberDialog::Show(this, delegate<void(int)>(this, &TeamPanel::OnPayCredit), Format(txPayCreditAmount, game.pc->credit), 1, counter, &counter);
		}
		break;
	}
}

//=================================================================================================
void TeamPanel::Changed()
{
	int s = 32 * Team.GetTeamSize();
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
	if(Net::IsOnline())
	{
		int s = (size.x - 16 - 12) / 4;

		bt[0].size.x = s;
		bt[0].size.y = 48;
		bt[0].pos = Int2(8, size.y - 58);
		bt[0].global_pos = bt[0].pos + global_pos;

		bt[1].size.x = s;
		bt[1].size.y = 48;
		bt[1].pos = Int2(8 + s + 4, size.y - 58);
		bt[1].global_pos = bt[1].pos + global_pos;

		bt[2].size.x = s;
		bt[2].size.y = 48;
		bt[2].pos = Int2(8 + s * 2 + 8, size.y - 58);
		bt[2].global_pos = bt[2].pos + global_pos;

		bt[3].size.x = s;
		bt[3].size.y = 48;
		bt[3].pos = Int2(8 + s * 3 + 12, size.y - 58);
		bt[3].global_pos = bt[3].pos + global_pos;
	}
	else
	{
		int s = (size.x - 16 - 6) / 2;

		bt[0].size.x = s;
		bt[0].size.y = 48;
		bt[0].pos = Int2(8, size.y - 58);
		bt[0].global_pos = bt[0].pos + global_pos;

		bt[1].size.x = s;
		bt[1].size.y = 48;
		bt[1].pos = Int2(8 + s + 4, size.y - 58);
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
		int count = min(counter, game.pc->credit);
		if(game.pc->credit == count)
			SimpleDialog(txPaidCredit);
		else
			SimpleDialog(Format(txPaidCreditPart, count, game.pc->credit - count));
		game.pc->unit->gold -= count;
		game.sound_mgr->PlaySound2d(game.sCoins);
		if(Net::IsLocal())
			game.pc->PayCredit(count);
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PAY_CREDIT;
			c.id = count;
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
		GetNumberDialog::Show(this, DialogEvent(this, &TeamPanel::OnGiveGold), Format(txGiveGoldAmount, target->GetName()), 1, game.pc->unit->gold, &counter);
	}
}

//=================================================================================================
void TeamPanel::ChangeLeader()
{
	if(target->IsAI())
		SimpleDialog(txOnlyPcLeader);
	else if(target == game.pc->unit)
	{
		if(Team.IsLeader())
			SimpleDialog(txAlreadyLeader);
		else if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_LEADER;
			c.id = Team.my_id;

			Team.leader_id = Team.my_id;
			Team.leader = game.pc->unit;

			game.AddMultiMsg(txYouAreLeader);
		}
		else
			SimpleDialog(txCantChangeLeader);
	}
	else if(Team.IsLeader(target))
		SimpleDialog(Format(txPcAlreadyLeader, target->GetName()));
	else if(Team.IsLeader() || Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_LEADER;
		c.id = target->player->id;

		if(Net::IsServer())
		{
			Team.leader_id = c.id;
			Team.leader = target;

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

	if(!Team.IsTeamMember(*target))
		SimpleDialog(Format(txCAlreadyLeft, target->GetName()));
	else if(counter > game.pc->unit->gold)
		SimpleDialog(txNotEnoughGold);
	else
	{
		game.pc->unit->gold -= counter;
		game.sound_mgr->PlaySound2d(game.sCoins);
		if(Net::IsLocal())
		{
			target->gold += counter;
			if(target->IsPlayer() && target->player != game.pc)
			{
				NetChangePlayer& c = Add1(target->player->player_info->changes);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.id = game.pc->id;
				c.count = counter;
				target->player->player_info->UpdateGold();
			}
		}
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = target->netid;
			c.count = counter;
		}
	}
}

//=================================================================================================
void TeamPanel::OnKick(int id)
{
	if(id == BUTTON_NO)
		return;

	if(!Team.IsTeamMember(*target))
		SimpleDialog(txAlreadyLeft);
	else
		N.KickPlayer(*target->player->player_info);
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
void TeamPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("czaszka.png", TeamPanel::tCzaszka);
	tex_mgr.AddLoadTask("korona.png", TeamPanel::tKorona);
}
