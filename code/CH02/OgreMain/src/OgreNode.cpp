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
#include "OgreNode.h"

#include "OgreException.h"
#include "OgreMath.h"

// Dependencies on render-related types due to ability to render node
#include "OgreMaterialManager.h"
#include "OgreMeshManager.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreCamera.h"

namespace Ogre {

    unsigned long Node::msNextGeneratedNameExt = 1;
	Node::QueuedUpdates Node::msQueuedUpdates;
    //-----------------------------------------------------------------------
    Node::Node()
		:mParent(0),
		mNeedParentUpdate(false),
		mNeedChildUpdate(false),
		mParentNotified(false),
        mQueuedForUpdate(false),
		mOrientation(Quaternion::IDENTITY),
		mPosition(Vector3::ZERO),
		mScale(Vector3::UNIT_SCALE),
        mInheritOrientation(true),
		mInheritScale(true),
		mDerivedOrientation(Quaternion::IDENTITY),
		mDerivedPosition(Vector3::ZERO),
		mDerivedScale(Vector3::UNIT_SCALE),
		mInitialPosition(Vector3::ZERO),
		mInitialOrientation(Quaternion::IDENTITY),
		mInitialScale(Vector3::UNIT_SCALE),
		mAccumAnimWeight(0.0f),
		mCachedTransformOutOfDate(true),
		mListener(0)
    {
        // Generate a name
		StringUtil::StrStreamType str;
		str << "Unnamed_" << msNextGeneratedNameExt++;
        mName = str.str();

        needUpdate();

    }
    //-----------------------------------------------------------------------
	Node::Node(const String& name) : Renderable(),
		mParent(0),
		mNeedParentUpdate(false),
		mNeedChildUpdate(false),
		mParentNotified(false),
        mQueuedForUpdate(false),
		mName(name),
		mOrientation(Quaternion::IDENTITY),
		mPosition(Vector3::ZERO),
		mScale(Vector3::UNIT_SCALE),
        mInheritOrientation(true),
		mInheritScale(true),
		mDerivedOrientation(Quaternion::IDENTITY),
		mDerivedPosition(Vector3::ZERO),
		mDerivedScale(Vector3::UNIT_SCALE),
		mInitialPosition(Vector3::ZERO),
		mInitialOrientation(Quaternion::IDENTITY),
		mInitialScale(Vector3::UNIT_SCALE),
		mAccumAnimWeight(0.0f),
		mCachedTransformOutOfDate(true),
		mListener(0)

    {

        needUpdate();

    }

    //-----------------------------------------------------------------------
    Node::~Node()
    {
		// Call listener (note, only called if there's something to do)
		if (mListener)
		{
			mListener->nodeDestroyed(this);
		}

		removeAllChildren();
		if(mParent)
			mParent->removeChild(this);

        if (mQueuedForUpdate)
        {
            // Erase from queued updates
            QueuedUpdates::iterator it =
                std::find(msQueuedUpdates.begin(), msQueuedUpdates.end(), this);
            assert(it != msQueuedUpdates.end());
            if (it != msQueuedUpdates.end())
            {
                // Optimised algorithm to erase an element from unordered vector.
                *it = msQueuedUpdates.back();
                msQueuedUpdates.pop_back();
            }
        }
	}
    //-----------------------------------------------------------------------
    Node* Node::getParent(void) const
    {
        return mParent;
    }

    //-----------------------------------------------------------------------
    void Node::setParent(Node* parent)
    {
		bool different = (parent != mParent);

        mParent = parent;
        // Request update from parent
		mParentNotified = false ;
        needUpdate();

		// Call listener (note, only called if there's something to do)
		if (mListener && different)
		{
			if (mParent)
				mListener->nodeAttached(this);
			else
				mListener->nodeDetached(this);
		}

    }

