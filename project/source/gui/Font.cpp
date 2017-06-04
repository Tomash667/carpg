#include "Pch.h"
#include "Base.h"
#include "Font.h"

//=================================================================================================
/* Taken from TFQ, modified for carpg
returns false when reached end of text
parametry:
out_begin - pierszy znak w tej linijce
out_end - ostatni znak w tej linijce
out_width - szerokoœæ tej linijki
in_out_index - offset w text
text - tekst
text_end - d³ugoœæ tekstu
flags - flagi (uwzglêdnia tylko DT_SINGLELINE, DT_USER_TEXT, DT_PARSE_SPECIAL)
width - maksymalna szerokoœæ tej linijki
*/
bool Font::SplitLine(uint& out_begin, uint& out_end, int& out_width, uint& in_out_index, cstring text, uint text_end, DWORD flags, int width) const
{
	// Heh, piszê ten algorytm chyba trzeci raz w ¿yciu.
	// Za ka¿dym razem idzie mi szybciej i lepiej.
	// Ale zawsze i tak jest dla mnie ogromnie trudny i skomplikowany.
	if(in_out_index >= text_end)
		return false;

	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	out_begin = in_out_index;
	out_width = 0;

	// Pojedyncza linia - specjalny szybki tryb
	if(IS_SET(flags, DT_SINGLELINE))
	{
		while(in_out_index < text_end)
		{
			char c = text[in_out_index];
			if(c == '$' && parse_special)
			{
				if(SkipSpecial(in_out_index, text, text_end))
					continue;
			}
			else
				++in_out_index;
			out_width += GetCharWidth(c);
		}
		out_end = in_out_index;

		return true;
	}

	// Zapamiêtany stan miejsca ostatniego wyst¹pienia spacji
	// Przyda siê na wypadek zawijania wierszy na granicy s³owa.
	uint last_space_index = string::npos;
	int width_when_last_space;

	for(;;)
	{
		// Koniec tekstu
		if(in_out_index >= text_end)
		{
			out_end = text_end;
			break;
		}

		// Pobierz znak
		char c = text[in_out_index];

		// Koniec wiersza
		if(c == '\n')
		{
			out_end = in_out_index;
			in_out_index++;
			break;
		}
		// Koniec wiersza \r
		else if(c == '\r')
		{
			out_end = in_out_index;
			in_out_index++;
			// Sekwencja \r\n - pomiñ \n
			if(in_out_index < text_end && text[in_out_index] == '\n')
				in_out_index++;
			break;
		}
		else if(c == '$' && parse_special && SkipSpecial(in_out_index, text, text_end))
			continue;
		else
		{
			// Szerokoœæ znaku
			int char_width = GetCharWidth(text[in_out_index]);

			// Jeœli nie ma automatycznego zawijania wierszy lub
			// jeœli siê zmieœci lub
			// to jest pierwszy znak (zabezpieczenie przed nieskoñczonym zapêtleniem dla width < szerokoœæ pierwszego znaku) -
			// dolicz go i ju¿
			if(/*Flags & FLAG_WRAP_NORMAL ||*/ out_width + char_width <= width || in_out_index == out_begin)
			{
				// Jeœli to spacja - zapamiêtaj dane
				if(c == ' ')
				{
					last_space_index = in_out_index;
					width_when_last_space = out_width;
				}
				out_width += char_width;
				in_out_index++;
			}
			// Jest automatyczne zawijanie wierszy i siê nie mieœci
			else
			{
				// Niemieszcz¹cy siê znak to spacja
				if(c == ' ')
				{
					out_end = in_out_index;
					// Mo¿na go przeskoczyæ
					in_out_index++;
					break;
				}
				// Poprzedni znak za tym to spacja
				else if(in_out_index > out_begin && text[in_out_index - 1] == ' ')
				{
					// Koniec bêdzie na tej spacji
					out_end = last_space_index;
					out_width = width_when_last_space;
					break;
				}

				// Zawijanie wierszy na granicy s³owa
				if(1/*Flags & FLAG_WRAP_WORD*/)
				{
					// By³a jakaœ spacja
					if(last_space_index != string::npos)
					{
						// Koniec bêdzie na tej spacji
						out_end = last_space_index;
						in_out_index = last_space_index + 1;
						out_width = width_when_last_space;
						break;
					}
					// Nie by³o spacji - trudno, zawinie siê jak na granicy znaku
				}

				out_end = in_out_index;
				break;
			}
		}
	}

	return true;
}

