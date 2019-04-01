#include "Pch.h"
#include "GameCore.h"
#include "Utility.h"
#include "Timer.h"

//-----------------------------------------------------------------------------
static cstring MUTEX_NAME = "CaRpgMutex";
static cstring SHARED_MEMORY_NAME = "CaRpg-DelayLockMemory";
static HANDLE mutex;
static HANDLE shmem;
static int* mem;
static int app_id = 1;

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
