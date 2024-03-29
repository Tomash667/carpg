#include "Pch.h"
#include "MpBox.h"

#include "BitStreamFunc.h"
#include "CommandParser.h"
#include "Game.h"
#include "GameGui.h"
#include "LevelGui.h"
#include "PlayerController.h"
#include "Team.h"

//=================================================================================================
MpBox::MpBox() : haveFocus(false)
{
	itb.parent = this;
	itb.maxCache = 10;
	itb.maxLines = 100;
	itb.escClear = true;
	itb.loseFocus = true;
	itb.pos = Int2(0, 0);
	itb.globalPos = Int2(100, 50);
	itb.size = Int2(320, 182);
	itb.event = InputTextBox::InputEvent(this, &MpBox::OnInput);
	itb.backgroundColor = Color(0, 142, 254, 43);
	itb.Init();

	visible = false;
}

//=================================================================================================
void MpBox::Draw()
{
	itb.Draw();
}

//=================================================================================================
void MpBox::Update(float dt)
{
	// hack for MpBox focus
	focusable = gameGui->levelGui->CanFocusMpBox();

	bool prevFocus = focus;
	focus = focusable;
	GamePanel::Update(dt);
	itb.mouseFocus = focus;
	focus = prevFocus;
	itb.Update(dt);
	haveFocus = itb.focus;
}

//=================================================================================================
void MpBox::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		if(haveFocus)
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
		cmdp->ParseCommand(str.substr(1), CommandParser::PrintMsgFunc(this, &MpBox::Add), PS_CHAT);
	else
	{
		if(Net::IsOnline() && net->activePlayers != 1)
		{
			// send text to server / other players
			BitStreamWriter f;
			f << ID_SAY;
			f.WriteCasted<byte>(team->myId);
			f << str;
			if(Net::IsServer())
				net->SendAll(f, MEDIUM_PRIORITY, RELIABLE);
			else
				net->SendClient(f, MEDIUM_PRIORITY, RELIABLE);
		}
		// add text
		cstring s = Format("%s: %s", game->pc->name.c_str(), str.c_str());
		Add(s);
		if(game->gameState == GS_LEVEL)
			gameGui->levelGui->AddSpeechBubble(game->pc->unit, str.c_str());
	}
}
