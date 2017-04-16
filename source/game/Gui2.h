#pragma once

#include "VertexDeclaration.h"
#include "Layout.h"

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

	size_t length() const
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
// Czcionka
struct Font
{
	TEX tex, texOutline;
	int height;
	float outline_shift;
	Glyph glyph[256];

	// zwraca szeroko�� znaku
	int GetCharWidth(char c) const
	{
		const Glyph& g = glyph[byte(c)];
		if(g.ok)
			return g.width;
		else
			return 0;
	}
	// oblicza szeroko�� linijki tekstu
	int LineWidth(cstring str) const;
	// oblicza wysoko�� i szeroko�� bloku tekstu
	INT2 CalculateSize(StringOrCstring str, int limit_width=INT_MAX) const;
	INT2 CalculateSizeWrap(StringOrCstring str, int border=32) const;
	// dzieli tekst na linijki
	bool SplitLine(size_t& OutBegin, size_t& OutEnd, int& OutWidth, size_t& InOutIndex, cstring Text, size_t TextEnd, DWORD Flags, int Width) const;
};

//-----------------------------------------------------------------------------
// W�asne flagi do rysowania tekstu
// aktualnie obs�ugiwane DT_LEFT, DT_CENTER, DT_RIGHT, DT_TOP, DT_VCENTER, DT_BOTTOM, DT_SINGLELINE, DT_OUTLINE
#define DT_OUTLINE (1<<31)

//-----------------------------------------------------------------------------
// Gui events (in comment is new gui meaning)
enum GuiEvent
{
	GuiEvent_GainFocus, // control get focus (old)
	GuiEvent_LostFocus, // control lost focus (old)
	GuiEvent_Moved, // control is moved
	GuiEvent_Resize, // control is resized
	GuiEvent_Show, // control is shown
	GuiEvent_WindowResize, // game window size change, only send to parent controls
	GuiEvent_Close, // window is closed (old)
	GuiEvent_Initialize, // send at control initialization
	GuiEvent_Hide, // control is hidden
	GuiEvent_LostMouseFocus, // control lost mouse focus
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
typedef delegate<void(int)> DialogEvent;
typedef delegate<void(int,int)> DialogEvent2;

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
	DialogInfo() : custom_names(nullptr), img(nullptr), have_tick(false), ticked(false), auto_wrap(false), type(DIALOG_OK), parent(nullptr), order(ORDER_TOP), pause(true)
	{
	}

	string name, text;
	GUI_DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	cstring* custom_names, tick_text;
	TEX img;
	bool pause, have_tick, ticked, auto_wrap;
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
// helper functions
namespace gui
{
	class Overlay;

	inline VEC4 ColorFromDWORD(DWORD color)
	{
		return VEC4(float((color & 0xFF0000) >> 16) / 255,
			float((color & 0xFF00) >> 8) / 255,
			float(color & 0xFF) / 255,
			float((color & 0xFF000000) >> 24) / 255);
	}
	
	inline INT2 GetImgSize(TEX img)
	{
		D3DSURFACE_DESC desc;
		img->GetLevelDesc(0, &desc);
		return INT2(desc.Width, desc.Height);
	}
}

//-----------------------------------------------------------------------------
// GUI
class IGUI
{
	struct Notification
	{
		enum State
		{
			Showing,
			Shown,
			Hiding
		};

		string text;
		TEX icon;
		float t, t2;
		State state;
	};

