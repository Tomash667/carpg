#pragma once

//-----------------------------------------------------------------------------
class GameState
{
public:
	virtual ~GameState() {}
	virtual bool OnEnter() { return true; }
};
