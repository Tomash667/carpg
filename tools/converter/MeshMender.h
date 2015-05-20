#pragma once

#pragma warning( disable : 4786)
#pragma warning( disable : 4100)

/*bool operator<( const VEC3 & lhs, const VEC3 & rhs )
{
	//needed to have a vertex in a map.
	//must be an absolute sort so that we can reliably find the exact
	//position again, not a fuzzy compare for equality based on an epsilon.
    if (lhs.x == rhs.x)
    {
        if (lhs.y == rhs.y)
        {
            if (lhs.z == rhs.z)
            {
                return false;
            }
            else
            {
                return lhs.z < rhs.z;
            }
        }
        else
        {
            return lhs.y < rhs.y;
        }
    }
    else
    {
        return lhs.x < rhs.x;
    }
}*/


class MeshMender
{
public:
	
	struct Vertex
	{
		VEC3 pos;
		VEC3 normal;
		float s;
		float t;
		VEC3 tangent;
		VEC3 binormal;
		
		Vertex() : 
			pos(0.0f ,0.0f ,0.0f ),
			normal(0.0f ,0.0f ,0.0f ),
			s(0.0f ),
			t(0.0f ),
			tangent(0.0f ,0.0f ,0.0f ),
			binormal(0.0f ,0.0f ,0.0f )
		{
		}
	};

	enum NormalCalcOption{ DONT_CALCULATE_NORMALS , CALCULATE_NORMALS };
	enum ExistingSplitOption{ DONT_RESPECT_SPLITS , RESPECT_SPLITS };
	enum CylindricalFixOption{ DONT_FIX_CYLINDRICAL , FIX_CYLINDRICAL };

	//Mend - given a mesh, output the new data complete with smoothed
	//		  normals, binormals, and tangents
	//
	//RETURNS true on success, false on failure
	//
	//theVerts  - should be initialized with your mesh data, NOTE that when
	//			  mesh mender is done with it, the number of vertices may grow
	//			  and it will be filled with normals, tangents and binormals
	//
	//theIndices - should be initialized with your mesh indices
	//				will contain the new indices..we are not adding triangles, 
	//				  so the number of indices passed back should be the same as the 
	//				  number of indices passed in, but they may point to new vertices now.
	//
	//mappingNewToOldVert - this should be passed in as an empty vector. after mending
	//				it will contain a mapping of newvertexindex -> oldvertexindex
	//				so it could be used to map any per vertex data you had in your original
	//				mesh to the new mesh like so:
	//				
	//					for each new vertex index
	//						newVert[index]->myData = oldVert[ mappingNewToOldVert[index]]->myData;
	//				
	//				where myData is some custom vertex data in your original mesh.
	//
	//minNormalsCreaseCosAngle - the minimum cosine of the angle between normals
	//							 so that they are allowed to be smoothed together
	//							 ranges between -1.0 and +1.0
	//							 this is ignored if computeNormals is set to DONT_CALCULATE_NORMALS
	//							 
	//
	//minTangentsCreaseCosAngle - the minimum cosine of the angle between tangents
	//							  so that they are allowed to be smoothed together
	//							  ranges between -1.0 and +1.0
	//
	//minBinormalsCreaseCosAngle - the minimum cosine of the angle between binormals
	//							   so that they are allowed to be smoothed together
	//		 					   ranges between -1.0 and +1.0
	//
	//weightNormalsByArea - an ammount to blend the normalized face normal, and the 
	//						unnormalized face normal together.  Thus weighting the
	//						normal by the face area by a given ammount
	//						ranges between 0.0 and +1.0
	//						0.0 means use the normalized face normals (not weighted by area)
	//						1.0 means use the unnormalized face normal(weighted by area)
	//						this is ignored if computeNormals is set to DONT_CALCULATE_NORMALS
	//
	//computeNormals - should mesh mender calculate normals? If this is set to DONT_CALCULATE_NORMALS 
	//					then the vertex normals after mesh mender is called will be the 
	//					same ones you pass in.  If you are automatically calculating normals yourself,
	//					you may find that meshmender provides greater control over how normals are smoothed
	//					together. I've been able to get better results using the Crease angle with 
	//					meshmender's smoothing groups
	//
	//respectExistingSplits - DONT_RESPECT_SPLITS means that neighboring triangles for smoothing will be determined 
	//						  based on position and not on indices.
	//						  RESPECT_SPLITS means that neighboring triangles will be determined based on the indices of the
	//						  triangle and not the positions of the vertices.
	//						  you can usually get better smoothing by not respecting existing splits
	//						  only respect them if you know they should be respected.  
	//
	//fixCylindricalWrapping - DONT_FIX_CYLINDRICAL means take the texture coordinates as they come
	//						   FIX_CYLINDRICAL means we might need to split the verts
	//						   at that point and generate the proper texture coordinate.
	//						   for instance, if we have tex coords   0.9 -> 0.0-> 0.2 we would need to add
	//						   a new vert so that we have       0.9 -> 1.0  0.0-> 0.2
	//						   this is only supported for texture coordinates in the range [ 0.0f , 1.0f ]
	//						   NOTE: don't leave this on for all meshes, only use it when you know
	//							you need it. If you have polygons that map to a large area in texture space
	//							this option could mess up the texture coordinates
	bool Mend(std::vector<Vertex> & theVerts,
			  std::vector<unsigned int> & theIndices,
			  std::vector<unsigned int> & mappingNewToOldVert,
			  const float minNormalsCreaseCosAngle = 0.0f,
			  const float minTangentsCreaseCosAngle = 0.0f ,
			  const float minBinormalsCreaseCosAngle = 0.0f,
			  const float weightNormalsByArea = 1.0f,
			  const NormalCalcOption computeNormals = CALCULATE_NORMALS,
			  const ExistingSplitOption respectExistingSplits = DONT_RESPECT_SPLITS,
			  const CylindricalFixOption fixCylindricalWrapping = DONT_FIX_CYLINDRICAL);

