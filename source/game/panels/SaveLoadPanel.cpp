#include "Pch.h"
#include "Base.h"
#include "SaveLoadPanel.h"
#include "Language.h"
#include "KeyStates.h"
#include "Class.h"
#include "Scrollbar.h"

//=================================================================================================
SaveLoad::SaveLoad(const DialogInfo& info) : Dialog(info), choice(0), tMiniSave(nullptr)
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

	size = INT2(610,400);

	bt[0].pos = INT2(238,344);
	bt[0].parent = this;
	bt[0].id = IdOk;
	bt[0].size = INT2(160,50);

	bt[1].pos = INT2(238+160+16,344);
	bt[1].parent = this;
	bt[1].id = IdCancel;
	bt[1].size = INT2(160,50);
	bt[1].text = GUI.txCancel;

	textbox.pos = INT2(265,52);
	textbox.size = INT2(297,88);
	textbox.parent = this;
	textbox.multiline = true;
	textbox.readonly = true;
	textbox.AddScrollbar();
}

//=================================================================================================
void SaveLoad::Draw(ControlDrawData* /*cdd*/)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);
	RECT r = {global_pos.x, global_pos.y+8, global_pos.x+size.x, global_pos.y+size.y};
	GUI.DrawText(GUI.fBig, save_mode ? txSaving : txLoading, DT_CENTER, BLACK, r);
	for(int i=0; i<2; ++i)
		bt[i].Draw();
	textbox.Draw();

	// nazwy slotów
	r.left = global_pos.x+12;
	r.right = r.left+256;
	r.top = global_pos.y+12+64;
	r.bottom = r.top+20;
	for(int i=0; i<MAX_SAVE_SLOTS; ++i)
	{
		cstring text;
		if(slots[i].valid)
		{
			if(slots[i].text.empty())
				text = Format(txSaveN, i+1);
			else
				text = slots[i].text.c_str();
		}
		else
		{
			if(i == MAX_SAVE_SLOTS-1)
				text = txQuickSave;
			else
				text = Format(txEmptySlot, i+1);
		}

		GUI.DrawText(GUI.default_font, text, DT_SINGLELINE|DT_VCENTER, choice == i ? GREEN : BLACK, r);

		r.top = r.bottom+4;
		r.bottom = r.top+20;
	}

	// obrazek
	if(tMiniSave)
	{
		RECT r2 = {global_pos.x+400-81, global_pos.y+42+103,0,0};
		r2.right = r2.left + 256;
		r2.bottom = r2.top + 192;
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
		RECT rect;
		rect.left = global_pos.x+12;
		rect.right = rect.left + 256;
		rect.top = global_pos.y+12+64;
		rect.bottom = rect.top+20;

		for(int i=0; i<MAX_SAVE_SLOTS; ++i)
		{
			if(PointInRect(GUI.cursor_pos, rect))
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
						cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice+1);
						if(FileExists(filename))
							D3DXCreateTextureFromFile(GUI.GetDevice(), filename, &tMiniSave);
					}
					SetText();
				}
			}

			rect.top = rect.bottom+4;
			rect.bottom = rect.top+20;
		}

		if(Key.PressedRelease(VK_ESCAPE))
			Event((GuiEvent)IdCancel);
	}

	for(int i=0; i<2; ++i)
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
		global_pos = pos = (GUI.wnd_size - size)/2;
		for(int i=0; i<2; ++i)
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
		cstring filename = Format("saves/%s/%d.jpg", online ? "multi" : "single", choice+1);
		if(FileExists(filename))
			D3DXCreateTextureFromFile(GUI.GetDevice(), filename, &tMiniSave);
	}
}

//=================================================================================================
void SaveLoad::SetText()
{
	textbox.Reset();
	if(choice == -1 || !slots[choice].valid)
		return;

	string& s = textbox.text;
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
		s += g_classes[(int)slot.player_class].name;
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
		s += Format(txSaveDate, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	}
	if(slot.game_year != -1 && slot.game_month != -1 && slot.game_day != -1)
		s += Format(txSaveTime, slot.game_year, slot.game_month+1, slot.game_day+1);
	if(!slot.location.empty())
		s += slot.location;
	if(online)
	{
		if(slot.multiplayers != -1)
			s += Format(txSavePlayers, slot.multiplayers);
	}

	textbox.UpdateScrollbar();
}
