#include "PCH.hpp"
#include "Mesh.h"

//-----------------------------------------------------------------------------
enum VertexDeclarationId
{
	VDI_DEFAULT, // Pos Tex Normal
	VDI_ANIMATED, // Pos Weights Indices Tex Normal
	VDI_TANGENT, // Pos Tex Normal Tangent Binormal
	VDI_ANIMATED_TANGENT, // Pos Weights Indices Tex Normal Tangent Binormal
	VDI_TEX, // Pos Tex
	VDI_COLOR, // Pos Color
	VDI_PARTICLE, // Pos Tex Color
	VDI_TERRAIN, // Pos Normal Tex Tex2
	VDI_POS, // Pos
	VDI_GRASS, // Pos Tex Normal Matrix
	VDI_MAX
};

const Vec3 DefaultSpecularColor(1, 1, 1);
const float DefaultSpecularIntensity = 0.2f;
const int DefaultSpecularHardness = 10;

inline float GetYaw(Matrix& m)
{
	if(m._21 > 0.998f || m._21 < -0.998f)
		return atan2(m._13, m._33);
	else
		return atan2(-m._31, m._11);
}


void Mesh::LoadSafe(cstring path)
{
	try
	{
		Load(path);
	}
	catch(cstring er)
	{
		throw Format("Reading error: %s", er);
	}
}

void ReadMatrix33(FileReader& f, Matrix& m)
{
	f.Read(m._11);
	f.Read(m._12);
	f.Read(m._13);
	m._14 = 0;
	f.Read(m._21);
	f.Read(m._22);
	f.Read(m._23);
	m._24 = 0;
	f.Read(m._31);
	f.Read(m._32);
	f.Read(m._33);
	m._34 = 0;
	f.Read(m._41);
	f.Read(m._42);
	f.Read(m._43);
	m._44 = 1;
}

