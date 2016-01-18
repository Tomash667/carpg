#include "Pch.h"
#include "Base.h"
#include "Gui2.h"
#include "Container.h"
#include "Dialog2.h"
#include "Language.h"
#include "Game.h"

//-----------------------------------------------------------------------------
IGUI GUI;
TEX IGUI::tBox, IGUI::tBox2, IGUI::tPix, IGUI::tDown;
bool ParseGroupIndex(cstring Text, size_t LineEnd, size_t& i, int& index, int& index2);

//=================================================================================================
IGUI::IGUI() : default_font(nullptr), tFontTarget(nullptr), vb(nullptr), vb2(nullptr), cursor_mode(CURSOR_NORMAL), vb2_locked(false), focused_ctrl(nullptr)
{
}

//=================================================================================================
IGUI::~IGUI()
{
	DeleteElements(created_dialogs);
}

//=================================================================================================
void IGUI::Init(IDirect3DDevice9* _device, ID3DXSprite* _sprite)
{
	device = _device;
	sprite = _sprite;
	tFontTarget = nullptr;

	CreateVertexBuffer();

	color_table[1] = VEC4(1,0,0,1);
	color_table[2] = VEC4(0,1,0,1);
	color_table[3] = VEC4(1,1,0,1);
	color_table[4] = VEC4(1,1,1,1);
	color_table[5] = VEC4(0,0,0,1);

	D3DXMatrixIdentity(&mIdentity);

	layer = new Container;
	layer->auto_focus = true;
	dialog_layer = new Container;
	dialog_layer->focus_top = true;
}

//=================================================================================================
void IGUI::SetText()
{
	txOk = Str("ok");
	txYes = Str("yes");
	txNo = Str("no");
	txCancel = Str("cancel");
}

//=================================================================================================
void IGUI::SetShader(ID3DXEffect* e)
{
	assert(e);

	eGui = e;
	techGui = eGui->GetTechniqueByName("gui");
	techGui2 = eGui->GetTechniqueByName("gui2");
	hGuiSize = eGui->GetParameterByName(nullptr, "size");
	hGuiTex = eGui->GetParameterByName(nullptr, "tex0");
	assert(techGui && techGui2 && hGuiSize && hGuiTex);
}

//=================================================================================================
Font* IGUI::CreateFont(cstring name, int size, int weight, int tex_size, int outline)
{
	assert(name && size > 0 && is_pow2(tex_size) && outline >= 0);

	// oblicz rozmiar czcionki
	HDC hdc = GetDC(nullptr);
	//							hack na skalowanie dpi
	int logic_size = -MulDiv(size, 96/*GetDeviceCaps(hdc, LOGPIXELSX)*/, 72);

	// stwórz czcionkê directx
	FONT dx_font;
	HRESULT hr = D3DXCreateFont(device, logic_size, 0, weight, 0, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, DEFAULT_QUALITY, PROOF_QUALITY | FF_DONTCARE, name, &dx_font);
	if(FAILED(hr))
	{
		ReleaseDC(nullptr, hdc);
		ERROR(Format("Failed to create directx font (%s, size:%d, weight:%d, code:%d).", name, size, weight, hr));
		return nullptr;
	}

	// stwórz czcionkê winapi
	HFONT font = ::CreateFontA(logic_size, 0, 0, 0, weight, false, false, false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_DONTCARE, name);
	if(!font)
	{
		DWORD error = GetLastError();
		ReleaseDC(nullptr, hdc);
		dx_font->Release();
		ERROR(Format("Failed to create font (%s, size:%d, weight:%d, code:%d).", name, size, weight, error));
		return nullptr;
	}

	// pobierz szerokoœci znaków i wysokoœæ czcionki
	int glyph_w[256];
	HGDIOBJ prev = SelectObject(hdc, (HGDIOBJ)font);
	if(GetCharWidth32(hdc, 0, 255, glyph_w) == 0)
	{
		ABC abc[256];
		if(GetCharABCWidths(hdc, 0, 255, abc) == 0)
		{
			ERROR(Format("B³¹d pobierania szerokoœci znaków (%s, rozmiar:%d, waga:%d, b³¹d:%d).", name, size, weight, GetLastError()));
			SelectObject(hdc, prev);
			DeleteObject(font);
			ReleaseDC(nullptr, hdc);
			dx_font->Release();
			return nullptr;
		}
		for(int i=0; i<=255; ++i)
		{
			ABC& a = abc[i];
			glyph_w[i] = a.abcA + a.abcB + a.abcC;
		}
	}
	TEXTMETRIC tm;
	GetTextMetricsA(hdc, &tm);
	int height = tm.tmHeight;
	SelectObject(hdc, prev);
	DeleteObject(font);
	ReleaseDC(nullptr, hdc);

	// stwórz render target
	if(!tFontTarget || tex_size > max_tex_size)
	{
		SafeRelease(tFontTarget);
		hr = device->CreateTexture(tex_size, tex_size, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tFontTarget, nullptr);
		if(FAILED(hr))
		{
			ERROR(Format("B³¹d tworzenia tekstury render target czcionki (rozmiar:%d, b³¹d:%d).", tex_size, hr));
			dx_font->Release();
			return nullptr;
		}
		max_tex_size = tex_size;
	}

	// stwórz czcionkê
	Font* f = new Font;
	f->tex = nullptr;
	f->texOutline = nullptr;
	for(int i=0; i<32; ++i)
		f->glyph[i].ok = false;

	int extra = outline+1;

	// ustaw znaki
	INT2 offset(extra,extra);
	bool warn_once = true;

	for(int i=32; i<=255; ++i)
	{
		int sum = glyph_w[i];
		if(sum)
		{
			if(offset.x+sum >= tex_size-3)
			{
				offset.x = extra;
				offset.y += height+extra;
				if(warn_once && offset.y+height > tex_size)
				{
					warn_once = false;
					WARN(Format("Czcionka %s (%d) nie mieœci siê w teksturze %d.", name, size, tex_size));
				}
			}
			Glyph& g = f->glyph[i];
			g.ok = true;
			g.uv.v1 = VEC2(float(offset.x)/tex_size, float(offset.y)/tex_size);
			g.uv.v2 = g.uv.v1 + VEC2(float(sum)/tex_size, float(height)/tex_size);
			g.width = sum;
			offset.x += sum+2+extra;
		}
		else
			f->glyph[i].ok = false;
	}

	// tab
	Glyph& tab = f->glyph['\t'];
	tab.ok = true;
	tab.width = 32;
	tab.uv = f->glyph[' '].uv;

	f->height = height;
	f->outline_shift = float(outline)/tex_size;

	bool error = !CreateFontInternal(f, dx_font, tex_size, 0, outline);
	if(!error && outline)
		error = !CreateFontInternal(f, dx_font, tex_size, outline, outline);

	// zwolnij czcionkê
	SafeRelease(dx_font);

	// przywróæ render target
	SURFACE surf;
	V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( device->SetRenderTarget(0, surf) );
	surf->Release();

	if(!error)
	{
		// zapisz wygenerowan¹ czcionkê do pliku
		/*D3DXSaveTextureToFile(Format("%s-%d.png", name, size), D3DXIFF_PNG, f->tex, nullptr);
		if(outline > 0)
			D3DXSaveTextureToFile(Format("%s-%d-outline.png", name, size), D3DXIFF_PNG, f->texOutline, nullptr);*/

		fonts.push_back(f);
		return f;
	}
	else
	{
		if(f->tex)
			f->tex->Release();
		delete f;
		return nullptr;
	}
}

