#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
struct GameMsg
{
	string msg;
	float time, fade;
	VEC2 pos;
	INT2 size;
	int type;
};

//-----------------------------------------------------------------------------
class GameMessagesContainer : public Control
{
public:
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Reset();

	list<GameMsg> msgs;
	int msgs_h;
};
