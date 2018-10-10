#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "Button.h"
#include "SaveSlot.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
class SaveLoad : public GameDialogBox
{
public:
	enum Id
	{
		IdOk = GuiEvent_Custom,
		IdCancel
	};

	explicit SaveLoad(const DialogInfo& info);
	void LoadLanguage();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void SetSaveMode(bool save_mode, bool online, SaveSlot* slots);
	void SetText();
	void UpdateSaveInfo(int slot);
	void RemoveHardcoreSave(int slot);
	void LoadSaveSlots();
	void ShowSavePanel();
	void ShowLoadPanel();
	bool TryLoad(int slot, bool quickload = false);

private:
	SaveSlot single_saves[MAX_SAVE_SLOTS], multi_saves[MAX_SAVE_SLOTS];
	SaveSlot* slots;
	TextBox textbox;
	Button bt[2];
	int choice;
	cstring txSaving, txLoading, txSave, txLoad, txSaveN, txQuickSave, txEmptySlot, txSaveDate, txSaveTime, txSavePlayers, txSaveName, txSavedGameN,
		txLoadError, txLoadErrorGeneric;
	TEX tMiniSave;
	string save_input_text, hardcore_savename;
	bool save_mode, online;
};