//=================================================================================================
int Font::LineWidth(cstring str, bool parse_special) const
{
	int w = 0;

	while(true)
	{
		char c = *str;
		if(c == 0)
			break;

		if(c == '$' && parse_special)
		{
			++str;
			c = *str;
			assert(c);
			switch(c)
			{
			case '$':
				w += glyph[byte('$')].width;
				++str;
				break;
			case 'c':
				++str;
				++str;
				break;
			}

			continue;
		}

		w += glyph[byte(c)].width;
		++str;
	}

	return w;
}

//=================================================================================================
INT2 Font::CalculateSize(StringOrCstring str, int limit_width) const
{
	int len = str.length();
	cstring text = str.c_str();

	INT2 size(0, 0);

	uint unused, index = 0;
	int width;

	while(SplitLine(unused, unused, width, index, text, len, 0, limit_width))
	{
		if(width > size.x)
			size.x = width;
		size.y += height;
	}

	return size;
}

//=================================================================================================
INT2 Font::CalculateSizeWrap(StringOrCstring str, const INT2& max_size, int border) const
{
	int max_width = max_size.x - border;
	INT2 size = CalculateSize(str, max_width);
	int lines = size.y / height;
	int line_pts = size.x / height;
	int total_pts = line_pts * lines;

	while(line_pts > 9 + lines)
	{
		++lines;
		line_pts = total_pts / lines;
	}

	return CalculateSize(str, line_pts * height);
}

//=================================================================================================
bool Font::ParseGroupIndex(cstring text, uint line_end, uint& i, int& index, int& index2)
{
	index = -1;
	index2 = -1;
	LocalString tmp_s;
	bool first = true;
	while(true)
	{
		assert(i < line_end);
		char c = text[i];
		if(c >= '0' && c <= '9')
			tmp_s += c;
		else if(c == ',' && first && !tmp_s.empty())
		{
			first = false;
			index = atoi(tmp_s.c_str());
			tmp_s.clear();
		}
		else if(c == ';' && !tmp_s.empty())
		{
			int new_index = atoi(tmp_s.c_str());
			if(first)
				index = new_index;
			else
				index2 = new_index;
			break;
		}
		else
		{
			// invalid hitbox counter
			assert(0);
			return false;
		}
		++i;
	}

	return true;
}

//=================================================================================================
bool Font::SkipSpecial(uint& in_out_index, cstring text, uint text_end) const
{
	// specjalna opcja
	in_out_index++;
	if(in_out_index < text_end)
	{
		switch(text[in_out_index])
		{
		case '$':
			return false;
		case 'c':
			// pomiñ kolor
			++in_out_index;
			++in_out_index;
			break;
		case 'h':
			// pomiñ hitbox
			++in_out_index;
			++in_out_index;
			break;
		case 'g':
			++in_out_index;
			if(text[in_out_index] == '+')
			{
				++in_out_index;
				int tmp;
				ParseGroupIndex(text, text_end, in_out_index, tmp, tmp);
				++in_out_index;
			}
			else if(text[in_out_index] == '-')
				++in_out_index;
			else
			{
				// unknown option
				assert(0);
				++in_out_index;
			}
			break;
		default:
			// nieznana opcja
			++in_out_index;
			assert(0);
			break;
		}
		return true;
	}
	else
	{
		// uszkodzony format tekstu, olej to
		++in_out_index;
		assert(0);
		return true;
	}
}

