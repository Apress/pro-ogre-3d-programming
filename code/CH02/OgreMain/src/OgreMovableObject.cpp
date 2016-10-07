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

#include "OgreMovableObject.h"
#include "OgreSceneNode.h"
#include "OgreTagPoint.h"
#include "OgreLight.h"
#include "OgreEntity.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreCamera.h"

namespace Ogre {
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	uint32 MovableObject::msDefaultQueryFlags = 0xFFFFFFFF;
	uint32 MovableObject::msDefaultVisibilityFlags = 0xFFFFFFFF;
    //-----------------------------------------------------------------------
    MovableObject::MovableObject()
		: mCreator(0), mManager(0), mParentNode(0), mParentIsTagPoint(false), 
		mVisible(true), mUpperDistance(0), mSquaredUpperDistance(0), 
		mBeyondFarDistance(false), mRenderQueueID(RENDER_QUEUE_MAIN),
		mRenderQueueIDSet(false), mQueryFlags(msDefaultQueryFlags),
		mVisibilityFlags(msDefaultVisibilityFlags), mCastShadows (true)
    {
    }
	//-----------------------------------------------------------------------
	MovableObject::MovableObject(const String& name) 
		: mName(name), mCreator(0), mManager(0), mParentNode(0), 
		mParentIsTagPoint(false), mVisible(true), mUpperDistance(0), 
		mSquaredUpperDistance(0), 
		mBeyondFarDistance(false), mRenderQueueID(RENDER_QUEUE_MAIN),
		mRenderQueueIDSet(false), mQueryFlags(msDefaultQueryFlags),
		mVisibilityFlags(msDefaultVisibilityFlags),
		mCastShadows (true)
	{
	}
	//-----------------------------------------------------------------------
    MovableObject::~MovableObject()
    {
        if (mParentNode)
        {
            // detach from parent
            if (mParentIsTagPoint)
            {
                // May be we are a lod entity which not in the parent entity child object list,
                // call this method could safely ignore this case.
                static_cast<TagPoint*>(mParentNode)->getParentEntity()->detachObjectFromBone(this);
            }
            else
            {
                // May be we are a lod entity which not in the parent node child object list,
                // call this method could safely ignore this case.
                static_cast<SceneNode*>(mParentNode)->detachObject(this);
            }
        }
    }
    //-----------------------------------------------------------------------
    void MovableObject::_notifyAttached(Node* parent, bool isTagPoint)
    {
        mParentNode = parent;
        mParentIsTagPoint = isTagPoint;
    }
    //-----------------------------------------------------------------------
    Node* MovableObject::getParentNode(void) const
    {
        return mParentNode;
    }
    //-----------------------------------------------------------------------
    SceneNode* MovableObject::getParentSceneNode(void) const
    {
        if (mParentIsTagPoint)
        {
            TagPoint* tp = static_cast<TagPoint*>(mParentNode);
            return tp->getParentEntity()->getParentSceneNode();
        }
        else
        {
            return static_cast<SceneNode*>(mParentNode);
        }
    }
    //-----------------------------------------------------------------------
    bool MovableObject::isAttached(void) const
    {
        return (mParentNode != 0);

    }
    //-----------------------------------------------------------------------
	bool MovableObject::isInScene(void) const
	{
		if (mParentNode != 0)
		{
			if (mParentIsTagPoint)
			{
				TagPoint* tp = static_cast<TagPoint*>(mParentNode);
				return tp->getParentEntity()->isInScene();
			}
			else
			{
				SceneNode* sn = static_cast<SceneNode*>(mParentNode);
				return sn->isInSceneGraph();
			}
		}
		else
		{
			return false;
		}
	}
    //-----------------------------------------------------------------------
    void MovableObject::setVisible(bool visible)
    {
        mVisible = visible;
    }
    //-----------------------------------------------------------------------
    bool MovableObject::getVisible(void) const
    {
        return mVisible;
    }
    //-----------------------------------------------------------------------
    bool MovableObject::isVisible(void) const
    {
		bool flagVis = true;
		if (Root::getSingleton()._getCurrentSceneManager())
		{
			flagVis = (mVisibilityFlags & 
				Root::getSingleton()._getCurrentSceneManager()->getVisibilityMask()) != 0;
		}

		return mVisible && !mBeyondFarDistance && flagVis;
    }
	//-----------------------------------------------------------------------
	void MovableObject::_notifyCurrentCamera(Camera* cam)
	{
		if (mParentNode)
		{
			if (cam->getUseRenderingDistance() && mUpperDistance > 0)
			{
				Real rad = getBoundingRadius();
				Real squaredDepth = mParentNode->getSquaredViewDepth(cam);
				// Max distance to still render
				Real maxDist = mUpperDistance + rad;
				if (squaredDepth > Math::Sqr(maxDist))
				{
					mBeyondFarDistance = true;
				}
				else
				{
					mBeyondFarDistance = false;
				}
			}
			else
			{
				mBeyondFarDistance = false;
			}
		}

	}
    //-----------------------------------------------------------------------
    void MovableObject::setRenderQueueGroup(uint8 queueID)
    {
        mRenderQueueID = queueID;
        mRenderQueueIDSet = true;
    }
    //-----------------------------------------------------------------------
    uint8 MovableObject::getRenderQueueGroup(void) const
    {
        return mRenderQueueID;
    }
    //-----------------------------------------------------------------------
	const Matrix4& MovableObject::_getParentNodeFullTransform(void) const
	{
		
		if(mParentNode)
		{
			// object attached to a sceneNode
			return mParentNode->_getFullTransform();
		}
        // fallback
        return Matrix4::IDENTITY;
	}
    //-----------------------------------------------------------------------
    const AxisAlignedBox& MovableObject::getWorldBoundingBox(bool derive) const
    {
        if (derive)
        {
            mWorldAABB = this->getBoundingBox();
            mWorldAABB.transform(_getParentNodeFullTransform());
        }

        return mWorldAABB;

    }
    //-----------------------------------------------------------------------
	const Sphere& MovableObject::getWorldBoundingSphere(bool derive) const
	{
		if (derive)
		{
			mWorldBoundingSphere.setRadius(getBoundingRadius());
			mWorldBoundingSphere.setCenter(mParentNode->_getDerivedPosition());
		}
		return mWorldBoundingSphere;
	}

