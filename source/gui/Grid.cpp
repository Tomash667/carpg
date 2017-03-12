#include "Pch.h"
#include "Base.h"
#include "Grid.h"
#include "KeyStates.h"

//=================================================================================================
Grid::Grid() : items(0), height(20), selected(-1), selection_type(COLOR), selection_color(WHITE), single_line(false), select_event(nullptr)
{

}

//=================================================================================================
void Grid::Draw(ControlDrawData*)
{
	int x = global_pos.x, y = global_pos.y;
	RECT r = {0,y,0,y+height};

	// box i nag³ówki
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
	{
		// box nag³ówka
		GUI.DrawItem(GUI.tBox, INT2(x,y), INT2(it->width,height), BLACK, 8, 32);
		// box zawartoœci
		GUI.DrawItem(GUI.tBox, INT2(x,y+height), INT2(it->width,size.y-height), BLACK, 8, 32);
		// tekst nag³ówka
		if(!it->title.empty())
		{
			r.left = x;
			r.right = x+it->width;
			GUI.DrawText(GUI.default_font, it->title, DT_CENTER|DT_VCENTER, BLACK, r, &r);
		}
		x += it->width;
	}

	// zawartoœæ
	Cell cell;
	int n, clip_state;
	y += height-int(scroll.offset);
	const int clip_y[4] = {global_pos.y, global_pos.y+height, global_pos.y+size.y-height, global_pos.y+size.y};
	RECT clip_r;

	DWORD text_flags = DT_CENTER|DT_VCENTER;
	if(single_line)
		text_flags |= DT_SINGLELINE;

	for(int i=0; i<items; ++i)
	{
		n = 0;
		x = global_pos.x;

		// ustal przycinanie komórek
		if(y < clip_y[0])
		{
			y += height;
			continue;
		}
		else if(y < clip_y[1])
			clip_state = 1;
		else if(y > clip_y[3])
			break;
		else if(y > clip_y[2])
			clip_state = 2;
		else
			clip_state = 0;

		// zaznaczenie t³a
		if(i == selected && selection_type == BACKGROUND)
		{
			RECT r2 = {x, y, x+total_width, y+height};
			if(clip_state == 1)
				r2.top = global_pos.y+height;
			else if(clip_state == 2)
				r2.bottom = global_pos.y+size.y;
			if(r2.top < r2.bottom)
				GUI.DrawSpriteRect(GUI.tPix, r2, selection_color);
		}

		for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it, ++n)
		{
			if(it->type == TEXT || it->type == TEXT_COLOR)
			{
				DWORD color;
				cstring text;

				if(it->type == TEXT)
				{
					color = BLACK;
					event(i, n, cell);
					text = cell.text;
				}
				else
				{
					TextColor tc;
					cell.text_color = &tc;
					event(i, n, cell);
					text = tc.text;
					color = tc.color;
				}

				if(selection_type == COLOR && i == selected)
					color = selection_color;

				r.left = x;
				r.right = x + it->width;
				r.top = y;
				r.bottom = y+height;

				if(clip_state == 0)
					GUI.DrawText(GUI.default_font, text, text_flags, color, r, &r);
				else
				{
					clip_r.left = r.left;
					clip_r.right = r.right;
					if(clip_state == 1)
					{
						clip_r.top = global_pos.y+height;
						clip_r.bottom = r.bottom;
					}
					else
					{
						clip_r.top = r.top;
						clip_r.bottom = global_pos.y+size.y;
					}
					GUI.DrawText(GUI.default_font, text, text_flags, color, r, &clip_r);
				}
			}
			else if(it->type == IMG)
			{
				event(i, n, cell);
				RECT* clipping = nullptr;
				if(clip_state != 0)
				{
					clipping = &clip_r;
					clip_r.left = 0;
					clip_r.right = GUI.wnd_size.x;
					if(clip_state == 1)
					{
						clip_r.top = global_pos.y+height;
						clip_r.bottom = GUI.wnd_size.y;
					}
					else
					{
						clip_r.top = 0;
						clip_r.bottom = global_pos.y+size.y;
					}
				}

				GUI.DrawSprite(cell.img, INT2(x+(it->width-16)/2, y+(height-16)/2), WHITE, clipping);
			}
			else //if(it->type == IMGSET)
			{
				cell.imgset = &imgset;
				imgset.clear();
				event(i, n, cell);
				if(!imgset.empty())
				{
					int img_total_width = 16*imgset.size();
					int y2 = y+(height-16)/2;
					int dist, startx;
					if(img_total_width > it->width && imgset.size() > 1)
					{
						dist = 16-(img_total_width-it->width)/(imgset.size()-1);
						startx = 0;
					}
					else
					{
						dist = 16;
						startx = (it->width - img_total_width)/2;
					}

					RECT* clipping = nullptr;
					if(clip_state != 0)
					{
						clipping = &clip_r;
						clip_r.left = 0;
						clip_r.right = GUI.wnd_size.x;
						if(clip_state == 1)
						{
							clip_r.top = global_pos.y+height;
							clip_r.bottom = GUI.wnd_size.y;
						}
						else
						{
							clip_r.top = 0;
							clip_r.bottom = global_pos.y+size.y;
						}
					}

					int x2 = x+startx;
					for(uint j=0; j<imgset.size(); ++j)
					{
						GUI.DrawSprite(imgset[j], INT2(x2,y2), WHITE, clipping);
						x2 += dist;
					}
				}
			}
			x += it->width;
		}
		y += height;
	}

	scroll.Draw();
}

