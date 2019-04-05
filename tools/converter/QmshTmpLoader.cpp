#include "PCH.hpp"
#include "QmshTmpLoader.h"

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
	T_SPLIT,
	T_GROUP
};

const Int2 QmshTmpLoader::QMSH_TMP_HANDLED_VERSION = Int2(12, 21);
const Vec3 DefaultDiffuseColor(0.8f, 0.8f, 0.8f);
const float DefaultDiffuseIntensity = 0.8f;
const Vec3 DefaultSpecularColor(1, 1, 1);
const float DefaultSpecularIntensity = 0.5f;
const int DefaultSpecularHardness = 50;

void QmshTmpLoader::LoadQmshTmpFile(tmp::QMSH *Out, const string &FileName)
{
	Tokenizer t(Tokenizer::F_JOIN_MINUS | Tokenizer::F_UNESCAPE);
	t.FromFile(FileName);

	t.AddKeywords(0, {
		{ "objects", T_OBJECTS },
		{ "mesh", T_MESH },
		{ "materials", T_MATERIALS },
		{ "vertices", T_VERTICES },
		{ "faces", T_FACES },
		{ "vertex_groups", T_VERTEX_GROUPS },
		{ "armature", T_ARMATURE },
		{ "bone", T_BONE },
		{ "head", T_HEAD },
		{ "tail", T_TAIL },
		{ "actions", T_ACTIONS },
		{ "params", T_PARAMS },
		{ "fps", T_FPS },
		{ "static", T_STATIC },
		{ "points", T_POINTS },
		{ "pos", T_POS },
		{ "rot", T_ROT },
		{ "scale", T_SCALE },
		{ "parent", T_PARENT },
		{ "image", T_IMAGE },
		{ "none", T_NONE },
		{ "empty", T_EMPTY },
		{ "type", T_TYPE },
		{ "size", T_SIZE },
		{ "matrix", T_MATRIX },
		{ "camera", T_CAMERA },
		{ "material", T_MATERIAL },
		{ "texture", T_TEXTURE },
		{ "diffuse", T_DIFFUSE },
		{ "normal", T_NORMAL },
		{ "specular", T_SPECULAR },
		{ "specular_color", T_SPECULAR_COLOR },
		{ "tangents", T_TANGENTS },
		{ "split", T_SPLIT },
		{ "group", T_GROUP }
		});

	try
	{
		LoadQmshTmpFileInternal(Out, t);
	}
	catch(const Tokenizer::Exception& ex)
	{
		throw Format("Failed to load qmsh tmp: %s", ex.ToString());
	}
}

