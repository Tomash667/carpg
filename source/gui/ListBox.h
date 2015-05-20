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
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	void Event(GuiEvent e);
	void Add(GuiElement* e);
	inline void Add(cstring text, TEX tex)
	{
		Add(new DefaultGuiElement(text, tex));
	}
	void Init(bool extended=false);
	void Sort();	
	void ScrollTo(int index);
	
	inline int GetIndex() const
	{
		return selected;
	}
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
	inline int GetItemHeight() const
	{
		return item_height;
	}
	inline const INT2& GetImageSize() const
	{
		return img_size;
	}
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
	inline void SetImageSize(const INT2& size)
	{
		assert(size.x >= 0 && size.y >= 0);
		img_size = size;
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
	INT2 img_size; // forced image size, INT2(0,0) if not forced, default INT2(0,0)
	bool extended;
};
