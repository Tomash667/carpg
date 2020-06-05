#include "Pch.h"
#include "SaveLoadPanel.h"

#include "Class.h"
#include "CreateServerPanel.h"
#include "Game.h"
#include "GameFile.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "Language.h"
#include "Level.h"
#include "Net.h"
#include "SaveState.h"
#include "Unit.h"
#include "World.h"

#include <GetTextDialog.h>
#include <Input.h>
#include <ResourceManager.h>
#include <Scrollbar.h>

//=================================================================================================
SaveLoad::SaveLoad(const DialogInfo& info) : DialogBox(info), choice(0)
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

	bt[1].text = gui->txCancel;
}

//=================================================================================================
void SaveLoad::Draw(ControlDrawData*)
{
	DrawPanel();
	Rect r = { global_pos.x, global_pos.y + 8, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(GameGui::font_big, save_mode ? txSaving : txLoading, DTF_CENTER, Color::Black, r);
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

		gui->DrawText(GameGui::font, text, DTF_SINGLELINE | DTF_VCENTER, choice == i ? Color::Green : Color::Black, r);

		r.Top() = r.Bottom() + 4;
		r.Bottom() = r.Top() + 20;
	}

	// image
	if(tMiniSave.tex)
	{
		Rect r2 = Rect::Create(Int2(global_pos.x + 400 - 81, global_pos.y + 42 + 103), Int2(256, 192));
		gui->DrawSpriteRect(&tMiniSave, r2);
	}
}

//=================================================================================================
void SaveLoad::Update(float dt)
{
	textbox.mouse_focus = focus;
	textbox.Update(dt);

	if(focus && input->Focus())
	{
		Rect rect = Rect::Create(Int2(global_pos.x + 12, global_pos.y + 76), Int2(256, 20));

		for(int i = 0; i < SaveSlot::MAX_SLOTS; ++i)
		{
			if(rect.IsInside(gui->cursor_pos))
			{
				gui->cursor_mode = CURSOR_HOVER;
				if(input->PressedRelease(Key::LeftButton) && choice != i)
				{
					choice = i;
					ValidateSelectedSave();
					if(!save_mode)
						bt[0].state = slots[i].valid ? Button::NONE : Button::DISABLED;
					SetSaveImage();
					SetText();
				}
				if(gui->DoubleClick(Key::LeftButton) && (save_mode || slots[i].valid))
					Event((GuiEvent)IdOk);
			}

			rect.Top() = rect.Bottom() + 4;
			rect.Bottom() = rect.Top() + 20;
		}

		if(input->PressedRelease(Key::Escape))
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
		global_pos = pos = (gui->wnd_size - size) / 2;
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
			net->mp_load = false;
			gui->CloseDialog(this);
			return;
		}

		if(save_mode)
		{
			// saving
			if(choice == SaveSlot::MAX_SLOTS - 1)
			{
				// quicksave
				gui->CloseDialog(this);
				game->SaveGameSlot(choice + 1, txQuickSave);
			}
			else
			{
				// enter save title
				SaveSlot& slot = slots[choice];
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
						gui->CloseDialog(this);
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

	ValidateSelectedSave();

	// setup buttons
	if(save_mode)
	{
		bt[0].state = Button::NONE;
		bt[0].text = txSave;
	}
	else
	{
		SaveSlot& slot = slots[choice];
		bt[0].state = slot.valid ? Button::NONE : Button::DISABLED;
		bt[0].text = txLoad;
	}

	SetSaveImage();
}

//=================================================================================================
void SaveLoad::SetSaveImage()
{
	SaveSlot& slot = slots[choice];
	tMiniSave.Release();
	if(slot.valid)
	{
		if(slot.img_size == 0)
		{
			cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice + 1);
			if(io::FileExists(filename))
			{
				tMiniSave.tex = res_mgr->LoadRawTexture(filename);
				tMiniSave.state = ResourceState::Loaded;
			}
		}
		else
		{
			cstring filename = Format("saves/%s/%d.sav", online ? "multi" : "single", choice + 1);
			Buffer* buf = FileReader::ReadToBuffer(filename, slot.img_offset, slot.img_size);
			tMiniSave.tex = res_mgr->LoadRawTexture(buf);
			tMiniSave.state = ResourceState::Loaded;
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
	if(slot.player_class)
	{
		if(exists)
			s += " ";
		s += slot.player_class->name;
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
	if(slot.game_date.IsValid())
		s += Format(txSaveTime, world->GetDate(slot.game_date));
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
					slot.game_date = Date(cfg.GetInt("game_year", -1),
						cfg.GetInt("game_month", -1),
						cfg.GetInt("game_day", -1));
					slot.hardcore = cfg.GetBool("hardcore");
					slot.mp_players.clear();
					slot.save_date = cfg.GetInt("save_date");
					const string& str = cfg.GetString("player_class");
					if(str == "0")
						slot.player_class = Class::TryGet("warrior");
					else if(str == "1")
						slot.player_class = Class::TryGet("hunter");
					else if(str == "2")
						slot.player_class = Class::TryGet("rogue");
					else
						slot.player_class = Class::TryGet(str);
				}
				else
				{
					slot.player_name.clear();
					slot.text.clear();
					slot.location.clear();
					slot.game_date = Date(-1, -1, -1);
					slot.player_class = nullptr;
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
	gui->ShowDialog(this);
}

//=================================================================================================
void SaveLoad::ShowLoadPanel()
{
	bool online = (net->mp_load || Net::IsServer());
	SetSaveMode(false, online, online ? multi_saves : single_saves);
	gui->ShowDialog(this);
}

//=================================================================================================
SaveSlot& SaveLoad::GetSaveSlot(int slot)
{
	return (Net::IsOnline() ? multi_saves[slot - 1] : single_saves[slot - 1]);
}

//=================================================================================================
void SaveLoad::ValidateSelectedSave()
{
	SaveSlot& slot = Net::IsOnline() ? multi_saves[choice] : single_saves[choice];
	cstring filename = Format("saves/%s/%d.sav", !Net::IsOnline() ? "single" : "multi", choice + 1);
	GameReader f(filename);
	if(!game->LoadGameHeader(f, slot))
	{
		if(choice == SaveSlot::MAX_SLOTS)
			slot.text = txQuickSave;
		slot.valid = false;
	}
	else
		slot.valid = true;
}
