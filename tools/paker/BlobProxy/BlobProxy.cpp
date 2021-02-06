#define PROXY_EXPORT
#include "BlobProxy.h"
#include <msclr\marshal_cppstd.h>

std::string gResult;

const char* AddChange(const char* ver, const char* prevVer, const char* path, unsigned int crc, bool recreate)
{
	Blob::BlobWorker^ worker = gcnew Blob::BlobWorker();
	System::String^ result = worker->Work(gcnew System::String(ver), gcnew System::String(prevVer), gcnew System::String(path), crc, recreate);
	if(result == nullptr)
		return nullptr;
	else
	{
		gResult = msclr::interop::marshal_as<std::string>(result);
		return gResult.c_str();
	}
}
