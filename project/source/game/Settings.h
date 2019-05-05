#pragma once

//-----------------------------------------------------------------------------
struct Settings
{
	Settings() : grass_range(40.f) {}

	int mouse_sensitivity;
	float mouse_sensitivity_f;
	float grass_range;

	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys(Config& cfg);
	void LoadGameKeys(Config& cfg);
};
