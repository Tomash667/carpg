#include "Pch.h"
#include "Journal.h"

#include "Game.h"
#include "GameGui.h"
#include "GameKeys.h"
#include "GameMessages.h"
#include "Language.h"
#include "Net.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

#include <GetTextDialog.h>
#include <Input.h>
#include <ResourceManager.h>
#include <SoundManager.h>

const Int2 ButtonShift(-90, 32);
const Int2 ButtonSize(115, 88);
const Int2 IconSize(48, 48);
const Int2 IconShift(-90 + 16, 32);
const int ButtonDist = 90;

//=================================================================================================
Journal::Journal() : mode(Quests)
{
	visible = false;
	Reset();

	font_height = GameGui::font->height;
}

//=================================================================================================
Journal::~Journal()
{
	for(Text& text : texts)
	{
		if(text.pooled)
			StringPool.SafeFree(text.pooled);
	}
}

//=================================================================================================
void Journal::LoadLanguage()
{
	Language::Section s = Language::GetSection("Journal");
	txAdd = s.Get("add");
	txNoteText = s.Get("noteText");
	txNoQuests = s.Get("noQuests");
	txNoRumors = s.Get("noRumors");
	txNoNotes = s.Get("noNotes");
	txAddNote = s.Get("addNote");
	txAddTime = s.Get("addTime");
	txGoldMonth = s.Get("goldMonth");
	txTotal = s.Get("total");
	txNoInvestments = s.Get("noInvestments");
}

//=================================================================================================
void Journal::LoadData()
{
	tBook = res_mgr->Load<Texture>("book.png");
	tButtonOn = res_mgr->Load<Texture>("journal_bt_on.png");
	tButtonOff = res_mgr->Load<Texture>("journal_bt_off.png");
	tArrowL = res_mgr->Load<Texture>("page_prev.png");
	tArrowR = res_mgr->Load<Texture>("page_next.png");
	tIcons[0] = res_mgr->Load<Texture>("journal_quests.png");
	tIcons[1] = res_mgr->Load<Texture>("journal_rumors.png");
	tIcons[2] = res_mgr->Load<Texture>("journal_notes.png");
	tIcons[3] = res_mgr->Load<Texture>("journal_investments.png");
}

//=================================================================================================
void Journal::Draw(ControlDrawData*)
{
	// background
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawSpriteRect(tBook, r);

	// buttons
	for(int i = 0; i < Max; ++i)
	{
		gui->DrawSprite(mode == i ? tButtonOn : tButtonOff, global_pos + Int2(ButtonShift.x, ButtonShift.y + ButtonDist * i));
		Int2 pos = global_pos + Int2(IconShift.x + (ButtonSize.x - IconSize.x) / 2, IconShift.y + ButtonDist * i + (ButtonSize.y - IconSize.y) / 2);
		if(mode == i)
			pos.x += 5;
		gui->DrawSprite(tIcons[i], pos);
	}

	// page text
	int x1 = page * 2, x2 = page * 2 + 1;
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

			const Color color[3] = { Color::Black, Color::Red, Color::Green };

			gui->DrawText(GameGui::font, it->text, 0, color[it->color], r);
		}
	}

	// page numbers
	int pages = texts.back().x + 1;
	gui->DrawText(GameGui::font, Format("%d/%d", x1 + 1, pages), DTF_BOTTOM | DTF_CENTER, Color::Black, rect);
	if(x2 != pages)
		gui->DrawText(GameGui::font, Format("%d/%d", x2 + 1, pages), DTF_BOTTOM | DTF_CENTER, Color::Black, rect2);

	// prev/next page arrows
	if(page != 0)
		gui->DrawSprite(tArrowL, Int2(rect.Left() + 5, rect.Bottom() - 16));
	if(texts.back().x > x2)
		gui->DrawSprite(tArrowR, Int2(rect2.Right() - 21, rect.Bottom() - 16));
}

