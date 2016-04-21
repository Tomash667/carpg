/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <algorithm>
#include "MeshMender.h"
#include "GlobalCode.hpp"
#include "MeshTask.hpp"
#include <cassert>
#include "Qmsh.h"

typedef const char* cstring;

cstring format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list,str);
	_vsnprintf_s((char*)buf[marker],FORMAT_LENGTH,FORMAT_LENGTH-1,str,list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH-1] = 0;
	va_end(list);

	marker = (marker+1)%FORMAT_STRINGS;

	return cbuf;
}

enum TOKEN
{
	T_OBJECTS = 1,
	T_MESH,
	T_MATERIALS,
	T_VERTICES,
	T_FACES,
	T_VERTEX_GROUPS,
	T_ARMATURE,
	T_BONE,
	T_HEAD,
	T_TAIL,
	T_ACTIONS,
	T_PARAMS,
	T_FPS,
	T_STATIC,
	T_POINTS,
	T_POS,
	T_ROT,
	T_SCALE,
	T_PARENT,
	T_IMAGE,
	T_NONE,
	T_EMPTY,
	T_TYPE,
	T_SIZE,
	T_MATRIX,
	T_CAMERA,
	T_MATERIAL,
	T_TEXTURE,
	T_DIFFUSE,
	T_NORMAL,
	T_SPECULAR,
	T_SPECULAR_COLOR,
	T_TANGENTS,
	T_SPLIT
};

const VEC3 DefaultDiffuseColor(0.8f,0.8f,0.8f);
const float DefaultDiffuseIntensity = 0.8f;
const VEC3 DefaultSpecularColor(1,1,1);
const float DefaultSpecularIntensity = 0.5f;
const int DefaultSpecularHardness = 50;

uint4 LOAD_VERSION;

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu QMSH TMP

namespace tmp
{

struct VERTEX
{
	VEC3 Pos;
	VEC3 Normal;
};

struct FACE
{
	uint MaterialIndex;
	bool Smooth;
	uint NumVertices; // 3 lub 4
	uint VertexIndices[4];
	VEC2 TexCoords[4];
	VEC3 Normal;
};

struct VERTEX_IN_GROUP
{
	uint Index;
	float Weight;
};

struct VERTEX_GROUP
{
	string Name;
	std::vector<VERTEX_IN_GROUP> VerticesInGroup;
};

struct Texture
{
	string image;
	bool use_diffuse, use_specular, use_specular_color, use_normal;
	float diffuse_factor, specular_factor, specular_color_factor, normal_factor;

	Texture() : use_diffuse(false), use_specular(false), use_specular_color(false), use_normal(false) {}
};

struct Material
{
	string name, image;
	int specular_hardness;
	float diffuse_intensity, specular_intensity;
	VEC3 diffuse_color, specular_color;
	std::vector< shared_ptr<Texture> > textures;
};

struct MESH
{
	string Name;
	string ParentArmature; // £añcuch pusty jeœli nie ma
	string ParentBone; // £añcuch pusty jeœli nie ma
	VEC3 Position;
	VEC3 Orientation;
	VEC3 Size;
	std::vector< shared_ptr<Material> > Materials;
	std::vector<VERTEX> Vertices;
	std::vector<FACE> Faces;
	std::vector< shared_ptr<VERTEX_GROUP> > VertexGroups;
	bool vertex_normals;
};

struct BONE
{
	string Name;
	string Parent; // £añcuch pusty jeœli nie ma parenta
	VEC3 Head_Bonespace;
	VEC3 Head_Armaturespace;
	float HeadRadius;
	VEC3 Tail_Bonespace;
	VEC3 Tail_Armaturespace;
	float TailRadius;
	float Roll_Bonespace;
	float Roll_Armaturespace;
	float Length;
	float Weight;
	MATRIX Matrix_Bonespace; // Liczy siê tylko minor 3x3, reszta jest jak w jednostkowej
	MATRIX Matrix_Armaturespace; // Jest pe³na 4x4
};

struct ARMATURE
{
	string Name;
	VEC3 Position;
	VEC3 Orientation;
	VEC3 Size;
	std::vector< shared_ptr<BONE> > Bones;
};

enum INTERPOLATION_METHOD
{
	INTERPOLATION_CONST,
	INTERPOLATION_LINEAR,
	INTERPOLATION_BEZIER,
};

struct CURVE
{
	string Name;
	INTERPOLATION_METHOD Interpolation;
	std::vector<VEC2> Points;
};

struct CHANNEL
{
	string Name;
	std::vector< shared_ptr<CURVE> > Curves;
};

struct ACTION
{
	string Name;
	std::vector< shared_ptr<CHANNEL> > Channels;
};

struct POINT
{
	string name, bone, type;
	VEC3 size, rot;
	MATRIX matrix;
};

struct QMSH
{
	std::vector< shared_ptr<MESH> > Meshes; // Dowolna iloœæ
	shared_ptr<ARMATURE> Armature; // Nie ma albo jest jeden
	std::vector< shared_ptr<ACTION> > Actions; // Dowolna iloœæ
	std::vector< shared_ptr<POINT> > points;
	int FPS;
	bool static_anim, force_tangents, split;
	VEC3 camera_pos, camera_target, camera_up;

	QMSH() : FPS(25), camera_pos(1,1,1), camera_target(0,0,0), camera_up(0,0,1), static_anim(false), force_tangents(false), split(false)
	{
	}
};

// Przechowuje dane na temat wybranej koœci we wsp. globalnych modelu w uk³adzie DirectX
struct BONE_INTER_DATA
{
	VEC3 HeadPos;
	VEC3 TailPos;
	float HeadRadius;
	float TailRadius;
	float Length;
	uint TmpBoneIndex; // Indeks tej koœci w Armature w TMP, liczony od 0 - stanowi mapowanie nowych na stare koœci
};

void ParseVertexGroup(VERTEX_GROUP *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		VERTEX_IN_GROUP vig;

		vig.Index = t.MustGetUint4();
		t.Next();

		vig.Weight = t.MustGetFloat();
		t.Next();

		Out->VerticesInGroup.push_back(vig);
	}
	t.Next();
}

void ParseBone(BONE *Out, Tokenizer &t, int wersja)
{
	t.AssertKeyword(T_BONE);
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Parent = t.GetString();
	t.Next();

	// head
	t.AssertKeyword(T_HEAD);
	t.Next();
	ParseVec3(&Out->Head_Bonespace, t);
	ParseVec3(&Out->Head_Armaturespace, t);
	Out->HeadRadius = t.MustGetFloat();
	t.Next();

	// tail
	t.AssertKeyword(T_TAIL);
	t.Next();
	ParseVec3(&Out->Tail_Bonespace, t);
	ParseVec3(&Out->Tail_Armaturespace, t);
	Out->TailRadius = t.MustGetFloat();
	t.Next();

	// Reszta
	if(wersja < 17)
	{
		t.MustGetFloat();
		t.Next();
		t.MustGetFloat();
		t.Next();
	}
	Out->Length = t.MustGetFloat();
	t.Next();
	if(wersja < 17)
	{
		Out->Weight = t.MustGetFloat();
		t.Next();
	}

	// Macierze
	ParseMatrix3x3(&Out->Matrix_Bonespace, t, wersja < 19);
	ParseMatrix4x4(&Out->Matrix_Armaturespace, t, wersja < 19);

	t.AssertSymbol('}');
	t.Next();
}

void ParseInterpolationMethod(INTERPOLATION_METHOD *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);

	if (t.GetString() == "CONST")
		*Out = INTERPOLATION_CONST;
	else if (t.GetString() == "LINEAR")
		*Out = INTERPOLATION_LINEAR;
	else if (t.GetString() == "BEZIER")
		*Out = INTERPOLATION_BEZIER;
	else
		t.CreateError();

	t.Next();
}

// used in old version (pre 18)
void ParseExtendMethod(Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);

	if (t.GetString() != "CONST" && t.GetString() != "EXTRAP" && t.GetString() != "CYCLIC" && t.GetString() != "CYCLIC_EXTRAP")
		t.CreateError();

	t.Next();
}

void ParseCurve(CURVE *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	ParseInterpolationMethod(&Out->Interpolation, t);
	if(LOAD_VERSION < 18)
		ParseExtendMethod(t);

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		VEC2 pt;
		ParseVec2(&pt, t);
		Out->Points.push_back(pt);
	}
	t.Next();
}

void ParseChannel(CHANNEL *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		shared_ptr<CURVE> curve(new CURVE);
		ParseCurve(curve.get(), t);
		Out->Curves.push_back(curve);
	}
	t.Next();
}

void ParseAction(ACTION *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		shared_ptr<CHANNEL> channel(new CHANNEL);
		ParseChannel(channel.get(), t);
		Out->Channels.push_back(channel);
	}
	t.Next();
}

} // namespace tmp


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje

void CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, BOX *OutBox)
{
	if (Qmsh.Vertices.empty())
	{
		*OutSphereRadius = 0.f;
		*OutBox = BOX(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	else
	{
		float MaxDistSq = LengthSq(Qmsh.Vertices[0].Pos);
		OutBox->p1 = OutBox->p2 = Qmsh.Vertices[0].Pos;

		for (uint i = 1; i < Qmsh.Vertices.size(); i++)
		{
			MaxDistSq = std::max(MaxDistSq, LengthSq(Qmsh.Vertices[i].Pos));
			Min(&OutBox->p1, OutBox->p1, Qmsh.Vertices[i].Pos);
			Max(&OutBox->p2, OutBox->p2, Qmsh.Vertices[i].Pos);
		}

		*OutSphereRadius = sqrtf(MaxDistSq);
	}
}

void CalcBoundingVolumes(const QMSH& mesh, QMSH_SUBMESH& sub)
{
	float radius = 0.f;
	VEC3 center;
	float inf = std::numeric_limits<float>::infinity();
	BOX box(inf,inf,inf,-inf,-inf,-inf);

	for(uint i=0; i<sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j=0; j<3; ++j)
		{
			uint index = mesh.Indices[face_index*3+j];
			box.AddInternalPoint(mesh.Vertices[index].Pos);
		}
	}

	box.CalcCenter(&center);
	sub.box = box;

	for(uint i=0; i<sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j=0; j<3; ++j)
		{
			uint index = mesh.Indices[face_index*3+j];
			const VEC3 &Tmp = mesh.Vertices[index].Pos;
			float dist = Distance(center, Tmp);
			radius = std::max(radius, dist);
		}
	}

	sub.center = center;
	sub.range = radius;
}

