#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar;

//-----------------------------------------------------------------------------
class TextBox : public Control, public OnCharHandler
{
public:
	explicit TextBox(bool is_new = false);
	~TextBox();

	// from Control
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

	void AddScrollbar();
	void Move(const Int2& global_pos);
	void Add(cstring str);
	void CalculateOffset(bool center);
	void Reset();
	void UpdateScrollbar();
	void UpdateSize(const Int2& pos, const Int2& size);
	void SetText(cstring text);
	const string& GetText() const { return text; }
	void SelectAll();
	void SetBackground(TEX t) { tBackground = t; }
	TEX GetBackground() { return tBackground; }
	bool IsMultiline() const { return multiline; }
	bool IsNumeric() const { return numeric; }
	bool IsReadonly() const { return readonly; }
	void SetMultiline(bool new_multiline) { assert(!initialized); multiline = new_multiline; }
	void SetNumeric(bool new_numeric) { numeric = new_numeric; }
	void SetReadonly(bool new_readonly) { readonly = new_readonly; }

	static TEX tBox;
	int limit, num_min, num_max;
	cstring label;
	Scrollbar* scrollbar;

private:
	void ValidateNumber();
	void GetCaretPos(const Int2& in_pos, Int2& index, Int2& pos, uint* char_index = nullptr);
	void CalculateSelection(const Int2& new_index, const Int2& new_pos);
	void CalculateSelection(Int2 index1, Int2 pos1, Int2 index2, Int2 pos2);
	void DeleteSelection();
	Int2 IndexToPos(const Int2& index);
	uint ToRawIndex(const Int2& index);
	void UpdateFontLines();

	string text;
	vector<FontLine> font_lines;
	Int2 real_size, text_size, caret_pos, select_start_pos, select_end_pos, caret_index, select_start_index, select_end_index, select_fixed_index;
	float caret_blink, offset_move;
	int offset, last_y_move;
	TEX tBackground;
	bool added, down, readonly, multiline, numeric, require_scrollbar;
};
