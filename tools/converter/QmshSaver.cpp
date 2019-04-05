#include "PCH.hpp"
#include "QmshSaver.h"

// Zapisuje QMSH do pliku
void QmshSaver::SaveQmsh(const QMSH &Qmsh, const string &FileName)
{
	Info("Saving QMSH file \"%s\"...", FileName.c_str());

	FileWriter f(FileName);

	assert(Qmsh.Flags < 0xFF);
	assert(Qmsh.Indices.size() % 3 == 0);

	// Nag³ówek
	f.WriteStringF("QMSH");
	f.Write((byte)QMSH::VERSION);
	f.Write((byte)Qmsh.Flags);
	f.Write((word)Qmsh.Vertices.size());
	f.Write((word)(Qmsh.Indices.size() / 3));
	f.Write((word)Qmsh.Submeshes.size());
	word n_bones = (word)Qmsh.Bones.size();
	if((Qmsh.Flags & FLAG_STATIC) != 0)
		n_bones = 0;
	f.Write(n_bones);
	f.Write((word)Qmsh.Animations.size());
	f.Write((word)Qmsh.Points.size());
	f.Write((word)Qmsh.Groups.size());

	// Bry³y otaczaj¹ce
	f.Write(Qmsh.BoundingSphereRadius);
	f.Write(Qmsh.BoundingBox);

	// points offset
	int offset_pos = f.GetPos();
	f.Write(0xDEADC0DE);

	// camera
	f.Write(Qmsh.camera_pos);
	f.Write(Qmsh.camera_target);
	f.Write(Qmsh.camera_up);

	// Wierzcho³ki
	for(uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		const QMSH_VERTEX & v = Qmsh.Vertices[vi];

		f.Write(v.Pos);

		if((Qmsh.Flags & FLAG_PHYSICS) == 0)
		{
			if((Qmsh.Flags & FLAG_SKINNING) != 0)
			{
				f.Write(v.Weight1);
				f.Write(v.BoneIndices);
			}

			f.Write(v.Normal);
			f.Write(v.Tex);

			if((Qmsh.Flags & FLAG_TANGENTS) != 0)
			{
				f.Write(v.Tangent);
				f.Write(v.Binormal);
			}
		}
	}

	// Indeksy (trójk¹ty)
	f.Write(&Qmsh.Indices[0], sizeof(word)*Qmsh.Indices.size());

	// Podsiatki
	for(uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		const QMSH_SUBMESH & s = *Qmsh.Submeshes[si].get();

		f.Write((word)s.FirstTriangle);
		f.Write((word)s.NumTriangles);
		f.Write((word)s.MinVertexIndexUsed);
		f.Write((word)s.NumVertexIndicesUsed);
		f.WriteString1(s.Name);
		f.WriteString1(s.texture);
		f.Write(s.specular_color);
		f.Write(s.specular_intensity);
		f.Write(s.specular_hardness);
		if((Qmsh.Flags & FLAG_TANGENTS) != 0)
		{
			f.WriteString1(s.normalmap_texture);
			if(!s.normalmap_texture.empty())
				f.Write(s.normal_factor);
		}
		f.WriteString1(s.specularmap_texture);
		if(!s.specularmap_texture.empty())
		{
			f.Write(s.specular_factor);
			f.Write(s.specular_color_factor);
		}
	}

	if((Qmsh.Flags & FLAG_SKINNING) != 0 && (Qmsh.Flags & FLAG_STATIC) == 0)
	{
		// bones
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			QMSH_BONE& bone = *Qmsh.Bones[bi].get();
			f.WriteString1(bone.Name);
			f.Write((word)bone.ParentIndex);
			f.Write(bone.matrix._11);
			f.Write(bone.matrix._12);
			f.Write(bone.matrix._13);
			f.Write(bone.matrix._21);
			f.Write(bone.matrix._22);
			f.Write(bone.matrix._23);
			f.Write(bone.matrix._31);
			f.Write(bone.matrix._32);
			f.Write(bone.matrix._33);
			f.Write(bone.matrix._41);
			f.Write(bone.matrix._42);
			f.Write(bone.matrix._43);
			f.Write(bone.RawMatrix);
			f.Write(bone.head);
			f.Write(bone.tail);
			f.Write(bone.connected);
		}

		// bone groups
		for(uint i = 0; i < Qmsh.Groups.size(); ++i)
		{
			const QMSH_GROUP& group = Qmsh.Groups[i];

			f.WriteString1(group.name);
			byte size = (byte)group.bones.size();
			f.Write(group.parent);
			f.Write(size);

			for(byte j = 0; j < size; ++j)
			{
				for(byte k = 0; k <= Qmsh.Bones.size(); ++k)
				{
					if(&*Qmsh.Bones[k] == group.bones[j])
					{
						f.Write(byte(k + 1));
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

			f.WriteString1(Animation.Name);
			f.Write(Animation.Length);
			f.Write((word)Animation.Keyframes.size());

			for(uint ki = 0; ki < Animation.Keyframes.size(); ki++)
			{
				const QMSH_KEYFRAME & Keyframe = *Animation.Keyframes[ki].get();

				f.Write(Keyframe.Time);

				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					f.Write(Keyframe.Bones[bi].Translation);
					f.Write(Keyframe.Bones[bi].Rotation);
					f.Write(Keyframe.Bones[bi].Scaling);
				}
			}
		}
	}

	// punkty
	int pos = f.GetPos();
	f.SetPos(offset_pos);
	f.Write(pos);
	f.SetPos(pos);
	for(uint i = 0; i < Qmsh.Points.size(); ++i)
	{
		const QMSH_POINT& point = *Qmsh.Points[i].get();

		f.WriteString1(point.name);
		f.Write(point.matrix);
		f.Write(unsigned short(point.bone + 1));
		f.Write(point.type);
		f.Write(point.size);
		f.Write(point.rot);
	}

	// split
	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::const_iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
		{
			const QMSH_SUBMESH& sub = **it;
			f.Write(sub.center);
			f.Write(sub.range);
			f.Write(sub.box);
		}
	}
}
