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
#include "OgreRectangle2D.h"

#include "OgreSimpleRenderable.h"
#include "OgreHardwareBufferManager.h"
#include "OgreCamera.h"

namespace Ogre {
#define POSITION_BINDING 0
#define TEXCOORD_BINDING 1

    Rectangle2D::Rectangle2D(bool includeTextureCoords) 
    {
        mRenderOp.vertexData = new VertexData();

        mRenderOp.indexData = 0;
        mRenderOp.vertexData->vertexCount = 4; 
        mRenderOp.vertexData->vertexStart = 0; 
        mRenderOp.operationType = RenderOperation::OT_TRIANGLE_STRIP; 
        mRenderOp.useIndexes = false; 

        VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
        VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

        decl->addElement(POSITION_BINDING, 0, VET_FLOAT3, VES_POSITION);


        HardwareVertexBufferSharedPtr vbuf = 
            HardwareBufferManager::getSingleton().createVertexBuffer(
            decl->getVertexSize(POSITION_BINDING),
            mRenderOp.vertexData->vertexCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        // Bind buffer
        bind->setBinding(POSITION_BINDING, vbuf);

        if (includeTextureCoords)
        {
            decl->addElement(TEXCOORD_BINDING, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);


            HardwareVertexBufferSharedPtr tvbuf = 
                HardwareBufferManager::getSingleton().createVertexBuffer(
                decl->getVertexSize(TEXCOORD_BINDING),
                mRenderOp.vertexData->vertexCount,
                HardwareBuffer::HBU_STATIC_WRITE_ONLY);

            // Bind buffer
            bind->setBinding(TEXCOORD_BINDING, tvbuf);

            // Set up basic tex coordinates
            float* pTex = static_cast<float*>(
                tvbuf->lock(HardwareBuffer::HBL_DISCARD));
            *pTex++ = 0.0f;
            *pTex++ = 0.0f;
            *pTex++ = 0.0f;
            *pTex++ = 1.0f;
            *pTex++ = 1.0f;
            *pTex++ = 0.0f;
            *pTex++ = 1.0f;
            *pTex++ = 1.0f;
            tvbuf->unlock();
        }

        // set basic white material
        this->setMaterial("BaseWhiteNoLighting");



    }

    Rectangle2D::~Rectangle2D() 
    {
        delete mRenderOp.vertexData;
    }

    void Rectangle2D::setCorners(Real left, Real top, Real right, Real bottom) 
    {
        HardwareVertexBufferSharedPtr vbuf = 
            mRenderOp.vertexData->vertexBufferBinding->getBuffer(POSITION_BINDING);
        float* pFloat = static_cast<float*>(vbuf->lock(HardwareBuffer::HBL_DISCARD));

        *pFloat++ = left;
        *pFloat++ = top;
        *pFloat++ = -1;

        *pFloat++ = left;
        *pFloat++ = bottom;
        *pFloat++ = -1;

        *pFloat++ = right;
        *pFloat++ = top;
        *pFloat++ = -1;

        *pFloat++ = right;
        *pFloat++ = bottom;
        *pFloat++ = -1;

        vbuf->unlock();

        mBox.setExtents(left, top, 0, right, bottom, 0);

    }

    // Override this method to prevent parent transforms (rotation,translation,scale)
    void Rectangle2D::getWorldTransforms( Matrix4* xform ) const
    {
        // return identity matrix to prevent parent transforms
        *xform = Matrix4::IDENTITY;
    }
    //-----------------------------------------------------------------------
    const Quaternion& Rectangle2D::getWorldOrientation(void) const
    {
        return Quaternion::IDENTITY;
    }
    //-----------------------------------------------------------------------
    const Vector3& Rectangle2D::getWorldPosition(void) const
    {
        return Vector3::ZERO;
    }


}