//=================================================================================================
bool IGUI::CreateFontInternal(Font* font, ID3DXFont* dx_font, int tex_size, int outline, int max_outline)
{
	// rozpocznij renderowanie do tekstury
	SURFACE surf;
	V( tFontTarget->GetSurfaceLevel(0, &surf) );
	V( device->SetRenderTarget(0, surf) );
	V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 0, 0) );
	V( device->BeginScene() );
	V( sprite->Begin(D3DXSPRITE_ALPHABLEND) );

	int extra = max_outline+1;

	// renderuj do tekstury
	INT2 offset(extra,extra);
	char cbuf[2] = {0,0};
	RECT rect = {0};

	if(outline)
	{
		for(int i=32; i<=255; ++i)
		{
			cbuf[0] = (char)i;
			const Glyph& g = font->glyph[i];
			if(g.ok)
			{
				if(offset.x+g.width >= tex_size-3)
				{
					offset.x = extra;
					offset.y += font->height+extra;
				}

				for(int j=0; j<8; ++j)
				{
					const float a = float(j)*PI/4;
					rect.left = offset.x + int(outline*sin(a));
					rect.top = offset.y + int(outline*cos(a));
					dx_font->DrawTextA(sprite, cbuf, 1, &rect, DT_LEFT|DT_NOCLIP, WHITE);
				}

				offset.x += g.width+2+extra;
			}
		}
	}
	else
	{
		for(int i=32; i<=255; ++i)
		{
			cbuf[0] = (char)i;
			const Glyph& g = font->glyph[i];
			if(g.ok)
			{
				if(offset.x+g.width >= tex_size-3)
				{
					offset.x = extra;
					offset.y += font->height+extra;
				}
				rect.left = offset.x;
				rect.top = offset.y;
				dx_font->DrawTextA(sprite, cbuf, 1, &rect, DT_LEFT|DT_NOCLIP, WHITE);
				offset.x += g.width+2+extra;
			}
		}
	}

	// koniec renderowania
	V( sprite->End() );
	V( device->EndScene() );

	TEX tex;

	// stwórz teksturê na now¹ czcionkê
	HRESULT hr = device->CreateTexture(tex_size, tex_size, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr);
	if(FAILED(hr))
	{
		ERROR(Format("B³¹d tworzenia tekstury czcionki (rozmiar: %d, b³¹d: %d).", tex_size, hr));
		return false;
	}

	// kopiuj do nowej tekstury
	SURFACE out_surf;
	V( tex->GetSurfaceLevel(0, &out_surf) );
	V( D3DXLoadSurfaceFromSurface(out_surf, nullptr, nullptr, surf, nullptr, nullptr, D3DX_DEFAULT, 0) );
	surf->Release();
	out_surf->Release();

	if(outline)
		font->texOutline = tex;
	else
		font->tex = tex;

	return true;
}

//=================================================================================================
/*
Kod przepisany z TFQ
Obs³ugiwane flagi tekstu to:
DT_SINGLELINE
DT_LEFT, DT_CENTER, DT_RIGHT,
DT_TOP, DT_VCENTER, DT_BOTTOM
*/
bool IGUI::DrawText(Font* font, StringOrCstring text, DWORD flags, DWORD color, const RECT& rect, const RECT* clipping, vector<Hitbox>* hitboxes, int* hitbox_counter,
					const vector<TextLine>* lines)
{
	assert(font);

	size_t LineBegin, LineEnd, LineIndex=0;
	int LineWidth, Width = abs(rect.right-rect.left);
	cstring Text = text.c_str();
	size_t TextEnd = text.length();
	VEC4 current_color(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);
	VEC4 default_color = current_color;
	outline_alpha = current_color.w;

	bool outline = (IS_SET(flags, DT_OUTLINE) && font->texOutline);

	tCurrent = font->tex;
	if(outline)
		tCurrent2 = font->texOutline;

	HitboxContext* hc;

	if(hitboxes)
	{
		hc = &tmpHitboxContext;
		hc->hitbox = hitboxes;
		hc->counter = (hitbox_counter ? *hitbox_counter : 0);
		hc->open = HitboxOpen::No;
	}
	else
		hc = nullptr;

	Lock(outline);

	typedef void (IGUI::*DrawLineF)(Font* font, cstring text, size_t LineBegin, size_t LineEnd, const VEC4& def_color, VEC4& color, int x, int y, const RECT* clipping, HitboxContext* hc);
	DrawLineF call;
	if(outline)
		call = &IGUI::DrawLineOutline;
	else
		call = &IGUI::DrawLine;

#define CALL (this->*call)

	bool bottom_clip = false;

	if(!IS_SET(flags, DT_VCENTER|DT_BOTTOM))
	{
		int y = rect.top;

		if(!lines)
		{
			// tekst pionowo po œrodku lub na dole
			while(font->SplitLine(LineBegin, LineEnd, LineWidth, LineIndex, Text, TextEnd, flags, Width))
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IS_SET(flags, DT_CENTER))
					x = rect.left + (Width-LineWidth)/2;
				else if(IS_SET(flags, DT_RIGHT))
					x = rect.right - LineWidth;
				else
					x = rect.left;

				int clip_result = (clipping ? Clip(x, y, LineWidth, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					CALL(font, Text, LineBegin, LineEnd, default_color, current_color, x, y, nullptr, hc);
				else if(clip_result == 5)
					CALL(font, Text, LineBegin, LineEnd, default_color, current_color, x, y, clipping, hc);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(Text, LineBegin, LineEnd, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
		else
		{
			for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IS_SET(flags, DT_CENTER))
					x = rect.left + (Width-it->width)/2;
				else if(IS_SET(flags, DT_RIGHT))
					x = rect.right - it->width;
				else
					x = rect.left;

				int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					CALL(font, Text, it->begin, it->end, default_color, current_color, x, y, nullptr, hc);
				else if(clip_result == 5)
					CALL(font, Text, it->begin, it->end, default_color, current_color, x, y, clipping, hc);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(Text, it->begin, it->end, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
	}
	else
	{
		// tekst u góry
		if(!lines)
		{
			static vector<TextLine> lines_data;
			lines_data.clear();

			// oblicz wszystkie linijki
			while(font->SplitLine(LineBegin, LineEnd, LineWidth, LineIndex, Text, TextEnd, flags, Width))
				lines_data.push_back(TextLine(LineBegin, LineEnd, LineWidth));

			lines = &lines_data;
		}

		// pocz¹tkowa pozycja y
		int y;
		if(IS_SET(flags, DT_BOTTOM))
			y = rect.bottom - lines->size()*font->height;
		else
			y = rect.top + (rect.bottom-rect.top-int(lines->size())*font->height)/2;

		for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
		{
			// pocz¹tkowa pozycja x w tej linijce
			int x;
			if(IS_SET(flags, DT_CENTER))
				x = rect.left + (Width-it->width)/2;
			else if(IS_SET(flags, DT_RIGHT))
				x = rect.right - it->width;
			else
				x = rect.left;

			int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

			// znaki w tej linijce
			if(clip_result == 0)
				CALL(font, Text, it->begin, it->end, default_color, current_color, x, y, nullptr, hc);
			else if(clip_result == 5)
				CALL(font, Text, it->begin, it->end, default_color, current_color, x, y, clipping, hc);
			else if(clip_result == 2)
			{
				// tekst jest pod widocznym regionem, przerwij rysowanie
				bottom_clip = true;
				break;
			}
			else if(hitboxes)
			{
				// pomiñ hitbox
				SkipLine(Text, it->begin, it->end, hc);
			}

			// zmieñ y na kolejn¹ linijkê
			y += font->height;
		}
	}

	Flush();

	if(hitbox_counter)
		*hitbox_counter = hc->counter;

	return !bottom_clip;
}

//=================================================================================================
inline VEC3 VEC32(const VEC2& v)
{
	return VEC3(v.x, v.y, 0);
}

//=================================================================================================
void IGUI::DrawLine(Font* font, cstring Text, size_t LineBegin, size_t LineEnd, const VEC4& default_color, VEC4& current_color, int x, int y, const RECT* clipping, HitboxContext* hc)
{
	for(size_t i=LineBegin; i<LineEnd; ++i)
	{
		char c = Text[i];
		if(c == '$')
		{
			++i;
			assert(i < LineEnd);
			c = Text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < LineEnd);
				c = Text[i];
				switch(c)
				{
				case '-':
					current_color = default_color;
					break;
				case 'r':
					current_color = VEC4(1,0,0,default_color.w);
					break;
				case 'g':
					current_color = VEC4(0,1,0,default_color.w);
					break;
				case 'b':
					current_color = VEC4(0, 0, 1, default_color.w);
					break;
				case 'y':
					current_color = VEC4(1,1,0,default_color.w);
					break;
				case 'w':
					current_color = VEC4(1,1,1,default_color.w);
					break;
				case 'k':
					current_color = VEC4(0,0,0,default_color.w);
					break;
				case '0':
					current_color = VEC4(0.5f, 1, 0, default_color.w);
					break;
				case '1':
					current_color = VEC4(1, 0.5f, 0, default_color.w);
					break;
				default:
					// nieznany kolor
					assert(0);
					break;
				}

				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < LineEnd);
				c = Text[i];
				if(c == '+')
				{
					// rozpocznij hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::No);
						hc->open = HitboxOpen::Yes;
						hc->region.left = INT_MAX;
					}
				}
				else if(c == '-')
				{
					// zamknij hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::Yes);
						hc->open = HitboxOpen::No;
						if(hc->region.left != INT_MAX)
						{
							Hitbox& h = Add1(hc->hitbox);
							h.rect = hc->region;
							h.index = hc->counter;
							h.index2 = -1;
						}
						++hc->counter;
					}
				}
				else
				{
					// nieznana opcja hitboxa
					assert(0);
				}

				continue;
			}
			else if(c == 'g')
			{
				// group hitbox
				// $g+123[,123];
				// $g-
				++i;
				assert(i < LineEnd);
				c = Text[i];
				if(c == '+')
				{
					// start group hitbox
					int index, index2;
					++i;
					assert(i < LineEnd);
					if(ParseGroupIndex(Text, LineEnd, i, index, index2) && hc)
					{
						assert(hc->open == HitboxOpen::No);
						hc->open = HitboxOpen::Group;
						hc->region.left = INT_MAX;
						hc->group_index = index;
						hc->group_index2 = index2;
					}
				}
				else if(c == '-')
				{
					// close group hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::Group);
						hc->open = HitboxOpen::No;
						if(hc->region.left != INT_MAX)
						{
							Hitbox& h = Add1(hc->hitbox);
							h.rect = hc->region;
							h.index = hc->group_index;
							h.index2 = hc->group_index2;
						}
					}
				}
				else
				{
					// invalid format
					assert(0);
				}

				continue;
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		Glyph& g = font->glyph[byte(c)];

		int clip_result = (clipping ? Clip(x, y, g.width, font->height, clipping) : 0);
		
		if(clip_result == 0)
		{
			// dodaj znak do bufora
			v->pos = VEC3(float(x),float(y), 0);
			v->color = current_color;
			v->tex = g.uv.LeftTop();
			++v;

			v->pos = VEC3(float(x+g.width),float(y), 0);
			v->color = current_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = VEC3(float(x),float(y+font->height), 0);
			v->color = current_color;
			v->tex = g.uv.LeftBottom();
			++v;

			v->pos = VEC3(float(x+g.width),float(y), 0);
			v->color = current_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = VEC3(float(x+g.width),float(y+font->height), 0);
			v->color = current_color;
			v->tex = g.uv.RightBottom();
			++v;

			v->pos = VEC3(float(x),float(y+font->height), 0);
			v->color = current_color;
			v->tex = g.uv.LeftBottom();
			++v;

			if(hc && hc->open != HitboxOpen::No)
			{
				if(hc->region.left == INT_MAX)
				{
					hc->region.left = x;
					hc->region.right = x+g.width;
					hc->region.top = y;
					hc->region.bottom = y+font->height;
				}
				else
				{
					if(x < hc->region.left)
						hc->region.left = x;
					if(x+g.width > hc->region.right)
						hc->region.right = x+g.width;
					if(y < hc->region.top)
						hc->region.top = y;
					if(y+font->height > hc->region.bottom)
						hc->region.bottom = y+font->height;
				}
			}

			++in_buffer;
			x += g.width;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			BOX2D orig_pos(float(x), float(y), float(x+g.width), float(y+font->height));
			BOX2D clip_pos(float(max(x, (int)clipping->left)), float(max(y, (int)clipping->top)), float(min(x+g.width, (int)clipping->right)), float(min(y+font->height, (int)clipping->bottom)));
			VEC2 orig_size = orig_pos.Size();
			VEC2 clip_size = clip_pos.Size();
			VEC2 s(clip_size.x/orig_size.x, clip_size.y/orig_size.y);
			VEC2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			VEC2 uv_size = g.uv.Size();
			BOX2D clip_uv(g.uv.v1+VEC2(shift.x*uv_size.x, shift.y*uv_size.y));
			clip_uv.v2 += VEC2(uv_size.x*s.x, uv_size.y*s.y);

			// dodaj znak do bufora
			v->pos = VEC32(clip_pos.LeftTop());
			v->color = current_color;
			v->tex = clip_uv.LeftTop();
			++v;

			v->pos = VEC32(clip_pos.RightTop());
			v->color = current_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = VEC32(clip_pos.LeftBottom());
			v->color = current_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			v->pos = VEC32(clip_pos.RightTop());
			v->color = current_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = VEC32(clip_pos.RightBottom());
			v->color = current_color;
			v->tex = clip_uv.RightBottom();
			++v;

			v->pos = VEC32(clip_pos.LeftBottom());
			v->color = current_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			if(hc && hc->open != HitboxOpen::No)
			{
				int a = (int)clip_pos.v1.x,
					b = (int)clip_pos.v2.x,
					c = (int)clip_pos.v1.y,
					d = (int)clip_pos.v2.y;
				if(hc->region.left == INT_MAX)
				{
					hc->region.left = a;
					hc->region.right = b;
					hc->region.top = c;
					hc->region.bottom = d;
				}
				else
				{
					if(a < hc->region.left)
						hc->region.left = a;
					if(b > hc->region.right)
						hc->region.right = b;
					if(c < hc->region.top)
						hc->region.top = c;
					if(d > hc->region.bottom)
						hc->region.bottom = d;
				}
			}

			++in_buffer;
			x += g.width;
		}
		else if(clip_result == 3)
		{
			// tekst jest ju¿ poza regionem z prawej, mo¿na przerwaæ
			break;
		}
		else
			x += g.width;

		if(in_buffer == 256)
			Flush(true);
	}

	// zamknij region
	if(hc && hc->open != HitboxOpen::No && hc->region.left != INT_MAX)
	{
		Hitbox& h = Add1(hc->hitbox);
		h.rect = hc->region;
		if(hc->open == HitboxOpen::Yes)
			h.index = hc->counter;
		else
		{
			h.index = hc->group_index;
			h.index = hc->group_index2;
		}
		hc->region.left = INT_MAX;
	}
}

