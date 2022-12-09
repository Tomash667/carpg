#include "Pch.h"
#include "Tile.h"

static const char tile_char[] = {
	' ', // UNUSED
	'_', // EMPTY
	'<', // ENTRY_PREV
	'>', // ENTRY_NEXT
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

void Tile::SetupFlags(Tile* tiles, const Int2& size, EntryType prevEntry, EntryType nextEntry)
{
	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
		{
			Tile& p = tiles[x + y * size.x];
			int ok; // -1 no, 0 no floor, 1 yes
			switch(p.type)
			{
			default:
				ok = -1;
				break;
			case EMPTY:
			case DOORS:
			case BARS:
			case BARS_FLOOR:
			case BARS_CEILING:
				ok = 1;
				break;
			case ENTRY_PREV:
				if(prevEntry == ENTRY_STAIRS_DOWN_IN_WALL)
					ok = 0;
				else if(prevEntry == ENTRY_DOOR)
					ok = 1;
				else
					ok = -1;
				break;
			case ENTRY_NEXT:
				if(nextEntry == ENTRY_STAIRS_DOWN_IN_WALL)
					ok = 0;
				else if(nextEntry == ENTRY_DOOR)
					ok = 1;
				else
					ok = -1;
				break;
			}
			if(ok == -1)
				continue;

			p.flags &= (Tile::F_CEILING | Tile::F_LOW_CEILING | Tile::F_BARS_FLOOR | Tile::F_BARS_CEILING | Tile::F_SPECIAL | Tile::F_SECOND_TEXTURE | Tile::F_REVEALED);

			// floor
			if(p.type == BARS || p.type == BARS_FLOOR)
			{
				p.flags |= Tile::F_BARS_FLOOR;

				if(!Any(tiles[x - 1 + y * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_RIGHT;
				if(!Any(tiles[x + 1 + y * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_LEFT;
				if(!Any(tiles[x + (y - 1) * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_BACK;
				if(!Any(tiles[x + (y + 1) * size.x].type, BARS, BARS_FLOOR))
					p.flags |= Tile::F_HOLE_FRONT;
			}
			else if(ok != 0)
				p.flags |= Tile::F_FLOOR;

			if(p.type == BARS || p.type == BARS_CEILING)
				assert(!IsSet(p.flags, Tile::F_LOW_CEILING));

			if(!IsSet(p.flags, Tile::F_LOW_CEILING))
			{
				if(IsSet(tiles[x - 1 + y * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_RIGHT;
				if(IsSet(tiles[x + 1 + y * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_LEFT;
				if(IsSet(tiles[x + (y - 1) * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_BACK;
				if(IsSet(tiles[x + (y + 1) * size.x].flags, Tile::F_LOW_CEILING))
					p.flags |= Tile::F_CEIL_FRONT;

				// ceiling
				if(p.type == BARS || p.type == BARS_CEILING)
				{
					p.flags |= Tile::F_BARS_CEILING;

					if(!Any(tiles[x - 1 + y * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_RIGHT;
					if(!Any(tiles[x + 1 + y * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_LEFT;
					if(!Any(tiles[x + (y - 1) * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_BACK;
					if(!Any(tiles[x + (y + 1) * size.x].type, BARS, BARS_CEILING))
						p.flags |= Tile::F_UP_FRONT;
				}
				else
					p.flags |= Tile::F_CEILING;
			}

			// walls
			if(Any(tiles[x - 1 + y * size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_RIGHT;
			if(Any(tiles[x + 1 + y * size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_LEFT;
			if(Any(tiles[x + (y - 1) * size.x].type, WALL, BLOCKADE_WALL))
				p.flags |= Tile::F_WALL_BACK;
			if(Any(tiles[x + (y + 1) * size.x].type, WALL, BLOCKADE_WALL))
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
