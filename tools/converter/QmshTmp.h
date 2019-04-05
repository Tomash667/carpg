#pragma once

namespace tmp
{
	struct VERTEX
	{
		Vec3 Pos;
		Vec3 Normal;
	};

	struct FACE
	{
		uint MaterialIndex;
		bool Smooth;
		uint NumVertices; // 3 lub 4
		uint VertexIndices[4];
		Vec2 TexCoords[4];
		Vec3 Normal;
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
		Vec3 diffuse_color, specular_color;
		std::vector< shared_ptr<Texture> > textures;
	};

	struct MESH
	{
		string Name;
		string ParentArmature; // £añcuch pusty jeœli nie ma
		string ParentBone; // £añcuch pusty jeœli nie ma
		Vec3 Position;
		Vec3 Orientation;
		Vec3 Size;
		std::vector< shared_ptr<Material> > Materials;
		std::vector<VERTEX> Vertices;
		std::vector<FACE> Faces;
		std::vector< shared_ptr<VERTEX_GROUP> > VertexGroups;
		bool vertex_normals;
	};

	struct BONE_GROUP
	{
		string Name;
		string Parent;
	};

	struct BONE
	{
		string Name;
		string Parent; // £añcuch pusty jeœli nie ma parenta
		string Group;
		Vec3 Head;
		float HeadRadius;
		Vec3 Tail;
		float TailRadius;
		Matrix matrix;
		bool connected;
	};

	struct ARMATURE
	{
		string Name;
		Vec3 Position;
		Vec3 Orientation;
		Vec3 Size;
		std::vector<shared_ptr<BONE_GROUP>> Groups;
		std::vector<shared_ptr<BONE>> Bones;

		uint FindBoneGroupIndex(const string& name)
		{
			for(uint i = 0; i < Groups.size(); ++i)
			{
				if(Groups[i]->Name == name)
					return i;
			}
			return (uint)-1;
		}
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
		std::vector<Vec2> Points;
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
		Vec3 size, rot;
		Matrix matrix;
	};

	struct QMSH
	{
		std::vector<shared_ptr<MESH>> Meshes; // Dowolna iloœæ
		shared_ptr<ARMATURE> Armature; // Nie ma albo jest jeden
		std::vector<shared_ptr<ACTION>> Actions; // Dowolna iloœæ
		std::vector<shared_ptr<POINT>> points;
		int FPS;
		bool static_anim, force_tangents, split;
		Vec3 camera_pos, camera_target, camera_up;

		QMSH() : FPS(25), camera_pos(1, 1, 1), camera_target(0, 0, 0), camera_up(0, 0, 1), static_anim(false), force_tangents(false), split(false)
		{
		}
	};
}
