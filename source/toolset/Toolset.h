#pragma once

#include "Panel.h"

class Engine;

class Toolset : public Panel
{
public:
	void Init(Engine* engine);
	void Start();
	void OnDraw();
	bool OnUpdate(float dt);

private:
	inline virtual bool NeedCursor() const override { return true; }
	virtual void Event(GuiEvent e) override;

	Engine* engine;
};
