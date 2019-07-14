#pragma once

//-----------------------------------------------------------------------------
class Timer
{
public:
	explicit Timer(bool start = true);

	void Start();
	float Tick();
	void Reset();

	void GetTime(int64& time) const { time = last_time; }
	double GetTicksPerSec() const { return ticks_per_sec; }
	bool IsStarted() const { return started; }

private:
	double ticks_per_sec;
	int64 last_time;
	bool started;
};
