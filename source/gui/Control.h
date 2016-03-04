#pragma once

//-----------------------------------------------------------------------------
#include "Gui2.h"

//-----------------------------------------------------------------------------
struct ControlDrawData
{
	RECT* clipping;
	vector<Hitbox>* hitboxes;
	int* hitbox_counter;
};

//-----------------------------------------------------------------------------
inline bool PointInRect(const INT2& pt, const RECT& r)
{
	return pt.x >= r.left && pt.y >= r.top && pt.x < r.right && pt.y < r.bottom;
}

//-----------------------------------------------------------------------------
inline bool PointInRect(const INT2& pt, int left, int top, int right, int bottom)
{
	return pt.x >= left && pt.y >= top && pt.x < right && pt.y < bottom;
}

//-----------------------------------------------------------------------------
inline bool PointInRect(const INT2& pt, const INT2& rpos, const INT2& rsize)
{
	return pt.x >= rpos.x && pt.y >= rpos.y && pt.x < rpos.x+rsize.x && pt.y < rpos.y+rsize.y;
}

//-----------------------------------------------------------------------------
class Control
{
public:
	Control() : pos(0, 0), global_pos(0, 0), size(0, 0), parent(nullptr), visible(true), focus(false), mouse_focus(false), focusable(false)
	{

	}
	virtual ~Control() {}

	INT2 pos, global_pos, size;
	Control* parent;
	bool visible, focus, mouse_focus, focusable;

	virtual void Draw(ControlDrawData* cdd=nullptr) {}
	virtual void Update(float dt) {}
	virtual void CalculateSize(int limit_width) {}
	virtual bool NeedCursor() const { return false; }

	inline INT2 GetCursorPos() const
	{
		return GUI.cursor_pos - pos;
	}
	virtual void Event(GuiEvent e)
	{

	}

	inline bool IsInside(const INT2& pt) const
	{
		return pt.x >= global_pos.x && pt.y >= global_pos.y && pt.x < global_pos.x+size.x && pt.y < global_pos.y+size.y;
	}

	static inline INT2 Center(const INT2& in_size) { return INT2((GUI.wnd_size.x - in_size.x)/2, (GUI.wnd_size.y - in_size.y)/2); }
	static inline INT2 Center(int w, int h) { return INT2((GUI.wnd_size.x - w)/2, (GUI.wnd_size.y - h)/2); }

	inline void SimpleDialog(cstring text)
	{
		GUI.SimpleDialog(text, this);
	}

	inline void GainFocus()
	{
		if(!focus)
		{
			focus = true;
			Event(GuiEvent_GainFocus);
		}
	}

	inline void LostFocus()
	{
		if(focus)
		{
			focus = false;
			Event(GuiEvent_LostFocus);
		}
	}

	static TEX tDialog;

	inline void ResizeImage(TEX t, INT2& new_size, INT2& img_size, VEC2& scale)
	{
		D3DSURFACE_DESC desc;
		t->GetLevelDesc(0, &desc);
		img_size = INT2(desc.Width, desc.Height);
		if(new_size == INT2(0, 0))
		{
			new_size = img_size;
			scale = VEC2(1, 1);
		}
		else if(new_size == img_size)
			scale = VEC2(1, 1);
		else
			scale = VEC2(float(new_size.x) / img_size.x, float(new_size.y) / img_size.y);
	}
};
