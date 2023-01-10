#include <CarpgLibCore.h>
#include <File.h>
#include <Crc.h>
#include <curl\curl.h>
#include "Updater.h"

struct MyProgress
{
	const string& filename;
	int prog;
};

static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	FileWriter& f = *reinterpret_cast<FileWriter*>(userp);

	size_t realsize = size * nmemb;
	f.Write(contents, realsize);
	return realsize;
}

static int UpdateProgress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
	MyProgress* myProg = (MyProgress*)p;
	if(dltotal == 0)
		return 0;
	int newProg = (int)(dlnow * 100 / dltotal);
	if(myProg->prog != newProg)
	{
		printf("\rDownloading file %s: %d%%", myProg->filename.c_str(), newProg);
		myProg->prog = newProg;
	}

	return 0;
}

bool DownloadFile(const string& path, const string& filename, uint crc)
{
	if(io::FileExists(filename.c_str()))
	{
		uint fileCrc = Crc::Calculate(filename);
		if(fileCrc == crc)
		{
			printf("File %s already downloaded.\n", filename.c_str());
			return true;
		}
		else
			printf("Existing file %s have invalid crc, redownloading.\n", filename.c_str());
	}

	if(StartsWith(path.c_str(), "file://"))
		CopyFileA(path.c_str() + 7, filename.c_str(), true);
	else
	{
		FileWriter f(filename);
		if(!f)
		{
			printf("ERROR: Failed to create file %s.\n", filename.c_str());
			return false;
		}

		printf("Downloading file %s: 0%%", filename.c_str());

		MyProgress prog{ filename, 0 };
		CURL* curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &f);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, UpdateProgress);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

		CURLcode res = curl_easy_perform(curl);
		f.Close();

		if(res != CURLE_OK)
		{
			printf("\nERROR: Download failed.\n");
			curl_easy_cleanup(curl);
			return false;
		}

		printf("\rDownloading file %s: 100%%\n", filename.c_str());
		curl_easy_cleanup(curl);
	}

	uint fileCrc = Crc::Calculate(filename);
	if(fileCrc != crc)
	{
		printf("ERROR: Invalid file crc.\n");
		return false;
	}

	return true;
}
