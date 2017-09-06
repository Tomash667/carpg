#pragma once

//-----------------------------------------------------------------------------
#include "Scrollbar.h"
#include "GuiElement.h"

//-----------------------------------------------------------------------------
class MenuList;
namespace gui
{
	class MenuStrip;
}

//-----------------------------------------------------------------------------
class ListBox : public Control, public OnCharHandler
{
public:
	enum Action
	{
		A_BEFORE_CHANGE_INDEX,
		A_INDEX_CHANGED,
		A_BEFORE_MENU_SHOW,
		A_MENU,
		A_DOUBLE_CLICK
	};

	typedef delegate<bool(int, int)> Handler;

	ListBox(bool is_new = false);
	~ListBox();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void OnChar(char c) override;

	void Add(GuiElement* e);
	void Add(cstring text, int value = 0, TEX tex = nullptr) { Add(new DefaultGuiElement(text, value, tex)); }
	void Sort();
	void ScrollTo(int index, bool center = false);
	GuiElement* Find(int value);
	int FindIndex(int value);
	void Select(int index, bool send_event = false);
	void Select(delegate<bool(GuiElement*)> pred, bool send_event = false);
	void ForceSelect(int index);
	void Insert(GuiElement* e, int index);
	void Remove(int index);
	void SetIndex(int index)
	{
		assert(index >= -1 && index < (int)items.size());
		selected = index;
	}
	void SetItemHeight(int height)
	{
		assert(height > 0);
		item_height = height;
	}
	void SetForceImageSize(const Int2& _size)
	{
		assert(_size.x >= 0 && _size.y >= 0);
		force_img_size = _size;
	}
	void Reset();

	int GetIndex() const { return selected; }
	GuiElement* GetItem() const { return selected == -1 ? nullptr : items[selected]; }
	template<typename T> T* GetItemCast() const { return (T*)GetItem(); }
	int GetItemHeight() const { return item_height; }
	const Int2& GetForceImageSize() const { return force_img_size; }
	vector<GuiElement*>& GetItems() { return items; }
	template<typename T> vector<T*>& GetItemsCast() { return (vector<T*>&)items; }
	uint GetCount() const { return items.size(); }

	bool IsCollapsed() { return collapsed; }
	bool IsEmpty() const { return items.empty(); }

	void SetCollapsed(bool new_collapsed) { assert(!initialized); collapsed = new_collapsed; }

	MenuList* menu;
	gui::MenuStrip* menu_strip;
	DialogEvent event_handler;
	Handler event_handler2;

private:
	int PosToIndex(int y);
	void OnSelect(int index);
	bool ChangeIndexEvent(int index, bool force, bool scroll_to);
	void UpdateScrollbarVisibility();

	Scrollbar scrollbar;
	vector<GuiElement*> items;
	int selected; // index of selected item or -1, default -1
	int item_height; // height of item, default 20
	Int2 real_size;
	Int2 force_img_size; // forced image size, Int2(0,0) if not forced, default Int2(0,0)
	bool collapsed, require_scrollbar;
};
