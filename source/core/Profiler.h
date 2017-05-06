#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"

//-----------------------------------------------------------------------------
class Profiler
{
public:
	struct Entry;

	Profiler();
	~Profiler();

	void Start();
	void End();
	void Push(cstring name);
	void Pop();
	bool IsStarted() const
	{
		return started;
	}
	const string& GetString() const
	{
		return str;
	}
	void Clear();

	static Profiler g_profiler;

private:
	void Print(Entry* e, int tab);

	bool started, enabled;
	Timer timer;
	Entry* e, *prev_e;
	vector<Entry*> stac;
	string str;
	int frames;
};

//-----------------------------------------------------------------------------
class ProfilerBlock
{
public:
	explicit ProfilerBlock(cstring name);
	~ProfilerBlock();

private:
	bool on;
};

//-----------------------------------------------------------------------------
#define PROFILER_BLOCK(name) ProfilerBlock JOIN(_block,__COUNTER__)##(name)
