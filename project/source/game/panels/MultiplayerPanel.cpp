#include "Pch.h"
#include "Base.h"
#include "MultiplayerPanel.h"
#include "KeyStates.h"
#include "Language.h"
#include "Game.h"

//=================================================================================================
MultiplayerPanel::MultiplayerPanel(const DialogInfo& info) : Dialog(info)
{
	size = INT2(344,380);
	bts.resize(5);

	txMultiplayerGame = Str("multiplayerGame");
	txNick = Str("nick");
	txNeedEnterNick = Str("needEnterNick");
	txEnterValidNick = Str("enterValidNick");

	const INT2 bt_size(180,44);
	const int x = (size.x-bt_size.x)/2;

	bts[0].text = Str("joinLAN");
	bts[0].parent = this;
	bts[0].id = IdJoinLan;
	bts[0].size = bt_size;
	bts[0].pos = INT2(x,100);
	
	bts[1].text = Str("joinIP");
	bts[1].parent = this;
	bts[1].id = IdJoinIp;
	bts[1].size = bt_size;
	bts[1].pos = INT2(x,150);

	bts[2].text = Str("host");
	bts[2].parent = this;
	bts[2].id = IdCreate;
	bts[2].size = bt_size;
	bts[2].pos = INT2(x,200);

	bts[3].text = Str("load");
	bts[3].parent = this;
	bts[3].id = IdLoad;
	bts[3].size = bt_size;
	bts[3].pos = INT2(x,250);

	bts[4].text = GUI.txCancel;
	bts[4].parent = this;
	bts[4].id = IdCancel;
	bts[4].size = bt_size;
	bts[4].pos = INT2(x,300);

	textbox.limit = 16;
	textbox.parent = this;
	textbox.pos = INT2((size.x-200)/2, 57);
	textbox.size = INT2(200,32);

	visible = false;
}

//=================================================================================================
void MultiplayerPanel::Draw(ControlDrawData*)
{
	// t³o
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	// tekst
	RECT r = {global_pos.x+12, global_pos.y+8, global_pos.x+size.x-12, global_pos.y+size.y};
	GUI.DrawText(GUI.fBig, txMultiplayerGame, DT_TOP|DT_CENTER, BLACK, r);

	// tekst nick
	r.top += 60;
	GUI.DrawText(GUI.default_font, txNick, 0, BLACK, r);

	// textbox
	textbox.Draw();

	// przyciski
	for(int i=0; i<5; ++i)
		bts[i].Draw();
}

//=================================================================================================
void MultiplayerPanel::Update(float dt)
{
	for(int i=0; i<5; ++i)
	{
		bts[i].mouse_focus = focus;
		bts[i].Update(dt);
	}

	textbox.mouse_focus = focus;
	textbox.Update(dt);

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void MultiplayerPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			textbox.focus = true;
			textbox.Event(GuiEvent_GainFocus);
		}
		pos = global_pos = (GUI.wnd_size - size)/2;
		for(int i=0; i<5; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		textbox.global_pos = global_pos + textbox.pos;
	}
	else if(e == GuiEvent_GainFocus)
	{
		textbox.focus = true;
		textbox.Event(GuiEvent_GainFocus);
	}
	else if(e == GuiEvent_LostFocus)
	{
		textbox.focus = false;
		textbox.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_Close)
	{
		textbox.focus = false;
		textbox.Event(GuiEvent_LostFocus);
		visible = false;
	}
	else if(e >= GuiEvent_Custom)
		event(e);
}

//=================================================================================================
void MultiplayerPanel::Show()
{
	textbox.SetText(game->player_name.c_str());
	GUI.ShowDialog(this);
}
