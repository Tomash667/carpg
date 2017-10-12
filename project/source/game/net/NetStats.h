#pragma once

class NetStats : public SingletonPtr<NetStats>
{
public:
	void Initialize();
	static void TryUpdate();
	static void Close();

private:
	struct Stats
	{
		static const uint MIN_SIZE = sizeof(int) * 4 + sizeof(uint) + sizeof(float) + 5;

		int cpu_flags, vs_ver, ps_ver, winver, ver;
		uint drive_id;
		float ram;
		vector<string> gpus;
		string ouid, uid, cpu_name, user_name;

		bool operator = (const Stats& stats);
		int operator == (const Stats& stats) const;

		void CalculateHash();
	};

	void Update();
	void InitializeUserName(Stats& s);
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
	bool have_changes, started;
};
