#include "Pch.h"
#include "Core.h"
#include "NetStats.h"
#include "Crc.h"
#include "Version.h"
#include "Base64.h"
#include <slikenet/HTTPConnection.h>
#include <slikenet/TCPInterface.h>
#include <process.h>
#include <Wincrypt.h>
#include <Lmcons.h>

NetStats* SingletonPtr<NetStats>::instance;
static volatile bool shutdown_thread;

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

bool NetStats::Stats::operator = (const NetStats::Stats& stats)
{
	bool changes = false;

	if(user_name != stats.user_name)
	{
		user_name = stats.user_name;
		changes = true;
	}
	if(cpu_flags != stats.cpu_flags)
	{
		cpu_flags = stats.cpu_flags;
		changes = true;
	}
	if(vs_ver != stats.vs_ver)
	{
		vs_ver = stats.vs_ver;
		changes = true;
	}
	if(ps_ver != stats.ps_ver)
	{
		ps_ver = stats.ps_ver;
		changes = true;
	}
	if(winver != stats.winver)
	{
		winver = stats.winver;
		changes = true;
	}
	if(ver != stats.ver)
	{
		ver = stats.ver;
		changes = true;
	}
	if(drive_id != stats.drive_id)
	{
		drive_id = stats.drive_id;
		changes = true;
	}
	if(ram != stats.ram)
	{
		ram = stats.ram;
		changes = true;
	}
	if(gpus != stats.gpus)
	{
		gpus = stats.gpus;
		changes = true;
	}
	if(cpu_name != stats.cpu_name)
	{
		cpu_name = stats.cpu_name;
		changes = true;
	}

	return changes;
}

int NetStats::Stats::operator == (const NetStats::Stats& stats) const
{
	int dif = 0;
	if(user_name != stats.user_name)
		++dif;
	if(cpu_name != stats.cpu_name)
		++dif;
	if(ram != stats.ram)
		++dif;
	if(drive_id != stats.drive_id)
		++dif;
	if(winver != stats.winver)
		++dif;
	bool gpu_match = false;
	for(auto& gpu : gpus)
	{
		for(auto& gpu2 : stats.gpus)
		{
			if(gpu == gpu2)
				gpu_match = true;
		}
	}
	if(!gpu_match)
		++dif;
	return dif;
}

void NetStats::Stats::CalculateHash()
{
	HCRYPTPROV crypt_prov;
	CryptAcquireContext(&crypt_prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);

	HCRYPTHASH hasher;
	CryptCreateHash(crypt_prov, CALG_MD5, 0, 0, &hasher);

	string to_hash = Format("%s;%s;%d;%u;%d", user_name.c_str(), cpu_name.c_str(), int(ram * 10), drive_id, winver);
	for(auto& gpu : gpus)
	{
		to_hash += gpu;
		to_hash += ';';
	}

	CryptHashData(hasher, (const byte*)to_hash.c_str(), to_hash.length(), 0);

	byte hash[16];
	DWORD hash_length = 16;
	CryptGetHashParam(hasher, HP_HASHVAL, hash, &hash_length, 0);

	CryptDestroyHash(hasher);
	CryptReleaseContext(crypt_prov, 0);

	char chars[] = "0123456789abcdef";
	uid.reserve(32);
	for(int i = 0; i < 16; ++i)
	{
		uid.push_back(chars[hash[i] >> 4]);
		uid.push_back(chars[hash[i] & 0xF]);
	}
}

void NetStats::Initialize()
{
	started = false;
	have_changes = false;
	Load();
	current.ver = VERSION;
	InitializeUserName(current);
	InitializeCpu(current);
	InitializeMemory(current);
	InitializeVersion(current);
	InitializeDrive(current);
	InitializeShaders(current);
	CheckMatch();
}

void NetStats::TryUpdate()
{
	NetStats* inst = NetStats::TryGet();
	if(inst)
		inst->Update();
}

void NetStats::Update()
{
	if(!have_changes)
		NetStats::Free();
	else
	{
		started = true;
		_beginthreadex(nullptr, 1024, [](void*) -> uint
		{
			NetStats::Get().UpdateAsync();
			return 0u;
		}, nullptr, 0, nullptr);
	}
}

