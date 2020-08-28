#include "Pch.h"
#include "CreateServerPanel.h"

#include "Const.h"
#include "GameGui.h"
#include "Language.h"
#include "Net.h"

#include <Input.h>

//=================================================================================================
CreateServerPanel::CreateServerPanel(const DialogInfo& info) : DialogBox(info)
{
	size = Int2(344, 360);
	bts.resize(2);

	const Int2 bt_size(180, 44);
	const int x = (size.x - bt_size.x) / 2;

	bts[0].id = IdOk;
	bts[0].parent = this;
	bts[0].pos = Int2(x, 260);
	bts[0].size = bt_size;

	bts[1].id = IdCancel;
	bts[1].parent = this;
	bts[1].pos = Int2(x, 310);
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

	checkbox.bt_size = Int2(32, 32);
	checkbox.checked = false;
	checkbox.id = IdHidden;
	checkbox.parent = this;
	checkbox.pos = Int2(60, 215);
	checkbox.size = Int2(200, 32);

	for(int i = 0; i < 2; ++i)
		cont.Add(bts[i]);
	for(int i = 0; i < 3; ++i)
		cont.Add(textbox[i]);
	cont.Add(checkbox);
}

//=================================================================================================
void CreateServerPanel::LoadLanguage()
{
	auto s = Language::GetSection("CreateServerPanel");

	txCreateServer = s.Get("createServer");
	txEnterServerName = s.Get("enterServerName");
	txInvalidPlayersCount = s.Get("invalidPlayersCount");
	txConfirmMaxPlayers = s.Get("confirmMaxPlayers");

	bts[0].text = s.Get("create");
	bts[1].text = gui->txCancel;

	textbox[0].label = s.Get("serverName");
	textbox[1].label = s.Get("serverPlayers");
	textbox[2].label = s.Get("serverPswd");

	checkbox.text = s.Get("lan");
}

//=================================================================================================
void CreateServerPanel::Draw(ControlDrawData*)
{
	DrawPanel();

	// tekst
	Rect r = { global_pos.x + 12, global_pos.y + 8, global_pos.x + size.x - 12, global_pos.y + size.y };
	gui->DrawText(GameGui::font_big, txCreateServer, DTF_TOP | DTF_CENTER, Color::Black, r);

	// reszta
	cont.Draw();
}

//=================================================================================================
void CreateServerPanel::Update(float dt)
{
	cont.Update(dt);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void CreateServerPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		if(e == GuiEvent_Show)
		{
			cont.GainFocus();
			visible = true;
		}
		global_pos = pos = (gui->wnd_size - size) / 2;
		cont.Move(global_pos);
		break;
	case GuiEvent_GainFocus:
		cont.GainFocus();
		break;
	case GuiEvent_Close:
		visible = false;
		cont.LostFocus();
		break;
	case GuiEvent_LostFocus:
		cont.LostFocus();
		break;
	case IdOk:
		{
			int players;
			TextHelper::ToInt(textbox[1].GetText().c_str(), players);
			if(players > MAX_PLAYERS_WARNING)
			{
				DialogInfo dialog = {};
				dialog.event = [this](int id) { if(id == BUTTON_YES) event(BUTTON_OK); };
				dialog.name = "confirm_max_players";
				dialog.order = ORDER_TOP;
				dialog.parent = this;
				dialog.text = txConfirmMaxPlayers;
				dialog.type = DIALOG_YESNO;
				gui->ShowDialog(dialog);
			}
			else
				event(BUTTON_OK);
		}
		break;
	case IdCancel:
		event(BUTTON_CANCEL);
		break;
	}
}

//=================================================================================================
void CreateServerPanel::Show()
{
	textbox[0].SetText(net->server_name.c_str());
	textbox[1].SetText(Format("%d", net->max_players));
	textbox[2].SetText(net->password.c_str());
	checkbox.checked = net->server_lan;
	gui->ShowDialog(this);
}
