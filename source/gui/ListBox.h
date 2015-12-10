#pragma once

//-----------------------------------------------------------------------------
#include "Scrollbar.h"
#include "MenuList.h"
#include "GuiElement.h"

//-----------------------------------------------------------------------------
class ListBox : public Control
{
public:
	ListBox();
	~ListBox();
	//-----------------------------------------------------------------------------
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	void Add(GuiElement* e);
	inline void Add(cstring text, int value=0, TEX tex=nullptr)
	{
		Add(new DefaultGuiElement(text, value, tex));
	}
	void Init(bool extended=false);
	void Sort();	
	void ScrollTo(int index);
	GuiElement* Find(int value);
	int FindIndex(int value);
	void Select(int index);
	//-----------------------------------------------------------------------------
	inline int GetIndex() const
	{
		return selected;
	}
	inline GuiElement* GetItem() const
	{
		if(selected == -1)
			return nullptr;
		else
			return items[selected];
	}
	template<typename T>
	inline T* GetItemCast() const
	{
		if(selected == -1)
			return nullptr;
		else
			return (T*)items[selected];
	}
	inline int GetItemHeight() const
	{
		return item_height;
	}
	inline const INT2& GetForceImageSize() const
	{
		return force_img_size;
	}
	inline vector<GuiElement*>& GetItems()
	{
		return items;
	}
	template<typename T>
	inline vector<T*>& GetItemsCast()
	{
		return (vector<T*>&)items;
	}
	//-----------------------------------------------------------------------------
	inline void SetIndex(int index)
	{
		assert(index >= -1 && index < (int)items.size());
		selected = index;
	}
	inline void SetItemHeight(int height)
	{
		assert(height > 0);
		item_height = height;
	}
	inline void SetForceImageSize(const INT2& size)
	{
		assert(size.x >= 0 && size.y >= 0);
		force_img_size = size;
	}
	//-----------------------------------------------------------------------------
	MenuList* menu;
	DialogEvent event_handler;

private:
	void OnSelect(int index);
	
	Scrollbar scrollbar;
	vector<GuiElement*> items;
	int selected; // index of selected item or -1, default -1
	int item_height; // height of item, default 20
	INT2 real_size;
	INT2 force_img_size; // forced image size, INT2(0,0) if not forced, default INT2(0,0)
	bool extended;
};
