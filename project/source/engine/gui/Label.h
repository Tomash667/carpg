#pragma once

#include "Control.h"

namespace gui
{
	struct LabelLayout
	{
		Font* font;
		Color color;
		uint align;
		Int2 padding;
	};

	class Label : public Control
	{
	public:
		Label(cstring text, bool auto_size = true);
		~Label();

		void Draw(ControlDrawData*) override;

		uint GetAlign() const { return GetLayout().align; }
		Color GetColor() const { return GetLayout().color; }
		Font* GetFont() const { return GetLayout().font; }
		LabelLayout* GetCustomLayout() const { return custom_layout; }
		const LabelLayout& GetLayout() const { return custom_layout ? *custom_layout : *layout->label; }
		const Int2& GetPadding() const { return GetLayout().padding; }
		const string& GetText() const { return text; }
		bool IsAutoSize() const { return auto_size; }

		void SetAlign(uint align);
		void SetColor(Color color);
		void SetCustomLayout(LabelLayout* layout);
		void SetFont(Font* font);
		void SetPadding(const Int2& padding);
		void SetText(Cstring s);
		void SetSize(const Int2& size);

	private:
		void CalculateSize();
		LabelLayout* EnsureLayout();

		string text;
		LabelLayout* custom_layout;
		Rect rect;
		bool auto_size, own_custom_layout;
	};
}
