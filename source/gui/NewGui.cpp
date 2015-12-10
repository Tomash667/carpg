#include "Pch.h"
#include "Base.h"

namespace NewGui
{
	class Control;
	class Container;

	typedef fastdelegate::FastDelegate1<Control*> Event;

	class Control
	{
	public:
		Control() : pos(0, 0), real_pos(0, 0), visible(true), real_visible(true)
		{
			id = last_id++;
		}

		virtual void OnDraw() {}
		virtual void OnUpdate(float dt) {}

		inline bool IsVisible() const { return real_visible; }

		virtual void Show(bool show = true)
		{
			visible = show;
			if(parent)
				real_visible = visible && parent->real_visible;
			else
				real_visible = visible;
		}
		inline void Hide()
		{
			Show(false);
		}

	protected:
		VEC2 pos, real_pos;
		int id;
		bool visible, real_visible;
		// cursor info
		Container* parent;

		static int last_id;
	};

	int Control::last_id = 0;

	class Button : public Control
	{
	public:
		Button(cstring text, Event onClick) : text(text), onClick(onClick) {}

	protected:
		string text;
		Event onClick;
	};

	class Container : public Control
	{
	public:
		virtual void OnDraw()
		{
			for(Control* control : controls)
			{
				if(control->IsVisible())
					control->OnDraw();
			}
		}

		virtual void OnUpdate(float dt)
		{
			for(Control* control : controls)
			{
				if(control->IsVisible())
					control->OnUpdate(dt);
			}
		}

		virtual void Show(bool show)
		{
			Control::Show(show);

		}

		void Add(Control* control)
		{
			assert(control);
			//if()
		}

	private:
		vector<Control*> controls;
	};

	class Property
	{
	public:
	};

	class GuiObject
	{
	public:
		virtual vector<Property*> GetProperties() { return nullptr; }
	};

	class PropertyGrid : public Control
	{
	public:
		inline GuiObject* GetObject() const { return object; }

		inline void SetObject(GuiObject* new_object) { object = new_object; }

	protected:
		GuiObject* object;
	};

	class Overlay : public Container
	{
	public:
	};

	class ItemWrapper : GuiObject
	{
	public:
	private:
		string id, name, mesh, desc;
	};


	void TestMsg(Control* c)
	{
		MessageBox(0, "test", 0, 0);
	}

	void Test()
	{
		Overlay* overlay = new Overlay;
		PropertyGrid* prop
		Button* ok = new Button("Test", TestMsg);

		overlay->Add(ok);
	}
};
