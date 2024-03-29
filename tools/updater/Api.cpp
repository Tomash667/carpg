#include <CarpgLibCore.h>
#include <Config.h>
#include <curl\curl.h>
#include <json.hpp>
#include "Updater.h"

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	Buffer* buf = reinterpret_cast<Buffer*>(userp);

	size_t realsize = size * nmemb;
	buf->Append(contents, realsize);
	return realsize;
}

Buffer* CurlRequest(cstring url)
{
	Config cfg;
	cfg.Load("updater.cfg");
	string lobby_url = cfg.GetString("lobby_url", "carpglobby.westeurope.cloudapp.azure.com");
	int lobby_port = cfg.GetInt("lobby_port", 8080);
	url = Format("%s:%d/api/%s", lobby_url.c_str(), lobby_port, url);

	Buffer* buf = Buffer::Get();
	buf->Clear();
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 4);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

	CURLcode res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		printf("ERROR: Curl request failed.\n");
		curl_easy_cleanup(curl);
		buf->Free();
		return nullptr;
	}

	int code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	curl_easy_cleanup(curl);

	if(code != 200)
	{
		printf("ERROR: Invalid api response code %d.\n", code);
		buf->Free();
		return nullptr;
	}

	return buf;
}

bool GetVersion(int version, int& newVersion, bool& canUpdate)
{
	cstring url = Format("version?ver=%d", version);
	Buffer* buf = CurlRequest(url);
	if(!buf)
		return false;

	cstring content = buf->AsString();
	auto j = nlohmann::json::parse(content);
	buf->Free();
	if(j["ok"] == false)
	{
		string& error = j["error"].get_ref<string&>();
		printf("ERROR: Api response: %s\n", error.c_str());
		return false;
	}

	newVersion = j["version"].get<int>();
	canUpdate = j["update"].get<bool>();
	return true;
}

bool GetUpdate(int version, vector<Update>& updates)
{
	cstring url = Format("update/%d", version);
	Buffer* buf = CurlRequest(url);
	if(!buf)
		return false;

	cstring content = buf->AsString();
	auto j = nlohmann::json::parse(content);
	buf->Free();
	if(j["ok"] == false)
	{
		string& error = j["error"].get_ref<string&>();
		printf("ERROR: Api response: %s\n", error.c_str());
		return false;
	}

	j = j["files"];
	for(auto it = j.begin(), end = j.end(); it != end; ++it)
	{
		auto obj = *it;
		int version = obj["version"].get<int>();
		string path = obj["path"].get<string>();
		uint crc = obj["crc"].get<uint>();
		updates.push_back({ version, path, crc });
	}
	return true;
}
