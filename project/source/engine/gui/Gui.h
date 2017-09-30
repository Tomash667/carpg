#pragma once

#include "Font.h"
#include "Layout.h"
#include "VertexDeclaration.h"

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
class DialogBox;

//-----------------------------------------------------------------------------
struct Hitbox
{
	Rect rect;
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
	Rect region;
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
typedef delegate<void(int, int)> DialogEvent2;

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
	uint begin, end;
	int width;

	TextLine(uint begin, uint end, int width) : begin(begin), end(end), width(width)
	{
	}
};

//-----------------------------------------------------------------------------
// helper functions
namespace gui
{
	class Overlay;

	inline Vec4 ColorFromDWORD(DWORD color)
	{
		return Vec4(float((color & 0xFF0000) >> 16) / 255,
			float((color & 0xFF00) >> 8) / 255,
			float(color & 0xFF) / 255,
			float((color & 0xFF000000) >> 24) / 255);
	}

	inline Int2 GetImgSize(TEX img)
	{
		D3DSURFACE_DESC desc;
		img->GetLevelDesc(0, &desc);
		return Int2(desc.Width, desc.Height);
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
	void Draw(bool draw_layers, bool draw_dialogs);
	bool AddFont(cstring filename);
	Font* CreateFont(cstring name, int size, int weight, int tex_size, int outline = 0);
	/* zaawansowane renderowanie tekstu (w porównaniu do ID3DXFont)
	zwraca false je¿eli by³ clipping od do³u (nie kontuuj tekstu w flow)
	Znak $ oznacza jak¹œ specjaln¹ czynnoœæ (o ile jest ustawiona flaga DT_PARSE_SPECIAL):
		$$ - wstaw $
		$c? - ustaw kolor (r-czerwony, g-zielony, y-¿ó³ty, b-czarny, w-bia³y, -przywróc domyœlny)
		$h+ - informacja o hitboxie
		$h- - koniec hitboxa
		/$b - przerwa w tekœcie
		/$n - nie przerywaj tekstu a¿ do nastêpnego takiego symbolu (np $njakiœ tekst$n - ten tekst nigdy nie zostanie rozdzielony pomiêdzy dwie linijki)
	*/
	bool DrawText(Font* font, StringOrCstring str, DWORD flags, DWORD color, const Rect& rect, const Rect* clipping = nullptr,
		vector<Hitbox>* hitboxes = nullptr, int* hitbox_counter = nullptr, const vector<TextLine>* lines = nullptr);
	void Add(Control* ctrl);
	void DrawItem(TEX t, const Int2& item_pos, const Int2& item_size, DWORD color, int corner = 16, int size = 64, const Box2d* clip_rect = nullptr);
	void Update(float dt, float mouse_speed);
	void DrawSprite(TEX t, const Int2& pos, DWORD color = WHITE, const Rect* clipping = nullptr);
	void OnReset();
	void OnReload();
	void OnClean();
	void OnChar(char c);
	DialogBox* ShowDialog(const DialogInfo& info);
	void ShowDialog(DialogBox* dialog);
	bool CloseDialog(DialogBox* d);
	void CloseDialogInternal(DialogBox* d);
	bool HaveTopDialog(cstring name) const;
	bool HaveDialog() const;
	void DrawSpriteFull(TEX t, DWORD color);
	void AddOnCharHandler(OnCharHandler* h) { on_char.push_back(h); }
	void RemoveOnCharHandler(OnCharHandler* h) { RemoveElement(on_char, h); }
	void SimpleDialog(cstring text, Control* parent, cstring name = "simple");
	void DrawSpriteRect(TEX t, const Rect& rect, DWORD color = WHITE);
	bool HaveDialog(cstring name);
	bool HaveDialog(DialogBox* dialog);
	IDirect3DDevice9* GetDevice() { return device; }
	bool AnythingVisible() const;
	void OnResize();
	void DrawSpriteRectPart(TEX t, const Rect& rect, const Rect& part, DWORD color = WHITE);
	void DrawSpriteTransform(TEX t, const Matrix& mat, DWORD color = WHITE);
	void DrawLine(const Vec2* lines, uint count, DWORD color = BLACK, bool strip = true);
	void LineBegin();
	void LineEnd();
	bool NeedCursor();
	bool DrawText3D(Font* font, StringOrCstring text, DWORD flags, DWORD color, const Vec3& pos, Rect* text_rect = nullptr);
	bool To2dPoint(const Vec3& pos, Int2& pt);
	static bool Intersect(vector<Hitbox>& hitboxes, const Int2& pt, int* index, int* index2 = nullptr);
	void DrawSpriteTransformPart(TEX t, const Matrix& mat, const Rect& part, DWORD color = WHITE);
	void CloseDialogs();
	bool HavePauseDialog() const;
	DialogBox* GetDialog(cstring name);
	void DrawSprite2(TEX t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr, DWORD color = WHITE);
	void AddNotification(cstring text, TEX icon, float timer);
	void DrawArea(DWORD color, const Int2& pos, const Int2& size, const Box2d* clip_rect = nullptr);
	void DrawArea(DWORD color, const Rect& rect, const Box2d* clip_rect = nullptr)
	{
		DrawArea(color, rect.LeftTop(), rect.Size(), clip_rect);
	}
	// 2.0
	void SetLayout(gui::Layout* _layout) { assert(_layout); layout = _layout; }
	gui::Layout* GetLayout() const { return layout; }
	void DrawArea(const Box2d& rect, const gui::AreaLayout& area_layout, const Box2d* clip_rect = nullptr);
	void SetOverlay(gui::Overlay* _overlay) { overlay = _overlay; }
	gui::Overlay* GetOverlay() const { return overlay; }
	bool MouseMoved() const { return cursor_pos != prev_cursor_pos; }
	void SetClipboard(cstring text);
	cstring GetClipboard();
	Rect GetSpriteRect(TEX t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr);
	void UseGrayscale(bool grayscale);
	struct DrawTextOptions
	{
		Font* font;
		cstring str;
		DWORD flags;
		DWORD color;
		Rect rect;
		const Rect* clipping;
		vector<Hitbox>* hitboxes;
		int* hitbox_counter;
		const vector<TextLine>* lines;
		Vec2 scale;
		uint lines_start;
		uint lines_end;
		uint str_length;

		DrawTextOptions(Font* font, StringOrCstring str) : font(font), str(str.c_str()), rect(rect), flags(DT_LEFT), color(BLACK), clipping(nullptr),
			hitboxes(nullptr), hitbox_counter(nullptr), lines(nullptr), scale(Vec2::One), lines_start(0), lines_end(UINT_MAX), str_length(str.length())
		{
		}
	};
	bool DrawText2(DrawTextOptions& options);

	Matrix mViewProj;
	Int2 cursor_pos, prev_cursor_pos, wnd_size;
	Font* default_font, *fBig, *fSmall;
	TEX tCursor[3];
	CursorMode cursor_mode;
	cstring txOk, txYes, txNo, txCancel;
	static TEX tBox, tBox2, tPix, tDown;
	Control* focused_ctrl;
	float mouse_wheel;
	vector<DialogBox*> created_dialogs;

private:
	void CreateVertexBuffer();
	void DrawLine(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& def_color, Vec4& color, int x, int y, const Rect* clipping,
		HitboxContext* hc, bool parse_special, const Vec2& scale);
	void DrawLineOutline(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& def_color, Vec4& color, int x, int y, const Rect* clipping,
		HitboxContext* hc, bool parse_special, const Vec2& scale);
	int Clip(int x, int y, int w, int h, const Rect* clipping);
	void Lock(bool outline = false);
	void Flush(bool lock = false);
	void SkipLine(cstring text, uint line_begin, uint line_end, HitboxContext* hc);
	bool CreateFontInternal(Font* font, ID3DXFont* dx_font, int tex_size, int outline, int max_outline);
	void UpdateNotifications(float dt);
	void DrawNotifications();
	void AddRect(const Vec2& left_top, const Vec2& right_bottom, const Vec4& color);

	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	TEX tFontTarget;
	TEX tSet, tCurrent, tCurrent2, tPixel;
	int max_tex_size;
	vector<Font*> fonts;
	ID3DXEffect* eGui;
	D3DXHANDLE techGui, techGui2, techGuiGrayscale;
	D3DXHANDLE hGuiSize, hGuiTex;
	Container* layer, *dialog_layer;
	VParticle* v, *v2;
	uint in_buffer, in_buffer2;
	Vec4 color_table[6];
	VB vb, vb2;
	HitboxContext tmpHitboxContext;
	vector<OnCharHandler*> on_char;
	bool vb2_locked, grayscale;
	float outline_alpha;
	gui::Layout* layout;
	gui::Overlay* overlay;
	IDirect3DVertexDeclaration9* vertex_decl;
};

//-----------------------------------------------------------------------------
extern IGUI GUI;
