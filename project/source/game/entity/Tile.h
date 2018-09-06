#pragma once

//-----------------------------------------------------------------------------
// Rodzaj pola mapy
enum POLE : byte
{
	NIEUZYTE,
	PUSTE,
	SCHODY_GORA,
	SCHODY_DOL,
	DRZWI,
	OTWOR_NA_DRZWI,
	KRATKA_PODLOGA,
	KRATKA_SUFIT,
	KRATKA,
	SCIANA,
	BLOKADA,
	BLOKADA_SCIANA,
	ZAJETE
};

//-----------------------------------------------------------------------------
// Struktura opisuj¹ca pole na mapie
struct Pole
{
	//-----------------------------------------------------------------------------
	// Flagi pola
	enum FLAGI
	{
		F_PODLOGA = 0x1,
		F_SUFIT = 0x2,
		F_NISKI_SUFIT = 0x4,
		F_KRATKA_PODLOGA = 0x8,
		F_KRATKA_SUFIT = 0x10,

		// unused 0x20 0x40 0x80

		F_SCIANA_LEWA = 0x100,
		F_SCIANA_PRAWA = 0x200,
		F_SCIANA_PRZOD = 0x400,
		F_SCIANA_TYL = 0x800,

		F_PODSUFIT_LEWA = 0x1000,
		F_PODSUFIT_PRAWA = 0x2000,
		F_PODSUFIT_PRZOD = 0x4000,
		F_PODSUFIT_TYL = 0x8000,

		F_GORA_LEWA = 0x10000,
		F_GORA_PRAWA = 0x20000,
		F_GORA_PRZOD = 0x40000,
		F_GORA_TYL = 0x80000,

		F_DZIURA_LEWA = 0x100000,
		F_DZIURA_PRAWA = 0x200000,
		F_DZIURA_PRZOD = 0x400000,
		F_DZIURA_TYL = 0x800000, // 1<<21

		F_SPECJALNE = 1 << 29, // póki co u¿ywane do oznaczenia drzwi do wiêzienia
		F_DRUGA_TEKSTURA = 1 << 30,
		F_ODKRYTE = 1 << 31
	};

	int flags;
	word room;
	POLE type;
	// jeszcze jest 1-2 bajty miejsca na coœ :o (jak pokój bêdzie byte)

	// DDDDGGGGRRRRSSSS000KKNSP
	// ####    ####    ########
	//     ####    ####
	// œciany >> 8
	// podsufit >> 12
	// góra >> 16
	// dó³ >> 20

	bool IsWall() const
	{
		return type == SCIANA || type == BLOKADA_SCIANA;
	}

	static void SetupFlags(Pole* tiles, const Int2& size);
	static void DebugDraw(Pole* tiles, const Int2& size);
};

//-----------------------------------------------------------------------------
// Czy pole blokuje ruch
inline bool czy_blokuje2(POLE p)
{
	return p >= SCIANA || p == NIEUZYTE;
}
inline bool czy_blokuje2(const Pole& p)
{
	return czy_blokuje2(p.type);
}
inline bool czy_blokuje21(POLE p)
{
	// czy to pole blokuje ruch albo nie powinno na nim byæ obiektów bo zablokujê droge
	return !(p == PUSTE || p == KRATKA || p == KRATKA_PODLOGA || p == KRATKA_SUFIT);
}
inline bool czy_blokuje21(const Pole& p)
{
	return czy_blokuje21(p.type);
}
