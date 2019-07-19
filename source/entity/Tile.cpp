#include "Pch.h"
#include "GameCore.h"
#include "Tile.h"

static const char tile_char[] = {
	' ', // UNUSED
	'_', // EMPTY
	'<', // STAIRS_UP
	'>', // STAIRS_DOWN
	'+', // DOORS
	'-', // HOLE_FOR_DOORS
	'/', // BARS_FLOOR
	'^', // BARS_CEILING
	'|', // BARS
	'#', // WALL
	'@', // BLOCKADE
	'$', // BLOCKADE_WALL
	'X' // USED
};

void Tile::SetupFlags(Tile* tiles, const Int2& size)
{
	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
		{
			Tile& p = tiles[x + y * size.x];
			if(p.type != EMPTY && p.type != DOORS && p.type != BARS && p.type != BARS_FLOOR && p.type != BARS_CEILING && p.type != STAIRS_DOWN)
				continue;

			p.flags &= (Tile::F_CEILING | Tile::F_LOW_CEILING | Tile::F_BARS_FLOOR | Tile::F_BARS_CEILING | Tile::F_SPECIAL | Tile::F_SECOND_TEXTURE | Tile::F_REVEALED);

			// pod³oga
			if(p.type == BARS || p.type == BARS_FLOOR)
			{
				p.flags |= Tile::F_BARS_FLOOR;

				if(!OR2_EQ(tiles[x - 1 + y * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_RIGHT;
				if(!OR2_EQ(tiles[x + 1 + y * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_LEFT;
				if(!OR2_EQ(tiles[x + (y - 1)*size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_BACK;
				if(!OR2_EQ(tiles[x + (y + 1)*size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_FRONT;
			}
			else if(p.type != STAIRS_DOWN)
				p.flags |= Tile::F_FLOOR;

			if(p.type == BARS || p.type == BARS_CEILING)
				assert(!IS_SET(p.flags, Tile::F_LOW_CEILING));

			if(!IS_SET(p.flags, Tile::F_LOW_CEILING))
			{
				if(IS_SET(tiles[x - 1 + y * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_RIGHT;
				if(IS_SET(tiles[x + 1 + y * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_LEFT;
				if(IS_SET(tiles[x + (y - 1)*size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_BACK;
				if(IS_SET(tiles[x + (y + 1)*size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_FRONT;

				// dziura w suficie
				if(p.type == BARS || p.type == BARS_CEILING)
				{
					p.flags |= Tile::F_BARS_CEILING;

					if(!OR2_EQ(tiles[x - 1 + y * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_RIGHT;
					if(!OR2_EQ(tiles[x + 1 + y * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_LEFT;
					if(!OR2_EQ(tiles[x + (y - 1)*size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_BACK;
					if(!OR2_EQ(tiles[x + (y + 1)*size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_FRONT;
				}
				else
				{
					// normalny sufit w tym miejscu
					p.flags |= Tile::F_CEILING;
				}
			}

			// œciany
			if(OR2_EQ(tiles[x - 1 + y * size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_RIGHT;
			if(OR2_EQ(tiles[x + 1 + y * size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_LEFT;
			if(OR2_EQ(tiles[x + (y - 1)*size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_BACK;
			if(OR2_EQ(tiles[x + (y + 1)*size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_FRONT;
		}
	}
}

void Tile::DebugDraw(Tile* tiles, const Int2& size)
{
	for(int y = size.y - 1; y >= 0; --y)
	{
		for(int x = 0; x < size.x; ++x)
			putchar(tile_char[tiles[x + y * size.x].type]);
		putchar('\n');
	}
}
