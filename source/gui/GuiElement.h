#pragma once

//-----------------------------------------------------------------------------
class GuiElement
{
public:
	GuiElement() : tex(NULL)
	{

	}

	virtual cstring ToString() = 0;

	TEX tex;
};

//-----------------------------------------------------------------------------
class DefaultGuiElement : public GuiElement
{
public:
	inline DefaultGuiElement(cstring _text, TEX _tex=NULL)
	{
		text = _text;
		tex = _tex;
	}

	inline cstring ToString()
	{
		return text.c_str();
	}

	string text;
};
