#include "Pch.h"
#include "Core.h"
#include "Journal.h"
#include "KeyStates.h"
#include "Game.h"
#include "GetTextDialog.h"
#include "Language.h"
#include "GameKeys.h"
#include "QuestManager.h"
#include "Quest.h"
#include "ResourceManager.h"

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
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawSpriteRect(tBook, r);
	GUI.DrawSprite(tPage[mode], global_pos - Int2(64, 0));

	int x1 = page * 2, x2 = page * 2 + 1;

	// wypisz teksty na odpowiednich stronach
	// mo�na by to zoptymalizowa� �e najpierw szuka do, p�nij a� do ko�ca
	for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
	{
		if(it->x == x1 || it->x == x2)
		{
			Rect r;
			if(it->x == x1)
				r = rect;
			else
				r = rect2;
			r.Top() += it->y * font_height;

			const DWORD color[3] = { BLACK, RED, GREEN };

			GUI.DrawText(GUI.default_font, it->text, 0, color[it->color], r);
		}
	}

	// numery stron
	int pages = texts.back().x + 1;
	GUI.DrawText(GUI.default_font, Format("%d/%d", x1 + 1, pages), DT_BOTTOM | DT_CENTER, BLACK, rect);
	if(x2 != pages)
		GUI.DrawText(GUI.default_font, Format("%d/%d", x2 + 1, pages), DT_BOTTOM | DT_CENTER, BLACK, rect2);

	// strza�ki 32, 243
	if(page != 0)
		GUI.DrawSprite(tArrowL, Int2(rect.Left() - 8, rect.Bottom() - 8));
	if(texts.back().x > x2)
		GUI.DrawSprite(tArrowR, Int2(rect2.Right() + 8, rect.Bottom() - 8));
}

