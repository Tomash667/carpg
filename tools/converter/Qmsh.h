#pragma once

struct QMSH_VERTEX
{
	Vec3 Pos;
	Vec3 Normal;
	Vec2 Tex;
	Vec3 Tangent;      // Aktualny tylko je�li Flags & FLAG_TANGENTS
	Vec3 Binormal;     // Aktualny tylko je�li Flags & FLAG_TANGENTS
	float Weight1;     // Aktualny tylko je�li Flags & FLAG_SKINNING
	uint BoneIndices; // Aktualny tylko je�li Flags & FLAG_SKINNING
};

struct QMSH_SUBMESH
{
	string Name;
	string MaterialName;
	string texture;
	string normalmap_texture;
	string specularmap_texture;
	uint FirstTriangle;
	uint NumTriangles;
	// Wyznaczaj� zakres. Ten zakres, wy��cznie ten i w ca�o�ci ten powinien by� przeznaczony na ten obiekt (Submesh).
	uint MinVertexIndexUsed;   // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra�ony w tr�jk�tach)
	uint NumVertexIndicesUsed; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra�ony w tr�jk�tach)
	Vec3 specular_color, center;
	int specular_hardness;
	float specular_intensity, specular_factor, specular_color_factor, normal_factor, range;
	BOX box;
};

struct QMSH_BONE
{
	string Name;
	// Ko�ci indeksowane s� od 1, 0 jest zarezerwowane dla braku ko�ci (czyli �e nie ma parenta)
	uint ParentIndex;
	// Macierz przeszta�caj�ca ze wsp. danej ko�ci do wsp. ko�ci nadrz�dnej w Bind Pose, ��cznie z translacj�
	MATRIX Matrix;
	MATRIX RawMatrix;
	Vec4 head, tail;

	// Indeksy podko�ci, indeksowane r�wnie� od 1
	// Tylko runtime, tego nie ma w pliku
	std::vector<uint> Children;

	// indeks grupy
	int group_index;

	// czy ko�� nie odkszta�ca modelu ?
	bool non_mesh;
	bool connected;
};

struct QMSH_KEYFRAME_BONE
{
	Vec3 Translation;
	QUATERNION Rotation;
	float Scaling;
};

struct QMSH_KEYFRAME
{
	float Time; // W sekundach
	std::vector<QMSH_KEYFRAME_BONE> Bones; // Liczba musi by� r�wna liczbie ko�ci
};

struct QMSH_ANIMATION
{
	string Name;
	float Length; // W sekundach
	std::vector< shared_ptr<QMSH_KEYFRAME> > Keyframes;
};

struct QMSH_POINT
{
	string name;
	unsigned short bone, type;
	Vec3 size, rot;
	MATRIX matrix;
};

struct QMSH_GROUP
{
	string name;
	unsigned short parent;
	std::vector<QMSH_BONE*> bones;
};

enum FLAGS
{
	FLAG_TANGENTS = 0x01,
	FLAG_SKINNING = 0x02,
	FLAG_STATIC   = 0x04,
	FLAG_PHYSICS  = 0x08,
	FLAG_SPLIT    = 0x10
};

struct QMSH
{
	uint Flags;
	std::vector<QMSH_VERTEX> Vertices;
	std::vector<uint2> Indices;
	std::vector<shared_ptr<QMSH_SUBMESH>> Submeshes;
	std::vector<shared_ptr<QMSH_BONE>> Bones; // Tylko kiedy Flags & SKINNING
	std::vector<shared_ptr<QMSH_ANIMATION>> Animations; // Tylko kiedy Flags & SKINNING
	std::vector<shared_ptr<QMSH_POINT>> Points;
	std::vector<QMSH_GROUP> Groups;

	// Bry�y otaczaj�ce
	// Dotycz� wierzcho�k�w w pozycji spoczynkowej.
	float BoundingSphereRadius;
	BOX BoundingBox;
	Vec3 camera_pos, camera_target, camera_up;

	uint real_bones;

	static const uint VERSION = 21u;
};