void NetStats::Close()
{
	NetStats* inst = NetStats::TryGet();
	if(inst)
	{
		if(inst->started)
		{
			shutdown_thread = true;
			for(int i = 0; i < 20; ++i)
			{
				Sleep(50);
				inst = NetStats::TryGet();
				if(!inst)
					break;
			}
		}
		else
			NetStats::Free();
	}
}

void NetStats::InitializeUserName(Stats& s)
{
	char buf[UNLEN + 1];
	DWORD len = UNLEN + 1;
	GetUserName(buf, &len);
	s.user_name = buf;
}

void NetStats::InitializeCpu(Stats& s)
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

	int cpu_flags = 0;
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
	s.cpu_flags = cpu_flags;

	int cpuInfo[4] = { -1 };
	char CPUBrandString[0x40];

	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	__cpuid(cpuInfo, 0x80000002);
	memcpy(CPUBrandString, cpuInfo, sizeof(cpuInfo));

	__cpuid(cpuInfo, 0x80000003);
	memcpy(CPUBrandString + 16, cpuInfo, sizeof(cpuInfo));

	__cpuid(cpuInfo, 0x80000004);
	memcpy(CPUBrandString + 32, cpuInfo, sizeof(cpuInfo));

	s.cpu_name = CPUBrandString;
	Trim(s.cpu_name);
}

void NetStats::InitializeMemory(Stats& s)
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
	s.ram = float(mem_mb) / 1024;
}

void NetStats::InitializeVersion(Stats& s)
{
	DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;

	dwVersion = GetVersion();
	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

	s.winver = dwMinorVersion | (dwMajorVersion << 16);
}

void NetStats::InitializeDrive(Stats& s)
{
	char windows_drive[MAX_PATH];
	GetWindowsDirectory(windows_drive, MAX_PATH);

	DWORD serial;
	GetVolumeInformation(Format("%c:\\", windows_drive[0]), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);

	s.drive_id = serial;
}

void NetStats::InitializeShaders(Stats& s)
{
	auto d3d = Direct3DCreate9(D3D_SDK_VERSION);
	assert(d3d);

	uint adapters = d3d->GetAdapterCount();

	s.vs_ver = 0;
	s.ps_ver = 0;
	s.gpus.clear();

	D3DADAPTER_IDENTIFIER9 info;
	D3DCAPS9 caps;
	for(uint i = 0; i < adapters; ++i)
	{
		d3d->GetAdapterIdentifier(i, 0, &info);
		s.gpus.push_back(info.Description);

		d3d->GetDeviceCaps(i, D3DDEVTYPE_HAL, &caps);
		int new_vs_ver = (caps.VertexShaderVersion & 0xFFFF);
		int new_ps_ver = (caps.PixelShaderVersion & 0xFFFF);
		if(new_vs_ver > s.vs_ver)
			s.vs_ver = new_vs_ver;
		if(new_ps_ver > s.ps_ver)
			s.ps_ver = new_ps_ver;
	}

	std::sort(s.gpus.begin(), s.gpus.end());

	d3d->Release();
}

void NetStats::Load()
{
	io2::FileReader f("carpg.bin");
	if(!f)
		return;

	uint ver, count;
	f >> ver;
	f >> count;
	if(!f || ver > 1 || !f.Ensure(count * Stats::MIN_SIZE))
	{
		Error("NetStats: Broken file 'carpg.bin'.");
		return;
	}
	if(ver == 0)
		return;

	Crc crc;

	stats.resize(count);
	for(Stats& stat : stats)
	{
		f >> stat.ouid;
		f >> stat.uid;
		f >> stat.user_name;
		f >> stat.cpu_flags;
		f >> stat.vs_ver;
		f >> stat.ps_ver;
		f >> stat.winver;
		f >> stat.drive_id;
		f >> stat.ram;
		f >> stat.gpus;
		f >> stat.cpu_name;
		f >> stat.ver;

		crc.Update(stat.ouid);
		crc.Update(stat.uid);
		crc.Update(stat.user_name);
		crc.Update(stat.cpu_flags);
		crc.Update(stat.vs_ver);
		crc.Update(stat.ps_ver);
		crc.Update(stat.winver);
		crc.Update(stat.drive_id);
		crc.Update(stat.ram);
		crc.Update(stat.gpus);
		crc.Update(stat.cpu_name);
		crc.Update(stat.ver);
	}

	uint saved_crc;
	f >> saved_crc;

	if(!f || saved_crc != crc.Get())
	{
		Error("NetStats: Broken file content 'carpg.bin'.");
		stats.clear();
	}
}

