#include "Pch.h"
#include "GameCore.h"
#include "SaveLoadPanel.h"
#include "SaveState.h"
#include "Language.h"
#include "KeyStates.h"
#include "Class.h"
#include "Scrollbar.h"
#include "Net.h"
#include "World.h"
#include "Level.h"
#include "Game.h"
#include "GlobalGui.h"
#include "GetTextDialog.h"
#include "GameMenu.h"
#include "CreateServerPanel.h"
#include "Unit.h"
#include "GameFile.h"
#include "DirectX.h"

//=================================================================================================
SaveLoad::SaveLoad(const DialogInfo& info) : GameDialogBox(info), choice(0), tMiniSave(nullptr)
{
	size = Int2(610, 400);

	bt[0].pos = Int2(238, 344);
	bt[0].parent = this;
	bt[0].id = IdOk;
	bt[0].size = Int2(160, 50);

	bt[1].pos = Int2(238 + 160 + 16, 344);
	bt[1].parent = this;
	bt[1].id = IdCancel;
	bt[1].size = Int2(160, 50);

	textbox.pos = Int2(265, 52);
	textbox.size = Int2(297, 88);
	textbox.parent = this;
	textbox.SetMultiline(true);
	textbox.SetReadonly(true);
	textbox.AddScrollbar();
}

//=================================================================================================
void SaveLoad::LoadLanguage()
{
	Language::Section s = Language::GetSection("SaveLoad");
	txSaving = s.Get("SAVINGGAME");
	txLoading = s.Get("LOADINGGAME");
	txSave = s.Get("save");
	txLoad = s.Get("load");
	txSaveN = s.Get("saveN");
	txQuickSave = s.Get("quickSave");
	txEmptySlot = s.Get("emptySlot");
	txSaveDate = s.Get("saveDate");
	txSaveTime = s.Get("saveTime");
	txSavePlayers = s.Get("savePlayers");
	txSaveName = s.Get("saveName");
	txSavedGameN = s.Get("savedGameN");

	bt[1].text = GUI.txCancel;
}

//=================================================================================================
void SaveLoad::Draw(ControlDrawData*)
{
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);
	Rect r = { global_pos.x, global_pos.y + 8, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(GUI.fBig, save_mode ? txSaving : txLoading, DTF_CENTER, Color::Black, r);
	for(int i = 0; i < 2; ++i)
		bt[i].Draw();
	textbox.Draw();

	// slot names
	r = Rect::Create(global_pos + Int2(12, 76), Int2(256, 20));
	for(int i = 0; i < SaveSlot::MAX_SLOTS; ++i)
	{
		cstring text;
		if(slots[i].valid)
		{
			if(slots[i].text.empty())
				text = Format(txSaveN, i + 1);
			else
				text = slots[i].text.c_str();
		}
		else
		{
			if(i == SaveSlot::MAX_SLOTS - 1)
				text = txQuickSave;
			else
				text = Format(txEmptySlot, i + 1);
		}

		GUI.DrawText(GUI.default_font, text, DTF_SINGLELINE | DTF_VCENTER, choice == i ? Color::Green : Color::Black, r);

		r.Top() = r.Bottom() + 4;
		r.Bottom() = r.Top() + 20;
	}

	// image
	if(tMiniSave)
	{
		Rect r2 = Rect::Create(Int2(global_pos.x + 400 - 81, global_pos.y + 42 + 103), Int2(256, 192));
		GUI.DrawSpriteRect(tMiniSave, r2);
	}
}

//=================================================================================================
void SaveLoad::Update(float dt)
{
	textbox.mouse_focus = focus;
	textbox.Update(dt);

	if(focus && Key.Focus())
	{
		Rect rect = Rect::Create(Int2(global_pos.x + 12, global_pos.y + 76), Int2(256, 20));

		for(int i = 0; i < SaveSlot::MAX_SLOTS; ++i)
		{
			if(rect.IsInside(GUI.cursor_pos))
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.PressedRelease(VK_LBUTTON) && choice != i)
				{
					choice = i;
					if(!save_mode)
						bt[0].state = slots[i].valid ? Button::NONE : Button::DISABLED;
					SetSaveImage();
					SetText();
				}
			}

			rect.Top() = rect.Bottom() + 4;
			rect.Bottom() = rect.Top() + 20;
		}

		if(Key.PressedRelease(VK_ESCAPE))
			Event((GuiEvent)IdCancel);
	}

	for(int i = 0; i < 2; ++i)
	{
		bt[i].mouse_focus = focus;
		bt[i].Update(dt);
	}
}