    //-----------------------------------------------------------------------
    const Matrix4& Node::_getFullTransform(void) const
    {
        if (mCachedTransformOutOfDate)
        {
            // Use derived values
            mCachedTransform.makeTransform(
                _getDerivedPosition(),
                _getDerivedScale(),
                _getDerivedOrientation());
            mCachedTransformOutOfDate = false;
        }
        return mCachedTransform;
    }
    //-----------------------------------------------------------------------
    void Node::_update(bool updateChildren, bool parentHasChanged)
    {
		// always clear information about parent notification
		mParentNotified = false ;

        // Short circuit the off case
        if (!updateChildren && !mNeedParentUpdate && !mNeedChildUpdate && !parentHasChanged )
        {
            return;
        }


        // See if we should process everyone
        if (mNeedParentUpdate || parentHasChanged)
        {
            // Update transforms from parent
            _updateFromParent();
			mNeedParentUpdate = false;

			// Call listener (note, only called if there's something to do)
			if (mListener)
			{
				mListener->nodeUpdated(this);
			}

		}

		if (mNeedChildUpdate || parentHasChanged)
		{

            ChildNodeMap::iterator it, itend;
			itend = mChildren.end();
            for (it = mChildren.begin(); it != itend; ++it)
            {
                Node* child = it->second;
                child->_update(true, true);
            }
            mChildrenToUpdate.clear();
        }
        else
        {
            // Just update selected children

            ChildUpdateSet::iterator it, itend;
			itend = mChildrenToUpdate.end();
            for(it = mChildrenToUpdate.begin(); it != itend; ++it)
            {
                Node* child = *it;
                child->_update(true, false);
            }

            mChildrenToUpdate.clear();
        }

        mNeedChildUpdate = false;

    }

    //-----------------------------------------------------------------------
    void Node::_updateFromParent(void) const
    {
        if (mParent)
        {
            // Update orientation
            const Quaternion& parentOrientation = mParent->_getDerivedOrientation();
            if (mInheritOrientation)
            {
                // Combine orientation with that of parent
                mDerivedOrientation = parentOrientation * mOrientation;
            }
            else
            {
                // No inheritence
                mDerivedOrientation = mOrientation;
            }

            // Update scale
            const Vector3& parentScale = mParent->_getDerivedScale();
            if (mInheritScale)
            {
                // Scale own position by parent scale, NB just combine
                // as equivalent axes, no shearing
                mDerivedScale = parentScale * mScale;
            }
            else
            {
                // No inheritence
                mDerivedScale = mScale;
            }

            // Change position vector based on parent's orientation & scale
            mDerivedPosition = parentOrientation * (parentScale * mPosition);

            // Add altered position vector to parents
            mDerivedPosition += mParent->_getDerivedPosition();
        }
        else
        {
            // Root node, no parent
            mDerivedOrientation = mOrientation;
            mDerivedPosition = mPosition;
            mDerivedScale = mScale;
        }

        mCachedTransformOutOfDate = true;


    }
    //-----------------------------------------------------------------------
    Node* Node::createChild(const Vector3& translate, const Quaternion& rotate)
    {
        Node* newNode = createChildImpl();
        newNode->translate(translate);
        newNode->rotate(rotate);
        this->addChild(newNode);

        return newNode;
    }
    //-----------------------------------------------------------------------
    Node* Node::createChild(const String& name, const Vector3& translate, const Quaternion& rotate)
    {
        Node* newNode = createChildImpl(name);
        newNode->translate(translate);
        newNode->rotate(rotate);
        this->addChild(newNode);

        return newNode;
    }
    //-----------------------------------------------------------------------
    void Node::addChild(Node* child)
    {
        assert(!child->mParent);

        mChildren.insert(ChildNodeMap::value_type(child->getName(), child));
        child->setParent(this);

    }
    //-----------------------------------------------------------------------
    unsigned short Node::numChildren(void) const
    {
        return static_cast< unsigned short >( mChildren.size() );
    }
    //-----------------------------------------------------------------------
    Node* Node::getChild(unsigned short index) const
    {
        if( index < mChildren.size() )
        {
            ChildNodeMap::const_iterator i = mChildren.begin();
            while (index--) ++i;
            return i->second;
        }
        else
            return NULL;
    }
    //-----------------------------------------------------------------------
    Node* Node::removeChild(unsigned short index)
    {
        Node* ret;
        if (index < mChildren.size())
        {
            ChildNodeMap::iterator i = mChildren.begin();
            while (index--) ++i;
            ret = i->second;
            // cancel any pending update
            cancelUpdate(ret);

            mChildren.erase(i);
            ret->setParent(NULL);
            return ret;
        }
        else
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Child index out of bounds.",
                "Node::getChild" );
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    Node* Node::removeChild(Node* child)
    {
        ChildNodeMap::iterator i, iend;
        iend = mChildren.end();
        for (i = mChildren.begin(); i != iend; ++i)
        {
            if (i->second == child)
            {
                // cancel any pending update
                cancelUpdate(child);

                mChildren.erase(i);
                child->setParent(NULL);
                break;
            }
        }
        return child;
    }
    //-----------------------------------------------------------------------
    const Quaternion& Node::getOrientation() const
    {
        return mOrientation;
    }

