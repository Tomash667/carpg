#pragma once

struct Update
{
	int ver;
	string path;
	uint crc;
};

int GetCurrentVersion(cstring exe);
cstring VersionToString(int version);
bool GetVersion(int version, int& newVersion, bool& canUpdate);
bool GetUpdate(int version, vector<Update>& updates);
bool DownloadFile(const string& path, const string& filename, uint crc);
bool RunInstallScripts();