//=================================================================================================
void SaveLoad::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		global_pos = pos = (GUI.wnd_size - size) / 2;
		for(int i = 0; i < 2; ++i)
			bt[i].global_pos = bt[i].pos + global_pos;
		textbox.global_pos = textbox.pos + global_pos;
		textbox.scrollbar->global_pos = textbox.scrollbar->pos + textbox.global_pos;
		if(e == GuiEvent_Show)
			SetText();
	}
	else if(e >= GuiEvent_Custom)
	{
		if(e == IdCancel)
		{
			N.mp_load = false;
			GUI.CloseDialog(this);
			return;
		}

		if(save_mode)
		{
			// saving
			SaveSlot& slot = slots[choice];
			if(choice == SaveSlot::MAX_SLOTS - 1)
			{
				// quicksave
				GUI.CloseDialog(this);
				game->SaveGameSlot(choice + 1, txQuickSave);
			}
			else
			{
				// enter save title
				cstring names[] = { nullptr, txSave };
				if(slot.valid)
					save_input_text = slot.text;
				else if(game->hardcore_mode)
					save_input_text = hardcore_savename;
				else
					save_input_text.clear();
				GetTextDialogParams params(txSaveName, save_input_text);
				params.custom_names = names;
				params.event = [this](int id)
				{
					if(id == BUTTON_OK && game->SaveGameSlot(choice + 1, save_input_text.c_str()))
					{
						GUI.CloseDialog(this);
					}
				};
				params.parent = this;
				GetTextDialog::Show(params);
			}
		}
		else
		{
			// load
			CloseDialog();
			game->TryLoadGame(choice + 1, false, false);
		}
	}
}

//=================================================================================================
void SaveLoad::SetSaveMode(bool save_mode, bool online, SaveSlot* slots)
{
	this->save_mode = save_mode;
	this->online = online;
	this->slots = slots;

	SaveSlot& slot = slots[choice];

	// setup buttons
	if(save_mode)
	{
		bt[0].state = Button::NONE;
		bt[0].text = txSave;
	}
	else
	{
		bt[0].state = slot.valid ? Button::NONE : Button::DISABLED;
		bt[0].text = txLoad;
	}

	SetSaveImage();
}

//=================================================================================================
void SaveLoad::SetSaveImage()
{
	SaveSlot& slot = slots[choice];
	SafeRelease(tMiniSave);
	if(slot.valid)
	{
		if(slot.img_size == 0)
		{
			cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice + 1);
			if(io::FileExists(filename))
				V(D3DXCreateTextureFromFile(GUI.GetDevice(), filename, &tMiniSave));
		}
		else
		{
			cstring filename = Format("saves/%s/%d.sav", online ? "multi" : "single", choice + 1);
			Buffer* buf = FileReader::ReadToBuffer(filename, slot.img_offset, slot.img_size);
			V(D3DXCreateTextureFromFileInMemory(GUI.GetDevice(), buf->Data(), buf->Size(), &tMiniSave));
			buf->Free();
		}
	}
}

