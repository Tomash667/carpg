#pragma once

//-----------------------------------------------------------------------------
class CriticalSection
{
	friend class StartCriticalSection;
	typedef void* Handle;

public:
	CriticalSection() : handle(nullptr) {}
	void Create(uint spin_count = 50);
	void Free();
	void Enter();
	void Leave();

private:
	Handle handle;
};

//-----------------------------------------------------------------------------
class StartCriticalSection
{
public:
	StartCriticalSection(CriticalSection& cs) : cs(cs)
	{
		cs.Enter();
	}
	~StartCriticalSection()
	{
		cs.Leave();
	}
private:
	CriticalSection& cs;
};
