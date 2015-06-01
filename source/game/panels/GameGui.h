#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct Unit;
struct DialogContext;
class Inventory;
class StatsPanel;
class TeamPanel;
class Minimap;
class GamePanelContainer;
class Journal;
class MpBox;
class GameMessagesContainer;

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
class GameGui : public Container
{
public:
	GameGui();

	void PreLoadInit();
	void PostLoadInit();

	void AddSpeechBubble(Unit* unit, cstring text);
	void Reset();
	bool UpdateChoice(DialogContext& ctx, int choices);
	void UpdateScrollbar(int choices);

	bool use_cursor;
	vector<SpeechBubble*> speech_bbs;
	bool nd_pass;
	INT2 debug_info_size, dialog_pos, dialog_size, profiler_size;	
	TEX tBar, tHpBar, tPoisonedHpBar, tManaBar, tShortcut, tShortcutHover, tShortcutDown, tSideButton[(int)SideButtonId::Max];
	cstring txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txGamePausedBig, txPressEsc;

private:
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	bool NeedCursor() const;
	void Event(GuiEvent e);
	void DrawSpeechBubbles();
	void UpdateSpeechBubbles(float dt);
	
	Scrollbar scrollbar;
	float sidebar;
	int sidebar_state[(int)SideButtonId::Max];

	// contained gui items
	GamePanelContainer* gp_trade; // contains inv_trade_mine & inv_trade_other
	Inventory* inventory, *inv_trade_mine, *inv_trade_other;
	StatsPanel* stats;
	TeamPanel* team_panel;
	Journal* journal;
	Minimap* minimap;
	MpBox* mp_box;
	GameMessagesContainer* game_messages;
};
