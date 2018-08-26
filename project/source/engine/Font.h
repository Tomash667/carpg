#pragma once

#undef CreateFont
#undef GetCharWidth

//-----------------------------------------------------------------------------
// Draw text flags
enum DrawTextFlag
{
	DTF_LEFT = 0,
	DTF_TOP = 0,
	DTF_CENTER = 1 << 0,
	DTF_RIGHT = 1 << 1,
	DTF_VCENTER = 1 << 2,
	DTF_BOTTOM = 1 << 3,
	DTF_SINGLELINE = 1 << 4,
	DTF_PARSE_SPECIAL = 1 << 5, // parse $ to set color/hitbox
	DTF_OUTLINE = 1 << 6 // draw outline around text
};

//-----------------------------------------------------------------------------
// Opis znaku czcionki
struct Glyph
{
	Box2d uv;
	int width;
	bool ok;
};

//-----------------------------------------------------------------------------
// Przechowuje string lub cstring
struct StringOrCstring
{
	union
	{
		string* str;
		cstring cstr;
	};
	bool is_str;

	StringOrCstring(cstring cstr) : cstr(cstr), is_str(false)
	{
	}

	StringOrCstring(string& _str) : str(&_str), is_str(true)
	{
	}

	StringOrCstring(const string& _str) : cstr(_str.c_str()), is_str(false)
	{
	}

	StringOrCstring(LocalString& _str) : str(_str.get_ptr()), is_str(true)
	{
	}

	uint length() const
	{
		return is_str ? str->length() : strlen(cstr);
	}

	cstring c_str()
	{
		return is_str ? str->c_str() : cstr;
	}

	void AddTo(string& s) const
	{
		if(is_str)
			s += *str;
		else
			s += cstr;
	}
};

//-----------------------------------------------------------------------------
struct FontLine
{
	uint begin, end, count, width;
};

//-----------------------------------------------------------------------------
// Czcionka
struct Font
{
	TEX tex, texOutline;
	int height;
	float outline_shift;
	Glyph glyph[256];

	// zwraca szerokoœæ znaku
	int GetCharWidth(char c) const
	{
		const Glyph& g = glyph[byte(c)];
		if(g.ok)
			return g.width;
		else
			return 0;
	}
	// oblicza szerokoœæ pojedyñczej linijki tekstu
	int LineWidth(cstring str, bool parse_special = false) const;
	// oblicza wysokoœæ i szerokoœæ bloku tekstu
	Int2 CalculateSize(StringOrCstring str, int limit_width = INT_MAX) const;
	Int2 CalculateSizeWrap(StringOrCstring str, const Int2& max_size, int border = 32) const;
	// split text between lines
	bool SplitLine(uint& out_begin, uint& out_end, int& out_width, uint& in_out_index, cstring text, uint text_end, uint flags, int width) const;

	static bool ParseGroupIndex(cstring text, uint line_end, uint& i, int& index, int& index2);
	bool HitTest(StringOrCstring str, int limit_width, int flags, const Int2& pos, uint& index, Int2& index2, Rect& rect, float& uv,
		const vector<FontLine>* font_lines = nullptr) const;

	// calculate position (top left corner of glyph) from index
	Int2 IndexToPos(uint index, StringOrCstring str, int limit_width, int flags) const;
	Int2 IndexToPos(const Int2& index, StringOrCstring str, int limit_width, int flags) const;
	Int2 IndexToPos(vector<FontLine>& font_lines, const Int2& index, StringOrCstring str, int limit_width, int flags) const;

	// precalculate line begin/end position, width, returns max width
	uint PrecalculateFontLines(vector<FontLine>& font_lines, StringOrCstring str, int limit_width, int flags) const;
	uint ToRawIndex(vector<FontLine>& font_lines, const Int2& index) const;
	Int2 FromRawIndex(vector<FontLine>& font_lines, uint index) const;

private:
	bool SkipSpecial(uint& in_out_index, cstring text, uint text_end) const;
	uint GetLineWidth(cstring text, uint line_begin, uint line_end, bool parse_special) const;
};
