#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
struct GameMsg
{
	string msg;
	float time, fade;
	Vec2 pos;
	Int2 size;
	int type;
};

//-----------------------------------------------------------------------------
class GameMessages : public Control
{
public:
	GameMessages();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	void Reset();
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void AddMessage(cstring text, float time, int type);
	void AddMessageIfNotExists(cstring text, float time, int type);

private:
	list<GameMsg> msgs;
	int msgs_h;
	cstring txGamePausedBig;
};