// Wylicza parametry bry³ otaczaj¹cych siatkê
void CalcBoundingVolumes(QMSH &Qmsh)
{
	Writeln("Calculating bounding volumes...");

	CalcBoundingVolumes(Qmsh, &Qmsh.BoundingSphereRadius, &Qmsh.BoundingBox);

	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
			CalcBoundingVolumes(Qmsh, **it);
	}
}

// Liczy kolizjê punktu z bry³¹ wyznaczon¹ przez dwie po³¹czone kule, ka¿da ma swój œrodek i promieñ
// Wygl¹ta to coœ jak na koñcach dwie pó³kule a w œrodku walec albo œciêty sto¿ek.
// Ka¿dy koniec ma dwa promienie. Zwraca wp³yw koœci na ten punkt, tzn.:
// - Jeœli punkt le¿y w promieniu promienia wewnêtrznego, zwraca 1.
// - Jeœli punkt le¿y w promieniu promienia zewnêtrznego, zwraca 1..0, im dalej tym mniej.
// - Jeœli punkt le¿y poza promieniem zewnêtrznym, zwraca 0.
float PointToBone(
	const VEC3 &Pt,
	const VEC3 &Center1, float InnerRadius1, float OuterRadius1,
	const VEC3 &Center2, float InnerRadius2, float OuterRadius2)
{
	float T = ClosestPointOnLine(Pt, Center1, Center2-Center1) / DistanceSq(Center1, Center2);
	if (T <= 0.f)
	{
		float D = Distance(Pt, Center1);
		if (D <= InnerRadius1)
			return 1.f;
		else if (D < OuterRadius1)
			return 1.f - (D - InnerRadius1) / (OuterRadius1 - InnerRadius1);
		else
			return 0.f;
	}
	else if (T >= 1.f)
	{
		float D = Distance(Pt, Center2);
		if (D <= InnerRadius2)
			return 1.f;
		else if (D < OuterRadius2)
			return 1.f - (D - InnerRadius2) / (OuterRadius2 - InnerRadius2);
		else
			return 0.f;
	}
	else
	{
		float InterInnerRadius = Lerp(InnerRadius1, InnerRadius2, T);
		float InterOuterRadius = Lerp(OuterRadius1, OuterRadius2, T);
		VEC3 InterCenter; Lerp(&InterCenter, Center1, Center2, T);

		float D = Distance(Pt, InterCenter);
		if (D <= InterInnerRadius)
			return 1.f;
		else if (D < InterOuterRadius)
			return 1.f - (D - InterInnerRadius) / (InterOuterRadius - InterInnerRadius);
		else
			return 0.f;
	}
}

