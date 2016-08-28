#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Scrollbar.h"
#include "Button.h"

//-----------------------------------------------------------------------------
struct FlowItem2
{
	enum Type
	{
		Item,
		Section,
		Button
	};

	Type type;
	string text;
	int group, id, tex_id;
	INT2 pos, size;
	Button::State state;

	// set for section
	inline void Set(cstring _text)
	{
		type = Section;
		text = _text;
		state = Button::NONE;
		group = -1;
		id = -1;
	}

	// set for item
	inline void Set(cstring _text, int _group, int _id)
	{
		type = Item;
		text = _text;
		group = _group;
		id = _id;
		state = Button::NONE;
	}

	// set for button
	inline void Set(int _group, int _id, int _tex_id, bool disabled = false)
	{
		type = Button;
		group = _group;
		id = _id;
		tex_id = _tex_id;
		state = (disabled ? Button::DISABLED : Button::NONE);
	}

	static ObjectPool<FlowItem2> Pool;
};

//-----------------------------------------------------------------------------
typedef delegate<void(int,int)> ButtonEvent;

//-----------------------------------------------------------------------------
class FlowContainer2 : public Control
{
public:
	FlowContainer2();
	~FlowContainer2();
	void Update(float dt);
	void Draw(ControlDrawData* cdd = nullptr);
	FlowItem2* Add();
	void Clear();
	// set group & index only if there is selection
	void GetSelected(int& group, int& id);
	void UpdateSize(const INT2& pos, const INT2& size, bool visible);
	void UpdatePos(const INT2& parent_pos);
	void Reposition();
	FlowItem2* Find(int group, int id);
	inline void ResetScrollbar()
	{
		scroll.offset = 0.f;
	}
	void SetItems(vector<FlowItem2*>& _items);
	inline int GetHeight() const
	{
		return scroll.total;
	}
	void UpdateText(FlowItem2* item, cstring text, bool batch = false);
	inline void UpdateText(int _group, int _id, cstring _text, bool batch = false)
	{
		UpdateText(Find(_group, _id), _text, batch);
	}
	void UpdateText();

	vector<FlowItem2*> items;
	ButtonEvent on_button;
	CustomButton* button_tex;
	INT2 button_size;
	bool word_warp, allow_select;
	VoidF on_select;
	FlowItem2* selected;

private:
	void UpdateScrollbar(int new_size);

	int group, id;
	Scrollbar scroll;
	bool batch_changes;
};
