/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include "GlobalCode.hpp"

void ParseVec2(VEC2 *Out, Tokenizer &t)
{
	Out->x = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->y = t.MustGetFloat();
	t.Next();
}

void ParseVec3(VEC3 *Out, Tokenizer &t)
{
	Out->x = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->y = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->z = t.MustGetFloat();
	t.Next();
}

void ParseMatrix3x3(MATRIX *Out, Tokenizer &t, bool old)
{
	Out->_11 = t.MustGetFloat(); t.Next();t.AssertSymbol(','); t.Next();
	Out->_12 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_13 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_21 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_22 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_23 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_31 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_32 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_33 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_14 = 0.f;
	Out->_24 = 0.f;
	Out->_34 = 0.f;

	Out->_41 = 0.f;
	Out->_42 = 0.f;
	Out->_43 = 0.f;

	Out->_44 = 1.f;
}

void ParseMatrix4x4(MATRIX *Out, Tokenizer &t, bool old)
{
	Out->_11 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_12 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_13 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_14 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_21 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_22 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_23 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_24 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_31 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_32 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_33 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_34 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }

	Out->_41 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_42 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_43 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
	Out->_44 = t.MustGetFloat(); t.Next(); if(old) { t.AssertSymbol(';'); t.Next(); }
}

// Przekszta³ca punkt lub wektor ze wspó³rzêdnych Blendera do DirectX
void BlenderToDirectxTransform(VEC3 *Out, const VEC3 &In)
{
	Out->x = In.x;
	Out->y = In.z;
	Out->z = In.y;
}

void BlenderToDirectxTransform(VEC3 *InOut)
{
	std::swap(InOut->y, InOut->z);
}

// Przekszta³ca macierz przekszta³caj¹c¹ punkty/wektory z takiej dzia³aj¹cej na wspó³rzêdnych Blendera
// na tak¹ dzia³aj¹c¹ na wspó³rzêdnych DirectX-a (i odwrotnie te¿).
void BlenderToDirectxTransform(MATRIX *Out, const MATRIX &In)
{
	Out->_11 = In._11;
	Out->_14 = In._14;
	Out->_41 = In._41;
	Out->_44 = In._44;

	Out->_12 = In._13;
	Out->_13 = In._12;

	Out->_43 = In._42;
	Out->_42 = In._43;

	Out->_31 = In._21;
	Out->_21 = In._31;

	Out->_24 = In._34;
	Out->_34 = In._24;

	Out->_22 = In._33;
	Out->_33 = In._22;
	Out->_23 = In._32;
	Out->_32 = In._23;
}

void BlenderToDirectxTransform(MATRIX *InOut)
{
	std::swap(InOut->_12, InOut->_13);
	std::swap(InOut->_42, InOut->_43);

	std::swap(InOut->_21, InOut->_31);
	std::swap(InOut->_24, InOut->_34);

	std::swap(InOut->_22, InOut->_33);
	std::swap(InOut->_23, InOut->_32);
}

// Sk³ada translacjê, rotacje i skalowanie w macierz w uk³adzie Blendera,
// która wykonuje te operacje transformuj¹c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void AssemblyBlenderObjectMatrix(MATRIX *Out, const VEC3 &Loc, const VEC3 &Rot, const VEC3 &Size)
{
	MATRIX TmpMat;
	Scaling(Out, Size);
	RotationX(&TmpMat, Rot.x);
	*Out *= TmpMat;
	RotationY(&TmpMat, Rot.y);
	*Out *= TmpMat;
	RotationZ(&TmpMat, Rot.z);
	*Out *= TmpMat;
	Translation(&TmpMat, Loc);
	*Out *= TmpMat;
}