    //-----------------------------------------------------------------------
    ShadowCaster::ShadowRenderableListIterator MovableObject::getShadowVolumeRenderableIterator(
        ShadowTechnique shadowTechnique, const Light* light, 
        HardwareIndexBufferSharedPtr* indexBuffer, 
        bool extrudeVertices, Real extrusionDist, unsigned long flags )
    {
        static ShadowRenderableList dummyList;
        return ShadowRenderableListIterator(dummyList.begin(), dummyList.end());
    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& MovableObject::getLightCapBounds(void) const
    {
        // Same as original bounds
        return getWorldBoundingBox();
    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& MovableObject::getDarkCapBounds(const Light& light, Real extrusionDist) const
    {
        // Extrude own light cap bounds
        mWorldDarkCapBounds = getLightCapBounds();
        this->extrudeBounds(mWorldDarkCapBounds, light.getAs4DVector(), 
            extrusionDist);
        return mWorldDarkCapBounds;

    }
    //-----------------------------------------------------------------------
    Real MovableObject::getPointExtrusionDistance(const Light* l) const
    {
        if (mParentNode)
        {
            return getExtrusionDistance(mParentNode->_getDerivedPosition(), l);
        }
        else
        {
            return 0;
        }
    }
	//-----------------------------------------------------------------------
	uint32 MovableObject::getTypeFlags(void) const
	{
		if (mCreator)
		{
			return mCreator->getTypeFlags();
		}
		else
		{
			return 0xFFFFFFFF;
		}
	}
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	MovableObject* MovableObjectFactory::createInstance(
		const String& name, SceneManager* manager, 
		const NameValuePairList* params)
	{
		MovableObject* m = createInstanceImpl(name, params);
		m->_notifyCreator(this);
		m->_notifyManager(manager);
		return m;
	}


}