	MeshMender();
	~MeshMender();

protected:

	float MinNormalsCreaseCosAngle; 
	float MinTangentsCreaseCosAngle;
	float MinBinormalsCreaseCosAngle;
	float WeightNormalsByArea;
	ExistingSplitOption  m_RespectExistingSplits;

	class CanSmoothChecker;
	class CanSmoothNormalsChecker;
	class CanSmoothTangentsChecker;
	class CanSmoothBinormalsChecker;
	friend class CanSmoothChecker;
	friend class CanSmoothNormalsChecker;
	friend class CanSmoothTangentsChecker;
	friend class CanSmoothBinormalsChecker;
		
	// sets up any internal data structures needed
	void SetUpData(std::vector<Vertex> & theVerts,
				   std::vector<unsigned int> & theIndices,
				   std::vector<unsigned int> & mappingNewToOldVert,
				   const NormalCalcOption computeNormals);

	typedef size_t NeighborhoodID;
	typedef size_t TriID;
	typedef std::vector<TriID> TriangleList;
	
	struct Triangle
	{
		size_t indices[3];

		// per face values
		VEC3 normal;
		VEC3 tangent;
		VEC3 binormal;

		// helper flags
		bool handled; 
		NeighborhoodID group;
		void Reset();
		
		TriID myID;	// a global id used to keep track of tris'
	};
		
	std::vector<Triangle> m_Triangles;

	//each vertex has a set of triangles that contain it.
	//those triangles are considered to be that vertex's children
	typedef std::map<VEC3, TriangleList> VertexChildrenMap;
	VertexChildrenMap m_VertexChildrenMap;

	//a neighbor group is defined to be the list of traingles
	//that all fall arround a single vertex, and can smooth with
	//eachother
	typedef std::vector<TriangleList> NeighborGroupList;

	size_t m_originalNumVerts; 

	//sets up the normal, binormal, and tangent for a triangle
	//assumes the triangle indices are set to match whats in the verts
	void SetUpFaceVectors(Triangle & t,
						  const std::vector<Vertex> & verts,
						  const NormalCalcOption computeNormals);

	//function responsible for growing the neighbor hood groups
	//arround a vertex
	void BuildGroups(Triangle * tri, //the tri of interest
					 TriangleList & possibleNeighbors, //all tris arround a vertex
					 NeighborGroupList & neighborGroups, //the neighbor groups to be updated
					 std::vector<Vertex> & theVerts,
					 CanSmoothChecker* smoothChecker,
					 const float& minCreaseAngle);

	//given 2 triangles, fill the two neighbor pointers with either
	//null or valid Triangle pointers.
	void FindNeighbors(Triangle * tri,
					   TriangleList & possibleNeighbors, 
					   Triangle ** neighbor1,
					   Triangle ** neighbor2,
					   std::vector<Vertex> & theVerts);
	
	bool SharesEdge(Triangle * triA, 
					Triangle * triB,
					std::vector<Vertex> & theVerts);
	
	bool SharesEdgeRespectSplits(Triangle * triA,
									Triangle * triB,
									std::vector<Vertex> & theVerts);

	//calculates the tangent and binormal per face
	void GetGradients(const MeshMender::Vertex & v0,
					  const MeshMender::Vertex & v1,
					  const MeshMender::Vertex & v2,
					  VEC3 & tangent,
					  VEC3 & binormal) const;

	void OrthogonalizeTangentsAndBinormals(std::vector<Vertex> & theVerts);

	void UpdateTheIndicesWithFinalIndices(std::vector<unsigned int> & theIndices);

	void FixCylindricalWrapping(std::vector<Vertex> & theVerts,
								std::vector<unsigned int> & theIndices,
								std::vector<unsigned int> & mappingNewToOldVert);

	bool TriHasEdge(const VEC3 & p0,
					const VEC3 & p1,
					const VEC3 & triA,
					const VEC3 & triB,
					const VEC3 & triC);

	bool TriHasEdge(const size_t & p0,
					const size_t & p1,
					const size_t & triA,
					const size_t & triB,
					const size_t & triC);

	void ProcessNormals(TriangleList& possibleNeighbors,
						std::vector<Vertex> & theVerts,
						std::vector<unsigned int> & mappingNewToOldVert,
						VEC3 workingPosition);
	
	void ProcessTangents(TriangleList& possibleNeighbors,
						 std::vector<Vertex> & theVerts,
						 std::vector<unsigned int> & mappingNewToOldVert,
						 VEC3 workingPosition);
	
	void ProcessBinormals(TriangleList& possibleNeighbors,
						  std::vector<Vertex> & theVerts,
						  std::vector<unsigned int> & mappingNewToOldVert,
						  VEC3 workingPosition);

	
	//make any triangle that used the oldIndex use the newIndex instead
	void UpdateIndices(const size_t oldIndex,
						const size_t newIndex,
						TriangleList& curGroup);

	//adds a new mapping entry,
	//takes into account that we may be mapping a new vertex to another new vertex,
	//and uses the original old vertex index....is that confusing?
	void AppendToMapping(const size_t oldIndex,
						 const size_t originalNumVerts,
						 std::vector<unsigned int> & mappingNewToOldVert);

};