void LoadQmshTmpFile(tmp::QMSH *Out, const string &FileName)
{
	FileStream input_file(FileName, FM_READ);
	Tokenizer t(&input_file, 0);

	t.RegisterKeyword(T_OBJECTS, "objects");
	t.RegisterKeyword(T_MESH, "mesh");
	t.RegisterKeyword(T_MATERIALS, "materials");
	t.RegisterKeyword(T_VERTICES, "vertices");
	t.RegisterKeyword(T_FACES, "faces");
	t.RegisterKeyword(T_VERTEX_GROUPS, "vertex_groups");
	t.RegisterKeyword(T_ARMATURE, "armature");
	t.RegisterKeyword(T_BONE, "bone");
	t.RegisterKeyword(T_HEAD, "head");
	t.RegisterKeyword(T_TAIL, "tail");
	t.RegisterKeyword(T_ACTIONS, "actions");
	t.RegisterKeyword(T_PARAMS, "params");
	t.RegisterKeyword(T_FPS, "fps");
	t.RegisterKeyword(T_STATIC, "static");
	t.RegisterKeyword(T_POINTS, "points");
	t.RegisterKeyword(T_POS, "pos");
	t.RegisterKeyword(T_ROT, "rot");
	t.RegisterKeyword(T_SCALE, "scale");
	t.RegisterKeyword(T_PARENT, "parent");
	t.RegisterKeyword(T_IMAGE, "image");
	t.RegisterKeyword(T_NONE, "none");
	t.RegisterKeyword(T_EMPTY, "empty");
	t.RegisterKeyword(T_BONE, "bone");
	t.RegisterKeyword(T_TYPE, "type");
	t.RegisterKeyword(T_SIZE, "size");
	t.RegisterKeyword(T_MATRIX, "matrix");
	t.RegisterKeyword(T_CAMERA, "camera");
	t.RegisterKeyword(T_MATERIAL, "material");
	t.RegisterKeyword(T_TEXTURE, "texture");
	t.RegisterKeyword(T_DIFFUSE, "diffuse");
	t.RegisterKeyword(T_NORMAL, "normal");
	t.RegisterKeyword(T_SPECULAR, "specular");
	t.RegisterKeyword(T_SPECULAR_COLOR, "specular_color");
	t.RegisterKeyword(T_TANGENTS, "tangents");
	t.RegisterKeyword(T_SPLIT, "split");

	t.Next();

	// Nag³ówek
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (t.GetString() != "QMSH") t.CreateError("B³êdny nag³ówek");
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (t.GetString() != "TMP") t.CreateError("B³êdny nag³ówek");
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_INTEGER);
	uint4 wersja = t.MustGetUint4();
	if(wersja < QMSH_TMP_VERSION_MIN || wersja > QMSH_TMP_VERSION_MAX)
		t.CreateError("B³êdna wersja");
	t.Next();
	LOAD_VERSION = wersja;

	// Pocz¹tek
	t.AssertKeyword(T_OBJECTS);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	// Obiekty
	for (;;)
	{
		if (t.GetToken() == Tokenizer::TOKEN_SYMBOL && t.GetChar() == '}')
		{
			t.Next();
			break;
		}
		
		t.AssertToken(Tokenizer::TOKEN_KEYWORD);
		// mesh
		if (t.GetId() == T_MESH )
		{
			t.Next();

			shared_ptr<tmp::MESH> object(new tmp::MESH);

			// Nazwa
			t.AssertToken(Tokenizer::TOKEN_STRING);
			object->Name = t.GetString();
			t.Next();

			// '{'
			t.AssertSymbol('{');
			t.Next();

			// pozycja
			t.AssertKeyword(T_POS);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			ParseVec3(&object->Position, t);
			// obrót
			t.AssertKeyword(T_ROT);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			ParseVec3(&object->Orientation, t);
			// skala
			t.AssertKeyword(T_SCALE);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			ParseVec3(&object->Size, t);
			// parent armature
			t.AssertKeyword(T_PARENT);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			t.AssertToken(Tokenizer::TOKEN_STRING);
			object->ParentArmature = t.GetString();
			t.Next();
			// parent bone
			t.AssertToken(Tokenizer::TOKEN_STRING);
			object->ParentBone = t.GetString();
			t.Next();

			// Materia³y
			t.AssertKeyword(T_MATERIALS);
			t.Next();
			if(wersja >= 16 && wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			t.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumMaterials = t.MustGetUint4();
			object->Materials.resize(NumMaterials);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			if(wersja >= 16)
			{
				for (uint mi = 0; mi < NumMaterials; mi++)
				{
					shared_ptr<tmp::Material> m(new tmp::Material);

					// material
					t.AssertKeyword(T_MATERIAL);
					t.Next();

					// name
					t.AssertToken(Tokenizer::TOKEN_STRING);
					m->name = t.GetString();
					t.Next();

					// {
					t.AssertSymbol('{');
					t.Next();

					if(wersja >= 18)
					{
						t.AssertKeyword(T_DIFFUSE);
						t.Next();
						ParseVec3(&m->diffuse_color, t);
						m->diffuse_intensity = t.MustGetFloat();
						t.Next();

						t.AssertKeyword(T_SPECULAR);
						t.Next();
						ParseVec3(&m->specular_color, t);
						m->specular_intensity = t.MustGetFloat();
						t.Next();
						m->specular_hardness = t.MustGetInt4();
						t.Next();
					}
					else
					{
						m->diffuse_color = DefaultDiffuseColor;
						m->diffuse_intensity = DefaultDiffuseIntensity;
						m->specular_color = DefaultSpecularColor;
						m->specular_intensity = DefaultSpecularIntensity;
						m->specular_hardness = DefaultSpecularHardness;
					}

					while(true)
					{
						// texture lub }
						if(t.QuerySymbol('}'))
						{
							t.Next();
							break;
						}
						t.AssertKeyword(T_TEXTURE);
						t.Next();

						// image
						t.AssertToken(Tokenizer::TOKEN_STRING);
						shared_ptr<tmp::Texture> tex(new tmp::Texture);
						tex->image = t.GetString();
						t.Next();

						// {
						t.AssertSymbol('{');
						t.Next();

						while(true)
						{
							// keyword lub }
							if(t.QuerySymbol('}'))
							{
								t.Next();
								break;
							}
							t.AssertToken(Tokenizer::TOKEN_KEYWORD);
							TOKEN id = (TOKEN)t.GetId();
							if(id != T_DIFFUSE && id != T_SPECULAR && id != T_SPECULAR_COLOR && id != T_NORMAL)
								t.CreateError("Oczekiwano na rodzaj tekstury.");
							t.Next();

							// wartoœæ
							float value = t.MustGetFloat();
							t.Next();

							switch(id)
							{
							case T_DIFFUSE:
								tex->use_diffuse = true;
								tex->diffuse_factor = value;
								break;
							case T_SPECULAR:
								tex->use_specular = true;
								tex->specular_factor = value;
								break;
							case T_SPECULAR_COLOR:
								tex->use_specular_color = true;
								tex->specular_color_factor = value;
								break;
							case T_NORMAL:
								tex->use_normal = true;
								tex->normal_factor = value;
								break;
							}
						}

						m->textures.push_back(tex);
					}

					object->Materials[mi] = m;
				}
			}
			else
			{
				for (uint mi = 0; mi < NumMaterials; mi++)
				{
					shared_ptr<tmp::Material> m(new tmp::Material);
					t.AssertToken(Tokenizer::TOKEN_STRING);
					m->name = t.GetString();
					t.Next();

					t.AssertSymbol(',');
					t.Next();

					t.AssertToken( Tokenizer::TOKEN_KEYWORD );
					if( t.GetId() == T_IMAGE )
					{
						t.Next();
						t.AssertToken(Tokenizer::TOKEN_STRING);
						m->image = t.GetString();
						t.Next();
					}
					else
					{
						t.AssertKeyword( T_NONE );
						t.Next();
					}

					m->diffuse_color = DefaultDiffuseColor;
					m->diffuse_intensity = DefaultDiffuseIntensity;
					m->specular_color = DefaultSpecularColor;
					m->specular_intensity = DefaultSpecularIntensity;
					m->specular_hardness = DefaultSpecularHardness;

					object->Materials[mi] = m;
				}
			}

			t.AssertSymbol('}');
			t.Next();

			// Wierzcho³ki
			t.AssertKeyword(T_VERTICES);
			t.Next();
			if(wersja >= 16 && wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			t.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumVertices = t.MustGetUint4();
			object->Vertices.resize(NumVertices);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			if(wersja >= 17)
			{
				object->vertex_normals = true;
				for (uint vi = 0; vi < NumVertices; vi++)
				{
					object->Vertices[vi].Pos.x = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Pos.y = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Pos.z = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Normal.x = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Normal.y = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Normal.z = t.MustGetFloat();
					t.Next();
				}
			}
			else
			{
				object->vertex_normals = false;
				for (uint vi = 0; vi < NumVertices; vi++)
				{
					object->Vertices[vi].Pos.x = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Pos.y = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					object->Vertices[vi].Pos.z = t.MustGetFloat();
					t.Next();
				}
			}

			t.AssertSymbol('}');
			t.Next();

			// Œcianki (faces)
			t.AssertKeyword(T_FACES); // faces
			t.Next();
			if(wersja >= 16 && wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			t.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumFaces = t.MustGetUint4();
			object->Faces.resize(NumFaces);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			for (uint fi = 0; fi < NumFaces; fi++)
			{
				tmp::FACE & f = object->Faces[fi];

				t.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.NumVertices = t.MustGetUint4();
				if (f.NumVertices != 3 && f.NumVertices != 4)
					throw Error("B³êdna liczba wierzcho³ków w œciance.");
				t.Next();

				t.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.MaterialIndex = t.MustGetUint4();
				t.Next();

				t.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.Smooth = (t.MustGetUint4() > 0);
				t.Next();

				// Wierzcho³ki tej œcianki
				for (uint vi = 0; vi < f.NumVertices; vi++)
				{
					// Indeks wierzcho³ka
					t.AssertToken(Tokenizer::TOKEN_INTEGER);
					f.VertexIndices[vi] = t.MustGetUint4();
					t.Next();

					// Wspó³rzêdne tekstury
					f.TexCoords[vi].x = t.MustGetFloat();
					t.Next();
					t.AssertSymbol(',');
					t.Next();
					f.TexCoords[vi].y = t.MustGetFloat();
					t.Next();
				}

				// normalna
				f.Normal.x = t.MustGetFloat();
				t.Next();
				t.AssertSymbol(',');
				t.Next();
				f.Normal.y = t.MustGetFloat();
				t.Next();
				t.AssertSymbol(',');
				t.Next();
				f.Normal.z = t.MustGetFloat();
				t.Next();
			}

			t.AssertSymbol('}');
			t.Next();

			// Grupy wierzcho³ków
			t.AssertKeyword(T_VERTEX_GROUPS); // vertex_groups
			t.Next();
			if(wersja >= 16 && wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			t.AssertSymbol('{');
			t.Next();

			while (!t.QuerySymbol('}'))
			{
				shared_ptr<tmp::VERTEX_GROUP> VertexGroup(new tmp::VERTEX_GROUP);
				ParseVertexGroup(VertexGroup.get(), t);
				object->VertexGroups.push_back(VertexGroup);
			}
			t.Next();

			// Obiekt gotowy

			Out->Meshes.push_back(object);
		}
		// armature
		else if (t.GetId() == T_ARMATURE)
		{
			t.Next();

			if (Out->Armature != NULL)
				t.CreateError("Multiple armature objects not allowed.");

			Out->Armature.reset(new tmp::ARMATURE);
			tmp::ARMATURE & arm = *Out->Armature.get();

			t.AssertToken(Tokenizer::TOKEN_STRING);
			arm.Name = t.GetString();
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			// Parametry ogólne
			ParseVec3(&arm.Position, t);
			ParseVec3(&arm.Orientation, t);
			ParseVec3(&arm.Size, t);

			if(wersja < 17)
			{
				t.AssertToken(Tokenizer::TOKEN_INTEGER);
				t.MustGetUint4();
				t.Next();

				t.AssertToken(Tokenizer::TOKEN_INTEGER);
				t.MustGetUint4();
				t.Next();
			}

			// Koœci
			while (!t.QuerySymbol('}'))
			{
				shared_ptr<tmp::BONE> bone(new tmp::BONE);
				ParseBone(bone.get(), t, wersja);
				arm.Bones.push_back(bone);
			}
		}
		else if( t.GetId() == T_EMPTY )
		{
			t.Next();
			shared_ptr<tmp::POINT> point(new tmp::POINT);

			// nazwa
			t.AssertToken(Tokenizer::TOKEN_STRING);
			point->name = t.GetString();
			t.Next();
			
			// {
			t.AssertSymbol('{');
			t.Next();

			// bone
			t.AssertKeyword(T_BONE);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			t.AssertToken(Tokenizer::TOKEN_STRING);
			point->bone = t.GetString();
			t.Next();

			// type
			t.AssertKeyword(T_TYPE);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			t.AssertToken(Tokenizer::TOKEN_STRING);
			point->type = t.GetString();
			t.Next();

			// size
			t.AssertKeyword(T_SIZE);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			if(wersja >= 14)
			{
				VEC3 size;
				ParseVec3(&size, t);

				// scale
				t.AssertKeyword(T_SCALE);
				t.Next();
				if(wersja < 18)
				{
					t.AssertSymbol(':');
					t.Next();
				}
				float scale = t.MustGetFloat();
				t.Next();

				size *= scale;
				BlenderToDirectxTransform(&point->size, size);
			}
			else
			{
				float scale = t.MustGetFloat();
				t.Next();

				point->size = VEC3(scale,scale,scale);
			}

			// macierz
			t.AssertKeyword(T_MATRIX);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			ParseMatrix4x4(&point->matrix, t, wersja < 19);

			// rot
			if(wersja >= 19)
			{
				t.AssertKeyword(T_ROT);
				t.Next();
				ParseVec3(&point->rot, t);
			}

			Out->points.push_back(point);
		}
		else if(t.GetId() == T_CAMERA)
		{
			// kamera

			// {
			t.Next();
			t.AssertSymbol('{');
			t.Next();

			// pozycja
			VEC3 pos, rot;
			ParseVec3(&pos, t);
			ParseVec3(&rot, t);

			// up
			if(wersja >= 15)
			{
				VEC3 up;
				ParseVec3(&up, t);
				Out->camera_up = up;
			}
			else
				Out->camera_up = VEC3(0,0,1);

			Out->camera_pos = pos;
			Out->camera_target = rot;
		}
		else
			t.CreateError();

		t.AssertSymbol('}');
		t.Next();
	}

	// actions
	t.AssertKeyword(T_ACTIONS);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		shared_ptr<tmp::ACTION> action(new tmp::ACTION);
		ParseAction(action.get(), t);
		Out->Actions.push_back(action);
	}
	t.Next();

	// params
	t.AssertKeyword(T_PARAMS);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		t.AssertToken(Tokenizer::TOKEN_KEYWORD);
		int id = t.GetId();
		switch(id)
		{
		case T_FPS:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->FPS = t.MustGetInt4();
			t.Next();
			break;
		case T_STATIC:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->static_anim = (t.MustGetInt4() > 0);
			t.Next();
			break;
		case T_TANGENTS:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->force_tangents = (t.MustGetInt4() > 0);
			t.Next();
			break;
		case T_SPLIT:
			t.Next();
			Out->split = (t.MustGetInt4() > 0);
			t.Next();
			break;
		default:
			t.CreateError();
			break;
		}
	}

	t.Next();
	t.AssertEOF();
}

// Przekszta³ca pozycje, wektory i wspó³rzêdne tekstur z uk³adu Blendera do uk³adu DirectX
// Uwzglêdnia te¿ przekszta³cenia ca³ych obiektów wprowadzaj¹c je do przekszta³ceñ poszczególnych wierzcho³ków.
// Zamienia te¿ kolejnoœæ wierzcho³ków w œciankach co by by³y odwrócone w³aœciw¹ stron¹.
// NIE zmienia danych koœci ani animacji
void TransformQmshTmpCoords(tmp::QMSH *InOut)
{
	for (uint oi = 0; oi < InOut->Meshes.size(); oi++)
	{
		tmp::MESH & o = *InOut->Meshes[oi].get();

		// Zbuduj macierz przekszta³cania tego obiektu
		MATRIX Mat, MatRot;
		AssemblyBlenderObjectMatrix(&Mat, o.Position, o.Orientation, o.Size);
		D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&MatRot, o.Orientation.y, o.Orientation.x, o.Orientation.z);
		// Jeœli obiekt jest sparentowany do Armature
		// To jednak nic, bo te wspó³rzêdne s¹ ju¿ wyeksportowane w uk³adzie globalnym modelu a nie wzglêdem parenta.
		if (!o.ParentArmature.empty() && !InOut->static_anim /*&& InOut->Armature != NULL*/ )
		{
			assert(InOut->Armature != NULL);
			MATRIX ArmatureMat, ArmatureMatRot;
			AssemblyBlenderObjectMatrix(&ArmatureMat, InOut->Armature->Position, InOut->Armature->Orientation, InOut->Armature->Size);
			D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&ArmatureMatRot, InOut->Armature->Orientation.y, InOut->Armature->Orientation.x, InOut->Armature->Orientation.z);
			Mat = Mat * ArmatureMat;
			MatRot = MatRot * ArmatureMatRot;
		}

		// Przekszta³æ wierzcho³ki
		for (uint vi = 0; vi < o.Vertices.size(); vi++)
		{
			tmp::VERTEX & v = o.Vertices[vi];
			tmp::VERTEX T;
			Transform(&T.Pos, v.Pos, Mat);
			Transform(&T.Normal, v.Normal, MatRot);
			BlenderToDirectxTransform(&v.Pos, T.Pos);
			BlenderToDirectxTransform(&v.Normal, T.Normal);
		}

		// Przekszta³æ œcianki
		for (uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			// Wsp. tekstur
			for (uint vi = 0; vi < f.NumVertices; vi++)
				f.TexCoords[vi].y = 1.0f - f.TexCoords[vi].y;

			// Zamieñ kolejnoœæ wierzcho³ków w œciance
			if (f.NumVertices == 3)
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[2]);
			}
			else
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[3]);
				std::swap(f.VertexIndices[1], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[3]);
				std::swap(f.TexCoords[1], f.TexCoords[2]);
			}

			BlenderToDirectxTransform( &f.Normal );
		}
	}
}

