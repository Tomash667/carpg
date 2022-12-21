#include "Pch.h"
#include "Console.h"

#include "CommandParser.h"
#include "GameKeys.h"

#include <Input.h>
#include <ResourceManager.h>

//=================================================================================================
Console::Console(const DialogInfo& info) : DialogBox(info), added(false)
{
	size = Int2(gui->wndSize.x, gui->wndSize.y / 3);
	itb.parent = this;
	itb.maxCache = 10;
	itb.maxLines = 100;
	itb.escClear = true;
	itb.loseFocus = false;
	itb.pos = Int2(0, 0);
	itb.globalPos = Int2(0, 0);
	itb.size = Int2(gui->wndSize.x, gui->wndSize.y / 3);
	itb.event = InputTextBox::InputEvent(this, &Console::OnInput);
	itb.Init();
}

//=================================================================================================
void Console::LoadData()
{
	tBackground = resMgr->Load<Texture>("console_bkg.jpg");
}

//=================================================================================================
void Console::Draw()
{
	// t³o
	Rect r = { 0, 0, gui->wndSize.x, gui->wndSize.y / 3 };
	gui->DrawSpriteRect(tBackground, r, 0xAAFFFFFF);

	// tekst
	itb.Draw();
}

//=================================================================================================
void Console::Update(float dt)
{
	itb.mouseFocus = focus;
	itb.Update(dt);
	if(input->Focus())
	{
		if(GKey.KeyDownUp(GK_CONSOLE))
		{
			Event(GuiEvent_LostFocus);
			gui->CloseDialog(this);
		}
		else if(focus)
		{
			if(input->Shortcut(KEY_CONTROL, Key::V))
			{
				cstring text = gui->GetClipboard();
				if(text)
					itb.inputStr += text;
			}
			else if(input->PressedRelease(Key::Tab))
			{
				// autocomplete on tab key
				string s = Trimmed(itb.inputStr);
				if(!s.empty() && s.find_first_of(' ') == string::npos)
				{
					cstring bestCmdName = nullptr;
					int bestIndex = -1, index = 0;

					for(const ConsoleCommand& cmd : cmdp->GetCommands())
					{
						if(strncmp(cmd.name, s.c_str(), s.length()) == 0)
						{
							if(bestIndex == -1 || strcmp(cmd.name, bestCmdName) < 0)
							{
								bestIndex = index;
								bestCmdName = cmd.name;
							}
						}
						++index;
					}

					if(bestIndex != -1)
					{
						itb.inputStr = bestCmdName;
						itb.inputStr += ' ';
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
	else if(e == GuiEvent_LostFocus || e == GuiEvent_Close)
	{
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_Resize || e == GuiEvent_WindowResize)
	{
		size = Int2(gui->wndSize.x, gui->wndSize.y / 3);
		itb.Event(GuiEvent_Resize);
	}
	else if(e == GuiEvent_Moved)
		itb.Event(GuiEvent_Moved);
}

//=================================================================================================
void Console::OnInput(const string& str)
{
	cmdp->ParseCommand(str, CommandParser::PrintMsgFunc(this, &Console::AddMsg), PS_CONSOLE);
}
