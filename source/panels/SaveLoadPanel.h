#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "Button.h"
#include "SaveSlot.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
class SaveLoad : public DialogBox
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
	void RemoveHardcoreSave(int slot);
	void LoadSaveSlots();
	void ShowSavePanel();
	void ShowLoadPanel();
	SaveSlot& GetSaveSlot(int slot);

private:
	void SetSaveImage();

	SaveSlot single_saves[SaveSlot::MAX_SLOTS], multi_saves[SaveSlot::MAX_SLOTS];
	SaveSlot* slots;
	TextBox textbox;
	Button bt[2];
	int choice;
	cstring txSaving, txLoading, txSave, txLoad, txSaveN, txQuickSave, txEmptySlot, txSaveDate, txSaveTime, txSavePlayers, txSaveName, txSavedGameN;
	Texture tMiniSave;
	string save_input_text, hardcore_savename;
	bool save_mode, online;
};
