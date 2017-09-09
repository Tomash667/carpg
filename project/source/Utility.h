#pragma once

//-----------------------------------------------------------------------------
namespace utility
{
	void InitDelayLock();
	void IncrementDelayLock();
	void WaitForDelayLock(int delay);
}
