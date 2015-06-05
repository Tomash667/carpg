#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct FlowItem2
{
	bool section;
	string text;
	int group, id;
	INT2 pos, size;

	inline void Set(cstring _text)
	{
		section = true;
		text = _text;
	}

	inline void Set(cstring _text, int _group, int _id)
	{
		section = false;
		text = _text;
		group = _group;
		id = _id;
	}
};

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
	void Reposition();

private:
	vector<FlowItem2*> items;
	int group, id;
	Scrollbar scroll;
	bool redo_select;
	INT2 last_pos;
};