void NetStats::Save()
{
	io2::FileWriter f("carpg.bin");
	f << 1;
	f << stats.size();

	Crc crc;

	for(auto& stat : stats)
	{
		f << stat.ouid;
		f << stat.uid;
		f << stat.user_name;
		f << stat.cpu_flags;
		f << stat.vs_ver;
		f << stat.ps_ver;
		f << stat.winver;
		f << stat.drive_id;
		f << stat.ram;
		f << stat.gpus;
		f << stat.cpu_name;
		f << stat.ver;

		crc.Update(stat.ouid);
		crc.Update(stat.uid);
		crc.Update(stat.user_name);
		crc.Update(stat.cpu_flags);
		crc.Update(stat.vs_ver);
		crc.Update(stat.ps_ver);
		crc.Update(stat.winver);
		crc.Update(stat.drive_id);
		crc.Update(stat.ram);
		crc.Update(stat.gpus);
		crc.Update(stat.cpu_name);
		crc.Update(stat.ver);
	}

	f << crc.Get();
}

void NetStats::CheckMatch()
{
	Stats* best_match = nullptr;
	int best_dif = 2;
	for(Stats& stat : stats)
	{
		int dif = (stat == current);
		if(dif <= 1 && dif < best_dif)
		{
			best_match = &stat;
			best_dif = dif;
		}
	}

	current.CalculateHash();
	if(best_match)
	{
		have_changes = (*best_match = current);
		current.ouid = best_match->ouid;
	}
	else
	{
		current.ouid = current.uid;
		stats.push_back(current);
		have_changes = true;
	}
}

void NetStats::UpdateAsync()
{
	if(Send())
		Save();
	NetStats::Free();
}

bool NetStats::Send()
{
	string data;
	FormatStr(data, R"raw(
{
	"ouid": "%s",
	"uid": "%s",
	"version": %d,
	"winver": %d,
	"ram": %g,
	"cpu_flags": %d,
	"vs_ver": %d,
	"ps_ver": %d
}
)raw", current.ouid.c_str(), current.uid.c_str(), current.ver, current.winver, current.ram, current.cpu_flags, current.vs_ver, current.ps_ver);
	Trim(data);

	string encrypted;
	Base64::Encode(data, &encrypted);
	Replace(encrypted, "+/=", "._-");

	Crc crc;
	crc.Update(encrypted);
	uint c = crc.Get();

	string request_data;
	// 2932966817
	FormatStr(request_data, "data=%s&c=%u", encrypted.c_str(), c);

	TCPInterface* tcp = TCPInterface::GetInstance();
	tcp->Start(0, 1);

	HTTPConnection* http = HTTPConnection::GetInstance();
	http->Init(tcp, "carpg.pl");

	http->Post("/api.php?action=stats", request_data.c_str());

	Timer t;
	t.Start();
	float dt = 0.f;
	bool ok = true;
	while(true)
	{
		if(shutdown_thread)
		{
			Info("NetStats: Shutting down.");
			ok = false;
			break;
		}

		Packet* packet = tcp->Receive();
		if(packet)
		{
			http->ProcessTCPPacket(packet);
			tcp->DeallocatePacket(packet);

			int code;
			if(http->HasBadResponse(&code, nullptr))
			{
				Error("NetStats: Bad server response (%d).", code);
				ok = false;
				break;
			}
			else
			{
				// ok
				break;
			}
		}

		dt += t.Tick();
		if(dt >= 10.f)
		{
			Error("NetStats: Timeout.");
			ok = false;
			break;
		}
		else
		{
			http->Update();
			Sleep(50);
		}
	}

	HTTPConnection::DestroyInstance(http);
	TCPInterface::DestroyInstance(tcp);

	return ok;
}
