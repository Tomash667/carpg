#pragma once

//-----------------------------------------------------------------------------
class GuiElement
{
public:
	inline GuiElement(int value = 0, TEX tex = nullptr) : value(value), tex(tex)
	{

	}
	virtual ~GuiElement() {}
	virtual cstring ToString() = 0;

	int value;
	TEX tex;
};

//-----------------------------------------------------------------------------
class DefaultGuiElement : public GuiElement
{
public:
	inline DefaultGuiElement(cstring _text, int value = 0, TEX tex = nullptr) : GuiElement(value, tex)
	{
		text = _text;
	}

	inline cstring ToString()
	{
		return text.c_str();
	}

	string text;
};
