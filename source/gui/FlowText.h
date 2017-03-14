// kontener na StaticText u¿ywany w StatsPanel i TeamPanel
#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "StaticText.h"

//-----------------------------------------------------------------------------
class FlowText : public Container
{
public:
	void Draw(ControlDrawData* cdd = nullptr) override;

	void Calculate();

	int total_size, moved;
	vector<Hitbox> hitboxes;
};
