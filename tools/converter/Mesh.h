#pragma once

struct VDefault
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
};

struct VAnimated
{
	Vec3 pos;
	float weights;
	uint indices;
	Vec3 normal;
	Vec2 tex;
};

struct VTangent
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
	Vec3 tangent;
	Vec3 binormal;
};

struct VAnimatedTangent
{
	Vec3 pos;
	float weights;
	uint indices;
	Vec3 normal;
	Vec2 tex;
	Vec3 tangent;
	Vec3 binormal;
};

struct VTex
{
	Vec3 pos;
	Vec2 tex;
};

struct VColor
{
	Vec3 pos;
	Vec4 color;
};

struct VParticle
{
	Vec3 pos;
	Vec2 tex;
	Vec4 color;
};

struct VTerrain
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
	Vec2 tex2;
};

struct VPos
{
	Vec3 pos;
};

struct Mesh
{
	enum MESH_FLAGS
	{
		F_TANGENTS = 1 << 0,
		F_ANIMATED = 1 << 1,
		F_STATIC = 1 << 2,
		F_PHYSICS = 1 << 3,
		F_SPLIT = 1 << 4
	};

	struct Header
	{
		char format[4];
		byte version, flags;
		word n_verts, n_tris, n_subs, n_bones, n_anims, n_points, n_groups;
		float radius;
		Box bbox;
		uint points_offset;
	};

	struct Submesh
	{
		word first; // pierwszy strójk¹t do narysowania
		word tris; // ile trójk¹tów narysowaæ
		word min_ind; // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra¿ony w trójk¹tach)
		word n_ind; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra¿ony w trójk¹tach)
		string name;//, normal_name, specular_name;
		string tex, tex_normal, tex_specular;
		Vec3 specular_color;
		float specular_intensity;
		int specular_hardness;
		float normal_factor, specular_factor, specular_color_factor;

		bool operator == (const Submesh& sub) const
		{
			return first == sub.first
				&& tris == sub.tris
				&& min_ind == sub.min_ind
				&& n_ind == sub.n_ind
				&& name == sub.name
				&& tex == sub.tex
				&& tex_normal == sub.tex_normal
				&& tex_specular == sub.tex_specular
				&& specular_color == sub.specular_color
				&& specular_intensity == sub.specular_intensity
				&& specular_hardness == sub.specular_hardness
				&& normal_factor == sub.normal_factor
				&& specular_factor == sub.specular_factor
				&& specular_color_factor == sub.specular_color_factor;
		}

		static const uint MIN_SIZE = 10;
	};

	struct BoneGroup
	{
		word parent;
		string name;
		vector<byte> bones;

		bool operator == (const BoneGroup& group) const
		{
			return parent == group.parent
				&& name == group.name
				&& bones == group.bones;
		}

		static const uint MIN_SIZE = 4;
	};

	struct Bone
	{
		string name;
		word id;
		word parent;
		Matrix mat, raw_mat;
		Vec4 head, tail;
		vector<word> childs;
		bool connected;

		bool operator == (const Bone& bone) const
		{
			return name == bone.name
				&& id == bone.id
				&& parent == bone.parent
				&& mat == bone.mat
				&& raw_mat == bone.raw_mat
				&& head == bone.head
				&& tail == bone.tail
				&& childs == bone.childs
				&& connected == bone.connected;
		}

		static const uint MIN_SIZE = 51;
	};

	struct KeyframeBone
	{
		Vec3 pos;
		Quat rot;
		float scale;

		bool operator == (const KeyframeBone& keyframe_bone) const
		{
			return pos == keyframe_bone.pos
				&& rot == keyframe_bone.rot
				&& scale == keyframe_bone.scale;
		}
	};

	struct Keyframe
	{
		float time;
		vector<KeyframeBone> bones;

		bool operator == (const Keyframe& keyframe) const
		{
			return time == keyframe.time
				&& bones == keyframe.bones;
		}
	};

	struct Animation
	{
		string name;
		float length;
		word n_frames;
		vector<Keyframe> frames;

		bool operator == (const Animation& ani) const
		{
			return name == ani.name
				&& length == ani.length
				&& n_frames == ani.n_frames
				&& frames == ani.frames;
		}

		static const uint MIN_SIZE = 7;
	};

	struct Point
	{
		enum Type : word
		{
			PLAIN_AXES,
			SPHERE,
			BOX,
			ARROWS,
			SINGLE_ARROW,
			CIRCLE,
			CONE,
			MAX
		};

		string name;
		Matrix mat;
		Vec3 rot;
		word bone;
		Type type;
		Vec3 size;

		static const uint MIN_SIZE = 73;

		bool IsSphere() const
		{
			return type == SPHERE;
		}
		bool IsBox() const
		{
			return type == BOX;
		}

		bool operator == (const Point& point) const
		{
			return name == point.name
				&& mat == point.mat
				&& rot == point.rot
				&& bone == point.bone
				&& type == point.type
				&& size == point.size;
		}
	};

	struct Split
	{
		Vec3 pos;
		float radius;
		Box box;
	};

	Mesh() : vdata(nullptr), fdata(nullptr)
	{
	}
	~Mesh()
	{
		delete[] vdata;
		delete[] fdata;
	}
	void LoadSafe(cstring path);
	void Load(cstring path);
	void LoadBoneGroups(FileReader& f);
	void Save(cstring path);
	Point* GetPoint(const string& id);

	Header head;
	uint vertex_size;
	vector<Submesh> subs;
	vector<Bone> bones;
	vector<Animation> anims;
	vector<Point> attach_points;
	vector<BoneGroup> groups;
	vector<Split> splits;
	Vec3 cam_pos, cam_target, cam_up;

	byte old_ver;
	byte* vdata;
	byte* fdata;
	uint vdata_size, fdata_size;
};
