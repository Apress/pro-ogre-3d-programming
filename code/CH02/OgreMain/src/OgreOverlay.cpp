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

#include "OgreOverlay.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreOverlayContainer.h"
#include "OgreCamera.h"
#include "OgreOverlayManager.h"
#include "OgreQuaternion.h"
#include "OgreVector3.h"


namespace Ogre {

    //---------------------------------------------------------------------
    Overlay::Overlay(const String& name) :
        mName(name),
        mRotate(0.0f), 
        mScrollX(0.0f), mScrollY(0.0f),
        mScaleX(1.0f), mScaleY(1.0f), 
        mTransformOutOfDate(true), mTransformUpdated(true), 
        mZOrder(100), mVisible(false), mInitialised(false)

    {
        mRootNode = new SceneNode(NULL);

    }
    //---------------------------------------------------------------------
    Overlay::~Overlay()
    {
        delete mRootNode;
    }
    //---------------------------------------------------------------------
    const String& Overlay::getName(void) const
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void Overlay::setZOrder(ushort zorder)
    {
        // Limit to 650 since this is multiplied by 100 to pad out for containers
        assert (zorder <= 650 && "Overlay ZOrder cannot be greater than 650!");

        mZOrder = zorder;

        // Notify attached 2D elements
        OverlayContainerList::iterator i, iend;
        iend = m2DElements.end();
        for (i = m2DElements.begin(); i != iend; ++i)
        {
            (*i)->_notifyZOrder(zorder);
        }

    }
    //---------------------------------------------------------------------
    ushort Overlay::getZOrder(void) const
    {
        return mZOrder;
    }
    //---------------------------------------------------------------------
    bool Overlay::isVisible(void) const
    {
        return mVisible;
    }
    //---------------------------------------------------------------------
    void Overlay::show(void)
    {
        mVisible = true;
		if (!mInitialised)
		{
			initialise();
		}
    }
    //---------------------------------------------------------------------
    void Overlay::hide(void)
    {
        mVisible = false;
    }
    //---------------------------------------------------------------------
	void Overlay::initialise(void)
	{
		OverlayContainerList::iterator i, iend;
		iend = m2DElements.end();
		for (i = m2DElements.begin(); i != m2DElements.end(); ++i)
		{
			(*i)->initialise();
		}
		mInitialised = true;
	}
	//---------------------------------------------------------------------
    void Overlay::add2D(OverlayContainer* cont)
    {
        m2DElements.push_back(cont);
        // Notify parent
        cont->_notifyParent(0, this);
        // Set Z order, scaled to separate overlays
        // NB max 100 container levels per overlay, should be plenty
        cont->_notifyZOrder(mZOrder * 100);

        Matrix4 xform;
        _getWorldTransforms(&xform);
        cont->_notifyWorldTransforms(xform);
        cont->_notifyViewport();
    }
    //---------------------------------------------------------------------
    void Overlay::remove2D(OverlayContainer* cont)
    {
        m2DElements.remove(cont);
    }
    //---------------------------------------------------------------------
    void Overlay::add3D(SceneNode* node)
    {
        mRootNode->addChild(node);
    }
    //---------------------------------------------------------------------
    void Overlay::remove3D(SceneNode* node)
    {
        mRootNode->removeChild(node->getName());
    }
    //---------------------------------------------------------------------
    void Overlay::clear(void)
    {
        mRootNode->removeAllChildren();
        m2DElements.clear();
        // Note no deallocation, memory handled by OverlayManager & SceneManager
    }
    //---------------------------------------------------------------------
    void Overlay::setScroll(Real x, Real y)
    {
        mScrollX = x;
        mScrollY = y;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    Real Overlay::getScrollX(void) const
    {
        return mScrollX;
    }
    //---------------------------------------------------------------------
    Real Overlay::getScrollY(void) const
    {
        return mScrollY;
    }
      //---------------------------------------------------------------------
    OverlayContainer* Overlay::getChild(const String& name)
    {

        OverlayContainerList::iterator i, iend;
        iend = m2DElements.end();
        for (i = m2DElements.begin(); i != iend; ++i)
        {
            if ((*i)->getName() == name)
			{
				return *i;

			}
        }
        return NULL;
    }
  //---------------------------------------------------------------------
    void Overlay::scroll(Real xoff, Real yoff)
    {
        mScrollX += xoff;
        mScrollY += yoff;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    void Overlay::setRotate(const Radian& angle)
    {
        mRotate = angle;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    void Overlay::rotate(const Radian& angle)
    {
        setRotate(mRotate + angle);
    }
    //---------------------------------------------------------------------
    void Overlay::setScale(Real x, Real y)
    {
        mScaleX = x;
        mScaleY = y;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    Real Overlay::getScaleX(void) const
    {
        return mScaleX;
    }
    //---------------------------------------------------------------------
    Real Overlay::getScaleY(void) const
    {
        return mScaleY;
    }
    //---------------------------------------------------------------------
    void Overlay::_getWorldTransforms(Matrix4* xform) const
    {
        if (mTransformOutOfDate)
        {
            updateTransform();
        }
        *xform = mTransform;

    }
    //-----------------------------------------------------------------------
    const Quaternion& Overlay::getWorldOrientation(void) const
    {
        // n/a
        return Quaternion::IDENTITY;
    }
    //-----------------------------------------------------------------------
    const Vector3& Overlay::getWorldPosition(void) const
    {
        // n/a
        return Vector3::ZERO;
    }
    //---------------------------------------------------------------------
    void Overlay::_findVisibleObjects(Camera* cam, RenderQueue* queue)
    {
        OverlayContainerList::iterator i, iend;

        if (OverlayManager::getSingleton().hasViewportChanged())
        {
            iend = m2DElements.end();
            for (i = m2DElements.begin(); i != iend; ++i)
            {
                (*i)->_notifyViewport();
            }
        }

        // update elements
        if (mTransformUpdated)
        {
            OverlayContainerList::iterator i, iend;
            Matrix4 xform;

            _getWorldTransforms(&xform);
            iend = m2DElements.end();
            for (i = m2DElements.begin(); i != iend; ++i)
            {
                (*i)->_notifyWorldTransforms(xform);
            }

            mTransformUpdated = false;
        }

        if (mVisible)
        {
            // Add 3D elements
            mRootNode->setPosition(cam->getDerivedPosition());
            mRootNode->setOrientation(cam->getDerivedOrientation());
            mRootNode->_update(true, false);
            // Set up the default queue group for the objects about to be added
            uint8 oldgrp = queue->getDefaultQueueGroup();
            ushort oldPriority = queue-> getDefaultRenderablePriority();
            queue->setDefaultQueueGroup(RENDER_QUEUE_OVERLAY);
            queue->setDefaultRenderablePriority((mZOrder*100)-1);
            mRootNode->_findVisibleObjects(cam, queue, true, false);
            // Reset the group
            queue->setDefaultQueueGroup(oldgrp);
            queue->setDefaultRenderablePriority(oldPriority);
            // Add 2D elements
            iend = m2DElements.end();
            for (i = m2DElements.begin(); i != iend; ++i)
            {
                (*i)->_update();

                (*i)->_updateRenderQueue(queue);
            }
        }



       
    }
    //---------------------------------------------------------------------
    void Overlay::updateTransform(void) const
    {
        // Ordering:
        //    1. Scale
        //    2. Rotate
        //    3. Translate

        Matrix3 rot3x3, scale3x3;
        rot3x3.FromEulerAnglesXYZ(Radian(0),Radian(0),mRotate);
        scale3x3 = Matrix3::ZERO;
        scale3x3[0][0] = mScaleX;
        scale3x3[1][1] = mScaleY;
        scale3x3[2][2] = 1.0f;

        mTransform = Matrix4::IDENTITY;
        mTransform = rot3x3 * scale3x3;
        mTransform.setTrans(Vector3(mScrollX, mScrollY, 0));

        mTransformOutOfDate = false;
    }
    //---------------------------------------------------------------------
	OverlayElement* Overlay::findElementAt(Real x, Real y)
	{
		OverlayElement* ret = NULL;
		int currZ = -1;
        OverlayContainerList::iterator i, iend;
        iend = m2DElements.end();
        for (i = m2DElements.begin(); i != iend; ++i)
        {
			int z = (*i)->getZOrder();
			if (z > currZ)
			{
				OverlayElement* elementFound = (*i)->findElementAt(x,y);
				if(elementFound)
				{
					currZ = elementFound->getZOrder();
					ret = elementFound;
				}
			}
        }
		return ret;
	}

}

