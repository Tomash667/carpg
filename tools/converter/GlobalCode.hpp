/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

// Parsuje wektor w postaci: x, y
void ParseVec2(VEC2 *Out, Tokenizer &t);
// Parsuje wektor w postaci: x, y, z
void ParseVec3(VEC3 *Out, Tokenizer &t);
// Parsuje macierz 3x3 w postaci trzech wierszy, w ka¿dym: ai1, ai2, ai3;
// Ostatni wiersz i ostatni¹ kolumnê wype³nia jak w identycznoœciowej
void ParseMatrix3x3(MATRIX *Out, Tokenizer &t, bool old);
// Parsuje macierz 4x4 w postaci czterech wierszy, w ka¿dym: ai1, ai2, ai3, ai4;
void ParseMatrix4x4(MATRIX *Out, Tokenizer &t, bool old);

// Przekszta³ca punkt lub wektor ze wspó³rzêdnych Blendera do DirectX
void BlenderToDirectxTransform(VEC3 *Out, const VEC3 &In);
void BlenderToDirectxTransform(VEC3 *InOut);
// Przekszta³ca macierz przekszta³caj¹c¹ punkty/wektory z takiej dzia³aj¹cej na wspó³rzêdnych Blendera
// na tak¹ dzia³aj¹c¹ na wspó³rzêdnych DirectX-a (i odwrotnie te¿).
void BlenderToDirectxTransform(MATRIX *Out, const MATRIX &In);
void BlenderToDirectxTransform(MATRIX *InOut);
// Sk³ada translacjê, rotacje i skalowanie w macierz w uk³adzie Blendera,
// która wykonuje te operacje transformuj¹c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void AssemblyBlenderObjectMatrix(MATRIX *Out, const VEC3 &Loc, const VEC3 &Rot, const VEC3 &Size);
