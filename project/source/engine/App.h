#pragma once

//-----------------------------------------------------------------------------
class App
{
public:
	virtual ~App() {}
	virtual bool OnInit() = 0;
	virtual void OnChar(char c) = 0;
	virtual void OnCleanup() = 0;
	virtual void OnDraw() = 0;
	virtual void OnReload() = 0;
	virtual void OnReset() = 0;
	virtual void OnResize() = 0;
	virtual void OnTick(float dt) = 0;
	virtual void OnFocus(bool focus, const Int2& activation_point) = 0;
};
