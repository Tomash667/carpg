#include "PCH.hpp"
#include "Converter.h"
#include "Mesh.h"

// Przechowuje dane na temat wybranej ko�ci we wsp. globalnych modelu w uk�adzie DirectX
struct BONE_INTER_DATA
{
	Vec3 HeadPos;
	Vec3 TailPos;
	float HeadRadius;
	float TailRadius;
	float Length;
	uint TmpBoneIndex; // Indeks tej ko�ci w Armature w TMP, liczony od 0 - stanowi mapowanie nowych na stare ko�ci
};

struct INTERMEDIATE_VERTEX_SKIN_DATA
{
	uint1 Index1;
	uint1 Index2;
	float Weight1;
};

cstring point_types[Mesh::Point::MAX] = {
	"PLAIN_AXES",
	"SPHERE",
	"CUBE",
	"ARROWS",
	"SINGLE_ARROW",
	"CIRCLE",
	"CONE"
};

void Converter::ConvertQmshTmpToQmsh(QMSH *Out, tmp::QMSH &QmshTmp, ConversionData& cs)
{
	// Przekszta�� uk�ad wsp�rz�dnych
	TransformQmshTmpCoords(&QmshTmp);

	// BoneInterData odpowiada ko�ciom wyj�ciowym QMSH.Bones, nie wej�ciowym z TMP.Armature!!!
	std::vector<BONE_INTER_DATA> BoneInterData;

	Out->Flags = 0;
	if(cs.export_phy)
		Out->Flags |= FLAG_PHYSICS;
	else
	{
		if(QmshTmp.static_anim)
			Out->Flags |= FLAG_STATIC;
		if(QmshTmp.Armature != NULL)
		{
			Out->Flags |= FLAG_SKINNING;

			// Przetw�rz ko�ci
			TmpToQmsh_Bones(Out, &BoneInterData, QmshTmp);

			// Przetw�rz animacje
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

	// Dla kolejnych obiekt�w TMP
	for(uint oi = 0; oi < QmshTmp.Meshes.size(); oi++)
	{
		tmp::MESH & o = *QmshTmp.Meshes[oi].get();

		// Policz wp�yw ko�ci na wierzcho�ki tej siatki
		std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> VertexSkinData;
		if((Out->Flags & FLAG_SKINNING) != 0)
			CalcVertexSkinData(&VertexSkinData, o, *Out, QmshTmp, BoneInterData);

		// Skonstruuj struktury po�rednie
		// UWAGA! Z�o�ono�� kwadratowa.
		// My�la�em nad tym ca�y dzie� i nic m�drego nie wymy�li�em jak to �adniej zrobi� :)

		bool MaterialIndicesUse[16];
		ZeroMem(&MaterialIndicesUse, 16 * sizeof(bool));

		std::vector<MeshMender::Vertex> NvVertices;
		std::vector<unsigned int> MappingNvToTmpVert;
		std::vector<unsigned int> NvIndices;
		std::vector<NV_FACE> NvFaces;
		std::vector<unsigned int> NvMappingOldToNewVert;

		NvVertices.reserve(o.Vertices.size());
		NvIndices.reserve(o.Faces.size() * 4);
		NvFaces.reserve(o.Faces.size());
		NvMappingOldToNewVert.reserve(o.Vertices.size());

		// Dla kolejnych �cianek TMP

		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			if(f.MaterialIndex >= 16)
				throw Error("Material index out of range.");
			MaterialIndicesUse[f.MaterialIndex] = true;

			// Pierwszy tr�jk�t
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[1], o.Vertices[f.VertexIndices[1]], f.TexCoords[1], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
			// (wierzcho�ki dodane/zidenksowane, indeksy dodane - zostaje �cianka)
			NvFaces.push_back(NV_FACE(f.MaterialIndex));

			// Drugi tr�jk�t
			if(f.NumVertices == 4)
			{
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[3], o.Vertices[f.VertexIndices[3]], f.TexCoords[3], f.Normal, o.vertex_normals));
				NvFaces.push_back(NV_FACE(f.MaterialIndex));
			}
		}

		// Struktury po�rednie chyba ju� poprawnie wype�nione, czas odpali� NVMeshMender!

		float MinCreaseCosAngle = cosf(DegToRad(30)); // AutoSmoothAngle w przeciwie�stwie do k�t�w Eulera jest w stopniach!
		float WeightNormalsByArea = 1.0f;
		bool CalcNormals = !o.vertex_normals && !cs.export_phy;
		bool RespectExistingSplits = CalcNormals;
		bool FixCylindricalWrapping = false;

		MeshMender mender;
		if(!mender.Mend(
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

		if(NvVertices.size() > (uint)MAXUINT2)
			throw Error(Format("Too many vertices in object \"#\": #") % o.Name % NvVertices.size());

		// Skonstruuj obiekty wyj�ciowe QMSH

		// Liczba u�ytych materia�ow
		uint NumMatUse = 0;
		for(uint mi = 0; mi < 16; mi++)
			if(MaterialIndicesUse[mi])
				NumMatUse++;

		// Wierzcho�ki (to b�dzie wsp�lne dla wszystkich podsiatek QMSH powsta�ych z bie��cego obiektu QMSH TMP)
		uint MinVertexIndexUsed = Out->Vertices.size();
		uint NumVertexIndicesUsed = NvVertices.size();
		Out->Vertices.reserve(Out->Vertices.size() + NumVertexIndicesUsed);
		for(uint vi = 0; vi < NvVertices.size(); vi++)
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
			if(QmshTmp.Armature != NULL)
			{
				const INTERMEDIATE_VERTEX_SKIN_DATA &ivsd = VertexSkinData[MappingNvToTmpVert[NvMappingOldToNewVert[vi]]];
				v.BoneIndices = (ivsd.Index1) | (ivsd.Index2 << 8);
				v.Weight1 = ivsd.Weight1;
			}
			Out->Vertices.push_back(v);
		}

		// Dla wszystkich u�ytych materia��w
		for(uint mi = 0; mi < 16; mi++)
		{
			if(MaterialIndicesUse[mi])
			{
				shared_ptr<QMSH_SUBMESH> submesh(new QMSH_SUBMESH);
				tmp::Material& m = *o.Materials[mi];

				// Nazwa podsiatki
				// (Je�li wi�cej u�ytych materia��w, to obiekt jest rozbijany na wiele podsiatek i do nazwy dodawany jest materia�)
				submesh->Name = (NumMatUse == 1 ? o.Name : o.Name + "." + m.name);

				submesh->specular_color = m.specular_color;
				submesh->specular_intensity = m.specular_intensity;
				submesh->specular_hardness = m.specular_hardness;
				submesh->normal_factor = 0;
				submesh->specular_factor = 0;
				submesh->specular_color_factor = 0;

				// Nazwa materia�u
				if(mi < o.Materials.size())
				{
					submesh->MaterialName = m.name;

					if(m.image.empty())
					{
						// nowe mo�liwo�ci, kilka tekstur
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
						// zwyk�a tekstura diffuse
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
				for(uint fi = 0, ii = 0; fi < NvFaces.size(); fi++, ii += 3)
				{
					if(NvFaces[fi].MaterialIndex == mi)
					{
						Out->Indices.push_back((uint2)(NvIndices[ii] + MinVertexIndexUsed));
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

	printf("Zapisywanie punkt�w.\n");

	// punkty
	const uint n_bones = Out->Bones.size();
	for(std::vector< shared_ptr<tmp::POINT> >::const_iterator it = QmshTmp.points.begin(), end = QmshTmp.points.end(); it != end; ++it)
	{
		shared_ptr<QMSH_POINT> point(new QMSH_POINT);
		tmp::POINT& pt = *((*it).get());

		point->name = pt.name;
		point->size = pt.size;
		point->type = (uint2)-1;
		for(int i = 0; i < Mesh::Point::MAX; ++i)
		{
			if(pt.type == point_types[i])
			{
				point->type = i;
				break;
			}
		}
		if(point->type == (uint2)-1)
			throw Error(format("Invalid point type '%s'.", pt.type.c_str()));
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

	CalcBoundingVolumes(*Out);
}

void Converter::TmpToQmsh_Bones(QMSH *Out, std::vector<BONE_INTER_DATA> *OutBoneInterData, const tmp::QMSH &QmshTmp)
{
	Writeln("Processing bones...");
	const tmp::ARMATURE & TmpArmature = *QmshTmp.Armature.get();

	// Ta macierz przekszta�ca wsp. z lokalnych obiektu Armature do globalnych �wiata.
	// B�dzie ju� w uk�adzie DirectX.
	MATRIX ArmatureToWorldMat;
	AssemblyBlenderObjectMatrix(&ArmatureToWorldMat, TmpArmature.Position, TmpArmature.Orientation, TmpArmature.Size);
	BlenderToDirectxTransform(&ArmatureToWorldMat);

	// Dla ka�dej ko�ci g��wnego poziomu
	for(uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpBone = *TmpArmature.Bones[bi].get();
		if(TmpBone.Parent.empty())
		{
			// Utw�rz ko�� QMSH
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

	// Sprawdzenie liczby ko�ci
	if(Out->Bones.size() != QmshTmp.Armature->Bones.size())
		Warning(Format("Skipped # invalid bones.") % (QmshTmp.Armature->Bones.size() - Out->Bones.size()), 0);

	// Policzenie Bone Inter Data
	OutBoneInterData->resize(Out->Bones.size());
	for(uint bi = 0; bi < Out->Bones.size(); bi++)
	{
		QMSH_BONE & OutBone = *Out->Bones[bi].get();
		for(uint bj = 0; bj <= TmpArmature.Bones.size(); bj++) // To <= to cze�� tego zabezpieczenia
		{
			assert(bj < TmpArmature.Bones.size()); // zabezpieczenie �e po�r�d ko�ci �r�d�owych zawsze znajdzie si� te� ta docelowa

			const tmp::BONE & InBone = *TmpArmature.Bones[bj].get();
			if(OutBone.Name == InBone.Name)
			{
				BONE_INTER_DATA & bid = (*OutBoneInterData)[bi];

				bid.TmpBoneIndex = bj;

				Vec3 v;
				BlenderToDirectxTransform(&v, InBone.Head);
				Transform(&bid.HeadPos, v, ArmatureToWorldMat);
				BlenderToDirectxTransform(&v, InBone.Tail);
				Transform(&bid.TailPos, v, ArmatureToWorldMat);

				if(!float_equal(TmpArmature.Size.x, TmpArmature.Size.y) || !float_equal(TmpArmature.Size.y, TmpArmature.Size.z))
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

// Funkcja rekurencyjna
void Converter::TmpToQmsh_Bone(QMSH_BONE *Out, uint OutIndex, QMSH *OutQmsh, const tmp::BONE &TmpBone, const tmp::ARMATURE &TmpArmature,
	uint ParentIndex, const MATRIX &WorldToParent)
{
	Out->Name = TmpBone.Name;
	Out->ParentIndex = ParentIndex;

	// TmpBone.Matrix_Armaturespace zawiera przekszta�cenie ze wsp. lokalnych tej ko�ci do wsp. armature w uk�. Blender
	// BoneToArmature zawiera przekszta�cenie ze wsp. lokalnych tej ko�ci do wsp. armature w uk�. DirectX
	MATRIX BoneToArmature;
	BlenderToDirectxTransform(&BoneToArmature, TmpBone.Matrix);
	// Out->Matrix b�dzie zwiera�o przekszta�cenie ze wsp. tej ko�ci do jej parenta w uk�. DirectX
	Out->Matrix = BoneToArmature * WorldToParent;
	MATRIX ArmatureToBone;
	Inverse(&ArmatureToBone, BoneToArmature);

	Out->RawMatrix = TmpBone.Matrix;
	Out->head = Vec4(TmpBone.Head, TmpBone.HeadRadius);
	Out->tail = Vec4(TmpBone.Tail, TmpBone.TailRadius);
	Out->connected = TmpBone.connected;

	// Ko�ci potomne
	for(uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpSubBone = *TmpArmature.Bones[bi].get();
		if(TmpSubBone.Parent == TmpBone.Name)
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

void Converter::TmpToQmsh_Animations(QMSH *OutQmsh, const tmp::QMSH &QmshTmp)
{
	Writeln("Processing animations...");

	for(uint i = 0; i < QmshTmp.Actions.size(); i++)
	{
		shared_ptr<QMSH_ANIMATION> Animation(new QMSH_ANIMATION);
		TmpToQmsh_Animation(Animation.get(), *QmshTmp.Actions[i].get(), *OutQmsh, QmshTmp);
		OutQmsh->Animations.push_back(Animation);
	}

	if(OutQmsh->Animations.size() > (uint4)MAXUINT2)
		Error("Too many animations");
}

void Converter::TmpToQmsh_Animation(QMSH_ANIMATION *OutAnimation, const tmp::ACTION &TmpAction, const QMSH &Qmsh, const tmp::QMSH &QmshTmp)
{
	OutAnimation->Name = TmpAction.Name;

	uint BoneCount = Qmsh.Bones.size();
	// Wsp�czynnik do przeliczania klatek na sekundy
	if(QmshTmp.FPS == 0)
		throw Error("FPS cannot be zero.");
	float TimeFactor = 1.f / (float)QmshTmp.FPS;

	// Lista wska�nik�w do krzywych dotycz�cych poszczeg�lnych parametr�w danej ko�ci QMSH
	// (Mog� by� NULL)

	struct CHANNEL_POINTERS {
		const tmp::CURVE *LocX, *LocY, *LocZ, *QuatX, *QuatY, *QuatZ, *QuatW, *SizeX, *SizeY, *SizeZ;
		uint LocX_index, LocY_index, LocZ_index, QuatX_index, QuatY_index, QuatZ_index, QuatW_index, SizeX_index, SizeY_index, SizeZ_index;
	};
	std::vector<CHANNEL_POINTERS> BoneChannelPointers;
	BoneChannelPointers.resize(BoneCount);
	bool WarningInterpolation = false, WarningExtend = false;
	float EarliestNextFrame = MAXFLOAT;
	// Dla kolejnych ko�ci
	for(uint bi = 0; bi < BoneCount; bi++)
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

		//Znajd� kana� odpowiadaj�cy tej ko�ci
		const string & BoneName = Qmsh.Bones[bi]->Name;
		for(uint ci = 0; ci < TmpAction.Channels.size(); ci++)
		{
			if(TmpAction.Channels[ci]->Name == BoneName)
			{
				// Kana� znaleziony - przejrzyj jego krzywe
				const tmp::CHANNEL &TmpChannel = *TmpAction.Channels[ci].get();
				for(uint curvi = 0; curvi < TmpChannel.Curves.size(); curvi++)
				{
					const tmp::CURVE *TmpCurve = TmpChannel.Curves[curvi].get();
					// Zidentyfikuj po nazwie
					if(TmpCurve->Name == "LocX")
					{
						BoneChannelPointers[bi].LocX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "LocY")
					{
						BoneChannelPointers[bi].LocY = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "LocZ")
					{
						BoneChannelPointers[bi].LocZ = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatX")
					{
						BoneChannelPointers[bi].QuatX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatY")
					{
						BoneChannelPointers[bi].QuatY = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatZ")
					{
						BoneChannelPointers[bi].QuatZ = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatW")
					{
						BoneChannelPointers[bi].QuatW = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleX")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleY")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleW")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
				}

				break;
			}
		}

		if(BoneChannelPointers[bi].LocX != NULL && BoneChannelPointers[bi].LocX->Interpolation == tmp::INTERPOLATION_CONST ||
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

	// Ostrze�enia
	if(WarningInterpolation)
		Warning("Constant IPO interpolation mode not supported.", 2352634);

	// Wygenerowanie klatek kluczowych
	// (Nie ma ani jednej krzywej albo �adna nie ma punkt�w - nie b�dzie klatek kluczowych)
	while(EarliestNextFrame < MAXFLOAT)
	{
		float NextNext = MAXFLOAT;
		shared_ptr<QMSH_KEYFRAME> Keyframe(new QMSH_KEYFRAME);
		Keyframe->Time = (EarliestNextFrame - 1) * TimeFactor; // - 1, bo Blender liczy klatki od 1
		Keyframe->Bones.resize(BoneCount);
		for(uint bi = 0; bi < BoneCount; bi++)
		{
			Keyframe->Bones[bi].Translation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocX, &BoneChannelPointers[bi].LocX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocY, &BoneChannelPointers[bi].LocY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocZ, &BoneChannelPointers[bi].LocZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatX, &BoneChannelPointers[bi].QuatX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatY, &BoneChannelPointers[bi].QuatY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatZ, &BoneChannelPointers[bi].QuatZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.w = -CalcTmpCurveValue(1.f, BoneChannelPointers[bi].QuatW, &BoneChannelPointers[bi].QuatW_index, EarliestNextFrame, &NextNext);
			float ScalingX = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeX, &BoneChannelPointers[bi].SizeX_index, EarliestNextFrame, &NextNext);
			float ScalingY = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeY, &BoneChannelPointers[bi].SizeY_index, EarliestNextFrame, &NextNext);
			float ScalingZ = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeZ, &BoneChannelPointers[bi].SizeZ_index, EarliestNextFrame, &NextNext);
			if(!float_equal(ScalingX, ScalingY) || !float_equal(ScalingY, ScalingZ))
				Warning("Non uniform scaling of bone in animation IPO not supported.", 537536);
			Keyframe->Bones[bi].Scaling = (ScalingX + ScalingY + ScalingZ) / 3.f;
		}
		OutAnimation->Keyframes.push_back(Keyframe);

		EarliestNextFrame = NextNext;
	}

	// Czas trwania animacji, skalowanie czasu
	if(OutAnimation->Keyframes.empty())
		OutAnimation->Length = 0.f;
	else if(OutAnimation->Keyframes.size() == 1)
	{
		OutAnimation->Length = 0.f;
		OutAnimation->Keyframes[0]->Time = 0.f;
	}
	else
	{
		float TimeOffset = -OutAnimation->Keyframes[0]->Time;
		for(uint ki = 0; ki < OutAnimation->Keyframes.size(); ki++)
			OutAnimation->Keyframes[ki]->Time += TimeOffset;
		OutAnimation->Length = OutAnimation->Keyframes[OutAnimation->Keyframes.size() - 1]->Time;
	}

	if(OutAnimation->Keyframes.size() > (uint4)MAXUINT2)
		Error(Format("Too many keyframes in animation \"#\"") % OutAnimation->Name);
}

// Zwraca warto�� krzywej w podanej klatce
// TmpCurve mo�e by� NULL.
// PtIndex - indeks nast�pnego punktu
// NextFrame - je�li nast�pna klatka tej krzywej istnieje i jest mniejsza ni� NextFrame, ustawia NextFrame na ni�
float Converter::CalcTmpCurveValue(float DefaultValue, const tmp::CURVE *TmpCurve, uint *InOutPtIndex, float Frame, float *InOutNextFrame)
{
	// Nie ma krzywej
	if(TmpCurve == NULL)
		return DefaultValue;
	// W og�le nie ma punkt�w
	if(TmpCurve->Points.empty())
		return DefaultValue;
	// To jest ju� koniec krzywej - przed�u�enie ostatniej warto�ci
	if(*InOutPtIndex >= TmpCurve->Points.size())
		return TmpCurve->Points[TmpCurve->Points.size() - 1].y;

	const Vec2 & NextPt = TmpCurve->Points[*InOutPtIndex];

	// - Przestawi� PtIndex (opcjonalnie)
	// - Przestawi� NextFrame
	// - Zwr�ci� warto��

	// Jeste�my dok�adnie w tym punkcie (lub za, co nie powinno mie� miejsca)
	if(float_equal(NextPt.x, Frame) || (Frame > NextPt.x))
	{
		(*InOutPtIndex)++;
		if(*InOutPtIndex < TmpCurve->Points.size())
			*InOutNextFrame = std::min(*InOutNextFrame, TmpCurve->Points[*InOutPtIndex].x);
		return NextPt.y;
	}
	// Jeste�my przed pierwszym punktem - przed�u�enie warto�ci sta�ej w lewo
	else if(*InOutPtIndex == 0)
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		return NextPt.y;
	}
	// Jeste�my mi�dzy punktem poprzednim a tym
	else
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		const Vec2 & PrevPt = TmpCurve->Points[*InOutPtIndex - 1];
		// Interpolacja liniowa warto�ci wynikowej
		float t = (Frame - PrevPt.x) / (NextPt.x - PrevPt.x);
		return Lerp(PrevPt.y, NextPt.y, t);
	}
}

// Przekszta�ca pozycje, wektory i wsp�rz�dne tekstur z uk�adu Blendera do uk�adu DirectX
// Uwzgl�dnia te� przekszta�cenia ca�ych obiekt�w wprowadzaj�c je do przekszta�ce� poszczeg�lnych wierzcho�k�w.
// Zamienia te� kolejno�� wierzcho�k�w w �ciankach co by by�y odwr�cone w�a�ciw� stron�.
// NIE zmienia danych ko�ci ani animacji
void Converter::TransformQmshTmpCoords(tmp::QMSH *InOut)
{
	for(uint oi = 0; oi < InOut->Meshes.size(); oi++)
	{
		tmp::MESH & o = *InOut->Meshes[oi].get();

		// Zbuduj macierz przekszta�cania tego obiektu
		MATRIX Mat, MatRot;
		AssemblyBlenderObjectMatrix(&Mat, o.Position, o.Orientation, o.Size);
		D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&MatRot, o.Orientation.y, o.Orientation.x, o.Orientation.z);
		// Je�li obiekt jest sparentowany do Armature
		// To jednak nic, bo te wsp�rz�dne s� ju� wyeksportowane w uk�adzie globalnym modelu a nie wzgl�dem parenta.
		if(!o.ParentArmature.empty() && !InOut->static_anim /*&& InOut->Armature != NULL*/)
		{
			assert(InOut->Armature != NULL);
			MATRIX ArmatureMat, ArmatureMatRot;
			AssemblyBlenderObjectMatrix(&ArmatureMat, InOut->Armature->Position, InOut->Armature->Orientation, InOut->Armature->Size);
			D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&ArmatureMatRot, InOut->Armature->Orientation.y, InOut->Armature->Orientation.x, InOut->Armature->Orientation.z);
			Mat = Mat * ArmatureMat;
			MatRot = MatRot * ArmatureMatRot;
		}

		// Przekszta�� wierzcho�ki
		for(uint vi = 0; vi < o.Vertices.size(); vi++)
		{
			tmp::VERTEX & v = o.Vertices[vi];
			tmp::VERTEX T;
			Transform(&T.Pos, v.Pos, Mat);
			Transform(&T.Normal, v.Normal, MatRot);
			BlenderToDirectxTransform(&v.Pos, T.Pos);
			BlenderToDirectxTransform(&v.Normal, T.Normal);
		}

		// Przekszta�� �cianki
		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			// Wsp. tekstur
			for(uint vi = 0; vi < f.NumVertices; vi++)
				f.TexCoords[vi].y = 1.0f - f.TexCoords[vi].y;

			// Zamie� kolejno�� wierzcho�k�w w �ciance
			if(f.NumVertices == 3)
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

			BlenderToDirectxTransform(&f.Normal);
		}
	}

	for(uint i = 0, count = InOut->points.size(); i < count; ++i)
	{
		tmp::POINT* point = InOut->points[i].get();
		BlenderToDirectxTransform(&point->size);
		point->rot.y = Clip(-point->rot.y);
	}
}

// Szuka takiego wierzcho�ka w NvVertices.
// Je�li nie znajdzie, dodaje nowy.
// Czy znalaz� czy doda�, zwraca jego indeks w NvVertices.
uint Converter::TmpConvert_AddVertex(std::vector<MeshMender::Vertex>& NvVertices, std::vector<unsigned int>& MappingNvToTmpVert, uint TmpVertIndex,
	const tmp::VERTEX& TmpVertex, const Vec2& TexCoord, const Vec3& face_normal, bool vertex_normals)
{
	for(uint i = 0; i < NvVertices.size(); i++)
	{
		MeshMender::Vertex& v = NvVertices[i];
		// Pozycja identyczna...
		if(v.pos == TmpVertex.Pos &&
			// TexCoord mniej wi�cej...
			float_equal(TexCoord.x, v.s) && float_equal(TexCoord.y, v.t))
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

void Converter::CalcVertexSkinData(std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> *Out, const tmp::MESH &TmpMesh, const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp, const std::vector<BONE_INTER_DATA> &BoneInterData)
{
	Out->resize(TmpMesh.Vertices.size());

	// Kolejny algorytm pisany z wielk� nadziej� �e zadzia�a, wolny ale co z tego, lepszy taki
	// ni� �aden, i tak si� nad nim namy�la�em ca�y dzie�, a drugi poprawia�em bo si� wszystko pomiesza�o!

	// Nie ma skinningu
	if(TmpMesh.ParentArmature.empty())
	{
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = 0;
			(*Out)[vi].Index2 = 0;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Jest skinning ca�ej siatki do jednej ko�ci
	else if(!TmpMesh.ParentBone.empty())
	{
		// Znajd� indeks tej ko�ci
		uint1 BoneIndex = 0;
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			if(Qmsh.Bones[bi]->Name == TmpMesh.ParentBone)
			{
				BoneIndex = bi + 1;
				break;
			}
		}
		// Nie znaleziono
		if(BoneIndex == 0)
			Warning("Object parented to non existing bone.", 13497325);
		// Przypisz t� ko�� do wszystkich wierzcho�k�w
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = BoneIndex;
			(*Out)[vi].Index2 = BoneIndex;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Vertex Group lub Envelope
	else
	{
		// U�� szybk� list� wierzcho�k�w z wagami dla ka�dej ko�ci

		// Typ reprezentuje dla danej ko�ci grup� wierzcho�k�w: Indeks wierzcho�ka => Waga
		typedef std::map<uint, float> INFLUENCE_MAP;
		std::vector<INFLUENCE_MAP> BoneInfluences;
		BoneInfluences.resize(Qmsh.Bones.size());
		// Dla ka�dej ko�ci
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// Znajd� odpowiadaj�c� jej grup� wierzcho�k�w w przetwarzanej w tym wywo�aniu funkcji siatce
			uint VertexGroupIndex = TmpMesh.VertexGroups.size();
			for(uint gi = 0; gi < TmpMesh.VertexGroups.size(); gi++)
			{
				if(TmpMesh.VertexGroups[gi]->Name == Qmsh.Bones[bi]->Name)
				{
					VertexGroupIndex = gi;
					break;
				}
			}
			// Je�li znaleziono, przepisz wszystkie wierzcho�ki
			if(VertexGroupIndex < TmpMesh.VertexGroups.size())
			{
				const tmp::VERTEX_GROUP & VertexGroup = *TmpMesh.VertexGroups[VertexGroupIndex].get();
				for(uint vi = 0; vi < VertexGroup.VerticesInGroup.size(); vi++)
				{
					BoneInfluences[bi].insert(INFLUENCE_MAP::value_type(
						VertexGroup.VerticesInGroup[vi].Index,
						VertexGroup.VerticesInGroup[vi].Weight));
				}
			}
		}

		// Dla ka�dego wierzcho�ka
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			// Zbierz wszystkie wp�ywaj�ce na niego ko�ci
			struct INFLUENCE
			{
				uint BoneIndex; // Uwaga! Wy�ej by�o VertexIndex -> Weight, a tutaj jest BoneIndex -> Weight - ale masakra !!!
				float Weight;

				bool operator > (const INFLUENCE &Influence) const { return Weight > Influence.Weight; }
			};
			std::vector<INFLUENCE> VertexInfluences;

			// Wp�yw przez Vertex Groups
			if(1)
			{
				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					INFLUENCE_MAP::iterator biit = BoneInfluences[bi].find(vi);
					if(biit != BoneInfluences[bi].end())
					{
						INFLUENCE Influence = { bi + 1, biit->second };
						VertexInfluences.push_back(Influence);
					}
				}
			}

			// �adna ko�� na niego nie wp�ywa - wyp�yw z Envelopes
			if(VertexInfluences.empty())
			{
				// U�� list� wp�ywaj�cych ko�ci
				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					// Nie jestem pewny czy to jest dok�adnie algorytm u�ywany przez Blender,
					// nie jest nigdzie dok�adnie opisany, ale z eksperyment�w podejrzewam, �e tak.
					// Ko�� w promieniu - wp�yw maksymalny.
					// Ko�� w zakresie czego� co nazwa�em Extra Envelope te� jest wp�yw (podejrzewam �e s�abnie z odleg�o�ci�)
					// Promie� Extra Envelope, jak uda�o mi si� wreszcie ustali�, jest niezale�ny od Radius w sensie
					// �e rozci�ga si� ponad Radius zawsze na tyle ile wynosi (BoneLength / 4)

					// Dodatkowy promie� zewn�trzny
					float ExtraEnvelopeRadius = BoneInterData[bi].Length * 0.25f;

					// Pozycja wierzcho�ka ju� jest w uk�adzie globalnym modelu i w konwencji DirectX
					float W = PointToBone(
						TmpMesh.Vertices[vi].Pos,
						BoneInterData[bi].HeadPos, BoneInterData[bi].HeadRadius, BoneInterData[bi].HeadRadius + ExtraEnvelopeRadius,
						BoneInterData[bi].TailPos, BoneInterData[bi].TailRadius, BoneInterData[bi].TailRadius + ExtraEnvelopeRadius);

					if(W > 0.f)
					{
						INFLUENCE Influence = { bi + 1, W };
						VertexInfluences.push_back(Influence);
					}
				}
			}
			// Jakie� ko�ci na niego wp�ywaj� - we� z tych ko�ci

			// Zero ko�ci
			if(VertexInfluences.empty())
			{
				(*Out)[vi].Index1 = 0;
				(*Out)[vi].Index2 = 0;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Tylko jedna ko��
			else if(VertexInfluences.size() == 1)
			{
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Dwie lub wi�cej ko�ci na li�cie wp�ywaj�cych na ten wierzcho�ek
			else
			{
				// Posortuj wp�ywy na wierzcho�ek malej�co, czyli od najwi�kszej wagi
				std::sort(VertexInfluences.begin(), VertexInfluences.end(), std::greater<INFLUENCE>());
				// We� dwie najwa�niejsze ko�ci
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[1].BoneIndex;
				// Oblicz wag� pierwszej
				// WA�NY WZ�R NA ZNORMALIZOWAN� WAG� PIERWSZ� Z DW�CH DOWOLNYCH WAG !!!
				(*Out)[vi].Weight1 = VertexInfluences[0].Weight / (VertexInfluences[0].Weight + VertexInfluences[1].Weight);
			}
		}
	}
}

// Liczy kolizj� punktu z bry�� wyznaczon� przez dwie po��czone kule, ka�da ma sw�j �rodek i promie�
// Wygl�ta to co� jak na ko�cach dwie p�kule a w �rodku walec albo �ci�ty sto�ek.
// Ka�dy koniec ma dwa promienie. Zwraca wp�yw ko�ci na ten punkt, tzn.:
// - Je�li punkt le�y w promieniu promienia wewn�trznego, zwraca 1.
// - Je�li punkt le�y w promieniu promienia zewn�trznego, zwraca 1..0, im dalej tym mniej.
// - Je�li punkt le�y poza promieniem zewn�trznym, zwraca 0.
float Converter::PointToBone(const Vec3 &Pt,
	const Vec3 &Center1, float InnerRadius1, float OuterRadius1,
	const Vec3 &Center2, float InnerRadius2, float OuterRadius2)
{
	float T = ClosestPointOnLine(Pt, Center1, Center2 - Center1) / DistanceSq(Center1, Center2);
	if(T <= 0.f)
	{
		float D = Distance(Pt, Center1);
		if(D <= InnerRadius1)
			return 1.f;
		else if(D < OuterRadius1)
			return 1.f - (D - InnerRadius1) / (OuterRadius1 - InnerRadius1);
		else
			return 0.f;
	}
	else if(T >= 1.f)
	{
		float D = Distance(Pt, Center2);
		if(D <= InnerRadius2)
			return 1.f;
		else if(D < OuterRadius2)
			return 1.f - (D - InnerRadius2) / (OuterRadius2 - InnerRadius2);
		else
			return 0.f;
	}
	else
	{
		float InterInnerRadius = Lerp(InnerRadius1, InnerRadius2, T);
		float InterOuterRadius = Lerp(OuterRadius1, OuterRadius2, T);
		Vec3 InterCenter; Lerp(&InterCenter, Center1, Center2, T);

		float D = Distance(Pt, InterCenter);
		if(D <= InterInnerRadius)
			return 1.f;
		else if(D < InterOuterRadius)
			return 1.f - (D - InterInnerRadius) / (InterOuterRadius - InterInnerRadius);
		else
			return 0.f;
	}
}

void Converter::CreateBoneGroups(QMSH& qmsh, const tmp::QMSH &QmshTmp, ConversionData& cs)
{
	if(QmshTmp.Armature == nullptr)
		return;

	if(!QmshTmp.Armature->Groups.empty())
	{
		// use exported bone groups
		qmsh.Groups.resize(QmshTmp.Armature->Groups.size());
		for(uint i = 0; i < qmsh.Groups.size(); ++i)
		{
			QMSH_GROUP& group = qmsh.Groups[i];
			tmp::BONE_GROUP& tmp_group = *QmshTmp.Armature->Groups[i].get();
			group.name = tmp_group.Name;
			if(tmp_group.Parent.empty())
				group.parent = i;
			else
			{
				group.parent = QmshTmp.Armature->FindBoneGroupIndex(tmp_group.Parent);
				if(group.parent == (uint)-1)
					throw Error(format("Invalid parent bone group '%s'.", tmp_group.Parent.c_str()));
			}
		}

		// verify all bones have valid bone group
		for(uint i = 0; i < qmsh.Bones.size(); ++i)
		{
			tmp::BONE& tmp_bone = *QmshTmp.Armature->Bones[i].get();
			if(tmp_bone.Group.empty())
				throw Error(format("Missing bone group for bone '%s'.", tmp_bone.Name.c_str()));
			uint index = QmshTmp.Armature->FindBoneGroupIndex(tmp_bone.Group);
			if(index == (uint)-1)
				throw Error(format("Invalid parent bone group '%s' for bone '%s'.", tmp_bone.Group.c_str(), tmp_bone.Name.c_str()));
			qmsh.Groups[index].bones.push_back(qmsh.Bones[i].get());
		}

		qmsh.real_bones = qmsh.Bones.size();
	}
	else if(cs.gopt == GO_ONE)
	{
		qmsh.Groups.resize(1);
		QMSH_GROUP& gr = qmsh.Groups[0];
		gr.name = "default";
		gr.parent = 0;

		for(uint i = 0; i < qmsh.Bones.size(); ++i)
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
			throw Error(format("B��d parsowania pliku konfiguracyjnego '%s'!\n%s", cs.group_file.c_str(), err), __FILE__, __LINE__);
		}
		catch(TokenizerError& err)
		{
			string msg;
			err.GetMessage_(&msg);
			throw Error(format("B��d parsowania pliku konfiguracyjnego '%s'!\n%s", cs.group_file.c_str(), msg.c_str()), __FILE__, __LINE__);
		}

		// policz ile jest specjalnych ko�ci
		int special_count = 0;
		for(std::vector< shared_ptr<QMSH_BONE> >::iterator it = qmsh.Bones.begin(), end = qmsh.Bones.end(); it != end; ++it)
		{
			if((*it)->non_mesh)
				++special_count;
		}

		// same normalne ko�ci, my job is done here!
		if(special_count == 0)
		{
			qmsh.real_bones = qmsh.Bones.size();
			return;
		}

		std::sort(qmsh.Bones.begin(), qmsh.Bones.end(), [](const shared_ptr<QMSH_BONE>& b1, const shared_ptr<QMSH_BONE>& b2)
		{
			return b1->non_mesh < b2->non_mesh;
		});
		qmsh.real_bones = qmsh.Bones.size() - special_count;

		printf("Plik wczytany.\n");
	}
}

void Converter::LoadBoneGroups(QMSH& qmsh, Tokenizer& t)
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
		throw format("Niew�a�ciwa liczba ko�ci %d do %d!", bone_count, qmsh.Bones.size());
	t.Next();

	// groups: X
	t.AssertKeyword(2);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	int groups = t.MustGetInt4();
	if(groups < 1 || groups > 255)
		throw format("Niew�a�ciwa liczba grup %d!", groups);
	t.Next();

	qmsh.Groups.resize(groups);

	// ustaw parent ID na -1 aby zobaczy� co nie zosta�o ustawione
	for(std::vector<QMSH_GROUP>::iterator it = qmsh.Groups.begin(), end = qmsh.Groups.end(); it != end; ++it)
		it->parent = (unsigned short)-1;

	// grupy
	// group: ID {
	//    name: "NAME"
	//    parent: ID
	// };
	for(int i = 0; i < groups; ++i)
	{
		// group: ID
		t.AssertKeyword(3);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int id = t.MustGetInt4();
		if(id < 0 || id >= groups)
			throw format("Z�y identyfikator grupy %d!", id);
		QMSH_GROUP& gr = qmsh.Groups[id];
		if(gr.parent != (unsigned short)-1)
			throw format("Grupa '%s' zosta�a ju� zdefiniowana!", gr.name.c_str());
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
				throw format("Grupa %d nie mo�e nazywa� si� '%s'! Grupa %d ma ju� t� nazw�!", id, gr.name.c_str(), index);
		}
		t.Next();

		// parent: ID
		t.AssertKeyword(5);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int pid = t.MustGetInt4();
		if(pid < 0 || pid >= groups)
			throw format("%d nie mo�e by� grup� nadrz�dn� dla grup '%s'!", pid, gr.name.c_str());
		if(id == pid && id != 0)
			throw format("Tylko zerowa grupa mo�e wskazywa� na sam� siebie, grupa '%s'!", gr.name.c_str());
		if(id == 0 && pid != 0)
			throw "Zerowa grupa musi wskazywa� na sam� siebie!";
		gr.parent = (unsigned short)pid;
		t.Next();

		// };
		t.AssertSymbol('}');
		t.Next();
		t.AssertSymbol(';');
		t.Next();
	}

	// sprawd� hierarchi� grup
	for(int i = 0; i < groups; ++i)
	{
		int id = qmsh.Groups[i].parent;
		bool ok = false;

		for(int j = 0; j < groups; ++j)
		{
			if(id == 0)
			{
				ok = true;
				break;
			}

			id = qmsh.Groups[id].parent;
		}

		if(!ok)
			throw format("Grupa %s ma uszkodzon� hierarchi�, nie prowadzi do zerowej!", qmsh.Groups[i].name.c_str());
	}

	// ustaw grupy w ko�ciach na -1
	for(std::vector<shared_ptr<QMSH_BONE> >::iterator it = qmsh.Bones.begin(), end = qmsh.Bones.end(); it != end; ++it)
	{
		(*it)->group_index = -1;
	}

	// wczytaj ko�ci
	// bone: "NAME" {
	//    group: ID
	//    specjal: X
	// };
	for(int i = 0; i < bone_count; ++i)
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
			throw format("Brak ko�ci o nazwie '%s'!", bone_name.c_str());
		if(bone->group_index != -1)
			throw format("Ko�� '%s' ma ju� ustawion� grup� %d!", bone_name.c_str(), bone->group_index);
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
			throw format("Brak grupy o id %d dla ko�ci '%s'!", group_id, bone_name.c_str());
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
			throw format("Nieznana warto�� specjalna %d dla ko�ci '%s'!", special, bone_name.c_str());
		bone->non_mesh = (special == 1 ? true : false);
		t.Next();

		// };
		t.AssertSymbol('}');
		t.Next();
		t.AssertSymbol(';');
		t.Next();
	}

	// sprawd� czy wszystkie grupy maj� jakie� ko�ci
	for(int i = 0; i < groups; ++i)
	{
		if(qmsh.Groups[i].bones.empty())
			throw format("Grupa '%s' nie ma ko�ci!", qmsh.Groups[i].name.c_str());
	}
}

// Sk�ada translacj�, rotacje i skalowanie w macierz w uk�adzie Blendera,
// kt�ra wykonuje te operacje transformuj�c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void Converter::AssemblyBlenderObjectMatrix(MATRIX *Out, const Vec3 &Loc, const Vec3 &Rot, const Vec3 &Size)
{
	MATRIX TmpMat;
	Scaling(Out, Size);
	RotationX(&TmpMat, Rot.x);
	*Out *= TmpMat;
	RotationY(&TmpMat, Rot.y);
	*Out *= TmpMat;
	RotationZ(&TmpMat, Rot.z);
	*Out *= TmpMat;
	Translation(&TmpMat, Loc);
	*Out *= TmpMat;
}

// Przekszta�ca punkt lub wektor ze wsp�rz�dnych Blendera do DirectX
void Converter::BlenderToDirectxTransform(Vec3 *Out, const Vec3 &In)
{
	Out->x = In.x;
	Out->y = In.z;
	Out->z = In.y;
}

void Converter::BlenderToDirectxTransform(Vec3 *InOut)
{
	std::swap(InOut->y, InOut->z);
}

// Przekszta�ca macierz przekszta�caj�c� punkty/wektory z takiej dzia�aj�cej na wsp�rz�dnych Blendera
// na tak� dzia�aj�c� na wsp�rz�dnych DirectX-a (i odwrotnie te�).
void Converter::BlenderToDirectxTransform(MATRIX *Out, const MATRIX &In)
{
	Out->_11 = In._11;
	Out->_14 = In._14;
	Out->_41 = In._41;
	Out->_44 = In._44;

	Out->_12 = In._13;
	Out->_13 = In._12;

	Out->_43 = In._42;
	Out->_42 = In._43;

	Out->_31 = In._21;
	Out->_21 = In._31;

	Out->_24 = In._34;
	Out->_34 = In._24;

	Out->_22 = In._33;
	Out->_33 = In._22;
	Out->_23 = In._32;
	Out->_32 = In._23;
}

void Converter::BlenderToDirectxTransform(MATRIX *InOut)
{
	std::swap(InOut->_12, InOut->_13);
	std::swap(InOut->_42, InOut->_43);

	std::swap(InOut->_21, InOut->_31);
	std::swap(InOut->_24, InOut->_34);

	std::swap(InOut->_22, InOut->_33);
	std::swap(InOut->_23, InOut->_32);
}

// Wylicza parametry bry� otaczaj�cych siatk�
void Converter::CalcBoundingVolumes(QMSH &Qmsh)
{
	Writeln("Calculating bounding volumes...");

	CalcBoundingVolumes(Qmsh, &Qmsh.BoundingSphereRadius, &Qmsh.BoundingBox);

	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
			CalcBoundingVolumes(Qmsh, **it);
	}
}

void Converter::CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, BOX *OutBox)
{
	if(Qmsh.Vertices.empty())
	{
		*OutSphereRadius = 0.f;
		*OutBox = BOX(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	else
	{
		float MaxDistSq = LengthSq(Qmsh.Vertices[0].Pos);
		OutBox->p1 = OutBox->p2 = Qmsh.Vertices[0].Pos;

		for(uint i = 1; i < Qmsh.Vertices.size(); i++)
		{
			MaxDistSq = std::max(MaxDistSq, LengthSq(Qmsh.Vertices[i].Pos));
			Min(&OutBox->p1, OutBox->p1, Qmsh.Vertices[i].Pos);
			Max(&OutBox->p2, OutBox->p2, Qmsh.Vertices[i].Pos);
		}

		*OutSphereRadius = sqrtf(MaxDistSq);
	}
}

void Converter::CalcBoundingVolumes(const QMSH& mesh, QMSH_SUBMESH& sub)
{
	float radius = 0.f;
	Vec3 center;
	float inf = std::numeric_limits<float>::infinity();
	BOX box(inf, inf, inf, -inf, -inf, -inf);

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			box.AddInternalPoint(mesh.Vertices[index].Pos);
		}
	}

	box.CalcCenter(&center);
	sub.box = box;

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			const Vec3 &Tmp = mesh.Vertices[index].Pos;
			float dist = Distance(center, Tmp);
			radius = std::max(radius, dist);
		}
	}

	sub.center = center;
	sub.range = radius;
}
