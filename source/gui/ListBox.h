#pragma once

//-----------------------------------------------------------------------------
#include "Scrollbar.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
class GuiElement
{
public:
	virtual cstring ToString() = 0;
};

//-----------------------------------------------------------------------------
class ListBox : public Control
{
public:
	ListBox();
	~ListBox();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	void Event(GuiEvent e);

	void Add(GuiElement* e);
	void Init(bool extended=false);
	inline int GetIndex() const { return selected; }
	template<typename T>
	inline T GetItem() const
	{
		if(selected == -1)
			return NULL;
		else
			return (T)items[selected];
	}
	inline GuiElement* GetBaseItem() const
	{
		if(selected == -1)
			return NULL;
		else
			return items[selected];
	}
	inline void SetIndex(int index)
	{
		assert(index >= -1 && index < (int)items.size());
		selected = index;
	}
	void ScrollTo(int index);
	template<typename T>
	inline vector<T>& GetItems()
	{
		return (vector<T>&)items;
	}
	inline vector<GuiElement*>& GetBaseItems()
	{
		return items;
	}
	template<typename T>
	inline T& GetItemRef() const
	{
		assert(selected != -1);
		return *(T*)items[selected];
	}

	DialogEvent e_change_index;
	MenuList* menu;

private:
	void OnSelect(int index);

	Scrollbar scrollbar;
	vector<GuiElement*> items;
	vector<string*> texts;
	int selected;
	INT2 real_size;
	bool extended;
};
