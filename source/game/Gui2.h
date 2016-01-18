#pragma once

#include "VertexDeclaration.h"

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

	inline size_t length() const
	{
		return is_str ? str->length() : strlen(cstr);
	}

	inline cstring c_str()
	{
		return is_str ? str->c_str() : cstr;
	}

	inline void AddTo(string& s) const
	{
		if(is_str)
			s += *str;
		else
			s += cstr;
	}
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
	inline int GetCharWidth(char c) const
	{
		const Glyph& g = glyph[byte(c)];
		if(g.ok)
			return g.width;
		else
			return 0;
	}
	// oblicza szerokoœæ linijki tekstu
	int LineWidth(cstring str) const;
	// oblicza wysokoœæ i szerokoœæ bloku tekstu
	INT2 CalculateSize(StringOrCstring str, int limit_width=INT_MAX) const;
	// dzieli tekst na linijki
	bool SplitLine(size_t& OutBegin, size_t& OutEnd, int& OutWidth, size_t& InOutIndex, cstring Text, size_t TextEnd, DWORD Flags, int Width) const;
};

//-----------------------------------------------------------------------------
// W³asne flagi do rysowania tekstu
// aktualnie obs³ugiwane DT_LEFT, DT_CENTER, DT_RIGHT, DT_TOP, DT_VCENTER, DT_BOTTOM, DT_SINGLELINE, DT_OUTLINE
#define DT_OUTLINE (1<<31)

//-----------------------------------------------------------------------------
// Zdarzenie gui
enum GuiEvent
{
	GuiEvent_GainFocus,
	GuiEvent_LostFocus,
	GuiEvent_Moved,
	GuiEvent_Resize,
	GuiEvent_Show,
	GuiEvent_WindowResize,
	GuiEvent_Close,
	GuiEvent_Custom
};

//-----------------------------------------------------------------------------
class Control;
class Container;
class Dialog;

//-----------------------------------------------------------------------------
struct Hitbox
{
	RECT rect;
	int index, index2;
};

//-----------------------------------------------------------------------------
enum class HitboxOpen
{
	No,
	Yes,
	Group
};

//-----------------------------------------------------------------------------
struct HitboxContext
{
	vector<Hitbox>* hitbox;
	int counter, group_index, group_index2;
	HitboxOpen open;
	RECT region;
};

//-----------------------------------------------------------------------------
enum CursorMode
{
	CURSOR_NORMAL,
	CURSOR_HAND,
	CURSOR_TEXT
};

//-----------------------------------------------------------------------------
enum GUI_DialogType
{
	DIALOG_OK,
	DIALOG_YESNO,
	DIALOG_CUSTOM
};

//-----------------------------------------------------------------------------
typedef fastdelegate::FastDelegate1<int> DialogEvent;
typedef fastdelegate::FastDelegate2<int, int> DialogEvent2;

//-----------------------------------------------------------------------------
enum DialogOrder
{
	ORDER_NORMAL,
	ORDER_TOP,
	ORDER_TOPMOST
};

//-----------------------------------------------------------------------------
struct DialogInfo
{
	DialogInfo() : custom_names(nullptr), have_tick(false), ticked(false)
	{

	}

	string name, text;
	GUI_DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	bool pause, have_tick, ticked;
	cstring* custom_names, tick_text;
};

//-----------------------------------------------------------------------------
class OnCharHandler
{
public:
	virtual void OnChar(char c) {}
};

//-----------------------------------------------------------------------------
struct TextLine
{
	size_t begin, end;
	int width;

	TextLine(size_t begin, size_t end, int width) : begin(begin), end(end), width(width)
	{

	}
};

