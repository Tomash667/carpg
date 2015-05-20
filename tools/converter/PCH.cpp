/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <set>
#include <iostream>

std::set<uint> g_WarningIds;

void Write(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	std::cout << s2;
}

void Writeln(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	std::cout << s2 << std::endl;
}

void Writeln()
{
	std::cout << std::endl;
}

void Warning(const string &s, uint OnceId)
{
	if (OnceId > 0)
		if (g_WarningIds.find(OnceId) != g_WarningIds.end())
			return;

	Write("Warning: ");
	Writeln(s);

	g_WarningIds.insert(OnceId);
}
