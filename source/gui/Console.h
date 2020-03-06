#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <InputTextBox.h>

//-----------------------------------------------------------------------------
// Ingame console, opened with tilde (~)
class Console : public DialogBox, public OnCharHandler
{
public:
	explicit Console(const DialogInfo& info);
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void AddMsg(cstring str) { itb.Add(str); }
	void OnInput(const string& str);
	void Reset() { itb.Reset(); }

private:
	Texture* tBackground;
	InputTextBox itb;
	bool added;
};
