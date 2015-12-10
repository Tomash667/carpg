#pragma once

class GuiProvider
{
public:
	Font* DefaultFont;
};

GuiProvider Provider;

class Control2
{
public:
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	Control2() : changes(false), global_pos(0,0), layoutUpdate(false), parent(nullptr), pos(0,0), size(0,0)
	{

	}

	virtual ~Control2()
	{

	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline virtual void BeginLayoutUpdate()
	{
		assert(!layoutUpdate);
		layoutUpdate = true;
	}

	inline virtual void EndLayoutUpdate()
	{
		assert(layoutUpdate);
		layoutUpdate = false;
		if(changes)
			UpdateLayout();
	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline const INT2& GetSize() const
	{
		return size;
	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline void SetSize(const INT2& new_size)
	{
		assert(new_size.x >= 0 && new_size.y >= 0);
		new_size = Max(new_size, INT2(0,0));
		if(new_size != size)
		{
			size = new_size;
			LayoutChange();
		}
	}

protected:
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline void LayoutChange()
	{
		if(!layoutUpdate)
			UpdateLayout();
		else
			changes = true;
	}

	inline virtual void UpdateLayout()
	{
		changes = false;
	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	bool changes;
	INT2 global_pos;
	bool layoutUpdate;
	Control2* parent;
	INT2 pos;
	INT2 size;
};

class Panel : public Control2
{
public:
	inline void Add(Control2* control)
	{
		assert(control);
		controls.push_back(control);
	}

protected:
	vector<Control2*> controls;
};

class Label : public Control2
{
public:
	string text;
};

class GuiItem
{
public:
	virtual cstring ToString() = 0;
};

class GuiItemContainer : public Control2
{
public:
	inline void Add(GuiItem* item)
	{
		assert(item);
		items.push_back(item);
	}

protected:
	vector<GuiItem*> items;
};

class ComboBox : public GuiItemContainer
{
public:
};

class ListBox2 : public GuiItemContainer
{
public:
};

class CheckBox2 : public Control2
{
public:
	inline CheckBox2() : checked(false)
	{

	}

	inline CheckBox2(cstring _text) : checked(false)
	{
		assert(_text);
		text = _text;
	}

	inline void SetText(cstring _text)
	{
		assert(_text);
		text = _text;
	}

private:
	string text;
	bool checked;
};

class Slider3 : public Control2
{
public:
};

template<typename T>
class LabelControl : public Control2
{
public:
	inline LabelControl()
	{

	}

	inline LabelControl(cstring _text)
	{
		assert(_text);
		label.text = _text;
	}

	inline void SetText(cstring text)
	{
		assert(text);
		label.text = text;
	}

	inline T& GetControl()
	{
		return control;
	}

private:
	Label label;
	T control;
};

typedef LabelControl<ComboBox> LabelComboBox;
typedef LabelControl<ListBox2> LabelListBox;
typedef LabelControl<Slider3> LabelSlider;

class Button2 : public Control2
{
public:
	typedef fastdelegate::FastDelegate0<void> ClickEvent;

	inline Button2() : onClick(nullptr)
	{

	}

	inline Button2(cstring _text, ClickEvent onClick=nullptr) : onClick(onClick)
	{
		assert(_text);
		text = _text;
	}

	inline void SetText(cstring _text)
	{
		assert(_text);
		text = _text;
	}

private:
	string text;
	ClickEvent onClick;
};

class FlowPanel : public Panel
{
public:
};

class TabElement : public Control2
{
	friend class TabControl;
public:
	inline TabElement() : panel(nullptr)
	{

	}

	inline TabElement(cstring _text) : panel(nullptr)
	{
		assert(_text);
		text = _text;
	}

	void BeginLayoutUpdate();
	void EndLayoutUpdate();
	void UpdateLayout();

	inline void SetText(cstring _text)
	{
		assert(_text);
		text = _text;
	}

	inline void SetPanel(Panel* _panel)
	{
		assert(panel);
		panel = _panel;
	}

private:
	Font* font;
	Panel* panel;
	string text;
};

class TabControl : public Control2
{
public:
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	enum TabWidthMode
	{
		TabWidthMode_Normal,
		TabWidthMode_Fixed,
		TabWidthMode_Max,
		TabWidthMode_Split
	};

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline TabControl() : clientOffset(4,4), clientSize(0,0), tabHeight(20), tabOffset(4,4) tabWidthMode(TabWidthMode_Normal)
	{

	}

	inline ~TabControl()
	{

	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	void Add(TabElement* e);
	void BeginLayoutUpdate();
	void EndLayoutUpdate();
	void UpdateLayout();

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline const INT2& GetClientOffset() const
	{
		return clientOffset;
	}

	inline const INT2& GetClientSize() const
	{
		return clientSize;
	}

	inline int GetTabHeight() const
	{
		return tabHeight;
	}

	inline const INT2& GetTabOffset() const
	{
		return tabOffset;
	}

	inline TabWidthMode GetTabWidthMode() const
	{
		return tabWidthMode;
	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	inline void SetClientOffset(INT2 offset)
	{
		assert(offset.x >= 0 && offset.y >= 0);
		offset = Max(offset, INT2(0,0));
		if(offset != clientOffset)
		{
			clientOffset = offset;
			LayoutChange();
		}
	}

	inline void SetTabHeight(int height)
	{
		assert(height >= 0);
		height = max(height, 0);
		if(height != tabHeight)
		{
			tabHeight = height;
			LayoutChange();
		}
	}

	inline void SetTabOffset(INT2 offset)
	{
		assert(offset.x >= 0 && offset.y >= 0);
		offset = Max(offset, INT2(0,0));
		if(offset != tabOffset)
		{
			tabOffset = offset;
			LayoutChange();
		}
	}

	inline void SetTabWidthMode(TabWidthMode mode)
	{
		if(mode != tabWidthMode)
		{
			tabWidthMode = mode;
			LayoutChange();
		}
	}

private:
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	INT2 clientOffset;
	INT2 clientSize;
	int tabHeight;
	Font* tabFont;
	INT2 tabOffset;
	vector<TabElement*> tabs;
	TabWidthMode tabWidthMode;
};
