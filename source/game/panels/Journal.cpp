#include "Pch.h"
#include "Base.h"
#include "Journal.h"
#include "KeyStates.h"
#include "Game.h"
#include "GetTextDialog.h"
#include "Language.h"
#include "GameKeys.h"

//-----------------------------------------------------------------------------
TEX Journal::tBook, Journal::tPage[3], Journal::tArrowL, Journal::tArrowR;

//=================================================================================================
Journal::Journal() : mode(Quests), game(Game::Get())
{
	visible = false;
	Reset();

	txAdd = Str("add");
	txNoteText = Str("noteText");
	txNoQuests = Str("noQuests");
	txNoRumors = Str("noRumors");
	txNoNotes = Str("noNotes");
	txAddNote = Str("addNote");
	txAddTime = Str("addTime");

	font_height = GUI.default_font->height;
}

//=================================================================================================
void Journal::Draw(ControlDrawData* /*cdd*/)
{
	RECT r = {global_pos.x, global_pos.y, global_pos.x+size.x, global_pos.y+size.y};
	GUI.DrawSpriteRect(tBook, r);
	GUI.DrawSprite(tPage[mode], global_pos - INT2(64,0));

	int x1 = page*2, x2 = page*2 + 1;

	// wypisz teksty na odpowiednich stronach
	// mo¿na by to zoptymalizowaæ ¿e najpierw szuka do, póŸnij a¿ do koñca
	for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
	{
		if(it->x == x1 || it->x == x2)
		{
			RECT r;
			if(it->x == x1)
				r = rect;
			else
				r = rect2;
			r.top += it->y * font_height;

			const DWORD color[3] = {BLACK, RED, GREEN};

			GUI.DrawText(GUI.default_font, it->text, 0, color[it->color], r);
		}
	}

	// numery stron
	int pages = texts.back().x+1;
	GUI.DrawText(GUI.default_font, Format("%d/%d", x1+1, pages), DT_BOTTOM|DT_CENTER, BLACK, rect);
	if(x2 != pages)
		GUI.DrawText(GUI.default_font, Format("%d/%d", x2+1, pages), DT_BOTTOM|DT_CENTER, BLACK, rect2);

	// strza³ki 32, 243
	if(page != 0)
		GUI.DrawSprite(tArrowL, INT2(rect.left-8, rect.bottom-8));
	if(texts.back().x > x2)
		GUI.DrawSprite(tArrowR, INT2(rect2.right+8, rect.bottom-8));
}

