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
#include "OgreStableHeaders.h"

#include "OgreMeshManager.h"

#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreMatrix4.h"
#include "OgreMatrix3.h"
#include "OgreVector3.h"
#include "OgrePlane.h"
#include "OgreHardwareBufferManager.h"
#include "OgrePatchSurface.h"
#include "OgreException.h"

namespace Ogre
{
	#define PI 3.1415926535897932384626433832795

    //-----------------------------------------------------------------------
    template<> MeshManager* Singleton<MeshManager>::ms_Singleton = 0;
    MeshManager* MeshManager::getSingletonPtr(void)
    {
        return ms_Singleton;
    }
    MeshManager& MeshManager::getSingleton(void)
    {  
        assert( ms_Singleton );  return ( *ms_Singleton );  
    }
    //-----------------------------------------------------------------------
    MeshManager::MeshManager():
    mBoundsPaddingFactor(0.01)
    {
        mPrepAllMeshesForShadowVolumes = false;

        mLoadOrder = 350.0f;
        mResourceType = "Mesh";

        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);

    }
    //-----------------------------------------------------------------------
    MeshManager::~MeshManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }
    //-----------------------------------------------------------------------
    void MeshManager::_initialise(void)
    {
        // Create prefab objects
        createPrefabPlane();


    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::load( const String& filename, const String& groupName, 
		HardwareBuffer::Usage vertexBufferUsage, 
		HardwareBuffer::Usage indexBufferUsage, 
		bool vertexBufferShadowed, bool indexBufferShadowed)
    {
        MeshPtr pMesh = getByName(filename);
        if (pMesh.isNull())
        {
            pMesh = this->create(filename, groupName);
			pMesh->setVertexBufferPolicy(vertexBufferUsage, vertexBufferShadowed);
			pMesh->setIndexBufferPolicy(indexBufferUsage, indexBufferShadowed);
        }
        pMesh->load();
        return pMesh;

    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::createManual( const String& name, const String& groupName, 
        ManualResourceLoader* loader)
    {
        MeshPtr pMesh = getByName(name);
        if (pMesh.isNull())
        {
            pMesh = create(name, groupName, true, loader);
        }

        return pMesh;
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::createPlane( const String& name, const String& groupName,
        const Plane& plane, Real width, Real height, int xsegments, int ysegments,
        bool normals, int numTexCoordSets, Real xTile, Real yTile, const Vector3& upVector,
		HardwareBuffer::Usage vertexBufferUsage, HardwareBuffer::Usage indexBufferUsage,
		bool vertexShadowBuffer, bool indexShadowBuffer)
    {
        // Create manual mesh which calls back self to load
        MeshPtr pMesh = createManual(name, groupName, this);
		// Planes can never be manifold
		pMesh->setAutoBuildEdgeLists(false);
        // store parameters
        MeshBuildParams params;
        params.type = MBT_PLANE;
        params.plane = plane;
        params.width = width;
        params.height = height;
        params.xsegments = xsegments;
        params.ysegments = ysegments;
        params.normals = normals;
        params.numTexCoordSets = numTexCoordSets;
        params.xTile = xTile;
        params.yTile = yTile;
        params.upVector = upVector;
        params.vertexBufferUsage = vertexBufferUsage;
        params.indexBufferUsage = indexBufferUsage;
        params.vertexShadowBuffer = vertexShadowBuffer;
        params.indexShadowBuffer = indexShadowBuffer;
        mMeshBuildParams[pMesh.getPointer()] = params;

        // to preserve previous behaviour, load immediately
        pMesh->load();

        return pMesh;
    }
	
	//-----------------------------------------------------------------------
	MeshPtr MeshManager::createCurvedPlane( const String& name, const String& groupName, 
        const Plane& plane, Real width, Real height, Real bow, int xsegments, int ysegments,
        bool normals, int numTexCoordSets, Real xTile, Real yTile, const Vector3& upVector,
			HardwareBuffer::Usage vertexBufferUsage, HardwareBuffer::Usage indexBufferUsage,
			bool vertexShadowBuffer, bool indexShadowBuffer)
    {
        // Create manual mesh which calls back self to load
        MeshPtr pMesh = createManual(name, groupName, this);
		// Planes can never be manifold
		pMesh->setAutoBuildEdgeLists(false);
        // store parameters
        MeshBuildParams params;
        params.type = MBT_CURVED_PLANE;
        params.plane = plane;
        params.width = width;
        params.height = height;
        params.curvature = bow;
        params.xsegments = xsegments;
        params.ysegments = ysegments;
        params.normals = normals;
        params.numTexCoordSets = numTexCoordSets;
        params.xTile = xTile;
        params.yTile = yTile;
        params.upVector = upVector;
        params.vertexBufferUsage = vertexBufferUsage;
        params.indexBufferUsage = indexBufferUsage;
        params.vertexShadowBuffer = vertexShadowBuffer;
        params.indexShadowBuffer = indexShadowBuffer;
        mMeshBuildParams[pMesh.getPointer()] = params;

        // to preserve previous behaviour, load immediately
        pMesh->load();

        return pMesh;

    }
    //-----------------------------------------------------------------------
	MeshPtr MeshManager::createCurvedIllusionPlane(
        const String& name, const String& groupName, const Plane& plane,
        Real width, Real height, Real curvature,
        int xsegments, int ysegments,
        bool normals, int numTexCoordSets,
        Real uTile, Real vTile, const Vector3& upVector,
		const Quaternion& orientation, 
        HardwareBuffer::Usage vertexBufferUsage, 
		HardwareBuffer::Usage indexBufferUsage,
		bool vertexShadowBuffer, bool indexShadowBuffer,
        int ySegmentsToKeep)
	{
        // Create manual mesh which calls back self to load
        MeshPtr pMesh = createManual(name, groupName, this);
		// Planes can never be manifold
		pMesh->setAutoBuildEdgeLists(false);
        // store parameters
        MeshBuildParams params;
        params.type = MBT_CURVED_ILLUSION_PLANE;
        params.plane = plane;
        params.width = width;
        params.height = height;
        params.curvature = curvature;
        params.xsegments = xsegments;
        params.ysegments = ysegments;
        params.normals = normals;
        params.numTexCoordSets = numTexCoordSets;
        params.xTile = uTile;
        params.yTile = vTile;
        params.upVector = upVector;
        params.orientation = orientation;
        params.vertexBufferUsage = vertexBufferUsage;
        params.indexBufferUsage = indexBufferUsage;
        params.vertexShadowBuffer = vertexShadowBuffer;
        params.indexShadowBuffer = indexShadowBuffer;
        params.ySegmentsToKeep = ySegmentsToKeep;
        mMeshBuildParams[pMesh.getPointer()] = params;

        // to preserve previous behaviour, load immediately
        pMesh->load();

        return pMesh;
	}

    //-----------------------------------------------------------------------
    void MeshManager::tesselate2DMesh(SubMesh* sm, int meshWidth, int meshHeight, 
		bool doubleSided, HardwareBuffer::Usage indexBufferUsage, bool indexShadowBuffer)
    {
        // The mesh is built, just make a list of indexes to spit out the triangles
        int vInc, uInc, v, u, iterations;
        int vCount, uCount;

        if (doubleSided)
        {
            iterations = 2;
            vInc = 1;
            v = 0; // Start with front
        }
        else
        {
            iterations = 1;
            vInc = 1;
            v = 0;
        }

        // Allocate memory for faces
        // Num faces, width*height*2 (2 tris per square), index count is * 3 on top
        sm->indexData->indexCount = (meshWidth-1) * (meshHeight-1) * 2 * iterations * 3;
		sm->indexData->indexBuffer = HardwareBufferManager::getSingleton().
			createIndexBuffer(HardwareIndexBuffer::IT_16BIT,
			sm->indexData->indexCount, indexBufferUsage, indexShadowBuffer);

        int v1, v2, v3;
        //bool firstTri = true;
		HardwareIndexBufferSharedPtr ibuf = sm->indexData->indexBuffer;
		// Lock the whole buffer
		unsigned short* pIndexes = static_cast<unsigned short*>(
			ibuf->lock(HardwareBuffer::HBL_DISCARD) );

        while (iterations--)
        {
            // Make tris in a zigzag pattern (compatible with strips)
            u = 0;
            uInc = 1; // Start with moving +u

            vCount = meshHeight - 1;
            while (vCount--)
            {
                uCount = meshWidth - 1;
                while (uCount--)
                {
                    // First Tri in cell
                    // -----------------
                    v1 = ((v + vInc) * meshWidth) + u;
                    v2 = (v * meshWidth) + u;
                    v3 = ((v + vInc) * meshWidth) + (u + uInc);
                    // Output indexes
                    *pIndexes++ = v1;
                    *pIndexes++ = v2;
                    *pIndexes++ = v3;
                    // Second Tri in cell
                    // ------------------
                    v1 = ((v + vInc) * meshWidth) + (u + uInc);
                    v2 = (v * meshWidth) + u;
                    v3 = (v * meshWidth) + (u + uInc);
                    // Output indexes
                    *pIndexes++ = v1;
                    *pIndexes++ = v2;
                    *pIndexes++ = v3;

                    // Next column
                    u += uInc;
                }
                // Next row
                v += vInc;
                u = 0;


            }

            // Reverse vInc for double sided
            v = meshHeight - 1;
            vInc = -vInc;

        }
		// Unlock
		ibuf->unlock();

    }

    //-----------------------------------------------------------------------
    void MeshManager::createPrefabPlane(void)
    {
        MeshPtr msh = create(
            "Prefab_Plane", 
            ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, 
            true, // manually loaded
            this);
		// Planes can never be manifold
		msh->setAutoBuildEdgeLists(false);
        // to preserve previous behaviour, load immediately
        msh->load();
    }
    //-----------------------------------------------------------------------
    void MeshManager::loadResource(Resource* res)
    {
        Mesh* msh = static_cast<Mesh*>(res);
        // Manual resource load
        if (res->getName() == "Prefab_Plane")
        {
            SubMesh* sub = msh->createSubMesh();
            float vertices[32] = {
			    -100, -100, 0,	// pos
			    0,0,1,			// normal
			    0,1,			// texcoord
                100, -100, 0,
                0,0,1,
                1,1,
                100,  100, 0,
                0,0,1,
                1,0,
                -100,  100, 0 ,
			    0,0,1,
                0,0 
		    };
            msh->sharedVertexData = new VertexData();
            msh->sharedVertexData->vertexCount = 4;
		    VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
		    VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;

		    size_t offset = 0;
		    decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
		    offset += VertexElement::getTypeSize(VET_FLOAT3);
		    decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
		    offset += VertexElement::getTypeSize(VET_FLOAT3);
		    decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
		    offset += VertexElement::getTypeSize(VET_FLOAT2);

		    HardwareVertexBufferSharedPtr vbuf = 
			    HardwareBufferManager::getSingleton().createVertexBuffer(
				    offset, 4, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		    bind->setBinding(0, vbuf);

		    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

		    sub->useSharedVertices = true;
		    HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
			    createIndexBuffer(
				    HardwareIndexBuffer::IT_16BIT, 
				    6, 
				    HardwareBuffer::HBU_STATIC_WRITE_ONLY);

            unsigned short faces[6] = {0,1,2,
                                    0,2,3 };
            sub->indexData->indexBuffer = ibuf;
		    sub->indexData->indexCount = 6;
		    sub->indexData->indexStart =0;
            ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

            msh->_setBounds(AxisAlignedBox(-100,-100,0,100,100,0), true);
            msh->_setBoundingSphereRadius(Math::Sqrt(100*100+100*100));
        }
        else
        {
            // Find build parameters
            MeshBuildParamsMap::iterator ibld = mMeshBuildParams.find(res);
            if (ibld == mMeshBuildParams.end())
            {
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                    "Cannot find build parameters for " + res->getName(),
                    "MeshManager::loadResource");
            }
            MeshBuildParams& params = ibld->second;

            switch(params.type)
            {
            case MBT_PLANE:
                loadManualPlane(msh, params);
                break;
            case MBT_CURVED_ILLUSION_PLANE:
                loadManualCurvedIllusionPlane(msh, params);
                break;
            case MBT_CURVED_PLANE:
                loadManualCurvedPlane(msh, params);
                break;
            default:
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                    "Unknown build parameters for " + res->getName(),
                    "MeshManager::loadResource");
            }
        }

    }
    //-----------------------------------------------------------------------
    void MeshManager::loadManualPlane(Mesh* pMesh, MeshBuildParams& params)
    {
        int i;

        SubMesh *pSub = pMesh->createSubMesh();

        // Set up vertex data
        // Use a single shared buffer
        pMesh->sharedVertexData = new VertexData();
        VertexData* vertexData = pMesh->sharedVertexData;
        // Set up Vertex Declaration
        VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
        size_t currOffset = 0;
        // We always need positions
        vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_POSITION);
        currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        // Optional normals
        if(params.normals)
        {
            vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_NORMAL);
            currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        }

        for (i = 0; i < params.numTexCoordSets; ++i)
        {
            // Assumes 2D texture coords
            vertexDecl->addElement(0, currOffset, VET_FLOAT2, VES_TEXTURE_COORDINATES, i);
            currOffset += VertexElement::getTypeSize(VET_FLOAT2);
        }

        vertexData->vertexCount = (params.xsegments + 1) * (params.ysegments + 1);

        // Allocate vertex buffer
        HardwareVertexBufferSharedPtr vbuf = 
            HardwareBufferManager::getSingleton().
            createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount,
            params.vertexBufferUsage, params.vertexShadowBuffer);

        // Set up the binding (one source only)
        VertexBufferBinding* binding = vertexData->vertexBufferBinding;
        binding->setBinding(0, vbuf);

        // Work out the transform required
        // Default orientation of plane is normal along +z, distance 0
        Matrix4 xlate, xform, rot;
        Matrix3 rot3;
        xlate = rot = Matrix4::IDENTITY;
        // Determine axes
        Vector3 zAxis, yAxis, xAxis;
        zAxis = params.plane.normal;
        zAxis.normalise();
        yAxis = params.upVector;
        yAxis.normalise();
        xAxis = yAxis.crossProduct(zAxis);
        if (xAxis.length() == 0)
        {
            //upVector must be wrong
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "The upVector you supplied is parallel to the plane normal, so is not valid.",
                "MeshManager::createPlane");
        }

        rot3.FromAxes(xAxis, yAxis, zAxis);
        rot = rot3;

        // Set up standard xform from origin
        xlate.setTrans(params.plane.normal * -params.plane.d);

        // concatenate
        xform = xlate * rot;

        // Generate vertex data
        // Lock the whole buffer
        float* pReal = static_cast<float*>(
            vbuf->lock(HardwareBuffer::HBL_DISCARD) );
        Real xSpace = params.width / params.xsegments;
        Real ySpace = params.height / params.ysegments;
        Real halfWidth = params.width / 2;
        Real halfHeight = params.height / 2;
        Real xTex = (1.0f * params.xTile) / params.xsegments;
        Real yTex = (1.0f * params.yTile) / params.ysegments;
        Vector3 vec;
        Vector3 min, max;
        Real maxSquaredLength;
        bool firstTime = true;

        for (int y = 0; y < params.ysegments + 1; ++y)
        {
            for (int x = 0; x < params.xsegments + 1; ++x)
            {
                // Work out centered on origin
                vec.x = (x * xSpace) - halfWidth;
                vec.y = (y * ySpace) - halfHeight;
                vec.z = 0.0f;
                // Transform by orientation and distance
                vec = xform * vec;
                // Assign to geometry
                *pReal++ = vec.x;
                *pReal++ = vec.y;
                *pReal++ = vec.z;

                // Build bounds as we go
                if (firstTime)
                {
                    min = vec;
                    max = vec;
                    maxSquaredLength = vec.squaredLength();
                    firstTime = false;
                }
                else
                {
                    min.makeFloor(vec);
                    max.makeCeil(vec);
                    maxSquaredLength = std::max(maxSquaredLength, vec.squaredLength());
                }

                if (params.normals)
                {
                    // Default normal is along unit Z
                    vec = Vector3::UNIT_Z;
                    // Rotate
                    vec = rot * vec;

                    *pReal++ = vec.x;
                    *pReal++ = vec.y;
                    *pReal++ = vec.z;
                }

                for (i = 0; i < params.numTexCoordSets; ++i)
                {
                    *pReal++ = x * xTex;
                    *pReal++ = 1 - (y * yTex);
                }


            } // x
        } // y

        // Unlock
        vbuf->unlock();
        // Generate face list
        pSub->useSharedVertices = true;
        tesselate2DMesh(pSub, params.xsegments + 1, params.ysegments + 1, false, 
            params.indexBufferUsage, params.indexShadowBuffer);

        pMesh->_setBounds(AxisAlignedBox(min, max), true);
        pMesh->_setBoundingSphereRadius(Math::Sqrt(maxSquaredLength));
    }
    //-----------------------------------------------------------------------
    void MeshManager::loadManualCurvedPlane(Mesh* pMesh, MeshBuildParams& params)
    {
        int i;
        SubMesh *pSub = pMesh->createSubMesh();

        // Set options
        pMesh->sharedVertexData = new VertexData();
        pMesh->sharedVertexData->vertexStart = 0;
        VertexBufferBinding* bind = pMesh->sharedVertexData->vertexBufferBinding;
        VertexDeclaration* decl = pMesh->sharedVertexData->vertexDeclaration;

        pMesh->sharedVertexData->vertexCount = (params.xsegments + 1) * (params.ysegments + 1);

        size_t offset = 0;
        decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
        offset += VertexElement::getTypeSize(VET_FLOAT3);
        if (params.normals)
        {
            decl->addElement(0, 0, VET_FLOAT3, VES_NORMAL);
            offset += VertexElement::getTypeSize(VET_FLOAT3);
        }

        for (i = 0; i < params.numTexCoordSets; ++i)
        {
            decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, i);
            offset += VertexElement::getTypeSize(VET_FLOAT2);
        }


        // Allocate memory
        HardwareVertexBufferSharedPtr vbuf = 
            HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, 
            pMesh->sharedVertexData->vertexCount, 
            params.vertexBufferUsage, 
            params.vertexShadowBuffer);
        bind->setBinding(0, vbuf);

        // Work out the transform required
        // Default orientation of plane is normal along +z, distance 0
        Matrix4 xlate, xform, rot;
        Matrix3 rot3;
        xlate = rot = Matrix4::IDENTITY;
        // Determine axes
        Vector3 zAxis, yAxis, xAxis;
        zAxis = params.plane.normal;
        zAxis.normalise();
        yAxis = params.upVector;
        yAxis.normalise();
        xAxis = yAxis.crossProduct(zAxis);
        if (xAxis.length() == 0)
        {
            //upVector must be wrong
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "The upVector you supplied is parallel to the plane normal, so is not valid.",
                "MeshManager::createPlane");
        }

        rot3.FromAxes(xAxis, yAxis, zAxis);
        rot = rot3;

        // Set up standard xform from origin
        xlate.setTrans(params.plane.normal * -params.plane.d);

        // concatenate
        xform = xlate * rot;

        // Generate vertex data
        float* pFloat = static_cast<float*>(
            vbuf->lock(HardwareBuffer::HBL_DISCARD)); 
        Real xSpace = params.width / params.xsegments;
        Real ySpace = params.height / params.ysegments;
        Real halfWidth = params.width / 2;
        Real halfHeight = params.height / 2;
        Real xTex = (1.0f * params.xTile) / params.xsegments;
        Real yTex = (1.0f * params.yTile) / params.ysegments;
        Vector3 vec;

        Vector3 min, max;
        Real maxSqLen;
        bool first = true;

        Real diff_x, diff_y, dist;

        for (int y = 0; y < params.ysegments + 1; ++y)
        {
            for (int x = 0; x < params.xsegments + 1; ++x)
            {
                // Work out centered on origin
                vec.x = (x * xSpace) - halfWidth;
                vec.y = (y * ySpace) - halfHeight;

                // Here's where curved plane is different from standard plane.  Amazing, I know.
                diff_x = (x - ((params.xsegments) / 2)) / static_cast<Real>((params.xsegments));
                diff_y = (y - ((params.ysegments) / 2)) / static_cast<Real>((params.ysegments));
                dist = sqrt(diff_x*diff_x + diff_y * diff_y );
                vec.z = (-sin((1-dist) * (PI/2)) * params.curvature) + params.curvature;

                // Transform by orientation and distance
                Vector3 pos = xform * vec;
                // Assign to geometry
                *pFloat++ = pos.x;
                *pFloat++ = pos.y;
                *pFloat++ = pos.z;

                // Record bounds
                if (first)
                {
                    min = max = vec;
                    maxSqLen = vec.squaredLength();
                    first = false;
                }
                else
                {
                    min.makeFloor(vec);
                    max.makeCeil(vec);
                    maxSqLen = std::max(maxSqLen, vec.squaredLength());
                }

                if (params.normals)
                {
                    // This part is kinda 'wrong' for curved planes... but curved planes are
                    //   very valuable outside sky planes, which don't typically need normals
                    //   so I'm not going to mess with it for now. 

                    // Default normal is along unit Z
                    //vec = Vector3::UNIT_Z;
                    // Rotate
                    vec = rot * vec;
					vec.normalise();

                    *pFloat++ = vec.x;
                    *pFloat++ = vec.y;
                    *pFloat++ = vec.z;
                }

                for (i = 0; i < params.numTexCoordSets; ++i)
                {
                    *pFloat++ = x * xTex;
                    *pFloat++ = 1 - (y * yTex);
                }

            } // x
        } // y
        vbuf->unlock();

        // Generate face list
        tesselate2DMesh(pSub, params.xsegments + 1, params.ysegments + 1, 
            false, params.indexBufferUsage, params.indexShadowBuffer);

        pMesh->_setBounds(AxisAlignedBox(min, max), true);
        pMesh->_setBoundingSphereRadius(Math::Sqrt(maxSqLen));

    }
    //-----------------------------------------------------------------------
    void MeshManager::loadManualCurvedIllusionPlane(Mesh* pMesh, MeshBuildParams& params)
    {
        int i;
        SubMesh *pSub = pMesh->createSubMesh();

        if (params.ySegmentsToKeep == -1) params.ySegmentsToKeep = params.ysegments;

        // Set up vertex data
        // Use a single shared buffer
        pMesh->sharedVertexData = new VertexData();
        VertexData* vertexData = pMesh->sharedVertexData;
        // Set up Vertex Declaration
        VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
        size_t currOffset = 0;
        // We always need positions
        vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_POSITION);
        currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        // Optional normals
        if(params.normals)
        {
            vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_NORMAL);
            currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        }

        for (i = 0; i < params.numTexCoordSets; ++i)
        {
            // Assumes 2D texture coords
            vertexDecl->addElement(0, currOffset, VET_FLOAT2, VES_TEXTURE_COORDINATES, i);
            currOffset += VertexElement::getTypeSize(VET_FLOAT2);
        }

        vertexData->vertexCount = (params.xsegments + 1) * (params.ySegmentsToKeep + 1);

        // Allocate vertex buffer
        HardwareVertexBufferSharedPtr vbuf = 
            HardwareBufferManager::getSingleton().
            createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount,
            params.vertexBufferUsage, params.vertexShadowBuffer);

        // Set up the binding (one source only)
        VertexBufferBinding* binding = vertexData->vertexBufferBinding;
        binding->setBinding(0, vbuf);

        // Work out the transform required
        // Default orientation of plane is normal along +z, distance 0
        Matrix4 xlate, xform, rot;
        Matrix3 rot3;
        xlate = rot = Matrix4::IDENTITY;
        // Determine axes
        Vector3 zAxis, yAxis, xAxis;
        zAxis = params.plane.normal;
        zAxis.normalise();
        yAxis = params.upVector;
        yAxis.normalise();
        xAxis = yAxis.crossProduct(zAxis);
        if (xAxis.length() == 0)
        {
            //upVector must be wrong
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "The upVector you supplied is parallel to the plane normal, so is not valid.",
                "MeshManager::createPlane");
        }

        rot3.FromAxes(xAxis, yAxis, zAxis);
        rot = rot3;

        // Set up standard xform from origin
        xlate.setTrans(params.plane.normal * -params.plane.d);

        // concatenate
        xform = xlate * rot;

        // Generate vertex data
        // Imagine a large sphere with the camera located near the top
        // The lower the curvature, the larger the sphere
        // Use the angle from viewer to the points on the plane
        // Credit to Aftershock for the general approach
        Real camPos;      // Camera position relative to sphere center

        // Derive sphere radius
        Vector3 vertPos;  // position relative to camera
        Real sphDist;      // Distance from camera to sphere along box vertex vector
        // Vector3 camToSph; // camera position to sphere
        Real sphereRadius;// Sphere radius
        // Actual values irrelevant, it's the relation between sphere radius and camera position that's important
        const Real SPHERE_RAD = 100.0;
        const Real CAM_DIST = 5.0;

        sphereRadius = SPHERE_RAD - params.curvature;
        camPos = sphereRadius - CAM_DIST;

        // Lock the whole buffer
        float* pFloat = static_cast<float*>(
            vbuf->lock(HardwareBuffer::HBL_DISCARD) );
        Real xSpace = params.width / params.xsegments;
        Real ySpace = params.height / params.ysegments;
        Real halfWidth = params.width / 2;
        Real halfHeight = params.height / 2;
        Vector3 vec, norm;
        Vector3 min, max;
        Real maxSquaredLength;
        bool firstTime = true;

        for (int y = params.ysegments - params.ySegmentsToKeep; y < params.ysegments + 1; ++y)
        {
            for (int x = 0; x < params.xsegments + 1; ++x)
            {
                // Work out centered on origin
                vec.x = (x * xSpace) - halfWidth;
                vec.y = (y * ySpace) - halfHeight;
                vec.z = 0.0f;
                // Transform by orientation and distance
                vec = xform * vec;
                // Assign to geometry
                *pFloat++ = vec.x;
                *pFloat++ = vec.y;
                *pFloat++ = vec.z;

                // Build bounds as we go
                if (firstTime)
                {
                    min = vec;
                    max = vec;
                    maxSquaredLength = vec.squaredLength();
                    firstTime = false;
                }
                else
                {
                    min.makeFloor(vec);
                    max.makeCeil(vec);
                    maxSquaredLength = std::max(maxSquaredLength, vec.squaredLength());
                }

                if (params.normals)
                {
                    // Default normal is along unit Z
                    norm = Vector3::UNIT_Z;
                    // Rotate
                    norm = params.orientation * norm;

                    *pFloat++ = norm.x;
                    *pFloat++ = norm.y;
                    *pFloat++ = norm.z;
                }

                // Generate texture coords
                // Normalise position
                // modify by orientation to return +y up
                vec = params.orientation.Inverse() * vec;
                vec.normalise();
                // Find distance to sphere
                sphDist = Math::Sqrt(camPos*camPos * (vec.y*vec.y-1.0) + sphereRadius*sphereRadius) - camPos*vec.y;

                vec.x *= sphDist;
                vec.z *= sphDist;

                // Use x and y on sphere as texture coordinates, tiled
                Real s = vec.x * (0.01 * params.xTile);
                Real t = 1 - (vec.z * (0.01 * params.yTile));
                for (i = 0; i < params.numTexCoordSets; ++i)
                {
                    *pFloat++ = s;
                    *pFloat++ = t;
                }


            } // x
        } // y

        // Unlock
        vbuf->unlock();
        // Generate face list
        pSub->useSharedVertices = true;
        tesselate2DMesh(pSub, params.xsegments + 1, params.ySegmentsToKeep + 1, false, 
            params.indexBufferUsage, params.indexShadowBuffer);

        pMesh->_setBounds(AxisAlignedBox(min, max), true);
        pMesh->_setBoundingSphereRadius(Math::Sqrt(maxSquaredLength));
    }
    //-----------------------------------------------------------------------
    PatchMeshPtr MeshManager::createBezierPatch(const String& name, const String& groupName, 
            void* controlPointBuffer, VertexDeclaration *declaration, 
            size_t width, size_t height,
            size_t uMaxSubdivisionLevel, size_t vMaxSubdivisionLevel,
            PatchSurface::VisibleSide visibleSide, 
            HardwareBuffer::Usage vbUsage, HardwareBuffer::Usage ibUsage,
            bool vbUseShadow, bool ibUseShadow)
    {
        if (width < 3 || height < 3)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "Bezier patch require at least 3x3 control points",
                "MeshManager::createBezierPatch");
        }

        MeshPtr pMesh = getByName(name);
        if (!pMesh.isNull())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "A mesh called " + name + 
                " already exists!", "MeshManager::createBezierPatch");
        }
        PatchMesh* pm = new PatchMesh(this, name, getNextHandle(), groupName);
        pm->define(controlPointBuffer, declaration, width, height,
            uMaxSubdivisionLevel, vMaxSubdivisionLevel, visibleSide, vbUsage, ibUsage,
            vbUseShadow, ibUseShadow);
        pm->load();
        ResourcePtr res(pm);
        addImpl(res);

        return res;
    }
    //-----------------------------------------------------------------------
    void MeshManager::setPrepareAllMeshesForShadowVolumes(bool enable)
    {
        mPrepAllMeshesForShadowVolumes = enable;
    }
    //-----------------------------------------------------------------------
    bool MeshManager::getPrepareAllMeshesForShadowVolumes(void)
    {
        return mPrepAllMeshesForShadowVolumes;
    }
    //-----------------------------------------------------------------------
    Real MeshManager::getBoundsPaddingFactor(void)
    {
        return mBoundsPaddingFactor;
    }
    //-----------------------------------------------------------------------
    void MeshManager::setBoundsPaddingFactor(Real paddingFactor)
    {
        mBoundsPaddingFactor = paddingFactor;
    }
    //-----------------------------------------------------------------------
    Resource* MeshManager::createImpl(const String& name, ResourceHandle handle, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* createParams)
    {
        // no use for createParams here
        return new Mesh(this, name, handle, group, isManual, loader);
    }
    //-----------------------------------------------------------------------

}