void QmshTmpLoader::LoadQmshTmpFileInternal(tmp::QMSH* Out, Tokenizer& t)
{
	t.Next();

	// Nag³ówek
	t.AssertItem("QMSH");
	t.Next();

	t.AssertItem("TMP");
	t.Next();

	uint wersja = t.MustGetUint();
	if(wersja < (uint)QMSH_TMP_HANDLED_VERSION.x || wersja > (uint)QMSH_TMP_HANDLED_VERSION.y)
		t.Throw("B³êdna wersja");
	t.Next();
	load_version = wersja;

	// Pocz¹tek
	t.AssertKeyword(T_OBJECTS);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	// Obiekty
	for(;;)
	{
		if(t.IsSymbol('}'))
		{
			t.Next();
			break;
		}

		TOKEN token = (TOKEN)t.MustGetKeywordId();
		if(token == T_MESH)
		{
			// mesh
			t.Next();

			shared_ptr<tmp::MESH> object(new tmp::MESH);

			// Nazwa
			object->Name = t.MustGetString();
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
			object->ParentArmature = t.MustGetString();
			t.Next();
			// parent bone
			object->ParentBone = t.MustGetString();
			t.Next();

			// Materia³y
			t.AssertKeyword(T_MATERIALS);
			t.Next();
			if(wersja >= 16 && wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}

			uint NumMaterials = t.MustGetUint();
			object->Materials.resize(NumMaterials);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			if(wersja >= 16)
			{
				for(uint mi = 0; mi < NumMaterials; mi++)
				{
					shared_ptr<tmp::Material> m(new tmp::Material);

					// material
					t.AssertKeyword(T_MATERIAL);
					t.Next();

					// name
					m->name = t.MustGetString();
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
						m->specular_hardness = t.MustGetInt();
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
						if(t.IsSymbol('}'))
						{
							t.Next();
							break;
						}
						t.AssertKeyword(T_TEXTURE);
						t.Next();

						// image
						shared_ptr<tmp::Texture> tex(new tmp::Texture);
						tex->image = t.MustGetString();
						t.Next();

						// {
						t.AssertSymbol('{');
						t.Next();

						while(true)
						{
							// keyword lub }
							if(t.IsSymbol('}'))
							{
								t.Next();
								break;
							}
							TOKEN id = (TOKEN)t.MustGetKeywordId();
							if(id != T_DIFFUSE && id != T_SPECULAR && id != T_SPECULAR_COLOR && id != T_NORMAL)
								t.Throw("Oczekiwano na rodzaj tekstury.");
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
				for(uint mi = 0; mi < NumMaterials; mi++)
				{
					shared_ptr<tmp::Material> m(new tmp::Material);
					m->name = t.MustGetString();
					t.Next();

					t.AssertSymbol(',');
					t.Next();

					if(t.IsKeyword(T_IMAGE))
					{
						t.Next();
						m->image = t.MustGetString();
						t.Next();
					}
					else if(t.IsKeyword(T_NONE))
						t.Next();
					else
					{
						int k1 = T_IMAGE,
							k2 = T_NONE;
						t.StartUnexpected().Add(tokenizer::T_KEYWORD, &k1).Add(tokenizer::T_KEYWORD, &k2).Throw();
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

			uint NumVertices = t.MustGetUint();
			object->Vertices.resize(NumVertices);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			if(wersja >= 17)
			{
				object->vertex_normals = true;
				for(uint vi = 0; vi < NumVertices; vi++)
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
				for(uint vi = 0; vi < NumVertices; vi++)
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

			uint NumFaces = t.MustGetUint();
			object->Faces.resize(NumFaces);
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			uint use_smooth = 0, use_flat = 0;
			for(uint fi = 0; fi < NumFaces; fi++)
			{
				tmp::FACE & f = object->Faces[fi];

				f.NumVertices = t.MustGetUint();
				if(f.NumVertices != 3 && f.NumVertices != 4)
					throw "B³êdna liczba wierzcho³ków w œciance.";
				t.Next();

				f.MaterialIndex = t.MustGetUint();
				t.Next();

				f.Smooth = (t.MustGetUint() > 0);
				if(f.Smooth)
					++use_smooth;
				else
					++use_flat;
				t.Next();

				// Wierzcho³ki tej œcianki
				for(uint vi = 0; vi < f.NumVertices; vi++)
				{
					// Indeks wierzcho³ka
					f.VertexIndices[vi] = t.MustGetUint();
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

			if(use_smooth > 0 && use_flat > 0)
			{
				WarnOnce(2, "Mixed smooth/flat shading.");
				object->vertex_normals = false;
			}
			else if(use_flat)
				object->vertex_normals = false;

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

			while(!t.IsSymbol('}'))
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
		else if(token == T_ARMATURE)
		{
			t.Next();

			if(Out->Armature != NULL)
				t.Throw("Multiple armature objects not allowed.");

			Out->Armature.reset(new tmp::ARMATURE);
			tmp::ARMATURE & arm = *Out->Armature.get();

			arm.Name = t.MustGetString();
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			// Parametry ogólne
			ParseVec3(&arm.Position, t);
			ParseVec3(&arm.Orientation, t);
			ParseVec3(&arm.Size, t);

			if(wersja < 17)
			{
				t.MustGetUint();
				t.Next();

				t.MustGetUint();
				t.Next();
			}

			// Koœci
			while(!t.IsSymbol('}'))
			{
				if(t.IsKeyword(T_GROUP))
				{
					shared_ptr<tmp::BONE_GROUP> group(new tmp::BONE_GROUP);
					ParseBoneGroup(group.get(), t);
					arm.Groups.push_back(group);
				}
				else
				{
					shared_ptr<tmp::BONE> bone(new tmp::BONE);
					ParseBone(bone.get(), t, wersja);
					arm.Bones.push_back(bone);
				}
			}
		}
		else if(token == T_EMPTY)
		{
			t.Next();
			shared_ptr<tmp::POINT> point(new tmp::POINT);

			// nazwa
			point->name = t.MustGetString();
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
			point->bone = t.MustGetString();
			t.Next();

			// type
			t.AssertKeyword(T_TYPE);
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol(':');
				t.Next();
			}
			point->type = t.MustGetString();
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
				Vec3 size;
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

				point->size = size * scale;
			}
			else
			{
				float scale = t.MustGetFloat();
				t.Next();

				point->size = Vec3(scale, scale, scale);
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
		else if(token == T_CAMERA)
		{
			// kamera

			// {
			t.Next();
			t.AssertSymbol('{');
			t.Next();

			// pozycja
			Vec3 pos, rot;
			ParseVec3(&pos, t);
			ParseVec3(&rot, t);

			// up
			if(wersja >= 15)
			{
				Vec3 up;
				ParseVec3(&up, t);
				Out->camera_up = up;
			}
			else
				Out->camera_up = Vec3(0, 0, 1);

			Out->camera_pos = pos;
			Out->camera_target = rot;
		}
		else
			t.Unexpected();

		t.AssertSymbol('}');
		t.Next();
	}

	// actions
	t.AssertKeyword(T_ACTIONS);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
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

	while(!t.IsSymbol('}'))
	{
		int id = t.MustGetKeywordId();
		switch(id)
		{
		case T_FPS:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->FPS = t.MustGetInt();
			t.Next();
			break;
		case T_STATIC:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->static_anim = (t.MustGetInt() > 0);
			t.Next();
			break;
		case T_TANGENTS:
			t.Next();
			if(wersja < 18)
			{
				t.AssertSymbol('=');
				t.Next();
			}
			Out->force_tangents = (t.MustGetInt() > 0);
			t.Next();
			break;
		case T_SPLIT:
			t.Next();
			Out->split = (t.MustGetInt() > 0);
			t.Next();
			break;
		default:
			t.Unexpected();
			break;
		}
	}

	t.Next();
	t.AssertEof();
}

void QmshTmpLoader::ParseVec2(Vec2 *Out, Tokenizer &t)
{
	Out->x = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->y = t.MustGetFloat();
	t.Next();
}

void QmshTmpLoader::ParseVec3(Vec3 *Out, Tokenizer &t)
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

void QmshTmpLoader::ParseMatrix3x3(Matrix *Out, Tokenizer &t, bool old)
{
	Out->_11 = t.MustGetFloat(); t.Next(); t.AssertSymbol(','); t.Next();
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

void QmshTmpLoader::ParseMatrix4x4(Matrix *Out, Tokenizer &t, bool old)
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

void QmshTmpLoader::ParseVertexGroup(tmp::VERTEX_GROUP *Out, Tokenizer &t)
{
	Out->Name = t.MustGetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		tmp::VERTEX_IN_GROUP vig;

		vig.Index = t.MustGetUint();
		t.Next();

		vig.Weight = t.MustGetFloat();
		t.Next();

		Out->VerticesInGroup.push_back(vig);
	}
	t.Next();
}

void QmshTmpLoader::ParseBone(tmp::BONE *Out, Tokenizer &t, int wersja)
{
	t.AssertKeyword(T_BONE);
	t.Next();

	Out->Name = t.MustGetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	// parent bone
	if(wersja >= 21)
	{
		t.AssertKeyword(T_PARENT);
		t.Next();
	}

	Out->Parent = t.MustGetString();
	t.Next();

	// bone group
	if(wersja >= 21)
	{
		t.AssertKeyword(T_GROUP);
		t.Next();

		Out->Group = t.MustGetString();
		t.Next();
	}

	// head
	t.AssertKeyword(T_HEAD);
	t.Next();
	if(wersja < 21)
	{
		// old bonespace head pos
		Vec3 unused;
		ParseVec3(&unused, t);
	}
	ParseVec3(&Out->Head, t);
	Out->HeadRadius = t.MustGetFloat();
	t.Next();

	// tail
	t.AssertKeyword(T_TAIL);
	t.Next();
	if(wersja < 21)
	{
		// old bonespace tail pos
		Vec3 unused;
		ParseVec3(&unused, t);
	}
	ParseVec3(&Out->Tail, t);
	Out->TailRadius = t.MustGetFloat();
	t.Next();

	// Reszta
	if(wersja < 17)
	{
		// old roll
		t.MustGetFloat();
		t.Next();
		t.MustGetFloat();
		t.Next();
	}
	if(wersja < 21)
	{
		// old length
		t.MustGetFloat();
		t.Next();
	}
	else
	{
		Out->connected = (t.MustGetInt() == 1);
		t.Next();
	}
	if(wersja < 17)
	{
		t.MustGetFloat();
		t.Next();
	}

	// Macierze
	if(wersja < 21)
	{
		// old bonespace matrix
		Matrix m;
		ParseMatrix3x3(&m, t, wersja < 19);
	}
	ParseMatrix4x4(&Out->matrix, t, wersja < 19);

	t.AssertSymbol('}');
	t.Next();
}

void QmshTmpLoader::ParseBoneGroup(tmp::BONE_GROUP* Out, Tokenizer& t)
{
	t.AssertKeyword(T_GROUP);
	t.Next();

	Out->Name = t.MustGetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	t.AssertKeyword(T_PARENT);
	t.Next();

	Out->Parent = t.MustGetString();
	t.Next();

	t.AssertSymbol('}');
	t.Next();
}

void QmshTmpLoader::ParseInterpolationMethod(tmp::INTERPOLATION_METHOD *Out, Tokenizer &t)
{
	const string& id = t.MustGetItem();
	if(id == "CONST")
		*Out = tmp::INTERPOLATION_CONST;
	else if(id == "LINEAR")
		*Out = tmp::INTERPOLATION_LINEAR;
	else if(id == "BEZIER")
		*Out = tmp::INTERPOLATION_BEZIER;
	else
		t.Unexpected();
	t.Next();
}

// used in old version (pre 18)
void QmshTmpLoader::ParseExtendMethod(Tokenizer &t)
{
	const string& id = t.MustGetItem();
	if(id != "CONST" && id != "EXTRAP" && id != "CYCLIC" && id != "CYCLIC_EXTRAP")
		t.Unexpected();
	t.Next();
}

void QmshTmpLoader::ParseCurve(tmp::CURVE *Out, Tokenizer &t)
{
	Out->Name = t.MustGetString();
	t.Next();

	ParseInterpolationMethod(&Out->Interpolation, t);
	if(load_version < 18)
		ParseExtendMethod(t);

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Vec2 pt;
		ParseVec2(&pt, t);
		Out->Points.push_back(pt);
	}
	t.Next();
}

void QmshTmpLoader::ParseChannel(tmp::CHANNEL *Out, Tokenizer &t)
{
	Out->Name = t.MustGetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		shared_ptr<tmp::CURVE> curve(new tmp::CURVE);
		ParseCurve(curve.get(), t);
		Out->Curves.push_back(curve);
	}
	t.Next();
}

void QmshTmpLoader::ParseAction(tmp::ACTION *Out, Tokenizer &t)
{
	Out->Name = t.MustGetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		shared_ptr<tmp::CHANNEL> channel(new tmp::CHANNEL);
		ParseChannel(channel.get(), t);
		Out->Channels.push_back(channel);
	}
	t.Next();
}
