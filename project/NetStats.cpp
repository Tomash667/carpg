#include "Pch.h"
#include "Core.h"
#include "NetStats.h"

struct CpuFlags
{
	enum E
	{
		X64 = 1 << 0,
		MMX = 1 << 1,
		SSE = 1 << 2,
		SSE2 = 1 << 3,
		SSE3 = 1 << 4,
		SSSE3 = 1 << 5,
		SSE41 = 1 << 6,
		SSE42 = 1 << 7,
		SSE4a = 1 << 8,
		AVX = 1 << 9,
		XOP = 1 << 10,
		FMA3 = 1 << 11,
		FMA4 = 1 << 12
	};
};

void NetStats::Initialize(int adapter)
{

}

void NetStats::InitializeCpu()
{
	bool x64 = false;
	bool MMX = false;
	bool SSE = false;
	bool SSE2 = false;
	bool SSE3 = false;
	bool SSSE3 = false;
	bool SSE41 = false;
	bool SSE42 = false;
	bool SSE4a = false;
	bool AVX = false;
	bool XOP = false;
	bool FMA3 = false;
	bool FMA4 = false;

	int info[4];
	__cpuid(info, 0);
	int nIds = info[0];

	__cpuid(info, 0x80000000);
	int nExIds = info[0];

	// Detect Instruction Set
	if(nIds >= 1)
	{
		__cpuid(info, 0x00000001);
		MMX = (info[3] & ((int)1 << 23)) != 0;
		SSE = (info[3] & ((int)1 << 25)) != 0;
		SSE2 = (info[3] & ((int)1 << 26)) != 0;
		SSE3 = (info[2] & ((int)1 << 0)) != 0;

		SSSE3 = (info[2] & ((int)1 << 9)) != 0;
		SSE41 = (info[2] & ((int)1 << 19)) != 0;
		SSE42 = (info[2] & ((int)1 << 20)) != 0;

		AVX = (info[2] & ((int)1 << 28)) != 0;
		FMA3 = (info[2] & ((int)1 << 12)) != 0;
	}

	if(nExIds >= 0x80000001)
	{
		__cpuid(info, 0x80000001);
		x64 = (info[3] & ((int)1 << 29)) != 0;
		SSE4a = (info[2] & ((int)1 << 6)) != 0;
		FMA4 = (info[2] & ((int)1 << 16)) != 0;
		XOP = (info[2] & ((int)1 << 11)) != 0;
	}

	cpu_flags = 0;
	if(x64)
		cpu_flags |= CpuFlags::X64;
	if(MMX)
		cpu_flags |= CpuFlags::MMX;
	if(SSE)
		cpu_flags |= CpuFlags::SSE;
	if(SSE2)
		cpu_flags |= CpuFlags::SSE2;
	if(SSE3)
		cpu_flags |= CpuFlags::SSE3;
	if(SSSE3)
		cpu_flags |= CpuFlags::SSSE3;
	if(SSE41)
		cpu_flags |= CpuFlags::SSE41;
	if(SSE42)
		cpu_flags |= CpuFlags::SSE42;
	if(SSE4a)
		cpu_flags |= CpuFlags::SSE4a;
	if(AVX)
		cpu_flags |= CpuFlags::AVX;
	if(XOP)
		cpu_flags |= CpuFlags::XOP;
	if(FMA3)
		cpu_flags |= CpuFlags::FMA3;
	if(FMA4)
		cpu_flags |= CpuFlags::FMA4;
}

void NetStats::InitializeMemory()
{
	ULONGLONG mem_kb;
	bool ok = false;

	// first try to use better function (Window Vista+)
	HMODULE kernel32 = LoadLibraryW(L"kernel32");
	if(kernel32 != nullptr)
	{
		auto f = reinterpret_cast<decltype(GetPhysicallyInstalledSystemMemory)*>(GetProcAddress(kernel32, "GetPhysicallyInstalledSystemMemory"));
		if(f)
		{
			f(&mem_kb);
			ok = true;
		}
	}

	// fallback function, return lower values
	if(!ok)
	{
		MEMORYSTATUSEX mem = { 0 };
		mem.dwLength = sizeof(mem);
		if(GlobalMemoryStatusEx(&mem) == 0)
			mem_kb = 0;
		else
			mem_kb = mem.ullTotalPhys / 1024;
	}

	auto mem_mb = mem_kb / 1024;
	ram = float(mem_mb) / 1024;
}

void NetStats::InitializeVersion()
{
	DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuild = 0;

	dwVersion = GetVersion();
	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

	winver = dwMinorVersion | (dwMajorVersion << 16);
}

void NetStats::InitializeShaders(int adapter)
{
	auto d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Engine: Failed to create direct3d object.";

	D3DCAPS9 caps;
	d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);

	vs_ver = caps.VertexShaderVersion;
	ps_ver = caps.PixelShaderVersion;
}
