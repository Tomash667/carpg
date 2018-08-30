#pragma once

class World
{
public:
	void Init();
	void OnNewGame();
	void Update(int days);

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void WriteTime(BitStreamWriter& f);
	void ReadTime(BitStreamReader& f);

	bool IsSameWeek(int worldtime2) const;
	cstring GetDate() const;
	int GetDay() const { return day; }
	int GetMonth() const { return month; }
	int GetWorldtime() const { return worldtime; }
	int GetYear() const { return year; }

private:
	int year; // in game year, starts at 100
	int month; // in game month, 0 to 11
	int day; // in game day, 0 to 29
	int worldtime; // number of passed game days, starts at 0
	cstring txDate;
};
extern World W;
