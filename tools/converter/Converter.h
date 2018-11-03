#pragma once

#include "Qmsh.h"
#include "QmshTmp.h"
#include "MeshMender.h"
#include "ConversionData.h"

struct BONE_INTER_DATA;
struct INTERMEDIATE_VERTEX_SKIN_DATA;

class Converter
{
public:
	void ConvertQmshTmpToQmsh(QMSH *Out, tmp::QMSH &QmshTmp, ConversionData& cs);

private:
	void TransformQmshTmpCoords(tmp::QMSH *InOut);
	void TmpToQmsh_Bones(QMSH *Out, std::vector<BONE_INTER_DATA> *OutBoneInterData, const tmp::QMSH &QmshTmp);
	void TmpToQmsh_Bone(QMSH_BONE *Out, uint OutIndex, QMSH *OutQmsh, const tmp::BONE &TmpBone, const tmp::ARMATURE &TmpArmature,
		uint ParentIndex, const MATRIX &WorldToParent);
	void TmpToQmsh_Animations(QMSH *OutQmsh, const tmp::QMSH &QmshTmp);
	void TmpToQmsh_Animation(QMSH_ANIMATION *OutAnimation, const tmp::ACTION &TmpAction, const QMSH &Qmsh, const tmp::QMSH &QmshTmp);
	float CalcTmpCurveValue(float DefaultValue, const tmp::CURVE *TmpCurve, uint *InOutPtIndex, float Frame, float *InOutNextFrame);
	uint TmpConvert_AddVertex(std::vector<MeshMender::Vertex>& NvVertices, std::vector<unsigned int>& MappingNvToTmpVert, uint TmpVertIndex,
		const tmp::VERTEX& TmpVertex, const Vec2& TexCoord, const Vec3& face_normal, bool vertex_normals);
	void CalcVertexSkinData(std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> *Out, const tmp::MESH &TmpMesh, const QMSH &Qmsh,
		const tmp::QMSH &QmshTmp, const std::vector<BONE_INTER_DATA> &BoneInterData);
	float PointToBone(const Vec3 &Pt,
		const Vec3 &Center1, float InnerRadius1, float OuterRadius1,
		const Vec3 &Center2, float InnerRadius2, float OuterRadius2);
	void CreateBoneGroups(QMSH& qmsh, const tmp::QMSH &QmshTmp, ConversionData& cs);
	void LoadBoneGroups(QMSH& qmsh, Tokenizer& t);
	// Sk�ada translacj�, rotacje i skalowanie w macierz w uk�adzie Blendera,
	// kt�ra wykonuje te operacje transformuj�c ze wsp. lokalnych obiektu o podanych
	// parametrach do wsp. globalnych Blendera
	void AssemblyBlenderObjectMatrix(MATRIX *Out, const Vec3 &Loc, const Vec3 &Rot, const Vec3 &Size);
	// Przekszta�ca punkt lub wektor ze wsp�rz�dnych Blendera do DirectX
	void BlenderToDirectxTransform(Vec3 *Out, const Vec3 &In);
	void BlenderToDirectxTransform(Vec3 *InOut);
	// Przekszta�ca macierz przekszta�caj�c� punkty/wektory z takiej dzia�aj�cej na wsp�rz�dnych Blendera
	// na tak� dzia�aj�c� na wsp�rz�dnych DirectX-a (i odwrotnie te�).
	void BlenderToDirectxTransform(MATRIX *Out, const MATRIX &In);
	void BlenderToDirectxTransform(MATRIX *InOut);
	void CalcBoundingVolumes(QMSH &Qmsh);
	void CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, BOX *OutBox);
	void CalcBoundingVolumes(const QMSH& mesh, QMSH_SUBMESH& sub);
};
