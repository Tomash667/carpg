#include "Pch.h"
#include "GameCore.h"
#include "Tile.h"

static const char tile_char[] = {
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

void Pole::SetupFlags(Pole* tiles, const Int2& size)
{
	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
		{
			Pole& p = tiles[x + y * size.x];
			if(p.type != PUSTE && p.type != DRZWI && p.type != KRATKA && p.type != KRATKA_PODLOGA && p.type != KRATKA_SUFIT && p.type != SCHODY_DOL)
				continue;

			// pod³oga
			if(p.type == KRATKA || p.type == KRATKA_PODLOGA)
			{
				p.flags |= Pole::F_KRATKA_PODLOGA;

				if(!OR2_EQ(tiles[x - 1 + y * size.x].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_PRAWA;
				if(!OR2_EQ(tiles[x + 1 + y * size.x].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_LEWA;
				if(!OR2_EQ(tiles[x + (y - 1)*size.x].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_TYL;
				if(!OR2_EQ(tiles[x + (y + 1)*size.x].type, KRATKA, KRATKA_PODLOGA))
					p.flags |= Pole::F_DZIURA_PRZOD;
			}
			else if(p.type != SCHODY_DOL)
				p.flags |= Pole::F_PODLOGA;

			if(p.type == KRATKA || p.type == KRATKA_SUFIT)
				assert(!IS_SET(p.flags, Pole::F_NISKI_SUFIT));

			if(!IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				if(IS_SET(tiles[x - 1 + y * size.x].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_PRAWA;
				if(IS_SET(tiles[x + 1 + y * size.x].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_LEWA;
				if(IS_SET(tiles[x + (y - 1)*size.x].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_TYL;
				if(IS_SET(tiles[x + (y + 1)*size.x].flags, Pole::F_NISKI_SUFIT))
					p.flags |= Pole::F_PODSUFIT_PRZOD;

				// dziura w suficie
				if(p.type == KRATKA || p.type == KRATKA_SUFIT)
				{
					p.flags |= Pole::F_KRATKA_SUFIT;

					if(!OR2_EQ(tiles[x - 1 + y * size.x].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_PRAWA;
					if(!OR2_EQ(tiles[x + 1 + y * size.x].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_LEWA;
					if(!OR2_EQ(tiles[x + (y - 1)*size.x].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_TYL;
					if(!OR2_EQ(tiles[x + (y + 1)*size.x].type, KRATKA, KRATKA_SUFIT))
						p.flags |= Pole::F_GORA_PRZOD;
				}
				else
				{
					// normalny sufit w tym miejscu
					p.flags |= Pole::F_SUFIT;
				}
			}

			// œciany
			if(OR2_EQ(tiles[x - 1 + y * size.x].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_PRAWA;
			if(OR2_EQ(tiles[x + 1 + y * size.x].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_LEWA;
			if(OR2_EQ(tiles[x + (y - 1)*size.x].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_TYL;
			if(OR2_EQ(tiles[x + (y + 1)*size.x].type, SCIANA, BLOKADA_SCIANA))
				p.flags |= Pole::F_SCIANA_PRZOD;
		}
	}
}

void Pole::DebugDraw(Pole* tiles, const Int2& size)
{
	for(int y = size.y - 1; y >= 0; --y)
	{
		for(int x = 0; x < size.x; ++x)
			putchar(tile_char[tiles[x + y * size.x].type]);
		putchar('\n');
	}
}
