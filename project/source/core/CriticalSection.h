#pragma once

//-----------------------------------------------------------------------------
class CriticalSection
{
	friend class StartCriticalSection;
public:
	CriticalSection() : valid(false)
	{
	}
	void Create(uint spin_count = 50)
	{
		if(!valid)
		{
			InitializeCriticalSectionAndSpinCount(&cs, 50);
			valid = true;
		}
	}
	void Free()
	{
		if(valid)
		{
			DeleteCriticalSection(&cs);
			valid = false;
		}
	}
	void Enter()
	{
		assert(valid);
		EnterCriticalSection(&cs);
	}
	void Leave()
	{
		assert(valid);
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION cs;
	bool valid;
};

//-----------------------------------------------------------------------------
class StartCriticalSection
{
public:
	StartCriticalSection(CRITICAL_SECTION& _cs) : cs(_cs)
	{
		EnterCriticalSection(&cs);
	}
	StartCriticalSection(CriticalSection& _cs) : cs(_cs.cs)
	{
		assert(_cs.valid);
		EnterCriticalSection(&cs);
	}
	~StartCriticalSection()
	{
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION& cs;
};
