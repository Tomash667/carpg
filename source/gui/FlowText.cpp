#include "Pch.h"
#include "Base.h"
#include "FlowText.h"

//=================================================================================================
void FlowText::Draw(ControlDrawData*)
{
	RECT rect = {global_pos.x, global_pos.y, global_pos.x+size.x, global_pos.y+size.y};
	ControlDrawData cdd;
	cdd.clipping = &rect;
	hitboxes.clear();
	cdd.hitboxes = &hitboxes;
	int counter = 0;
	cdd.hitbox_counter = &counter;

	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		StaticText* t = (StaticText*)*it;
		if(t->visible)
		{
			RECT r = {
				t->global_pos.x,
				t->global_pos.y,
				t->global_pos.x+t->size.x,
				t->global_pos.y+t->size.y
			};
			if(!GUI.DrawText(t->font, t->text.c_str(), 0, t->color, r, cdd.clipping, cdd.hitboxes, cdd.hitbox_counter))
				break;
		}
	}
}

//=================================================================================================
void FlowText::Calculate()
{
	int offset = 0, offset_x = 0;
	int width = 0;
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		StaticText* st = (StaticText*)*it;
		INT2 s = st->font->CalculateSize(st->text, size.x-offset_x);
		/*if(offset + c->size.y > size.y)
		{
			offset_x += width+8;
			width = 0;
			offset = 0;
		}*/
		st->size = s;
		if(st->size.x > width)
			width = st->size.x;
		st->pos.x = offset_x;
		st->pos.y = offset-moved;
		st->global_pos.x = global_pos.x + st->pos.x;
		st->global_pos.y = global_pos.y + st->pos.y;
		offset += st->size.y;
	}

	total_size = offset;
}
