#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>

//-----------------------------------------------------------------------------
// Display information when loading or waiting in multiplayer
class InfoBox : public DialogBox
{
public:
	explicit InfoBox(const DialogInfo& info);
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(cstring text);
};
