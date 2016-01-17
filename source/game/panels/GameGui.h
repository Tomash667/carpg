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
	Active,
	Stats,
	Talk,
	Max
};

//-----------------------------------------------------------------------------
struct SpeechBubble
{
	string text;
	Unit* unit;
	INT2 size;
	float time, length;
	int skip_id;
	VEC3 last_pos;
	bool visible;
};

//-----------------------------------------------------------------------------
struct BuffImage
{
	VEC2 pos;
	TEX tex;
	int id;

	inline BuffImage(const VEC2& pos, TEX tex, int id) : pos(pos), tex(tex), id(id)
	{

	}
};

//-----------------------------------------------------------------------------
class GameGui : public Container
{
public:
	GameGui();
	~GameGui();

	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	bool NeedCursor() const;
	void Event(GuiEvent e);
	void AddSpeechBubble(Unit* unit, cstring text);
	void AddSpeechBubble(const VEC3& pos, cstring text);
	void Reset();
	bool UpdateChoice(DialogContext& ctx, int choices);
	void UpdateScrollbar(int choices);	
	bool HavePanelOpen() const;
	inline bool CanFocusMpBox() const
	{
		return !HavePanelOpen();
	}
	void ClosePanels(bool close_mp_box=false);
	void LoadData();
	void GetGamePanels(vector<GamePanel*>& panels);
	OpenPanel GetOpenPanel();
	void ShowPanel(OpenPanel p, OpenPanel open = OpenPanel::Unknown);
	void PositionPanels();
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	inline bool IsMouseInsideDialog() const
	{
		return PointInRect(GUI.cursor_pos, dialog_pos, dialog_size);
	}

	bool use_cursor;

	// panels
	GamePanelContainer* gp_trade;
	Inventory* inventory, *inv_trade_mine, *inv_trade_other;
	StatsPanel* stats;
	TeamPanel* team_panel;
	Journal* journal;
	Minimap* minimap;
	MpBox* mp_box;
	GameMessages* game_messages;	

private:
	void DrawFront();
	void DrawBack();
	void DrawDeathScreen();
	void DrawEndOfGameScreen();
	void DrawSpeechBubbles();
	void UpdateSpeechBubbles(float dt);
	void GetTooltip(TooltipController*, int group, int id);

	Game& game;
	TooltipController tooltip;
	cstring txMenu, txBuffPoison, txBuffAlcohol, txBuffRegeneration, txBuffNatural, txBuffFood, txBuffAntimagic;
	float buff_scale;
	vector<BuffImage> buff_images;
	float sidebar;
	int sidebar_state[(int)SideButtonId::Max];
	TEX tBar, tHpBar, tPoisonedHpBar, tManaBar, tShortcut, tShortcutHover, tShortcutDown, tSideButton[(int)SideButtonId::Max], tBuffPoison, tBuffAlcohol, tBuffRegeneration, tBuffFood, tBuffNatural,
		tBuffAntimagic;
	Scrollbar scrollbar;
	vector<SpeechBubble*> speech_bbs;
	cstring txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txPressEsc;
	INT2 debug_info_size, dialog_pos, dialog_size, profiler_size;
};
