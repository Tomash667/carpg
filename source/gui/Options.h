#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <CheckBox.h>
#include <Button.h>
#include <ListBox.h>
#include <Scrollbar.h>

//-----------------------------------------------------------------------------
// Options panel (resolution, language, graphics settings etc)
class Options : public DialogBox
{
public:
	enum Id
	{
		IdFullscreen = GuiEvent_Custom,
		IdGlow,
		IdNormal,
		IdSpecular,
		IdVsync,
		IdChangeRes,
		IdOk,
		IdControls,
		IdSoundVolume,
		IdMusicVolume,
		IdMouseSensitivity,
		IdGrassRange
	};

	explicit Options(const DialogInfo& info);
	void LoadLanguage();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	void SetOptions();
	void SetSoundDevices();
	void OnChangeRes(int res_id);
	void OnChangeMultisampling(int id);
	void OnChangeLanguage(int id);
	void ChangeLanguage(int id);
	void OnChangeSoundDevice(int id);

	CheckBox check[5];
	ListBox res, multisampling, language, soundDevice;
	cstring txTitle, txResolution, txMultisampling, txLanguage, txMultisamplingError, txNeedRestart, txSoundVolume, txMusicVolume, txMouseSensitivity,
		txGrassRange, txSoundDevice, txDefaultDevice;
	string languageId;
	Scrollbar scroll[4];
	int soundVolume, musicVolume, mouseSensitivity, grassRange;
};
