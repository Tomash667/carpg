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
	Language::Section& s = Language::GetSection("TeamPanel");

	bt[0].text = s.Get("giveGold");
	bt[1].text = s.Get("payCredit");
	bt[2].text = s.Get("changeLeader");
	bt[3].text = s.Get("kick");

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
	txCAlreadyLeft = s.Get("cAlreadyLeft");
}

//=================================================================================================
void TeamPanel::LoadData()
{
	ResourceManager& res_mgr = ResourceManager::Get();
	tCrown = res_mgr.Load<Texture>("korona.png");
	tSkull = res_mgr.Load<Texture>("czaszka.png");
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
	gui->DrawText(GlobalGui::font_big, txTeam, DTF_TOP | DTF_CENTER, Color::Black, rect);

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
	for(Unit& unit : Team.members)
	{
		if(unit.GetClass() != Class::INVALID)
		{
			Texture* t = ClassInfo::classes[(int)unit.GetClass()].icon;
			Int2 img_size;
			Vec2 scale;
			t->ResizeImage(Int2(32, 32), img_size, scale);
			mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &Vec2((float)offset.x, (float)offset.y));
			gui->DrawSprite2(t, mat, nullptr, &rect, Color::White);
		}
		if(&unit == Team.leader)
			gui->DrawSprite(tCrown, Int2(offset.x + 32, offset.y), Color::White, &rect);
		if(!unit.IsAlive())
			gui->DrawSprite(tSkull, Int2(offset.x + 64, offset.y), Color::White, &rect);

		Rect r2 = {
			offset.x + 96,
			offset.y,
			offset.x + 1000,
			offset.y + 32
		};
		s = "$h+";
		s += Format(txCharInTeam, unit.GetName(), unit.IsPlayer() ? pc_share : (unit.hero->free ? 0 : npc_share), unit.GetCredit());
		if(unit.IsPlayer() && Net::IsOnline())
		{
			if(Net::IsServer())
			{
				if(&unit != game.pc->unit)
					s += Format(txPing, N.peer->GetAveragePing(unit.player->player_info->adr));
			}
			else if(&unit == game.pc->unit)
				s += Format(txPing, N.peer->GetAveragePing(N.server));
			s += Format(txDays, unit.player->free_days);
		}
		s += ")$h-";
		if(!gui->DrawText(GlobalGui::font, s->c_str(), DTF_VCENTER | DTF_SINGLELINE | DTF_PARSE_SPECIAL, (n == picked ? Color::White : Color::Black), r2, &rect, &hitboxes, &hitbox_counter))
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
		if(input->Focus() && IsInside(gui->cursor_pos))
			scrollbar.ApplyMouseWheel();

		scrollbar.mouse_focus = mouse_focus;
		scrollbar.Update(dt);

		if(picking && input->Focus())
		{
			picked = -1;
			gui->Intersect(hitboxes, gui->cursor_pos, &picked);

			if(input->Pressed(Key::LeftButton))
			{
				picking = false;
				if(picked >= 0)
				{
					target = Team.members.ptrs[picked];
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
			else if(input->Pressed(Key::RightButton))
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
		counter = 1;
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
		gui->ShowDialog(info);
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