//=================================================================================================
void IGUI::DrawLineOutline(Font* font, cstring Text, size_t LineBegin, size_t LineEnd, const VEC4& default_color, VEC4& current_color, int x, int y, const RECT* clipping, HitboxContext* hc)
{
	VEC4 col(0,0,0,outline_alpha);
	int prev_x = x, prev_y = y;

	// przesuniêcie glifu przez outline
	const float osh = font->outline_shift;

	for(size_t i=LineBegin; i<LineEnd; ++i)
	{
		char c = Text[i];
		if(c == '$')
		{
			++i;
			assert(i < LineEnd);
			c = Text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < LineEnd);
				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < LineEnd);
				continue;
			}
			else if(c == 'g')
			{
				++i;
				assert(i < LineEnd);
				c = Text[i];
				if(c == '+')
				{
					++i;
					assert(i < LineEnd);
					int tmp;
					ParseGroupIndex(Text, LineEnd, i, tmp, tmp);
				}
				else if(c == '-')
					continue;
				else
				{
					// invalid group format
					assert(0);
				}
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		Glyph& g = font->glyph[byte(c)];

		int clip_result = (clipping ? Clip(x, y, g.width, font->height, clipping) : 0);

		if(clip_result == 0)
		{
			// dodaj znak do bufora
			v2->pos = VEC3(float(x)-osh,float(y)-osh, 0);
			v2->color = col;
			v2->tex = g.uv.LeftTop();
			++v2;

			v2->pos = VEC3(float(x+g.width)+osh,float(y)-osh, 0);
			v2->color = col;
			v2->tex = g.uv.RightTop();
			++v2;

			v2->pos = VEC3(float(x)-osh,float(y+font->height)+osh, 0);
			v2->color = col;
			v2->tex = g.uv.LeftBottom();
			++v2;

			v2->pos = VEC3(float(x+g.width)+osh,float(y)-osh, 0);
			v2->color = col;
			v2->tex = g.uv.RightTop();
			++v2;

			v2->pos = VEC3(float(x+g.width)+osh,float(y+font->height)+osh, 0);
			v2->color = col;
			v2->tex = g.uv.RightBottom();
			++v2;

			v2->pos = VEC3(float(x)-osh,float(y+font->height)+osh, 0);
			v2->color = col;
			v2->tex = g.uv.LeftBottom();
			++v2;

			if(hc && hc->open != HitboxOpen::No)
			{
				if(hc->region.left == INT_MAX)
				{
					hc->region.left = x;
					hc->region.right = x+g.width;
					hc->region.top = y;
					hc->region.bottom = y+font->height;
				}
				else
				{
					if(x < hc->region.left)
						hc->region.left = x;
					if(x+g.width > hc->region.right)
						hc->region.right = x+g.width;
					if(y < hc->region.top)
						hc->region.top = y;
					if(y+font->height > hc->region.bottom)
						hc->region.bottom = y+font->height;
				}
			}

			++in_buffer2;
			x += g.width;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			BOX2D orig_pos(float(x)-osh, float(y)-osh, float(x+g.width)+osh, float(y+font->height)+osh);
			BOX2D clip_pos(float(max(x, (int)clipping->left)), float(max(y, (int)clipping->top)), float(min(x+g.width, (int)clipping->right)), float(min(y+font->height, (int)clipping->bottom)));
			VEC2 orig_size = orig_pos.Size();
			VEC2 clip_size = clip_pos.Size();
			VEC2 s(clip_size.x/orig_size.x, clip_size.y/orig_size.y);
			VEC2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			VEC2 uv_size = g.uv.Size();
			BOX2D clip_uv(g.uv.v1+VEC2(shift.x*uv_size.x, shift.y*uv_size.y));
			clip_uv.v2 += VEC2(uv_size.x*s.x, uv_size.y*s.y);

			// dodaj znak do bufora
			v2->pos = VEC32(clip_pos.LeftTop());
			v2->color = col;
			v2->tex = clip_uv.LeftTop();
			++v2;

			v2->pos = VEC32(clip_pos.RightTop());
			v2->color = col;
			v2->tex = clip_uv.RightTop();
			++v2;

			v2->pos = VEC32(clip_pos.LeftBottom());
			v2->color = col;
			v2->tex = clip_uv.LeftBottom();
			++v2;

			v2->pos = VEC32(clip_pos.RightTop());
			v2->color = col;
			v2->tex = clip_uv.RightTop();
			++v2;

			v2->pos = VEC32(clip_pos.RightBottom());
			v2->color = col;
			v2->tex = clip_uv.RightBottom();
			++v2;

			v2->pos = VEC32(clip_pos.LeftBottom());
			v2->color = col;
			v2->tex = clip_uv.LeftBottom();
			++v2;

			if(hc && hc->open != HitboxOpen::No)
			{
				int a = (int)clip_pos.v1.x,
					b = (int)clip_pos.v2.x,
					c = (int)clip_pos.v1.y,
					d = (int)clip_pos.v2.y;
				if(hc->region.left == INT_MAX)
				{
					hc->region.left = a;
					hc->region.right = b;
					hc->region.top = c;
					hc->region.bottom = d;
				}
				else
				{
					if(a < hc->region.left)
						hc->region.left = a;
					if(b > hc->region.right)
						hc->region.right = b;
					if(c < hc->region.top)
						hc->region.top = c;
					if(d > hc->region.bottom)
						hc->region.bottom = d;
				}
			}

			++in_buffer2;
			x += g.width;
		}
		else if(clip_result == 3)
		{
			// tekst jest ju¿ poza regionem z prawej, mo¿na przerwaæ
			break;
		}
		else
			x += g.width;

		if(in_buffer2 == 256)
			Flush(true);
	}

	DrawLine(font, Text, LineBegin, LineEnd, default_color, current_color, prev_x, prev_y, clipping, hc);
}

