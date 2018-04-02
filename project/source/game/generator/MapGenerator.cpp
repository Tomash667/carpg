#include "Pch.h"
#include "GameCore.h"
#include "MapGenerator.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
const float Room::HEIGHT = 4.f;
const float Room::HEIGHT_LOW = 3.f;

//-----------------------------------------------------------------------------
// Graficzna reprezentacja pola mapy
const char znak[] = {
	' ', // NIEUZYTE
	'_', // PUSTE
	'<', // SCHODY_GORA
	'>', // SCHODY_DOL
	'+', // DRZWI
	'-', // OTWOR_NA_DRZWI
	'/', // KRATKA_PODLOGA
	'^', // KRATKA_SUFIT
	'|', // KRATKA
	'#', // SCIANA
	'@', // BLOKADA
	'$' // BLOKADA_SCIANA
};

//=================================================================================================
void MapGenerator::SetFlags(Pole* mapa, uint wh)
{
	assert(mapa && wh > 0);

	OpcjeMapy opcje;
	opcje.mapa = mapa;
	opcje.w = wh;
	opcje.h = wh;

	this->opcje = &opcje;
	this->mapa = mapa;
	SetFlags();
}

//=================================================================================================
void MapGenerator::SetFlags()
{
	for(int y = 0; y < opcje->h; ++y)
	{
		for(int x = 0; x < opcje->w; ++x)
		{
			Pole& p = mapa[x + y * opcje->w];
			if(p.type != PUSTE && p.type != DRZWI && p.type != KRATKA && p.type != KRATKA_PODLOGA && p.type != KRATKA_SUFIT && p.type != SCHODY_DOL)
				continue;

			// pod³oga
			if(p.type == KRATKA || p.type == KRATKA_PODLOGA)
			{
				p.flags |= Pole::F_KRATKA_PODLOGA;

				if(!OR2_EQ(mapa[x - 1 + y * opcje->w].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_PRAWA;
				if(!OR2_EQ(mapa[x + 1 + y * opcje->w].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_LEWA;
				if(!OR2_EQ(mapa[x + (y - 1)*opcje->w].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_TYL;
				if(!OR2_EQ(mapa[x + (y + 1)*opcje->w].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_PRZOD;
			}
			else if(p.type != SCHODY_DOL)
				p.flags |= Pole::F_PODLOGA;

			if(p.type == KRATKA || p.type == KRATKA_SUFIT)
				assert(!IS_SET(p.flags, Pole::F_NISKI_SUFIT));

			if(!IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				if(IS_SET(mapa[x - 1 + y * opcje->w].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_PRAWA;
				if(IS_SET(mapa[x + 1 + y * opcje->w].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_LEWA;
				if(IS_SET(mapa[x + (y - 1)*opcje->w].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_TYL;
				if(IS_SET(mapa[x + (y + 1)*opcje->w].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_PRZOD;

				// dziura w suficie
				if(p.type == KRATKA || p.type == KRATKA_SUFIT)
				{
					p.flags |= Pole::F_KRATKA_SUFIT;

					if(!OR2_EQ(mapa[x - 1 + y * opcje->w].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_PRAWA;
					if(!OR2_EQ(mapa[x + 1 + y * opcje->w].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_LEWA;
					if(!OR2_EQ(mapa[x + (y - 1)*opcje->w].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_TYL;
					if(!OR2_EQ(mapa[x + (y + 1)*opcje->w].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_PRZOD;
				}
				else
				{
					// normalny sufit w tym miejscu
					p.flags |= Pole::F_SUFIT;
				}
			}

			// œciany
			if(OR2_EQ(mapa[x - 1 + y * opcje->w].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_PRAWA;
			if(OR2_EQ(mapa[x + 1 + y * opcje->w].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_LEWA;
			if(OR2_EQ(mapa[x + (y - 1)*opcje->w].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_TYL;
			if(OR2_EQ(mapa[x + (y + 1)*opcje->w].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_PRZOD;
		}
	}
}

//=================================================================================================
void MapGenerator::Draw(Pole* mapa, uint w, uint h)
{
	assert(mapa && w && h);

	OpcjeMapy opcje;
	opcje.mapa = mapa;
	opcje.w = w;
	opcje.h = h;

	this->opcje = &opcje;
	this->mapa = mapa;
	Draw();
}

//=================================================================================================
void MapGenerator::Draw()
{
	for(int y = opcje->h - 1; y >= 0; --y)
	{
		for(int x = 0; x < opcje->w; ++x)
			putchar(znak[mapa[x + y * opcje->w].type]);
		putchar('\n');
	}
}



//=================================================================================================
void Room::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &size, sizeof(size), &tmp, nullptr);
	uint ile = connected.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	if(ile)
		WriteFile(file, &connected[0], sizeof(int)*ile, &tmp, nullptr);
	WriteFile(file, &target, sizeof(target), &tmp, nullptr);
}

//=================================================================================================
void Room::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &size, sizeof(size), &tmp, nullptr);
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	connected.resize(ile);
	if(ile)
		ReadFile(file, &connected[0], sizeof(int)*ile, &tmp, nullptr);
	if(LOAD_VERSION >= V_0_5)
		ReadFile(file, &target, sizeof(target), &tmp, nullptr);
	else
	{
		int old_target;
		bool corridor;
		ReadFile(file, &old_target, sizeof(old_target), &tmp, nullptr);
		ReadFile(file, &corridor, sizeof(corridor), &tmp, nullptr);
		if(old_target == 0)
			target = (corridor ? RoomTarget::Corridor : RoomTarget::None);
		else
			target = (RoomTarget)(old_target + 1);
	}
}

//=================================================================================================
void Room::Write(BitStream& stream) const
{
	stream.Write(pos);
	stream.Write(size);
	WriteVectorCast<byte, byte>(stream, connected);
	stream.WriteCasted<byte>(target);
}

//=================================================================================================
bool Room::Read(BitStream& stream)
{
	return stream.Read(pos)
		&& stream.Read(size)
		&& ReadVectorCast<byte, byte>(stream, connected)
		&& stream.ReadCasted<byte>(target);
}