//=================================================================================================
void Journal::Update(float dt)
{
	if(!focus)
		return;

	Mode new_mode = Invalid;

	if(Key.Focus())
	{
		// zmiana trybu
		if(Key.PressedRelease('1'))
			new_mode = Quests;
		else if(Key.PressedRelease('2'))
			new_mode = Rumors;
		else if(Key.PressedRelease('3'))
			new_mode = Notes;
		// zmiana strony
		if(page != 0 && GKey.PressedRelease(GK_MOVE_LEFT))
			--page;
		if(texts.back().x > page*2 + 1 && GKey.PressedRelease(GK_MOVE_RIGHT))
			++page;
		// do góry (jeœli jest w queœcie to wychodzi do listy)
		if(mode == Quests && details && GKey.PressedRelease(GK_MOVE_FORWARD))
		{
			details = false;
			page = prev_page;
			Build();
		}
		// zmiana wybranego zadania
		if(mode == Quests && !game.quests.empty())
		{
			byte key;
			if((key = GKey.PressedR(GK_ROTATE_LEFT)) != VK_NONE)
			{
				if(!details)
				{
					// otwórz ostatni quest na tej stronie
					open_quest = max((page+1)*rect_lines*2-1, int(game.quests.size())-1);
					prev_page = page;
					page = 0;
					Build();
				}
				else
				{
					// poprzedni quest
					--open_quest;
					if(open_quest == -1)
						open_quest = game.quests.size()-1;
					page = 0;
					Build();
				}
			}
			if((key = GKey.PressedR(GK_ROTATE_RIGHT)) != VK_NONE)
			{
				if(!details)
				{
					// pierwszy quest na stronie
					open_quest = page*rect_lines*2;
					prev_page = page;
					page = 0;
					Build();
				}
				else
				{
					// nastêpny
					++open_quest;
					if(open_quest == (int)game.quests.size())
						open_quest = 0;
					page = 0;
					Build();
				}
			}
		}

		if(new_mode != Invalid)
		{
			if(new_mode == mode)
			{
				if(mode == Quests && details)
				{
					// otwórz star¹ stronê
					details = false;
					page = prev_page;
					Build();
				}
				else if(page != 0)
				{
					page = 0;
					Build();
				}
			}
			else
			{
				mode = new_mode;
				details = false;
				page = 0;
				Build();
			}
			new_mode = Invalid;
		}
	}

	// wybór zak³adki z lewej
	if(PointInRect(GUI.cursor_pos, global_pos.x-64+28, global_pos.y+32, global_pos.x-64+82, global_pos.y+119))
		new_mode = Quests;
	else if(PointInRect(GUI.cursor_pos, global_pos.x-64+28, global_pos.y+122, global_pos.x-64+82, global_pos.y+209))
		new_mode = Rumors;
	else if(PointInRect(GUI.cursor_pos, global_pos.x-64+28, global_pos.y+212, global_pos.x-64+82, global_pos.y+299))
		new_mode = Notes;

	if(new_mode != Invalid)
	{
		GUI.cursor_mode = CURSOR_HAND;
		if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
		{
			// zmieñ zak³adkê
			if(new_mode == mode)
			{
				if(mode == Quests && details)
				{
					// otwórz star¹ stronê
					details = false;
					page = prev_page;
					Build();
				}
				else if(page != 0)
				{
					page = 0;
					Build();
				}
			}
			else
			{
				mode = new_mode;
				details = false;
				page = 0;
				Build();
			}
		}
	}
	else if(mode == Quests)
	{
		if(!game.quests.empty() && !details)
		{
			// wybór questa
			int co = -1;
			if(PointInRect(GUI.cursor_pos, rect))
				co = (GUI.cursor_pos.y - rect.top)/font_height;
			else if(PointInRect(GUI.cursor_pos, rect2))
				co = (GUI.cursor_pos.y - rect.top)/font_height +rect_lines;

			if(co != -1)
			{
				co += page*rect_lines*2;
				if(co < int(game.quests.size()))
				{
					GUI.cursor_mode = CURSOR_HAND;
					if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
					{
						details = true;
						prev_page = page;
						page = 0;
						open_quest = co;
						Build();
					}
				}
			}
		}
	}
	else if(mode == Notes)
	{
		Text& last_text = texts.back();
		if(last_text.x == page*2 || last_text.y == page*2+1)
		{
			bool ok = false;
			if(last_text.x % 2 == 0)
			{
				if(GUI.cursor_pos.x >= rect.left && GUI.cursor_pos.x <= rect.right)
					ok = true;
			}
			else
			{
				if(GUI.cursor_pos.x >= rect2.left && GUI.cursor_pos.x <= rect2.right)
					ok = true;
			}

			if(ok && GUI.cursor_pos.y >= rect.top+last_text.y*font_height && GUI.cursor_pos.y <= rect.top+(last_text.y+1)*font_height)
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
				{
					// dodaj notatkê
					cstring names[] = { nullptr, txAdd };
					input.clear();
					GetTextDialogParams params(txNoteText, input);
					params.custom_names = names;
					params.event = delegate<void(int)>(this, &Journal::OnAddNote);
					params.limit = 255;
					params.lines = 8;
					params.multiline = true;
					params.parent = this;
					params.width = 400;
					GetTextDialog::Show(params);
				}
			}
		}
	}

	if(new_mode == Invalid)
	{
		if(page != 0)
		{
			if(PointInRect(GUI.cursor_pos, INT2(rect.left-8,rect.bottom-8), INT2(16,16)))
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
					--page;
			}
		}
		if(texts.back().x > page*2 + 1)
		{
			if(PointInRect(GUI.cursor_pos, INT2(rect2.right+8,rect.bottom-8), INT2(16,16)))
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
					++page;
			}
		}
	}

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void Journal::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize || e == GuiEvent_Resize || e == GuiEvent_Moved)
	{
		rect.left = 32;
		rect.right = 32+206;
		rect.top = 16;
		rect.bottom = 16+416;

		rect2.left = 259;
		rect2.right = 471-6;
		rect2.top = 16;
		rect2.bottom = 431;

		float sx = float(size.x)/512,
			sy = float(size.y)/512;

		rect.left = global_pos.x + int(sx * rect.left);
		rect.right = global_pos.x + int(sx * rect.right);
		rect.top = global_pos.y + int(sy * rect.top);
		rect.bottom = global_pos.y + int(sy * rect.bottom);

		rect2.left = global_pos.x + int(sx * rect2.left);
		rect2.right = global_pos.x + int(sx * rect2.right);
		rect2.top = global_pos.y + int(sy * rect2.top);
		rect2.bottom = global_pos.y + int(sy * rect2.bottom);

		rect_w = abs(rect.right - rect.left);
		rect_lines = abs(rect.bottom - rect.top)/font_height;

		if(e == GuiEvent_Resize)
			Build();
	}
}

