#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "Scrollbar.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
struct Game;
struct Unit;
struct DialogContext;
class GamePanel;
class GamePanelContainer;
class Inventory;
class StatsPanel;
class TeamPanel;
class Journal;
class Minimap;
class MpBox;
class GameMessages;
class ActionPanel;

//-----------------------------------------------------------------------------
enum class OpenPanel
{
	None,
	Stats,
	Inventory,
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
	Inventory,
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
	void LoadData();
	void GetGamePanels(vector<GamePanel*>& panels);
	OpenPanel GetOpenPanel();
	void ShowPanel(OpenPanel p, OpenPanel open = OpenPanel::Unknown);
	void PositionPanels();
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	bool IsMouseInsideDialog() const { return PointInRect(GUI.cursor_pos, dialog_pos, dialog_size); }

	// panels
	GamePanelContainer* gp_trade;
	Inventory* inventory, *inv_trade_mine, *inv_trade_other;
	StatsPanel* stats;
	TeamPanel* team_panel;
	Journal* journal;
	Minimap* minimap;
	MpBox* mp_box;
	GameMessages* game_messages;
	ActionPanel* action_panel;
	//
	bool use_cursor;

private:
	void DrawFront();
	void DrawBack();
	void DrawDeathScreen();
	void DrawEndOfGameScreen();
	void DrawSpeechBubbles();
	void DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha);
	void UpdateSpeechBubbles(float dt);
	void GetTooltip(TooltipController*, int group, int id);

	Game& game;
	TooltipController tooltip;
	float buff_scale;
	vector<BuffImage> buff_images;
	float sidebar;
	int sidebar_state[(int)SideButtonId::Max];
	TEX tBar, tHpBar, tPoisonedHpBar, tStaminaBar, tManaBar, tShortcut, tShortcutHover, tShortcutDown, tSideButton[(int)SideButtonId::Max], tMinihp[2], tMinistamina, tCelownik,
		tBubble, tObwodkaBolu;
	Scrollbar scrollbar;
	vector<SpeechBubble*> speech_bbs;
	cstring txMenu, txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txPressEsc, txHp, txStamina;
	Int2 debug_info_size, dialog_pos, dialog_size, profiler_size;
};
