#include "Pch.h"
#include "GameCore.h"
#include "MpBox.h"
#include "Game.h"
#include "GameGui.h"
#include "LevelGui.h"
#include "BitStreamFunc.h"
#include "Team.h"
#include "CommandParser.h"

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
	itb.event = InputTextBox::InputEvent(this, &MpBox::OnInput);
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
	focusable = game_gui->level_gui->CanFocusMpBox();

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
	if(str[0] == '/')
		cmdp->ParseCommand(str.substr(1), CommandParser::PrintMsgFunc(game, &Game::AddMultiMsg), PS_CHAT);
	else
	{
		if(Net::IsOnline() && net->active_players != 1)
		{
			// send text to server / other players
			BitStreamWriter f;
			f << ID_SAY;
			f.WriteCasted<byte>(Team.my_id);
			f << str;
			if(Net::IsServer())
				net->SendAll(f, MEDIUM_PRIORITY, RELIABLE);
			else
				net->SendClient(f, MEDIUM_PRIORITY, RELIABLE);
		}
		// add text
		cstring s = Format("%s: %s", game->player_name.c_str(), str.c_str());
		game->AddMultiMsg(s);
		if(game->game_state == GS_LEVEL)
			game_gui->level_gui->AddSpeechBubble(game->pc->unit, str.c_str());
	}
}
