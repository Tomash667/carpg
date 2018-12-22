#include "Pch.h"
#include "GameCore.h"
#include "SaveLoadPanel.h"
#include "SaveState.h"
#include "Language.h"
#include "KeyStates.h"
#include "Class.h"
#include "Scrollbar.h"
#include "DirectX.h"
#include "Net.h"
#include "World.h"
#include "Level.h"
#include "Game.h"
#include "GlobalGui.h"
#include "GetTextDialog.h"
#include "GameMenu.h"
#include "CreateServerPanel.h"
#include "Unit.h"

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
	txSaving = Str("SAVINGGAME");
	txLoading = Str("LOADINGGAME");
	txSave = Str("save");
	txLoad = Str("load");
	txSaveN = Str("saveN");
	txQuickSave = Str("quickSave");
	txEmptySlot = Str("emptySlot");
	txSaveDate = Str("saveDate");
	txSaveTime = Str("saveTime");
	txSavePlayers = Str("savePlayers");
	txSaveName = Str("saveName");
	txSavedGameN = Str("savedGameN");
	txLoadError = Str("loadError");
	txLoadErrorGeneric = Str("loadErrorGeneric");

	bt[1].text = GUI.txCancel;
}

//=================================================================================================
void SaveLoad::Draw(ControlDrawData* /*cdd*/)
{
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);
	Rect r = { global_pos.x, global_pos.y + 8, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(GUI.fBig, save_mode ? txSaving : txLoading, DTF_CENTER, Color::Black, r);
	for(int i = 0; i < 2; ++i)
		bt[i].Draw();
	textbox.Draw();

	// nazwy slotów
	r = Rect::Create(global_pos + Int2(12, 76), Int2(256, 20));
	for(int i = 0; i < MAX_SAVE_SLOTS; ++i)
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
			if(i == MAX_SAVE_SLOTS - 1)
				text = txQuickSave;
			else
				text = Format(txEmptySlot, i + 1);
		}

		GUI.DrawText(GUI.default_font, text, DTF_SINGLELINE | DTF_VCENTER, choice == i ? Color::Green : Color::Black, r);

		r.Top() = r.Bottom() + 4;
		r.Bottom() = r.Top() + 20;
	}

	// obrazek
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

		for(int i = 0; i < MAX_SAVE_SLOTS; ++i)
		{
			if(rect.IsInside(GUI.cursor_pos))
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.PressedRelease(VK_LBUTTON) && choice != i)
				{
					choice = i;
					if(!save_mode)
						bt[0].state = slots[i].valid ? Button::NONE : Button::DISABLED;
					SafeRelease(tMiniSave);
					if(slots[i].valid)
					{
						cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice + 1);
						if(io::FileExists(filename))
							D3DXCreateTextureFromFile(GUI.GetDevice(), filename, &tMiniSave);
					}
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
			if(choice == MAX_SAVE_SLOTS - 1)
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
			TryLoad(choice + 1);
		}
	}
}

//=================================================================================================
void SaveLoad::SetSaveMode(bool sm, bool o, SaveSlot* _slots)
{
	save_mode = sm;
	online = o;
	slots = _slots;

	SaveSlot& slot = slots[choice];

	// ustaw przycisk
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

	// ustaw obrazek
	SafeRelease(tMiniSave);
	if(slot.valid)
	{
		cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice + 1);
		if(io::FileExists(filename))
			D3DXCreateTextureFromFile(GUI.GetDevice(), filename, &tMiniSave);
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
	if(slot.save_date != 0)
	{
		tm t;
		localtime_s(&t, &slot.save_date);
		s += Format(txSaveDate, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	}
	if(slot.game_year != -1 && slot.game_month != -1 && slot.game_day != -1)
		s += Format(txSaveTime, slot.game_year, slot.game_month + 1, slot.game_day + 1);
	if(!slot.location.empty())
		s += slot.location;
	if(online)
	{
		if(slot.multiplayers != -1)
			s += Format(txSavePlayers, slot.multiplayers);
	}

	textbox.SetText(s);
	textbox.UpdateScrollbar();
}

//=================================================================================================
void SaveLoad::UpdateSaveInfo(int slot)
{
	SaveSlot& ss = (Net::IsOnline() ? multi_saves[slot - 1] : single_saves[slot - 1]);
	ss.valid = true;
	ss.game_day = W.GetDay();
	ss.game_month = W.GetMonth();
	ss.game_year = W.GetYear();
	ss.location = L.GetCurrentLocationText();
	ss.player_name = game->pc->name;
	ss.player_class = game->pc->unit->GetClass();
	ss.save_date = time(nullptr);
	ss.text = (text[0] != 0 ? text : Format(txSavedGameN, slot));
	ss.hardcore = game->hardcore_mode;

	Config cfg;
	cfg.Add("game_day", ss.game_day);
	cfg.Add("game_month", ss.game_month);
	cfg.Add("game_year", ss.game_year);
	cfg.Add("location", ss.location);
	cfg.Add("player_name", ss.player_name);
	cfg.Add("player_class", ClassInfo::classes[(int)ss.player_class].id);
	cfg.Add("save_date", Format("%I64d", ss.save_date));
	cfg.Add("text", ss.text);
	cfg.Add("hardcore", ss.hardcore);

	if(Net::IsOnline())
	{
		ss.multiplayers = N.active_players;
		cfg.Add("multiplayers", ss.multiplayers);
	}

	cfg.Save(Format("saves/%s/%d.txt", Net::IsOnline() ? "multi" : "single", slot));
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
		for(int i = 1; i <= MAX_SAVE_SLOTS; ++i)
		{
			SaveSlot& slot = (multi == 0 ? single_saves : multi_saves)[i - 1];
			cstring filename = Format("saves/%s/%d.sav", multi == 0 ? "single" : "multi", i);
			if(io::FileExists(filename))
			{
				slot.valid = true;
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
					if(multi == 1)
						slot.multiplayers = cfg.GetInt("multiplayers");
					else
						slot.multiplayers = -1;
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
					slot.multiplayers = -1;
					slot.save_date = 0;
					slot.hardcore = false;
				}
			}
			else
				slot.valid = false;

			if(i == MAX_SAVE_SLOTS)
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
	SetSaveMode(false, N.mp_load, N.mp_load ? multi_saves : single_saves);
	GUI.ShowDialog(this);
}

//=================================================================================================
bool SaveLoad::TryLoad(int slot, bool quickload)
{
	try
	{
		game->LoadGameSlot(slot);
		return true;
	}
	catch(const SaveException& ex)
	{
		if(quickload && ex.missing_file)
		{
			Warn("Missing quicksave.");
			return false;
		}

		Error("Failed to load game: %s", ex.msg);
		cstring dialog_text;
		if(ex.localized_msg)
			dialog_text = Format("%s%s", txLoadError, ex.localized_msg);
		else
			dialog_text = txLoadErrorGeneric;
		GUI.SimpleDialog(dialog_text, nullptr);
		N.mp_load = false;
		return false;
	}
}
