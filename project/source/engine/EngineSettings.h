#pragma once

struct EngineSettings
{
	int adapter;
	int shader_version;

	EngineSettings() : adapter(0), shader_version(-1) {}
};
