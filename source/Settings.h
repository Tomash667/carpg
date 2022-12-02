#pragma once

//-----------------------------------------------------------------------------
struct Settings
{
	Settings() : grassRange(40.f) {}

	int mouseSensitivity;
	float grassRange;

	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys(Config& cfg);
	void LoadGameKeys(Config& cfg);
	float GetMouseSensitivity() const;
};