//=================================================================================================
void Journal::Update(float dt)
{
	for(pair<Mode, int>& change : changes)
	{
		if(mode == change.first)
		{
			if(mode == Quests && details)
			{
				if(change.second == open_quest)
					Build();
			}
			else
				Build();
		}
	}

	if(!focus || !input->Focus())
		return;

	Mode new_mode = Invalid;

	// ----- handle keyboard
	// change mode
	for(int i = 0; i < Max; ++i)
	{
		if(input->PressedRelease(Key::N1 + i))
			new_mode = (Mode)i;
	}

	// change page
	if(page != 0 && GKey.PressedRelease(GK_MOVE_LEFT))
		--page;
	if(texts.back().x > page * 2 + 1 && GKey.PressedRelease(GK_MOVE_RIGHT))
		++page;

	// return to quests list
	if(mode == Quests && details && GKey.PressedRelease(GK_MOVE_FORWARD))
	{
		details = false;
		page = prev_page;
		Build();
	}

	// change open quest
	if(mode == Quests && !quest_mgr->quests.empty())
	{
		if(GKey.PressedR(GK_ROTATE_LEFT) != Key::None)
		{
			if(!details)
			{
				// last quest on this page
				open_quest = max((page + 1) * rect_lines * 2 - 1, int(quest_mgr->quests.size()) - 1);
				prev_page = page;
				page = 0;
				Build();
			}
			else
			{
				// prev quest
				--open_quest;
				if(open_quest == -1)
					open_quest = quest_mgr->quests.size() - 1;
				page = 0;
				Build();
			}
		}
		if(GKey.PressedR(GK_ROTATE_RIGHT) != Key::None)
		{
			if(!details)
			{
				// first quest on this page
				open_quest = page * rect_lines * 2;
				prev_page = page;
				page = 0;
				Build();
			}
			else
			{
				// next quest
				++open_quest;
				if(open_quest == (int)quest_mgr->quests.size())
					open_quest = 0;
				page = 0;
				Build();
			}
		}
	}

	// change mode with mouse
	for(int i = 0; i < 4; ++i)
	{
		const Int2 pos(global_pos.x + ButtonShift.x, global_pos.y + ButtonShift.y + ButtonDist * i);
		if(PointInRect(gui->cursor_pos, pos, ButtonSize))
		{
			gui->cursor_mode = CURSOR_HOVER;
			if(input->PressedRelease(Key::LeftButton))
				new_mode = (Mode)i;
		}
	}

	if(new_mode != Invalid)
	{
		// change mode
		if(new_mode == mode)
		{
			if(mode == Quests && details)
			{
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
	else if(mode == Quests)
	{
		if(!quest_mgr->quests.empty() && !details)
		{
			// select quest
			int x, y = (gui->cursor_pos.y - rect.Top()) / font_height;
			if(rect.IsInside(gui->cursor_pos))
				x = page * 2;
			else if(rect2.IsInside(gui->cursor_pos))
				x = page * 2 + 1;
			else
				x = -1;

			if(x != -1)
			{
				int index = 0;
				bool ok = false;
				for(Text& text : texts)
				{
					if(x == text.x && y >= text.y && y < text.y + text.h)
					{
						ok = true;
						break;
					}
					++index;
				}

				if(ok)
				{
					gui->cursor_mode = CURSOR_HOVER;
					if(input->Focus() && input->PressedRelease(Key::LeftButton))
					{
						details = true;
						prev_page = page;
						page = 0;
						open_quest = index;
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
				if(gui->cursor_pos.x >= rect.Left() && gui->cursor_pos.x <= rect.Right())
					ok = true;
			}
			else
			{
				if(gui->cursor_pos.x >= rect2.Left() && gui->cursor_pos.x <= rect2.Right())
					ok = true;
			}

			if(ok && gui->cursor_pos.y >= rect.Top() + last_text.y * font_height && gui->cursor_pos.y <= rect.Top() + (last_text.y + 1) * font_height)
			{
				gui->cursor_mode = CURSOR_HOVER;
				if(input->Focus() && input->PressedRelease(Key::LeftButton))
				{
					// add note
					cstring names[] = { nullptr, txAdd };
					input_str.clear();
					GetTextDialogParams params(txNoteText, input_str);
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
			if(PointInRect(gui->cursor_pos, rect.LeftBottom() + Int2(5, -16), Int2(16, 16)))
			{
				gui->cursor_mode = CURSOR_HOVER;
				if(input->Focus() && input->PressedRelease(Key::LeftButton))
					--page;
			}
		}
		if(texts.back().x > page * 2 + 1)
		{
			if(PointInRect(gui->cursor_pos, rect2.RightBottom() + Int2(-21, -16), Int2(16, 16)))
			{
				gui->cursor_mode = CURSOR_HOVER;
				if(input->Focus() && input->PressedRelease(Key::LeftButton))
					++page;
			}
		}
	}

	if(input->Focus() && input->PressedRelease(Key::Escape))
		Hide();
}

//=================================================================================================
void Journal::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize || e == GuiEvent_Resize || e == GuiEvent_Moved)
	{
		rect = Rect(32, 16, 238, 432);
		rect2 = Rect(270, 16, 476, 432);

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
	const bool devmode = game->devmode;

	for(Text& text : texts)
	{
		if(text.pooled)
			StringPool.Free(text.pooled);
	}
	texts.clear();
	x = 0;
	y = 0;

	switch(mode)
	{
	case Quests:
		if(!details)
		{
			// list of quests
			if(quest_mgr->quests.empty())
				AddEntry(txNoQuests);
			else
			{
				for(Quest* quest : quest_mgr->quests)
				{
					int color = 0;
					if(quest->state == Quest::Failed)
						color = 1;
					else if(quest->state == Quest::Completed)
						color = 2;

					if(devmode)
						AddEntry(Format("%s (%p)", quest->name.c_str(), quest), color, false, true);
					else
						AddEntry(quest->name.c_str(), color, false);
				}
			}
		}
		else
		{
			// details of single quest
			Quest* quest = quest_mgr->quests[open_quest];
			for(vector<string>::iterator it = quest->msgs.begin(), end = quest->msgs.end(); it != end; ++it)
				AddEntry(it->c_str());
		}
		break;

	case Rumors:
		if(rumors.empty())
			AddEntry(txNoRumors);
		else
		{
			for(vector<string>::iterator it = rumors.begin(), end = rumors.end(); it != end; ++it)
				AddEntry(it->c_str());
		}
		break;

	case Notes:
		if(notes.empty())
			AddEntry(txNoNotes);
		else
		{
			for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
				AddEntry(it->c_str());
		}

		AddEntry(txAddNote);
		break;

	case Investments:
		{
			const vector<Team::Investment>& investments = team->GetInvestments();
			if(investments.empty())
				AddEntry(txNoInvestments, 0, false);
			else
			{
				int total = 0;
				for(const Team::Investment& investment : investments)
				{
					AddEntry(Format("%s: %d %s", investment.name.c_str(), investment.gold, txGoldMonth), 0, false, true);
					total += investment.gold;
				}
				AddEntry("--------------------", 0, false);
				AddEntry(Format("%s: %d %s", txTotal, total, txGoldMonth), 0, false, true);
			}
		}
		break;
	}
}

//=================================================================================================
void Journal::AddEntry(cstring text, int color, bool spacing, bool pooled)
{
	Text& t = Add1(texts);
	t.color = color;
	if(pooled)
	{
		string* str = StringPool.Get();
		*str = text;
		t.pooled = str;
		t.text = str->c_str();
	}
	else
	{
		t.pooled = nullptr;
		t.text = text;
	}

	// how many lines this text takes?
	const Int2 size = GameGui::font->CalculateSize(text, rect_w);
	t.h = size.y / font_height;
	if(spacing)
		++t.h;

	if(y + t.h >= rect_lines)
	{
		if(y == 0)
		{
			// this text takes more then single page...
			assert(0);
			t.x = x;
			t.y = y;
			++x;
		}
		else
		{
			++x;
			y = 0;
			t.x = x;
			t.y = y;
			y += t.h;
			if(y == rect_lines)
			{
				y = 0;
				++x;
			}
		}
	}
	else
	{
		t.x = x;
		t.y = y;
		y += t.h;
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
		notes.push_back(Format(txAddTime, world->GetDate(), input_str.c_str()));
		sound_mgr->PlaySound2d(game_gui->messages->snd_scribble);
		Build();
		if(Net::IsClient())
			Net::PushChange(NetChange::ADD_NOTE);
	}
}

//=================================================================================================
void Journal::NeedUpdate(Mode at_mode, int quest_id)
{
	changes.push_back(std::make_pair(at_mode, quest_id));
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

	rumors.push_back(Format(txAddTime, world->GetDate(), text));
	NeedUpdate(Journal::Rumors);
	game_gui->messages->AddGameMsg3(GMS_ADDED_RUMOR);
}

//=================================================================================================
void Journal::Save(GameWriter& f)
{
	f.WriteStringArray<uint, word>(rumors);
	f.WriteStringArray<uint, word>(notes);
}

//=================================================================================================
void Journal::Load(GameReader& f)
{
	f.ReadStringArray<uint, word>(rumors);
	f.ReadStringArray<uint, word>(notes);
}
