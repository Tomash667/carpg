#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct Unit;
struct DialogContext;

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
class GameGui : public Control
{
public:
	GameGui();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	bool NeedCursor() const;
	void Event(GuiEvent e);

	void DrawSpeechBubbles();
	void UpdateSpeechBubbles(float dt);
	void AddSpeechBubble(Unit* unit, cstring text);
	void Reset();
	bool UpdateChoice(DialogContext& ctx, int choices);
	void UpdateScrollbar(int choices);

	Scrollbar scrollbar;
	vector<SpeechBubble*> speech_bbs;
	cstring txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txGamePausedBig, txPressEsc;
	bool nd_pass;
	INT2 debug_info_size, dialog_pos, dialog_size, profiler_size;
};