// Szuka takiego wierzcho³ka w NvVertices.
// Jeœli nie znajdzie, dodaje nowy.
// Czy znalaz³ czy doda³, zwraca jego indeks w NvVertices.
uint TmpConvert_AddVertex(
	std::vector<MeshMender::Vertex>& NvVertices,
	std::vector<unsigned int>& MappingNvToTmpVert,
	uint TmpVertIndex,
	const tmp::VERTEX& TmpVertex,
	const VEC2& TexCoord,
	const VEC3& face_normal,
	bool vertex_normals)
{
	for (uint i = 0; i < NvVertices.size(); i++)
	{
		MeshMender::Vertex& v = NvVertices[i];
		// Pozycja identyczna...
		if (v.pos == TmpVertex.Pos &&
			// TexCoord mniej wiêcej...
			float_equal(TexCoord.x, v.s) && float_equal(TexCoord.y,v.t))
		{
			if(!vertex_normals || v.normal == TmpVertex.Normal)
				return i;
		}
	}

	// Nie znaleziono
	MeshMender::Vertex v;
	v.pos = TmpVertex.Pos;
	v.s = TexCoord.x;
	v.t = TexCoord.y;
	v.normal = (vertex_normals ? TmpVertex.Normal : face_normal);
	NvVertices.push_back(v);
	MappingNvToTmpVert.push_back(TmpVertIndex);

	return NvVertices.size() - 1;
}

// Funkcja rekurencyjna
void TmpToQmsh_Bone(
	QMSH_BONE *Out,
	uint OutIndex,
	QMSH *OutQmsh,
	const tmp::BONE &TmpBone,
	const tmp::ARMATURE &TmpArmature,
	uint ParentIndex,
	const MATRIX &WorldToParent)
{
	Out->Name = TmpBone.Name;
	Out->ParentIndex = ParentIndex;

	// TmpBone.Matrix_Armaturespace zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. Blender
	// BoneToArmature zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. DirectX
	MATRIX BoneToArmature;
	BlenderToDirectxTransform(&BoneToArmature, TmpBone.Matrix_Armaturespace);
	// Out->Matrix bêdzie zwiera³o przekszta³cenie ze wsp. tej koœci do jej parenta w uk³. DirectX
	Out->Matrix = BoneToArmature * WorldToParent;
	BlenderToDirectxTransform(&Out->point, TmpBone.Head_Armaturespace);
	MATRIX ArmatureToBone;
	Inverse(&ArmatureToBone, BoneToArmature);

	// Koœci potomne
	for (uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpSubBone = *TmpArmature.Bones[bi].get();
		if (TmpSubBone.Parent == TmpBone.Name)
		{
			shared_ptr<QMSH_BONE> SubBone(new QMSH_BONE);
			OutQmsh->Bones.push_back(SubBone);
			Out->Children.push_back(OutQmsh->Bones.size());

			TmpToQmsh_Bone(
				SubBone.get(),
				OutQmsh->Bones.size(), // Bez -1, bo indeksujemy od 0
				OutQmsh,
				TmpSubBone,
				TmpArmature,
				OutIndex,
				ArmatureToBone);
		}
	}
}

void TmpToQmsh_Bones(
	QMSH *Out,
	std::vector<tmp::BONE_INTER_DATA> *OutBoneInterData,
	const tmp::QMSH &QmshTmp )
{
	Writeln("Processing bones...");
	const tmp::ARMATURE & TmpArmature = *QmshTmp.Armature.get();

	// Ta macierz przekszta³ca wsp. z lokalnych obiektu Armature do globalnych œwiata.
	// Bêdzie ju¿ w uk³adzie DirectX.
	MATRIX ArmatureToWorldMat;
	AssemblyBlenderObjectMatrix(&ArmatureToWorldMat, TmpArmature.Position, TmpArmature.Orientation, TmpArmature.Size);
	BlenderToDirectxTransform(&ArmatureToWorldMat);
	//MATRIX WorldToArmatureMat;
	//Inverse(&WorldToArmatureMat, ArmatureToWorldMat);

	// Dla ka¿dej koœci g³ównego poziomu
	for (uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpBone = *TmpArmature.Bones[bi].get();
		if (TmpBone.Parent.empty())
		{
			// Utwórz koœæ QMSH
			shared_ptr<QMSH_BONE> Bone(new QMSH_BONE);
			Out->Bones.push_back(Bone);

			Bone->Name = TmpBone.Name;
			Bone->ParentIndex = 0; // Brak parenta

			TmpToQmsh_Bone(
				Bone.get(),
				Out->Bones.size(), // Bez -1, bo indeksujemy od 0
				Out,
				TmpBone,
				TmpArmature,
				0, // Nie ma parenta
				ArmatureToWorldMat);
		}
	}

	// Sprawdzenie liczby koœci
	if (Out->Bones.size() != QmshTmp.Armature->Bones.size())
		Warning(Format("Skipped # invalid bones.") % (QmshTmp.Armature->Bones.size() - Out->Bones.size()), 0);

	// Policzenie Bone Inter Data
	OutBoneInterData->resize(Out->Bones.size());
	for (uint bi = 0; bi < Out->Bones.size(); bi++)
	{
		QMSH_BONE & OutBone = *Out->Bones[bi].get();
		for (uint bj = 0; bj <= TmpArmature.Bones.size(); bj++) // To <= to czeœæ tego zabezpieczenia
		{
			assert(bj < TmpArmature.Bones.size()); // zabezpieczenie ¿e poœród koœci Ÿród³owych zawsze znajdzie siê te¿ ta docelowa

			const tmp::BONE & InBone = *TmpArmature.Bones[bj].get();
			if (OutBone.Name == InBone.Name)
			{
				tmp::BONE_INTER_DATA & bid = (*OutBoneInterData)[bi];

				bid.TmpBoneIndex = bj;

				VEC3 v;
				BlenderToDirectxTransform(&v, InBone.Head_Armaturespace);
				Transform(&bid.HeadPos, v, ArmatureToWorldMat);
				BlenderToDirectxTransform(&v, InBone.Tail_Armaturespace);
				Transform(&bid.TailPos, v, ArmatureToWorldMat);

				if (!float_equal(TmpArmature.Size.x, TmpArmature.Size.y) || !float_equal(TmpArmature.Size.y, TmpArmature.Size.z))
					Warning("Non uniform scaling of Armature object may give invalid bone envelopes.", 2342235);

				float ScaleFactor = (TmpArmature.Size.x + TmpArmature.Size.y + TmpArmature.Size.z) / 3.f;

				bid.HeadRadius = InBone.HeadRadius * ScaleFactor;
				bid.TailRadius = InBone.TailRadius * ScaleFactor;

				bid.Length = Distance(bid.HeadPos, bid.TailPos);

				break;
			}
		}
	}
}

// Zwraca wartoœæ krzywej w podanej klatce
// TmpCurve mo¿e byæ NULL.
// PtIndex - indeks nastêpnego punktu
// NextFrame - jeœli nastêpna klatka tej krzywej istnieje i jest mniejsza ni¿ NextFrame, ustawia NextFrame na ni¹
float CalcTmpCurveValue(
	float DefaultValue,
	const tmp::CURVE *TmpCurve,
	uint *InOutPtIndex,
	float Frame,
	float *InOutNextFrame)
{
	// Nie ma krzywej
	if (TmpCurve == NULL)
		return DefaultValue;
	// W ogóle nie ma punktów
	if (TmpCurve->Points.empty())
		return DefaultValue;
	// To jest ju¿ koniec krzywej - przed³u¿enie ostatniej wartoœci
	if (*InOutPtIndex >= TmpCurve->Points.size())
		return TmpCurve->Points[TmpCurve->Points.size()-1].y;

	const VEC2 & NextPt = TmpCurve->Points[*InOutPtIndex];

	// - Przestawiæ PtIndex (opcjonalnie)
	// - Przestawiæ NextFrame
	// - Zwróciæ wartoœæ

	// Jesteœmy dok³adnie w tym punkcie (lub za, co nie powinno mieæ miejsca)
	if (float_equal(NextPt.x, Frame) || (Frame > NextPt.x))
	{
		(*InOutPtIndex)++;
		if (*InOutPtIndex < TmpCurve->Points.size())
			*InOutNextFrame = std::min(*InOutNextFrame, TmpCurve->Points[*InOutPtIndex].x);
		return NextPt.y;
	}
	// Jesteœmy przed pierwszym punktem - przed³u¿enie wartoœci sta³ej w lewo
	else if (*InOutPtIndex == 0)
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		return NextPt.y;
	}
	// Jesteœmy miêdzy punktem poprzednim a tym
	else
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		const VEC2 & PrevPt = TmpCurve->Points[*InOutPtIndex - 1];
		// Interpolacja liniowa wartoœci wynikowej
		float t = (Frame - PrevPt.x) / (NextPt.x - PrevPt.x);
		return Lerp(PrevPt.y, NextPt.y, t);
	}
}

