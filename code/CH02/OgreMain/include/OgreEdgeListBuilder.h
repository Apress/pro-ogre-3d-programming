/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
-----------------------------------------------------------------------------
*/
#ifndef __EdgeListBuilder_H__
#define __EdgeListBuilder_H__

#include "OgrePrerequisites.h"
#include "OgreVector4.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreRenderOperation.h"

namespace Ogre {


    /** This class contains the information required to describe the edge connectivity of a
        given set of vertices and indexes. 
    @remarks 
        This information is built using the EdgeListBuilder class. Note that for a given mesh,
        which can be made up of multiple submeshes, there are separate edge lists for when 
    */
    class _OgreExport EdgeData
    {
    public:
        /** Basic triangle structure. */
        struct Triangle {
            /** The set of indexes this triangle came from (NB it is possible that the triangles on 
               one side of an edge are using a different vertex buffer from those on the other side.) */
            size_t indexSet; 
            /** The vertex set these vertices came from. */
            size_t vertexSet;
            size_t vertIndex[3];/// Vertex indexes, relative to the original buffer
            size_t sharedVertIndex[3]; /// Vertex indexes, relative to a shared vertex buffer with 
                                        // duplicates eliminated (this buffer is not exposed)
	        Vector4 normal;   // unit vector othogonal to this face, plus distance from origin
            bool lightFacing; // Working vector used when calculating the silhouette
        };
        /** Edge data. */
        struct Edge {
            /** The indexes of the 2 tris attached, note that tri 0 is the one where the 
                indexes run _anti_ clockwise along the edge. Indexes must be
                reversed for tri 1. */
            size_t triIndex[2];
            /** The vertex indices for this edge. Note that both vertices will be in the vertex
                set as specified in 'vertexSet', which will also be the same as tri 0 */
            size_t vertIndex[2];
            /** Vertex indices as used in the shared vertex list, not exposed. */
            size_t sharedVertIndex[2];
            /** Indicates if this is a degenerate edge, ie it does not have 2 triangles */
            bool degenerate;
        };

        typedef std::vector<Triangle> TriangleList;
        typedef std::vector<Edge> EdgeList;

        /** A group of edges sharing the same vertex data. */
        struct EdgeGroup
        {
            /** The vertex set index that contains the vertices for this edge group. */
            size_t vertexSet;
            /** Pointer to vertex data used by this edge group. */
            const VertexData* vertexData;
            /** The edges themselves. */
            EdgeList edges;

        };

        typedef std::vector<EdgeGroup> EdgeGroupList;
        TriangleList triangles;
        EdgeGroupList edgeGroups;
		// manifold? NB This value is not stored in the  binary Mesh format yet so
		// cannot be relied upon unless this has been calculated interactively.
		//bool isClosed; // manifold?


        /** Calculate the light facing state of the triangles in this edge list
        @remarks
            This is normally the first stage of calculating a silhouette, ie
            establishing which tris are facing the light and which are facing
            away. This state is stored in the 'lightFacing' flag in each 
            Triangle.
        @param lightPos 4D position of the light in object space, note that 
            for directional lights (which have no position), the w component
            is 0 and the x/y/z position are the direction.
        */
        void updateTriangleLightFacing(const Vector4& lightPos);
        /** Updates the face normals for this edge list based on (changed)
            position information, useful for animated objects. 
        @param vertexSet The vertex set we are updating
        @param positionBuffer The updated position buffer, must contain ONLY xyz
        */
        void updateFaceNormals(size_t vertexSet, HardwareVertexBufferSharedPtr positionBuffer);



        // Debugging method
        void log(Log* log);
        
    };

    /** General utility class for building edge lists for geometry.
    @remarks
        You can add multiple sets of vertex and index data to build and edge list. 
        Edges will be built between the various sets as well as within sets; this allows 
        you to use a model which is built from multiple SubMeshes each using 
        separate index and (optionally) vertex data and still get the same connectivity 
        information. It's important to note that the indexes for the edge will be constrained
        to a single vertex buffer though (this is required in order to render the edge).
    */
    class _OgreExport EdgeListBuilder 
    {
    public:

