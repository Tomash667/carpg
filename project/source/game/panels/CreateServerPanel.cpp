#include "Pch.h"
#include "GameCore.h"
#include "CreateServerPanel.h"
#include "Language.h"
#include "KeyStates.h"
#include "Const.h"
#include "Game.h"

//=================================================================================================
CreateServerPanel::CreateServerPanel(const DialogInfo& info) : GameDialogBox(info)
{
	size = Int2(344, 320);
	bts.resize(2);

	const Int2 bt_size(180, 44);
	const int x = (size.x - bt_size.x) / 2;

	bts[0].id = GuiEvent_Custom + BUTTON_OK;
	bts[0].parent = this;
	bts[0].pos = Int2(x, 220);
	bts[0].size = bt_size;

	bts[1].id = GuiEvent_Custom + BUTTON_CANCEL;
	bts[1].parent = this;
	bts[1].pos = Int2(x, 270);
	bts[1].size = bt_size;

	textbox[0].limit = 16;
	textbox[0].parent = this;
	textbox[0].pos = Int2(60, 70);
	textbox[0].size = Int2(200, 32);

	textbox[1].limit = 16;
	textbox[1].parent = this;
	textbox[1].pos = Int2(60, 120);
	textbox[1].size = Int2(200, 32);
	textbox[1].SetNumeric(true);
	textbox[1].num_min = MIN_PLAYERS;
	textbox[1].num_max = MAX_PLAYERS;

	textbox[2].limit = 16;
	textbox[2].parent = this;
	textbox[2].pos = Int2(60, 170);
	textbox[2].size = Int2(200, 32);

	for(int i = 0; i < 2; ++i)
		cont.Add(bts[i]);

	for(int i = 0; i < 3; ++i)
		cont.Add(textbox[i]);
}

//=================================================================================================
void CreateServerPanel::LoadLanguage()
{
	txCreateServer = Str("createServer");
	txEnterServerName = Str("enterServerName");
	txInvalidPlayersCount = Str("invalidPlayersCount");

	bts[0].text = Str("create");
	bts[1].text = GUI.txCancel;

	textbox[0].label = Str("serverName");
	textbox[1].label = Str("serverPlayers");
	textbox[2].label = Str("serverPswd");
}

//=================================================================================================
void CreateServerPanel::Draw(ControlDrawData*)
{
	// t�o
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	// tekst
	Rect r = { global_pos.x + 12, global_pos.y + 8, global_pos.x + size.x - 12, global_pos.y + size.y };
	GUI.DrawText(GUI.fBig, txCreateServer, DTF_TOP | DTF_CENTER, Color::Black, r);

	// reszta
	cont.Draw();
}

//=================================================================================================
void CreateServerPanel::Update(float dt)
{
	cont.Update(dt);

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)(GuiEvent_Custom + BUTTON_CANCEL));
}

//=================================================================================================
void CreateServerPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			cont.GainFocus();
			visible = true;
		}
		global_pos = pos = (GUI.wnd_size - size) / 2;
		cont.Move(global_pos);
	}
	else if(e == GuiEvent_GainFocus)
		cont.GainFocus();
	else if(e == GuiEvent_Close)
	{
		visible = false;
		cont.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
		cont.LostFocus();
	if(e == GuiEvent_Custom + BUTTON_OK)
		event(BUTTON_OK);
	else if(e == GuiEvent_Custom + BUTTON_CANCEL)
		event(BUTTON_CANCEL);
}

//=================================================================================================
void CreateServerPanel::Show()
{
	textbox[0].SetText(N.server_name.c_str());
	textbox[1].SetText(Format("%d", N.max_players));
	textbox[2].SetText(N.password.c_str());
	GUI.ShowDialog(this);
}
