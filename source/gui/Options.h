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
	void Draw(ControlDrawData* cdd = nullptr) override;
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

	CheckBox check[4];
	ListBox res, multisampling, language, soundDevice;
	cstring txOPTIONS, txResolution, txMultisampling, txLanguage, txMultisamplingError, txNeedRestart, txSoundVolume, txMusicVolume, txMouseSensitivity,
		txGrassRange, txSoundDevice, txDefaultDevice;
	string language_id;
	Scrollbar scroll[4];
	int sound_volume, music_volume, mouse_sensitivity, grass_range;
};
