// show box tooltip for elements under cursor
#include "Pch.h"
#include "EngineCore.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
static const int INVALID_INDEX = -1;
static const float TIMER = 0.3f;

//=================================================================================================
void TooltipController::Init(TooltipGetText _get_text)
{
	assert(_get_text);
	get_text = _get_text;
	Clear();
}

//=================================================================================================
void TooltipController::Clear()
{
	group = INVALID_INDEX;
	id = INVALID_INDEX;
	anything = false;
	state = State::NOT_VISIBLE;
	alpha = 0.f;
	timer = 0.f;
}

//=================================================================================================
void TooltipController::UpdateTooltip(float dt, int new_group, int new_id)
{
	if(new_group != INVALID_INDEX)
	{
		if(new_group != group || new_id != id)
		{
			state = State::COUNTING;
			group = new_group;
			id = new_id;
			timer = TIMER;
		}
		else
			timer -= dt;

		if(state == State::COUNTING)
		{
			if(timer <= 0.f)
			{
				state = State::VISIBLE;
				alpha = 0.f;

				FormatBox();
			}
		}
		else
		{
			alpha += dt * 5;
			if(alpha >= 1.f)
				alpha = 1.f;
		}

		if(state == State::VISIBLE)
		{
			pos = GUI.cursor_pos + Int2(24, 24);
			if(pos.x + size.x >= GUI.wnd_size.x)
				pos.x = GUI.wnd_size.x - size.x - 1;
			if(pos.y + size.y >= GUI.wnd_size.y)
				pos.y = GUI.wnd_size.y - size.y - 1;
		}
	}
	else
	{
		state = State::NOT_VISIBLE;
		group = INVALID_INDEX;
		id = INVALID_INDEX;
	}
}

//=================================================================================================
void TooltipController::Draw(ControlDrawData*)
{
	if(state != State::VISIBLE || !anything)
		return;

	int a = int(alpha * 222),
		a2 = int(alpha * 255);

	// box
	GUI.DrawItem(tDialog, pos, size, Color::Alpha(a), 12);

	// image
	if(img)
		GUI.DrawSprite(img, pos + Int2(12, 12), Color::Alpha(a2));

	Rect r;

	// big text
	if(!big_text.empty())
	{
		r = r_big_text;
		r += pos;
		GUI.DrawText(GUI.fBig, big_text, DTF_PARSE_SPECIAL, Color(0, 0, 0, a2), r);
	}

	// text
	if(!text.empty())
	{
		r = r_text;
		r += pos;
		GUI.DrawText(GUI.default_font, text, DTF_PARSE_SPECIAL, Color(0, 0, 0, a2), r);
	}

	// small text
	if(!small_text.empty())
	{
		r = r_small_text;
		r += pos;
		GUI.DrawText(GUI.fSmall, small_text, DTF_PARSE_SPECIAL, Color(0, 0, 0, a2), r);
	}
}

//=================================================================================================
void TooltipController::FormatBox()
{
	get_text(this, group, id);

	if(!anything)
		return;

	int w = 0, h = 0;

	Int2 img_size;
	if(img)
		img_size = gui::GetSize(img);
	else
	{
		img_size.x = 0;
		img_size.y = 0;
	}

	// big text
	if(!big_text.empty())
	{
		Int2 text_size = GUI.fBig->CalculateSize(big_text, 400);
		w = text_size.x;
		h = text_size.y + 12;
		r_big_text.Left() = 0;
		r_big_text.Right() = w;
		r_big_text.Top() = 12;
		r_big_text.Bottom() = h + 12;
	}

	// text
	Int2 text_size(0, 0);
	if(!text.empty())
	{
		if(h)
			h += 5;
		else
			h = 12;
		text_size = GUI.default_font->CalculateSize(text, 400);
		if(text_size.x > w)
			w = text_size.x;
		r_text.Left() = 0;
		r_text.Right() = w;
		r_text.Top() = h;
		h += text_size.y;
		r_text.Bottom() = h;
	}

	int shift = 12;

	// image
	if(img)
	{
		shift += img_size.x + 4;
		w += img_size.x + 4;
	}

	// small text
	if(!small_text.empty())
	{
		if(h)
			h += 5;
		Int2 text_size = GUI.fSmall->CalculateSize(small_text, 400);
		text_size.x += 12;
		if(text_size.x > w)
			w = text_size.x;
		r_small_text.Left() = 12;
		r_small_text.Right() = w;
		r_small_text.Top() = h;
		h += text_size.y;
		r_small_text.Bottom() = h;

		if(!small_text.empty())
		{
			int img_bot = img_size.y + 24;
			if(r_small_text.Top() < img_bot)
			{
				int dif = r_small_text.SizeY();
				r_small_text.Top() = img_bot;
				r_small_text.Bottom() = img_bot + dif;
				h = r_small_text.Bottom();
			}
		}
	}
	else if(img)
		h += max(0, img_size.y - text_size.y);

	w += 24;
	h += 12;

	r_big_text += Int2(shift, 0);
	r_text += Int2(shift, 0);

	size = Int2(w, h);
}
