#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct TextColor
{
	cstring text;
	DWORD color;
};

//-----------------------------------------------------------------------------
union Cell
{
	cstring text;
	vector<TEX>* imgset;
	TEX img;
	TextColor* text_color;
};

//-----------------------------------------------------------------------------
typedef delegate<void(int, int, Cell&)> GridEvent;
typedef delegate<void(int, int, int)> SelectGridEvent;

//-----------------------------------------------------------------------------
class Grid : public Control
{
public:
	enum Type
	{
		TEXT,
		TEXT_COLOR,
		IMG,
		IMGSET
	};

	struct Column
	{
		Type type;
		int width;
		string title;
	};

	enum SelectionType
	{
		NONE,
		COLOR,
		BACKGROUND
	};

	Grid();
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);

	void Init();
	void Move(INT2& global_pos);
	inline void LostFocus() { scroll.LostFocus(); }
	inline void AddColumn(Type type, int width, cstring title=nullptr)
	{
		Column& c = Add1(columns);
		c.type = type;
		c.width = width;
		if(title)
			c.title = title;
	}
	void AddItem();
	void AddItems(int count);
	void RemoveItem(int id);
	void Reset();

	vector<Column> columns;
	int items, height, selected, total_width;
	GridEvent event;
	Scrollbar scroll;
	vector<TEX> imgset;
	SelectionType selection_type;
	DWORD selection_color;
	bool single_line;
	SelectGridEvent select_event;
};
