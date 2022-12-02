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
	size = Int2(240, 376);
	bts.resize(2);

	const Int2 btSize(180, 44);
	const int x = (size.x - btSize.x) / 2;

	bts[0].id = IdOk;
	bts[0].parent = this;
	bts[0].pos = Int2(x, 260);
	bts[0].size = btSize;

	bts[1].id = IdCancel;
	bts[1].parent = this;
	bts[1].pos = Int2(x, 310);
	bts[1].size = btSize;

	const Int2 textboxSize = Int2(200, 32);
	const int textboxX = (size.x - textboxSize.x) / 2;

	textbox[0].limit = 16;
	textbox[0].parent = this;
	textbox[0].pos = Int2(textboxX, 70);
	textbox[0].size = textboxSize;

	textbox[1].limit = 16;
	textbox[1].parent = this;
	textbox[1].pos = Int2(textboxX, 124);
	textbox[1].size = textboxSize;
	textbox[1].SetNumeric(true);
	textbox[1].numMin = MIN_PLAYERS;
	textbox[1].numMax = MAX_PLAYERS;

	textbox[2].limit = 16;
	textbox[2].parent = this;
	textbox[2].pos = Int2(textboxX, 178);
	textbox[2].size = textboxSize;

	checkbox.btSize = Int2(32, 32);
	checkbox.checked = false;
	checkbox.id = IdHidden;
	checkbox.parent = this;
	checkbox.pos = Int2(textboxX, 215);
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
void CreateServerPanel::Draw()
{
	DrawPanel();

	// tekst
	Rect r = { globalPos.x + 12, globalPos.y + 8, globalPos.x + size.x - 12, globalPos.y + size.y };
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
		globalPos = pos = (gui->wndSize - size) / 2;
		cont.Move(globalPos);
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
				dialog.order = DialogOrder::Top;
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
	textbox[0].SetText(net->serverName.c_str());
	textbox[1].SetText(Format("%d", net->max_players));
	textbox[2].SetText(net->password.c_str());
	checkbox.checked = net->serverLan;
	gui->ShowDialog(this);
}
