#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
struct GameMsg : public ObjectPoolProxy<GameMsg>
{
	string msg, pattern;
	float time, fade;
	Vec2 pos;
	Int2 size;
	int type, id, value;
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
	GameMsg& AddMessage(cstring text, float time, int type);
	void AddMessageIfNotExists(cstring text, float time, int type);
	void AddOrUpdateMessageWithPattern(cstring pattern, int type, int id, int value, float time);

private:
	vector<GameMsg*> msgs;
	int msgs_h;
	cstring txGamePausedBig;
};