//=================================================================================================
void IGUI::Lock(bool outline)
{
	V( vb->Lock(0, 0, (void**)&v, D3DLOCK_DISCARD) );
	in_buffer = 0;

	if(outline)
	{
		V( vb2->Lock(0, 0, (void**)&v2, D3DLOCK_DISCARD) );
		in_buffer2 = 0;
		vb2_locked = true;
	}
	else
		vb2_locked = false;
}

//=================================================================================================
void IGUI::Flush(bool lock)
{
	if(vb2_locked)
	{
		// odblokuj drugi bufor
		V( vb2->Unlock() );

		// rysuj o ile jest co
		if(in_buffer2)
		{
			// ustaw teksturê
			if(tCurrent2 != tSet)
			{
				tSet = tCurrent2;
				V( eGui->SetTexture(hGuiTex, tSet) );
				V( eGui->CommitChanges() );
			}

			// rysuj
			V( device->SetStreamSource(0, vb2, 0, sizeof(VParticle)) );
			V( device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, in_buffer*2) );
		}
	}

	V( vb->Unlock() );

	if(in_buffer)
	{
		// ustaw teksturê
		if(tCurrent != tSet)
		{
			tSet = tCurrent;
			V( eGui->SetTexture(hGuiTex, tSet) );
			V( eGui->CommitChanges() );
		}

		// rysuj
		V( device->SetStreamSource(0, vb, 0, sizeof(VParticle)) );
		V( device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, in_buffer*2) );
	}

	if(lock)
		Lock(vb2_locked);
	else
		vb2_locked = false;
}

//=================================================================================================
void IGUI::Draw(const INT2& _wnd_size)
{
	PROFILER_BLOCK("DrawGui");

	wnd_size = _wnd_size;

	Game& game = Game::Get();
	if(!IS_SET(game.draw_flags, DF_GUI|DF_MENU))
		return;

	game.SetAlphaTest(false);
	game.SetAlphaBlend(true);
	game.SetNoCulling(true);
	game.SetNoZWrite(false);

	V( device->SetVertexDeclaration(game.vertex_decl[VDI_PARTICLE]) );

	tSet = nullptr;
	tCurrent = nullptr;
	tCurrent2 = nullptr;

	UINT passes;
	
	V( eGui->SetTechnique(techGui) );
	VEC4 wnd_s(float(wnd_size.x), float(wnd_size.y), 0, 0);
	V( eGui->SetVector(hGuiSize, &wnd_s) );
	V( eGui->Begin(&passes, 0) );
	V( eGui->BeginPass(0) );

	// rysowanie
	if(IS_SET(game.draw_flags, DF_GUI))
		layer->Draw();
	if(IS_SET(game.draw_flags, DF_MENU))
		dialog_layer->Draw();

	// kursor
	if(NeedCursor())
		DrawSprite(tCursor[cursor_mode], cursor_pos);
	
	V( eGui->EndPass() );
	V( eGui->End() );
}

//=================================================================================================
void IGUI::Add(Control* ctrl)
{
	layer->Add(ctrl);
}

//=================================================================================================
void IGUI::DrawItem(TEX t, const INT2& item_pos, const INT2& item_size, DWORD color, int corner, int size)
{
	assert(t);

	if(item_size.x < corner || item_size.y < corner)
	{
		if(item_size.x == 0 || item_size.y == 0)
			return;

		RECT r = {item_pos.x, item_pos.y, item_pos.x+item_size.x, item_pos.y+item_size.y};

		DrawSpriteRect(t, r, color);
		return;
	}

	tCurrent = t;
	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	/*
		0---1----------2---3
		|   |          |   |
		|   |          |   |
		4---5----------6---7
		|   |          |   |
		|   |          |   |
		|   |          |   |
		|   |          |   |
		8---9---------10--11
		|   |          |   |
		|   |          |   |
		12-13---------14--15
	*/

	int ids[9*6] = {
		0, 1, 4, 4, 1, 5,
		1, 2, 5, 5, 2, 6,
		2, 3, 6, 6, 3, 7,
		4, 5, 8, 8, 5, 9,
		5, 6, 9, 9, 6, 10,
		6, 7, 10, 10, 7, 11,
		8, 9, 12, 12, 9, 13,
		9, 10, 13, 13, 10, 14,
		10, 11, 14, 14, 11, 15
	};

	VEC2 ipos[16] = {
		VEC2(float(item_pos.x), float(item_pos.y)),
		VEC2(float(item_pos.x+corner), float(item_pos.y)),
		VEC2(float(item_pos.x+item_size.x-corner), float(item_pos.y)),
		VEC2(float(item_pos.x+item_size.x), float(item_pos.y)),

		VEC2(float(item_pos.x), float(item_pos.y+corner)),
		VEC2(float(item_pos.x+corner), float(item_pos.y+corner)),
		VEC2(float(item_pos.x+item_size.x-corner), float(item_pos.y+corner)),
		VEC2(float(item_pos.x+item_size.x), float(item_pos.y+corner)),

		VEC2(float(item_pos.x), float(item_pos.y+item_size.y-corner)),
		VEC2(float(item_pos.x+corner), float(item_pos.y+item_size.y-corner)),
		VEC2(float(item_pos.x+item_size.x-corner), float(item_pos.y+item_size.y-corner)),
		VEC2(float(item_pos.x+item_size.x), float(item_pos.y+item_size.y-corner)),

		VEC2(float(item_pos.x), float(item_pos.y+item_size.y)),
		VEC2(float(item_pos.x+corner), float(item_pos.y+item_size.y)),
		VEC2(float(item_pos.x+item_size.x-corner), float(item_pos.y+item_size.y)),
		VEC2(float(item_pos.x+item_size.x), float(item_pos.y+item_size.y)),
	};

	VEC2 itex[16] = {
		VEC2(0,0),
		VEC2(float(corner)/size,0),
		VEC2(float(size-corner)/size,0),
		VEC2(1,0),

		VEC2(0,float(corner)/size),
		VEC2(float(corner)/size,float(corner)/size),
		VEC2(float(size-corner)/size,float(corner)/size),
		VEC2(1,float(corner)/size),

		VEC2(0,float(size-corner)/size),
		VEC2(float(corner)/size,float(size-corner)/size),
		VEC2(float(size-corner)/size,float(size-corner)/size),
		VEC2(1,float(size-corner)/size),

		VEC2(0,1),
		VEC2(float(corner)/size,1),
		VEC2(float(size-corner)/size,1),
		VEC2(1,1),
	};

	for(int i=0; i<9*6; ++i)
	{
		int id = ids[i];
		v->pos.x = ipos[id].x;
		v->pos.y = ipos[id].y;
		v->pos.z = 0;
		v->color = col;
		v->tex = itex[id];
		++v;
	}

	in_buffer = 9;
	Flush();
}