void TmpToQmsh_Animation(
	QMSH_ANIMATION *OutAnimation,
	const tmp::ACTION &TmpAction,
	const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp)
{
	OutAnimation->Name = TmpAction.Name;

	uint BoneCount = Qmsh.Bones.size();
	// Wspó³czynnik do przeliczania klatek na sekundy
	if (QmshTmp.FPS == 0)
		throw Error("FPS cannot be zero.");
	float TimeFactor = 1.f / (float)QmshTmp.FPS;

	// Lista wskaŸników do krzywych dotycz¹cych poszczególnych parametrów danej koœci QMSH
	// (Mog¹ byæ NULL)

	struct CHANNEL_POINTERS {
		const tmp::CURVE *LocX, *LocY, *LocZ, *QuatX, *QuatY, *QuatZ, *QuatW, *SizeX, *SizeY, *SizeZ;
		uint LocX_index, LocY_index, LocZ_index, QuatX_index, QuatY_index, QuatZ_index, QuatW_index, SizeX_index, SizeY_index, SizeZ_index;
	};
	std::vector<CHANNEL_POINTERS> BoneChannelPointers;
	BoneChannelPointers.resize(BoneCount);
	bool WarningInterpolation = false, WarningExtend = false;
	float EarliestNextFrame = MAXFLOAT;
	// Dla kolejnych koœci
	for (uint bi = 0; bi < BoneCount; bi++)
	{
		BoneChannelPointers[bi].LocX = NULL;  BoneChannelPointers[bi].LocX_index = 0;
		BoneChannelPointers[bi].LocY = NULL;  BoneChannelPointers[bi].LocY_index = 0;
		BoneChannelPointers[bi].LocZ = NULL;  BoneChannelPointers[bi].LocZ_index = 0;
		BoneChannelPointers[bi].QuatX = NULL; BoneChannelPointers[bi].QuatX_index = 0;
		BoneChannelPointers[bi].QuatY = NULL; BoneChannelPointers[bi].QuatY_index = 0;
		BoneChannelPointers[bi].QuatZ = NULL; BoneChannelPointers[bi].QuatZ_index = 0;
		BoneChannelPointers[bi].QuatW = NULL; BoneChannelPointers[bi].QuatW_index = 0;
		BoneChannelPointers[bi].SizeX = NULL; BoneChannelPointers[bi].SizeX_index = 0;
		BoneChannelPointers[bi].SizeY = NULL; BoneChannelPointers[bi].SizeY_index = 0;
		BoneChannelPointers[bi].SizeZ = NULL; BoneChannelPointers[bi].SizeZ_index = 0;

		//ZnajdŸ kana³ odpowiadaj¹cy tej koœci
		const string & BoneName = Qmsh.Bones[bi]->Name;
		for (uint ci = 0; ci < TmpAction.Channels.size(); ci++)
		{
			if (TmpAction.Channels[ci]->Name == BoneName)
			{
				// Kana³ znaleziony - przejrzyj jego krzywe
				const tmp::CHANNEL &TmpChannel = *TmpAction.Channels[ci].get();
				for (uint curvi = 0; curvi < TmpChannel.Curves.size(); curvi++)
				{
					const tmp::CURVE *TmpCurve = TmpChannel.Curves[curvi].get();
					// Zidentyfikuj po nazwie
					if (TmpCurve->Name == "LocX")
					{
						BoneChannelPointers[bi].LocX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "LocY")
					{
						BoneChannelPointers[bi].LocY = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "LocZ")
					{
						BoneChannelPointers[bi].LocZ = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatX")
					{
						BoneChannelPointers[bi].QuatX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatY")
					{
						BoneChannelPointers[bi].QuatY = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatZ")
					{
						BoneChannelPointers[bi].QuatZ = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatW")
					{
						BoneChannelPointers[bi].QuatW = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleX")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleY")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleW")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
				}

				break;
			}
		}

		if (BoneChannelPointers[bi].LocX != NULL && BoneChannelPointers[bi].LocX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocY != NULL && BoneChannelPointers[bi].LocY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocZ != NULL && BoneChannelPointers[bi].LocZ->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatX != NULL && BoneChannelPointers[bi].QuatX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatY != NULL && BoneChannelPointers[bi].QuatY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatZ != NULL && BoneChannelPointers[bi].QuatZ->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatW != NULL && BoneChannelPointers[bi].QuatW->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeX != NULL && BoneChannelPointers[bi].SizeX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeY != NULL && BoneChannelPointers[bi].SizeY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeZ != NULL && BoneChannelPointers[bi].SizeZ->Interpolation == tmp::INTERPOLATION_CONST)
		{
			WarningInterpolation = true;
		}
	}

	// Ostrze¿enia
	if (WarningInterpolation)
		Warning("Constant IPO interpolation mode not supported.", 2352634);

	// Wygenerowanie klatek kluczowych
	// (Nie ma ani jednej krzywej albo ¿adna nie ma punktów - nie bêdzie klatek kluczowych)
	while (EarliestNextFrame < MAXFLOAT)
	{
		float NextNext = MAXFLOAT;
		shared_ptr<QMSH_KEYFRAME> Keyframe(new QMSH_KEYFRAME);
		Keyframe->Time = (EarliestNextFrame - 1) * TimeFactor; // - 1, bo Blender liczy klatki od 1
		Keyframe->Bones.resize(BoneCount);
		for (uint bi = 0; bi < BoneCount; bi++)
		{
			Keyframe->Bones[bi].Translation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocX, &BoneChannelPointers[bi].LocX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocY, &BoneChannelPointers[bi].LocY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocZ, &BoneChannelPointers[bi].LocZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.x =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatX, &BoneChannelPointers[bi].QuatX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.z =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatY, &BoneChannelPointers[bi].QuatY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.y =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatZ, &BoneChannelPointers[bi].QuatZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.w = - CalcTmpCurveValue(1.f, BoneChannelPointers[bi].QuatW, &BoneChannelPointers[bi].QuatW_index, EarliestNextFrame, &NextNext);
			float ScalingX = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeX, &BoneChannelPointers[bi].SizeX_index, EarliestNextFrame, &NextNext);
			float ScalingY = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeY, &BoneChannelPointers[bi].SizeY_index, EarliestNextFrame, &NextNext);
			float ScalingZ = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeZ, &BoneChannelPointers[bi].SizeZ_index, EarliestNextFrame, &NextNext);
			if (!float_equal(ScalingX, ScalingY) || !float_equal(ScalingY, ScalingZ))
				Warning("Non uniform scaling of bone in animation IPO not supported.", 537536);
			Keyframe->Bones[bi].Scaling = (ScalingX + ScalingY + ScalingZ) / 3.f;
		}
		OutAnimation->Keyframes.push_back(Keyframe);

		EarliestNextFrame = NextNext;
	}

	// Czas trwania animacji, skalowanie czasu
	if (OutAnimation->Keyframes.empty())
		OutAnimation->Length = 0.f;
	else if (OutAnimation->Keyframes.size() == 1)
	{
		OutAnimation->Length = 0.f;
		OutAnimation->Keyframes[0]->Time = 0.f;
	}
	else
	{
		float TimeOffset = -OutAnimation->Keyframes[0]->Time;
		for (uint ki = 0; ki < OutAnimation->Keyframes.size(); ki++)
			OutAnimation->Keyframes[ki]->Time += TimeOffset;
		OutAnimation->Length = OutAnimation->Keyframes[OutAnimation->Keyframes.size()-1]->Time;
	}

	if (OutAnimation->Keyframes.size() > (uint4)MAXUINT2)
		Error(Format("Too many keyframes in animation \"#\"") % OutAnimation->Name);
}

void TmpToQmsh_Animations(QMSH *OutQmsh, const tmp::QMSH &QmshTmp )
{
	Writeln("Processing animations...");

	for (uint i = 0; i < QmshTmp.Actions.size(); i++)
	{
		shared_ptr<QMSH_ANIMATION> Animation(new QMSH_ANIMATION);
		TmpToQmsh_Animation(Animation.get(), *QmshTmp.Actions[i].get(), *OutQmsh, QmshTmp);
		OutQmsh->Animations.push_back(Animation);
	}

	if (OutQmsh->Animations.size() > (uint4)MAXUINT2)
		Error("Too many animations");
}

struct INTERMEDIATE_VERTEX_SKIN_DATA
{
	uint1 Index1;
	uint1 Index2;
	float Weight1;
};

void CalcVertexSkinData(
	std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> *Out,
	const tmp::MESH &TmpMesh,
	const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp,
	const std::vector<tmp::BONE_INTER_DATA> &BoneInterData)
{
	Out->resize(TmpMesh.Vertices.size());

	// Kolejny algorytm pisany z wielk¹ nadziej¹ ¿e zadzia³a, wolny ale co z tego, lepszy taki
	// ni¿ ¿aden, i tak siê nad nim namyœla³em ca³y dzieñ, a drugi poprawia³em bo siê wszystko pomiesza³o!

	// Nie ma skinningu
	if (TmpMesh.ParentArmature.empty())
	{
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = 0;
			(*Out)[vi].Index2 = 0;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Jest skinning ca³ej siatki do jednej koœci
	else if (!TmpMesh.ParentBone.empty())
	{
		// ZnajdŸ indeks tej koœci
		uint1 BoneIndex = 0;
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			if (Qmsh.Bones[bi]->Name == TmpMesh.ParentBone)
			{
				BoneIndex = bi + 1;
				break;
			}
		}
		// Nie znaleziono
		if (BoneIndex == 0)
			Warning("Object parented to non existing bone.", 13497325);
		// Przypisz t¹ koœæ do wszystkich wierzcho³ków
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = BoneIndex;
			(*Out)[vi].Index2 = BoneIndex;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Vertex Group lub Envelope
	else
	{
		// U³ó¿ szybk¹ listê wierzcho³ków z wagami dla ka¿dej koœci

		// Typ reprezentuje dla danej koœci grupê wierzcho³ków: Indeks wierzcho³ka => Waga
		typedef std::map<uint, float> INFLUENCE_MAP;
		std::vector<INFLUENCE_MAP> BoneInfluences;
		BoneInfluences.resize(Qmsh.Bones.size());
		// Dla ka¿dej koœci
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// ZnajdŸ odpowiadaj¹c¹ jej grupê wierzcho³ków w przetwarzanej w tym wywo³aniu funkcji siatce
			uint VertexGroupIndex = TmpMesh.VertexGroups.size();
			for (uint gi = 0; gi < TmpMesh.VertexGroups.size(); gi++)
			{
				if (TmpMesh.VertexGroups[gi]->Name == Qmsh.Bones[bi]->Name)
				{
					VertexGroupIndex = gi;
					break;
				}
			}
			// Jeœli znaleziono, przepisz wszystkie wierzcho³ki
			if (VertexGroupIndex < TmpMesh.VertexGroups.size())
			{
				const tmp::VERTEX_GROUP & VertexGroup = *TmpMesh.VertexGroups[VertexGroupIndex].get();
				for (uint vi = 0; vi < VertexGroup.VerticesInGroup.size(); vi++)
				{
					BoneInfluences[bi].insert(INFLUENCE_MAP::value_type(
						VertexGroup.VerticesInGroup[vi].Index,
						VertexGroup.VerticesInGroup[vi].Weight));
				}
			}
		}

		// Dla ka¿dego wierzcho³ka
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			// Zbierz wszystkie wp³ywaj¹ce na niego koœci
			struct INFLUENCE
			{
				uint BoneIndex; // Uwaga! Wy¿ej by³o VertexIndex -> Weight, a tutaj jest BoneIndex -> Weight - ale masakra !!!
				float Weight;

				bool operator > (const INFLUENCE &Influence) const { return Weight > Influence.Weight; }
			};
			std::vector<INFLUENCE> VertexInfluences;

			// Wp³yw przez Vertex Groups
			if (1)
			{
				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					INFLUENCE_MAP::iterator biit = BoneInfluences[bi].find(vi);
					if (biit != BoneInfluences[bi].end())
					{
						INFLUENCE Influence = { bi + 1, biit->second };
						VertexInfluences.push_back(Influence);
					}
				}
			}

			// ¯adna koœæ na niego nie wp³ywa - wyp³yw z Envelopes
			if (VertexInfluences.empty())
			{
				// U³ó¿ listê wp³ywaj¹cych koœci
				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					// Nie jestem pewny czy to jest dok³adnie algorytm u¿ywany przez Blender,
					// nie jest nigdzie dok³adnie opisany, ale z eksperymentów podejrzewam, ¿e tak.
					// Koœæ w promieniu - wp³yw maksymalny.
					// Koœæ w zakresie czegoœ co nazwa³em Extra Envelope te¿ jest wp³yw (podejrzewam ¿e s³abnie z odleg³oœci¹)
					// Promieñ Extra Envelope, jak uda³o mi siê wreszcie ustaliæ, jest niezale¿ny od Radius w sensie
					// ¿e rozci¹ga siê ponad Radius zawsze na tyle ile wynosi (BoneLength / 4)

					// Dodatkowy promieñ zewnêtrzny
					float ExtraEnvelopeRadius = BoneInterData[bi].Length * 0.25f;

					// Pozycja wierzcho³ka ju¿ jest w uk³adzie globalnym modelu i w konwencji DirectX
					float W = PointToBone(
						TmpMesh.Vertices[vi].Pos,
						BoneInterData[bi].HeadPos, BoneInterData[bi].HeadRadius, BoneInterData[bi].HeadRadius + ExtraEnvelopeRadius,
						BoneInterData[bi].TailPos, BoneInterData[bi].TailRadius, BoneInterData[bi].TailRadius + ExtraEnvelopeRadius);

					if (W > 0.f)
					{
						INFLUENCE Influence = { bi + 1, W };
						VertexInfluences.push_back(Influence);
					}
				}
			}
			// Jakieœ koœci na niego wp³ywaj¹ - weŸ z tych koœci

			// Zero koœci
			if (VertexInfluences.empty())
			{
				(*Out)[vi].Index1 = 0;
				(*Out)[vi].Index2 = 0;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Tylko jedna koœæ
			else if (VertexInfluences.size() == 1)
			{
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Dwie lub wiêcej koœci na liœcie wp³ywaj¹cych na ten wierzcho³ek
			else
			{
				// Posortuj wp³ywy na wierzcho³ek malej¹co, czyli od najwiêkszej wagi
				std::sort(VertexInfluences.begin(), VertexInfluences.end(), std::greater<INFLUENCE>());
				// WeŸ dwie najwa¿niejsze koœci
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[1].BoneIndex;
				// Oblicz wagê pierwszej
				// WA¯NY WZÓR NA ZNORMALIZOWAN¥ WAGÊ PIERWSZ¥ Z DWÓCH DOWOLNYCH WAG !!!
				(*Out)[vi].Weight1 = VertexInfluences[0].Weight / (VertexInfluences[0].Weight + VertexInfluences[1].Weight);
			}
		}
	}
}

