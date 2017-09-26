#pragma once

class NetStats : public SingletonPtr<NetStats>
{
public:
	void Initialize();
	void Update();

private:
	struct Stats
	{
		static const uint MIN_SIZE = sizeof(UUID) + sizeof(int) * 4 + sizeof(uint) + sizeof(float) + 2;

		UUID uid;
		int cpu_flags, vs_ver, ps_ver, winver, ver;
		uint drive_id;
		float ram;
		vector<string> gpus;
		string cpu_name;

		bool operator = (const Stats& stats);
		int operator == (const Stats& stats) const;
	};

	void InitializeCpu(Stats& s);
	void InitializeMemory(Stats& s);
	void InitializeVersion(Stats& s);
	void InitializeDrive(Stats& s);
	void InitializeShaders(Stats& s);
	void Load();
	void Save();
	void CheckMatch();
	void UpdateAsync();
	bool Send();

	Stats current;
	vector<Stats> stats;
	bool have_changes;
};
