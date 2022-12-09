#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Button.h>
#include <TextBox.h>
#include "SaveSlot.h"

//-----------------------------------------------------------------------------
// Save & load panel
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
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void SetSaveMode(bool saveMode, bool online, SaveSlot* slots);
	void SetText();
	void RemoveHardcoreSave(int slot);
	void LoadSaveSlots();
	void ShowSavePanel();
	void ShowLoadPanel();
	SaveSlot& GetSaveSlot(int slotIndex, bool isOnline);
	Texture* GetSaveImage(int slotIndex, bool isOnline);
	const string& GetSaveText(SaveSlot& slot);

private:
	void SetSaveImage() { GetSaveImage(choice + 1, online); }
	void ValidateSelectedSave();

	SaveSlot singleSaves[SaveSlot::MAX_SLOTS], multiSaves[SaveSlot::MAX_SLOTS];
	SaveSlot* slots;
	TextBox textbox;
	Button bt[2];
	int choice, hover;
	cstring txSaving, txLoading, txSave, txLoad, txSaveN, txQuickSave, txEmptySlot, txSaveDate, txSaveTime, txSavePlayers, txSaveName, txSavedGameN;
	Texture tMiniSave;
	string saveInputText, hardcoreSavename, saveText;
	bool saveMode, online;
};