//=================================================================================================
void SaveLoad::SetText()
{
	textbox.Reset();
	if(choice == -1 || !slots[choice].valid)
		return;

	LocalString s;
	SaveSlot& slot = slots[choice];

	bool exists = false;
	if(!slot.player_name.empty())
	{
		s += slot.player_name;
		exists = true;
	}
	if(slot.player_class != Class::INVALID)
	{
		if(exists)
			s += " ";
		s += ClassInfo::classes[(int)slot.player_class].name;
		exists = true;
	}
	if(slot.hardcore)
	{
		if(exists)
			s += " ";
		s += "(hardcore)";
		exists = true;
	}
	if(exists)
		s += "\n";
	if(online && !slot.mp_players.empty())
	{
		s += txSavePlayers;
		bool first = true;
		for(string& str : slot.mp_players)
		{
			if(first)
				first = false;
			else
				s += ", ";
			s += str;
		}
		s += "\n";
	}
	if(slot.save_date != 0)
	{
		tm t;
		localtime_s(&t, &slot.save_date);
		s += Format(txSaveDate, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	}
	if(slot.game_year != -1 && slot.game_month != -1 && slot.game_day != -1)
		s += Format(txSaveTime, W.GetDate(slot.game_year, slot.game_month, slot.game_day));
	if(!slot.location.empty())
		s += slot.location;

	textbox.SetText(s);
	textbox.UpdateScrollbar();
}

//=================================================================================================
void SaveLoad::RemoveHardcoreSave(int slot)
{
	SaveSlot& s = single_saves[slot - 1];
	s.valid = false;
	hardcore_savename = s.text;
}

//=================================================================================================
void SaveLoad::LoadSaveSlots()
{
	for(int multi = 0; multi < 2; ++multi)
	{
		for(int i = 1; i <= SaveSlot::MAX_SLOTS; ++i)
		{
			SaveSlot& slot = (multi == 0 ? single_saves : multi_saves)[i - 1];
			cstring filename = Format("saves/%s/%d.sav", multi == 0 ? "single" : "multi", i);
			GameReader f(filename);
			if(!game->LoadGameHeader(f, slot))
			{
				if(i == SaveSlot::MAX_SLOTS)
					slot.text = txQuickSave;
				continue;
			}

			slot.valid = true;
			if(slot.load_version < V_0_9)
			{
				filename = Format("saves/%s/%d.txt", multi == 0 ? "single" : "multi", i);
				if(io::FileExists(filename))
				{
					Config cfg;
					cfg.Load(filename);
					slot.player_name = cfg.GetString("player_name", "");
					slot.location = cfg.GetString("location", "");
					slot.text = cfg.GetString("text", "");
					slot.game_day = cfg.GetInt("game_day");
					slot.game_month = cfg.GetInt("game_month");
					slot.game_year = cfg.GetInt("game_year");
					slot.hardcore = cfg.GetBool("hardcore");
					slot.mp_players.clear();
					slot.save_date = cfg.GetInt64("save_date");
					const string& str = cfg.GetString("player_class");
					if(str == "0")
						slot.player_class = Class::WARRIOR;
					else if(str == "1")
						slot.player_class = Class::HUNTER;
					else if(str == "2")
						slot.player_class = Class::ROGUE;
					else
					{
						ClassInfo* ci = ClassInfo::Find(str);
						if(ci && ci->pickable)
							slot.player_class = ci->class_id;
						else
							slot.player_class = Class::INVALID;
					}
				}
				else
				{
					slot.player_name.clear();
					slot.text.clear();
					slot.location.clear();
					slot.game_day = -1;
					slot.game_month = -1;
					slot.game_year = -1;
					slot.player_class = Class::INVALID;
					slot.mp_players.clear();
					slot.save_date = 0;
					slot.hardcore = false;
				}
				slot.img_size = 0;
			}

			if(i == SaveSlot::MAX_SLOTS)
				slot.text = txQuickSave;
		}
	}
}

//=================================================================================================
void SaveLoad::ShowSavePanel()
{
	SetSaveMode(true, Net::IsOnline(), Net::IsOnline() ? multi_saves : single_saves);
	GUI.ShowDialog(this);
}

//=================================================================================================
void SaveLoad::ShowLoadPanel()
{
	bool online = (N.mp_load || Net::IsServer());
	SetSaveMode(false, online, online ? multi_saves : single_saves);
	GUI.ShowDialog(this);
}

//=================================================================================================
SaveSlot& SaveLoad::GetSaveSlot(int slot)
{
	return (Net::IsOnline() ? multi_saves[slot - 1] : single_saves[slot - 1]);
}
