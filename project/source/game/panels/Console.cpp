#include "Pch.h"
#include "GameCore.h"
#include "Console.h"
#include "KeyStates.h"
#include "Game.h"
#include "ResourceManager.h"

//=================================================================================================
Console::Console(const DialogInfo& info) : GameDialogBox(info), added(false)
{
	size = Int2(GUI.wnd_size.x, GUI.wnd_size.y / 3);
	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = true;
	itb.lose_focus = false;
	itb.pos = Int2(0, 0);
	itb.global_pos = Int2(0, 0);
	itb.size = Int2(GUI.wnd_size.x, GUI.wnd_size.y / 3);
	itb.event = InputEvent(this, &Console::OnInput);
	itb.background = nullptr;
	itb.Init();
}

//=================================================================================================
void Console::LoadData()
{
	ResourceManager::Get<Texture>().AddLoadTask("tlo_konsoli.jpg", tBackground);
}

//=================================================================================================
void Console::Draw(ControlDrawData*)
{
	// t³o
	Rect r = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y / 3 };
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
		if(GKey.KeyDownUp(GK_CONSOLE))
		{
			Event(GuiEvent_LostFocus);
			GUI.CloseDialog(this);
		}
		else if(focus)
		{
			if(Key.Shortcut(KEY_CONTROL, 'V'))
			{
				cstring text = GUI.GetClipboard();
				if(text)
					itb.input += text;
			}
			else if(Key.PressedRelease(VK_TAB))
			{
				// autocomplete on tab key
				string s = Trimmed(itb.input);
				if(!s.empty() && s.find_first_of(' ') == string::npos)
				{
					cstring best_cmd_name = nullptr;
					int best_index = -1, index = 0;

					for(ConsoleCommand& cmd : Game::Get().cmds)
					{
						if(strncmp(cmd.name, s.c_str(), s.length()) == 0)
						{
							if(best_index == -1 || strcmp(cmd.name, best_cmd_name) < 0)
							{
								best_index = index;
								best_cmd_name = cmd.name;
							}
						}
						++index;
					}

					if(best_index != -1)
					{
						itb.input = best_cmd_name;
						itb.input += ' ';
					}
				}
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
		size = Int2(GUI.wnd_size.x, GUI.wnd_size.y / 3);
		itb.Event(GuiEvent_Resize);
	}
	else if(e == GuiEvent_Moved)
		itb.Event(GuiEvent_Moved);
}

//=================================================================================================
void Console::OnInput(const string& str)
{
	Game::Get().ParseCommand(str, PrintMsgFunc(this, &Console::AddMsg), PS_CONSOLE);
}
