#pragma once

#include "Control.h"

namespace gui
{
	class Label : public Control
	{
	public:
		Label(cstring text, bool auto_size = true) : text(text), override(nullptr), auto_size(auto_size) {}

		void Draw(ControlDrawData*) override;
		void Event(GuiEvent e) override;

		inline const Layout::Label& GetLayout() const { return override ? *override : layout->label; }
		inline Layout::Label* GetLayoutOverride() const { return override; }
		inline const string& GetText() const { return text; }
		inline bool IsAutoSize() const { return auto_size; }

		void SetLayoutOverride(Layout::Label* override);
		void SetText(const AnyString& s);
		void SetSize(const INT2& size);

	private:
		void CalculatePos();
		void CalculateSize();

		string text;
		Layout::Label* override;
		RECT rect;
		bool auto_size;
	};
}
