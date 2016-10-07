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
#include "OgreSubEntity.h"

#include "OgreEntity.h"
#include "OgreSkeletonInstance.h"
#include "OgreSceneManager.h"
#include "OgreMaterialManager.h"
#include "OgreSubMesh.h"
#include "OgreTagPoint.h"
#include "OgreLogManager.h"
#include "OgreMesh.h"
#include "OgreException.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    SubEntity::SubEntity (Entity* parent, SubMesh* subMeshBasis)
        : Renderable(), mParentEntity(parent), mMaterialName("BaseWhite"),
		mSubMesh(subMeshBasis)
    {
        mpMaterial = MaterialManager::getSingleton().getByName(mMaterialName);
        mMaterialLodIndex = 0;
        mVisible = true;
        mSkelAnimVertexData = 0;
		mSoftwareVertexAnimVertexData = 0;
		mHardwareVertexAnimVertexData = 0;
		mHardwarePoseCount = 0;



    }
    //-----------------------------------------------------------------------
    SubEntity::~SubEntity()
    {
        if (mSkelAnimVertexData)
            delete mSkelAnimVertexData;
		if (mHardwareVertexAnimVertexData)
			delete mHardwareVertexAnimVertexData;
		if (mSoftwareVertexAnimVertexData)
			delete mSoftwareVertexAnimVertexData;
    }
    //-----------------------------------------------------------------------
    SubMesh* SubEntity::getSubMesh(void)
    {
        return mSubMesh;
    }
    //-----------------------------------------------------------------------
    const String& SubEntity::getMaterialName(void) const
    {
        return mMaterialName;
    }
    //-----------------------------------------------------------------------
    void SubEntity::setMaterialName( const String& name)
    {

        //String oldName = mMaterialName;
        mMaterialName = name;
        mpMaterial = MaterialManager::getSingleton().getByName(mMaterialName);

        if (mpMaterial.isNull())
        {
            LogManager::getSingleton().logMessage("Can't assign material " + name + 
                " to SubEntity of " + mParentEntity->getName() + " because this "
                "Material does not exist. Have you forgotten to define it in a "
                ".material script?");
            mpMaterial = MaterialManager::getSingleton().getByName("BaseWhite");
            if (mpMaterial.isNull())
            {
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Can't assign default material "
                    "to SubEntity of " + mParentEntity->getName() + ". Did "
                    "you forget to call MaterialManager::initialise()?",
                    "SubEntity.setMaterialName");
            }
        }
        // Ensure new material loaded (will not load again if already loaded)
        mpMaterial->load();

        // tell parent to reconsider material vertex processing options
        mParentEntity->reevaluateVertexProcessing();


    }
    //-----------------------------------------------------------------------
    const MaterialPtr& SubEntity::getMaterial(void) const
    {
        return mpMaterial;
    }
    //-----------------------------------------------------------------------
    Technique* SubEntity::getTechnique(void) const
    {
        return mpMaterial->getBestTechnique(mMaterialLodIndex);
    }
    //-----------------------------------------------------------------------
    void SubEntity::getRenderOperation(RenderOperation& op)
    {
		// Use LOD
        mSubMesh->_getRenderOperation(op, mParentEntity->mMeshLodIndex);
		// Deal with any vertex data overrides
		op.vertexData = getVertexDataForBinding();

    }
	//-----------------------------------------------------------------------
	VertexData* SubEntity::getVertexDataForBinding(void)
	{
		if (mSubMesh->useSharedVertices)
		{
			return mParentEntity->getVertexDataForBinding();
		}
		else
		{
			Entity::VertexDataBindChoice c = 
				mParentEntity->chooseVertexDataForBinding(
					mSubMesh->getVertexAnimationType() != VAT_NONE);
			switch(c)
			{
			case Entity::BIND_ORIGINAL:
				return mSubMesh->vertexData;
			case Entity::BIND_HARDWARE_MORPH:
				return mHardwareVertexAnimVertexData;
			case Entity::BIND_SOFTWARE_MORPH:
				return mSoftwareVertexAnimVertexData;
			case Entity::BIND_SOFTWARE_SKELETAL:
				return mSkelAnimVertexData;
			};
			// keep compiler happy
			return mSubMesh->vertexData;

		}
	}
    //-----------------------------------------------------------------------
    void SubEntity::getWorldTransforms(Matrix4* xform) const
    {
        if (!mParentEntity->mNumBoneMatrices ||
            !mParentEntity->isHardwareAnimationEnabled())
        {
            // No skeletal animation, or software skinning
            *xform = mParentEntity->_getParentNodeFullTransform();
        }
        else
        {
            // Hardware skinning, pass all actually used matrices
            const Mesh::IndexMap& indexMap = mSubMesh->useSharedVertices ?
                mSubMesh->parent->sharedBlendIndexToBoneIndexMap : mSubMesh->blendIndexToBoneIndexMap;
            assert(indexMap.size() <= mParentEntity->mNumBoneMatrices);

            if (mParentEntity->_isSkeletonAnimated())
            {
                // Bones, use cached matrices built when Entity::_updateRenderQueue was called
                assert(mParentEntity->mBoneWorldMatrices);

                Mesh::IndexMap::const_iterator it, itend;
                itend = indexMap.end();
                for (it = indexMap.begin(); it != itend; ++it, ++xform)
                {
                    *xform = mParentEntity->mBoneWorldMatrices[*it];
                }
            }
            else
            {
                // All animations disabled, use parent entity world transform only
                std::fill_n(xform, indexMap.size(), mParentEntity->_getParentNodeFullTransform());
            }
        }
    }
    //-----------------------------------------------------------------------
    const Quaternion& SubEntity::getWorldOrientation(void) const
    {
        return mParentEntity->mParentNode->_getDerivedOrientation();
    }
    //-----------------------------------------------------------------------
    const Vector3& SubEntity::getWorldPosition(void) const
    {
        return mParentEntity->mParentNode->_getDerivedPosition();
    }

    //-----------------------------------------------------------------------
    unsigned short SubEntity::getNumWorldTransforms(void) const
    {
        if (!mParentEntity->mNumBoneMatrices ||
            !mParentEntity->isHardwareAnimationEnabled())
        {
            // No skeletal animation, or software skinning
            return 1;
        }
        else
        {
            // Hardware skinning, pass all actually used matrices
            const Mesh::IndexMap& indexMap = mSubMesh->useSharedVertices ?
                mSubMesh->parent->sharedBlendIndexToBoneIndexMap : mSubMesh->blendIndexToBoneIndexMap;
            assert(indexMap.size() <= mParentEntity->mNumBoneMatrices);

            return static_cast<unsigned short>(indexMap.size());
        }
    }
    //-----------------------------------------------------------------------
    Real SubEntity::getSquaredViewDepth(const Camera* cam) const
    {
        Node* n = mParentEntity->getParentNode();
        assert(n);
        return n->getSquaredViewDepth(cam);
    }
    //-----------------------------------------------------------------------
    bool SubEntity::getNormaliseNormals(void) const
    {
        return mParentEntity->mNormaliseNormals;
    }
    //-----------------------------------------------------------------------
    const LightList& SubEntity::getLights(void) const
    {
        SceneNode* n = mParentEntity->getParentSceneNode();
        assert(n);
        return n->findLights(mParentEntity->getBoundingRadius());
    }
    //-----------------------------------------------------------------------
    void SubEntity::setVisible(bool visible)
    {
        mVisible = visible;
    }
    //-----------------------------------------------------------------------
    bool SubEntity::isVisible(void) const
    {
        return mVisible;

    }
    //-----------------------------------------------------------------------
    void SubEntity::prepareTempBlendBuffers(void)
    {
		if (mSubMesh->useSharedVertices)
			return;

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

		if (!mSubMesh->useSharedVertices)
		{
			if (mSubMesh->getVertexAnimationType() != VAT_NONE)
			{
				// Create temporary vertex blend info
				// Prepare temp vertex data if needed
				// Clone without copying data, don't remove any blending info
				// (since if we skeletally animate too, we need it)
				mSoftwareVertexAnimVertexData = mSubMesh->vertexData->clone(false);
				mParentEntity->extractTempBufferInfo(mSoftwareVertexAnimVertexData, &mTempVertexAnimInfo);

				// Also clone for hardware usage, don't remove blend info since we'll
				// need it if we also hardware skeletally animate
				mHardwareVertexAnimVertexData = mSubMesh->vertexData->clone(false);
			}

			if (mParentEntity->hasSkeleton())
			{
				// Create temporary vertex blend info
				// Prepare temp vertex data if needed
				// Clone without copying data, remove blending info
				// (since blend is performed in software)
				mSkelAnimVertexData = 
					mParentEntity->cloneVertexDataRemoveBlendInfo(mSubMesh->vertexData);
				mParentEntity->extractTempBufferInfo(mSkelAnimVertexData, &mTempSkelAnimInfo);

			}
		}
    }
    //-----------------------------------------------------------------------
    bool SubEntity::getCastsShadows(void) const
    {
        return mParentEntity->getCastShadows();
    }
	//-----------------------------------------------------------------------
	VertexData* SubEntity::_getSkelAnimVertexData(void) 
	{
		assert (mSkelAnimVertexData && "Not software skinned or has no dedicated geometry!");
		return mSkelAnimVertexData;
	}
	//-----------------------------------------------------------------------
	VertexData* SubEntity::_getSoftwareVertexAnimVertexData(void)
	{
		assert (mSoftwareVertexAnimVertexData && "Not vertex animated or has no dedicated geometry!");
		return mSoftwareVertexAnimVertexData;
	}
	//-----------------------------------------------------------------------
	VertexData* SubEntity::_getHardwareVertexAnimVertexData(void)
	{
		assert (mHardwareVertexAnimVertexData && "Not vertex animated or has no dedicated geometry!");
		return mHardwareVertexAnimVertexData;
	}
	//-----------------------------------------------------------------------
	TempBlendedBufferInfo* SubEntity::_getSkelAnimTempBufferInfo(void) 
	{
		return &mTempSkelAnimInfo;
	}
	//-----------------------------------------------------------------------
	TempBlendedBufferInfo* SubEntity::_getVertexAnimTempBufferInfo(void) 
	{
		return &mTempVertexAnimInfo;
	}
	//-----------------------------------------------------------------------
	void SubEntity::_updateCustomGpuParameter(
		const GpuProgramParameters::AutoConstantEntry& constantEntry,
		GpuProgramParameters* params) const
	{
		if (constantEntry.paramType == GpuProgramParameters::ACT_ANIMATION_PARAMETRIC)
		{
			// Set up to 4 values, or up to limit of hardware animation entries
			// Pack into 4-element constants offset based on constant data index
			// If there are more than 4 entries, this will be called more than once
			Vector4 val(0.0f,0.0f,0.0f,0.0f);

			size_t animIndex = constantEntry.data * 4;
			for (size_t i = 0; i < 4 && 
				animIndex < mHardwareVertexAnimVertexData->hwAnimationDataList.size();
				++i, ++animIndex)
			{
				val.val[i] = 
					mHardwareVertexAnimVertexData->hwAnimationDataList[animIndex].parametric;
			}
			// set the parametric morph value
			params->setConstant(constantEntry.index, val);
		}
		else
		{
			// default
			return Renderable::_updateCustomGpuParameter(constantEntry, params);
		}
	}
	//-----------------------------------------------------------------------------
	void SubEntity::_markBuffersUnusedForAnimation(void)
	{
		mVertexAnimationAppliedThisFrame = false;
	}
	//-----------------------------------------------------------------------------
	void SubEntity::_markBuffersUsedForAnimation(void)
	{
		mVertexAnimationAppliedThisFrame = true;
	}
	//-----------------------------------------------------------------------------
	void SubEntity::_restoreBuffersForUnusedAnimation(bool hardwareAnimation)
	{
		// Rebind original positions if:
		//  We didn't apply any animation and 
		//    We're morph animated (hardware binds keyframe, software is missing)
		//    or we're pose animated and software (hardware is fine, still bound)
		if (mSubMesh->getVertexAnimationType() != VAT_NONE && 
			!mSubMesh->useSharedVertices && 
			!mVertexAnimationAppliedThisFrame &&
			(!hardwareAnimation || mSubMesh->getVertexAnimationType() == VAT_MORPH))
		{
			const VertexElement* srcPosElem = 
				mSubMesh->vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
			HardwareVertexBufferSharedPtr srcBuf = 
				mSubMesh->vertexData->vertexBufferBinding->getBuffer(
				srcPosElem->getSource());

			// Bind to software
			const VertexElement* destPosElem = 
				mSoftwareVertexAnimVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
			mSoftwareVertexAnimVertexData->vertexBufferBinding->setBinding(
				destPosElem->getSource(), srcBuf);

		}
	}


}
