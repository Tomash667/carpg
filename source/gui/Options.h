#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <CheckBox.h>
#include <Button.h>
#include <ListBox.h>
#include <Scrollbar.h>

//-----------------------------------------------------------------------------
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
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void SetOptions();
	void OnChangeRes(int res_id);
	void OnChangeMultisampling(int id);
	void OnChangeLanguage(int id);
	void ChangeLanguage(int id);

	CheckBox check[5];
	ListBox res, multisampling, language;
	cstring txOPTIONS, txResolution, txMultisampling, txLanguage, txMultisamplingError, txNeedRestart, txSoundVolume, txMusicVolume, txMouseSensitivity,
		txGrassRange;
	string language_id;
	Scrollbar scroll[4];
	int sound_volume, music_volume, mouse_sensitivity, grass_range;
};
