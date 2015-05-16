// czasomierz
#pragma once

//-----------------------------------------------------------------------------
typedef unsigned int uint;

//-----------------------------------------------------------------------------
class Timer
{
public:
	Timer(bool start=true);

	void Start();
	float Tick();
	void Reset();

	inline void GetTime(LONGLONG& time) const { time = last_time; }
	inline double GetTicksPerSec() const { return ticks_per_sec; }

private:
	double ticks_per_sec;
	LONGLONG last_time;
	float old_time;
	bool use_hpc;
};