//=================================================================================================
void IGUI::Update(float dt)
{
	PROFILER_BLOCK("UpdateGui");
	cursor_mode = CURSOR_NORMAL;
	layer->focus = dialog_layer->Empty();
	if(focused_ctrl)
	{
		if(!focused_ctrl->visible)
			focused_ctrl = nullptr;
		else if(dialog_layer->Empty())
		{
			layer->dont_focus = true;
			layer->Update(dt);
			layer->dont_focus = false;
		}
		else
		{
			focused_ctrl->LostFocus();
			focused_ctrl = nullptr;
		}
	}
	if(!focused_ctrl)
	{
		layer->Update(dt);
		dialog_layer->focus = true;
		dialog_layer->Update(dt);
	}
}

//=================================================================================================
void IGUI::DrawSprite(TEX t, const INT2& pos, DWORD color, const RECT* clipping)
{
	assert(t);

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);

	int clip_result = (clipping ? Clip(pos.x, pos.y, desc.Width, desc.Height, clipping) : 0);
	if(clip_result > 0 && clip_result < 5)
		return;

	tCurrent = t;
	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	if(clip_result == 0)
	{
		v->pos = VEC3(float(pos.x),float(pos.y),0);
		v->color = col;
		v->tex = VEC2(0,0);
		++v;

		v->pos = VEC3(float(pos.x+desc.Width),float(pos.y),0);
		v->color = col;
		v->tex = VEC2(1,0);
		++v;

		v->pos = VEC3(float(pos.x),float(pos.y+desc.Height),0);
		v->color = col;
		v->tex = VEC2(0,1);
		++v;

		v->pos = VEC3(float(pos.x),float(pos.y+desc.Height),0);
		v->color = col;
		v->tex = VEC2(0,1);
		++v;

		v->pos = VEC3(float(pos.x+desc.Width),float(pos.y),0);
		v->color = col;
		v->tex = VEC2(1,0);
		++v;

		v->pos = VEC3(float(pos.x+desc.Width),float(pos.y+desc.Height),0);
		v->color = col;
		v->tex = VEC2(1,1);
		++v;

		in_buffer = 1;
		Flush();
	}
	else
	{
		BOX2D orig_pos(float(pos.x), float(pos.y), float(pos.x+desc.Width), float(pos.y+desc.Height));
		BOX2D clip_pos(float(max(pos.x, (int)clipping->left)), float(max(pos.y, (int)clipping->top)),
			float(min(pos.x+(int)desc.Width, (int)clipping->right)), float(min(pos.y+(int)desc.Height, (int)clipping->bottom)));
		VEC2 orig_size = orig_pos.Size();
		VEC2 clip_size = clip_pos.Size();
		VEC2 s(clip_size.x/orig_size.x, clip_size.y/orig_size.y);
		VEC2 shift = clip_pos.v1 - orig_pos.v1;
		shift.x /= orig_size.x;
		shift.y /= orig_size.y;
		BOX2D clip_uv(VEC2(shift.x, shift.y));
		clip_uv.v2 += VEC2(s.x, s.y);

		v->pos = VEC32(clip_pos.LeftTop());
		v->color = col;
		v->tex = clip_uv.LeftTop();
		++v;

		v->pos = VEC32(clip_pos.RightTop());
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = VEC32(clip_pos.LeftBottom());
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos =  VEC32(clip_pos.LeftBottom());
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos = VEC32(clip_pos.RightTop());
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = VEC32(clip_pos.RightBottom());
		v->color = col;
		v->tex = clip_uv.RightBottom();
		++v;

		in_buffer = 1;
		Flush();
	}
}

//=================================================================================================
void IGUI::OnReset()
{
	SafeRelease(vb);
	SafeRelease(vb2);
	SafeRelease(tFontTarget);
}

//=================================================================================================
void IGUI::OnReload()
{
	CreateVertexBuffer();
}

//=================================================================================================
void IGUI::OnClean()
{
	OnReset();

	for(vector<Font*>::iterator it = fonts.begin(), end = fonts.end(); it != end; ++it)
	{
		if((*it)->tex)
			(*it)->tex->Release();
		if((*it)->texOutline)
			(*it)->texOutline->Release();
		delete *it;
	}

	delete layer;
	delete dialog_layer;
}

//=================================================================================================
void IGUI::CreateVertexBuffer()
{
	V( device->CreateVertexBuffer(sizeof(VParticle)*6*256, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr) );
	V( device->CreateVertexBuffer(sizeof(VParticle)*6*256, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb2, nullptr) );
}

//=================================================================================================
/* ukradzione z TFQ, zmodyfikowane pod carpg
	zwraca czy jest jeszcze jakiœ tekst w tej linijce
	parametry:
		OutBegin - pierszy znak w tej linijce
		OutEnd - ostatni znak w tej linijce
		OutWidth - szerokoœæ tej linijki
		InOutIndex - offset w Text
		Text - chyba tekst ale nie jestem pewien :3
		TextEnd - d³ugoœæ tekstu
		Flags - flagi (uwzglêdnia tylko DT_SINGLELINE)
		Width - maksymalna szerokoœæ tej linijki
*/
bool Font::SplitLine(size_t& OutBegin, size_t& OutEnd, int& OutWidth, size_t& InOutIndex, cstring Text, size_t TextEnd, DWORD Flags, int Width) const
{
	// Heh, piszê ten algorytm chyba trzeci raz w ¿yciu.
	// Za ka¿dym razem idzie mi szybciej i lepiej.
	// Ale zawsze i tak jest dla mnie ogromnie trudny i skomplikowany.
	if (InOutIndex >= TextEnd)
		return false;

	OutBegin = InOutIndex;
	OutWidth = 0;

	// Pojedyncza linia - specjalny szybki tryb
	if(IS_SET(Flags, DT_SINGLELINE))
	{
		while (InOutIndex < TextEnd)
			OutWidth += GetCharWidth(Text[InOutIndex++]);
		OutEnd = InOutIndex;

		return true;
	}

	char Ch;
	int CharWidth;
	// Zapamiêtany stan miejsca ostatniego wyst¹pienia spacji
	// Przyda siê na wypadek zawijania wierszy na granicy s³owa.
	size_t LastSpaceIndex = string::npos;
	int WidthWhenLastSpace;

	for (;;)
	{
		// Koniec tekstu
		if (InOutIndex >= TextEnd)
		{
			OutEnd = TextEnd;
			break;
		}

		// Pobierz znak
		Ch = Text[InOutIndex];

		// Koniec wiersza
		if (Ch == '\n')
		{
			OutEnd = InOutIndex;
			InOutIndex++;
			break;
		}
		// Koniec wiersza \r
		else if (Ch == '\r')
		{
			OutEnd = InOutIndex;
			InOutIndex++;
			// Sekwencja \r\n - pomiñ \n
			if (InOutIndex < TextEnd && Text[InOutIndex] == '\n')
				InOutIndex++;
			break;
		}
		else if(Ch == '$')
		{
			// specjalna opcja
			InOutIndex++;
			if(InOutIndex < TextEnd)
			{
				switch(Text[InOutIndex])
				{
				case '$':
					goto znak_$;
				case 'c':
					// pomiñ kolor
					++InOutIndex;
					++InOutIndex;
					break;
				case 'h':
					// pomiñ hitbox
					++InOutIndex;
					++InOutIndex;
					break;
				case 'g':
					++InOutIndex;
					if(Text[InOutIndex] == '+')
					{
						++InOutIndex;
						int tmp;
						ParseGroupIndex(Text, TextEnd, InOutIndex, tmp, tmp);
						++InOutIndex;
					}
					else if(Text[InOutIndex] == '-')
						++InOutIndex;
					else
					{
						// unknown option
						assert(0);
						++InOutIndex;
					}
					break;
				default:
					// nieznana opcja
					++InOutIndex;
					assert(0);
					break;
				}
			}
			else
			{
				// uszkodzony format tekstu, olej to
				++InOutIndex;
				assert(0);
			}
		}
		// Inny znak
		else
		{
znak_$:
			// Szerokoœæ znaku
			CharWidth = GetCharWidth(Text[InOutIndex]);

			// Jeœli nie ma automatycznego zawijania wierszy lub
			// jeœli siê zmieœci lub
			// to jest pierwszy znak (zabezpieczenie przed nieskoñczonym zapêtleniem dla Width < szerokoœæ pierwszego znaku) -
			// dolicz go i ju¿
			if (/*Flags & FLAG_WRAP_NORMAL ||*/ OutWidth + CharWidth <= Width || InOutIndex == OutBegin)
			{
				// Jeœli to spacja - zapamiêtaj dane
				if (Ch == ' ')
				{
					LastSpaceIndex = InOutIndex;
					WidthWhenLastSpace = OutWidth;
				}
				OutWidth += CharWidth;
				InOutIndex++;
			}
			// Jest automatyczne zawijanie wierszy i siê nie mieœci
			else
			{
				// Niemieszcz¹cy siê znak to spacja
				if (Ch == ' ')
				{
					OutEnd = InOutIndex;
					// Mo¿na go przeskoczyæ
					InOutIndex++;
					break;
				}
				// Poprzedni znak za tym to spacja
				else if (InOutIndex > OutBegin && Text[InOutIndex-1] == ' ')
				{
					// Koniec bêdzie na tej spacji
					OutEnd = LastSpaceIndex;
					OutWidth = WidthWhenLastSpace;
					break;
				}

				// Zawijanie wierszy na granicy s³owa
				if (1/*Flags & FLAG_WRAP_WORD*/)
				{
					// By³a jakaœ spacja
					if (LastSpaceIndex != string::npos)
					{
						// Koniec bêdzie na tej spacji
						OutEnd = LastSpaceIndex;
						InOutIndex = LastSpaceIndex+1;
						OutWidth = WidthWhenLastSpace;
						break;
					}
					// Nie by³o spacji - trudno, zawinie siê jak na granicy znaku
				}

				OutEnd = InOutIndex;
				break;
			}
		}
	}

	return true;
}

