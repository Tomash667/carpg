// czasomierz
#include "Pch.h"
#include "Timer.h"

//=================================================================================================
// Konstruktor zegara, uruchamia jeœli podano taki argument
//=================================================================================================
Timer::Timer(bool start) : started(false)
{
	if(start)
		Start();
}

//=================================================================================================
// Sprawdza czy mo¿na u¿ywaæ HPC, zeruje
//=================================================================================================
void Timer::Start()
{
	LARGE_INTEGER qwTicksPerSec = {0};
	use_hpc = (QueryPerformanceFrequency(&qwTicksPerSec) != 0);

	if(use_hpc)
	{
		LARGE_INTEGER qwTime;

		ticks_per_sec = (double)qwTicksPerSec.QuadPart;
		QueryPerformanceCounter(&qwTime);
		last_time = qwTime.QuadPart;
	}
	else
		old_time = GetTickCount() / 1000.f;

	started = true;
}

//=================================================================================================
// Kolejna klatka, oblicza delte czasu
//=================================================================================================
float Timer::Tick()
{
	assert(started);

	float delta;

	if(use_hpc)
	{
		LARGE_INTEGER qwTime;

		QueryPerformanceCounter(&qwTime);
		delta = (float)((double)(qwTime.QuadPart - last_time) / ticks_per_sec);
		last_time = qwTime.QuadPart;
	}
	else
	{
		float new_time = GetTickCount() / 1000.0f;
		delta = new_time - old_time;
		old_time = new_time;
	}

	if(delta < 0)
		delta = 0;
	
	return delta;
}

//=================================================================================================
// Resetuje zegar
//=================================================================================================
void Timer::Reset()
{
	if(use_hpc)
	{
		LARGE_INTEGER qwTime;

		QueryPerformanceCounter(&qwTime);
		last_time = qwTime.QuadPart;
	}
	else
		old_time = GetTickCount() / 1000.f;
}
