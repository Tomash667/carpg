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
	LARGE_INTEGER qwTicksPerSec = { 0 };
	QueryPerformanceFrequency(&qwTicksPerSec);

	LARGE_INTEGER qwTime;
	ticks_per_sec = (double)qwTicksPerSec.QuadPart;
	QueryPerformanceCounter(&qwTime);
	last_time = qwTime.QuadPart;

	started = true;
}

//=================================================================================================
// Kolejna klatka, oblicza delte czasu
//=================================================================================================
float Timer::Tick()
{
	assert(started);

	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	float delta = (float)((double)(qwTime.QuadPart - last_time) / ticks_per_sec);
	last_time = qwTime.QuadPart;

	return delta < 0 ? 0 : delta;
}

//=================================================================================================
// Resetuje zegar
//=================================================================================================
void Timer::Reset()
{
	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	last_time = qwTime.QuadPart;
}