//=================================================================================================
int Font::LineWidth(cstring str) const
{
	int w = 0;

	while(true)
	{
		char c = *str;
		if(c == 0)
			break;

		if(c == '$')
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

	INT2 size(0,0);

	size_t unused, index=0;
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
/*
Przycinanie tekstu do wybranego regionu, zwraca:
0 - tekst w ca³oœci w regionie
1 - tekst nad regionem
2 - tekst pod regionem
3 - tekst poza regionem z prawej
4 - tekst poza regionem z lewej
5 - wymaga czêœciowego clippingu, czêœciowo w regionie
*/
int IGUI::Clip(int x, int y, int w, int h, const RECT* clipping)
{
	if(x >= clipping->left && y >= clipping->top && x+w < clipping->right && y+h < clipping->bottom)
	{
		// tekst w ca³oœci w regionie
		return 0;
	}
	else if(y+h < clipping->top)
	{
		// tekst nad regionem
		return 1;
	}
	else if(y > clipping->bottom)
	{
		// tekst pod regionem
		return 2;
	}
	else if(x > clipping->right)
	{
		// tekst poza regionem z prawej
		return 3;
	}
	else if(x+w < clipping->left)
	{
		// tekst poza regionem z lewej
		return 4;
	}
	else
	{
		// wymaga czêœciowego clippingu
		return 5;
	}
}

//=================================================================================================
void IGUI::SkipLine(cstring text, size_t LineBegin, size_t LineEnd, HitboxContext* hc)
{
	for(size_t i=LineBegin; i<LineEnd; ++i)
	{
		char c = text[i];
		if(c == '$')
		{
			++i;
			c = text[i];
			if(c == 'h')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Yes;
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Yes);
					hc->open = HitboxOpen::No;
					++hc->counter;
				}
				else
					assert(0);
			}
			else if(c == 'g')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Group;
					int tmp;
					++i;
					ParseGroupIndex(text, LineEnd, i, tmp, tmp);
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Group);
					hc->open = HitboxOpen::No;
				}
				else
					assert(0);
			}
		}
	}
}

//=================================================================================================
Dialog* IGUI::ShowDialog(const DialogInfo& info)
{
	Dialog* d;

	if(info.have_tick)
		d = new DialogWithCheckbox(info);
	else
		d = new Dialog(info);
	created_dialogs.push_back(d);

	// oblicz rozmiar
	d->size = default_font->CalculateSize(info.text) + INT2(24,24);

	int min_size;

	// przyciski
	if(info.type == DIALOG_OK)
	{
		Button& bt = Add1(d->bts);
		bt.text = txOk;
		bt.id = GuiEvent_Custom+BUTTON_OK;
		bt.size = default_font->CalculateSize(bt.text) + INT2(24,24);
		bt.parent = d;

		min_size = bt.size.x+24;
	}
	else
	{
		d->bts.resize(2);
		Button& bt1 = d->bts[0],
			&bt2 = d->bts[1];

		if(info.custom_names)
		{
			bt1.text = (info.custom_names[0] ? info.custom_names[0] : txYes);
			bt2.text = (info.custom_names[1] ? info.custom_names[1] : txNo);
		}
		else
		{
			bt1.text = txYes;
			bt2.text = txNo;
		}

		bt1.id = GuiEvent_Custom+BUTTON_YES;
		bt1.size = default_font->CalculateSize(bt1.text) + INT2(24,24);
		bt1.parent = d;

		bt2.id = GuiEvent_Custom+BUTTON_NO;
		bt2.size = default_font->CalculateSize(bt2.text) + INT2(24,24);
		bt2.parent = d;

		bt1.size = bt2.size = Max(bt1.size, bt2.size);
		min_size = bt1.size.x*2 + 24 + 16;
	}

	// powiêksz rozmiar okna o przyciski
	if(d->size.x < min_size)
		d->size.x = min_size;
	d->size.y += d->bts[0].size.y+8;

	// checkbox
	if(info.have_tick)
	{
		d->size.y += 32;
		DialogWithCheckbox* dwc = (DialogWithCheckbox*)d;
		dwc->checkbox.bt_size = INT2(32,32);
		dwc->checkbox.checked = info.ticked;
		dwc->checkbox.id = GuiEvent_Custom+BUTTON_CHECKED;
		dwc->checkbox.parent = dwc;
		dwc->checkbox.text = info.tick_text;
		dwc->checkbox.pos = INT2(12,40);
		dwc->checkbox.size = INT2(d->size.x-24,32);
	}

	// ustaw przyciski
	if(d->bts.size() == 1)
	{
		Button& bt = d->bts[0];
		bt.pos.x = (d->size.x-bt.size.x)/2;
		bt.pos.y = d->size.y-8-bt.size.y;
	}
	else
	{
		Button& bt1 = d->bts[0],
			&bt2 = d->bts[1];
		bt1.pos.y = bt2.pos.y = d->size.y-8-bt1.size.y;
		bt1.pos.x = 12;
		bt2.pos.x = d->size.x - bt2.size.x - 12;
	}

	// dodaj
	d->need_delete = true;
	ShowDialog(d);

	return d;
}