void LoadBoneGroups(QMSH& qmsh, Tokenizer& t)
{
	t.RegisterKeyword(0, "file");
	t.RegisterKeyword(1, "bones");
	t.RegisterKeyword(2, "groups");
	t.RegisterKeyword(3, "group");
	t.RegisterKeyword(4, "name");
	t.RegisterKeyword(5, "parent");
	t.RegisterKeyword(6, "special");
	t.RegisterKeyword(7, "bone");
	t.RegisterKeyword(8, "output");
	t.Next();

	// file: name
	t.AssertKeyword(0);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	t.AssertToken(Tokenizer::TOKEN_STRING);
	t.Next();

	// [output: name]
	if(t.QueryKeyword(8))
	{
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		t.AssertToken(Tokenizer::TOKEN_STRING);
		t.Next();
	}

	// bones: X
	t.AssertKeyword(1);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	int bone_count = t.MustGetInt4();
	if(bone_count != qmsh.Bones.size())
		throw format("Niew³aœciwa liczba koœci %d do %d!", bone_count, qmsh.Bones.size());
	t.Next();

	// groups: X
	t.AssertKeyword(2);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	int groups = t.MustGetInt4();
	if(groups < 1 || groups > 255)
		throw format("Niew³aœciwa liczba grup %d!", groups);
	t.Next();

	qmsh.Groups.resize(groups);
	
	// ustaw parent ID na -1 aby zobaczyæ co nie zosta³o ustawione
	for(std::vector<QMSH_GROUP>::iterator it = qmsh.Groups.begin(), end = qmsh.Groups.end(); it != end; ++it)
		it->parent = (unsigned short)-1;

	// grupy
	// group: ID {
	//    name: "NAME"
	//    parent: ID
	// };
	for(int i=0; i<groups; ++i)
	{
		// group: ID
		t.AssertKeyword(3);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int id = t.MustGetInt4();
		if(id < 0 || id >= groups)
			throw format("Z³y identyfikator grupy %d!", id);
		QMSH_GROUP& gr = qmsh.Groups[id];
		if(gr.parent != (unsigned short)-1)
			throw format("Grupa '%s' zosta³a ju¿ zdefiniowana!", gr.name.c_str());
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		// name: "NAME"
		t.AssertKeyword(4);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		t.AssertToken(Tokenizer::TOKEN_STRING);
		gr.name = t.GetString();
		if(gr.name.empty())
			throw format("Nazwa grupy %d jest pusta!", id);
		int index = 0;
		for(std::vector<QMSH_GROUP>::iterator it = qmsh.Groups.begin(), end = qmsh.Groups.end(); it != end; ++it, ++index)
		{
			if(&*it != &gr && it->name == gr.name)
				throw format("Grupa %d nie mo¿e nazywaæ siê '%s'! Grupa %d ma ju¿ t¹ nazwê!", id, gr.name.c_str(), index);
		}
		t.Next();

		// parent: ID
		t.AssertKeyword(5);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int pid = t.MustGetInt4();
		if(pid < 0 || pid >= groups)
			throw format("%d nie mo¿e byæ grup¹ nadrzêdn¹ dla grup '%s'!", pid, gr.name.c_str());
		if(id == pid && id != 0)
			throw format("Tylko zerowa grupa mo¿e wskazywaæ na sam¹ siebie, grupa '%s'!", gr.name.c_str());
		if(id == 0 && pid != 0)
			throw "Zerowa grupa musi wskazywaæ na sam¹ siebie!";
		gr.parent = (unsigned short)pid;
		t.Next();

		// };
		t.AssertSymbol('}');
		t.Next();
		t.AssertSymbol(';');
		t.Next();
	}

	// sprawdŸ hierarchiê grup
	for(int i=0; i<groups; ++i)
	{
		int id = qmsh.Groups[i].parent;
		bool ok = false;
		
		for(int j=0; j<groups; ++j)
		{
			if(id == 0)
			{
				ok = true;
				break;
			}

			id = qmsh.Groups[id].parent;
		}

		if(!ok)
			throw format("Grupa %s ma uszkodzon¹ hierarchiê, nie prowadzi do zerowej!", qmsh.Groups[i].name.c_str());
	}

	// ustaw grupy w koœciach na -1
	for(std::vector<shared_ptr<QMSH_BONE> >::iterator it = qmsh.Bones.begin(), end = qmsh.Bones.end(); it != end; ++it)
	{
		(*it)->group_index = -1;
	}

	// wczytaj koœci
	// bone: "NAME" {
	//    group: ID
	//    specjal: X
	// };
	for(int i=0; i<bone_count; ++i)
	{
		// bone: "NAME"
		t.AssertKeyword(7);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		t.AssertToken(Tokenizer::TOKEN_STRING);
		string bone_name = t.GetString();
		QMSH_BONE* bone = NULL;
		int index = 0;
		for(std::vector<shared_ptr<QMSH_BONE> >::iterator it = qmsh.Bones.begin(), end = qmsh.Bones.end(); it != end; ++it)
		{
			if((*it)->Name == bone_name)
			{
				bone = &**it;
				break;
			}
		}
		if(!bone)
			throw format("Brak koœci o nazwie '%s'!", bone_name.c_str());
		if(bone->group_index != -1)
			throw format("Koœæ '%s' ma ju¿ ustawion¹ grupê %d!", bone_name.c_str(), bone->group_index);
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		// group: ID
		t.AssertKeyword(3);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int group_id = t.MustGetInt4();
		if(group_id < 0 || group_id >= groups)
			throw format("Brak grupy o id %d dla koœci '%s'!", group_id, bone_name.c_str());
		qmsh.Groups[group_id].bones.push_back(bone);
		bone->group_index = group_id;
		t.Next();

		// special: X
		t.AssertKeyword(6);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int special = t.MustGetInt4();
		if(special != 0 && special != 1)
			throw format("Nieznana wartoœæ specjalna %d dla koœci '%s'!", special, bone_name.c_str());
		bone->non_mesh = (special == 1 ? true : false);
		t.Next();

		// };
		t.AssertSymbol('}');
		t.Next();
		t.AssertSymbol(';');
		t.Next();
	}

	// sprawdŸ czy wszystkie grupy maj¹ jakieœ koœci
	for(int i=0; i<groups; ++i)
	{
		if(qmsh.Groups[i].bones.empty())
			throw format("Grupa '%s' nie ma koœci!", qmsh.Groups[i].name.c_str());
	}
}

bool SortBones(const shared_ptr<QMSH_BONE>& b1, const shared_ptr<QMSH_BONE>& b2)
{
	return b1->non_mesh < b2->non_mesh;
}

void CreateBoneGroups(QMSH& qmsh, const tmp::QMSH &QmshTmp, ConversionData& cs)
{
	if(cs.gopt == GO_ONE)
	{
		qmsh.Groups.resize(1);
		QMSH_GROUP& gr = qmsh.Groups[0];
		gr.name = "default";
		gr.parent = 0;

		for(uint i=0; i<qmsh.Bones.size(); ++i)
			gr.bones.push_back(&*qmsh.Bones[i]);
		
		qmsh.real_bones = qmsh.Bones.size();
	}
	else
	{
		FileStream input_file(cs.group_file, FM_READ);
		Tokenizer t(&input_file, 0);

		printf("Wczytywanie pliku konfiguracyjnego '%s'.\n", cs.group_file.c_str());
		
		try
		{
			LoadBoneGroups(qmsh, t);
		}
		catch(cstring err)
		{
			throw Error(format("B³¹d parsowania pliku konfiguracyjnego '%s'!\n%s", cs.group_file.c_str(), err), __FILE__, __LINE__);
		}
		catch(TokenizerError& err)
		{
			string msg;
			err.GetMessage_(&msg);
			throw Error(format("B³¹d parsowania pliku konfiguracyjnego '%s'!\n%s", cs.group_file.c_str(), msg.c_str()), __FILE__, __LINE__);
		}

		// policz ile jest specjalnych koœci
		int special_count = 0;
		for(std::vector< shared_ptr<QMSH_BONE> >::iterator it = qmsh.Bones.begin(), end = qmsh.Bones.end(); it != end; ++it)
		{
			if((*it)->non_mesh)
				++special_count;
		}

		// same normalne koœci, my job is done here!
		if(special_count == 0)
		{
			qmsh.real_bones = qmsh.Bones.size();
			return;
		}

		std::sort(qmsh.Bones.begin(), qmsh.Bones.end(), SortBones);
		qmsh.real_bones = qmsh.Bones.size() - special_count;

		printf("Plik wczytany.\n");
	}
}

