#include "Pch.h"
#include "SaveLoadPanel.h"

#include "Class.h"
#include "CreateServerPanel.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "Language.h"
#include "Level.h"
#include "Net.h"
#include "Unit.h"
#include "World.h"

#include <GetTextDialog.h>
#include <Input.h>
#include <ResourceManager.h>
#include <Scrollbar.h>

//=================================================================================================
SaveLoad::SaveLoad(const DialogInfo& info) : DialogBox(info), choice(0)
{
	size = Int2(600, 414);

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
void SaveLoad::Draw()
{
	DrawPanel();
	Rect r = { globalPos.x, globalPos.y + 8, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(GameGui::font_big, saveMode ? txSaving : txLoading, DTF_CENTER, Color::Black, r);
	for(int i = 0; i < 2; ++i)
		bt[i].Draw();
	textbox.Draw();

	// slot names
	r = Rect::Create(globalPos + Int2(20, 76), Int2(256, 20));
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

		Color color;
		if(choice == i)
			color = Color::Green;
		else if(hover == i)
			color = Color(0, 128, 0);
		else
			color = Color::Black;

		gui->DrawText(GameGui::font, text, DTF_SINGLELINE | DTF_VCENTER, color, r);

		r.Top() = r.Bottom() + 4;
		r.Bottom() = r.Top() + 20;
	}

	// image
	if(tMiniSave.tex)
	{
		Rect r2 = Rect::Create(Int2(globalPos.x + 400 - 81, globalPos.y + 42 + 103), Int2(256, 192));
		gui->DrawSpriteRect(&tMiniSave, r2);
	}
}

//=================================================================================================
void SaveLoad::Update(float dt)
{
	textbox.mouseFocus = focus;
	textbox.Update(dt);
	hover = -1;

	if(focus && input->Focus())
	{
		Rect rect = Rect::Create(Int2(globalPos.x + 20, globalPos.y + 76), Int2(256, 20));

		for(int i = 0; i < SaveSlot::MAX_SLOTS; ++i)
		{
			if(rect.IsInside(gui->cursorPos))
			{
				hover = i;
				gui->SetCursorMode(CURSOR_HOVER);
				if(input->PressedRelease(Key::LeftButton) && choice != i)
				{
					choice = i;
					ValidateSelectedSave();
					if(!saveMode)
						bt[0].state = slots[i].valid ? Button::NONE : Button::DISABLED;
					SetSaveImage();
					SetText();
				}
				if(gui->DoubleClick(Key::LeftButton) && (saveMode || slots[i].valid))
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
		bt[i].mouseFocus = focus;
		bt[i].Update(dt);
	}
}

//=================================================================================================
void SaveLoad::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		hover = -1;
		globalPos = pos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 2; ++i)
			bt[i].globalPos = bt[i].pos + globalPos;
		textbox.globalPos = textbox.pos + globalPos;
		textbox.scrollbar->globalPos = textbox.scrollbar->pos + textbox.globalPos;
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

		if(saveMode)
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
					saveInputText = slot.text;
				else if(game->hardcore_mode)
					saveInputText = hardcoreSavename;
				else
					saveInputText.clear();
				GetTextDialogParams params(txSaveName, saveInputText);
				params.customNames = names;
				params.event = [this](int id)
				{
					if(id == BUTTON_OK && game->SaveGameSlot(choice + 1, saveInputText.c_str()))
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
void SaveLoad::SetSaveMode(bool saveMode, bool online, SaveSlot* slots)
{
	this->saveMode = saveMode;
	this->online = online;
	this->slots = slots;

	ValidateSelectedSave();

	// setup buttons
	if(saveMode)
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
Texture* SaveLoad::GetSaveImage(int slotIndex, bool isOnline)
{
	SaveSlot& slot = (isOnline ? multiSaves : singleSaves)[slotIndex - 1];
	tMiniSave.Release();
	if(!slot.valid)
		return nullptr;
	if(slot.img_size == 0)
	{
		cstring filename = Format("saves/%s/%d.jpg", isOnline ? "multi" : "single", slotIndex);
		if(io::FileExists(filename))
		{
			tMiniSave.tex = resMgr->LoadRawTexture(filename);
			tMiniSave.state = ResourceState::Loaded;
		}
		else
			return nullptr;
	}
	else
	{
		cstring filename = Format("saves/%s/%d.sav", isOnline ? "multi" : "single", slotIndex);
		Buffer* buf = GameReader::ReadToBuffer(filename, slot.img_offset, slot.img_size);
		tMiniSave.tex = resMgr->LoadRawTexture(buf);
		tMiniSave.state = ResourceState::Loaded;
		buf->Free();
	}
	return &tMiniSave;
}

//=================================================================================================
void SaveLoad::SetText()
{
	textbox.Reset();
	if(choice == -1 || !slots[choice].valid)
		return;

	const string& text = GetSaveText(slots[choice]);
	textbox.SetText(text.c_str());
	textbox.UpdateScrollbar();
}

//=================================================================================================
const string& SaveLoad::GetSaveText(SaveSlot& slot)
{
	saveText.clear();

	bool exists = false;
	if(!slot.player_name.empty())
	{
		saveText += slot.player_name;
		exists = true;
	}
	if(slot.player_class)
	{
		if(exists)
			saveText += " ";
		saveText += slot.player_class->name;
		exists = true;
	}
	if(slot.hardcore)
	{
		if(exists)
			saveText += " ";
		saveText += "(hardcore)";
		exists = true;
	}
	if(exists)
		saveText += "\n";
	if(online && !slot.mp_players.empty())
	{
		saveText += txSavePlayers;
		bool first = true;
		for(string& str : slot.mp_players)
		{
			if(first)
				first = false;
			else
				saveText += ", ";
			saveText += str;
		}
		saveText += "\n";
	}
	if(slot.save_date != 0)
	{
		tm t;
		localtime_s(&t, &slot.save_date);
		saveText += Format(txSaveDate, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	}
	if(slot.game_date.IsValid())
		saveText += Format(txSaveTime, world->GetDate(slot.game_date));
	if(!slot.location.empty())
		saveText += slot.location;
	return saveText;
}

//=================================================================================================
void SaveLoad::RemoveHardcoreSave(int slot)
{
	SaveSlot& s = singleSaves[slot - 1];
	s.valid = false;
	hardcoreSavename = s.text;
}

//=================================================================================================
void SaveLoad::LoadSaveSlots()
{
	for(int multi = 0; multi < 2; ++multi)
	{
		for(int i = 1; i <= SaveSlot::MAX_SLOTS; ++i)
		{
			SaveSlot& slot = (multi == 0 ? singleSaves : multiSaves)[i - 1];
			cstring filename = Format("saves/%s/%d.sav", multi == 0 ? "single" : "multi", i);
			GameReader f(filename);
			if(!game->LoadGameHeader(f, slot))
			{
				if(i == SaveSlot::MAX_SLOTS)
					slot.text = txQuickSave;
				continue;
			}

			slot.valid = true;
			if(i == SaveSlot::MAX_SLOTS)
				slot.text = txQuickSave;
		}
	}
}

//=================================================================================================
void SaveLoad::ShowSavePanel()
{
	SetSaveMode(true, Net::IsOnline(), Net::IsOnline() ? multiSaves : singleSaves);
	gui->ShowDialog(this);
}

//=================================================================================================
void SaveLoad::ShowLoadPanel()
{
	bool online = (net->mp_load || Net::IsServer());
	SetSaveMode(false, online, online ? multiSaves : singleSaves);
	gui->ShowDialog(this);
}

//=================================================================================================
SaveSlot& SaveLoad::GetSaveSlot(int slot, bool isOnline)
{
	return (isOnline ? multiSaves : singleSaves)[slot - 1];
}

//=================================================================================================
void SaveLoad::ValidateSelectedSave()
{
	SaveSlot& slot = Net::IsOnline() ? multiSaves[choice] : singleSaves[choice];
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
