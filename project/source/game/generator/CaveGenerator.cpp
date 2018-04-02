#include "Pch.h"
#include "GameCore.h"
#include "CaveGenerator.h"

//=================================================================================================
CaveGenerator::~CaveGenerator()
{
	delete[] m1;
	delete[] m2;
}

//=================================================================================================
void CaveGenerator::Generate(Pole*& mapa, int size, Int2& stairs, int& stairs_dir, vector<Int2>& holes, Rect* ext, bool devmode)
{
	assert(InRange(size, 10, 100));

	int size2 = size * size;

	while(GenerateInternal(size) < 200);

	mapa = new Pole[size2];
	memset(mapa, 0, sizeof(Pole)*size2);

	// rozmiar
	if(ext)
		ext->Set(minx, miny, maxx + 1, maxy + 1);

	// kopiuj
	for(int i = 0; i < size2; ++i)
		mapa[i].type = (m2[i] ? SCIANA : PUSTE);

	// schody
	do
	{
		Int2 pt, dir;

		switch(Rand() % 4)
		{
		case 0: // dó³
			dir = Int2(0, -1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, size - 1);
			break;
		case 1: // lewa
			dir = Int2(1, 0);
			pt = Int2(0, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		case 2: // góra
			dir = Int2(0, 1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, 0);
			break;
		case 3: // prawa
			dir = Int2(-1, 0);
			pt = Int2(size - 1, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		}

		do
		{
			pt += dir;
			if(pt.x == -1 || pt.x == size || pt.y == -1 || pt.y == size)
				break;
			if(mapa[pt.x + pt.y*size].type == PUSTE)
			{
				pt -= dir;
				// sprawdŸ z ilu stron jest puste pole
				int ile = 0, dir2;
				if(mapa[pt.x - 1 + pt.y*size].type == PUSTE)
				{
					++ile;
					dir2 = 1;
				}
				if(mapa[pt.x + 1 + pt.y*size].type == PUSTE)
				{
					++ile;
					dir2 = 3;
				}
				if(mapa[pt.x + (pt.y - 1)*size].type == PUSTE)
				{
					++ile;
					dir2 = 0;
				}
				if(mapa[pt.x + (pt.y + 1)*size].type == PUSTE)
				{
					++ile;
					dir2 = 2;
				}

				if(ile == 1)
				{
					stairs = pt;
					stairs_dir = dir2;
					mapa[pt.x + pt.y*size].type = SCHODY_GORA;
					goto dalej;
				}
				else
					break;
			}
		} while(1);
	} while(1);

dalej:

	// losowe dziury w suficie
	for(int count = 0, tries = 50; tries > 0 && count < 15; --tries)
	{
		Int2 pt(Random(1, size - 1), Random(1, size - 1));
		if(mapa[pt.x + pt.y*size].type == PUSTE)
		{
			bool ok = true;
			for(vector<Int2>::iterator it = holes.begin(), end = holes.end(); it != end; ++it)
			{
				if(Int2::Distance(pt, *it) < 5)
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				mapa[pt.x + pt.y*size].type = KRATKA_SUFIT;
				holes.push_back(pt);
				++count;
			}
		}
	}

	// flagi
	this->mapa = mapa;
	OpcjeMapy opcje;
	opcje.w = opcje.h = size;
	this->opcje = &opcje;
	SetFlags();

	// rysuj
	if(devmode)
		Draw();
}

//=================================================================================================
int CaveGenerator::GenerateInternal(int _size)
{
	size2 = _size * _size;

	if(!m1 || _size > size)
	{
		delete[] m1;
		delete[] m2;
		m1 = new bool[size2];
		m2 = new bool[size2];
	}

	size = _size;

	// wype³nij losowo
	FillMap(m1);

	// celluar automata
	for(int i = 0; i < iter; ++i)
	{
		Make(m1, m2);
		bool* m = m1;
		m1 = m2;
		m2 = m;

		// krawêdzie
		for(int j = 0; j < size; ++j)
		{
			m1[j] = true;
			m1[j + (size - 1)*size] = true;
			m1[j*size + size - 1] = true;
			m1[j*size] = true;
		}
	}

	// wybierz najwiêkszy obszar i go wype³nij
	return Finish(m1, m2);
}

//=================================================================================================
void CaveGenerator::FillMap(bool* m)
{
	for(int i = 0; i < size2; ++i)
	{
		m[i] = (Rand() % 100 < fill);
	}
}

//=================================================================================================
void CaveGenerator::Make(bool* m1, bool* m2)
{
	for(int y = 1; y < size - 1; ++y)
	{
		for(int x = 1; x < size - 1; ++x)
		{
			int ile = 0;
			for(int yy = -1; yy <= 1; ++yy)
			{
				for(int xx = -1; xx <= 1; ++xx)
				{
					if(m1[x + xx + (y + yy)*size])
						++ile;
				}
			}

			m2[x + y * size] = (ile >= 5);
		}
	}

	for(int i = 0; i < size; ++i)
	{
		m2[i] = m1[i];
		m2[i + (size - 1)*size] = m1[i + (size - 1)*size];
		m2[i*size] = m1[i*size];
		m2[i*size + size - 1] = m1[i*size + size - 1];
	}
}

//=================================================================================================
int CaveGenerator::Finish(bool* m, bool* m2)
{
	memset(m2, 0, sizeof(bool)*size2);
	int top = -1, topi = -1;

	// znajdŸ najwiêkszy obszar
	for(int i = 0; i < size2; ++i)
	{
		if(!m[i] || m2[i])
			continue;

		int ile = CalculateFill(m, m2, i);
		if(ile > top)
		{
			top = ile;
			topi = i;
		}
	}

	// wype³nij obszar
	minx = size;
	miny = size;
	maxx = 0;
	maxy = 0;
	return FillCave(m, m2, topi);
}

//=================================================================================================
int CaveGenerator::CalculateFill(bool* m, bool* m2, int start)
{
	int ile = 0;
	v.push_back(start);
	m2[start] = true;

	do
	{
		int i = v.back();
		v.pop_back();
		++ile;
		m2[i] = true;

		int x = i % size;
		int y = i / size;

#define T(x) if(!m[x] && !m2[x]) { m2[x] = true; v.push_back(x); }

		if(x > 0)
			T(x - 1 + y * size);
		if(x < size - 1)
			T(x + 1 + y * size);
		if(y < 0)
			T(x + (y - 1)*size);
		if(y < size - 1)
			T(x + (y + 1)*size);

#undef T
	} while(!v.empty());

	return ile;
}

//=================================================================================================
int CaveGenerator::FillCave(bool* m, bool* m2, int start)
{
	v.push_back(start);
	m[start] = true;
	memset(m2, 1, sizeof(bool)*size2);
	int c = 1;

	do
	{
		int i = v.back();
		v.pop_back();

		int x = i % size;
		int y = i / size;

		if(x < minx)
			minx = x;
		if(y < miny)
			miny = y;
		if(x > maxx)
			maxx = x;
		if(y > maxy)
			maxy = y;

#define T(t) if(!m[t]) { v.push_back(t); m[t] = true; m2[t] = false; ++c; }

		if(x > 0)
			T(x - 1 + y * size);
		if(x < size - 1)
			T(x + 1 + y * size);
		if(y < 0)
			T(x + (y - 1)*size);
		if(y < size - 1)
			T(x + (y + 1)*size);

#undef T
	} while(!v.empty());

	return c;
}

//=================================================================================================
void CaveGenerator::RegenerateFlags(Pole* mapa, int size)
{
	assert(mapa && InRange(size, 10, 100));

	// clear all flags (except F_NISKI_SUFIT, F_DRUGA_TEKSTURA, F_ODKRYTE)
	for(int i = 0, s = size * size; i < s; ++i)
		CLEAR_BIT(mapa[i].flags, 0xFFFFFFFF & ~Pole::F_NISKI_SUFIT & ~Pole::F_DRUGA_TEKSTURA & ~Pole::F_ODKRYTE);

	// ustaw flagi
	this->mapa = mapa;
	OpcjeMapy opcje;
	opcje.w = opcje.h = size;
	this->opcje = &opcje;
	SetFlags();
}