	static const int MAX_ACTIVE_NOTIFICATIONS = 3;
	Notification* active_notifications[MAX_ACTIVE_NOTIFICATIONS];
	vector<Notification*> pending_notifications;

public:
	IGUI();
	~IGUI();
	void Init(IDirect3DDevice9* device, ID3DXSprite* sprite);
	void InitLayout();
	void SetText();
	void SetShader(ID3DXEffect* e);
	void Draw(const INT2& wnd_size);
	Font* CreateFont(cstring name, int size, int weight, int tex_size, int outline=0);
	/* zaawansowane renderowanie tekstu (w por�wnaniu do ID3DXFont)
	zwraca false je�eli by� clipping od do�u (nie kontuuj tekstu w flow)
	Znak $ oznacza jak�� specjaln� czynno��:
		$$ - wstaw $
		$c? - ustaw kolor (r-czerwony, g-zielony, y-��ty, b-czarny, w-bia�y, -przywr�c domy�lny)
		$h+ - informacja o hitboxie
		$h- - koniec hitboxa
		/$b - przerwa w tek�cie
		/$n - nie przerywaj tekstu a� do nast�pnego takiego symbolu (np $njaki� tekst$n - ten tekst nigdy nie zostanie rozdzielony pomi�dzy dwie linijki)
	*/
	bool DrawText(Font* font, StringOrCstring text, DWORD flags, DWORD color, const RECT& rect, const RECT* clipping=nullptr, vector<Hitbox>* hitboxes=nullptr,
		int* hitbox_counter=nullptr, const vector<TextLine>* lines=nullptr);
	void Add(Control* ctrl);
	void DrawItem(TEX t, const INT2& item_pos, const INT2& item_size, DWORD color, int corner = 16, int size = 64, const BOX2D* clip_rect = nullptr);
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
	void AddOnCharHandler(OnCharHandler* h) { on_char.push_back(h); }
	void RemoveOnCharHandler(OnCharHandler* h) { RemoveElement(on_char, h); }
	void SimpleDialog(cstring text, Control* parent, cstring name="simple");
	void DrawSpriteRect(TEX t, const RECT& rect, DWORD color=WHITE);
	bool HaveDialog(cstring name);
	IDirect3DDevice9* GetDevice() { return device; }
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
	static bool Intersect(vector<Hitbox>& hitboxes, const INT2& pt, int* index, int* index2=nullptr);
	void DrawSpriteTransformPart(TEX t, const MATRIX& mat, const RECT& part, DWORD color=WHITE);
	void CloseDialogs();
	bool HavePauseDialog() const;
	Dialog* GetDialog(cstring name);
	void DrawSprite2(TEX t, const MATRIX* mat, const RECT* part, const RECT* clipping, DWORD color);
	void AddNotification(cstring text, TEX icon, float timer);
	void DrawArea(DWORD color, const INT2& pos, const INT2& size, const BOX2D* clip_rect = nullptr);
	void DrawArea(DWORD color, const RECT& rect, const BOX2D* clip_rect = nullptr)
	{
		DrawArea(color, INT2(rect.left, rect.top), INT2(rect.right - rect.left, rect.bottom - rect.top), clip_rect);
	}
	// 2.0
	void SetLayout(gui::Layout* _layout) { assert(_layout); layout = _layout; }
	gui::Layout* GetLayout() const { return layout; }
	void DrawArea(const BOX2D& rect, const gui::AreaLayout& area_layout, const BOX2D* clip_rect = nullptr);
	void SetOverlay(gui::Overlay* _overlay) { overlay = _overlay; }
	gui::Overlay* GetOverlay() const { return overlay; }
	bool MouseMoved() const { return cursor_pos != prev_cursor_pos; }
	void SetClipboard(cstring text);
	cstring GetClipboard();

	MATRIX mIdentity, mViewProj;
	INT2 cursor_pos, prev_cursor_pos, wnd_size;
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
	void DrawLine(Font* font, cstring text, size_t LineBegin, size_t LineEnd, const VEC4& def_color, VEC4& color, int x, int y, const RECT* clipping,
		HitboxContext* hc);
	void DrawLineOutline(Font* font, cstring text, size_t LineBegin, size_t LineEnd, const VEC4& def_color, VEC4& color, int x, int y, const RECT* clipping,
		HitboxContext* hc);
	int Clip(int x, int y, int w, int h, const RECT* clipping);
	void Lock(bool outline=false);
	void Flush(bool lock=false);
	void SkipLine(cstring text, size_t LineBegin, size_t LineEnd, HitboxContext* hc);
	bool CreateFontInternal(Font* font, ID3DXFont* dx_font, int tex_size, int outline, int max_outline);
	void UpdateNotifications(float dt);
	void DrawNotifications();
	void AddRect(const VEC2& left_top, const VEC2& right_bottom, const VEC4& color);

	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	TEX tFontTarget;
	TEX tSet, tCurrent, tCurrent2, tPixel;
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
	gui::Layout* layout;
	gui::Overlay* overlay;
};

//-----------------------------------------------------------------------------
extern IGUI GUI;
