#pragma once

//-----------------------------------------------------------------------------
// Typy ramek
enum FRAME_INDEX
{
	F_CAST,
	F_TAKE_WEAPON,
	F_ATTACK1_START,
	F_ATTACK1_END,
	F_ATTACK2_START,
	F_ATTACK2_END,
	F_ATTACK3_START,
	F_ATTACK3_END,
	F_BASH,
	F_MAX
};

//-----------------------------------------------------------------------------
// Flags for which weapon use which attack animation
enum WeaponAnimationFlags
{
	A_SHORT_BLADE = 1 << 0,
	A_LONG_BLADE = 1 << 1,
	A_BLUNT = 1 << 2,
	A_AXE = 1 << 3
};

//-----------------------------------------------------------------------------
// Informacje o animacjach ataku postaci
struct AttackFrameInfo
{
	struct Entry
	{
		float start, end;
		int flags;

		float Lerp() const
		{
			return ::Lerp(start, end, 2.f / 3);
		}
	};
	vector<Entry> e;
};

//-----------------------------------------------------------------------------
// Informacje o ramce animacji
struct FrameInfo
{
	string id;
	AttackFrameInfo* extra;
	float t[F_MAX];
	int attacks;

	FrameInfo() : extra(nullptr), attacks(0), t() {}
	~FrameInfo()
	{
		delete extra;
	}

	float Lerp(int frame) const
	{
		return ::Lerp(t[frame], t[frame + 1], 2.f / 3);
	}

	static vector<FrameInfo*> frames;
	static FrameInfo* TryGet(Cstring id);
};