//=================================================================================================
bool Font::HitTest(StringOrCstring str, int limit_width, int flags, const INT2& pos, uint& index, INT2& index2, IBOX2D& rect, float& uv, const vector<FontLine>* font_lines) const
{
	if(pos.x < 0 || pos.y < 0)
		return false;

	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	uint text_end = str.length();
	cstring text = str.c_str();
	int width = 0, prev_width = 0;
	index = 0;

	// simple single line mode
	if(IS_SET(flags, DT_SINGLELINE))
	{
		if(pos.y > height)
			return false;

		while(index < text_end)
		{
			char c = text[index];
			if(c == '$' && parse_special)
			{
				if(SkipSpecial(index, text, text_end))
					continue;
			}
			else
				++index;
			prev_width = width;
			width += GetCharWidth(c);
			if(width >= pos.x)
				break;
		}
		--index;
		rect.p1 = INT2(prev_width, 0);
		rect.p2 = INT2(width, height);
		uv = min(1.f, 1.f - float(width - pos.x) / (width - prev_width));
		index2 = INT2(index, 0);
		return true;
	}

	// get correct line
	int line = pos.y / height;
	uint line_begin, line_end;
	if(font_lines)
	{
		line = min(font_lines->size() - 1, (uint)line);
		auto& font_line = font_lines->at(line);
		line_begin = font_line.begin;
		line_end = font_line.end;
	}
	else
	{
		int current_line = 0;
		do
		{
			if(!SplitLine(line_begin, line_end, width, index, text, text_end, flags, limit_width))
			{
				line = current_line - 1;
				break;
			}
			if(current_line == line)
				break;
			++current_line;
		} while(true);
	}
	
	index = line_begin;
	width = 0;
	while(index < line_end)
	{
		char c = text[index];
		if(c == '$' && parse_special)
		{
			if(SkipSpecial(index, text, line_end))
				continue;
		}
		else
			++index;
		prev_width = width;
		width += GetCharWidth(c);
		if(width >= pos.x)
			break;
	}
	if(index > 0)
		--index;
	rect.p1 = INT2(prev_width, line * height);
	rect.p2 = INT2(width, (line + 1) * height);
	uv = min(1.f, 1.f - float(width - pos.x) / (width - prev_width));
	index2 = INT2(index - line_begin, line);
	return true;
}

//=================================================================================================
INT2 Font::IndexToPos(uint expected_index, StringOrCstring str, int limit_width, int flags) const
{
	assert(expected_index <= str.length());

	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	uint text_end = str.length();
	cstring text = str.c_str();
	uint index = 0;

	if(IS_SET(flags, DT_SINGLELINE))
	{
		int width = 0;

		while(index < text_end && index != expected_index)
		{
			char c = text[index];
			if(c == '$' && parse_special)
			{
				if(SkipSpecial(index, text, text_end))
					continue;
			}
			else
				++index;

			int char_width = GetCharWidth(c);
			width += char_width;
		}

		return INT2(width, 0);
	}

	uint line_begin, line_end;
	uint line = 0;
	while(true)
	{
		int line_width;
		bool ok = SplitLine(line_begin, line_end, line_width, index, text, text_end, flags, limit_width);
		assert(ok);
		if(expected_index >= line_begin && expected_index <= line_end)
		{
			int width = 0;
			index = line_begin;
			while(index < line_end && index != expected_index)
			{
				char c = text[index];
				if(c == '$' && parse_special)
				{
					if(SkipSpecial(index, text, text_end))
						continue;
				}
				else
					++index;

				int char_width = GetCharWidth(c);
				width += char_width;
			}

			return INT2(width, line * height);
		}
		++line;
	}
}

//=================================================================================================
INT2 Font::IndexToPos(const INT2& expected_index, StringOrCstring str, int limit_width, int flags) const
{
	assert(expected_index.x >= 0 && expected_index.y >= 0);

	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	uint text_end = str.length();
	cstring text = str.c_str();
	uint index = 0;

	if(IS_SET(flags, DT_SINGLELINE))
	{
		assert(expected_index.x <= (int)str.length());
		assert(expected_index.y == 0);

		int width = 0;

		while(index < text_end && index != expected_index.x)
		{
			char c = text[index];
			if(c == '$' && parse_special)
			{
				if(SkipSpecial(index, text, text_end))
					continue;
			}
			else
				++index;

			int char_width = GetCharWidth(c);
			width += char_width;
		}

		return INT2(width, 0);
	}

	uint line_begin, line_end;
	uint line = 0;
	while(true)
	{
		int line_width;
		bool ok = SplitLine(line_begin, line_end, line_width, index, text, text_end, flags, limit_width);
		assert(ok);
		if(line == expected_index.y)
		{
			assert((uint)expected_index.x < line_end - line_begin);
			int width = 0;
			index = line_begin;
			while(index < line_end && index - line_begin != expected_index.x)
			{
				char c = text[index];
				if(c == '$' && parse_special)
				{
					if(SkipSpecial(index, text, text_end))
						continue;
				}
				else
					++index;

				int char_width = GetCharWidth(c);
				width += char_width;
			}

			return INT2(width, line * height);
		}
		++line;
	}
}