void Mesh::Load(cstring path)
{
	FileReader f(path);

	// head
	uint size = sizeof(Header) - sizeof(uint);
	f.Read(&head, size);
	if(!f)
		throw "Failed to read file header.";
	if(memcmp(head.format, "QMSH", 4) != 0)
		throw Format("Invalid file signature '%.4s'.", head.format);
	if(head.version < 12 || head.version > 21)
		throw Format("Invalid file version '%d'.", head.version);
	if(head.n_bones >= 32)
		throw Format("Too many bones (%d).", head.n_bones);
	if(head.n_subs == 0)
		throw "Missing model mesh!";
	if(IsSet(head.flags, F_ANIMATED) && !IsSet(head.flags, F_STATIC))
	{
		if(head.n_bones == 0)
			throw "No bones.";
		if(head.version >= 13 && head.n_groups == 0)
			throw "No bone groups.";
	}
	if(head.version >= 20)
		head.points_offset = f.Read<uint>();
	else
		head.points_offset = 0;
	if(!IsSet(head.flags, F_ANIMATED))
		head.n_groups = 0; // fix for old models

	// camera
	if(head.version >= 13)
	{
		f.Read(cam_pos);
		f.Read(cam_target);
		if(head.version >= 15)
			f.Read(cam_up);
		else
			cam_up = Vec3(0, 1, 0);
		//if(!stream)
		//	throw "Missing camera data.";
	}
	else
	{
		cam_pos = Vec3(1, 1, 1);
		cam_target = Vec3(0, 0, 0);
		cam_up = Vec3(0, 1, 0);
	}

	// vertex size
	if(IsSet(head.flags, F_PHYSICS))
		vertex_size = sizeof(Vec3);
	else
	{
		if(IsSet(head.flags, F_ANIMATED))
		{
			if(IsSet(head.flags, F_TANGENTS))
				vertex_size = sizeof(VAnimatedTangent);
			else
				vertex_size = sizeof(VAnimated);
		}
		else
		{
			if(IsSet(head.flags, F_TANGENTS))
				vertex_size = sizeof(VTangent);
			else
				vertex_size = sizeof(VDefault);
		}
	}

	// ------ vertices
	// ensure size
	vdata_size = vertex_size * head.n_verts;
	//if(!f.Ensure(vdata_size))
	//	throw "Failed to read vertex buffer.";

	// create vertex buffer
	vdata = new byte[vdata_size];

	// read
	f.Read(vdata, vdata_size);

	// ----- triangles
	// ensure size
	fdata_size = sizeof(word) * head.n_tris * 3;
	//if(!f.Ensure(size))
	//	throw "Failed to read index buffer.";

	// create index buffer
	fdata = new byte[fdata_size];

	// read
	f.Read(fdata, fdata_size);

	// ----- submeshes
	//size = Submesh::MIN_SIZE * head.n_subs;
	//if(!f.Ensure(size))
	//	throw "Failed to read submesh data.";
	subs.resize(head.n_subs);

	for(word i = 0; i < head.n_subs; ++i)
	{
		Submesh& sub = subs[i];

		f.Read(sub.first);
		f.Read(sub.tris);
		f.Read(sub.min_ind);
		f.Read(sub.n_ind);
		f.ReadString1(sub.name);
		f.ReadString1(sub.tex);

		if(head.version >= 16)
		{
			// specular value
			if(head.version >= 18)
			{
				f.Read(sub.specular_color);
				f.Read(sub.specular_intensity);
				f.Read(sub.specular_hardness);
			}
			else
			{
				sub.specular_color = DefaultSpecularColor;
				sub.specular_intensity = DefaultSpecularIntensity;
				sub.specular_hardness = DefaultSpecularHardness;
			}

			// normalmap
			if(IsSet(head.flags, F_TANGENTS))
			{
				f.ReadString1(sub.tex_normal);
				if(!sub.tex_normal.empty())
					f.Read(sub.normal_factor);
			}
			else
				sub.tex_normal = "";

			// specular map
			f.ReadString1(sub.tex_specular);
			if(!sub.tex_specular.empty())
			{
				f.Read(sub.specular_factor);
				f.Read(sub.specular_color_factor);
			}
		}
		else
		{
			sub.tex_specular = "";
			sub.tex_normal = "";
			sub.specular_color = DefaultSpecularColor;
			sub.specular_intensity = DefaultSpecularIntensity;
			sub.specular_hardness = DefaultSpecularHardness;
		}

		//if(!stream)
		//	throw format("Failed to read submesh %u.", i);
	}

	// animation data
	if(IsSet(head.flags, F_ANIMATED) && !IsSet(head.flags, F_STATIC))
	{
		// bones
		size = Bone::MIN_SIZE * head.n_bones;
		//if(!f.Ensure(size))
		//	throw "Failed to read bones.";
		bones.resize(head.n_bones);


		for(byte i = 0; i < head.n_bones; ++i)
		{
			Bone& bone = bones[i];
			if(head.version >= 21)
			{
				f.ReadString1(bone.name);
				f.Read(bone.parent);
				ReadMatrix33(f, bone.mat);
				f.Read(bone.raw_mat);
				f.Read(bone.head);
				f.Read(bone.tail);
				f.Read(bone.connected);
			}
			else
			{
				f.Read(bone.parent);
				ReadMatrix33(f, bone.mat);
				bone.raw_mat = bone.mat;
				bone.head = Vec4(1, 1, 0, 0.1f);
				bone.tail = Vec4(1, 1, 1, 0.1f);
				bone.connected = false;
				f.ReadString1(bone.name);
			}
		}

		//if(!stream)
		//	throw "Failed to read bones data.";

		// bone groups
		if(head.version >= 21)
			LoadBoneGroups(f);

		// animations
		//size = Animation::MIN_SIZE * head.n_anims;
		//if(!f.Ensure(size))
		//	throw "Failed to read animations.";
		anims.resize(head.n_anims);

		for(byte i = 0; i < head.n_anims; ++i)
		{
			Animation& anim = anims[i];

			f.ReadString1(anim.name);
			f.Read(anim.length);
			f.Read(anim.n_frames);

			//size = anim.n_frames * (4 + sizeof(KeyframeBone) * head.n_bones);
			//if(!f.Ensure(size))
			//	throw format("Failed to read animation %u data.", i);

			anim.frames.resize(anim.n_frames);

			for(word j = 0; j < anim.n_frames; ++j)
			{
				f.Read(anim.frames[j].time);
				anim.frames[j].bones.resize(head.n_bones);
				f.Read(anim.frames[j].bones.data(), sizeof(KeyframeBone) * head.n_bones);
			}
		}

		// add zero bone to count
		//++head.n_bones;
	}

	// points
	//uint size = Point::MIN_SIZE * head.n_points;
	//if(!f.Ensure(size))
	//	throw "Failed to read points.";
	attach_points.resize(head.n_points);
	for(word i = 0; i < head.n_points; ++i)
	{
		Point& p = attach_points[i];

		f.ReadString1(p.name);
		f.Read(p.mat);
		f.Read(p.bone);
		if(head.version == 12)
			++p.bone; // in that version there was different counting
		f.Read(p.type);
		if(head.version >= 14)
			f.Read(p.size);
		else
		{
			f.Read(p.size.x);
			p.size.y = p.size.z = p.size.x;
		}
		if(head.version >= 19)
		{
			f.Read(p.rot);
			if(head.version < 21)
				p.rot.y = Clip(-p.rot.y);
		}
		else
		{
			// fallback, it was often wrong but thats the way it was (works good for PI/2 and PI*3/2, inverted for 0 and PI, bad for other)
			p.rot = Vec3(0, -GetYaw(p.mat), 0);
		}
	}

	if(head.version < 21 && IsSet(head.flags, F_ANIMATED) && !IsSet(head.flags, F_STATIC))
		LoadBoneGroups(f);

	// splits
	if(IsSet(head.flags, F_SPLIT))
	{
		size = sizeof(Split) * head.n_subs;
		//if(!f.Ensure(size))
		//	throw "Failed to read mesh splits.";
		splits.resize(head.n_subs);
		f.Read(splits.data(), size);
	}

	old_ver = head.version;
	head.version = 21;
}

