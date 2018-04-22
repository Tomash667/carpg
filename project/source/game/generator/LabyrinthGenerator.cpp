#include "Pch.h"
#include "GameCore.h"
#include "LabyrinthGenerator.h"

//=================================================================================================
void LabyrinthGenerator::Generate(Pole*& mapa, const Int2& size, const Int2& room_size, Int2& stairs, int& stairs_dir, Int2& room_pos, int kraty_szansa, bool devmode)
{
	// mo¿na by daæ, ¿e nie ma centralnego pokoju
	assert(room_size.x > 4 && room_size.y > 4 && size.x >= room_size.x * 2 + 4 && size.y >= room_size.y * 2 + 4);
	Int2 maze_size(size.x - 2, size.y - 2), doors;

	while(GenerateInternal(maze_size, room_size, room_pos, doors) < 600);

	++room_pos.x;
	++room_pos.y;
	mapa = new Pole[size.x*size.y];
	memset(mapa, 0, sizeof(Pole)*size.x*size.y);

	// kopiuj mapê
	for(int y = 0; y < maze_size.y; ++y)
	{
		for(int x = 0; x < maze_size.x; ++x)
			mapa[x + 1 + (y + 1)*size.x].type = (maze[x + y * maze_size.x] ? PUSTE : SCIANA);
	}

	mapa[doors.x + 1 + (doors.y + 1)*size.x].type = DRZWI;

	// blokady
	for(int x = 0; x < size.x; ++x)
	{
		mapa[x].type = BLOKADA;
		mapa[x + (size.y - 1)*size.x].type = BLOKADA;
	}

	for(int y = 1; y < size.y - 1; ++y)
	{
		mapa[y*size.x].type = BLOKADA;
		mapa[size.x - 1 + y * size.x].type = BLOKADA;
	}

	// schody
	int order[4] = { 0,1,2,3 };
	for(int i = 0; i < 5; ++i)
		std::swap(order[Rand() % 4], order[Rand() % 4]);
	bool ok = false;
	for(int i = 0; i < 4 && !ok; ++i)
	{
		switch(order[i])
		{
		case 0: // dó³
			{
				int start = Random(1, size.x - 1);
				int p = start;
				do
				{
					if(mapa[p + size.x].type == PUSTE)
					{
						int ile = 0;
						if(mapa[p - 1 + size.x].type != PUSTE)
							++ile;
						if(mapa[p + 1 + size.x].type != PUSTE)
							++ile;
						if(mapa[p].type != PUSTE)
							++ile;
						if(mapa[p + size.x * 2].type != PUSTE)
							++ile;

						if(ile == 3)
						{
							stairs.x = p;
							stairs.y = 1;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case 1: // lewa
			{
				int start = Random(1, size.y - 1);
				int p = start;
				do
				{
					if(mapa[1 + p * size.x].type == PUSTE)
					{
						int ile = 0;
						if(mapa[p*size.x].type != PUSTE)
							++ile;
						if(mapa[2 + p * size.x].type != PUSTE)
							++ile;
						if(mapa[1 + (p - 1)*size.x].type != PUSTE)
							++ile;
						if(mapa[1 + (p + 1)*size.x].type != PUSTE)
							++ile;

						if(ile == 3)
						{
							stairs.x = 1;
							stairs.y = p;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case 2: // góra
			{
				int start = Random(1, size.x - 1);
				int p = start;
				do
				{
					if(mapa[p + (size.y - 2)*size.x].type == PUSTE)
					{
						int ile = 0;
						if(mapa[p - 1 + (size.y - 2)*size.x].type != PUSTE)
							++ile;
						if(mapa[p + 1 + (size.y - 2)*size.x].type != PUSTE)
							++ile;
						if(mapa[p + (size.y - 1)*size.x].type != PUSTE)
							++ile;
						if(mapa[p + (size.y - 3)*size.x].type != PUSTE)
							++ile;

						if(ile == 3)
						{
							stairs.x = p;
							stairs.y = size.y - 2;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case 3: // prawa
			{
				int start = Random(1, size.y - 1);
				int p = start;
				do
				{
					if(mapa[size.x - 2 + p * size.x].type == PUSTE)
					{
						int ile = 0;
						if(mapa[size.x - 3 + p * size.x].type != PUSTE)
							++ile;
						if(mapa[size.x - 1 + p * size.x].type != PUSTE)
							++ile;
						if(mapa[size.x - 2 + (p - 1)*size.x].type != PUSTE)
							++ile;
						if(mapa[size.x - 2 + (p + 1)*size.x].type != PUSTE)
							++ile;

						if(ile == 3)
						{
							stairs.x = size.x - 2;
							stairs.y = p;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		}
	}
	if(!ok)
		throw "Failed to generate labirynth.";
	mapa[stairs.x + stairs.y*size.x].type = SCHODY_GORA;

	// ustal kierunek schodów
	if(mapa[stairs.x + (stairs.y + 1)*size.x].type == PUSTE)
		stairs_dir = 2;
	else if(mapa[stairs.x - 1 + stairs.y*size.x].type == PUSTE)
		stairs_dir = 1;
	else if(mapa[stairs.x + (stairs.y - 1)*size.x].type == PUSTE)
		stairs_dir = 0;
	else if(mapa[stairs.x + 1 + stairs.y*size.x].type == PUSTE)
		stairs_dir = 3;
	else
	{
		assert(0);
	}

	// kraty
	if(kraty_szansa > 0)
	{
		for(int y = 1; y < size.y - 1; ++y)
		{
			for(int x = 1; x < size.x - 1; ++x)
			{
				Pole& p = mapa[x + y * size.x];
				if(p.type == PUSTE && !IsInside(Int2(x, y), room_pos, room_size) && Rand() % 100 < kraty_szansa)
				{
					int j = Rand() % 3;
					if(j == 0)
						p.type = KRATKA_PODLOGA;
					else if(j == 1)
						p.type = KRATKA_SUFIT;
					else
						p.type = KRATKA;
				}
			}
		}
	}

	// flagi
	this->mapa = mapa;
	OpcjeMapy opcje;
	opcje.w = size.x;
	opcje.h = size.y;
	this->opcje = &opcje;
	SetFlags();

	// rysuj
	if(devmode)
		Draw();
}

//=================================================================================================
int LabyrinthGenerator::GenerateInternal(const Int2& maze_size, const Int2& room_size, Int2& room_pos, Int2& doors)
{
	list<Int2> drillers;
	maze.resize(maze_size.x * maze_size.y, false);
	for(vector<bool>::iterator it = maze.begin(), end = maze.end(); it != end; ++it)
		*it = false;

	int mx = maze_size.x / 2 - room_size.x / 2,
		my = maze_size.y / 2 - room_size.y / 2;
	room_pos.x = mx;
	room_pos.y = my;

	for(int y = 0; y < room_size.y; ++y)
	{
		for(int x = 0; x < room_size.x; ++x)
		{
			maze[x + mx + (y + my)*maze_size.x] = true;
		}
	}

	Int2 start, start2;
	int dir;

	switch(Rand() % 4)
	{
	case 0:
		start.x = maze_size.x / 2;
		start.y = maze_size.y / 2 + room_size.y / 2;
		start2 = start;
		start.y--;
		dir = 1;
		break;
	case 1:
		start.x = maze_size.x / 2 - room_size.x / 2;
		start.y = maze_size.y / 2;
		start2 = start;
		start2.x--;
		dir = 2;
		break;
	case 2:
		start.x = maze_size.x / 2;
		start.y = maze_size.y / 2 - room_size.y / 2;
		start2 = start;
		start2.y--;
		dir = 0;
		break;
	case 3:
		start.x = maze_size.x / 2 + room_size.x / 2;
		start.y = maze_size.y / 2;
		start2 = start;
		start.x--;
		dir = 3;
		break;
	}
	drillers.push_back(start2);

	int pc = 0;

	while(!drillers.empty())
	{
		list<Int2>::iterator m, _m;
		m = drillers.begin();
		_m = drillers.end();
		while(m != _m)
		{
			bool remove_driller = false;
			switch(dir)
			{
			case 0:
				m->y -= 2;
				if(m->y < 0 || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[(m->y + 1)*maze_size.x + m->x] = true;
				break;
			case 1:
				m->y += 2;
				if(m->y >= maze_size.y || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[(m->y - 1)*maze_size.x + m->x] = true;
				break;
			case 2:
				m->x -= 2;
				if(m->x < 0 || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[m->y*maze_size.x + m->x + 1] = true;
				break;
			case 3:
				m->x += 2;
				if(m->x >= maze_size.x || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[m->y*maze_size.x + m->x - 1] = true;
				break;
			}
			if(remove_driller)
				m = drillers.erase(m);
			else
			{
				drillers.push_back(*m);
				// uncomment the line below to make the maze easier
				// if (Rand()%2)
				drillers.push_back(*m);

				maze[m->x + m->y*maze_size.x] = true;
				++m;
				++pc;
			}

			dir = Rand() % 4;
		}
	}

	for(int x = 0; x < room_size.x; ++x)
	{
		maze[mx + x + my * maze_size.x] = false;
		maze[mx + x + (my + room_size.y - 1)*maze_size.x] = false;
	}
	for(int y = 0; y < room_size.y; ++y)
	{
		maze[mx + (my + y)*maze_size.x] = false;
		maze[mx + room_size.x - 1 + (my + y)*maze_size.x] = false;
	}
	maze[start.x + start.y*maze_size.x] = true;
	maze[start2.x + start2.y*maze_size.x] = true;

	doors = Int2(start.x, start.y);

	return pc;
}
