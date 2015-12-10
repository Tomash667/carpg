#include "Pch.h"
#include "Base.h"
#include "Console.h"
#include "KeyStates.h"
#include "Game.h"

//-----------------------------------------------------------------------------
TEX Console::tBackground;

//=================================================================================================
Console::Console(const DialogInfo& info) : Dialog(info), added(false)
{
	size = INT2(GUI.wnd_size.x,GUI.wnd_size.y/3);
	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = true;
	itb.lose_focus = false;
	itb.pos = INT2(0,0);
	itb.global_pos = INT2(0,0);
	itb.size = INT2(GUI.wnd_size.x,GUI.wnd_size.y/3);
	itb.event = InputEvent(this, &Console::OnInput);
	itb.background = nullptr;
	itb.Init();
}

//=================================================================================================
void Console::Draw(ControlDrawData* cdd/* =nullptr */)
{
	// t³o
	RECT r = {0, 0, GUI.wnd_size.x, GUI.wnd_size.y/3};
	GUI.DrawSpriteRect(tBackground, r, 0xAAFFFFFF);

	// tekst
	itb.Draw();
}

//=================================================================================================
void Console::Update(float dt)
{
	itb.mouse_focus = focus;
	itb.Update(dt);
	if(Key.Focus())
	{
		if(game->KeyDownUp(GK_CONSOLE))
		{
			Event(GuiEvent_LostFocus);
			GUI.CloseDialog(this);
		}
		else if(focus && Key.Shortcut(VK_CONTROL, 'V'))
		{
			if(OpenClipboard(game->hwnd))
			{
				if(IsClipboardFormatAvailable(CF_TEXT) == TRUE)
				{
					HANDLE clipboard_data = GetClipboardData(CF_TEXT);
					cstring msg = (cstring)GlobalLock(clipboard_data);
					itb.input += msg;
					GlobalUnlock(clipboard_data);
				}
				CloseClipboard();
			}
		}
	}
}

//=================================================================================================
void Console::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		itb.focus = true;
		itb.Event(GuiEvent_GainFocus);
	}
	else if(e == GuiEvent_LostFocus)
	{
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_Resize || e == GuiEvent_WindowResize)
	{
		size = INT2(GUI.wnd_size.x,GUI.wnd_size.y/3);
		itb.Event(GuiEvent_Resize);
	}
	else if(e == GuiEvent_Moved)
		itb.Event(GuiEvent_Moved);
}

//=================================================================================================
void Console::AddText(cstring str)
{
	itb.Add(str);
}

//=================================================================================================
void Console::OnInput(const string& str)
{
	Game::Get().ParseCommand(str, PrintMsgFunc(this, &Console::AddText), PS_CONSOLE);
}
