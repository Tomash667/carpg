#pragma once

class NetStats : public Singleton<NetStats>
{
public:
	void Initialize(int adapter);

private:
	void InitializeCpu();
	void InitializeMemory();
	void InitializeVersion();
	void InitializeShaders(int adapter);

	int cpu_flags, vs_ver, ps_ver, winver;
	float ram;
};
