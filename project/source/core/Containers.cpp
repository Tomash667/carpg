#include "Pch.h"
#include "Core.h"
#ifdef CHECK_POOL_LEAKS
#include "WindowsIncludes.h"
#define IN
#define OUT
#pragma warning(push)
#pragma warning(disable:4091)
#include <Dbghelp.h>
#pragma warning(pop)
#endif
#undef FAR
#ifndef COMMON_ONLY
#include <zlib.h>
#endif

ObjectPool<string> StringPool;
ObjectPool<vector<void*>> VectorPool;
ObjectPool<vector<byte>> BufPool;

#ifdef CHECK_POOL_LEAKS

ObjectPoolLeakManager ObjectPoolLeakManager::instance;

struct ObjectPoolLeakManager::CallStackEntry
{
	static const uint MAX_FRAMES = 32;
	void* frames[MAX_FRAMES];
};

ObjectPoolLeakManager::~ObjectPoolLeakManager()
{
	if(!call_stacks.empty())
	{
		OutputDebugString(Format("!!!!!!!!!!!!!!!!!!!!!!!!!!\nObjectPool leaks detected (%u):\n", call_stacks.size()));

		HANDLE handle = GetCurrentProcess();
		SymInitialize(handle, nullptr, true);
		byte symbol_data[sizeof(SYMBOL_INFO) + 1000] = { 0 };
		SYMBOL_INFO& symbol = *(SYMBOL_INFO*)&symbol_data[0];
		symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol.MaxNameLen = 1000;
		DWORD64 displacement;
		IMAGEHLP_LINE64 line;

		uint index = 0;
		for(auto& pcs : call_stacks)
		{
			OutputDebugString(Format("[%u] Address: %p Call stack:\n", index, pcs.first));
			CallStackEntry& cs = *pcs.second;
			for(uint i = 0; i < CallStackEntry::MAX_FRAMES; ++i)
			{
				SymFromAddr(handle, (DWORD64)cs.frames[i], &displacement, &symbol);
				DWORD disp;
				SymGetLineFromAddr64(handle, (DWORD64)cs.frames[i], &disp, &line);
				OutputDebugString(Format("\t%s (%d): %s\n", line.FileName, line.LineNumber, symbol.Name));
				if(cs.frames[i] == nullptr)
					break;
			}
			delete pcs.second;
			++index;
			OutputDebugString("\n\n");
		}
	}

	DeleteElements(call_stack_pool);
}

void ObjectPoolLeakManager::Register(void* ptr)
{
	assert(ptr);
	assert(call_stacks.find(ptr) == call_stacks.end());

	CallStackEntry* cs;
	if(call_stack_pool.empty())
		cs = new CallStackEntry;
	else
	{
		cs = call_stack_pool.back();
		call_stack_pool.pop_back();
	}

	memset(cs, 0, sizeof(CallStackEntry));

	RtlCaptureStackBackTrace(1, CallStackEntry::MAX_FRAMES, cs->frames, nullptr);

	call_stacks[ptr] = cs;
}

void ObjectPoolLeakManager::Unregister(void* ptr)
{
	assert(ptr);

	auto it = call_stacks.find(ptr);
	assert(it != call_stacks.end());
	call_stack_pool.push_back(it->second);
	call_stacks.erase(it);
}

#endif

//=================================================================================================
#ifndef COMMON_ONLY
Buffer* Buffer::Decompress(uint real_size)
{
	Buffer* buf = Buffer::Get();
	buf->Resize(real_size);
	uLong size = real_size;
	uncompress((Bytef*)buf->Data(), &size, (const Bytef*)Data(), Size());
	Free();
	return buf;
}
#endif
