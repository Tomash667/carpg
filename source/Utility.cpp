#include "Pch.h"
#include "Utility.h"

#include <Timer.h>

//-----------------------------------------------------------------------------
static cstring MUTEX_NAME = "CaRpgMutex";
static cstring SHARED_MEMORY_NAME = "CaRpg-DelayLockMemory";
static HANDLE mutex;
static HANDLE shmem;
static int* mem;
static int app_id = 1;
static string compile_time;

//=================================================================================================
bool utility::InitMutex()
{
	mutex = CreateMutex(nullptr, false, MUTEX_NAME);
	if(!mutex)
	{
		Error("Failed to create delay mutex (%d).", GetLastError());
		return false;
	}

	shmem = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(int), SHARED_MEMORY_NAME);
	if(!shmem)
	{
		Error("Failed to create shared memory (%d).", GetLastError());
		CloseHandle(mutex);
		mutex = nullptr;
		return false;
	}

	mem = (int*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if(!mem)
	{
		Error("Failed to map view of file (%d).", GetLastError());
		CloseHandle(shmem);
		CloseHandle(mutex);
		mutex = nullptr;
		return false;
	}

	return true;
}

//=================================================================================================
void utility::InitDelayLock()
{
	if(!InitMutex())
		return;

	WaitForSingleObject(mutex, INFINITE);
	*mem = 1;
	ReleaseMutex(mutex);
	Info("Created delay mutex.");
}

//=================================================================================================
void utility::IncrementDelayLock()
{
	if(!mutex)
		return;

	Info("Signaling delay mutex.");
	WaitForSingleObject(mutex, INFINITE);
	*mem += 1;
	ReleaseMutex(mutex);

	UnmapViewOfFile(mem);
	CloseHandle(shmem);
	CloseHandle(mutex);
	mutex = nullptr;
}

//=================================================================================================
void utility::WaitForDelayLock(int delay)
{
	app_id = delay;

	if(!InitMutex())
		return;

	Info("Waiting for delay mutex.");
	Timer t;
	float time = 0.f;
	int last = 0;
	while(true)
	{
		WaitForSingleObject(mutex, INFINITE);
		int value = *mem;
		ReleaseMutex(mutex);
		if(value == delay)
			break;
		else
		{
			time += t.Tick();
			if(last != value)
			{
				last = value;
				time = 0.f;
			}
			else if(time >= (last == 0 ? 10.f : 120.f))
			{
				Info("Delay lock timeout.");
				break;
			}
			Sleep(250);
		}
	}
}

//=================================================================================================
int utility::GetAppId()
{
	return app_id;
}

//=================================================================================================
const string& utility::GetCompileTime()
{
	if(!compile_time.empty())
		return compile_time;

	int len = GetModuleFileName(nullptr, BUF, 256);
	HANDLE file;

	if(len == 256)
	{
		char* b = new char[2048];
		GetModuleFileName(nullptr, b, 2048);
		file = CreateFile(b, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		delete[] b;
	}
	else
		file = CreateFile(BUF, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if(file == INVALID_HANDLE_VALUE)
	{
		compile_time = "0";
		return compile_time;
	}

	// read header position
	int offset;
	DWORD tmp;
	SetFilePointer(file, 0x3C, nullptr, FILE_BEGIN);
	ReadFile(file, &offset, sizeof(offset), &tmp, nullptr);
	SetFilePointer(file, offset + 8, nullptr, FILE_BEGIN);

	// read time
	static_assert(sizeof(time_t) == 8, "time_t must be 64 bit");
	union TimeUnion
	{
		time_t t;
		struct
		{
			uint low;
			uint high;
		};
	};
	TimeUnion datetime = { 0 };
	ReadFile(file, &datetime.low, sizeof(datetime.low), &tmp, nullptr);

	CloseHandle(file);

	tm t;
	errno_t err = gmtime_s(&t, &datetime.t);
	if(err == 0)
	{
		strftime(BUF, 256, "%Y-%m-%d %H:%M:%S", &t);
		compile_time = BUF;
	}
	else
		compile_time = "0";

	return compile_time;
}