void Mesh::LoadBoneGroups(FileReader& f)
{
	// groups
	if(head.version == 12 && head.n_groups < 2)
	{
		head.n_groups = 1;
		groups.resize(1);

		BoneGroup& gr = groups[0];
		gr.name = "default";
		gr.parent = 0;
		gr.bones.reserve(head.n_bones - 1);

		for(word i = 1; i < head.n_bones; ++i)
			gr.bones.push_back((byte)i);
	}
	else
	{
		//if(!f.Ensure(BoneGroup::MIN_SIZE * head.n_groups))
		//	throw "Failed to read bone groups.";
		groups.resize(head.n_groups);
		for(word i = 0; i < head.n_groups; ++i)
		{
			BoneGroup& gr = groups[i];

			f.ReadString1(gr.name);

			// parent group
			f.Read(gr.parent);
			assert(gr.parent < head.n_groups);
			assert(gr.parent != i || i == 0);

			// bone indexes
			byte count;
			f.Read(count);
			gr.bones.resize(count);
			f.Read(gr.bones.data(), gr.bones.size());
			if(head.version == 12)
			{
				for(byte& b : gr.bones)
					++b;
			}
		}
	}
}

void Mesh::Save(cstring path)
{
	FileWriter f(path);

	// head
	f.Write(head);
	int offset_points_pos = f.GetPos() - 4;

	// camera
	f.Write(cam_pos);
	f.Write(cam_target);
	f.Write(cam_up);

	// vertices
	f.Write(vdata, vdata_size);

	// triangles
	f.Write(fdata, fdata_size);

	// ----- submeshes
	for(word i = 0; i < head.n_subs; ++i)
	{
		Submesh& sub = subs[i];

		f.Write(sub.first);
		f.Write(sub.tris);
		f.Write(sub.min_ind);
		f.Write(sub.n_ind);
		f.WriteString1(sub.name);
		f.WriteString1(sub.tex);

		// specular value
		f.Write(sub.specular_color);
		f.Write(sub.specular_intensity);
		f.Write(sub.specular_hardness);

		// normalmap
		if(IsSet(head.flags, F_TANGENTS))
		{
			f.WriteString1(sub.tex_normal);
			if(!sub.tex_normal.empty())
				f.Write(sub.normal_factor);
		}

		// specular map
		f.WriteString1(sub.tex_specular);
		if(!sub.tex_specular.empty())
		{
			f.Write(sub.specular_factor);
			f.Write(sub.specular_color_factor);
		}
	}

	// animation data
	if(IsSet(head.flags, F_ANIMATED) && !IsSet(head.flags, F_STATIC))
	{
		// bones
		for(byte i = 0; i < head.n_bones; ++i)
		{
			Bone& bone = bones[i];
			f.WriteString1(bone.name);
			f.Write(bone.parent);
			f.Write(bone.mat._11);
			f.Write(bone.mat._12);
			f.Write(bone.mat._13);
			f.Write(bone.mat._21);
			f.Write(bone.mat._22);
			f.Write(bone.mat._23);
			f.Write(bone.mat._31);
			f.Write(bone.mat._32);
			f.Write(bone.mat._33);
			f.Write(bone.mat._41);
			f.Write(bone.mat._42);
			f.Write(bone.mat._43);
			f.Write(bone.raw_mat);
			f.Write(bone.head);
			f.Write(bone.tail);
			f.Write(bone.connected);
		}

		// bone groups
		for(word i = 0; i < head.n_groups; ++i)
		{
			BoneGroup& gr = groups[i];

			f.WriteString1(gr.name);

			// parent group
			f.Write(gr.parent);

			// bone indexes
			byte count = (byte)gr.bones.size();
			f.Write(count);
			f.Write(gr.bones.data(), gr.bones.size());
		}

		// animations
		for(byte i = 0; i < head.n_anims; ++i)
		{
			Animation& anim = anims[i];

			f.WriteString1(anim.name);
			f.Write(anim.length);
			f.Write(anim.n_frames);

			for(word j = 0; j < anim.n_frames; ++j)
			{
				f.Write(anim.frames[j].time);
				f.Write(anim.frames[j].bones.data(), sizeof(KeyframeBone) * head.n_bones);
			}
		}
	}

	// points
	int points_pos = f.GetPos();
	f.SetPos(offset_points_pos);
	f.Write(points_pos);
	f.SetPos(points_pos);
	for(word i = 0; i < head.n_points; ++i)
	{
		Point& p = attach_points[i];

		f.WriteString1(p.name);
		f.Write(p.mat);
		f.Write(p.bone);
		f.Write(p.type);
		f.Write(p.size);
		f.Write(p.rot);
	}

	// splits
	if(IsSet(head.flags, F_SPLIT))
	{
		f.Write(splits.data(), sizeof(Split) * head.n_subs);
	}
}

Mesh::Point* Mesh::GetPoint(const string& id)
{
	for(Point& point : attach_points)
	{
		if(point.name == id)
			return &point;
	}
	return nullptr;
}
