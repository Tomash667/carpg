#include "PCH.hpp"
#include "Converter.h"
#include "Mesh.h"

const Vec3 DefaultSpecularColor(1, 1, 1);
const float DefaultSpecularIntensity = 0.2f;
const int DefaultSpecularHardness = 10;

// Przechowuje dane na temat wybranej koœci we wsp. globalnych modelu w uk³adzie DirectX
struct BONE_INTER_DATA
{
	Vec3 HeadPos;
	Vec3 TailPos;
	float HeadRadius;
	float TailRadius;
	float Length;
	uint TmpBoneIndex; // Indeks tej koœci w Armature w TMP, liczony od 0 - stanowi mapowanie nowych na stare koœci
};

struct INTERMEDIATE_VERTEX_SKIN_DATA
{
	byte Index1;
	byte Index2;
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
	// Przekszta³æ uk³ad wspó³rzêdnych
	TransformQmshTmpCoords(&QmshTmp);

	// BoneInterData odpowiada koœciom wyjœciowym QMSH.Bones, nie wejœciowym z TMP.Armature!!!
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

	Info("Processing mesh (NVMeshMender and more)...");

	struct NV_FACE
	{
		uint MaterialIndex;
		NV_FACE(uint MaterialIndex) : MaterialIndex(MaterialIndex) { }
	};