void ConvertQmshTmpToQmsh(QMSH *Out, const tmp::QMSH &QmshTmp, ConversionData& cs)
{
	// BoneInterData odpowiada koœciom wyjœciowym QMSH.Bones, nie wejœciowym z TMP.Armature!!!
	std::vector<tmp::BONE_INTER_DATA> BoneInterData;

	Out->Flags = 0;
	if(cs.export_phy)
		Out->Flags |= FLAG_PHYSICS;
	else
	{
		if(QmshTmp.static_anim)
			Out->Flags |= FLAG_STATIC;
		if (QmshTmp.Armature != NULL)
		{
			Out->Flags |= FLAG_SKINNING;
			
			// Przetwórz koœci
			TmpToQmsh_Bones(Out, &BoneInterData, QmshTmp);

			// Przetwórz animacje
			if(!QmshTmp.static_anim && cs.gopt != GO_CREATE)
				TmpToQmsh_Animations(Out, QmshTmp);
		}

		if(cs.gopt == GO_CREATE)
			return;

		CreateBoneGroups(*Out, QmshTmp, cs);
	}

	if(QmshTmp.split)
		Out->Flags |= FLAG_SPLIT;

	BlenderToDirectxTransform(&Out->camera_pos, QmshTmp.camera_pos);
	BlenderToDirectxTransform(&Out->camera_target, QmshTmp.camera_target);
	BlenderToDirectxTransform(&Out->camera_up, QmshTmp.camera_up);

	printf("Processing mesh (NVMeshMender and more)...\n");

	struct NV_FACE
	{
		uint MaterialIndex;
		NV_FACE(uint MaterialIndex) : MaterialIndex(MaterialIndex) { }
	};

	// Dla kolejnych obiektów TMP
	for (uint oi = 0; oi < QmshTmp.Meshes.size(); oi++)
	{
		tmp::MESH & o = *QmshTmp.Meshes[oi].get();

		// Policz wp³yw koœci na wierzcho³ki tej siatki
		std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> VertexSkinData;
		if ((Out->Flags & FLAG_SKINNING) != 0)
			CalcVertexSkinData(&VertexSkinData, o, *Out, QmshTmp, BoneInterData);

		// Skonstruuj struktury poœrednie
		// UWAGA! Z³o¿onoœæ kwadratowa.
		// Myœla³em nad tym ca³y dzieñ i nic m¹drego nie wymyœli³em jak to ³adniej zrobiæ :)

		bool MaterialIndicesUse[16];
		ZeroMem(&MaterialIndicesUse, 16*sizeof(bool));

		std::vector<MeshMender::Vertex> NvVertices;
		std::vector<unsigned int> MappingNvToTmpVert;
		std::vector<unsigned int> NvIndices;
		std::vector<NV_FACE> NvFaces;
		std::vector<unsigned int> NvMappingOldToNewVert;

		NvVertices.reserve(o.Vertices.size());
		NvIndices.reserve(o.Faces.size()*4);
		NvFaces.reserve(o.Faces.size());
		NvMappingOldToNewVert.reserve(o.Vertices.size());

		// Dla kolejnych œcianek TMP

		for (uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			if (f.MaterialIndex >= 16)
				throw Error("Material index out of range.");
			MaterialIndicesUse[f.MaterialIndex] = true;

			// Pierwszy trójk¹t
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[1], o.Vertices[f.VertexIndices[1]], f.TexCoords[1], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
			// (wierzcho³ki dodane/zidenksowane, indeksy dodane - zostaje œcianka)
			NvFaces.push_back(NV_FACE(f.MaterialIndex));

			// Drugi trójk¹t
			if (f.NumVertices == 4)
			{
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[3], o.Vertices[f.VertexIndices[3]], f.TexCoords[3], f.Normal, o.vertex_normals));
				NvFaces.push_back(NV_FACE(f.MaterialIndex));
			}
		}

		// Struktury poœrednie chyba ju¿ poprawnie wype³nione, czas odpaliæ NVMeshMender!

		float MinCreaseCosAngle = cosf(DegToRad(30)); // AutoSmoothAngle w przeciwieñstwie do k¹tów Eulera jest w stopniach!
		float WeightNormalsByArea = 1.0f;
		bool CalcNormals = !o.vertex_normals && !cs.export_phy;
		bool RespectExistingSplits = CalcNormals;
		bool FixCylindricalWrapping = false;

		MeshMender mender;
		if (!mender.Mend(
			NvVertices,
			NvIndices,
			NvMappingOldToNewVert,
			MinCreaseCosAngle, MinCreaseCosAngle, MinCreaseCosAngle,
			WeightNormalsByArea,
			CalcNormals ? MeshMender::CALCULATE_NORMALS : MeshMender::DONT_CALCULATE_NORMALS,
			RespectExistingSplits ? MeshMender::RESPECT_SPLITS : MeshMender::DONT_RESPECT_SPLITS,
			FixCylindricalWrapping ? MeshMender::FIX_CYLINDRICAL : MeshMender::DONT_FIX_CYLINDRICAL))
		{
			throw Error("NVMeshMender failed.");
		}

		assert(NvIndices.size() % 3 == 0);

		if (NvVertices.size() > (uint)MAXUINT2)
			throw Error(Format("Too many vertices in object \"#\": #") % o.Name % NvVertices.size());

		// Skonstruuj obiekty wyjœciowe QMSH

		// Liczba u¿ytych materia³ow
		uint NumMatUse = 0;
		for (uint mi = 0; mi < 16; mi++)
			if (MaterialIndicesUse[mi])
				NumMatUse++;

		// Wierzcho³ki (to bêdzie wspólne dla wszystkich podsiatek QMSH powsta³ych z bie¿¹cego obiektu QMSH TMP)
		uint MinVertexIndexUsed = Out->Vertices.size();
		uint NumVertexIndicesUsed = NvVertices.size();
		Out->Vertices.reserve(Out->Vertices.size() + NumVertexIndicesUsed);
		for (uint vi = 0; vi < NvVertices.size(); vi++)
		{
			QMSH_VERTEX v;
			v.Pos = NvVertices[vi].pos;
			v.Tex.x = NvVertices[vi].s;
			v.Tex.y = NvVertices[vi].t;
			v.Normal = NvVertices[vi].normal;
			//BlenderToDirectxTransform(&v.Tangent, NvVertices[vi].tangent);
			//BlenderToDirectxTransform(&v.Binormal, NvVertices[vi].binormal);
			v.Tangent = NvVertices[vi].tangent;
			v.Binormal = NvVertices[vi].binormal;
			if (QmshTmp.Armature != NULL)
			{
				const INTERMEDIATE_VERTEX_SKIN_DATA &ivsd = VertexSkinData[MappingNvToTmpVert[NvMappingOldToNewVert[vi]]];
				v.BoneIndices = (ivsd.Index1) | (ivsd.Index2 << 8);
				v.Weight1 = ivsd.Weight1;
			}
			Out->Vertices.push_back(v);
		}

		// Dla wszystkich u¿ytych materia³ów
		for (uint mi = 0; mi < 16; mi++)
		{
			if (MaterialIndicesUse[mi])
			{
				shared_ptr<QMSH_SUBMESH> submesh(new QMSH_SUBMESH);
				tmp::Material& m = *o.Materials[mi];

				submesh->name = m.name;
				submesh->specular_color = m.specular_color;
				submesh->specular_intensity = m.specular_intensity;
				submesh->specular_hardness = m.specular_hardness;
				submesh->normal_factor = 0;
				submesh->specular_factor = 0;
				submesh->specular_color_factor = 0;

				if (mi < o.Materials.size())
				{
					if(m.image.empty())
					{
						// nowe mo¿liwoœci, kilka tekstur
						for(std::vector<shared_ptr<tmp::Texture> >::iterator tit = m.textures.begin(), tend = m.textures.end(); tit != tend; ++tit)
						{
							tmp::Texture& t = **tit;
							if(t.use_diffuse && !t.use_normal && !t.use_specular && !t.use_specular_color)
							{
								// diffuse
								if(!submesh->texture.empty())
									throw Error(Format("Mesh '#', material '#', texture '#': material already have diffuse texture '#'!") % o.Name % m.name % t.image % submesh->texture);
								submesh->texture = t.image;
								if(!float_equal(t.diffuse_factor, 1.f))
								{
									printf("WARN: Mesh '%s', material '%s', texture '%s' has diffuse factor %g (only 1.0 is supported).", o.Name.c_str(), m.name.c_str(), t.image.c_str(),
										t.diffuse_factor);
								}
							}
							else if(!t.use_diffuse && t.use_normal && !t.use_specular && !t.use_specular_color)
							{
								// normal
								if(!submesh->normalmap_texture.empty())
								{
									throw Error(Format("Mesh '#', material '#', texture '#': material already have normal texture '#'!") % o.Name % m.name % t.image %
										submesh->normalmap_texture);
								}
								submesh->normalmap_texture = t.image;
								submesh->normal_factor = t.normal_factor;

								// eksportuj tangents
								Out->Flags |= FLAG_TANGENTS;
							}
							else if(!t.use_diffuse && !t.use_normal && t.use_specular && t.use_specular_color)
							{
								// specular
								if(!submesh->specularmap_texture.empty())
								{
									throw Error(Format("Mesh '#', material '#', texture '#': material already have specular texture '#'!") % o.Name % m.name % t.image % 
										submesh->specularmap_texture);
								}
								submesh->specularmap_texture = t.image;
								submesh->specular_factor = t.specular_factor;
								submesh->specular_color_factor = t.specular_color_factor;
							}
						}
					}
					else
					{
						// zwyk³a tekstura diffuse
						submesh->texture = m.image;
					}
				}

				// Indeksy
				submesh->MinVertexIndexUsed = MinVertexIndexUsed;
				submesh->NumVertexIndicesUsed = NumVertexIndicesUsed;
				submesh->FirstTriangle = Out->Indices.size() / 3;

				// Dodaj indeksy
				submesh->NumTriangles = 0;
				Out->Indices.reserve(Out->Indices.size() + NvIndices.size());
				for (uint fi = 0, ii = 0; fi < NvFaces.size(); fi++, ii += 3)
				{
					if (NvFaces[fi].MaterialIndex == mi)
					{
						Out->Indices.push_back((uint2)(NvIndices[ii    ] + MinVertexIndexUsed));
						Out->Indices.push_back((uint2)(NvIndices[ii + 1] + MinVertexIndexUsed));
						Out->Indices.push_back((uint2)(NvIndices[ii + 2] + MinVertexIndexUsed));
						submesh->NumTriangles++;
					}
				}

				// Podsiatka gotowa
				Out->Submeshes.push_back(submesh);
			}
		}
	}

	if(QmshTmp.force_tangents)
		Out->Flags |= FLAG_TANGENTS;

	printf("Zapisywanie punktów.\n");

	// punkty
	const uint n_bones = Out->Bones.size();
	for(std::vector< shared_ptr<tmp::POINT> >::const_iterator it = QmshTmp.points.begin(), end = QmshTmp.points.end(); it != end; ++it)
	{
		shared_ptr<QMSH_POINT> point(new QMSH_POINT);
		tmp::POINT& pt = *((*it).get());

		point->name = pt.name;
		point->size = pt.size;
		
		if(pt.type == "CUBE")
			point->type = 2;
		else if(pt.type == "SPHERE")
			point->type = 1;
		else
			point->type = 0;

		BlenderToDirectxTransform(&point->matrix, pt.matrix);
		BlenderToDirectxTransform(&point->rot, pt.rot);
			
		uint i = 0;
		for(std::vector< shared_ptr<QMSH_BONE> >::const_iterator it = Out->Bones.begin(), end = Out->Bones.end(); it != end; ++i, ++it)
		{
			if((*it)->Name == pt.bone)
			{
				point->bone = i;
				break;
			}
		}

		Out->Points.push_back(point);
	}
}