//=================================================================================================
void Journal::Reset()
{
	mode = Quests;
	details = false;
	page = 0;
}

//=================================================================================================
void Journal::Show()
{
	visible = true;
	Build();
	Event(GuiEvent_Show);
}

//=================================================================================================
void Journal::Hide()
{
	visible = false;
}

//=================================================================================================
void Journal::Build()
{
	texts.clear();
	x = 0;
	y = 0;

	if(mode == Quests)
	{
		// zadania
		if(!details)
		{
			// lista zadañ
			if(game.quests.empty())
				AddEntry(txNoQuests, 0, true);
			else
			{
				for(vector<Quest*>::iterator it = game.quests.begin(), end = game.quests.end(); it != end; ++it)
				{
					int color = 0;
					if((*it)->state == Quest::Failed)
						color = 1;
					else if((*it)->state == Quest::Completed)
						color = 2;
					AddEntry((*it)->name.c_str(), color, true);
				}
			}
		}
		else
		{
			// szczegó³y pojedyñczego zadania
			Quest* quest = game.quests[open_quest];
			for(vector<string>::iterator it = quest->msgs.begin(), end = quest->msgs.end(); it != end; ++it)
				AddEntry(it->c_str(), 0, false);
		}
	}
	else if(mode == Rumors)
	{
		// rumors
		if(game.rumors.empty())
			AddEntry(txNoRumors, 0, false);
		else
		{
			for(vector<string>::iterator it = game.rumors.begin(), end = game.rumors.end(); it != end; ++it)
				AddEntry(it->c_str(), 0, false);
		}
	}
	else
	{
		// notatki
		if(game.notes.empty())
			AddEntry(txNoNotes, 0, false);
		else
		{
			for(vector<string>::iterator it = game.notes.begin(), end = game.notes.end(); it != end; ++it)
				AddEntry(it->c_str(), 0, false);
		}

		AddEntry(txAddNote, 0, false);
	}
}

//=================================================================================================
void Journal::AddEntry(cstring text, int color, bool singleline)
{
	if(singleline)
	{
		Text& t = Add1(texts);
		t.text = text;
		t.color = color;
		t.x = x;
		t.y = y;
		++y;
		if(y == rect_lines)
		{
			y = 0;
			++x;
		}
		return;
	}

	// ile linijek zajmuje tekst?
	INT2 osize = GUI.default_font->CalculateSize(text, rect_w);
	int h = osize.y/font_height+1;

	if(y + h >= rect_lines)
	{
		if(y == 0)
		{
			// ten tekst zajmuje ponad jedn¹ stronê
			assert(0);
			Text& t = Add1(texts);
			t.text = text;
			t.color = color;
			t.x = x;
			t.y = y;
			++x;
		}
		else
		{
			++x;
			y = 0;
			Text& t = Add1(texts);
			t.text = text;
			t.color = color;
			t.x = x;
			t.y = y;
			y += h;
			if(y == rect_lines)
			{
				y = 0;
				++x;
			}
		}
	}
	else
	{
		Text& t = Add1(texts);
		t.text = text;
		t.color = color;
		t.x = x;
		t.y = y;
		y += h;
		if(y == rect_lines)
		{
			y = 0;
			++x;
		}
	}
}

//=================================================================================================
void Journal::OnAddNote(int id)
{
	if(id == BUTTON_OK)
	{
		game.notes.push_back(Format(txAddTime, game.day+1, game.month+1, game.year, input.c_str()));
		Build();
		if(!game.IsLocal())
			game.PushNetChange(NetChange::ADD_NOTE);
	}
}

//=================================================================================================
void Journal::NeedUpdate(Mode at_mode, int quest_id)
{
	if(mode == at_mode)
	{
		if(mode == Quests && details)
		{
			if(quest_id == open_quest)
				Build();
		}
		else
			Build();
	}
}
