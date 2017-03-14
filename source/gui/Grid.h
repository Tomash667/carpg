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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	void Init();
	void Move(INT2& global_pos);
	void LostFocus() { scroll.LostFocus(); }
	void AddColumn(Type type, int width, cstring title = nullptr);
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
	SelectGridEvent select_event;
	bool single_line;
};
