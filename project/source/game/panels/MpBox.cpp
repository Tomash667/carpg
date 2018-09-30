#include "Pch.h"
#include "GameCore.h"
#include "MpBox.h"
#include "Game.h"
#include "GlobalGui.h"
#include "GameGui.h"
#include "BitStreamFunc.h"

//=================================================================================================
MpBox::MpBox() : have_focus(false)
{
	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = true;
	itb.lose_focus = true;
	itb.pos = Int2(0, 0);
	itb.global_pos = Int2(100, 50);
	itb.size = Int2(320, 182);
	itb.event = InputEvent(this, &MpBox::OnInput);
	itb.background = &GUI.tPix;
	itb.background_color = Color(0, 142, 254, 43);
	itb.Init();

	visible = false;
}

//=================================================================================================
void MpBox::Draw(ControlDrawData*)
{
	itb.Draw();
}

//=================================================================================================
void MpBox::Update(float dt)
{
	// hack for mp_box focus
	focusable = Game::Get().gui->game_gui->CanFocusMpBox();

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
	else
	{
		if(Net::IsOnline() && N.active_players != 1)
		{
			// send text to server / other players
			BitStreamWriter f;
			f << ID_SAY;
			f.WriteCasted<byte>(game.my_id);
			f << str;
			if(Net::IsServer())
				N.SendAll(f, MEDIUM_PRIORITY, RELIABLE, Stream_Chat);
			else
				N.SendClient(f, MEDIUM_PRIORITY, RELIABLE, Stream_Chat);
		}
		// add text
		cstring s = Format("%s: %s", game.player_name.c_str(), str.c_str());
		game.AddMultiMsg(s);
		if(game.game_state == GS_LEVEL)
			game.gui->game_gui->AddSpeechBubble(game.pc->unit, str.c_str());
	}
}
