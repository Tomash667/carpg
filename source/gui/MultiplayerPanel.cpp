#include "Pch.h"
#include "MultiplayerPanel.h"

#include "Game.h"
#include "GameGui.h"
#include "Language.h"

#include <Input.h>

//=================================================================================================
MultiplayerPanel::MultiplayerPanel(const DialogInfo& info) : DialogBox(info)
{
	size = Int2(240, 374);
	bts.resize(5);

	const Int2 btSize(180, 44);
	const int x = (size.x - btSize.x) / 2;

	bts[0].parent = this;
	bts[0].id = IdJoinLan;
	bts[0].size = btSize;
	bts[0].pos = Int2(x, 110);

	bts[1].parent = this;
	bts[1].id = IdJoinIp;
	bts[1].size = btSize;
	bts[1].pos = Int2(x, 160);

	bts[2].parent = this;
	bts[2].id = IdCreate;
	bts[2].size = btSize;
	bts[2].pos = Int2(x, 210);

	bts[3].parent = this;
	bts[3].id = IdLoad;
	bts[3].size = btSize;
	bts[3].pos = Int2(x, 260);

	bts[4].parent = this;
	bts[4].id = IdCancel;
	bts[4].size = btSize;
	bts[4].pos = Int2(x, 310);

	textbox.limit = 16;
	textbox.parent = this;
	textbox.pos = Int2((size.x - 200) / 2, 70);
	textbox.size = Int2(200, 32);

	visible = false;
}

//=================================================================================================
void MultiplayerPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("MultiplayerPanel");

	txMultiplayerGame = s.Get("multiplayerGame");
	txNeedEnterNick = s.Get("needEnterNick");
	txEnterValidNick = s.Get("enterValidNick");

	textbox.label = s.Get("nick");

	bts[0].text = s.Get("join");
	bts[1].text = s.Get("joinIP");
	bts[2].text = s.Get("host");
	bts[3].text = s.Get("load");
	bts[4].text = gui->txCancel;
}

//=================================================================================================
void MultiplayerPanel::Draw()
{
	DrawPanel();

	// tekst
	Rect r = { globalPos.x + 12, globalPos.y + 8, globalPos.x + size.x - 12, globalPos.y + size.y };
	gui->DrawText(GameGui::fontBig, txMultiplayerGame, DTF_TOP | DTF_CENTER, Color::Black, r);

	// textbox
	textbox.Draw();

	// przyciski
	for(int i = 0; i < 5; ++i)
		bts[i].Draw();
}

//=================================================================================================
void MultiplayerPanel::Update(float dt)
{
	for(int i = 0; i < 5; ++i)
	{
		bts[i].mouseFocus = focus;
		bts[i].Update(dt);
	}

	textbox.mouseFocus = focus;
	textbox.Update(dt);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void MultiplayerPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		if(e == GuiEvent_Show)
		{
			visible = true;
			textbox.focus = true;
			textbox.Event(GuiEvent_GainFocus);
		}
		pos = globalPos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 5; ++i)
			bts[i].globalPos = globalPos + bts[i].pos;
		textbox.globalPos = globalPos + textbox.pos;
		break;
	case GuiEvent_GainFocus:
		textbox.focus = true;
		textbox.Event(GuiEvent_GainFocus);
		break;
	case GuiEvent_LostFocus:
		textbox.focus = false;
		textbox.Event(GuiEvent_LostFocus);
		break;
	case GuiEvent_Close:
		textbox.focus = false;
		textbox.Event(GuiEvent_LostFocus);
		visible = false;
		break;
	case IdCancel:
		CloseDialog();
		break;
	default:
		if(e >= GuiEvent_Custom)
			event(e);
		break;
	}
}

//=================================================================================================
void MultiplayerPanel::Show()
{
	textbox.SetText(game->playerName.c_str());
	gui->ShowDialog(this);
}
