/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://ogre.sourceforge.net/

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
#include "OgreEntity.h"

#include "OgreMeshManager.h"
#include "OgreSubMesh.h"
#include "OgreSubEntity.h"
#include "OgreException.h"
#include "OgreSceneManager.h"
#include "OgreLogManager.h"
#include "OgreSkeleton.h"
#include "OgreBone.h"
#include "OgreCamera.h"
#include "OgreTagPoint.h"
#include "OgreAxisAlignedBox.h"
#include "OgreHardwareBufferManager.h"
#include "OgreVector4.h"
#include "OgreRoot.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreSkeletonInstance.h"
#include "OgreEdgeListBuilder.h"
#include "OgreStringConverter.h"
#include "OgreAnimation.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    Entity::Entity ()
		: mAnimationState(NULL),
          mSkelAnimVertexData(0),
		  mSoftwareVertexAnimVertexData(0),
		  mHardwareVertexAnimVertexData(0),
          mPreparedForShadowVolumes(false),
          mBoneWorldMatrices(NULL),
          mBoneMatrices(NULL),
          mNumBoneMatrices(0),
		  mFrameAnimationLastUpdated(std::numeric_limits<unsigned long>::max()),
          mFrameBonesLastUpdated(NULL),
		  mSharedSkeletonEntities(NULL),
		  mDisplaySkeleton(false),
	      mHardwareAnimation(false),
		  mVertexProgramInUse(false),
		  mSoftwareAnimationRequests(0),
		  mSoftwareAnimationNormalsRequests(0),
		  mMeshLodIndex(0),
		  mMeshLodFactorInv(1.0f),
		  mMinMeshLodIndex(99),
		  mMaxMeshLodIndex(0),		// Backwards, remember low value = high detail
		  mMaterialLodFactorInv(1.0f),
		  mMinMaterialLodIndex(99),
		  mMaxMaterialLodIndex(0), 		// Backwards, remember low value = high detail
          mSkeletonInstance(0),
		  mLastParentXform(Matrix4::ZERO),
          mFullBoundingBox(),
		  mNormaliseNormals(false)
    {
    }
    //-----------------------------------------------------------------------
    Entity::Entity( const String& name, MeshPtr& mesh) :
		MovableObject(name),
        mMesh(mesh),
        mAnimationState(NULL),
		mSkelAnimVertexData(0),
		mSoftwareVertexAnimVertexData(0),
		mHardwareVertexAnimVertexData(0),
        mPreparedForShadowVolumes(false),
        mBoneWorldMatrices(NULL),
        mBoneMatrices(NULL),
        mNumBoneMatrices(0),
		mFrameAnimationLastUpdated(std::numeric_limits<unsigned long>::max()),
        mFrameBonesLastUpdated(NULL),
        mSharedSkeletonEntities(NULL),
		mDisplaySkeleton(false),
		mHardwareAnimation(false),
		mVertexProgramInUse(false),
		mSoftwareAnimationRequests(0),
		mSoftwareAnimationNormalsRequests(0),
		mMeshLodIndex(0),
		mMeshLodFactorInv(1.0f),
		mMinMeshLodIndex(99),
		mMaxMeshLodIndex(0),		// Backwards, remember low value = high detail
		mMaterialLodFactorInv(1.0f),
		mMinMaterialLodIndex(99),
		mMaxMaterialLodIndex(0), 		// Backwards, remember low value = high detail
		mSkeletonInstance(0),
		mLastParentXform(Matrix4::ZERO),
        mFullBoundingBox(),
		mNormaliseNormals(false)
{
        // Is mesh skeletally animated?
        if (mMesh->hasSkeleton() && !mMesh->getSkeleton().isNull())
        {
            mSkeletonInstance = new SkeletonInstance(mMesh->getSkeleton());
            mSkeletonInstance->load();
        }

        // Build main subentity list
        buildSubEntityList(mesh, &mSubEntityList);

        // Check if mesh is using manual LOD
        if (mesh->isLodManual())
        {
            ushort i, numLod;
            numLod = mesh->getNumLodLevels();
            // NB skip LOD 0 which is the original
            for (i = 1; i < numLod; ++i)
            {
                const MeshLodUsage& usage = mesh->getLodLevel(i);
                // Manually create entity
                Entity* lodEnt = new Entity(name + "Lod" + StringConverter::toString(i),
                    usage.manualMesh);
                mLodEntityList.push_back(lodEnt);
            }
        }


        // Initialise the AnimationState, if Mesh has animation
		if (hasSkeleton())
		{
			mFrameBonesLastUpdated = new unsigned long(std::numeric_limits<unsigned long>::max());
			mNumBoneMatrices = mSkeletonInstance->getNumBones();
			mBoneMatrices = new Matrix4[mNumBoneMatrices];
		}
        if (hasSkeleton() || hasVertexAnimation())
        {
            mAnimationState = new AnimationStateSet();
            mesh->_initAnimationState(mAnimationState);
            prepareTempBlendBuffers();
        }

        reevaluateVertexProcessing();


        // Do we have a mesh where edge lists are not going to be available?
        if (!mesh->isEdgeListBuilt() && !mesh->getAutoBuildEdgeLists())
        {
            setCastShadows(false);
        }
    }
    //-----------------------------------------------------------------------
    Entity::~Entity()
    {
        // Delete submeshes
        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();
        for (i = mSubEntityList.begin(); i != iend; ++i)
        {
            // Delete SubEntity
            delete *i;
        }
        // Delete LOD entities
        LODEntityList::iterator li, liend;
        liend = mLodEntityList.end();
        for (li = mLodEntityList.begin(); li != liend; ++li)
        {
            // Delete
            delete (*li);
        }

        // Delete shadow renderables
        ShadowRenderableList::iterator si, siend;
        siend = mShadowRenderables.end();
        for (si = mShadowRenderables.begin(); si != siend; ++si)
        {
            delete *si;
        }

        // Detach all child objects, do this manually to avoid needUpdate() call
        // which can fail because of deleted items
        detachAllObjectsImpl();

        if (mSkeletonInstance) {
            delete [] mBoneWorldMatrices;

            if (mSharedSkeletonEntities) {
                mSharedSkeletonEntities->erase(this);
                if (mSharedSkeletonEntities->empty()) {
                    delete mSharedSkeletonEntities;
                    delete mFrameBonesLastUpdated;
                    delete mSkeletonInstance;
                    delete [] mBoneMatrices;
                    delete mAnimationState;
                }
            } else {
                delete mFrameBonesLastUpdated;
                delete mSkeletonInstance;
                delete [] mBoneMatrices;
                delete mAnimationState;
            }
        }
		else if (hasVertexAnimation())
		{
			delete mAnimationState;
		}

		delete mSkelAnimVertexData;
		delete mSoftwareVertexAnimVertexData;
		delete mHardwareVertexAnimVertexData;

    }
	//-----------------------------------------------------------------------
	bool Entity::hasVertexAnimation(void) const
	{
		return mMesh->hasVertexAnimation();
	}
    //-----------------------------------------------------------------------
    const MeshPtr& Entity::getMesh(void) const
    {
        return mMesh;
    }
    //-----------------------------------------------------------------------
    SubEntity* Entity::getSubEntity(unsigned int index) const
    {
        if (index >= mSubEntityList.size())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
            "Index out of bounds.",
            "Entity::getSubEntity");
        return mSubEntityList[index];
    }
    //-----------------------------------------------------------------------
    SubEntity* Entity::getSubEntity(const String& name) const
    {
        ushort index = mMesh->_getSubMeshIndex(name);
        return getSubEntity(index);
    }
    //-----------------------------------------------------------------------
    unsigned int Entity::getNumSubEntities(void) const
    {
        return static_cast< unsigned int >( mSubEntityList.size() );
    }
    //-----------------------------------------------------------------------
    Entity* Entity::clone( const String& newName) const
    {
   		if (!mManager)
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
				"Cannot clone an Entity that wasn't created through a "
				"SceneManager", "Entity::clone");
		}
	    Entity* newEnt = mManager->createEntity(newName, getMesh()->getName() );
        // Copy material settings
        SubEntityList::const_iterator i;
        unsigned int n = 0;
        for (i = mSubEntityList.begin(); i != mSubEntityList.end(); ++i, ++n)
        {
            newEnt->getSubEntity(n)->setMaterialName((*i)->getMaterialName());
        }
        if (mAnimationState)
        {
            delete newEnt->mAnimationState;
            newEnt->mAnimationState = new AnimationStateSet(*mAnimationState);
        }
        return newEnt;
    }
    //-----------------------------------------------------------------------
    void Entity::setMaterialName(const String& name)
    {
        // Set for all subentities
        SubEntityList::iterator i;
        for (i = mSubEntityList.begin(); i != mSubEntityList.end(); ++i)
        {
            (*i)->setMaterialName(name);
        }

    }
    //-----------------------------------------------------------------------
    void Entity::_notifyCurrentCamera(Camera* cam)
    {
		MovableObject::_notifyCurrentCamera(cam);

        // Calculate the LOD
        if (mParentNode)
        {
            Real squaredDepth = mParentNode->getSquaredViewDepth(cam);

            // Do Mesh LOD
            // Adjust this depth by the entity bias factor
            Real tmp = squaredDepth * mMeshLodFactorInv;
            // Now adjust it by the camera bias
            tmp = tmp * cam->_getLodBiasInverse();
            // Get the index at this biased depth
            mMeshLodIndex = mMesh->getLodIndexSquaredDepth(tmp);
            // Apply maximum detail restriction (remember lower = higher detail)
            mMeshLodIndex = std::max(mMaxMeshLodIndex, mMeshLodIndex);
            // Apply minimum detail restriction (remember higher = lower detail)
            mMeshLodIndex = std::min(mMinMeshLodIndex, mMeshLodIndex);

            // Now do material LOD
            // Adjust this depth by the entity bias factor
            tmp = squaredDepth * mMaterialLodFactorInv;
            // Now adjust it by the camera bias
            tmp = tmp * cam->_getLodBiasInverse();
            SubEntityList::iterator i, iend;
            iend = mSubEntityList.end();
            for (i = mSubEntityList.begin(); i != iend; ++i)
            {
                // Get the index at this biased depth
                unsigned short idx = (*i)->mpMaterial->getLodIndexSquaredDepth(tmp);
                // Apply maximum detail restriction (remember lower = higher detail)
                idx = std::max(mMaxMaterialLodIndex, idx);
                // Apply minimum detail restriction (remember higher = lower detail)
                (*i)->mMaterialLodIndex = std::min(mMinMaterialLodIndex, idx);
            }


        }
        // Notify any child objects
        ChildObjectList::iterator child_itr = mChildObjectList.begin();
        ChildObjectList::iterator child_itr_end = mChildObjectList.end();
        for( ; child_itr != child_itr_end; child_itr++)
        {
            (*child_itr).second->_notifyCurrentCamera(cam);
        }


    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& Entity::getBoundingBox(void) const
    {
        // Get from Mesh
        mFullBoundingBox = mMesh->getBounds();
        mFullBoundingBox.merge(getChildObjectsBoundingBox());

        // Don't scale here, this is taken into account when world BBox calculation is done

        return mFullBoundingBox;
    }
    //-----------------------------------------------------------------------
    AxisAlignedBox Entity::getChildObjectsBoundingBox(void) const
    {
        AxisAlignedBox aa_box;
        AxisAlignedBox full_aa_box;
        full_aa_box.setNull();

        ChildObjectList::const_iterator child_itr = mChildObjectList.begin();
        ChildObjectList::const_iterator child_itr_end = mChildObjectList.end();
        for( ; child_itr != child_itr_end; child_itr++)
        {
            aa_box = child_itr->second->getBoundingBox();
            TagPoint* tp = (TagPoint*)child_itr->second->getParentNode();
            // Use transform local to skeleton since world xform comes later
            aa_box.transform(tp->_getFullLocalTransform());

            full_aa_box.merge(aa_box);
        }

        return full_aa_box;
    }
	//-----------------------------------------------------------------------
	const AxisAlignedBox& Entity::getWorldBoundingBox(bool derive) const
	{
		if (derive)
		{
			// derive child bounding boxes
			ChildObjectList::const_iterator child_itr = mChildObjectList.begin();
			ChildObjectList::const_iterator child_itr_end = mChildObjectList.end();
			for( ; child_itr != child_itr_end; child_itr++)
			{
				child_itr->second->getWorldBoundingBox(true);
			}
		}
		return MovableObject::getWorldBoundingBox(derive);
	}
	//-----------------------------------------------------------------------
	const Sphere& Entity::getWorldBoundingSphere(bool derive) const
	{
		if (derive)
		{
			// derive child bounding boxes
			ChildObjectList::const_iterator child_itr = mChildObjectList.begin();
			ChildObjectList::const_iterator child_itr_end = mChildObjectList.end();
			for( ; child_itr != child_itr_end; child_itr++)
			{
				child_itr->second->getWorldBoundingSphere(true);
			}
		}
		return MovableObject::getWorldBoundingSphere(derive);

	}
    //-----------------------------------------------------------------------
    void Entity::_updateRenderQueue(RenderQueue* queue)
    {
        // Check we're not using a manual LOD
        if (mMeshLodIndex > 0 && mMesh->isLodManual())
        {
            // Use alternate entity
            assert( static_cast< size_t >( mMeshLodIndex - 1 ) < mLodEntityList.size() &&
                "No LOD EntityList - did you build the manual LODs after creating the entity?");
            // index - 1 as we skip index 0 (original lod)
            if (hasSkeleton() && mLodEntityList[mMeshLodIndex - 1]->hasSkeleton())
            {
                // Copy the animation state set to lod entity, we assume the lod
                // entity only has a subset animation states
                mAnimationState->copyMatchingState(
					mLodEntityList[mMeshLodIndex - 1]->mAnimationState);
            }
            mLodEntityList[mMeshLodIndex - 1]->_updateRenderQueue(queue);
            return;
        }

        // Add each visible SubEntity to the queue
        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();
        for (i = mSubEntityList.begin(); i != iend; ++i)
        {
            if((*i)->isVisible())
            {
                if(mRenderQueueIDSet)
                {
                    queue->addRenderable(*i, mRenderQueueID);
                }
                else
                {
                    queue->addRenderable(*i);
                }
            }
        }

        // Since we know we're going to be rendered, take this opportunity to
        // update the animation
        if (hasSkeleton() || hasVertexAnimation())
        {
            updateAnimation();

            //--- pass this point,  we are sure that the transformation matrix of each bone and tagPoint have been updated
            ChildObjectList::iterator child_itr = mChildObjectList.begin();
            ChildObjectList::iterator child_itr_end = mChildObjectList.end();
            for( ; child_itr != child_itr_end; child_itr++)
            {
                if ((*child_itr).second->isVisible())
                    (*child_itr).second->_updateRenderQueue(queue);
            }
        }

        // HACK to display bones
        // This won't work if the entity is not centered at the origin
        // TODO work out a way to allow bones to be rendered when Entity not centered
        if (mDisplaySkeleton && hasSkeleton())
        {
            int numBones = mSkeletonInstance->getNumBones();
            for (int b = 0; b < numBones; ++b)
            {
                Bone* bone = mSkeletonInstance->getBone(b);
                if(mRenderQueueIDSet)
                {
                     queue->addRenderable(bone, mRenderQueueID);
                } else {
                     queue->addRenderable(bone);
                }
            }
        }




    }
    //-----------------------------------------------------------------------
    AnimationState* Entity::getAnimationState(const String& name) const
    {
        if (!mAnimationState)
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Entity is not animated",
                "Entity::getAnimationState");
        }

		return mAnimationState->getAnimationState(name);
    }
    //-----------------------------------------------------------------------
    AnimationStateSet* Entity::getAllAnimationStates(void) const
    {
        return mAnimationState;
    }
    //-----------------------------------------------------------------------
    const String& Entity::getMovableType(void) const
    {
		return EntityFactory::FACTORY_TYPE_NAME;
    }
	//-----------------------------------------------------------------------
	bool Entity::tempVertexAnimBuffersBound(void) const
	{
		// Do we still have temp buffers for software vertex animation bound?
		bool ret = true;
		if (mMesh->sharedVertexData && mMesh->getSharedVertexDataAnimationType() != VAT_NONE)
		{
			ret = ret && mTempVertexAnimInfo.buffersCheckedOut(true, false);
		}
		for (SubEntityList::const_iterator i = mSubEntityList.begin();
			i != mSubEntityList.end(); ++i)
		{
			SubEntity* sub = *i;
			if (!sub->getSubMesh()->useSharedVertices
				&& sub->getSubMesh()->getVertexAnimationType() != VAT_NONE)
			{
				ret = ret && sub->_getVertexAnimTempBufferInfo()->buffersCheckedOut(true, false);
			}
		}
		return ret;
	}
    //-----------------------------------------------------------------------
    bool Entity::tempSkelAnimBuffersBound(bool requestNormals) const
    {
        // Do we still have temp buffers for software skeleton animation bound?
        if (mSkelAnimVertexData)
        {
            if (!mTempSkelAnimInfo.buffersCheckedOut(true, requestNormals))
                return false;
        }
        for (SubEntityList::const_iterator i = mSubEntityList.begin();
            i != mSubEntityList.end(); ++i)
        {
            SubEntity* sub = *i;
            if (sub->isVisible() && sub->mSkelAnimVertexData)
            {
                if (!sub->mTempSkelAnimInfo.buffersCheckedOut(true, requestNormals))
                    return false;
            }
        }
        return true;
    }
    //-----------------------------------------------------------------------
    void Entity::updateAnimation(void)
    {
		Root& root = Root::getSingleton();
		bool hwAnimation = isHardwareAnimationEnabled();
		bool forcedSwAnimation = getSoftwareAnimationRequests()>0;
		bool forcedNormals = getSoftwareAnimationNormalsRequests()>0;
		bool stencilShadows = false;
		if (getCastShadows() && root._getCurrentSceneManager())
			stencilShadows =  root._getCurrentSceneManager()->isShadowTechniqueStencilBased();
		bool softwareAnimation = !hwAnimation || stencilShadows || forcedSwAnimation;
		// Blend normals in s/w only if we're not using h/w animation,
		// since shadows only require positions
		bool blendNormals = !hwAnimation || forcedNormals;
        // Animation dirty if animation state modified or manual bones modified
        bool animationDirty =
            (mFrameAnimationLastUpdated != mAnimationState->getDirtyFrameNumber()) ||
            (hasSkeleton() && getSkeleton()->getManualBonesDirty());

		// We only do these tasks if animation is dirty
		// Or, if we're using a skeleton and manual bones have been moved
		// Or, if we're using software animation and temp buffers are unbound
        if (animationDirty ||
			(softwareAnimation && hasVertexAnimation() && !tempVertexAnimBuffersBound()) ||
			(softwareAnimation && hasSkeleton() && !tempSkelAnimBuffersBound(blendNormals)))
        {
			if (hasVertexAnimation())
			{
				if (softwareAnimation)
				{
					// grab & bind temporary buffer for positions
					if (mSoftwareVertexAnimVertexData
						&& mMesh->getSharedVertexDataAnimationType() != VAT_NONE)
					{
						mTempVertexAnimInfo.checkoutTempCopies(true, false);
						// NB we suppress hardware upload while doing blend if we're
						// hardware animation, because the only reason for doing this
						// is for shadow, which need only be uploaded then
						mTempVertexAnimInfo.bindTempCopies(mSoftwareVertexAnimVertexData,
							hwAnimation);
					}
					SubEntityList::iterator i, iend;
					iend = mSubEntityList.end();
					for (i = mSubEntityList.begin(); i != iend; ++i)
					{
						// Blend dedicated geometry
						SubEntity* se = *i;
						if (se->isVisible() && se->mSoftwareVertexAnimVertexData
							&& se->getSubMesh()->getVertexAnimationType() != VAT_NONE)
						{
							se->mTempVertexAnimInfo.checkoutTempCopies(true, false);
							se->mTempVertexAnimInfo.bindTempCopies(se->mSoftwareVertexAnimVertexData,
								hwAnimation);
						}

					}
				}
				applyVertexAnimation(hwAnimation, stencilShadows);
			}

			if (hasSkeleton())
			{
				cacheBoneMatrices();

				// Software blend?
				if (softwareAnimation)
				{
					// Ok, we need to do a software blend
					// Firstly, check out working vertex buffers
					if (mSkelAnimVertexData)
					{
						// Blend shared geometry
						// NB we suppress hardware upload while doing blend if we're
						// hardware animation, because the only reason for doing this
						// is for shadow, which need only be uploaded then
						mTempSkelAnimInfo.checkoutTempCopies(true, blendNormals);
						mTempSkelAnimInfo.bindTempCopies(mSkelAnimVertexData,
							hwAnimation);
						// Blend, taking source from either mesh data or morph data
						Mesh::softwareVertexBlend(
							(mMesh->getSharedVertexDataAnimationType() != VAT_NONE) ?
								mSoftwareVertexAnimVertexData :	mMesh->sharedVertexData,
							mSkelAnimVertexData,
							mBoneMatrices, &mMesh->sharedBlendIndexToBoneIndexMap[0],
							blendNormals);
					}
					SubEntityList::iterator i, iend;
					iend = mSubEntityList.end();
					for (i = mSubEntityList.begin(); i != iend; ++i)
					{
						// Blend dedicated geometry
						SubEntity* se = *i;
						if (se->isVisible() && se->mSkelAnimVertexData)
						{
							se->mTempSkelAnimInfo.checkoutTempCopies(true, blendNormals);
							se->mTempSkelAnimInfo.bindTempCopies(se->mSkelAnimVertexData,
								hwAnimation);
							// Blend, taking source from either mesh data or morph data
							Mesh::softwareVertexBlend(
								(se->getSubMesh()->getVertexAnimationType() != VAT_NONE)?
									se->mSoftwareVertexAnimVertexData : se->mSubMesh->vertexData,
								se->mSkelAnimVertexData,
								mBoneMatrices, &se->mSubMesh->blendIndexToBoneIndexMap[0],
								blendNormals);
						}

					}

				}
			}

            // Trigger update of bounding box if necessary
            if (!mChildObjectList.empty())
                mParentNode->needUpdate();

			mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber();
        }

        // Need to update the child object's transforms when animation dirty
        // or parent node transform has altered.
        if (hasSkeleton() &&
            (animationDirty || mLastParentXform != _getParentNodeFullTransform()))
        {
            // Cache last parent transform for next frame use too.
            mLastParentXform = _getParentNodeFullTransform();

            //--- Update the child object's transforms
            ChildObjectList::iterator child_itr = mChildObjectList.begin();
            ChildObjectList::iterator child_itr_end = mChildObjectList.end();
            for( ; child_itr != child_itr_end; child_itr++)
            {
                (*child_itr).second->getParentNode()->_update(true, true);
            }

            // Also calculate bone world matrices, since are used as replacement world matrices,
            // but only if it's used (when using hardware animation and skeleton animated).
            if (hwAnimation && _isSkeletonAnimated())
            {
                // Allocate bone world matrices on demand, for better memory footprint
                // when using software animation.
                if (!mBoneWorldMatrices)
                {
                    mBoneWorldMatrices = new Matrix4[mNumBoneMatrices];
                }

                for (unsigned short i = 0; i < mNumBoneMatrices; ++i)
                {
                    mBoneWorldMatrices[i] = mLastParentXform * mBoneMatrices[i];
                }
            }
        }
    }
	//-----------------------------------------------------------------------
	void Entity::initHardwareAnimationElements(VertexData* vdata,
		ushort numberOfElements)
	{
		if (vdata->hwAnimationDataList.size() < numberOfElements)
		{
			vdata->allocateHardwareAnimationElements(numberOfElements);
		}
		// Initialise parametrics incase we don't use all of them
		for (size_t i = 0; i < vdata->hwAnimationDataList.size(); ++i)
		{
			vdata->hwAnimationDataList[i].parametric = 0.0f;
		}
		// reset used count
		vdata->hwAnimDataItemsUsed = 0;

	}
	//-----------------------------------------------------------------------
	void Entity::applyVertexAnimation(bool hardwareAnimation, bool stencilShadows)
	{
		const MeshPtr& msh = getMesh();
		bool swAnim = !hardwareAnimation || stencilShadows || (mSoftwareAnimationRequests>0);

		// make sure we have enough hardware animation elements to play with
		if (hardwareAnimation)
		{
			if (mHardwareVertexAnimVertexData
				&& msh->getSharedVertexDataAnimationType() != VAT_NONE)
			{
				initHardwareAnimationElements(mHardwareVertexAnimVertexData,
					(msh->getSharedVertexDataAnimationType() == VAT_POSE)
					? mHardwarePoseCount : 1);
			}
			for (SubEntityList::iterator si = mSubEntityList.begin();
				si != mSubEntityList.end(); ++si)
			{
				SubEntity* sub = *si;
				if (sub->getSubMesh()->getVertexAnimationType() != VAT_NONE &&
					!sub->getSubMesh()->useSharedVertices)
				{
					initHardwareAnimationElements(
						sub->_getHardwareVertexAnimVertexData(),
						(sub->getSubMesh()->getVertexAnimationType() == VAT_POSE)
						? sub->mHardwarePoseCount : 1);
				}
			}

		}
		else
		{
			// May be blending multiple poses in software
			// Suppress hardware upload of buffers
			if (mSoftwareVertexAnimVertexData &&
				mMesh->getSharedVertexDataAnimationType() == VAT_POSE)
			{
				const VertexElement* elem = mSoftwareVertexAnimVertexData
					->vertexDeclaration->findElementBySemantic(VES_POSITION);
				HardwareVertexBufferSharedPtr buf = mSoftwareVertexAnimVertexData
					->vertexBufferBinding->getBuffer(elem->getSource());
				buf->suppressHardwareUpdate(true);
			}
			for (SubEntityList::iterator si = mSubEntityList.begin();
				si != mSubEntityList.end(); ++si)
			{
				SubEntity* sub = *si;
				if (!sub->getSubMesh()->useSharedVertices &&
					sub->getSubMesh()->getVertexAnimationType() == VAT_POSE)
				{
					VertexData* data = sub->_getSoftwareVertexAnimVertexData();
					const VertexElement* elem = data->vertexDeclaration
						->findElementBySemantic(VES_POSITION);
					HardwareVertexBufferSharedPtr buf = data
						->vertexBufferBinding->getBuffer(elem->getSource());
					buf->suppressHardwareUpdate(true);
				}
			}
		}


		// Now apply the animation(s)
		// Note - you should only apply one morph animation to each set of vertex data
		// at once; if you do more, only the last one will actually apply
		markBuffersUnusedForAnimation();
		ConstEnabledAnimationStateIterator animIt = mAnimationState->getEnabledAnimationStateIterator();
		while(animIt.hasMoreElements())
		{
            const AnimationState* state = animIt.getNext();
            Animation* anim = msh->_getAnimationImpl(state->getAnimationName());
            if (anim)
            {
                anim->apply(this, state->getTimePosition(), state->getWeight(),
                    swAnim, hardwareAnimation);
            }
		}
		// Deal with cases where no animation applied
		restoreBuffersForUnusedAnimation(hardwareAnimation);

		// Unsuppress hardware upload if we suppressed it
		if (!hardwareAnimation)
		{
			if (mSoftwareVertexAnimVertexData &&
				msh->getSharedVertexDataAnimationType() == VAT_POSE)
			{
				const VertexElement* elem = mSoftwareVertexAnimVertexData
					->vertexDeclaration->findElementBySemantic(VES_POSITION);
				HardwareVertexBufferSharedPtr buf = mSoftwareVertexAnimVertexData
					->vertexBufferBinding->getBuffer(elem->getSource());
				buf->suppressHardwareUpdate(false);
			}
			for (SubEntityList::iterator si = mSubEntityList.begin();
				si != mSubEntityList.end(); ++si)
			{
				SubEntity* sub = *si;
				if (!sub->getSubMesh()->useSharedVertices &&
					sub->getSubMesh()->getVertexAnimationType() == VAT_POSE)
				{
					VertexData* data = sub->_getSoftwareVertexAnimVertexData();
					const VertexElement* elem = data->vertexDeclaration
						->findElementBySemantic(VES_POSITION);
					HardwareVertexBufferSharedPtr buf = data
						->vertexBufferBinding->getBuffer(elem->getSource());
					buf->suppressHardwareUpdate(false);
				}
			}
		}

	}
	//-----------------------------------------------------------------------------
	void Entity::markBuffersUnusedForAnimation(void)
	{
		mVertexAnimationAppliedThisFrame = false;
		for (SubEntityList::iterator i = mSubEntityList.begin();
			i != mSubEntityList.end(); ++i)
		{
			(*i)->_markBuffersUnusedForAnimation();
		}
	}
	//-----------------------------------------------------------------------------
	void Entity::_markBuffersUsedForAnimation(void)
	{
		mVertexAnimationAppliedThisFrame = true;
		// no cascade
	}
	//-----------------------------------------------------------------------------
	void Entity::restoreBuffersForUnusedAnimation(bool hardwareAnimation)
	{
		// Rebind original positions if:
		//  We didn't apply any animation and
		//    We're morph animated (hardware binds keyframe, software is missing)
		//    or we're pose animated and software (hardware is fine, still bound)
		if (mMesh->sharedVertexData &&
			!mVertexAnimationAppliedThisFrame &&
			(!hardwareAnimation || mMesh->getSharedVertexDataAnimationType() == VAT_MORPH))
		{
			const VertexElement* srcPosElem =
				mMesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
			HardwareVertexBufferSharedPtr srcBuf =
				mMesh->sharedVertexData->vertexBufferBinding->getBuffer(
					srcPosElem->getSource());

			// Bind to software
			const VertexElement* destPosElem =
				mSoftwareVertexAnimVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
			mSoftwareVertexAnimVertexData->vertexBufferBinding->setBinding(
				destPosElem->getSource(), srcBuf);

		}

		for (SubEntityList::iterator i = mSubEntityList.begin();
			i != mSubEntityList.end(); ++i)
		{
			(*i)->_restoreBuffersForUnusedAnimation(hardwareAnimation);
		}
	}
	//-----------------------------------------------------------------------
	void Entity::_updateAnimation(void)
	{
		// Externally visible method
		if (hasSkeleton() || hasVertexAnimation())
		{
			updateAnimation();
		}
	}
	//-----------------------------------------------------------------------
    bool Entity::_isAnimated(void) const
    {
        return (mAnimationState && mAnimationState->hasEnabledAnimationState()) ||
               (getSkeleton() && getSkeleton()->hasManualBones());
    }
	//-----------------------------------------------------------------------
    bool Entity::_isSkeletonAnimated(void) const
    {
        return getSkeleton() &&
            (mAnimationState->hasEnabledAnimationState() || getSkeleton()->hasManualBones());
    }
	//-----------------------------------------------------------------------
	VertexData* Entity::_getSkelAnimVertexData(void) const
	{
		assert (mSkelAnimVertexData && "Not software skinned or has no shared vertex data!");
        return mSkelAnimVertexData;
	}
	//-----------------------------------------------------------------------
	VertexData* Entity::_getSoftwareVertexAnimVertexData(void) const
	{
		assert (mSoftwareVertexAnimVertexData && "Not vertex animated or has no shared vertex data!");
		return mSoftwareVertexAnimVertexData;
	}
	//-----------------------------------------------------------------------
	VertexData* Entity::_getHardwareVertexAnimVertexData(void) const
	{
		assert (mHardwareVertexAnimVertexData && "Not vertex animated or has no shared vertex data!");
		return mHardwareVertexAnimVertexData;
	}
	//-----------------------------------------------------------------------
	TempBlendedBufferInfo* Entity::_getSkelAnimTempBufferInfo(void)
	{
		return &mTempSkelAnimInfo;
	}
	//-----------------------------------------------------------------------
	TempBlendedBufferInfo* Entity::_getVertexAnimTempBufferInfo(void)
	{
		return &mTempVertexAnimInfo;
	}
    //-----------------------------------------------------------------------
    void Entity::cacheBoneMatrices(void)
    {
        Root& root = Root::getSingleton();
        unsigned long currentFrameNumber = root.getCurrentFrameNumber();
        if (*mFrameBonesLastUpdated  != currentFrameNumber) {

            mSkeletonInstance->setAnimationState(*mAnimationState);
            mSkeletonInstance->_getBoneMatrices(mBoneMatrices);
            *mFrameBonesLastUpdated  = currentFrameNumber;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setDisplaySkeleton(bool display)
    {
        mDisplaySkeleton = display;
    }
    //-----------------------------------------------------------------------
    bool Entity::getDisplaySkeleton(void) const
    {
        return mDisplaySkeleton;
    }
    //-----------------------------------------------------------------------
    Entity* Entity::getManualLodLevel(size_t index) const
    {
        assert(index < mLodEntityList.size());

        return mLodEntityList[index];
    }
    //-----------------------------------------------------------------------
    size_t Entity::getNumManualLodLevels(void) const
    {
        return mLodEntityList.size();
    }
    //-----------------------------------------------------------------------
    void Entity::setMeshLodBias(Real factor, ushort maxDetailIndex, ushort minDetailIndex)
    {
        assert(factor > 0.0f && "Bias factor must be > 0!");
        mMeshLodFactorInv = 1.0f / factor;
        mMaxMeshLodIndex = maxDetailIndex;
        mMinMeshLodIndex = minDetailIndex;

    }
    //-----------------------------------------------------------------------
    void Entity::setMaterialLodBias(Real factor, ushort maxDetailIndex, ushort minDetailIndex)
    {
        assert(factor > 0.0f && "Bias factor must be > 0!");
        mMaterialLodFactorInv = 1.0f / factor;
        mMaxMaterialLodIndex = maxDetailIndex;
        mMinMaterialLodIndex = minDetailIndex;

    }
    //-----------------------------------------------------------------------
    void Entity::buildSubEntityList(MeshPtr& mesh, SubEntityList* sublist)
    {
        // Create SubEntities
        unsigned short i, numSubMeshes;
        SubMesh* subMesh;
        SubEntity* subEnt;

        numSubMeshes = mesh->getNumSubMeshes();
        for (i = 0; i < numSubMeshes; ++i)
        {
            subMesh = mesh->getSubMesh(i);
            subEnt = new SubEntity(this, subMesh);
            if (subMesh->isMatInitialised())
                subEnt->setMaterialName(subMesh->getMaterialName());
            sublist->push_back(subEnt);
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setPolygonModeOverrideable(bool overrideable)
    {
        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();

        for( i = mSubEntityList.begin(); i != iend; ++i )
        {
            (*i)->setPolygonModeOverrideable(overrideable);
        }
    }

    //-----------------------------------------------------------------------
    TagPoint* Entity::attachObjectToBone(const String &boneName, MovableObject *pMovable, const Quaternion &offsetOrientation, const Vector3 &offsetPosition)
    {
        if (mChildObjectList.find(pMovable->getName()) != mChildObjectList.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
                "An object with the name " + pMovable->getName() + " already attached",
                "Entity::attachObjectToBone");
        }
        if(pMovable->isAttached())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Object already attached to a sceneNode or a Bone",
                "Entity::attachObjectToBone");
        }
        if (!hasSkeleton())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "This entity's mesh has no skeleton to attach object to.",
                "Entity::attachObjectToBone");
        }
        Bone* bone = mSkeletonInstance->getBone(boneName);
        if (!bone)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot locate bone named " + boneName,
                "Entity::attachObjectToBone");
        }

        TagPoint *tp = mSkeletonInstance->createTagPointOnBone(
            bone, offsetOrientation, offsetPosition);
        tp->setParentEntity(this);
        tp->setChildObject(pMovable);

        attachObjectImpl(pMovable, tp);

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();

		return tp;
    }

    //-----------------------------------------------------------------------
    void Entity::attachObjectImpl(MovableObject *pObject, TagPoint *pAttachingPoint)
    {
        assert(mChildObjectList.find(pObject->getName()) == mChildObjectList.end());
        mChildObjectList[pObject->getName()] = pObject;
        pObject->_notifyAttached(pAttachingPoint, true);
    }

    //-----------------------------------------------------------------------
    MovableObject* Entity::detachObjectFromBone(const String &name)
    {
        ChildObjectList::iterator i = mChildObjectList.find(name);

        if (i == mChildObjectList.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "No child object entry found named " + name,
                "Entity::detachObjectFromBone");
        }
        MovableObject *obj = i->second;
        detachObjectImpl(obj);
        mChildObjectList.erase(i);

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();

        return obj;
    }
    //-----------------------------------------------------------------------
    void Entity::detachObjectFromBone(MovableObject* obj)
    {
        ChildObjectList::iterator i, iend;
        iend = mChildObjectList.end();
        for (i = mChildObjectList.begin(); i != iend; ++i)
        {
            if (i->second == obj)
            {
                detachObjectImpl(obj);
                mChildObjectList.erase(i);

                // Trigger update of bounding box if necessary
                if (mParentNode)
                    mParentNode->needUpdate();
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::detachAllObjectsFromBone(void)
    {
        detachAllObjectsImpl();

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();
    }
    //-----------------------------------------------------------------------
    void Entity::detachObjectImpl(MovableObject* pObject)
    {
        TagPoint* tp = static_cast<TagPoint*>(pObject->getParentNode());

        // free the TagPoint so we can reuse it later
        mSkeletonInstance->freeTagPoint(tp);

        pObject->_notifyAttached((TagPoint*)0);
    }
    //-----------------------------------------------------------------------
    void Entity::detachAllObjectsImpl(void)
    {
        ChildObjectList::const_iterator i, iend;
        iend = mChildObjectList.end();
        for (i = mChildObjectList.begin(); i != iend; ++i)
        {
            detachObjectImpl(i->second);
        }
        mChildObjectList.clear();
    }

    //-----------------------------------------------------------------------
    Entity::ChildObjectListIterator Entity::getAttachedObjectIterator()
    {
        return ChildObjectListIterator(mChildObjectList.begin(), mChildObjectList.end());
    }
    //-----------------------------------------------------------------------
    Real Entity::getBoundingRadius(void) const
    {
        Real rad = mMesh->getBoundingSphereRadius();
        // Scale by largest scale factor
        if (mParentNode)
        {
            const Vector3& s = mParentNode->_getDerivedScale();
            rad *= std::max(s.x, std::max(s.y, s.z));
        }
        return rad;
    }
    //-----------------------------------------------------------------------
    void Entity::prepareTempBlendBuffers(void)
    {
        if (mSkelAnimVertexData)
        {
            delete mSkelAnimVertexData;
            mSkelAnimVertexData = 0;
        }
		if (mSoftwareVertexAnimVertexData)
		{
			delete mSoftwareVertexAnimVertexData;
			mSoftwareVertexAnimVertexData = 0;
		}
		if (mHardwareVertexAnimVertexData)
		{
			delete mHardwareVertexAnimVertexData;
			mHardwareVertexAnimVertexData = 0;
		}

		if (hasVertexAnimation())
		{
			// Shared data
			if (mMesh->sharedVertexData
				&& mMesh->getSharedVertexDataAnimationType() != VAT_NONE)
			{
				// Create temporary vertex blend info
				// Prepare temp vertex data if needed
				// Clone without copying data, don't remove any blending info
				// (since if we skeletally animate too, we need it)
				mSoftwareVertexAnimVertexData = mMesh->sharedVertexData->clone(false);
				extractTempBufferInfo(mSoftwareVertexAnimVertexData, &mTempVertexAnimInfo);

				// Also clone for hardware usage, don't remove blend info since we'll
				// need it if we also hardware skeletally animate
				mHardwareVertexAnimVertexData = mMesh->sharedVertexData->clone(false);
			}
		}

        if (hasSkeleton())
        {
            // Shared data
            if (mMesh->sharedVertexData)
            {
                // Create temporary vertex blend info
                // Prepare temp vertex data if needed
                // Clone without copying data, remove blending info
                // (since blend is performed in software)
                mSkelAnimVertexData =
                    cloneVertexDataRemoveBlendInfo(mMesh->sharedVertexData);
                extractTempBufferInfo(mSkelAnimVertexData, &mTempSkelAnimInfo);
            }

        }

		// Do SubEntities
		SubEntityList::iterator i, iend;
		iend = mSubEntityList.end();
		for (i = mSubEntityList.begin(); i != iend; ++i)
		{
			SubEntity* s = *i;
			s->prepareTempBlendBuffers();
		}

        // It's prepared for shadow volumes only if mesh has been prepared for shadow volumes.
        mPreparedForShadowVolumes = mMesh->isPreparedForShadowVolumes();
    }
    //-----------------------------------------------------------------------
    void Entity::extractTempBufferInfo(VertexData* sourceData, TempBlendedBufferInfo* info)
    {
        info->extractFrom(sourceData);
    }
    //-----------------------------------------------------------------------
    VertexData* Entity::cloneVertexDataRemoveBlendInfo(const VertexData* source)
    {
        // Clone without copying data
        VertexData* ret = source->clone(false);
        const VertexElement* blendIndexElem =
            source->vertexDeclaration->findElementBySemantic(VES_BLEND_INDICES);
        const VertexElement* blendWeightElem =
            source->vertexDeclaration->findElementBySemantic(VES_BLEND_WEIGHTS);
        // Remove blend index
        if (blendIndexElem)
        {
            // Remove buffer reference
            ret->vertexBufferBinding->unsetBinding(blendIndexElem->getSource());

        }
        if (blendWeightElem &&
            blendWeightElem->getSource() != blendIndexElem->getSource())
        {
            // Remove buffer reference
            ret->vertexBufferBinding->unsetBinding(blendWeightElem->getSource());
        }
        // remove elements from declaration
        ret->vertexDeclaration->removeElement(VES_BLEND_INDICES);
        ret->vertexDeclaration->removeElement(VES_BLEND_WEIGHTS);

        // Copy reference to wcoord buffer
        if (!source->hardwareShadowVolWBuffer.isNull())
            ret->hardwareShadowVolWBuffer = source->hardwareShadowVolWBuffer;

        return ret;
    }
    //-----------------------------------------------------------------------
    EdgeData* Entity::getEdgeList(void)
    {
        // Get from Mesh
        return mMesh->getEdgeList(mMeshLodIndex);
    }
    //-----------------------------------------------------------------------
    void Entity::reevaluateVertexProcessing(void)
    {
        // init
        mHardwareAnimation = false;
        mVertexProgramInUse = false; // assume false because we just assign this
        bool firstPass = true;

        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();
        for (i = mSubEntityList.begin(); i != iend; ++i)
        {
			SubEntity* sub = *i;
            const MaterialPtr& m = sub->getMaterial();
            // Make sure it's loaded
            m->load();
            Technique* t = m->getBestTechnique();
            if (!t)
            {
                // No supported techniques
                continue;
            }
            Pass* p = t->getPass(0);
            if (!p)
            {
                // No passes, invalid
                continue;
            }
            if (p->hasVertexProgram())
            {
                // If one material uses a vertex program, set this flag
                // Causes some special processing like forcing a separate light cap
                mVertexProgramInUse = true;

                if (hasSkeleton())
				{
					// All materials must support skinning for us to consider using
					// hardware animation - if one fails we use software
					if (firstPass)
					{
						mHardwareAnimation = p->getVertexProgram()->isSkeletalAnimationIncluded();
						firstPass = false;
					}
					else
					{
						mHardwareAnimation = mHardwareAnimation &&
							p->getVertexProgram()->isSkeletalAnimationIncluded();
					}
				}

				VertexAnimationType animType = VAT_NONE;
				if (sub->getSubMesh()->useSharedVertices)
				{
					animType = mMesh->getSharedVertexDataAnimationType();
				}
				else
				{
					animType = sub->getSubMesh()->getVertexAnimationType();
				}
				if (animType == VAT_MORPH)
				{
					// All materials must support morph animation for us to consider using
					// hardware animation - if one fails we use software
					if (firstPass)
					{
						mHardwareAnimation = p->getVertexProgram()->isMorphAnimationIncluded();
						firstPass = false;
					}
					else
					{
						mHardwareAnimation = mHardwareAnimation &&
							p->getVertexProgram()->isMorphAnimationIncluded();
					}
				}
				else if (animType == VAT_POSE)
				{
					// All materials must support pose animation for us to consider using
					// hardware animation - if one fails we use software
					if (firstPass)
					{
						mHardwareAnimation = p->getVertexProgram()->isPoseAnimationIncluded();
						if (sub->getSubMesh()->useSharedVertices)
							mHardwarePoseCount = p->getVertexProgram()->getNumberOfPosesIncluded();
						else
							sub->mHardwarePoseCount = p->getVertexProgram()->getNumberOfPosesIncluded();
						firstPass = false;
					}
					else
					{
						mHardwareAnimation = mHardwareAnimation &&
							p->getVertexProgram()->isPoseAnimationIncluded();
						if (sub->getSubMesh()->useSharedVertices)
							mHardwarePoseCount = std::max(mHardwarePoseCount,
								p->getVertexProgram()->getNumberOfPosesIncluded());
						else
							sub->mHardwarePoseCount = std::max(sub->mHardwarePoseCount,
								p->getVertexProgram()->getNumberOfPosesIncluded());
					}
				}

            }
        }

    }
    //-----------------------------------------------------------------------
    ShadowCaster::ShadowRenderableListIterator
        Entity::getShadowVolumeRenderableIterator(
        ShadowTechnique shadowTechnique, const Light* light,
        HardwareIndexBufferSharedPtr* indexBuffer,
        bool extrude, Real extrusionDistance, unsigned long flags)
    {
        assert(indexBuffer && "Only external index buffers are supported right now");
        assert((*indexBuffer)->getType() == HardwareIndexBuffer::IT_16BIT &&
            "Only 16-bit indexes supported for now");

        // Potentially delegate to LOD entity
        if (mMesh->isLodManual() && mMeshLodIndex > 0)
        {
            // Use alternate entity
            assert( static_cast< size_t >( mMeshLodIndex - 1 ) < mLodEntityList.size() &&
                "No LOD EntityList - did you build the manual LODs after creating the entity?");
            // delegate, we're using manual LOD and not the top lod index
            if (hasSkeleton() && mLodEntityList[mMeshLodIndex - 1]->hasSkeleton())
            {
                // Copy the animation state set to lod entity, we assume the lod
                // entity only has a subset animation states
                mAnimationState->copyMatchingState(
					mLodEntityList[mMeshLodIndex - 1]->mAnimationState);
            }
            return mLodEntityList[mMeshLodIndex-1]->getShadowVolumeRenderableIterator(
                shadowTechnique, light, indexBuffer, extrude,
                extrusionDistance, flags);
        }


        // Prepare temp buffers if required
        if (!mPreparedForShadowVolumes)
        {
            mMesh->prepareForShadowVolume();
            // reset frame last updated to force update of animations if they exist
            if (mAnimationState)
                mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber() - 1;
            // re-prepare buffers
            prepareTempBlendBuffers();
        }


        bool hasAnimation = (hasSkeleton() || hasVertexAnimation());

        // Update any animation
        if (hasAnimation)
        {
            updateAnimation();
        }

        // Calculate the object space light details
        Vector4 lightPos = light->getAs4DVector();
        Matrix4 world2Obj = mParentNode->_getFullTransform().inverse();
        lightPos =  world2Obj * lightPos;

        // We need to search the edge list for silhouette edges
        EdgeData* edgeList = getEdgeList();

		if (!edgeList)
		{
			// we can't get an edge list for some reason, return blank
			// really we shouldn't be able to get here, but this is a safeguard
			return ShadowRenderableListIterator(mShadowRenderables.begin(), mShadowRenderables.end());
		}

        // Init shadow renderable list if required
        bool init = mShadowRenderables.empty();

        EdgeData::EdgeGroupList::iterator egi;
        ShadowRenderableList::iterator si, siend;
        EntityShadowRenderable* esr = 0;
        if (init)
            mShadowRenderables.resize(edgeList->edgeGroups.size());

        bool isAnimated = hasAnimation;
        bool updatedSharedGeomNormals = false;
        siend = mShadowRenderables.end();
        egi = edgeList->edgeGroups.begin();
        for (si = mShadowRenderables.begin(); si != siend; ++si, ++egi)
        {
            const VertexData *pVertData;
            if (isAnimated)
            {
                // Use temp buffers
                pVertData = findBlendedVertexData(egi->vertexData);
            }
            else
            {
                pVertData = egi->vertexData;
            }
            if (init)
            {
                // Try to find corresponding SubEntity; this allows the
                // linkage of visibility between ShadowRenderable and SubEntity
                SubEntity* subent = findSubEntityForVertexData(egi->vertexData);
                // Create a new renderable, create a separate light cap if
                // we're using a vertex program (either for this model, or
                // for extruding the shadow volume) since otherwise we can
                // get depth-fighting on the light cap

                *si = new EntityShadowRenderable(this, indexBuffer, pVertData,
                    mVertexProgramInUse || !extrude, subent);
            }
            else
            {
                // If we have animation, we have no guarantee that the position
                // buffer we used last frame is the same one we used last frame
                // since a temporary buffer is requested each frame
                // therefore, we need to update the EntityShadowRenderable
                // with the current position buffer
                static_cast<EntityShadowRenderable*>(*si)->rebindPositionBuffer(pVertData, hasAnimation);

            }
            // Get shadow renderable
            esr = static_cast<EntityShadowRenderable*>(*si);
            HardwareVertexBufferSharedPtr esrPositionBuffer = esr->getPositionBuffer();
            // For animated entities we need to recalculate the face normals
            if (hasAnimation)
            {
                if (egi->vertexData != mMesh->sharedVertexData || !updatedSharedGeomNormals)
                {
                    // recalculate face normals
                    edgeList->updateFaceNormals(egi->vertexSet, esrPositionBuffer);
                    // If we're not extruding in software we still need to update
                    // the latter part of the buffer (the hardware extruded part)
                    // with the latest animated positions
                    if (!extrude)
                    {
                        // Lock, we'll be locking the (suppressed hardware update) shadow buffer
                        float* pSrc = static_cast<float*>(
                            esrPositionBuffer->lock(HardwareBuffer::HBL_NORMAL));
                        float* pDest = pSrc + (egi->vertexData->vertexCount * 3);
                        memcpy(pDest, pSrc, sizeof(float) * 3 * egi->vertexData->vertexCount);
                        esrPositionBuffer->unlock();
                    }
                    if (egi->vertexData == mMesh->sharedVertexData)
                    {
                        updatedSharedGeomNormals = true;
                    }
                }
            }
            // Extrude vertices in software if required
            if (extrude)
            {
                extrudeVertices(esrPositionBuffer,
                    egi->vertexData->vertexCount,
                    lightPos, extrusionDistance);

            }
            // Stop suppressing hardware update now, if we were
            esrPositionBuffer->suppressHardwareUpdate(false);

        }
        // Calc triangle light facing
        updateEdgeListLightFacing(edgeList, lightPos);

        // Generate indexes and update renderables
        generateShadowVolume(edgeList, *indexBuffer, light,
            mShadowRenderables, flags);


        return ShadowRenderableListIterator(mShadowRenderables.begin(), mShadowRenderables.end());
    }
    //-----------------------------------------------------------------------
    const VertexData* Entity::findBlendedVertexData(const VertexData* orig)
    {
		bool skel = hasSkeleton();

        if (orig == mMesh->sharedVertexData)
        {
			return skel? mSkelAnimVertexData : mSoftwareVertexAnimVertexData;
        }
        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();
        for (i = mSubEntityList.begin(); i != iend; ++i)
        {
            SubEntity* se = *i;
            if (orig == se->getSubMesh()->vertexData)
            {
				return skel? se->_getSkelAnimVertexData() : se->_getSoftwareVertexAnimVertexData();
            }
        }
        // None found
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
            "Cannot find blended version of the vertex data specified.",
            "Entity::findBlendedVertexData");
    }
    //-----------------------------------------------------------------------
    SubEntity* Entity::findSubEntityForVertexData(const VertexData* orig)
    {
        if (orig == mMesh->sharedVertexData)
        {
            return 0;
        }

        SubEntityList::iterator i, iend;
        iend = mSubEntityList.end();
        for (i = mSubEntityList.begin(); i != iend; ++i)
        {
            SubEntity* se = *i;
            if (orig == se->getSubMesh()->vertexData)
            {
                return se;
            }
        }

        // None found
        return 0;
    }
    //-----------------------------------------------------------------------
    void Entity::addSoftwareAnimationRequest(bool normalsAlso)
    {
        mSoftwareAnimationRequests++;
        if (normalsAlso) {
            mSoftwareAnimationNormalsRequests++;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::removeSoftwareAnimationRequest(bool normalsAlso)
    {
        if (mSoftwareAnimationRequests == 0 ||
            (normalsAlso && mSoftwareAnimationNormalsRequests == 0))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Attempt to remove nonexistant request.",
                        "Entity::removeSoftwareAnimationRequest");
        }
        mSoftwareAnimationRequests--;
        if (normalsAlso) {
            mSoftwareAnimationNormalsRequests--;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::_notifyAttached(Node* parent, bool isTagPoint)
    {
        MovableObject::_notifyAttached(parent, isTagPoint);
        // Also notify LOD entities
        LODEntityList::iterator i, iend;
        iend = mLodEntityList.end();
        for (i = mLodEntityList.begin(); i != iend; ++i)
        {
            (*i)->_notifyAttached(parent, isTagPoint);
        }

    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    Entity::EntityShadowRenderable::EntityShadowRenderable(Entity* parent,
        HardwareIndexBufferSharedPtr* indexBuffer, const VertexData* vertexData,
        bool createSeparateLightCap, SubEntity* subent, bool isLightCap)
        : mParent(parent), mSubEntity(subent)
    {
        // Save link to vertex data
        mCurrentVertexData = vertexData;

        // Initialise render op
        mRenderOp.indexData = new IndexData();
        mRenderOp.indexData->indexBuffer = *indexBuffer;
        mRenderOp.indexData->indexStart = 0;
        // index start and count are sorted out later

        // Create vertex data which just references position component (and 2 component)
        mRenderOp.vertexData = new VertexData();
        mRenderOp.vertexData->vertexDeclaration =
            HardwareBufferManager::getSingleton().createVertexDeclaration();
        mRenderOp.vertexData->vertexBufferBinding =
            HardwareBufferManager::getSingleton().createVertexBufferBinding();
        // Map in position data
        mRenderOp.vertexData->vertexDeclaration->addElement(0,0,VET_FLOAT3, VES_POSITION);
        mOriginalPosBufferBinding =
            vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
        mPositionBuffer = vertexData->vertexBufferBinding->getBuffer(mOriginalPosBufferBinding);
        mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mPositionBuffer);
        // Map in w-coord buffer (if present)
        if(!vertexData->hardwareShadowVolWBuffer.isNull())
        {
            mRenderOp.vertexData->vertexDeclaration->addElement(1,0,VET_FLOAT1, VES_TEXTURE_COORDINATES, 0);
            mWBuffer = vertexData->hardwareShadowVolWBuffer;
            mRenderOp.vertexData->vertexBufferBinding->setBinding(1, mWBuffer);
        }
        // Use same vertex start as input
        mRenderOp.vertexData->vertexStart = vertexData->vertexStart;

        if (isLightCap)
        {
            // Use original vertex count, no extrusion
            mRenderOp.vertexData->vertexCount = vertexData->vertexCount;
        }
        else
        {
            // Vertex count must take into account the doubling of the buffer,
            // because second half of the buffer is the extruded copy
            mRenderOp.vertexData->vertexCount =
                vertexData->vertexCount * 2;
            if (createSeparateLightCap)
            {
                // Create child light cap
                mLightCap = new EntityShadowRenderable(parent,
                    indexBuffer, vertexData, false, subent, true);
            }
        }

    }
    //-----------------------------------------------------------------------
    Entity::EntityShadowRenderable::~EntityShadowRenderable()
    {
        delete mRenderOp.indexData;
        delete mRenderOp.vertexData;
    }
    //-----------------------------------------------------------------------
    void Entity::EntityShadowRenderable::getWorldTransforms(Matrix4* xform) const
    {
        *xform = mParent->_getParentNodeFullTransform();
    }
    //-----------------------------------------------------------------------
    const Quaternion& Entity::EntityShadowRenderable::getWorldOrientation(void) const
    {
        return mParent->getParentNode()->_getDerivedOrientation();
    }
    //-----------------------------------------------------------------------
    const Vector3& Entity::EntityShadowRenderable::getWorldPosition(void) const
    {
        return mParent->getParentNode()->_getDerivedPosition();
    }
    //-----------------------------------------------------------------------
    void Entity::EntityShadowRenderable::rebindPositionBuffer(const VertexData* vertexData, bool force)
    {
        if (force || mCurrentVertexData != vertexData)
        {
            mCurrentVertexData = vertexData;
            mPositionBuffer = mCurrentVertexData->vertexBufferBinding->getBuffer(
                mOriginalPosBufferBinding);
            mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mPositionBuffer);
            if (mLightCap)
            {
                static_cast<EntityShadowRenderable*>(mLightCap)->rebindPositionBuffer(vertexData, force);
            }
        }
    }
    //-----------------------------------------------------------------------
    bool Entity::EntityShadowRenderable::isVisible(void) const
    {
        if (mSubEntity)
        {
            return mSubEntity->isVisible();
        }
        else
        {
            return ShadowRenderable::isVisible();
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setRenderQueueGroup(uint8 queueID)
    {
        MovableObject::setRenderQueueGroup(queueID);

        // Set render queue for all manual LOD entities
        if (mMesh->isLodManual())
        {
            LODEntityList::iterator li, liend;
            liend = mLodEntityList.end();
            for (li = mLodEntityList.begin(); li != liend; ++li)
            {
                (*li)->setRenderQueueGroup(queueID);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::shareSkeletonInstanceWith(Entity* entity)
    {
        if (entity->getMesh()->getSkeleton() != getMesh()->getSkeleton())
        {
            OGRE_EXCEPT(Exception::ERR_RT_ASSERTION_FAILED,
                "The supplied entity has a different skeleton.",
                "Entity::shareSkeletonWith");
        }
        if (!mSkeletonInstance)
        {
            OGRE_EXCEPT(Exception::ERR_RT_ASSERTION_FAILED,
                "This entity has no skeleton.",
                "Entity::shareSkeletonWith");
        }
        if (mSharedSkeletonEntities != NULL && entity->mSharedSkeletonEntities != NULL)
        {
            OGRE_EXCEPT(Exception::ERR_RT_ASSERTION_FAILED,
                "Both entities already shares their SkeletonInstances! At least "
                "one of the instances must not share it's instance.",
                "Entity::shareSkeletonWith");
        }

        //check if we already share our skeletoninstance, we don't want to delete it if so
        if (mSharedSkeletonEntities != NULL)
        {
            entity->shareSkeletonInstanceWith(this);
        }
        else
        {
            delete mSkeletonInstance;
            delete [] mBoneMatrices;
            delete mAnimationState;
            delete mFrameBonesLastUpdated;
            mSkeletonInstance = entity->mSkeletonInstance;
            mNumBoneMatrices = entity->mNumBoneMatrices;
            mBoneMatrices = entity->mBoneMatrices;
            mAnimationState = entity->mAnimationState;
            mFrameBonesLastUpdated = entity->mFrameBonesLastUpdated;
            if (entity->mSharedSkeletonEntities == NULL)
            {
                entity->mSharedSkeletonEntities = new EntitySet();
                entity->mSharedSkeletonEntities->insert(entity);
            }
            mSharedSkeletonEntities = entity->mSharedSkeletonEntities;
            mSharedSkeletonEntities->insert(this);
        }

		if (mSkelAnimVertexData)
		{
			delete mSkelAnimVertexData;
			mSkelAnimVertexData = 0;
		}
		if (mHardwareVertexAnimVertexData)
		{
			delete mHardwareVertexAnimVertexData;
			mHardwareVertexAnimVertexData = 0;
		}
		if (mSoftwareVertexAnimVertexData)
		{
			delete mSoftwareVertexAnimVertexData;
			mSoftwareVertexAnimVertexData = 0;
		}


    }
    //-----------------------------------------------------------------------
    void Entity::stopSharingSkeletonInstance()
    {
        if (mSharedSkeletonEntities == NULL)
        {
            OGRE_EXCEPT(Exception::ERR_RT_ASSERTION_FAILED,
                "This entity is not sharing it's skeletoninstance.",
                "Entity::shareSkeletonWith");
        }
        //check if there's no other than us sharing the skeleton instance
        if (mSharedSkeletonEntities->size() == 1)
        {
            //just reset
            delete mSharedSkeletonEntities;
            mSharedSkeletonEntities = 0;
        }
        else
        {
            mSkeletonInstance = new SkeletonInstance(mMesh->getSkeleton());
            mSkeletonInstance->load();
            mAnimationState = new AnimationStateSet();
            mMesh->_initAnimationState(mAnimationState);
            mFrameBonesLastUpdated = new unsigned long(std::numeric_limits<unsigned long>::max());
            mNumBoneMatrices = mSkeletonInstance->getNumBones();
            mBoneMatrices = new Matrix4[mNumBoneMatrices];
            prepareTempBlendBuffers();

            mSharedSkeletonEntities->erase(this);
            if (mSharedSkeletonEntities->size() == 1)
            {
                (*mSharedSkeletonEntities->begin())->stopSharingSkeletonInstance();
            }
            mSharedSkeletonEntities = 0;
        }
    }
    //-----------------------------------------------------------------------
	void Entity::refreshAvailableAnimationState(void)
	{
		if (hasSkeleton())
		{
			mSkeletonInstance->_refreshAnimationState(mAnimationState);
		}
	}
	//-----------------------------------------------------------------------
	uint32 Entity::getTypeFlags(void) const
	{
		return SceneManager::ENTITY_TYPE_MASK;
	}
	//-----------------------------------------------------------------------
	VertexData* Entity::getVertexDataForBinding(void)
	{
		Entity::VertexDataBindChoice c =
			chooseVertexDataForBinding(mMesh->getSharedVertexDataAnimationType() != VAT_NONE);
		switch(c)
		{
		case BIND_ORIGINAL:
			return mMesh->sharedVertexData;
		case BIND_HARDWARE_MORPH:
			return mHardwareVertexAnimVertexData;
		case BIND_SOFTWARE_MORPH:
			return mSoftwareVertexAnimVertexData;
		case BIND_SOFTWARE_SKELETAL:
			return mSkelAnimVertexData;
		};
		// keep compiler happy
		return mMesh->sharedVertexData;
	}
	//-----------------------------------------------------------------------
	Entity::VertexDataBindChoice Entity::chooseVertexDataForBinding(bool vertexAnim) const
	{
		if (hasSkeleton())
		{
			if (!mHardwareAnimation)
			{
				// all software skeletal binds same vertex data
				// may be a 2-stage s/w transform including morph earlier though
				return BIND_SOFTWARE_SKELETAL;
			}
			else if (vertexAnim)
			{
				// hardware morph animation
				return BIND_HARDWARE_MORPH;
			}
			else
			{
				// hardware skeletal, no morphing
				return BIND_ORIGINAL;
			}
		}
		else if (vertexAnim)
		{
			// morph only, no skeletal
			if (mHardwareAnimation)
			{
				return BIND_HARDWARE_MORPH;
			}
			else
			{
				return BIND_SOFTWARE_MORPH;
			}

		}
		else
		{
			return BIND_ORIGINAL;
		}

	}
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	String EntityFactory::FACTORY_TYPE_NAME = "Entity";
	//-----------------------------------------------------------------------
	const String& EntityFactory::getType(void) const
	{
		return FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------
	MovableObject* EntityFactory::createInstanceImpl( const String& name,
		const NameValuePairList* params)
	{
		// must have mesh parameter
		MeshPtr pMesh;
		if (params != 0)
		{
			NameValuePairList::const_iterator ni = params->find("mesh");
			if (ni != params->end())
			{
				// Get mesh (load if required)
				pMesh = MeshManager::getSingleton().load(
					ni->second,
					// autodetect group location
					ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
			}

		}
		if (pMesh.isNull())
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"'mesh' parameter required when constructing an Entity.",
				"EntityFactory::createInstance");
		}

		return new Entity(name, pMesh);

	}
	//-----------------------------------------------------------------------
	void EntityFactory::destroyInstance( MovableObject* obj)
	{
		delete obj;
	}


}
