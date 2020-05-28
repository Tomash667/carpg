#pragma once

struct Date
{
	Date() {}
	Date(int year, int month, int day) : year(year), month(month), day(day) {}

	void AddDays(int days)
	{
		day += days;
		if(day >= 30)
		{
			int count = day / 30;
			month += count;
			day -= count * 30;
			if(month >= 12)
			{
				count = month / 12;
				year += count;
				month -= count * 12;
			}
		}
	}

	bool IsValid() const
	{
		return year >= 1 && InRange(month, 0, 11) && InRange(day, 0, 29);
	}

	int year;
	int month; // 0-11
	int day; // 0-29
};