        EdgeListBuilder();
        virtual ~EdgeListBuilder();
        /** Add a set of vertex geometry data to the edge builder. 
        @remarks
            You must add at least one set of vertex data to the builder before invoking the
            build method.
        */
        void addVertexData(const VertexData* vertexData);
        /** Add a set of index geometry data to the edge builder. 
        @remarks
            You must add at least one set of index data to the builder before invoking the
            build method.
        @param indexData The index information which describes the triangles.
        @param vertexSet The vertex data set this index data refers to; you only need to alter this
            if you have added multiple sets of vertices
        @param opType The operation type used to render these indexes. Only triangle types
            are supported (no point or line types)
        */
        void addIndexData(const IndexData* indexData, size_t vertexSet = 0, 
            RenderOperation::OperationType opType = RenderOperation::OT_TRIANGLE_LIST);

        /** Builds the edge information based on the information built up so far.
        @remarks
            The caller takes responsibility for deleting the returned structure.
        */
        EdgeData* build(void);

        /// Debugging method
        void log(Log* l);
    protected:

        /** A vertex can actually represent several vertices in the final model, because
		vertices along texture seams etc will have been duplicated. In order to properly
		evaluate the surface properties, a single common vertex is used for these duplicates,
		and the faces hold the detail of the duplicated vertices.
		*/
        struct CommonVertex {
            Vector3  position;  // location of point in euclidean space
	        size_t index;       // place of vertex in common vertex list
            size_t vertexSet;   // The vertex set this came from
            size_t indexSet;    // The index set this was referenced (first) from
            size_t originalIndex; // place of vertex in original vertex set
        };
        /** A set of indexed geometry data */
        struct Geometry {
            size_t vertexSet;           // The vertex data set this geometry data refers to
            size_t indexSet;            // The index data set this geometry data refers to
            const IndexData* indexData; // The index information which describes the triangles.
            RenderOperation::OperationType opType;  // The operation type used to render this geometry
        };
        /** Comparator for sorting geometries by vertex set */
        struct geometryLess {
            bool operator()(const Geometry& a, const Geometry& b) const
            {
                if (a.vertexSet < b.vertexSet) return true;
                if (a.vertexSet > b.vertexSet) return false;
                return a.indexSet < b.indexSet;
            }
        };
        /** Comparator for unique vertex list */
        struct vectorLess {
            bool operator()(const Vector3& a, const Vector3& b) const
            {
                if (a.x < b.x) return true;
                if (a.x > b.x) return false;
                if (a.y < b.y) return true;
                if (a.y > b.y) return false;
                return a.z < b.z;
            }
        };

        typedef std::vector<const VertexData*> VertexDataList;
        typedef std::vector<Geometry> GeometryList;
        typedef std::vector<CommonVertex> CommonVertexList;

        GeometryList mGeometryList;
        VertexDataList mVertexDataList;
        CommonVertexList mVertices;
        EdgeData* mEdgeData;
		/// Map for identifying common vertices
		typedef std::map<Vector3, size_t, vectorLess> CommonVertexMap;
		CommonVertexMap mCommonVertexMap;
        /** Edge map, used to connect edges. Note we allow many triangles on an edge,
        after connected an existing edge, we will remove it and never used again.
        */
        typedef std::multimap< std::pair<size_t, size_t>, std::pair<size_t, size_t> > EdgeMap;
        EdgeMap mEdgeMap;

        void buildTrianglesEdges(const Geometry &geometry);

        /// Finds an existing common vertex, or inserts a new one
        size_t findOrCreateCommonVertex(const Vector3& vec, size_t vertexSet, 
            size_t indexSet, size_t originalIndex);
        /// Connect existing edge or create a new edge - utility method during building
        void connectOrCreateEdge(size_t vertexSet, size_t triangleIndex, size_t vertIndex0, size_t vertIndex1, 
            size_t sharedVertIndex0, size_t sharedVertIndex1);
    };

}
#endif

