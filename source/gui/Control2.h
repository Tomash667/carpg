#pragma once

#include "Control.h"

/*
NEW GUI BASICS:
+ send GuiEvent_Resize then GuiEvent_Moved at creation
+ visibility is checked by parent control, gui_focus too
+ invisible controls are not drawed or updated
+ invisible controls are repositioned/resized
+ Focus
	+ all controls in chain have focus (overlay have focus, one of panels have focus, textbox in this panel have focus)
	+ containers control child focust using member 'Control* child_focus'
	+ control should use GUI.GainFocus(control), GUI.LostFocus(control)
*/

namespace gui
{
	enum ElementState
	{
		NONE,
		HOVER,
		PRESSED
	};

	enum Flags
	{
		REQUIRE_UPDATE = 1<<0,
		TRACK_CURSOR = 1<<1
	};

	enum UpdateFlags
	{
		UF_IN = 1<<0,
		UF_PARENT_IN = 1<<1,
		UF_RESIZE = 1<<2,
		UF_REPOSITION = 1<<3
	};

	enum GuiEvent2
	{
		GE_GainFocus,
		GE_LostFocus,
		GE_Moved,
		GE_Resized,
		GE_Shown,
		GE_WindowResized,
		GE_Closed,
		GE_Created
	};

	class Control2 : protected Control
	{
	public:
		Control2(bool is_container = false) : is_container(is_container) {}

		inline virtual void Draw2() {}
		virtual void Event2(GuiEvent2 e, void* data);
		virtual void UpdateSize();
		virtual void UpdatePosition();
		/*Control2(int flags = 0, int id = 0) : flags(flags), id(id) {}

		inline int GetId() const { return id; }
		inline const INT2& GetLocalPosition() const { return pos; }
		inline const INT2& GetPosition() const { return global_pos; }
		inline const INT2& GetSize() const { return size; }

		inline void SetId(int _id) { id = _id; }
		inline virtual void SetSize(const INT2& _size) { size = _size; }

		inline virtual void Draw2() {}
		void BeginUpdate();
		void EndUpdate();

	protected:
		virtual void OnBeginUpdate() {}
		virtual void OnEndUpdate() {}*/

		inline bool IsContainer() const { return is_container; }

	private:
		inline void Draw(ControlDrawData* cdd = nullptr) final { Draw2(); }
		inline void Event(GuiEvent e) final { Event2((GuiEvent2)e, nullptr); }

		//int id, flags, update_flags;
		bool is_container;
	};

	typedef delegate<void(Control2*, int)> Event;
	typedef delegate<void(int)> Action;
}