//=================================================================================================
void Journal::Update(float dt)
{
	if(!focus)
		return;

	Mode new_mode = Invalid;
	QuestManager& quest_manager = QuestManager::Get();

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
		if(texts.back().x > page * 2 + 1 && GKey.PressedRelease(GK_MOVE_RIGHT))
			++page;
		// do g�ry (je�li jest w que�cie to wychodzi do listy)
		if(mode == Quests && details && GKey.PressedRelease(GK_MOVE_FORWARD))
		{
			details = false;
			page = prev_page;
			Build();
		}
		// zmiana wybranego zadania
		if(mode == Quests && !quest_manager.quests.empty())
		{
			byte key;
			if((key = GKey.PressedR(GK_ROTATE_LEFT)) != VK_NONE)
			{
				if(!details)
				{
					// otw�rz ostatni quest na tej stronie
					open_quest = max((page + 1)*rect_lines * 2 - 1, int(quest_manager.quests.size()) - 1);
					prev_page = page;
					page = 0;
					Build();
				}
				else
				{
					// poprzedni quest
					--open_quest;
					if(open_quest == -1)
						open_quest = quest_manager.quests.size() - 1;
					page = 0;
					Build();
				}
			}
			if((key = GKey.PressedR(GK_ROTATE_RIGHT)) != VK_NONE)
			{
				if(!details)
				{
					// pierwszy quest na stronie
					open_quest = page*rect_lines * 2;
					prev_page = page;
					page = 0;
					Build();
				}
				else
				{
					// nast�pny
					++open_quest;
					if(open_quest == (int)quest_manager.quests.size())
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
					// otw�rz star� stron�
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

	// wyb�r zak�adki z lewej
	if(PointInRect(GUI.cursor_pos, global_pos.x - 64 + 28, global_pos.y + 32, global_pos.x - 64 + 82, global_pos.y + 119))
		new_mode = Quests;
	else if(PointInRect(GUI.cursor_pos, global_pos.x - 64 + 28, global_pos.y + 122, global_pos.x - 64 + 82, global_pos.y + 209))
		new_mode = Rumors;
	else if(PointInRect(GUI.cursor_pos, global_pos.x - 64 + 28, global_pos.y + 212, global_pos.x - 64 + 82, global_pos.y + 299))
		new_mode = Notes;

	if(new_mode != Invalid)
	{
		GUI.cursor_mode = CURSOR_HAND;
		if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
		{
			// zmie� zak�adk�
			if(new_mode == mode)
			{
				if(mode == Quests && details)
				{
					// otw�rz star� stron�
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
		if(!quest_manager.quests.empty() && !details)
		{
			// wyb�r questa
			int co = -1;
			if(rect.IsInside(GUI.cursor_pos))
				co = (GUI.cursor_pos.y - rect.Top()) / font_height;
			else if(rect2.IsInside(GUI.cursor_pos))
				co = (GUI.cursor_pos.y - rect.Top()) / font_height + rect_lines;

			if(co != -1)
			{
				co += page*rect_lines * 2;
				if(co < int(quest_manager.quests.size()))
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
		if(last_text.x == page * 2 || last_text.y == page * 2 + 1)
		{
			bool ok = false;
			if(last_text.x % 2 == 0)
			{
				if(GUI.cursor_pos.x >= rect.Left() && GUI.cursor_pos.x <= rect.Right())
					ok = true;
			}
			else
			{
				if(GUI.cursor_pos.x >= rect2.Left() && GUI.cursor_pos.x <= rect2.Right())
					ok = true;
			}

			if(ok && GUI.cursor_pos.y >= rect.Top() + last_text.y*font_height && GUI.cursor_pos.y <= rect.Top() + (last_text.y + 1)*font_height)
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
				{
					// dodaj notatk�
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
			if(PointInRect(GUI.cursor_pos, rect.LeftBottom() - Int2(8, 8), Int2(16, 16)))
			{
				GUI.cursor_mode = CURSOR_HAND;
				if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
					--page;
			}
		}
		if(texts.back().x > page * 2 + 1)
		{
			if(PointInRect(GUI.cursor_pos, rect.RightBottom() + Int2(8, -8), Int2(16, 16)))
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
		rect = Rect(32, 16, 238, 432);
		rect2 = Rect(259, 16, 455, 432);

		Vec2 scale = Vec2(size) / 512;
		rect = rect * scale + global_pos;
		rect2 = rect2 * scale + global_pos;

		rect_w = rect.SizeX();
		rect_lines = rect.SizeY() / font_height;

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
	notes.clear();
	rumors.clear();
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
		// quests
		QuestManager& quest_manager = QuestManager::Get();
		if(!details)
		{
			// list of quests
			if(quest_manager.quests.empty())
				AddEntry(txNoQuests, 0, true);
			else
			{
				for(vector<Quest*>::iterator it = quest_manager.quests.begin(), end = quest_manager.quests.end(); it != end; ++it)
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
			// details of single quest
			Quest* quest = quest_manager.quests[open_quest];
			for(vector<string>::iterator it = quest->msgs.begin(), end = quest->msgs.end(); it != end; ++it)
				AddEntry(it->c_str(), 0, false);
		}
	}
	else if(mode == Rumors)
	{
		// rumors
		if(rumors.empty())
			AddEntry(txNoRumors, 0, false);
		else
		{
			for(vector<string>::iterator it = rumors.begin(), end = rumors.end(); it != end; ++it)
				AddEntry(it->c_str(), 0, false);
		}
	}
	else
	{
		// notes
		if(notes.empty())
			AddEntry(txNoNotes, 0, false);
		else
		{
			for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
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
	Int2 osize = GUI.default_font->CalculateSize(text, rect_w);
	int h = osize.y / font_height + 1;

	if(y + h >= rect_lines)
	{
		if(y == 0)
		{
			// ten tekst zajmuje ponad jedn� stron�
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
		notes.push_back(Format(txAddTime, game.day + 1, game.month + 1, game.year, input.c_str()));
		Build();
		if(!Net::Net::IsLocal())
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

//=================================================================================================
void Journal::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("book.png", tBook);
	tex_mgr.AddLoadTask("dziennik_przyciski.png", tPage[0]);
	tex_mgr.AddLoadTask("dziennik_przyciski2.png", tPage[1]);
	tex_mgr.AddLoadTask("dziennik_przyciski3.png", tPage[2]);
	tex_mgr.AddLoadTask("strzalka_l.png", tArrowL);
	tex_mgr.AddLoadTask("strzalka_p.png", tArrowR);
}

//=================================================================================================
void Journal::AddRumor(cstring text)
{
	assert(text);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ADD_RUMOR;
		c.id = rumors.size();
	}

	rumors.push_back(Format(txAddTime, game.day + 1, game.month + 1, game.year, text));
	NeedUpdate(Journal::Rumors);
	game.AddGameMsg3(GMS_ADDED_RUMOR);
}

//=================================================================================================
void Journal::Save(HANDLE file)
{
	uint count = rumors.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<string>::iterator it = rumors.begin(), end = rumors.end(); it != end; ++it)
	{
		word len = (word)it->length();
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		WriteFile(file, it->c_str(), len, &tmp, nullptr);
	}

	count = notes.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
	{
		word len = (word)it->length();
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		WriteFile(file, it->c_str(), len, &tmp, nullptr);
	}
}

//=================================================================================================
void Journal::Load(HANDLE file)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	rumors.resize(count);
	for(vector<string>::iterator it = rumors.begin(), end = rumors.end(); it != end; ++it)
	{
		word len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		it->resize(len);
		ReadFile(file, (void*)it->c_str(), len, &tmp, nullptr);
	}

	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	notes.resize(count);
	for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
	{
		word len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		it->resize(len);
		ReadFile(file, (void*)it->c_str(), len, &tmp, nullptr);
	}
}