    //-----------------------------------------------------------------------
    void Node::setOrientation( const Quaternion & q )
    {
        mOrientation = q;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::setOrientation( Real w, Real x, Real y, Real z)
    {
        mOrientation.w = w;
        mOrientation.x = x;
        mOrientation.y = y;
        mOrientation.z = z;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::resetOrientation(void)
    {
        mOrientation = Quaternion::IDENTITY;
        needUpdate();
    }

    //-----------------------------------------------------------------------
    void Node::setPosition(const Vector3& pos)
    {
        mPosition = pos;
        needUpdate();
    }


    //-----------------------------------------------------------------------
    void Node::setPosition(Real x, Real y, Real z)
    {
        Vector3 v(x,y,z);
        setPosition(v);
    }

    //-----------------------------------------------------------------------
    const Vector3 & Node::getPosition(void) const
    {
        return mPosition;
    }
    //-----------------------------------------------------------------------
    Matrix3 Node::getLocalAxes(void) const
    {
        Vector3 axisX = Vector3::UNIT_X;
        Vector3 axisY = Vector3::UNIT_Y;
        Vector3 axisZ = Vector3::UNIT_Z;

        axisX = mOrientation * axisX;
        axisY = mOrientation * axisY;
        axisZ = mOrientation * axisZ;

        return Matrix3(axisX.x, axisY.x, axisZ.x,
                       axisX.y, axisY.y, axisZ.y,
                       axisX.z, axisY.z, axisZ.z);
    }

    //-----------------------------------------------------------------------
    void Node::translate(const Vector3& d, TransformSpace relativeTo)
    {
        Vector3 adjusted;
        switch(relativeTo)
        {
        case TS_LOCAL:
            // position is relative to parent so transform downwards
            mPosition += mOrientation * d;
        	break;
        case TS_WORLD:
            // position is relative to parent so transform upwards
            if (mParent)
            {
                mPosition += (mParent->_getDerivedOrientation().Inverse() * d)
                    / mParent->_getDerivedScale();
            }
            else
            {
                mPosition += d;
            }
        	break;
        case TS_PARENT:
            mPosition += d;
            break;
        }
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void Node::translate(Real x, Real y, Real z, TransformSpace relativeTo)
    {
        Vector3 v(x,y,z);
        translate(v, relativeTo);
    }
    //-----------------------------------------------------------------------
    void Node::translate(const Matrix3& axes, const Vector3& move, TransformSpace relativeTo)
    {
        Vector3 derived = axes * move;
        translate(derived, relativeTo);
    }
    //-----------------------------------------------------------------------
    void Node::translate(const Matrix3& axes, Real x, Real y, Real z, TransformSpace relativeTo)
    {
        Vector3 d(x,y,z);
        translate(axes,d,relativeTo);
    }
    //-----------------------------------------------------------------------
    void Node::roll(const Radian& angle, TransformSpace relativeTo)
    {
        rotate(Vector3::UNIT_Z, angle, relativeTo);
    }
    //-----------------------------------------------------------------------
    void Node::pitch(const Radian& angle, TransformSpace relativeTo)
    {
        rotate(Vector3::UNIT_X, angle, relativeTo);
    }
    //-----------------------------------------------------------------------
    void Node::yaw(const Radian& angle, TransformSpace relativeTo)
    {
        rotate(Vector3::UNIT_Y, angle, relativeTo);

    }
    //-----------------------------------------------------------------------
    void Node::rotate(const Vector3& axis, const Radian& angle, TransformSpace relativeTo)
    {
        Quaternion q;
        q.FromAngleAxis(angle,axis);
        rotate(q, relativeTo);
    }

    //-----------------------------------------------------------------------
    void Node::rotate(const Quaternion& q, TransformSpace relativeTo)
    {
        switch(relativeTo)
        {
        case TS_PARENT:
            // Rotations are normally relative to local axes, transform up
            mOrientation = q * mOrientation;
            break;
        case TS_WORLD:
            // Rotations are normally relative to local axes, transform up
            mOrientation = mOrientation * _getDerivedOrientation().Inverse()
                * q * _getDerivedOrientation();
            break;
        case TS_LOCAL:
            // Note the order of the mult, i.e. q comes after
            mOrientation = mOrientation * q;
            break;
        }
        needUpdate();
    }
    //-----------------------------------------------------------------------
    const Quaternion & Node::_getDerivedOrientation(void) const
    {
		if (mNeedParentUpdate)
		{
        	_updateFromParent();
			mNeedParentUpdate = false;
		}
        return mDerivedOrientation;
    }
    //-----------------------------------------------------------------------
    const Vector3 & Node::_getDerivedPosition(void) const
    {
		if (mNeedParentUpdate)
		{
        	_updateFromParent();
			mNeedParentUpdate = false;
		}
        return mDerivedPosition;
    }
    //-----------------------------------------------------------------------
    const Vector3 & Node::_getDerivedScale(void) const
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
            mNeedParentUpdate = false;
        }
        return mDerivedScale;
    }
    //-----------------------------------------------------------------------
    void Node::removeAllChildren(void)
    {
		ChildNodeMap::iterator i, iend;
		iend = mChildren.end();
		for (i = mChildren.begin(); i != iend; ++i)
		{
			i->second->setParent(0);
		}
        mChildren.clear();
		mChildrenToUpdate.clear();
    }
    //-----------------------------------------------------------------------
    void Node::setScale(const Vector3& scale)
    {
        mScale = scale;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::setScale(Real x, Real y, Real z)
    {
        mScale.x = x;
        mScale.y = y;
        mScale.z = z;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    const Vector3 & Node::getScale(void) const
    {
        return mScale;
    }
    //-----------------------------------------------------------------------
    void Node::setInheritOrientation(bool inherit)
    {
        mInheritOrientation = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    bool Node::getInheritOrientation(void) const
    {
        return mInheritOrientation;
    }
    //-----------------------------------------------------------------------
    void Node::setInheritScale(bool inherit)
    {
        mInheritScale = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    bool Node::getInheritScale(void) const
    {
        return mInheritScale;
    }
    //-----------------------------------------------------------------------
    void Node::scale(const Vector3& scale)
    {
        mScale = mScale * scale;
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void Node::scale(Real x, Real y, Real z)
    {
        mScale.x *= x;
        mScale.y *= y;
        mScale.z *= z;
        needUpdate();

    }
    //-----------------------------------------------------------------------
    const String& Node::getName(void) const
    {
        return mName;
    }
    //-----------------------------------------------------------------------
    const MaterialPtr& Node::getMaterial(void) const
    {
        if (mpMaterial.isNull())
        {
            mpMaterial = MaterialManager::getSingleton().getByName("Core/NodeMaterial");
			if (mpMaterial.isNull())
				OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find material Core/NodeMaterial",
					"Node::getMaterial" );
            mpMaterial->load();
        }
        return mpMaterial;

    }
    //-----------------------------------------------------------------------
    void Node::getRenderOperation(RenderOperation& op)
    {
        static SubMesh* pSubMesh = 0;
        if (!pSubMesh)
        {
            MeshPtr pMesh = MeshManager::getSingleton().load("axes.mesh",
				ResourceGroupManager::BOOTSTRAP_RESOURCE_GROUP_NAME);
            pSubMesh = pMesh->getSubMesh(0);
        }
        pSubMesh->_getRenderOperation(op);
    }
    //-----------------------------------------------------------------------
    void Node::getWorldTransforms(Matrix4* xform) const
    {
        // Assumes up to date
        *xform = this->_getFullTransform();
    }
    //-----------------------------------------------------------------------
    const Quaternion& Node::getWorldOrientation(void) const
    {
        return _getDerivedOrientation();
    }
    //-----------------------------------------------------------------------
    const Vector3& Node::getWorldPosition(void) const
    {
        return _getDerivedPosition();
    }
    //-----------------------------------------------------------------------
    void Node::setInitialState(void)
    {
        mInitialPosition = mPosition;
        mInitialOrientation = mOrientation;
        mInitialScale = mScale;
    }
    //-----------------------------------------------------------------------
    void Node::resetToInitialState(void)
    {
        mPosition = mInitialPosition;
        mOrientation = mInitialOrientation;
        mScale = mInitialScale;

        // Reset weights
        mAccumAnimWeight = 0.0f;
        mTransFromInitial = Vector3::ZERO;
        mRotFromInitial = Quaternion::IDENTITY;
        mScaleFromInitial = Vector3::UNIT_SCALE;

        needUpdate();
    }
    //-----------------------------------------------------------------------
    const Vector3& Node::getInitialPosition(void) const
    {
        return mInitialPosition;
    }
    //-----------------------------------------------------------------------
    const Quaternion& Node::getInitialOrientation(void) const
    {
        return mInitialOrientation;

    }
    //-----------------------------------------------------------------------
    const Vector3& Node::getInitialScale(void) const
    {
        return mInitialScale;
    }
    //-----------------------------------------------------------------------
    Node* Node::getChild(const String& name) const
    {
        ChildNodeMap::const_iterator i = mChildren.find(name);

        if (i == mChildren.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Child node named " + name +
                " does not exist.", "Node::getChild");
        }
        return i->second;

    }
    //-----------------------------------------------------------------------
    Node* Node::removeChild(const String& name)
    {
        ChildNodeMap::iterator i = mChildren.find(name);

        if (i == mChildren.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Child node named " + name +
                " does not exist.", "Node::removeChild");
        }

        Node* ret = i->second;
        // Cancel any pending update
        cancelUpdate(ret);

        mChildren.erase(i);
        ret->setParent(NULL);

        return ret;


    }
    //-----------------------------------------------------------------------
    Node::ChildNodeIterator Node::getChildIterator(void)
    {
        return ChildNodeIterator(mChildren.begin(), mChildren.end());
    }
	//-----------------------------------------------------------------------
	Node::ConstChildNodeIterator Node::getChildIterator(void) const
	{
		return ConstChildNodeIterator(mChildren.begin(), mChildren.end());
	}
    //-----------------------------------------------------------------------
    void Node::_weightedTransform(Real weight, const Vector3& translate,
       const Quaternion& rotate, const Vector3& scale)
    {
        // If no previous transforms, we can just apply
        if (mAccumAnimWeight == 0.0f)
        {
            mRotFromInitial = rotate;
            mTransFromInitial = translate;
            mScaleFromInitial = scale;
            mAccumAnimWeight = weight;
        }
        else
        {
            // Blend with existing
            Real factor = weight / (mAccumAnimWeight + weight);
            mTransFromInitial += (translate - mTransFromInitial) * factor;
            mRotFromInitial =
                Quaternion::Slerp(factor, mRotFromInitial, rotate);
            // For scale, find delta from 1.0, factor then add back before applying
            Vector3 scaleDiff = (scale - Vector3::UNIT_SCALE) * factor;
            mScaleFromInitial = mScaleFromInitial *
                (scaleDiff + Vector3::UNIT_SCALE);
            mAccumAnimWeight += weight;

        }

        // Update final based on bind position + offsets
        mOrientation = mInitialOrientation * mRotFromInitial;
        mPosition = mInitialPosition + mTransFromInitial;
        mScale = mInitialScale * mScaleFromInitial;
        needUpdate();

    }
    //-----------------------------------------------------------------------
    Real Node::getSquaredViewDepth(const Camera* cam) const
    {
        Vector3 diff = _getDerivedPosition() - cam->getDerivedPosition();

        // NB use squared length rather than real depth to avoid square root
        return diff.squaredLength();
    }
    //-----------------------------------------------------------------------
    void Node::needUpdate(bool forceParentUpdate)
    {

        mNeedParentUpdate = true;
		mNeedChildUpdate = true;
        mCachedTransformOutOfDate = true;

        // Make sure we're not root and parent hasn't been notified before
        if (mParent && (!mParentNotified || forceParentUpdate))
        {
            mParent->requestUpdate(this, forceParentUpdate);
			mParentNotified = true ;
        }

        // all children will be updated
        mChildrenToUpdate.clear();
    }
    //-----------------------------------------------------------------------
    void Node::requestUpdate(Node* child, bool forceParentUpdate)
    {
        // If we're already going to update everything this doesn't matter
        if (mNeedChildUpdate)
        {
            return;
        }

        mChildrenToUpdate.insert(child);
        // Request selective update of me, if we didn't do it before
        if (mParent && (!mParentNotified || forceParentUpdate))
		{
            mParent->requestUpdate(this, forceParentUpdate);
			mParentNotified = true ;
		}

    }
    //-----------------------------------------------------------------------
    void Node::cancelUpdate(Node* child)
    {
        mChildrenToUpdate.erase(child);

        // Propogate this up if we're done
        if (mChildrenToUpdate.empty() && mParent && !mNeedChildUpdate)
        {
            mParent->cancelUpdate(this);
			mParentNotified = false ;
        }
    }
	//-----------------------------------------------------------------------
	void Node::queueNeedUpdate(Node* n)
	{
        // Don't queue the node more than once
        if (!n->mQueuedForUpdate)
        {
            n->mQueuedForUpdate = true;
		    msQueuedUpdates.push_back(n);
        }
	}
	//-----------------------------------------------------------------------
	void Node::processQueuedUpdates(void)
	{
		for (QueuedUpdates::iterator i = msQueuedUpdates.begin();
			i != msQueuedUpdates.end(); ++i)
		{
			// Update, and force parent update since chances are we've ended
			// up with some mixed state in there due to re-entrancy
            Node* n = *i;
            n->mQueuedForUpdate = false;
			n->needUpdate(true);
		}
		msQueuedUpdates.clear();
	}
    //-----------------------------------------------------------------------
    const LightList& Node::getLights(void) const
    {
        // Nodes should not be lit by the scene, this will not get called
        static LightList ll;
        return ll;
    }
}