//=================================================================================================
void Grid::Update(float dt)
{
	if(Key.Focus() && focus)
	{
		if(GUI.cursor_pos.x >= global_pos.x && GUI.cursor_pos.x < global_pos.x+total_width
			&& GUI.cursor_pos.y >= global_pos.y+height && GUI.cursor_pos.y < global_pos.y+size.y)
		{
			int n = (GUI.cursor_pos.y - (global_pos.y+height) + int(scroll.offset))/height;
			if(n >= 0 && n < items)
			{
				if(selection_type != NONE)
				{
					GUI.cursor_mode = CURSOR_HAND;
					if(Key.PressedRelease(VK_LBUTTON))
						selected = n;
				}
				if(select_event)
				{
					int y = GUI.cursor_pos.x - global_pos.x, ysum=0;
					int col = -1;

					for(int i=0; i<(int)columns.size(); ++i)
					{
						ysum += columns[i].width;
						if(y <= ysum)
						{
							col = i;
							break;
						}
					}

					// celowo nie zaznacza 1 kolumny!
					if(col > 0)
					{
						GUI.cursor_mode = CURSOR_HAND;
						if(Key.PressedRelease(VK_LBUTTON))
							select_event(n, col, 0);
						else if(Key.PressedRelease(VK_RBUTTON))
							select_event(n, col, 1);
					}
				}
			}
		}

		if(IsInside(GUI.cursor_pos))
			scroll.ApplyMouseWheel();
		scroll.Update(dt);
	}
}

//=================================================================================================
void Grid::Init()
{
	scroll.pos = INT2(size.x-16,height);
	scroll.size = INT2(16,size.y-height);
	scroll.total = height*items;
	scroll.part = scroll.size.y;
	scroll.offset = 0;

	total_width = 0;
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
		total_width += it->width;
}

//=================================================================================================
void Grid::Move(INT2& _global_pos)
{
	global_pos = _global_pos + pos;
	scroll.global_pos = global_pos + scroll.pos;
}

//=================================================================================================
void Grid::AddColumn(Type type, int width, cstring title)
{
	Column& c = Add1(columns);
	c.type = type;
	c.width = width;
	if(title)
		c.title = title;
}

//=================================================================================================
void Grid::AddItem()
{
	++items;
	scroll.total = items*height;
}

//=================================================================================================
void Grid::AddItems(int count)
{
	assert(count > 0);
	items += count;
	scroll.total = items*height;
}

//=================================================================================================
void Grid::RemoveItem(int id)
{
	if(selected == id)
		selected = -1;
	else if(selected > id)
		--selected;
	--items;
	scroll.total = items*height;
	const float s = float(scroll.total-scroll.part); 
	if(scroll.offset > s)
		scroll.offset = s;
	if(scroll.part >= scroll.total)
		scroll.offset = 0;
}

//=================================================================================================
void Grid::Reset()
{
	items = 0;
	selected = -1;
	scroll.total = 0;
	scroll.part = size.y;
	scroll.offset = 0;
}
