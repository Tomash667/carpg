#pragma once

struct QMSH_VERTEX
{
	Vec3 Pos;
	Vec3 Normal;
	Vec2 Tex;
	Vec3 Tangent;      // Aktualny tylko jeœli Flags & FLAG_TANGENTS
	Vec3 Binormal;     // Aktualny tylko jeœli Flags & FLAG_TANGENTS
	float Weight1;     // Aktualny tylko jeœli Flags & FLAG_SKINNING
	uint BoneIndices; // Aktualny tylko jeœli Flags & FLAG_SKINNING
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
	// Wyznaczaj¹ zakres. Ten zakres, wy³¹cznie ten i w ca³oœci ten powinien byæ przeznaczony na ten obiekt (Submesh).
	uint MinVertexIndexUsed;   // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra¿ony w trójk¹tach)
	uint NumVertexIndicesUsed; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra¿ony w trójk¹tach)
	Vec3 specular_color, center;
	int specular_hardness;
	float specular_intensity, specular_factor, specular_color_factor, normal_factor, range;
	Box box;
};

struct QMSH_BONE
{
	string Name;
	// Koœci indeksowane s¹ od 1, 0 jest zarezerwowane dla braku koœci (czyli ¿e nie ma parenta)
	uint ParentIndex;
	// Macierz przeszta³caj¹ca ze wsp. danej koœci do wsp. koœci nadrzêdnej w Bind Pose, ³¹cznie z translacj¹
	Matrix matrix;
	Matrix RawMatrix;
	Vec4 head, tail;

	// Indeksy podkoœci, indeksowane równie¿ od 1
	// Tylko runtime, tego nie ma w pliku
	std::vector<uint> Children;

	// indeks grupy
	int group_index;

	// czy koœæ nie odkszta³ca modelu ?
	bool non_mesh;
	bool connected;
};

struct QMSH_KEYFRAME_BONE
{
	Vec3 Translation;
	Quat Rotation;
	float Scaling;
};

struct QMSH_KEYFRAME
{
	float Time; // W sekundach
	std::vector<QMSH_KEYFRAME_BONE> Bones; // Liczba musi byæ równa liczbie koœci
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
	Matrix matrix;
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
	std::vector<word> Indices;
	std::vector<shared_ptr<QMSH_SUBMESH>> Submeshes;
	std::vector<shared_ptr<QMSH_BONE>> Bones; // Tylko kiedy Flags & SKINNING
	std::vector<shared_ptr<QMSH_ANIMATION>> Animations; // Tylko kiedy Flags & SKINNING
	std::vector<shared_ptr<QMSH_POINT>> Points;
	std::vector<QMSH_GROUP> Groups;

	// Bry³y otaczaj¹ce
	// Dotycz¹ wierzcho³ków w pozycji spoczynkowej.
	float BoundingSphereRadius;
	Box BoundingBox;
	Vec3 camera_pos, camera_target, camera_up;

	static const uint VERSION = 21u;
};
