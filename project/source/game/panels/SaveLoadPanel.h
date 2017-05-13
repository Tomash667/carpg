#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "Button.h"
#include "SaveSlot.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
class SaveLoad : public Dialog
{
public:
	enum Id
	{
		IdOk = GuiEvent_Custom,
		IdCancel
	};

	explicit SaveLoad(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void SetSaveMode(bool save_mode, bool online, SaveSlot* slots);
	void SetText();

	Button bt[2];
	cstring txSaving, txLoading, txSave, txLoad, txSaveN, txQuickSave, txEmptySlot, txSaveDate, txSaveTime, txSavePlayers, txSaveName;
	bool save_mode, online;
	SaveSlot* slots;
	int choice;
	TEX tMiniSave;
	TextBox textbox;
};
