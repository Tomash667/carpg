/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500

#include "Base.hpp"
#include "Math.hpp"
#include "Error.hpp"
#include "Stream.hpp"
#include "Files.hpp"
#include "Tokenizer.hpp"

#include <map>

#include <windows.h>
#include <d3dx9.h>
#include <cstdio>

using namespace common;

void Write(const string &s);
void Writeln(const string &s);
void Writeln();
// OnceId - jeœli niezerowy, tylko jedno ostrze¿enie tego typu bêdzie wygenerowane a nastêpne ju¿ nie
void Warning(const string &s, uint OnceId = 0);
