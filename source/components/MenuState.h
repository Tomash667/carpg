#pragma once

//-----------------------------------------------------------------------------
#include "BaseGameState.h"

//-----------------------------------------------------------------------------
class MenuState : public BaseGameState
{
public:
	void PassErrors(uint errors, uint warnings);

private:
	void OnEnter() override;
	void StartGameMode();

	uint load_errors, load_warnings;
};