//=================================================================================================
uint Font::PrecalculateFontLines(vector<FontLine>& font_lines, StringOrCstring str, int limit_width, int flags) const
{
	font_lines.clear();
	
	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	uint text_end = str.length();
	cstring text = str.c_str();
	uint index = 0;

	if(IS_SET(flags, DT_SINGLELINE))
	{
		uint width = GetLineWidth(text, 0, text_end, parse_special);
		font_lines.push_back({ 0, text_end, text_end, width });
		return width;
	}
	else
	{
		uint line_begin, line_end = 0, max_width = 0;
		int line_width;
		while(SplitLine(line_begin, line_end, line_width, index, text, text_end, flags, limit_width))
		{
			font_lines.push_back({ line_begin, line_end, line_end - line_begin, (uint)line_width });
			if(line_width > (int)max_width)
				max_width = line_width;
		}
		if(font_lines.empty())
			font_lines.push_back({ 0, 0, 0, 0 });
		else if(font_lines.back().end != text_end)
			font_lines.push_back({ text_end, text_end, 0, 0 });
			
		return max_width;
	}
}

//=================================================================================================
uint Font::GetLineWidth(cstring text, uint line_begin, uint line_end, bool parse_special) const
{
	uint width = 0;
	uint index = line_begin;

	while(index < line_end)
	{
		char c = text[index];
		if(c == '$' && parse_special)
		{
			if(SkipSpecial(index, text, line_end))
				continue;
		}
		else
			++index;

		int char_width = GetCharWidth(c);
		width += char_width;
	}

	return width;
}

//=================================================================================================
INT2 Font::IndexToPos(vector<FontLine>& font_lines, const INT2& expected_index, StringOrCstring str, int limit_width, int flags) const
{
	assert(expected_index.x >= 0 && expected_index.y >= 0);

	bool parse_special = IS_SET(flags, DT_PARSE_SPECIAL);
	uint text_end = str.length();
	cstring text = str.c_str();
	uint index = 0;
	int width = 0;

	if(IS_SET(flags, DT_SINGLELINE))
	{
		assert(expected_index.x <= (int)str.length());
		assert(expected_index.y == 0);

		while(index < text_end && index != expected_index.x)
		{
			char c = text[index];
			if(c == '$' && parse_special)
			{
				if(SkipSpecial(index, text, text_end))
					continue;
			}
			else
				++index;

			int char_width = GetCharWidth(c);
			width += char_width;
		}

		return INT2(width, 0);
	}

	assert((uint)expected_index.y < font_lines.size());
	auto& line = font_lines[expected_index.y];
	assert((uint)expected_index.x <= line.count);

	index = line.begin;
	while(index < line.end && index - line.begin != expected_index.x)
	{
		char c = text[index];
		if(c == '$' && parse_special)
		{
			if(SkipSpecial(index, text, text_end))
				continue;
		}
		else
			++index;

		int char_width = GetCharWidth(c);
		width += char_width;
	}

	return INT2(width, expected_index.y * height);
}

//=================================================================================================
uint Font::ToRawIndex(vector<FontLine>& font_lines, const INT2& index) const
{
	assert(index.x >= 0 && index.y >= 0 && index.y < (int)font_lines.size());
	auto& line = font_lines[index.y];
	assert(index.x <= (int)line.count);
	return line.begin + index.x;
}

//=================================================================================================
INT2 Font::FromRawIndex(vector<FontLine>& font_lines, uint index) const
{
	uint line_index = 0;
	for(auto& line : font_lines)
	{
		if(index <= line.end)
			return INT2(index - line.begin, line_index);
		++line_index;
	}

	assert(0);
	return INT2(0, 0);
}
