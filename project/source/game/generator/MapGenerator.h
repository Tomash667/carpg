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

//-----------------------------------------------------------------------------
// Room target
enum class RoomTarget
{
	None,
	Corridor,
	StairsUp,
	StairsDown,
	Treasury,
	Portal,
	Prison,
	Throne,
	PortalCreate,
	Ramp
};

//-----------------------------------------------------------------------------
// Struktura opisuj¹ca pomieszczenie w podziemiach
struct Room
{
	enum Corner
	{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	};

	Int2 pos, size;
	vector<int> connected;
	RoomTarget target;
	union
	{
		float y;
		float yb[4];
	};
	short level, counter;

	static const int MIN_SIZE = 19;
	static const float HEIGHT;
	static const float HEIGHT_LOW;

	Vec3 Center() const
	{
		return Vec3(float(pos.x * 2 + size.x), y, float(pos.y * 2 + size.y));
	}
	Int2 CenterTile() const
	{
		return pos + size / 2;
	}
	bool IsInside(float x, float z) const
	{
		return (x >= 2.f*pos.x && z >= 2.f*pos.y && x <= 2.f*(pos.x + size.x) && z <= 2.f*(pos.y + size.y));
	}
	bool IsInside(const Vec3& _pos) const
	{
		return IsInside(_pos.x, _pos.z);
	}
	float Distance(const Vec3& _pos) const
	{
		if(IsInside(_pos))
			return 0.f;
		else
			return Vec3::Distance2d(_pos, Center());
	}
	float Distance(const Room& room) const
	{
		return Vec3::Distance2d(Center(), room.Center());
	}
	Vec3 GetRandomPos() const
	{
		return Vec3(Random(2.f*(pos.x + 1), 2.f*(pos.x + size.x - 1)), y, Random(2.f*(pos.y + 1), 2.f*(pos.y + size.y - 1)));
	}
	bool GetRandomPos(float margin, Vec3& out_pos) const
	{
		float min_size = (min(size.x, size.y) - 1) * 2.f;
		if(margin * 2 >= min_size)
			return false;
		out_pos = Vec3(
			Random(2.f*(pos.x + 1) + margin, 2.f*(pos.x + size.x - 1) - margin),
			y,
			Random(2.f*(pos.y + 1) + margin, 2.f*(pos.y + size.y - 1) - margin));
		return true;
	}
	Vec3 GetRandomPos(float margin) const
	{
		assert(margin * 2 < (min(size.x, size.y) - 1) * 2.f);
		return Vec3(
			Random(2.f*(pos.x + 1) + margin, 2.f*(pos.x + size.x - 1) - margin),
			y,
			Random(2.f*(pos.y + 1) + margin, 2.f*(pos.y + size.y - 1) - margin));
	}

	bool IsCorridor() const { return target == RoomTarget::Corridor; }
	bool IsRamp() const { return target == RoomTarget::Ramp; }
	bool IsCorridorOrRamp() const { return IsCorridor() || IsRamp(); }
	bool CanJoinRoom() const { return target == RoomTarget::None || target == RoomTarget::StairsUp || target == RoomTarget::StairsDown; }

	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);

	static const word INVALID_ROOM = (word)-1;
};

//-----------------------------------------------------------------------------
// Opcje mapy
struct OpcjeMapy
{
	//-----------------------------------------------------------------------------
	// Kszta³t mapy podziemi
	enum KSZTALT_MAPY
	{
		PROSTOKAT,
		OKRAG
	};

	//-----------------------------------------------------------------------------
	// Gdzie stworzyæ schody?
	enum GDZIE_SCHODY
	{
		BRAK,
		LOSOWO,
		NAJDALEJ, // schody s¹ jak najdalej od zerowego pokoju
	};

	// input
	int w, h;
	Int2 room_size, corridor_size, ramp_length;
	int korytarz_szansa, polacz_korytarz, polacz_pokoj, kraty_szansa,
		ramp_chance_min, // minimum number of rooms before trying to generate ramp
		ramp_chance, // chance % per room count above minimum to generate ramp
		ramp_chance_other, // chance % to generate ramp in oposite direction
		ramp_decrease_chance; // decrease chance when ramp is generated for other ramp in same room
	KSZTALT_MAPY ksztalt;
	GDZIE_SCHODY schody_gora, schody_dol;
	bool stop,
		ramp_go_up; // true - down, false - up

					// input/output
	Pole* mapa;
	vector<Room>* rooms;
	Room* schody_gora_pokoj, *schody_dol_pokoj;
	int schody_gora_kierunek, schody_dol_kierunek;
	Int2 schody_gora_pozycja, schody_dol_pozycja;

	// output
	int blad;
	bool schody_dol_w_scianie, devmode;

	// tylko nowe zmienne s¹ zerowane dla kompatybilnoœci ze starym kodem
	OpcjeMapy() : stop(false)
	{
	}
};

//-----------------------------------------------------------------------------
struct MapGenerator
{
	void SetFlags(Pole* mapa, uint wh);
	void Draw(Pole* mapa, uint w, uint h);

protected:
	void SetFlags();
	void Draw();

	Pole* mapa;
	OpcjeMapy* opcje;
};