	// Dla kolejnych obiektów TMP
	for(uint oi = 0; oi < QmshTmp.Meshes.size(); oi++)
	{
		tmp::MESH & o = *QmshTmp.Meshes[oi].get();

		// Policz wp³yw koœci na wierzcho³ki tej siatki
		std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> VertexSkinData;
		if((Out->Flags & FLAG_SKINNING) != 0)
			CalcVertexSkinData(&VertexSkinData, o, *Out, QmshTmp, BoneInterData);

		// Skonstruuj struktury poœrednie
		// UWAGA! Z³o¿onoœæ kwadratowa.
		// Myœla³em nad tym ca³y dzieñ i nic m¹drego nie wymyœli³em jak to ³adniej zrobiæ :)

		bool MaterialIndicesUse[16] = {};

		std::vector<MeshMender::Vertex> NvVertices;
		std::vector<unsigned int> MappingNvToTmpVert;
		std::vector<unsigned int> NvIndices;
		std::vector<NV_FACE> NvFaces;
		std::vector<unsigned int> NvMappingOldToNewVert;

		NvVertices.reserve(o.Vertices.size());
		NvIndices.reserve(o.Faces.size() * 4);
		NvFaces.reserve(o.Faces.size());
		NvMappingOldToNewVert.reserve(o.Vertices.size());

		// Dla kolejnych œcianek TMP

		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			if(f.MaterialIndex >= 16)
				throw "Material index out of range.";
			MaterialIndicesUse[f.MaterialIndex] = true;

			// Pierwszy trójk¹t
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[1], o.Vertices[f.VertexIndices[1]], f.TexCoords[1], f.Normal, o.vertex_normals));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
			// (wierzcho³ki dodane/zidenksowane, indeksy dodane - zostaje œcianka)
			NvFaces.push_back(NV_FACE(f.MaterialIndex));

			// Drugi trójk¹t
			if(f.NumVertices == 4)
			{
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal, o.vertex_normals));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[3], o.Vertices[f.VertexIndices[3]], f.TexCoords[3], f.Normal, o.vertex_normals));
				NvFaces.push_back(NV_FACE(f.MaterialIndex));
			}
		}

		// Struktury poœrednie chyba ju¿ poprawnie wype³nione, czas odpaliæ NVMeshMender!

		float MinCreaseCosAngle = cosf(ToRadians(30)); // AutoSmoothAngle w przeciwieñstwie do k¹tów Eulera jest w stopniach!
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
			throw "NVMeshMender failed.";
		}

		assert(NvIndices.size() % 3 == 0);

		if(NvVertices.size() > (uint)std::numeric_limits<word>::max())
			throw Format("Too many vertices in object \"%s\": %u", o.Name.c_str(), NvVertices.size());

		// Skonstruuj obiekty wyjœciowe QMSH

		// Liczba u¿ytych materia³ow
		uint NumMatUse = 0;
		for(uint mi = 0; mi < 16; mi++)
			if(MaterialIndicesUse[mi])
				NumMatUse++;

		// Wierzcho³ki (to bêdzie wspólne dla wszystkich podsiatek QMSH powsta³ych z bie¿¹cego obiektu QMSH TMP)
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

		// Dla wszystkich u¿ytych materia³ów
		for(uint mi = 0; mi < 16; mi++)
		{
			if(MaterialIndicesUse[mi])
			{
				shared_ptr<QMSH_SUBMESH> submesh(new QMSH_SUBMESH);

				submesh->normal_factor = 0;
				submesh->specular_factor = 0;
				submesh->specular_color_factor = 0;

				// Nazwa materia³u
				if(mi < o.Materials.size())
				{
					tmp::Material& m = *o.Materials[mi];

					// Nazwa podsiatki
					// (Jeœli wiêcej u¿ytych materia³ów, to obiekt jest rozbijany na wiele podsiatek i do nazwy dodawany jest materia³)
					submesh->Name = (NumMatUse == 1 ? o.Name : o.Name + "." + m.name);
					submesh->MaterialName = m.name;
					submesh->specular_color = m.specular_color;
					submesh->specular_intensity = m.specular_intensity;
					submesh->specular_hardness = m.specular_hardness;

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
								{
									throw Format("Mesh '%s', material '%s', texture '%s': material already have diffuse texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image.c_str(), submesh->texture.c_str());
								}
								submesh->texture = t.image;
								if(!Equal(t.diffuse_factor, 1.f))
								{
									Warn("Mesh '%s', material '%s', texture '%s' has diffuse factor %g (only 1.0 is supported).", o.Name.c_str(), m.name.c_str(), t.image.c_str(),
										t.diffuse_factor);
								}
							}
							else if(!t.use_diffuse && t.use_normal && !t.use_specular && !t.use_specular_color)
							{
								// normal
								if(!submesh->normalmap_texture.empty())
								{
									throw Format("Mesh '%s', material '%s', texture '%s': material already have normal texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image.c_str(), submesh->normalmap_texture.c_str());
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
									throw Format("Mesh '%s', material '%s', texture '%s': material already have specular texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image, submesh->specularmap_texture.c_str());
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
				else
				{
					submesh->Name = o.Name;
					submesh->MaterialName = o.Name;
					submesh->specular_color = DefaultSpecularColor;
					submesh->specular_intensity = DefaultSpecularIntensity;
					submesh->specular_hardness = DefaultSpecularHardness;
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
						Out->Indices.push_back((word)(NvIndices[ii] + MinVertexIndexUsed));
						Out->Indices.push_back((word)(NvIndices[ii + 1] + MinVertexIndexUsed));
						Out->Indices.push_back((word)(NvIndices[ii + 2] + MinVertexIndexUsed));
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

	Info("Zapisywanie punktów.");

	// punkty
	const uint n_bones = Out->Bones.size();
	for(std::vector< shared_ptr<tmp::POINT> >::const_iterator it = QmshTmp.points.begin(), end = QmshTmp.points.end(); it != end; ++it)
	{
		shared_ptr<QMSH_POINT> point(new QMSH_POINT);
		tmp::POINT& pt = *((*it).get());

		point->name = pt.name;
		point->size = Vec3(abs(pt.size.x), abs(pt.size.y), abs(pt.size.z));
		BlenderToDirectxTransform(&point->size);
		point->type = (word)-1;
		for(int i = 0; i < Mesh::Point::MAX; ++i)
		{
			if(pt.type == point_types[i])
			{
				point->type = i;
				break;
			}
		}
		if(point->type == (word)-1)
			throw Format("Invalid point type '%s'.", pt.type.c_str());
		BlenderToDirectxTransform(&point->matrix, pt.matrix);
		BlenderToDirectxTransform(&point->rot, pt.rot);
		point->rot.y = Clip(-point->rot.y);

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
	Info("Processing bones...");
	const tmp::ARMATURE & TmpArmature = *QmshTmp.Armature.get();

	// Ta macierz przekszta³ca wsp. z lokalnych obiektu Armature do globalnych œwiata.
	// Bêdzie ju¿ w uk³adzie DirectX.
	Matrix ArmatureToWorldMat;
	AssemblyBlenderObjectMatrix(&ArmatureToWorldMat, TmpArmature.Position, TmpArmature.Orientation, TmpArmature.Size);
	BlenderToDirectxTransform(&ArmatureToWorldMat);

	// Dla ka¿dej koœci g³ównego poziomu
	for(uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpBone = *TmpArmature.Bones[bi].get();
		if(TmpBone.Parent.empty())
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
	if(Out->Bones.size() != QmshTmp.Armature->Bones.size())
		WarnOnce(1, "Skipped %u invalid bones.", QmshTmp.Armature->Bones.size() - Out->Bones.size());

	// Policzenie Bone Inter Data
	OutBoneInterData->resize(Out->Bones.size());
	for(uint bi = 0; bi < Out->Bones.size(); bi++)
	{
		QMSH_BONE & OutBone = *Out->Bones[bi].get();
		for(uint bj = 0; bj <= TmpArmature.Bones.size(); bj++) // To <= to czeœæ tego zabezpieczenia
		{
			assert(bj < TmpArmature.Bones.size()); // zabezpieczenie ¿e poœród koœci Ÿród³owych zawsze znajdzie siê te¿ ta docelowa

			const tmp::BONE & InBone = *TmpArmature.Bones[bj].get();
			if(OutBone.Name == InBone.Name)
			{
				BONE_INTER_DATA & bid = (*OutBoneInterData)[bi];

				bid.TmpBoneIndex = bj;

				Vec3 v;
				BlenderToDirectxTransform(&v, InBone.Head);
				bid.HeadPos = Vec3::Transform(v, ArmatureToWorldMat);
				BlenderToDirectxTransform(&v, InBone.Tail);
				bid.TailPos = Vec3::Transform(v, ArmatureToWorldMat);

				if(!Equal(TmpArmature.Size.x, TmpArmature.Size.y) || !Equal(TmpArmature.Size.y, TmpArmature.Size.z))
					WarnOnce(2342235, "Non uniform scaling of Armature object may give invalid bone envelopes.");

				float ScaleFactor = (TmpArmature.Size.x + TmpArmature.Size.y + TmpArmature.Size.z) / 3.f;

				bid.HeadRadius = InBone.HeadRadius * ScaleFactor;
				bid.TailRadius = InBone.TailRadius * ScaleFactor;

				bid.Length = Vec3::Distance(bid.HeadPos, bid.TailPos);

				break;
			}
		}
	}
}

// Funkcja rekurencyjna
void Converter::TmpToQmsh_Bone(QMSH_BONE *Out, uint OutIndex, QMSH *OutQmsh, const tmp::BONE &TmpBone, const tmp::ARMATURE &TmpArmature,
	uint ParentIndex, const Matrix &WorldToParent)
{
	Out->Name = TmpBone.Name;
	Out->ParentIndex = ParentIndex;

	// TmpBone.Matrix_Armaturespace zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. Blender
	// BoneToArmature zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. DirectX
	Matrix BoneToArmature;
	BlenderToDirectxTransform(&BoneToArmature, TmpBone.matrix);
	// Out->Matrix bêdzie zwiera³o przekszta³cenie ze wsp. tej koœci do jej parenta w uk³. DirectX
	Out->matrix = BoneToArmature * WorldToParent;
	Matrix ArmatureToBone = BoneToArmature.Inverse();

	Out->RawMatrix = TmpBone.matrix;
	Out->head = Vec4(TmpBone.Head, TmpBone.HeadRadius);
	Out->tail = Vec4(TmpBone.Tail, TmpBone.TailRadius);
	Out->connected = TmpBone.connected;

	// Koœci potomne
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
	Info("Processing animations...");

	for(uint i = 0; i < QmshTmp.Actions.size(); i++)
	{
		shared_ptr<QMSH_ANIMATION> Animation(new QMSH_ANIMATION);
		TmpToQmsh_Animation(Animation.get(), *QmshTmp.Actions[i].get(), *OutQmsh, QmshTmp);
		OutQmsh->Animations.push_back(Animation);
	}

	if(OutQmsh->Animations.size() > std::numeric_limits<word>::max())
		throw "Too many animations";
}

void Converter::TmpToQmsh_Animation(QMSH_ANIMATION *OutAnimation, const tmp::ACTION &TmpAction, const QMSH &Qmsh, const tmp::QMSH &QmshTmp)
{
	OutAnimation->Name = TmpAction.Name;

	uint BoneCount = Qmsh.Bones.size();
	// Wspó³czynnik do przeliczania klatek na sekundy
	if(QmshTmp.FPS == 0)
		throw "FPS cannot be zero.";
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
	float EarliestNextFrame = std::numeric_limits<float>::max();
	// Dla kolejnych koœci
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

		//ZnajdŸ kana³ odpowiadaj¹cy tej koœci
		const string & BoneName = Qmsh.Bones[bi]->Name;
		for(uint ci = 0; ci < TmpAction.Channels.size(); ci++)
		{
			if(TmpAction.Channels[ci]->Name == BoneName)
			{
				// Kana³ znaleziony - przejrzyj jego krzywe
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

	// Ostrze¿enia
	if(WarningInterpolation)
		WarnOnce(2352634, "Constant IPO interpolation mode not supported.");

	// Wygenerowanie klatek kluczowych
	// (Nie ma ani jednej krzywej albo ¿adna nie ma punktów - nie bêdzie klatek kluczowych)
	while(EarliestNextFrame < std::numeric_limits<float>::max())
	{
		float NextNext = std::numeric_limits<float>::max();
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
			if(!Equal(ScalingX, ScalingY) || !Equal(ScalingY, ScalingZ))
				WarnOnce(537536, "Non uniform scaling of bone in animation IPO not supported.");
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

	if(OutAnimation->Keyframes.size() > std::numeric_limits<word>::max())
		throw Format("Too many keyframes in animation \"%s\"", OutAnimation->Name.c_str());
}

// Zwraca wartoœæ krzywej w podanej klatce
// TmpCurve mo¿e byæ NULL.
// PtIndex - indeks nastêpnego punktu
// NextFrame - jeœli nastêpna klatka tej krzywej istnieje i jest mniejsza ni¿ NextFrame, ustawia NextFrame na ni¹
float Converter::CalcTmpCurveValue(float DefaultValue, const tmp::CURVE *TmpCurve, uint *InOutPtIndex, float Frame, float *InOutNextFrame)
{
	// Nie ma krzywej
	if(TmpCurve == NULL)
		return DefaultValue;
	// W ogóle nie ma punktów
	if(TmpCurve->Points.empty())
		return DefaultValue;
	// To jest ju¿ koniec krzywej - przed³u¿enie ostatniej wartoœci
	if(*InOutPtIndex >= TmpCurve->Points.size())
		return TmpCurve->Points[TmpCurve->Points.size() - 1].y;

	const Vec2 & NextPt = TmpCurve->Points[*InOutPtIndex];

	// - Przestawiæ PtIndex (opcjonalnie)
	// - Przestawiæ NextFrame
	// - Zwróciæ wartoœæ

	// Jesteœmy dok³adnie w tym punkcie (lub za, co nie powinno mieæ miejsca)
	if(Equal(NextPt.x, Frame) || (Frame > NextPt.x))
	{
		(*InOutPtIndex)++;
		if(*InOutPtIndex < TmpCurve->Points.size())
			*InOutNextFrame = std::min(*InOutNextFrame, TmpCurve->Points[*InOutPtIndex].x);
		return NextPt.y;
	}
	// Jesteœmy przed pierwszym punktem - przed³u¿enie wartoœci sta³ej w lewo
	else if(*InOutPtIndex == 0)
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		return NextPt.y;
	}
	// Jesteœmy miêdzy punktem poprzednim a tym
	else
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		const Vec2 & PrevPt = TmpCurve->Points[*InOutPtIndex - 1];
		// Interpolacja liniowa wartoœci wynikowej
		float t = (Frame - PrevPt.x) / (NextPt.x - PrevPt.x);
		return Lerp(PrevPt.y, NextPt.y, t);
	}
}

// Przekszta³ca pozycje, wektory i wspó³rzêdne tekstur z uk³adu Blendera do uk³adu DirectX
// Uwzglêdnia te¿ przekszta³cenia ca³ych obiektów wprowadzaj¹c je do przekszta³ceñ poszczególnych wierzcho³ków.
// Zamienia te¿ kolejnoœæ wierzcho³ków w œciankach co by by³y odwrócone w³aœciw¹ stron¹.
// NIE zmienia danych koœci ani animacji
void Converter::TransformQmshTmpCoords(tmp::QMSH *InOut)
{
	for(uint oi = 0; oi < InOut->Meshes.size(); oi++)
	{
		tmp::MESH & o = *InOut->Meshes[oi].get();

		// Zbuduj macierz przekszta³cania tego obiektu
		Matrix Mat, MatRot;
		AssemblyBlenderObjectMatrix(&Mat, o.Position, o.Orientation, o.Size);
		MatRot = Matrix::Rotation(o.Orientation);
		// Jeœli obiekt jest sparentowany do Armature
		// To jednak nic, bo te wspó³rzêdne s¹ ju¿ wyeksportowane w uk³adzie globalnym modelu a nie wzglêdem parenta.
		if(!o.ParentArmature.empty() && !InOut->static_anim /*&& InOut->Armature != NULL*/)
		{
			assert(InOut->Armature != NULL);
			Matrix ArmatureMat, ArmatureMatRot;
			AssemblyBlenderObjectMatrix(&ArmatureMat, InOut->Armature->Position, InOut->Armature->Orientation, InOut->Armature->Size);
			ArmatureMatRot = Matrix::Rotation(InOut->Armature->Orientation);
			Mat = Mat * ArmatureMat;
			MatRot = MatRot * ArmatureMatRot;
		}

		// Przekszta³æ wierzcho³ki
		for(uint vi = 0; vi < o.Vertices.size(); vi++)
		{
			tmp::VERTEX & v = o.Vertices[vi];
			tmp::VERTEX T;
			T.Pos = Vec3::Transform(v.Pos, Mat);
			T.Normal = Vec3::Transform(v.Normal, MatRot);
			BlenderToDirectxTransform(&v.Pos, T.Pos);
			BlenderToDirectxTransform(&v.Normal, T.Normal);
		}

		// Przekszta³æ œcianki
		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			// Wsp. tekstur
			for(uint vi = 0; vi < f.NumVertices; vi++)
				f.TexCoords[vi].y = 1.0f - f.TexCoords[vi].y;

			// Zamieñ kolejnoœæ wierzcho³ków w œciance
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
}

// Szuka takiego wierzcho³ka w NvVertices.
// Jeœli nie znajdzie, dodaje nowy.
// Czy znalaz³ czy doda³, zwraca jego indeks w NvVertices.
uint Converter::TmpConvert_AddVertex(std::vector<MeshMender::Vertex>& NvVertices, std::vector<unsigned int>& MappingNvToTmpVert, uint TmpVertIndex,
	const tmp::VERTEX& TmpVertex, const Vec2& TexCoord, const Vec3& face_normal, bool vertex_normals)
{
	for(uint i = 0; i < NvVertices.size(); i++)
	{
		MeshMender::Vertex& v = NvVertices[i];
		// Pozycja identyczna...
		if(v.pos == TmpVertex.Pos &&
			// TexCoord mniej wiêcej...
			Equal(TexCoord.x, v.s) && Equal(TexCoord.y, v.t))
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

	// Kolejny algorytm pisany z wielk¹ nadziej¹ ¿e zadzia³a, wolny ale co z tego, lepszy taki
	// ni¿ ¿aden, i tak siê nad nim namyœla³em ca³y dzieñ, a drugi poprawia³em bo siê wszystko pomiesza³o!

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
	// Jest skinning ca³ej siatki do jednej koœci
	else if(!TmpMesh.ParentBone.empty())
	{
		// ZnajdŸ indeks tej koœci
		byte BoneIndex = 0;
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
			WarnOnce(13497325, "Object parented to non existing bone.");
		// Przypisz t¹ koœæ do wszystkich wierzcho³ków
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
		// U³ó¿ szybk¹ listê wierzcho³ków z wagami dla ka¿dej koœci

		// Typ reprezentuje dla danej koœci grupê wierzcho³ków: Indeks wierzcho³ka => Waga
		typedef std::map<uint, float> INFLUENCE_MAP;
		std::vector<INFLUENCE_MAP> BoneInfluences;
		BoneInfluences.resize(Qmsh.Bones.size());
		// Dla ka¿dej koœci
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// ZnajdŸ odpowiadaj¹c¹ jej grupê wierzcho³ków w przetwarzanej w tym wywo³aniu funkcji siatce
			uint VertexGroupIndex = TmpMesh.VertexGroups.size();
			for(uint gi = 0; gi < TmpMesh.VertexGroups.size(); gi++)
			{
				if(TmpMesh.VertexGroups[gi]->Name == Qmsh.Bones[bi]->Name)
				{
					VertexGroupIndex = gi;
					break;
				}
			}
			// Jeœli znaleziono, przepisz wszystkie wierzcho³ki
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

		// Dla ka¿dego wierzcho³ka
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
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

			// ¯adna koœæ na niego nie wp³ywa - wyp³yw z Envelopes
			if(VertexInfluences.empty())
			{
				// U³ó¿ listê wp³ywaj¹cych koœci
				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
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

					if(W > 0.f)
					{
						INFLUENCE Influence = { bi + 1, W };
						VertexInfluences.push_back(Influence);
					}
				}
			}
			// Jakieœ koœci na niego wp³ywaj¹ - weŸ z tych koœci

			// Zero koœci
			if(VertexInfluences.empty())
			{
				(*Out)[vi].Index1 = 0;
				(*Out)[vi].Index2 = 0;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Tylko jedna koœæ
			else if(VertexInfluences.size() == 1)
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

// Liczy kolizjê punktu z bry³¹ wyznaczon¹ przez dwie po³¹czone kule, ka¿da ma swój œrodek i promieñ
// Wygl¹ta to coœ jak na koñcach dwie pó³kule a w œrodku walec albo œciêty sto¿ek.
// Ka¿dy koniec ma dwa promienie. Zwraca wp³yw koœci na ten punkt, tzn.:
// - Jeœli punkt le¿y w promieniu promienia wewnêtrznego, zwraca 1.
// - Jeœli punkt le¿y w promieniu promienia zewnêtrznego, zwraca 1..0, im dalej tym mniej.
// - Jeœli punkt le¿y poza promieniem zewnêtrznym, zwraca 0.
float Converter::PointToBone(const Vec3 &Pt,
	const Vec3 &Center1, float InnerRadius1, float OuterRadius1,
	const Vec3 &Center2, float InnerRadius2, float OuterRadius2)
{
	float T = ClosestPointOnLine(Pt, Center1, Center2 - Center1) / Vec3::DistanceSquared(Center1, Center2);
	if(T <= 0.f)
	{
		float D = Vec3::Distance(Pt, Center1);
		if(D <= InnerRadius1)
			return 1.f;
		else if(D < OuterRadius1)
			return 1.f - (D - InnerRadius1) / (OuterRadius1 - InnerRadius1);
		else
			return 0.f;
	}
	else if(T >= 1.f)
	{
		float D = Vec3::Distance(Pt, Center2);
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
		Vec3 InterCenter = Vec3::Lerp(Center1, Center2, T);

		float D = Vec3::Distance(Pt, InterCenter);
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
					throw Format("Invalid parent bone group '%s'.", tmp_group.Parent.c_str());
			}
		}

		// verify all bones have valid bone group
		for(uint i = 0; i < qmsh.Bones.size(); ++i)
		{
			tmp::BONE& tmp_bone = *QmshTmp.Armature->Bones[i].get();
			if(tmp_bone.Group.empty())
				throw Format("Missing bone group for bone '%s'.", tmp_bone.Name.c_str());
			uint index = QmshTmp.Armature->FindBoneGroupIndex(tmp_bone.Group);
			if(index == (uint)-1)
				throw Format("Invalid parent bone group '%s' for bone '%s'.", tmp_bone.Group.c_str(), tmp_bone.Name.c_str());
			qmsh.Groups[index].bones.push_back(qmsh.Bones[i].get());
		}
	}
	else if(cs.gopt == GO_ONE)
	{
		qmsh.Groups.resize(1);
		QMSH_GROUP& gr = qmsh.Groups[0];
		gr.name = "default";
		gr.parent = 0;

		for(uint i = 0; i < qmsh.Bones.size(); ++i)
			gr.bones.push_back(&*qmsh.Bones[i]);
	}
	else
	{
		Tokenizer t(Tokenizer::F_JOIN_MINUS | Tokenizer::F_UNESCAPE);
		t.FromFile(cs.group_file.c_str());

		Info("Wczytywanie pliku konfiguracyjnego '%s'.", cs.group_file.c_str());

		try
		{
			LoadBoneGroups(qmsh, t);
		}
		catch(const Tokenizer::Exception& ex)
		{
			throw Format("B³¹d parsowania pliku konfiguracyjnego '%s'!\n%s", cs.group_file.c_str(), ex.ToString());
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
			return;

		std::sort(qmsh.Bones.begin(), qmsh.Bones.end(), [](const shared_ptr<QMSH_BONE>& b1, const shared_ptr<QMSH_BONE>& b2)
		{
			return b1->non_mesh < b2->non_mesh;
		});

		Info("Plik wczytany.");
	}
}

enum Keyword
{
	K_FILE,
	K_BONES,
	K_GROUPS,
	K_GROUP,
	K_NAME,
	K_PARENT,
	K_SPECIAL,
	K_BONE,
	K_OUTPUT
};

void Converter::LoadBoneGroups(QMSH& qmsh, Tokenizer& t)
{
	t.AddKeywords(0, {
		{ "file", K_FILE },
		{ "bones", K_BONES },
		{ "groups", K_GROUPS },
		{ "group", K_GROUP },
		{ "name", K_NAME },
		{ "parent", K_PARENT },
		{ "special", K_SPECIAL },
		{ "bone", K_BONE },
		{ "output", K_OUTPUT }
		});
	t.Next();

	// file: name
	t.AssertKeyword(K_FILE);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	t.AssertString();
	t.Next();

	// [output: name]
	if(t.IsKeyword(K_OUTPUT))
	{
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		t.AssertString();
		t.Next();
	}

	// bones: X
	t.AssertKeyword(K_BONES);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	int bone_count = t.MustGetInt();
	if(bone_count != qmsh.Bones.size())
		throw Format("Niew³aœciwa liczba koœci %d do %d!", bone_count, qmsh.Bones.size());
	t.Next();

	// groups: X
	t.AssertKeyword(K_GROUPS);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	int groups = t.MustGetInt();
	if(groups < 1 || groups > 255)
		throw Format("Niew³aœciwa liczba grup %d!", groups);
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
	for(int i = 0; i < groups; ++i)
	{
		// group: ID
		t.AssertKeyword(K_GROUP);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int id = t.MustGetInt();
		if(id < 0 || id >= groups)
			throw Format("Z³y identyfikator grupy %d!", id);
		QMSH_GROUP& gr = qmsh.Groups[id];
		if(gr.parent != (unsigned short)-1)
			throw Format("Grupa '%s' zosta³a ju¿ zdefiniowana!", gr.name.c_str());
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		// name: "NAME"
		t.AssertKeyword(K_NAME);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		gr.name = t.MustGetString();
		if(gr.name.empty())
			throw Format("Nazwa grupy %d jest pusta!", id);
		int index = 0;
		for(std::vector<QMSH_GROUP>::iterator it = qmsh.Groups.begin(), end = qmsh.Groups.end(); it != end; ++it, ++index)
		{
			if(&*it != &gr && it->name == gr.name)
				throw Format("Grupa %d nie mo¿e nazywaæ siê '%s'! Grupa %d ma ju¿ t¹ nazwê!", id, gr.name.c_str(), index);
		}
		t.Next();

		// parent: ID
		t.AssertKeyword(K_PARENT);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int pid = t.MustGetInt();
		if(pid < 0 || pid >= groups)
			throw Format("%d nie mo¿e byæ grup¹ nadrzêdn¹ dla grup '%s'!", pid, gr.name.c_str());
		if(id == pid && id != 0)
			throw Format("Tylko zerowa grupa mo¿e wskazywaæ na sam¹ siebie, grupa '%s'!", gr.name.c_str());
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
			throw Format("Grupa %s ma uszkodzon¹ hierarchiê, nie prowadzi do zerowej!", qmsh.Groups[i].name.c_str());
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
	for(int i = 0; i < bone_count; ++i)
	{
		// bone: "NAME"
		t.AssertKeyword(K_BONE);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		string bone_name = t.MustGetString();
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
			throw Format("Brak koœci o nazwie '%s'!", bone_name.c_str());
		if(bone->group_index != -1)
			throw Format("Koœæ '%s' ma ju¿ ustawion¹ grupê %d!", bone_name.c_str(), bone->group_index);
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		// group: ID
		t.AssertKeyword(K_GROUP);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int group_id = t.MustGetInt();
		if(group_id < 0 || group_id >= groups)
			throw Format("Brak grupy o id %d dla koœci '%s'!", group_id, bone_name.c_str());
		qmsh.Groups[group_id].bones.push_back(bone);
		bone->group_index = group_id;
		t.Next();

		// special: X
		t.AssertKeyword(K_SPECIAL);
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		int special = t.MustGetInt();
		if(special != 0 && special != 1)
			throw Format("Nieznana wartoœæ specjalna %d dla koœci '%s'!", special, bone_name.c_str());
		bone->non_mesh = (special == 1 ? true : false);
		t.Next();

		// };
		t.AssertSymbol('}');
		t.Next();
		t.AssertSymbol(';');
		t.Next();
	}

	// sprawdŸ czy wszystkie grupy maj¹ jakieœ koœci
	for(int i = 0; i < groups; ++i)
	{
		if(qmsh.Groups[i].bones.empty())
			throw Format("Grupa '%s' nie ma koœci!", qmsh.Groups[i].name.c_str());
	}
}

// Sk³ada translacjê, rotacje i skalowanie w macierz w uk³adzie Blendera,
// która wykonuje te operacje transformuj¹c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void Converter::AssemblyBlenderObjectMatrix(Matrix *Out, const Vec3 &Loc, const Vec3 &Rot, const Vec3 &Size)
{
	Matrix TmpMat;
	*Out = Matrix::Scale(Size);
	TmpMat = Matrix::RotationX(Rot.x);
	*Out *= TmpMat;
	TmpMat = Matrix::RotationY(Rot.y);
	*Out *= TmpMat;
	TmpMat = Matrix::RotationZ(Rot.z);
	*Out *= TmpMat;
	TmpMat = Matrix::Translation(Loc);
	*Out *= TmpMat;
}

// Przekszta³ca punkt lub wektor ze wspó³rzêdnych Blendera do DirectX
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

// Przekszta³ca macierz przekszta³caj¹c¹ punkty/wektory z takiej dzia³aj¹cej na wspó³rzêdnych Blendera
// na tak¹ dzia³aj¹c¹ na wspó³rzêdnych DirectX-a (i odwrotnie te¿).
void Converter::BlenderToDirectxTransform(Matrix *Out, const Matrix &In)
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

void Converter::BlenderToDirectxTransform(Matrix *InOut)
{
	std::swap(InOut->_12, InOut->_13);
	std::swap(InOut->_42, InOut->_43);

	std::swap(InOut->_21, InOut->_31);
	std::swap(InOut->_24, InOut->_34);

	std::swap(InOut->_22, InOut->_33);
	std::swap(InOut->_23, InOut->_32);
}

// Wylicza parametry bry³ otaczaj¹cych siatkê
void Converter::CalcBoundingVolumes(QMSH &Qmsh)
{
	Info("Calculating bounding volumes...");

	CalcBoundingVolumes(Qmsh, &Qmsh.BoundingSphereRadius, &Qmsh.BoundingBox);

	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
			CalcBoundingVolumes(Qmsh, **it);
	}
}

void Converter::CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, Box *OutBox)
{
	if(Qmsh.Vertices.empty())
	{
		*OutSphereRadius = 0.f;
		*OutBox = Box(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	else
	{
		float MaxDistSq = Qmsh.Vertices[0].Pos.LengthSquared();
		OutBox->v1 = OutBox->v2 = Qmsh.Vertices[0].Pos;

		for(uint i = 1; i < Qmsh.Vertices.size(); i++)
		{
			MaxDistSq = std::max(MaxDistSq, Qmsh.Vertices[i].Pos.LengthSquared());
			OutBox->v1 = Vec3::Min(OutBox->v1, Qmsh.Vertices[i].Pos);
			OutBox->v2 = Vec3::Max(OutBox->v2, Qmsh.Vertices[i].Pos);
		}

		*OutSphereRadius = sqrtf(MaxDistSq);
	}
}

void Converter::CalcBoundingVolumes(const QMSH& mesh, QMSH_SUBMESH& sub)
{
	float radius = 0.f;
	Vec3 center;
	float inf = std::numeric_limits<float>::infinity();
	Box box(inf, inf, inf, -inf, -inf, -inf);

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			box.AddPoint(mesh.Vertices[index].Pos);
		}
	}

	center = box.Midpoint();
	sub.box = box;

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			const Vec3 &Tmp = mesh.Vertices[index].Pos;
			float dist = Vec3::Distance(center, Tmp);
			radius = std::max(radius, dist);
		}
	}

	sub.center = center;
	sub.range = radius;
}
