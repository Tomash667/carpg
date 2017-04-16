#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "CheckBox.h"
#include "Button.h"
#include "ListBox.h"
//#include "TabControl.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
class WndRes;

//-----------------------------------------------------------------------------
class Options : public Dialog
{
public:
	enum Id
	{
		IdFullscreen = GuiEvent_Custom,
		IdGlow,
		IdNormal,
		IdSpecular,
		IdChangeRes,
		IdOk,
		IdControls,
		IdSoundVolume,
		IdMusicVolume,
		IdMouseSensitivity,
		IdGrassRange
	};

	explicit Options(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void OnChangeRes(int res_id);
	void OnChangeMultisampling(int id);
	void OnChangeLanguage(int id);
	void ChangeLanguage(int id);

	// new events
	//void OpenControls();

	CheckBox check[4];
	ListBox res, multisampling, language;
	cstring txOPTIONS, txResolution, txMultisampling, txLanguage, txMultisamplingError, txNeedRestart, txSoundVolume, txMusicVolume, txMouseSensitivity, txGrassRange;
	string language_id;
	//TabControl tabControl;
	Scrollbar scroll[4];
	int sound_volume, music_volume, mouse_sensitivity, grass_range;
};