//=================================================================================================
void IGUI::ShowDialog(Dialog* d)
{
	d->Event(GuiEvent_Show);

	if(dialog_layer->Empty())
	{
		// nie ma ¿adnych innych dialogów, aktywuj
		dialog_layer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else if(d->order == ORDER_TOPMOST)
	{
		// dezaktywuj aktualny i aktywuj nowy
		Control* prev_d = dialog_layer->Top();
		prev_d->focus = false;
		prev_d->Event(GuiEvent_LostFocus);
		dialog_layer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else
	{
		// szukaj pierwszego dialogu który jest wy¿ej ni¿ ten
		DialogOrder above_order = DialogOrder(d->order+1);
		vector<Dialog*>& ctrls = (vector<Dialog*>&)dialog_layer->GetControls();
		vector<Dialog*>::iterator first_above = ctrls.end();
		for(vector<Dialog*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->order >= above_order)
			{
				first_above = it;
				break;
			}
		}

		if(first_above == ctrls.end())
		{
			// brak nadrzêdnego dialogu, dezaktywuj aktualny i aktywuj nowy
			Control* prev_d = dialog_layer->Top();
			prev_d->focus = false;
			prev_d->Event(GuiEvent_LostFocus);
			dialog_layer->Add(d);
			d->focus = true;
			d->Event(GuiEvent_GainFocus);
		}
		else
		{
			// jest nadrzêdny dialog, dodaj przed nim i nie aktywuj
			ctrls.insert(first_above, d);
		}
	}

	dialog_layer->inside_loop = false;
}

//=================================================================================================
bool IGUI::CloseDialog(Dialog* d)
{
	assert(d);

	if(dialog_layer->Empty())
		return false;

	Control* prev_top = dialog_layer->Top();
	CloseDialogInternal(d);
	if(!dialog_layer->Empty() && prev_top != dialog_layer->Top())
	{
		Control* next_d = dialog_layer->Top();
		next_d->focus = true;
		next_d->Event(GuiEvent_GainFocus);
	}

	return true;
}

//=================================================================================================
void IGUI::CloseDialogInternal(Dialog* d)
{
	assert(d);

	d->Event(GuiEvent_Close);
	dialog_layer->Remove(d);

	if(!dialog_layer->Empty())
	{
		vector<Dialog*>& dialogs = (vector<Dialog*>&)dialog_layer->GetControls();
		static vector<Dialog*> to_remove;
		for(vector<Dialog*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
		{
			if((*it)->parent == d)
				to_remove.push_back(*it);
		}
		if(!to_remove.empty())
		{
			for(vector<Dialog*>::iterator it = to_remove.begin(), end = to_remove.end(); it != end; ++it)
				CloseDialogInternal(*it);
			to_remove.clear();
		}
	}
}

//=================================================================================================
bool IGUI::HaveTopDialog(cstring name) const
{
	assert(name);

	if(dialog_layer->Empty())
		return false;

	Dialog* d = (Dialog*)(dialog_layer->Top());
	return d->name == name;
}

//=================================================================================================
bool IGUI::HaveDialog() const
{
	return !dialog_layer->Empty();
}

//=================================================================================================
void IGUI::DrawSpriteFull(TEX t, const DWORD color)
{
	assert(t);

	tCurrent = t;
	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	v->pos = VEC3(0,0,0);
	v->color = col;
	v->tex = VEC2(0,0);
	++v;

	v->pos = VEC3(float(wnd_size.x),0,0);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC3(0,float(wnd_size.y),0);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC3(0,float(wnd_size.y),0);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC3(float(wnd_size.x),0,0);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC3(float(wnd_size.x),float(wnd_size.y),0);
	v->color = col;
	v->tex = VEC2(1,1);
	++v;

	in_buffer = 1;
	Flush();
}

//=================================================================================================
void IGUI::OnChar(char c)
{
	for(vector<OnCharHandler*>::iterator it = on_char.begin(), end = on_char.end(); it != end; ++it)
	{
		Control* ctrl = dynamic_cast<Control*>(*it);
		if(ctrl->visible)
			(*it)->OnChar(c);
	}
}

//=================================================================================================
void IGUI::SimpleDialog(cstring text, Control* parent, cstring name)
{
	DialogInfo di;
	di.event = nullptr;
	di.name = name;
	di.parent = parent;
	di.pause = false;
	di.text = text;
	di.order = ORDER_NORMAL;
	di.type = DIALOG_OK;

	if(parent)
	{
		Dialog* d = dynamic_cast<Dialog*>(parent);
		if(d)
			di.order = d->order;
	}

	ShowDialog(di);
}

//=================================================================================================
void IGUI::DrawSpriteRect(TEX t, const RECT& rect, DWORD color)
{
	assert(t);

	tCurrent = t;
	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	v->pos = VEC3(float(rect.left),float(rect.top),0);
	v->color = col;
	v->tex = VEC2(0,0);
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC3(float(rect.right),float(rect.bottom),0);
	v->color = col;
	v->tex = VEC2(1,1);
	++v;

	in_buffer = 1;
	Flush();
}

//=================================================================================================
bool IGUI::HaveDialog(cstring name)
{
	assert(name);
	vector<Dialog*>& dialogs = (vector<Dialog*>&)dialog_layer->GetControls();
	for each(Dialog* dialog in dialogs)
	{
		if(dialog->name == name)
			return true;
	}
	return false;
}

//=================================================================================================
bool IGUI::AnythingVisible() const
{
	return !dialog_layer->Empty() || layer->AnythingVisible();
}

//=================================================================================================
void IGUI::OnResize(const INT2& _wnd_size)
{
	wnd_size = _wnd_size;
	layer->Event(GuiEvent_WindowResize);
	dialog_layer->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void IGUI::DrawSpriteRectPart(TEX t, const RECT& rect, const RECT& part, DWORD color)
{
	assert(t);

	tCurrent = t;
	Lock();

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);
	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);
	BOX2D uv(float(part.left)/desc.Width, float(part.top)/desc.Height, float(part.right)/desc.Width, float(part.bottom)/desc.Height);

	v->pos = VEC3(float(rect.left),float(rect.top),0);
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	in_buffer = 1;
	Flush();
}

//=================================================================================================
void IGUI::DrawSpriteTransform(TEX t, const MATRIX& mat, DWORD color)
{
	assert(t);

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);

	tCurrent = t;
	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	VEC2 leftTop(0,0),
		 rightTop(float(desc.Width),0),
		 leftBottom(0,float(desc.Height)),
		 rightBottom(float(desc.Width),float(desc.Height));

	D3DXVec2TransformCoord(&leftTop, &leftTop, &mat);
	D3DXVec2TransformCoord(&rightTop, &rightTop, &mat);
	D3DXVec2TransformCoord(&leftBottom, &leftBottom, &mat);
	D3DXVec2TransformCoord(&rightBottom, &rightBottom, &mat);

	v->pos = VEC32(leftTop);
	v->color = col;
	v->tex = VEC2(0,0);
	++v;

	v->pos = VEC32(rightTop);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC32(leftBottom);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC32(leftBottom);
	v->color = col;
	v->tex = VEC2(0,1);
	++v;

	v->pos = VEC32(rightTop);
	v->color = col;
	v->tex = VEC2(1,0);
	++v;

	v->pos = VEC32(rightBottom);
	v->color = col;
	v->tex = VEC2(1,1);
	++v;

	in_buffer = 1;
	Flush();
}

//=================================================================================================
void IGUI::DrawLine(const VEC2* lines, uint count, DWORD color, bool strip)
{
	assert(lines && count);

	Lock();

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);
	uint ile = count;

	if(strip)
	{
		v->pos = VEC32(*lines++);
		v->color = col;
		++v;

		while(ile--)
		{
			v->pos = VEC32(*lines++);
			v->color = col;
			++v;
		}
	}
	else
	{
		while(ile--)
		{
			v->pos = VEC32(*lines++);
			v->color = col;
			++v;

			v->pos = VEC32(*lines++);
			v->color = col;
			++v;
		}
	}

	V( vb->Unlock() );
	V( device->SetVertexDeclaration(Game::Get().vertex_decl[VDI_PARTICLE]) );
	V( device->SetStreamSource(0, vb, 0, sizeof(VParticle)) );
	V( device->DrawPrimitive(strip ? D3DPT_LINESTRIP : D3DPT_LINELIST, 0, count) );
}

//=================================================================================================
void IGUI::LineBegin()
{
	eGui->EndPass();
	eGui->End();
	eGui->SetTechnique(techGui2);
	UINT passes;
	eGui->Begin(&passes, 0);
	eGui->BeginPass(0);
}

//=================================================================================================
void IGUI::LineEnd()
{
	eGui->EndPass();
	eGui->End();
	eGui->SetTechnique(techGui);
	UINT passes;
	eGui->Begin(&passes, 0);
	eGui->BeginPass(0);
}

//=================================================================================================
bool IGUI::NeedCursor()
{
	if(!dialog_layer->Empty())
		return true;
	else
		return layer->visible && layer->NeedCursor();
}

//=================================================================================================
void IGUI::DrawText3D(Font* font, StringOrCstring text, DWORD flags, DWORD color, const VEC3& pos, float hpp)
{
	assert(font);

	INT2 pt;
	if(!To2dPoint(pos, pt))
		return;

	INT2 size = font->CalculateSize(text);

	// tekst
	RECT r = {pt.x-size.x/2, pt.y-size.y-4, pt.x+size.x/2+1, pt.y-4};
	DrawText(font, text, flags|DT_NOCLIP, color, r);

	// pasek hp
	if(hpp >= 0.f)
	{
		DWORD color2 = COLOR_RGBA(255,255,255,(color & 0xFF000000)>>24);

		RECT r2 = {r.left, r.bottom, r.right, r.bottom+4};
		GUI.DrawSpriteRect(tMinihp[0], r2, color2);

		r2.right = r2.left + int(hpp*(r2.right - r2.left));
		RECT r3 = {0, 0, int(hpp*64), 4};
		GUI.DrawSpriteRectPart(tMinihp[1], r2, r3, color2);
	}
}

