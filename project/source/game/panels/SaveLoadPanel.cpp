#include "Pch.h"
#include "Core.h"
#include "SaveLoadPanel.h"
#include "Language.h"
#include "KeyStates.h"
#include "Class.h"
#include "Scrollbar.h"

//=================================================================================================
SaveLoad::SaveLoad(const DialogInfo& info) : DialogBox(info), choice(0), tMiniSave(nullptr)
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

	size = Int2(610, 400);

	bt[0].pos = Int2(238, 344);
	bt[0].parent = this;
	bt[0].id = IdOk;
	bt[0].size = Int2(160, 50);

	bt[1].pos = Int2(238 + 160 + 16, 344);
	bt[1].parent = this;
	bt[1].id = IdCancel;
	bt[1].size = Int2(160, 50);
	bt[1].text = GUI.txCancel;

	textbox.pos = Int2(265, 52);
	textbox.size = Int2(297, 88);
	textbox.parent = this;
	textbox.SetMultiline(true);
	textbox.SetReadonly(true);
	textbox.AddScrollbar();
}

//=================================================================================================
void SaveLoad::Draw(ControlDrawData* /*cdd*/)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255, 255, 255, 128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255, 255, 255, 222), 16);
	Rect r = { global_pos.x, global_pos.y + 8, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(GUI.fBig, save_mode ? txSaving : txLoading, DT_CENTER, BLACK, r);
	for(int i = 0; i < 2; ++i)
		bt[i].Draw();
	textbox.Draw();

	// nazwy slot�w
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

		GUI.DrawText(GUI.default_font, text, DT_SINGLELINE | DT_VCENTER, choice == i ? GREEN : BLACK, r);

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
		if(event)
			event(e);
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

	bool jest = false;
	if(!slot.player_name.empty())
	{
		s += slot.player_name;
		jest = true;
	}
	if(slot.player_class != Class::INVALID)
	{
		if(jest)
			s += " ";
		s += ClassInfo::classes[(int)slot.player_class].name;
		jest = true;
	}
	if(slot.hardcore)
	{
		if(jest)
			s += " ";
		s += "(hardcore)";
		jest = true;
	}
	if(jest)
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
