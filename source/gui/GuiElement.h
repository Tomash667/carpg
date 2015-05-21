#pragma once

//-----------------------------------------------------------------------------
class GuiElement
{
public:
	inline GuiElement(int value = 0, TEX tex = NULL) : value(value), tex(tex)
	{

	}

	virtual cstring ToString() = 0;

	int value;
	TEX tex;
};

//-----------------------------------------------------------------------------
class DefaultGuiElement : public GuiElement
{
public:
	inline DefaultGuiElement(cstring _text, int value = 0, TEX tex = NULL) : GuiElement(value, tex)
	{
		text = _text;
	}

	inline cstring ToString()
	{
		return text.c_str();
	}

	string text;
};
