#pragma once

#undef CreateFont
#undef GetCharWidth

//-----------------------------------------------------------------------------
// W³asne flagi do rysowania tekstu
// aktualnie obs³ugiwane DT_LEFT, DT_CENTER, DT_RIGHT, DT_TOP, DT_VCENTER, DT_BOTTOM, DT_SINGLELINE oraz te poni¿ej
#define DT_PARSE_SPECIAL (1<<30) // parse $ to set color/hitbox
#define DT_OUTLINE (1<<31) // draw outline around text

//-----------------------------------------------------------------------------
// Opis znaku czcionki
struct Glyph
{
	BOX2D uv;
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
	INT2 CalculateSize(StringOrCstring str, int limit_width = INT_MAX) const;
	INT2 CalculateSizeWrap(StringOrCstring str, const INT2& max_size, int border = 32) const;
	// split text between lines
	bool SplitLine(uint& out_begin, uint& out_end, int& out_width, uint& in_out_index, cstring text, uint text_end, DWORD flags, int width) const;

	static bool ParseGroupIndex(cstring text, uint line_end, uint& i, int& index, int& index2);
	bool HitTest(StringOrCstring str, int limit_width, int flags, const INT2& pos, uint& index, INT2& index2, IBOX2D& rect, float& uv,
		const vector<FontLine>* font_lines = nullptr) const;

	// calculate position (top left corner of glyph) from index
	INT2 IndexToPos(uint index, StringOrCstring str, int limit_width, int flags) const;
	INT2 IndexToPos(const INT2& index, StringOrCstring str, int limit_width, int flags) const;
	INT2 IndexToPos(vector<FontLine>& font_lines, const INT2& index, StringOrCstring str, int limit_width, int flags) const;

	// precalculate line begin/end position, width, returns max width
	uint PrecalculateFontLines(vector<FontLine>& font_lines, StringOrCstring str, int limit_width, int flags) const;
	uint ToRawIndex(vector<FontLine>& font_lines, const INT2& index) const;
	INT2 FromRawIndex(vector<FontLine>& font_lines, uint index) const;

private:
	bool SkipSpecial(uint& in_out_index, cstring text, uint text_end) const;
	uint GetLineWidth(cstring text, uint line_begin, uint line_end, bool parse_special) const;
};
