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
class ListBox : public Control
{
public:
	enum Action
	{
		A_BEFORE_CHANGE_INDEX,
		A_INDEX_CHANGED,
		A_BEFORE_MENU_SHOW,
		A_MENU
	};

	ListBox(bool is_new = false);
	~ListBox();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Add(GuiElement* e);
	void Add(cstring text, int value = 0, TEX tex = nullptr) { Add(new DefaultGuiElement(text, value, tex)); }
	void Init(bool extended = false);
	void Sort();
	void ScrollTo(int index);
	GuiElement* Find(int value);
	int FindIndex(int value);
	void Select(int index, bool send_event = false);
	void ForceSelect(int index);
	int GetIndex() const { return selected; }
	GuiElement* GetItem() const { return selected == -1 ? nullptr : items[selected]; }
	template<typename T> T* GetItemCast() const { return (T*)GetItem(); }
	int GetItemHeight() const { return item_height; }
	const INT2& GetForceImageSize() const { return force_img_size; }
	vector<GuiElement*>& GetItems() { return items; }
	template<typename T> vector<T*>& GetItemsCast() { return (vector<T*>&)items; }
	uint GetCount() const { return items.size(); }
	void Insert(GuiElement* e, int index);
	bool IsEmpty() const { return items.empty(); }
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
	void SetForceImageSize(const INT2& _size)
	{
		assert(_size.x >= 0 && _size.y >= 0);
		force_img_size = _size;
	}

	MenuList* menu;
	gui::MenuStrip* menu_strip;
	DialogEvent event_handler;
	delegate<bool(int,int)> event_handler2;

private:
	int PosToIndex(int y);
	void OnSelect(int index);
	bool ChangeIndexEvent(int index, bool force);

	Scrollbar scrollbar;
	vector<GuiElement*> items;
	int selected; // index of selected item or -1, default -1
	int item_height; // height of item, default 20
	INT2 real_size;
	INT2 force_img_size; // forced image size, INT2(0,0) if not forced, default INT2(0,0)
	bool extended;
};
