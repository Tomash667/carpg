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
class GameMessages : public Control
{
public:
	GameMessages();

	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Reset();
	void Save(File& f) const;
	void Load(File& f);
	void AddMessage(cstring text, float time, int type);
	void AddMessageIfNotExists(cstring text, float time, int type);

private:
	list<GameMsg> msgs;
	int msgs_h;
	cstring txGamePausedBig;
};
