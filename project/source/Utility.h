#pragma once

//-----------------------------------------------------------------------------
namespace utility
{
	bool InitMutex();
	void InitDelayLock();
	void IncrementDelayLock();
	void WaitForDelayLock(int delay);
	int GetAppId();
}
