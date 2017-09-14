#include "Pch.h"
#include "Core.h"
#include "Utility.h"

//-----------------------------------------------------------------------------
static cstring MUTEX_NAME = "CaRpgMutex";
static cstring SHARED_MEMORY_NAME = "CaRpg-DelayLockMemory";
static HANDLE mutex;
static HANDLE shmem;
static int* mem;
static int app_id = 1;

//=================================================================================================
void utility::InitDelayLock()
{
	mutex = CreateMutex(nullptr, false, MUTEX_NAME);
	if(!mutex)
	{
		Error("Failed to create delay mutex (%d).", GetLastError());
		return;
	}

	shmem = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(int), SHARED_MEMORY_NAME);
	if(!shmem)
	{
		Error("Failed to create shared memory (%d).", GetLastError());
		CloseHandle(mutex);
		mutex = nullptr;
		return;
	}

	mem = (int*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if(!mem)
	{
		Error("Failed to map view of file (%d).", GetLastError());
		CloseHandle(shmem);
		CloseHandle(mutex);
		mutex = nullptr;
		return;
	}

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

	mutex = CreateMutex(nullptr, false, MUTEX_NAME);
	if(!mutex)
	{
		Error("Failed to create delay mutex (%d).", GetLastError());
		return;
	}

	shmem = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(int), SHARED_MEMORY_NAME);
	if(!shmem)
	{
		Error("Failed to create shared memory (%d).", GetLastError());
		CloseHandle(mutex);
		mutex = nullptr;
		return;
	}

	mem = (int*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if(!mem)
	{
		Error("Failed to map view of file (%d).", GetLastError());
		CloseHandle(shmem);
		CloseHandle(mutex);
		mutex = nullptr;
		return;
	}

	Info("Waiting for delay mutex.");
	bool wait = true;
	while(wait)
	{
		WaitForSingleObject(mutex, INFINITE);
		if(*mem == delay)
			wait = false;
		ReleaseMutex(mutex);
		Sleep(250);
	}
}

//=================================================================================================
int utility::GetAppId()
{
	return app_id;
}
