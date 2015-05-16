#include "Pch.h"
#include "Base.h"
#include "MpBox.h"
#include "Game.h"

//=================================================================================================
MpBox::MpBox() : have_focus(false)
{
	pos = global_pos = INT2(100,50);
	size = INT2(320,182);
	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = true;
	itb.lose_focus = true;
	itb.pos = INT2(0,0);
	itb.global_pos = INT2(100,50);
	itb.size = INT2(320,182);
	itb.event = InputEvent(this, &MpBox::OnInput);
	itb.background = &GUI.tPix;
	itb.background_color = COLOR_RGBA(0,142,254,43);
	itb.Init();

	visible = false;
}

//=================================================================================================
void MpBox::Draw(ControlDrawData*)
{
	if(!GamePanel::bt_drawn)
		GamePanel::DrawButton();

	if(allow_move)
		GamePanel::Draw();

	itb.Draw();
}

//=================================================================================================
void MpBox::Update(float dt)
{
	// hack na focus mp_box
	Game& game = Game::Get();
	focusable = !(game.journal->visible || game.minimap->visible || game.gp_cont->visible || game.gp_trade->visible);

	if(!GamePanel::bt_updated)
		GamePanel::UpdateButton(dt);

	bool prev_focus = focus;
	focus = focusable;
	GamePanel::Update(dt);
	itb.mouse_focus = focus;
	focus = prev_focus;
	itb.Update(dt);
	have_focus = itb.focus;
}

//=================================================================================================
void MpBox::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		if(have_focus)
		{
			itb.focus = true;
			itb.Event(GuiEvent_GainFocus);
		}
	}
	else if(e == GuiEvent_LostFocus)
	{
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_Resize)
		itb.Event(GuiEvent_Resize);
	else if(e == GuiEvent_Moved)
		itb.Event(GuiEvent_Moved);
}

//=================================================================================================
void MpBox::Reset()
{
	itb.Reset();
}

//=================================================================================================
void MpBox::OnInput(const string& str)
{
	Game& game = Game::Get();
	if(str[0] == '/')
		game.ParseCommand(str.substr(1), PrintMsgFunc(&game, &Game::AddMultiMsg), PS_CHAT);
	else if(game.IsOnline())
	{
		// wyœlij tekst
		if(game.players != 1)
		{
			game.net_stream.Reset();
			game.net_stream.Write(ID_SAY);
			game.net_stream.WriteCasted<byte>(game.my_id);
			WriteString1(game.net_stream, str);
			game.peer->Send(&game.net_stream, MEDIUM_PRIORITY, RELIABLE, 0, game.sv_server ? UNASSIGNED_SYSTEM_ADDRESS : game.server, game.sv_server);
		}
		cstring s = Format("%s: %s", game.player_name.c_str(), str.c_str());
		game.AddMultiMsg(s);
		LOG(s);
		if(game.game_state == GS_LEVEL)
			game.game_gui->AddSpeechBubble(game.pc->unit, str.c_str());
	}
}
