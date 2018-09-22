#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "Scrollbar.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
enum class OpenPanel
{
	None,
	Stats,
	InventoryPanel,
	Team,
	Journal,
	Minimap,
	Action,
	Trade,
	Unknown
};

//-----------------------------------------------------------------------------
enum class SideButtonId
{
	Menu,
	Team,
	Minimap,
	Journal,
	InventoryPanel,
	Action,
	Stats,
	Talk,
	Max
};

//-----------------------------------------------------------------------------
struct SpeechBubble
{
	string text;
	Unit* unit;
	Int2 size;
	float time, length;
	int skip_id;
	Vec3 last_pos;
	bool visible;
};

//-----------------------------------------------------------------------------
struct BuffImage
{
	Vec2 pos;
	TEX tex;
	int id;

	BuffImage(const Vec2& pos, TEX tex, int id) : pos(pos), tex(tex), id(id)
	{
	}
};

//-----------------------------------------------------------------------------
class GameGui : public Container
{
public:
	GameGui();
	~GameGui();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	bool NeedCursor() const override;
	void Event(GuiEvent e) override;
	void AddSpeechBubble(Unit* unit, cstring text);
	void AddSpeechBubble(const Vec3& pos, cstring text);
	void Reset();
	bool UpdateChoice(DialogContext& ctx, int choices);
	void UpdateScrollbar(int choices);
	bool HavePanelOpen() const;
	bool CanFocusMpBox() const { return !HavePanelOpen(); }
	void ClosePanels(bool close_mp_box = false);
	void GetGamePanels(vector<GamePanel*>& panels);
	OpenPanel GetOpenPanel();
	void ShowPanel(OpenPanel p, OpenPanel open = OpenPanel::Unknown);
	void PositionPanels();
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	bool IsMouseInsideDialog() const { return PointInRect(GUI.cursor_pos, dialog_pos, dialog_size); }
	void Setup();
	void RemoveUnit(Unit* unit);

	bool use_cursor;

private:
	struct UnitView
	{
		Unit* unit;
		Vec3 last_pos;
		float time;
		bool valid;
	};

	struct SortedUnitView
	{
		Unit* unit;
		float dist;
		int alpha;
		Vec3* last_pos;
	};

	struct SortedSpeechBubble
	{
		SpeechBubble* bubble;
		float dist;
		Int2 pt;
	};

	void DrawFront();
	void DrawBack();
	void DrawDeathScreen();
	void DrawEndOfGameScreen();
	void DrawSpeechBubbles();
	void DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha);
	void UpdateSpeechBubbles(float dt);
	void GetTooltip(TooltipController*, int group, int id);
	void SortUnits();
	void UpdatePlayerView(float dt);

	Game& game;
	TooltipController tooltip;
	float buff_scale;
	vector<BuffImage> buff_images;
	vector<SortedUnitView> sorted_units;
	float sidebar;
	int sidebar_state[(int)SideButtonId::Max];
	TEX tBar, tHpBar, tPoisonedHpBar, tStaminaBar, tManaBar, tShortcut, tShortcutHover, tShortcutDown, tSideButton[(int)SideButtonId::Max], tMinihp[2], tMinistamina,
		tCrosshair, tBubble, tObwodkaBolu, tActionCooldown;
	Scrollbar scrollbar;
	vector<SpeechBubble*> speech_bbs;
	vector<SortedSpeechBubble> sorted_speech_bbs;
	cstring txMenu, txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txPressEsc, txHp, txStamina;
	Int2 debug_info_size, dialog_pos, dialog_size, profiler_size;
	vector<UnitView> unit_views;
};