//=================================================================================================
bool IGUI::To2dPoint(const VEC3& pos, INT2& pt)
{
	VEC4 v4;
	D3DXVec3Transform(&v4, &pos, &mViewProj);

	if(v4.z < 0)
	{
		// jest poza kamer¹
		return false;
	}

	VEC3 v3;

	// see if we are in world space already
	v3 = VEC3(v4.x,v4.y,v4.z);
	if(v4.w != 1)
	{
		if(v4.w == 0)
			v4.w = 0.00001f;
		v3 /= v4.w;
	}

	pt.x = int(v3.x*(wnd_size.x/2)+(wnd_size.x/2));
	pt.y = -int(v3.y*(wnd_size.y/2)-(wnd_size.y/2));

	return true;
}

//=================================================================================================
/*void IGUI::DrawSpritePart(TEX t, const INT2& pos, const RECT& part, DWORD color)
{
	assert(t);

	tCurrent = t;
	Lock();

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);
	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);
	BOX2D uv(float(part.left)/desc.Width, float(part.top)/desc.Height, float(part.right)/desc.Width, float(part.bottom)/desc.Height);

	v->pos = VEC3(float(rect.left),float(rect.top),0);
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC3(float(rect.left),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.top),0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC3(float(rect.right),float(rect.bottom),0);
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	in_buffer = 1;
	Flush();
}*/

//=================================================================================================
bool IGUI::Intersect(vector<Hitbox>& hitboxes, const INT2& pt, int* index, int* index2)
{
	for(vector<Hitbox>::iterator it = hitboxes.begin(), end = hitboxes.end(); it != end; ++it)
	{
		if(PointInRect(pt, it->rect))
		{
			if(index)
				*index = it->index;
			if(index2)
				*index2 = it->index2;
			return true;
		}
	}

	return false;
}

//=================================================================================================
void IGUI::DrawSpriteTransformPart(TEX t, const MATRIX& mat, const RECT& part, DWORD color)
{
	assert(t);

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);

	tCurrent = t;
	Lock();

	BOX2D uv(float(part.left)/desc.Width, float(part.top/desc.Height), float(part.right)/desc.Width, float(part.bottom)/desc.Height);

	VEC4 col = VEC4(float((color&0xFF0000)>>16)/255, float((color&0xFF00)>>8)/255, float(color&0xFF)/255, float((color&0xFF000000)>>24)/255);

	VEC2 leftTop(float(part.left),float(part.top)),
		rightTop(float(part.right),float(part.top)),
		leftBottom(float(part.left),float(part.bottom)),
		rightBottom(float(part.right),float(part.bottom));

	D3DXVec2TransformCoord(&leftTop, &leftTop, &mat);
	D3DXVec2TransformCoord(&rightTop, &rightTop, &mat);
	D3DXVec2TransformCoord(&leftBottom, &leftBottom, &mat);
	D3DXVec2TransformCoord(&rightBottom, &rightBottom, &mat);

	v->pos = VEC32(leftTop);
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = VEC32(rightTop);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC32(leftBottom);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC32(leftBottom);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = VEC32(rightTop);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = VEC32(rightBottom);
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	in_buffer = 1;
	Flush();
}

//=================================================================================================
void IGUI::CloseDialogs()
{
	vector<Dialog*>& dialogs = (vector<Dialog*>&)dialog_layer->GetControls();
	for(Dialog* dialog : dialogs)
	{
		if(OR2_EQ(dialog->type, DIALOG_OK, DIALOG_YESNO))
			delete dialog;
		else
			dialog->Event(GuiEvent_Close);
		DEBUG_DO(RemoveElementTry(created_dialogs, dialog));
	}
	dialogs.clear();
	dialog_layer->inside_loop = false;
	assert(created_dialogs.empty());
	created_dialogs.clear();
}

//=================================================================================================
bool IGUI::HavePauseDialog() const
{
	vector<Dialog*>& dialogs = (vector<Dialog*>&)dialog_layer->GetControls();
	for(vector<Dialog*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->pause)
			return true;
	}
	return false;
}

//=================================================================================================
Dialog* IGUI::GetDialog(cstring name)
{
	assert(name);
	if(dialog_layer->Empty())
		return nullptr;
	vector<Dialog*>& dialogs = (vector<Dialog*>&)dialog_layer->GetControls();
	for(vector<Dialog*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->name == name)
			return *it;
	}
	return nullptr;
}

struct GuiRect
{
	float m_left, m_top, m_right, m_bottom, m_u, m_v, m_u2, m_v2;

	void Set(uint width, uint height)
	{
		m_left = 0;
		m_top = 0;
		m_right = (float)width;
		m_bottom = (float)height;
		m_u = 0;
		m_v = 0;
		m_u2 = 1;
		m_v2 = 1;
	}

	void Set(uint width, uint height, const RECT& part)
	{
		m_left = (float)part.left;
		m_top = (float)part.top;
		m_right = (float)part.right;
		m_bottom = (float)part.bottom;
		m_u = m_left / width;
		m_v = m_top / height;
		m_u2 = m_right / width;
		m_v2 = m_bottom / height;
	}

	void Transform(const MATRIX* mat)
	{
		VEC2 leftTop(m_left, m_top);
		VEC2 rightBottom(m_right, m_bottom);
		D3DXVec2TransformCoord(&leftTop, &leftTop, mat);
		D3DXVec2TransformCoord(&rightBottom, &rightBottom, mat);
		m_left = leftTop.x;
		m_top = leftTop.y;
		m_right = rightBottom.x;
		m_bottom = rightBottom.y;
	}

	bool Clip(const RECT& clipping)
	{
		const float c_left = (float)clipping.left;
		const float c_top = (float)clipping.top;
		const float c_right = (float)clipping.right;
		const float c_bottom = (float)clipping.bottom;
		if(m_left >= c_right || c_left >= m_right || m_top >= c_bottom || c_top >= m_bottom) // no intersection
			return false;
		if(m_left >= c_left && m_right <= c_right && m_top >= c_top && m_bottom <= c_bottom) // fully inside
			return true;
		const float left = max(m_left, c_left);
		const float right = min(m_right, c_right);
		const float top = max(m_top, c_top);
		const float bottom = min(m_bottom, c_bottom);
		const VEC2 orig_size(m_right - m_left, m_bottom - m_top);
		const float u = lerp(m_u, m_u2, (left - m_left) / orig_size.x);
		m_u2 = lerp(m_u, m_u2, 1.f - (m_right - right) / orig_size.x);
		m_u = u;
		const float v = lerp(m_v, m_v2, (top - m_top) / orig_size.y);
		m_v2 = lerp(m_v, m_v2, 1.f - (m_bottom - bottom) / orig_size.y);
		m_v = v;
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		return true;
	}

	void Populate(VParticle*& v, const VEC4& col)
	{
		v->pos = VEC3(m_left, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v);
		++v;

		v->pos = VEC3(m_right, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v);
		++v;

		v->pos = VEC3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v2);
		++v;

		v->pos = VEC3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v2);
		++v;

		v->pos = VEC3(m_right, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v);
		++v;

		v->pos = VEC3(m_right, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v2);
		++v;
	}
};

//=================================================================================================
void IGUI::DrawSprite2(TEX t, const MATRIX* mat, const RECT* part, const RECT* clipping, DWORD color)
{
	assert(t && mat);

	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);
	GuiRect rect;
	
	// set pos & uv
	if(part)
		rect.Set(desc.Width, desc.Height, *part);
	else
		rect.Set(desc.Width, desc.Height);

	// transform
	if(mat)
		rect.Transform(mat);

	// clipping
	if(clipping && !rect.Clip(*clipping))
		return;

	tCurrent = t;
	Lock();

	// fill vertex buffer
	VEC4 col;
	ColorToVec(color, col);
	rect.Populate(v, col);
	in_buffer = 1;
	Flush();
}

bool ParseGroupIndex(cstring Text, size_t LineEnd, size_t& i, int& index, int& index2)
{
	index = -1;
	index2 = -1;
	LocalString tmp_s;
	bool first = true;
	while(true)
	{
		assert(i < LineEnd);
		char c = Text[i];
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
