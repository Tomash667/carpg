#include "Pch.h"
#include "Base.h"
#include "Multiplayer.h"
#include "Stream.h"

Multiplayer MP;
cstring Multiplayer::PLAYER_DATA_FILE = "player_data";

bool Multiplayer::LoadPlayerGuid(const string& path)
{
	player_guid_path = path;

	bool have_guid = false,
		revert = false,
		ok;

	StreamReader f(path);
	f >> player_guid;
	ok = f.Ok();
	f.Close();

	if(!ok)
	{
		LOG("Missing player guid.");
		CoCreateGuid(&player_guid);

		StreamWriter f2(path);
		if(!f2)
		{
			ERROR(Format("Failed to open file '%s' (%d), reverting to default.", path.c_str(), f2.GetError()));
			f2.Open(PLAYER_DATA_FILE);
			if(!f2)
			{
				ERROR(Format("Failed to open file '%s' (%d), reverting to default.", PLAYER_DATA_FILE, f2.GetError()));
				return true;
			}
			revert = true;
		}

		f2 << player_guid;
	}

	return !revert;
}
