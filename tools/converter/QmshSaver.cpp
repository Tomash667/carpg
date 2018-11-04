#include "PCH.hpp"
#include "QmshSaver.h"

// Zapisuje QMSH do pliku
void QmshSaver::SaveQmsh(const QMSH &Qmsh, const string &FileName)
{
	ERR_TRY;

	Writeln("Saving QMSH file \"" + FileName + "\"...");

	FileStream F(FileName, FM_WRITE);

	assert(Qmsh.Flags < 0xFF);
	assert(Qmsh.Indices.size() % 3 == 0);

	// Nag��wek
	F.WriteStringF("QMSH");
	F.WriteEx((uint1)QMSH::VERSION);
	F.WriteEx((uint1)Qmsh.Flags);
	F.WriteEx((uint2)Qmsh.Vertices.size());
	F.WriteEx((uint2)(Qmsh.Indices.size() / 3));
	F.WriteEx((uint2)Qmsh.Submeshes.size());
	uint2 n_bones = Qmsh.Bones.size();
	if((Qmsh.Flags & FLAG_STATIC) != 0)
		n_bones = 0;
	F.WriteEx(n_bones);
	F.WriteEx((uint2)Qmsh.Animations.size());
	F.WriteEx((uint2)Qmsh.Points.size());
	F.WriteEx((uint2)Qmsh.Groups.size());

	// Bry�y otaczaj�ce
	F.WriteEx(Qmsh.BoundingSphereRadius);
	F.WriteEx(Qmsh.BoundingBox);

	// points offset
	int offset_pos = F.GetPos();
	F.WriteEx(0xDEADC0DE);

	// camera
	F.WriteEx(Qmsh.camera_pos);
	F.WriteEx(Qmsh.camera_target);
	F.WriteEx(Qmsh.camera_up);

	// Wierzcho�ki
	for(uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		const QMSH_VERTEX & v = Qmsh.Vertices[vi];

		F.WriteEx(v.Pos);

		if((Qmsh.Flags & FLAG_PHYSICS) == 0)
		{
			if((Qmsh.Flags & FLAG_SKINNING) != 0)
			{
				F.WriteEx(v.Weight1);
				F.WriteEx(v.BoneIndices);
			}

			F.WriteEx(v.Normal);
			F.WriteEx(v.Tex);

			if((Qmsh.Flags & FLAG_TANGENTS) != 0)
			{
				F.WriteEx(v.Tangent);
				F.WriteEx(v.Binormal);
			}
		}
	}

	// Indeksy (tr�jk�ty)
	F.Write(&Qmsh.Indices[0], sizeof(uint2)*Qmsh.Indices.size());

	// Podsiatki
	for(uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		const QMSH_SUBMESH & s = *Qmsh.Submeshes[si].get();

		F.WriteEx((uint2)s.FirstTriangle);
		F.WriteEx((uint2)s.NumTriangles);
		F.WriteEx((uint2)s.MinVertexIndexUsed);
		F.WriteEx((uint2)s.NumVertexIndicesUsed);
		F.WriteString1(s.Name);
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

	if((Qmsh.Flags & FLAG_SKINNING) != 0 && (Qmsh.Flags & FLAG_STATIC) == 0)
	{
		// bones
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			QMSH_BONE& bone = *Qmsh.Bones[bi].get();
			F.WriteString1(bone.Name);
			F.WriteEx((uint2)bone.ParentIndex);
			F.WriteEx(bone.Matrix._11);
			F.WriteEx(bone.Matrix._12);
			F.WriteEx(bone.Matrix._13);
			F.WriteEx(bone.Matrix._21);
			F.WriteEx(bone.Matrix._22);
			F.WriteEx(bone.Matrix._23);
			F.WriteEx(bone.Matrix._31);
			F.WriteEx(bone.Matrix._32);
			F.WriteEx(bone.Matrix._33);
			F.WriteEx(bone.Matrix._41);
			F.WriteEx(bone.Matrix._42);
			F.WriteEx(bone.Matrix._43);
			F.Write(bone.RawMatrix);
			F.Write(bone.head);
			F.Write(bone.tail);
			F.Write(bone.connected);
		}

		// bone groups
		for(uint i = 0; i < Qmsh.Groups.size(); ++i)
		{
			const QMSH_GROUP& group = Qmsh.Groups[i];

			F.WriteString1(group.name);
			byte size = group.bones.size();
			F.WriteEx(group.parent);
			F.WriteEx(size);

			for(byte j = 0; j < size; ++j)
			{
				for(byte k = 0; k <= Qmsh.Bones.size(); ++k)
				{
					if(&*Qmsh.Bones[k] == group.bones[j])
					{
						F.WriteEx(byte(k + 1));
						break;
					}
				}
			}
		}
	}

	// Animacje
	if((Qmsh.Flags & FLAG_SKINNING) != 0)
	{
		for(uint ai = 0; ai < Qmsh.Animations.size(); ai++)
		{
			const QMSH_ANIMATION & Animation = *Qmsh.Animations[ai].get();

			F.WriteString1(Animation.Name);
			F.WriteEx(Animation.Length);
			F.WriteEx((uint2)Animation.Keyframes.size());

			for(uint ki = 0; ki < Animation.Keyframes.size(); ki++)
			{
				const QMSH_KEYFRAME & Keyframe = *Animation.Keyframes[ki].get();

				F.WriteEx(Keyframe.Time);

				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					F.WriteEx(Keyframe.Bones[bi].Translation);
					F.WriteEx(Keyframe.Bones[bi].Rotation);
					F.WriteEx(Keyframe.Bones[bi].Scaling);
				}
			}
		}
	}

	// punkty
	int pos = F.GetPos();
	F.SetPos(offset_pos);
	F.WriteEx(pos);
	F.SetPos(pos);
	for(uint i = 0; i < Qmsh.Points.size(); ++i)
	{
		const QMSH_POINT& point = *Qmsh.Points[i].get();

		F.WriteString1(point.name);
		F.WriteEx(point.matrix);
		F.WriteEx(unsigned short(point.bone + 1));
		F.WriteEx(point.type);
		F.WriteEx(point.size);
		F.WriteEx(point.rot);
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
