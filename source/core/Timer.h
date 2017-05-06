// czasomierz
#pragma once

//-----------------------------------------------------------------------------
typedef unsigned int uint;

//-----------------------------------------------------------------------------
class Timer
{
public:
	explicit Timer(bool start = true);

	void Start();
	float Tick();
	void Reset();

	void GetTime(LONGLONG& time) const { time = last_time; }
	double GetTicksPerSec() const { return ticks_per_sec; }
	bool IsStarted() const { return started; }

private:
	double ticks_per_sec;
	LONGLONG last_time;
	float old_time;
	bool use_hpc;
	bool started;
};
