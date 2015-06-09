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
	}

	// set for item
	inline void Set(cstring _text, int _group, int _id)
	{
		type = Item;
		text = _text;
		group = _group;
		id = _id;
	}

	// set for button
	inline void Set(int _group, int _id, int _tex_id)
	{
		type = Button;
		group = _group;
		id = _id;
		tex_id = _tex_id;
		state = Button::NONE;
	}
};

//-----------------------------------------------------------------------------
typedef fastdelegate::FastDelegate2<int, int> ButtonEvent;

//-----------------------------------------------------------------------------
class FlowContainer2 : public Control
{
public:
	FlowContainer2();
	~FlowContainer2();
	void Update(float dt);
	void Draw(ControlDrawData* cdd = NULL);
	FlowItem2* Add();
	void Clear();
	// set group & index only if there is selection
	void GetSelected(int& group, int& id);
	void UpdateSize(const INT2& pos, const INT2& size, bool visible);
	void UpdatePos(const INT2& parent_pos);
	void Reposition();

	ButtonEvent on_button;
	CustomButton* button_tex;
	INT2 button_size;

private:
	vector<FlowItem2*> items;
	int group, id;
	Scrollbar scroll;
};
