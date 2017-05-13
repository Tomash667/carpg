#pragma once 

class Multiplayer
{
public:
	string player_guid_path;
	GUID player_guid;

	static cstring PLAYER_DATA_FILE;

	bool LoadPlayerGuid(const string& path);
};

extern Multiplayer MP;