//-----------------------------------------------------------------------------
// GUI
class IGUI
{
public:
	IGUI();
	~IGUI();
	void Init(IDirect3DDevice9* device, ID3DXSprite* sprite);
	void SetText();
	void SetShader(ID3DXEffect* e);
	void Draw(const INT2& wnd_size);
	Font* CreateFont(cstring name, int size, int weight, int tex_size, int outline=0);
	/* zaawansowane renderowanie tekstu (w porównaniu do ID3DXFont)
	zwraca false je¿eli by³ clipping od do³u (nie kontuuj tekstu w flow)
	Znak $ oznacza jak¹œ specjaln¹ czynnoœæ:
		$$ - wstaw $
		$c? - ustaw kolor (r-czerwony, g-zielony, y-¿ó³ty, b-czarny, w-bia³y, -przywróc domyœlny)
		$h+ - informacja o hitboxie
		$h- - koniec hitboxa
		/$b - przerwa w tekœcie
		/$n - nie przerywaj tekstu a¿ do nastêpnego takiego symbolu (np $njakiœ tekst$n - ten tekst nigdy nie zostanie rozdzielony pomiêdzy dwie linijki)
	*/
	bool DrawText(Font* font, StringOrCstring text, DWORD flags, DWORD color, const RECT& rect, const RECT* clipping=nullptr, vector<Hitbox>* hitboxes=nullptr, int* hitbox_counter=nullptr,
		const vector<TextLine>* lines=nullptr);
	void Add(Control* ctrl);
	void DrawItem(TEX t, const INT2& item_pos, const INT2& item_size, DWORD color, int corner=16, int size=64);
	void Update(float dt);
	void DrawSprite(TEX t, const INT2& pos, DWORD color=WHITE, const RECT* clipping=nullptr);
	void OnReset();
	void OnReload();
	void OnClean();
	void OnChar(char c);
	Dialog* ShowDialog(const DialogInfo& info);
	void ShowDialog(Dialog* dialog);
	bool CloseDialog(Dialog* d);
	void CloseDialogInternal(Dialog* d);
	bool HaveTopDialog(cstring name) const;
	bool HaveDialog() const;
	void DrawSpriteFull(TEX t, DWORD color);
	inline void AddOnCharHandler(OnCharHandler* h) { on_char.push_back(h); }
	inline void RemoveOnCharHandler(OnCharHandler* h) { RemoveElement(on_char, h); }
	void SimpleDialog(cstring text, Control* parent, cstring name="simple");
	void DrawSpriteRect(TEX t, const RECT& rect, DWORD color=WHITE);
	bool HaveDialog(cstring name);
	inline IDirect3DDevice9* GetDevice() { return device; }
	bool AnythingVisible() const;
	void OnResize(const INT2& wnd_size);
	void DrawSpriteRectPart(TEX t, const RECT& rect, const RECT& part, DWORD color=WHITE);
	void DrawSpriteTransform(TEX t, const MATRIX& mat, DWORD color=WHITE);
	void DrawLine(const VEC2* lines, uint count, DWORD color=BLACK, bool strip=true);
	void LineBegin();
	void LineEnd();
	bool NeedCursor();
	void DrawText3D(Font* font, StringOrCstring text, DWORD flags, DWORD color, const VEC3& pos, float hpp=-1.f);
	bool To2dPoint(const VEC3& pos, INT2& pt);
	//void DrawSpritePart(TEX t, const INT2& pos, const RECT& part, DWORD color=WHITE);
	//void DrawSprite(TEX t, const RECT& rect, const RECT* part, const RECT* clipping, DWORD color);
	//void DrawSpriteTransform(TEX t, MATRIX& mat, const RECT* part, DWORD color);
	static bool Intersect(vector<Hitbox>& hitboxes, const INT2& pt, int* index, int* index2=nullptr);
	void DrawSpriteTransformPart(TEX t, const MATRIX& mat, const RECT& part, DWORD color=WHITE);
	void CloseDialogs();
	bool HavePauseDialog() const;
	Dialog* GetDialog(cstring name);
	void DrawSprite2(TEX t, const MATRIX* mat, const RECT* part, const RECT* clipping, DWORD color);

	MATRIX mIdentity, mViewProj;
	INT2 cursor_pos, wnd_size;
	Font* default_font, *fBig, *fSmall;
	TEX tCursor[3], tMinihp[2];
	CursorMode cursor_mode;
	cstring txOk, txYes, txNo, txCancel;
	static TEX tBox, tBox2, tPix, tDown;
	Control* focused_ctrl;
	float mouse_wheel;
	vector<Dialog*> created_dialogs;

private:
	void CreateVertexBuffer();
	void DrawLine(Font* font, cstring text, size_t LineBegin, size_t LineEnd, const VEC4& def_color, VEC4& color, int x, int y, const RECT* clipping, HitboxContext* hc);
	void DrawLineOutline(Font* font, cstring text, size_t LineBegin, size_t LineEnd, const VEC4& def_color, VEC4& color, int x, int y, const RECT* clipping, HitboxContext* hc);
	int Clip(int x, int y, int w, int h, const RECT* clipping);
	void Lock(bool outline=false);
	void Flush(bool lock=false);
	void SkipLine(cstring text, size_t LineBegin, size_t LineEnd, HitboxContext* hc);
	bool CreateFontInternal(Font* font, ID3DXFont* dx_font, int tex_size, int outline, int max_outline);

	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	TEX tFontTarget;
	TEX tSet, tCurrent, tCurrent2;
	int max_tex_size;
	vector<Font*> fonts;
	ID3DXEffect* eGui;
	D3DXHANDLE techGui, techGui2;
	D3DXHANDLE hGuiSize, hGuiTex;
	Container* layer, *dialog_layer;
	VParticle* v, *v2;
	uint in_buffer, in_buffer2;
	VEC4 color_table[6];
	VB vb, vb2;
	HitboxContext tmpHitboxContext;
	vector<OnCharHandler*> on_char;
	bool vb2_locked;
	float outline_alpha;
};

//-----------------------------------------------------------------------------
extern IGUI GUI;
