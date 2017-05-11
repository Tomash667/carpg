#pragma once

#include "Control.h"

namespace gui
{
	struct LabelLayout
	{
		Font* font;
		DWORD color;
		DWORD align;
		INT2 padding;
	};

	class Label : public Control
	{
	public:
		Label(cstring text, bool auto_size = true);
		~Label();

		void Draw(ControlDrawData*) override;

		DWORD GetAlign() const { return GetLayout().align; }
		DWORD GetColor() const { return GetLayout().color; }
		Font* GetFont() const { return GetLayout().font; }
		LabelLayout* GetCustomLayout() const { return custom_layout; }
		const LabelLayout& GetLayout() const { return custom_layout ? *custom_layout : *layout->label; }
		const INT2& GetPadding() const { return GetLayout().padding; }
		const string& GetText() const { return text; }
		bool IsAutoSize() const { return auto_size; }

		void SetAlign(DWORD align);
		void SetColor(DWORD color);
		void SetCustomLayout(LabelLayout* layout);
		void SetFont(Font* font);
		void SetPadding(const INT2& padding);
		void SetText(const AnyString& s);
		void SetSize(const INT2& size);

	private:
		void CalculateSize();
		LabelLayout* EnsureLayout();

		string text;
		LabelLayout* custom_layout;
		RECT rect;
		bool auto_size, own_custom_layout;
	};
}