void LoadQmshTmp(QMSH *Out, ConversionData& cs )
{
	// Wczytaj plik QMSH.TMP
	Writeln("Loading QMSH TMP file \"" + cs.input + "\"...");

	tmp::QMSH Tmp;
	LoadQmshTmpFile(&Tmp, cs.input);
	if(Tmp.Meshes.empty())
		throw Error("Empty mesh!");

	// Przekszta³æ uk³ad wspó³rzêdnych
	TransformQmshTmpCoords(&Tmp);

	// Przekonwertuj do QMSH
	ConvertQmshTmpToQmsh(Out, Tmp, cs);
}

// Zapisuje QMSH do pliku
void SaveQmsh(const QMSH &Qmsh, const string &FileName)
{
	ERR_TRY;

	Writeln("Saving QMSH file \"" + FileName + "\"...");

	FileStream F(FileName, FM_WRITE);

	assert(Qmsh.Flags < 0xFF);
	assert(Qmsh.Indices.size() % 3 == 0);

	// Nag³ówek
	F.WriteStringF("QMSH");
	F.WriteEx((uint1)QMSH_VERSION);
	F.WriteEx((uint1)Qmsh.Flags);
	F.WriteEx((uint2)Qmsh.Vertices.size());
	F.WriteEx((uint2)(Qmsh.Indices.size()/3));
	F.WriteEx((uint2)Qmsh.Submeshes.size());
	uint2 n_bones = Qmsh.Bones.size();
	if( (Qmsh.Flags & FLAG_STATIC) != 0 )
		n_bones = 0;
	F.WriteEx(n_bones);
	F.WriteEx((uint2)Qmsh.Animations.size());
	F.WriteEx((uint2)Qmsh.Points.size());
	F.WriteEx((uint2)Qmsh.Groups.size());

	// Bry³y otaczaj¹ce
	F.WriteEx(Qmsh.BoundingSphereRadius);
	F.WriteEx(Qmsh.BoundingBox);

	//
	//F.WriteEx(Qmsh.real_bones);
	F.WriteEx(Qmsh.camera_pos);
	F.WriteEx(Qmsh.camera_target);
	F.WriteEx(Qmsh.camera_up);

	// Wierzcho³ki
	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		const QMSH_VERTEX & v = Qmsh.Vertices[vi];

		F.WriteEx(v.Pos);

		if((Qmsh.Flags & FLAG_PHYSICS) == 0)
		{
			if ((Qmsh.Flags & FLAG_SKINNING) != 0)
			{
				F.WriteEx(v.Weight1);
				F.WriteEx(v.BoneIndices);
			}

			F.WriteEx(v.Normal);
			F.WriteEx(v.Tex);

			if ((Qmsh.Flags & FLAG_TANGENTS) != 0)
			{
				F.WriteEx(v.Tangent);
				F.WriteEx(v.Binormal);
			}
		}
	}

	// Indeksy (trójk¹ty)
	F.Write(&Qmsh.Indices[0], sizeof(uint2)*Qmsh.Indices.size());

	// Podsiatki
	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		const QMSH_SUBMESH & s = *Qmsh.Submeshes[si].get();

		F.WriteEx((uint2)s.FirstTriangle);
		F.WriteEx((uint2)s.NumTriangles);
		F.WriteEx((uint2)s.MinVertexIndexUsed);
		F.WriteEx((uint2)s.NumVertexIndicesUsed);
		F.WriteString1(s.name);
		F.WriteString1(s.texture);
		F.WriteEx(s.specular_color);
		F.WriteEx(s.specular_intensity);
		F.WriteEx(s.specular_hardness);
		if((Qmsh.Flags & FLAG_TANGENTS) != 0)
		{
			F.WriteString1(s.normalmap_texture);
			if(!s.normalmap_texture.empty())
				F.WriteEx(s.normal_factor);
		}
		F.WriteString1(s.specularmap_texture);
		if(!s.specularmap_texture.empty())
		{
			F.WriteEx(s.specular_factor);
			F.WriteEx(s.specular_color_factor);
		}
	}

	// Koœci
	if ((Qmsh.Flags & FLAG_SKINNING) != 0 && (Qmsh.Flags & FLAG_STATIC) == 0)
	{
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			F.WriteEx((uint2)Qmsh.Bones[bi]->ParentIndex);

			F.WriteEx(Qmsh.Bones[bi]->Matrix._11);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._12);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._13);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._21);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._22);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._23);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._31);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._32);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._33);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._41);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._42);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._43);

			//F.WriteEx(Qmsh.Bones[bi]->point);

			F.WriteString1(Qmsh.Bones[bi]->Name);
		}
	}

	// Animacje
	if ((Qmsh.Flags & FLAG_SKINNING) != 0)
	{
		for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
		{
			const QMSH_ANIMATION & Animation = *Qmsh.Animations[ai].get();

			F.WriteString1(Animation.Name);
			F.WriteEx(Animation.Length);
			F.WriteEx((uint2)Animation.Keyframes.size());

			for (uint ki = 0; ki < Animation.Keyframes.size(); ki++)
			{
				const QMSH_KEYFRAME & Keyframe = *Animation.Keyframes[ki].get();

				F.WriteEx(Keyframe.Time);

				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					F.WriteEx(Keyframe.Bones[bi].Translation);
					F.WriteEx(Keyframe.Bones[bi].Rotation);
					F.WriteEx(Keyframe.Bones[bi].Scaling);
				}
			}
		}
	}
	
	// punkty
	for(uint i=0; i<Qmsh.Points.size(); ++i)
	{
		const QMSH_POINT& point = *Qmsh.Points[i].get();

		F.WriteString1(point.name);
		F.WriteEx(point.matrix);
		F.WriteEx(unsigned short(point.bone+1));
		F.WriteEx(point.type);
		F.WriteEx(point.size);
		F.WriteEx(point.rot);
	}

	// grupy
	if((Qmsh.Flags & FLAG_SKINNING) != 0 && (Qmsh.Flags & FLAG_STATIC) == 0)
	{
		for(uint i=0; i<Qmsh.Groups.size(); ++i)
		{
			const QMSH_GROUP& group = Qmsh.Groups[i];

			F.WriteString1(group.name);
			byte size = group.bones.size();
			F.WriteEx(group.parent);
			F.WriteEx(size);

			for(byte j=0; j<size; ++j)
			{
				for(byte k=0; k<=Qmsh.Bones.size(); ++k)
				{
					if(&*Qmsh.Bones[k] == group.bones[j])
					{
						F.WriteEx(byte(k+1));
						break;
					}
				}
			}
		}
	}

	// split
	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::const_iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
		{
			const QMSH_SUBMESH& sub = **it;
			F.WriteEx(sub.center);
			F.WriteEx(sub.range);
			F.WriteEx(sub.box);
		}
	}

	ERR_CATCH_FUNC("Cannot save mesh to file: " + FileName);
}

//====================================================================================================
// Konwersja qmsh.tmp -> qmsh
// Lub generowanie pliku konfiguracyjnego
//====================================================================================================
void Convert(ConversionData& cs)
{
	QMSH Qmsh;

	LoadQmshTmp(&Qmsh, cs);

	if(cs.gopt == GO_CREATE)
	{
		printf("Zapisywanie pliku konfiguracyjnego: %s.\n", cs.group_file.c_str());

		FileStream file(cs.group_file, FM_WRITE);

		// nazwa pliku
		file.WriteStringF(format("file: \"%s\"\n", cs.input.c_str()));

		// nazwa pliku wyjœciowego
		if(cs.force_output)
			file.WriteStringF(format("output: \"%s\"\n", cs.output.c_str()));
		
		// liczba koœci, grupy
		file.WriteStringF(format("bones: %u\ngroups: 1\ngroup: 0 {\n\tname: \"default\"\n\tparent: 0\n};\n", Qmsh.Bones.size()));

		// koœci
		for(std::vector<shared_ptr<QMSH_BONE> >::iterator it = Qmsh.Bones.begin(), end = Qmsh.Bones.end(); it != end; ++it)
			file.WriteStringF(format("bone: \"%s\" {\n\tgroup: 0\n\tspecial: 0\n};\n", (*it)->Name.c_str()));
	}
	else
	{
		CalcBoundingVolumes(Qmsh);
		SaveQmsh(Qmsh, cs.output);
	}
}
