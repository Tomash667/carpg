#pragma once

#include "QmshTmp.h"

class QmshTmpLoader
{
public:
	void LoadQmshTmpFile(tmp::QMSH *Out, const string &FileName);

	static const Int2 QMSH_TMP_HANDLED_VERSION;

private:
	void LoadQmshTmpFileInternal(tmp::QMSH* Out, Tokenizer& t);
	// Parsuje wektor w postaci: x, y
	void ParseVec2(Vec2 *Out, Tokenizer &t);
	// Parsuje wektor w postaci: x, y, z
	void ParseVec3(Vec3 *Out, Tokenizer &t);
	// Parsuje macierz 3x3 w postaci trzech wierszy, w ka¿dym: ai1, ai2, ai3;
	// Ostatni wiersz i ostatni¹ kolumnê wype³nia jak w identycznoœciowej
	void ParseMatrix3x3(Matrix *Out, Tokenizer &t, bool old);
	// Parsuje macierz 4x4 w postaci czterech wierszy, w ka¿dym: ai1, ai2, ai3, ai4;
	void ParseMatrix4x4(Matrix *Out, Tokenizer &t, bool old);
	void ParseVertexGroup(tmp::VERTEX_GROUP *Out, Tokenizer &t);
	void ParseBone(tmp::BONE *Out, Tokenizer &t, int wersja);
	void ParseBoneGroup(tmp::BONE_GROUP* Out, Tokenizer& t);
	void ParseInterpolationMethod(tmp::INTERPOLATION_METHOD *Out, Tokenizer &t);
	void ParseExtendMethod(Tokenizer &t);
	void ParseCurve(tmp::CURVE *Out, Tokenizer &t);
	void ParseChannel(tmp::CHANNEL *Out, Tokenizer &t);
	void ParseAction(tmp::ACTION *Out, Tokenizer &t);

	uint load_version;
};
