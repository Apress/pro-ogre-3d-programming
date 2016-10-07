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
#include "OgreAnimationTrack.h"
#include "OgreAnimation.h"
#include "OgreKeyFrame.h"
#include "OgreNode.h"
#include "OgreLogManager.h"
#include "OgreHardwareBufferManager.h"
#include "OgreMesh.h"
#include "OgreException.h"

namespace Ogre {

    namespace {
        // Locally key frame search helper
        struct KeyFrameTimeLess
        {
            bool operator() (const KeyFrame* kf, const KeyFrame* kf2) const
            {
                return kf->getTime() < kf2->getTime();
            }
        };
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    AnimationTrack::AnimationTrack(Animation* parent, unsigned short handle) :
		mParent(parent), mHandle(handle)
    {
    }
    //---------------------------------------------------------------------
    AnimationTrack::~AnimationTrack()
    {
        removeAllKeyFrames();
    }
    //---------------------------------------------------------------------
    unsigned short AnimationTrack::getNumKeyFrames(void) const
    {
        return (unsigned short)mKeyFrames.size();
    }
    //---------------------------------------------------------------------
    KeyFrame* AnimationTrack::getKeyFrame(unsigned short index) const
    {
		// If you hit this assert, then the keyframe index is out of bounds
        assert( index < (ushort)mKeyFrames.size() );

        return mKeyFrames[index];
    }
    //---------------------------------------------------------------------
    Real AnimationTrack::getKeyFramesAtTime(Real timePos, KeyFrame** keyFrame1, KeyFrame** keyFrame2,
            unsigned short* firstKeyIndex) const
    {
        Real totalAnimationLength = mParent->getLength();

        // Wrap time
        while (timePos > totalAnimationLength)
        {
            timePos -= totalAnimationLength;
        }

        // Parametric time
        // t1 = time of previous keyframe
        // t2 = time of next keyframe
        Real t1, t2;

        // Find first keyframe after or on current time
		KeyFrame timeKey(0, timePos);
        KeyFrameList::const_iterator i =
            std::lower_bound(mKeyFrames.begin(), mKeyFrames.end(), &timeKey, KeyFrameTimeLess());

        if (i == mKeyFrames.end())
        {
            // There is no keyframe after this time, wrap back to first
            *keyFrame2 = mKeyFrames.front();
            t2 = totalAnimationLength + (*keyFrame2)->getTime();

            // Use last keyframe as previous keyframe
            --i;
        }
        else
        {
            *keyFrame2 = *i;
            t2 = (*keyFrame2)->getTime();

            // Find last keyframe before or on current time
            if (i != mKeyFrames.begin() && timePos < (*i)->getTime())
            {
                --i;
            }
        }

        // Fill index of the first key
        if (firstKeyIndex)
        {
            *firstKeyIndex = std::distance(mKeyFrames.begin(), i);
        }

        *keyFrame1 = *i;

        t1 = (*keyFrame1)->getTime();

        if (t1 == t2)
        {
            // Same KeyFrame (only one)
            return 0.0;
        }
        else
        {
            return (timePos - t1) / (t2 - t1);
        }
    }
    //---------------------------------------------------------------------
    KeyFrame* AnimationTrack::createKeyFrame(Real timePos)
    {
        KeyFrame* kf = createKeyFrameImpl(timePos);

        // Insert just before upper bound
        KeyFrameList::iterator i =
            std::upper_bound(mKeyFrames.begin(), mKeyFrames.end(), kf, KeyFrameTimeLess());
        mKeyFrames.insert(i, kf);

        _keyFrameDataChanged();

        return kf;

    }
    //---------------------------------------------------------------------
    void AnimationTrack::removeKeyFrame(unsigned short index)
    {
		// If you hit this assert, then the keyframe index is out of bounds
        assert( index < (ushort)mKeyFrames.size() );

        KeyFrameList::iterator i = mKeyFrames.begin();

        i += index;

        delete *i;

        mKeyFrames.erase(i);

        _keyFrameDataChanged();


    }
    //---------------------------------------------------------------------
    void AnimationTrack::removeAllKeyFrames(void)
    {
        KeyFrameList::iterator i = mKeyFrames.begin();

        for (; i != mKeyFrames.end(); ++i)
        {
            delete *i;
        }

        _keyFrameDataChanged();

        mKeyFrames.clear();

    }
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	// Numeric specialisations
	//---------------------------------------------------------------------
	NumericAnimationTrack::NumericAnimationTrack(Animation* parent,
		unsigned short handle)
		: AnimationTrack(parent, handle)
	{
	}
	//---------------------------------------------------------------------
	NumericAnimationTrack::NumericAnimationTrack(Animation* parent,
		unsigned short handle, AnimableValuePtr& target)
		:AnimationTrack(parent, handle), mTargetAnim(target)
	{
	}
	//---------------------------------------------------------------------
	const AnimableValuePtr& NumericAnimationTrack::getAssociatedAnimable(void) const
	{
		return mTargetAnim;
	}
	//---------------------------------------------------------------------
	void NumericAnimationTrack::setAssociatedAnimable(const AnimableValuePtr& val)
	{
		mTargetAnim = val;
	}
	//---------------------------------------------------------------------
	KeyFrame* NumericAnimationTrack::createKeyFrameImpl(Real time)
	{
		return new NumericKeyFrame(this, time);
	}
	//---------------------------------------------------------------------
	void NumericAnimationTrack::getInterpolatedKeyFrame(Real timeIndex,
		KeyFrame* kf) const
	{
		NumericKeyFrame* kret = static_cast<NumericKeyFrame*>(kf);

        // Keyframe pointers
		KeyFrame *kBase1, *kBase2;
        NumericKeyFrame *k1, *k2;
        unsigned short firstKeyIndex;

        Real t = this->getKeyFramesAtTime(timeIndex, &kBase1, &kBase2, &firstKeyIndex);
		k1 = static_cast<NumericKeyFrame*>(kBase1);
		k2 = static_cast<NumericKeyFrame*>(kBase2);

        if (t == 0.0)
        {
            // Just use k1
            kret->setValue(k1->getValue());
        }
        else
        {
            // Interpolate by t
			AnyNumeric diff = k2->getValue() - k1->getValue();
			kret->setValue(k1->getValue() + diff * t);
        }
	}
	//---------------------------------------------------------------------
	void NumericAnimationTrack::apply(Real timePos, Real weight, bool accumulate,
		Real scale)
	{
		applyToAnimable(mTargetAnim, timePos, weight, scale);
	}
	//---------------------------------------------------------------------
	void NumericAnimationTrack::applyToAnimable(const AnimableValuePtr& anim, Real timePos,
		Real weight, Real scale)
	{
		// Nothing to do if no keyframes
		if (mKeyFrames.empty())
			return;

		NumericKeyFrame kf(0, timePos);
		getInterpolatedKeyFrame(timePos, &kf);
		// add to existing. Weights are not relative, but treated as
		// absolute multipliers for the animation
		AnyNumeric val = kf.getValue() * weight * scale;

		anim->applyDeltaValue(val);

	}
	//--------------------------------------------------------------------------
	NumericKeyFrame* NumericAnimationTrack::createNumericKeyFrame(Real timePos)
	{
		return static_cast<NumericKeyFrame*>(createKeyFrame(timePos));
	}
	//--------------------------------------------------------------------------
	NumericKeyFrame* NumericAnimationTrack::getNumericKeyFrame(unsigned short index) const
	{
		return static_cast<NumericKeyFrame*>(getKeyFrame(index));
	}
    //---------------------------------------------------------------------
	//---------------------------------------------------------------------
	// Node specialisations
	//---------------------------------------------------------------------
	NodeAnimationTrack::NodeAnimationTrack(Animation* parent, unsigned short handle)
		: AnimationTrack(parent, handle), mTargetNode(0), mSplineBuildNeeded(false),
		mUseShortestRotationPath(true)
	{
	}
	//---------------------------------------------------------------------
	NodeAnimationTrack::NodeAnimationTrack(Animation* parent, unsigned short handle,
		Node* targetNode)
		: AnimationTrack(parent, handle), mTargetNode(targetNode),
		mSplineBuildNeeded(false), mUseShortestRotationPath(true)
	{
	}
	//---------------------------------------------------------------------
    void NodeAnimationTrack::getInterpolatedKeyFrame(Real timeIndex, KeyFrame* kf) const
    {
		TransformKeyFrame* kret = static_cast<TransformKeyFrame*>(kf);

        // Keyframe pointers
		KeyFrame *kBase1, *kBase2;
        TransformKeyFrame *k1, *k2;
        unsigned short firstKeyIndex;

        Real t = this->getKeyFramesAtTime(timeIndex, &kBase1, &kBase2, &firstKeyIndex);
		k1 = static_cast<TransformKeyFrame*>(kBase1);
		k2 = static_cast<TransformKeyFrame*>(kBase2);

        if (t == 0.0)
        {
            // Just use k1
            kret->setRotation(k1->getRotation());
            kret->setTranslate(k1->getTranslate());
            kret->setScale(k1->getScale());
        }
        else
        {
            // Interpolate by t
            Animation::InterpolationMode im = mParent->getInterpolationMode();
            Animation::RotationInterpolationMode rim =
                mParent->getRotationInterpolationMode();
            Vector3 base;
            switch(im)
            {
            case Animation::IM_LINEAR:
                // Interpolate linearly
                // Rotation
                // Interpolate to nearest rotation if mUseShortestRotationPath set
                if (rim == Animation::RIM_LINEAR)
                {
                    kret->setRotation( Quaternion::nlerp(t, k1->getRotation(),
                        k2->getRotation(), mUseShortestRotationPath) );
                }
                else //if (rim == Animation::RIM_SPHERICAL)
                {
                    kret->setRotation( Quaternion::Slerp(t, k1->getRotation(),
					    k2->getRotation(), mUseShortestRotationPath) );
                }

                // Translation
                base = k1->getTranslate();
                kret->setTranslate( base + ((k2->getTranslate() - base) * t) );

                // Scale
                base = k1->getScale();
                kret->setScale( base + ((k2->getScale() - base) * t) );
                break;

            case Animation::IM_SPLINE:
                // Spline interpolation

                // Build splines if required
                if (mSplineBuildNeeded)
                {
                    buildInterpolationSplines();
                }

                // Rotation, take mUseShortestRotationPath into account
                kret->setRotation( mRotationSpline.interpolate(firstKeyIndex, t,
					mUseShortestRotationPath) );

                // Translation
                kret->setTranslate( mPositionSpline.interpolate(firstKeyIndex, t) );

                // Scale
                kret->setScale( mScaleSpline.interpolate(firstKeyIndex, t) );

                break;
            }

        }

    }
    //---------------------------------------------------------------------
    void NodeAnimationTrack::apply(Real timePos, Real weight, bool accumulate,
		Real scale)
    {
        applyToNode(mTargetNode, timePos, weight, accumulate, scale);

    }
    //---------------------------------------------------------------------
    Node* NodeAnimationTrack::getAssociatedNode(void) const
    {
        return mTargetNode;
    }
    //---------------------------------------------------------------------
    void NodeAnimationTrack::setAssociatedNode(Node* node)
    {
        mTargetNode = node;
    }
    //---------------------------------------------------------------------
    void NodeAnimationTrack::applyToNode(Node* node, Real timePos, Real weight,
		bool accumulate, Real scl)
    {
		// Nothing to do if no keyframes
		if (mKeyFrames.empty())
			return;

        TransformKeyFrame kf(0, timePos);
		getInterpolatedKeyFrame(timePos, &kf);
		if (accumulate)
        {
            // add to existing. Weights are not relative, but treated as absolute multipliers for the animation
            Vector3 translate = kf.getTranslate() * weight * scl;
			node->translate(translate);

			// interpolate between no-rotation and full rotation, to point 'weight', so 0 = no rotate, 1 = full
            Quaternion rotate;
            Animation::RotationInterpolationMode rim =
                mParent->getRotationInterpolationMode();
            if (rim == Animation::RIM_LINEAR)
            {
                rotate = Quaternion::nlerp(weight, Quaternion::IDENTITY, kf.getRotation());
            }
            else //if (rim == Animation::RIM_SPHERICAL)
            {
                rotate = Quaternion::Slerp(weight, Quaternion::IDENTITY, kf.getRotation());
            }
			node->rotate(rotate);

			Vector3 scale = kf.getScale();
			// Not sure how to modify scale for cumulative anims... leave it alone
			//scale = ((Vector3::UNIT_SCALE - kf.getScale()) * weight) + Vector3::UNIT_SCALE;
			if (scl != 1.0f && scale != Vector3::UNIT_SCALE)
			{
				scale = Vector3::UNIT_SCALE + (scale - Vector3::UNIT_SCALE) * scl;
			}
			node->scale(scale);
		}
        else
        {
			// apply using weighted transform method
			Vector3 scale = kf.getScale();
			if (scl != 1.0f && scale != Vector3::UNIT_SCALE)
			{
				scale = Vector3::UNIT_SCALE + (scale - Vector3::UNIT_SCALE) * scl;
			}
			node->_weightedTransform(weight, kf.getTranslate() * scl, kf.getRotation(),
				scale);
		}

        /*
        // DEBUG
        if (!mMainWindow)
        {
            mMainWindow = Root::getSingleton().getRenderWindow("OGRE Render Window");
        }
        String msg = "Time: ";
        msg << timePos;
        mMainWindow->setDebugText(msg);
        */

        //node->rotate(kf.getRotation() * weight);
        //node->translate(kf.getTranslate() * weight);




    }
    //---------------------------------------------------------------------
    void NodeAnimationTrack::buildInterpolationSplines(void) const
    {
        // Don't calc automatically, do it on request at the end
        mPositionSpline.setAutoCalculate(false);
        mRotationSpline.setAutoCalculate(false);
        mScaleSpline.setAutoCalculate(false);

        mPositionSpline.clear();
        mRotationSpline.clear();
        mScaleSpline.clear();

        KeyFrameList::const_iterator i, iend;
        iend = mKeyFrames.end(); // precall to avoid overhead
        for (i = mKeyFrames.begin(); i != iend; ++i)
        {
			TransformKeyFrame* kf = static_cast<TransformKeyFrame*>(*i);
            mPositionSpline.addPoint(kf->getTranslate());
            mRotationSpline.addPoint(kf->getRotation());
            mScaleSpline.addPoint(kf->getScale());
        }

        mPositionSpline.recalcTangents();
        mRotationSpline.recalcTangents();
        mScaleSpline.recalcTangents();


        mSplineBuildNeeded = false;
    }

    //---------------------------------------------------------------------
	void NodeAnimationTrack::setUseShortestRotationPath(bool useShortestPath)
	{
		mUseShortestRotationPath = useShortestPath ;
	}

    //---------------------------------------------------------------------
	bool NodeAnimationTrack::getUseShortestRotationPath() const
	{
		return mUseShortestRotationPath ;
	}
    //---------------------------------------------------------------------
    void NodeAnimationTrack::_keyFrameDataChanged(void) const
    {
        mSplineBuildNeeded = true;
    }
    //---------------------------------------------------------------------
	bool NodeAnimationTrack::hasNonZeroKeyFrames(void) const
	{
        KeyFrameList::const_iterator i = mKeyFrames.begin();
        for (; i != mKeyFrames.end(); ++i)
        {
			// look for keyframes which have any component which is non-zero
			// Since exporters can be a little inaccurate sometimes we use a
			// tolerance value rather than looking for nothing
			TransformKeyFrame* kf = static_cast<TransformKeyFrame*>(*i);
			Vector3 trans = kf->getTranslate();
			Vector3 scale = kf->getScale();
			Vector3 axis;
			Radian angle;
			kf->getRotation().ToAngleAxis(angle, axis);
			Real tolerance = 1e-3f;
			if (!trans.positionEquals(Vector3::ZERO, tolerance) ||
				!scale.positionEquals(Vector3::UNIT_SCALE, tolerance) ||
				!Math::RealEqual(angle.valueRadians(), 0.0f, tolerance))
			{
				return true;
			}

		}

		return false;
	}
    //---------------------------------------------------------------------
	void NodeAnimationTrack::optimise(void)
	{
		// Eliminate duplicate keyframes from 2nd to penultimate keyframe
		// NB only eliminate middle keys from sequences of 5+ identical keyframes
		// since we need to preserve the boundary keys in place, and we need
		// 2 at each end to preserve tangents for spline interpolation
		Vector3 lasttrans;
		Vector3 lastscale;
		Quaternion lastorientation;
        KeyFrameList::iterator i = mKeyFrames.begin();
		Radian quatTolerance(1e-3f);
		std::list<unsigned short> removeList;
		unsigned short k = 0;
		ushort dupKfCount = 0;
        for (; i != mKeyFrames.end(); ++i, ++k)
        {
			TransformKeyFrame* kf = static_cast<TransformKeyFrame*>(*i);
			Vector3 newtrans = kf->getTranslate();
			Vector3 newscale = kf->getScale();
			Quaternion neworientation = kf->getRotation();
			// Ignore first keyframe; now include the last keyframe as we eliminate
			// only k-2 in a group of 5 to ensure we only eliminate middle keys
			if (i != mKeyFrames.begin() &&
				newtrans.positionEquals(lasttrans) &&
				newscale.positionEquals(lastscale) &&
				neworientation.equals(lastorientation, quatTolerance))
			{
				++dupKfCount;

				// 4 indicates this is the 5th duplicate keyframe
				if (dupKfCount == 4)
				{
					// remove the 'middle' keyframe
					removeList.push_back(k-2);
					--dupKfCount;
				}
			}
			else
			{
				// reset
				dupKfCount = 0;
				lasttrans = newtrans;
				lastscale = newscale;
				lastorientation = neworientation;
			}
		}

		// Now remove keyframes, in reverse order to avoid index revocation
		std::list<unsigned short>::reverse_iterator r = removeList.rbegin();
		for (; r!= removeList.rend(); ++r)
		{
			removeKeyFrame(*r);
		}


	}
	//--------------------------------------------------------------------------
	KeyFrame* NodeAnimationTrack::createKeyFrameImpl(Real time)
	{
		return new TransformKeyFrame(this, time);
	}
	//--------------------------------------------------------------------------
	TransformKeyFrame* NodeAnimationTrack::createNodeKeyFrame(Real timePos)
	{
		return static_cast<TransformKeyFrame*>(createKeyFrame(timePos));
	}
	//--------------------------------------------------------------------------
	TransformKeyFrame* NodeAnimationTrack::getNodeKeyFrame(unsigned short index) const
	{
		return static_cast<TransformKeyFrame*>(getKeyFrame(index));
	}
	//--------------------------------------------------------------------------
	VertexAnimationTrack::VertexAnimationTrack(Animation* parent,
		unsigned short handle, VertexAnimationType animType)
		: AnimationTrack(parent, handle), mAnimationType(animType)
	{
	}
	//--------------------------------------------------------------------------
	VertexAnimationTrack::VertexAnimationTrack(Animation* parent, unsigned short handle,
		VertexAnimationType animType, VertexData* targetData, TargetMode target)
		: AnimationTrack(parent, handle), mAnimationType(animType),
		mTargetVertexData(targetData), mTargetMode(target)
	{
	}
	//--------------------------------------------------------------------------
	VertexMorphKeyFrame* VertexAnimationTrack::createVertexMorphKeyFrame(Real timePos)
	{
		if (mAnimationType != VAT_MORPH)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Morph keyframes can only be created on vertex tracks of type morph.",
				"VertexAnimationTrack::createVertexMorphKeyFrame");
		}
		return static_cast<VertexMorphKeyFrame*>(createKeyFrame(timePos));
	}
	//--------------------------------------------------------------------------
	VertexPoseKeyFrame* VertexAnimationTrack::createVertexPoseKeyFrame(Real timePos)
	{
		if (mAnimationType != VAT_POSE)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Pose keyframes can only be created on vertex tracks of type pose.",
				"VertexAnimationTrack::createVertexPoseKeyFrame");
		}
		return static_cast<VertexPoseKeyFrame*>(createKeyFrame(timePos));
	}
	//--------------------------------------------------------------------------
	void VertexAnimationTrack::apply(Real timePos, Real weight, bool accumulate,
		Real scale)
	{
		applyToVertexData(mTargetVertexData, timePos, weight);
	}
	//--------------------------------------------------------------------------
	void VertexAnimationTrack::applyToVertexData(VertexData* data,
		Real timePos, Real weight,  const PoseList* poseList)
	{
		// Nothing to do if no keyframes
		if (mKeyFrames.empty())
			return;

		// Get keyframes
		KeyFrame *kf1, *kf2;
		Real t = getKeyFramesAtTime(timePos, &kf1, &kf2);

		if (mAnimationType == VAT_MORPH)
		{
			VertexMorphKeyFrame* vkf1 = static_cast<VertexMorphKeyFrame*>(kf1);
			VertexMorphKeyFrame* vkf2 = static_cast<VertexMorphKeyFrame*>(kf2);

			if (mTargetMode == TM_HARDWARE)
			{
				// If target mode is hardware, need to bind our 2 keyframe buffers,
				// one to main pos, one to morph target texcoord
				assert(!data->hwAnimationDataList.empty() &&
					"Haven't set up hardware vertex animation elements!");

				// no use for TempBlendedBufferInfo here btw
				// NB we assume that position buffer is unshared
				// VertexDeclaration::getAutoOrganisedDeclaration should see to that
				const VertexElement* posElem =
					data->vertexDeclaration->findElementBySemantic(VES_POSITION);
				// Set keyframe1 data as original position
				data->vertexBufferBinding->setBinding(
					posElem->getSource(), vkf1->getVertexBuffer());
				// Set keyframe2 data as derived
				data->vertexBufferBinding->setBinding(
					data->hwAnimationDataList[0].targetVertexElement->getSource(),
					vkf2->getVertexBuffer());
				// save T for use later
				data->hwAnimationDataList[0].parametric = t;

			}
			else
			{
				// If target mode is software, need to software interpolate each vertex

				Mesh::softwareVertexMorph(
					t, vkf1->getVertexBuffer(), vkf2->getVertexBuffer(), data);
			}
		}
		else
		{
			// Pose

			VertexPoseKeyFrame* vkf1 = static_cast<VertexPoseKeyFrame*>(kf1);
			VertexPoseKeyFrame* vkf2 = static_cast<VertexPoseKeyFrame*>(kf2);

			// For each pose reference in key 1, we need to locate the entry in
			// key 2 and interpolate the influence
			const VertexPoseKeyFrame::PoseRefList& poseList1 = vkf1->getPoseReferences();
			const VertexPoseKeyFrame::PoseRefList& poseList2 = vkf2->getPoseReferences();
			for (VertexPoseKeyFrame::PoseRefList::const_iterator p1 = poseList1.begin();
				p1 != poseList1.end(); ++p1)
			{
				Real startInfluence = p1->influence;
				Real endInfluence = 0;
				// Search for entry in keyframe 2 list (if not there, will be 0)
				for (VertexPoseKeyFrame::PoseRefList::const_iterator p2 = poseList2.begin();
					p2 != poseList2.end(); ++p2)
				{
					if (p1->poseIndex == p2->poseIndex)
					{
						endInfluence = p2->influence;
						break;
					}
				}
				// Interpolate influence
				Real influence = startInfluence + t*(endInfluence - startInfluence);
				// Scale by animation weight
				influence = weight * influence;
				// Get pose
				assert (p1->poseIndex <= poseList->size());
				Pose* pose = (*poseList)[p1->poseIndex];
				// apply
				applyPoseToVertexData(pose, data, influence);
			}
			// Now deal with any poses in key 2 which are not in key 1
			for (VertexPoseKeyFrame::PoseRefList::const_iterator p2 = poseList2.begin();
				p2 != poseList2.end(); ++p2)
			{
				bool found = false;
				for (VertexPoseKeyFrame::PoseRefList::const_iterator p1 = poseList1.begin();
					p1 != poseList1.end(); ++p1)
				{
					if (p1->poseIndex == p2->poseIndex)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					// Need to apply this pose too, scaled from 0 start
					Real influence = t * p2->influence;
					// Scale by animation weight
					influence = weight * influence;
					// Get pose
					assert (p2->poseIndex <= poseList->size());
					const Pose* pose = (*poseList)[p2->poseIndex];
					// apply
					applyPoseToVertexData(pose, data, influence);
				}
			} // key 2 iteration
		} // morph or pose animation
	}
	//-----------------------------------------------------------------------------
	void VertexAnimationTrack::applyPoseToVertexData(const Pose* pose,
		VertexData* data, Real influence)
	{
		if (mTargetMode == TM_HARDWARE)
		{
			// Hardware
			// If target mode is hardware, need to bind our pose buffer
			// to a target texcoord
			assert(!data->hwAnimationDataList.empty() &&
				"Haven't set up hardware vertex animation elements!");
			// no use for TempBlendedBufferInfo here btw
			// Set pose target as required
			size_t hwIndex = data->hwAnimDataItemsUsed++;
			// If we try to use too many poses, ignore extras
			if (hwIndex < data->hwAnimationDataList.size())
			{
				VertexData::HardwareAnimationData& animData = data->hwAnimationDataList[hwIndex];
				data->vertexBufferBinding->setBinding(
					animData.targetVertexElement->getSource(),
					pose->_getHardwareVertexBuffer(data->vertexCount));
				// save final influence in parametric
				animData.parametric = influence;

			}

		}
		else
		{
			// Software
			Mesh::softwareVertexPoseBlend(influence, pose->getVertexOffsets(), data);
		}

	}
	//--------------------------------------------------------------------------
	VertexMorphKeyFrame* VertexAnimationTrack::getVertexMorphKeyFrame(unsigned short index) const
	{
		if (mAnimationType != VAT_MORPH)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Morph keyframes can only be created on vertex tracks of type morph.",
				"VertexAnimationTrack::getVertexMorphKeyFrame");
		}

		return static_cast<VertexMorphKeyFrame*>(getKeyFrame(index));
	}
	//--------------------------------------------------------------------------
	VertexPoseKeyFrame* VertexAnimationTrack::getVertexPoseKeyFrame(unsigned short index) const
	{
		if (mAnimationType != VAT_POSE)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Pose keyframes can only be created on vertex tracks of type pose.",
				"VertexAnimationTrack::getVertexPoseKeyFrame");
		}

		return static_cast<VertexPoseKeyFrame*>(getKeyFrame(index));
	}
	//--------------------------------------------------------------------------
	KeyFrame* VertexAnimationTrack::createKeyFrameImpl(Real time)
	{
		switch(mAnimationType)
		{
		default:
		case VAT_MORPH:
            return new VertexMorphKeyFrame(this, time);
		case VAT_POSE:
			return new VertexPoseKeyFrame(this, time);
		};

	}
	//---------------------------------------------------------------------
	bool VertexAnimationTrack::hasNonZeroKeyFrames(void) const
	{
		if (mAnimationType == VAT_MORPH)
		{
			return !mKeyFrames.empty();
		}
		else
		{

			KeyFrameList::const_iterator i = mKeyFrames.begin();
			for (; i != mKeyFrames.end(); ++i)
			{
				// look for keyframes which have a pose influence which is non-zero
				const VertexPoseKeyFrame* kf = static_cast<const VertexPoseKeyFrame*>(*i);
				VertexPoseKeyFrame::ConstPoseRefIterator poseIt
					= kf->getPoseReferenceIterator();
				while (poseIt.hasMoreElements())
				{
					const VertexPoseKeyFrame::PoseRef& poseRef = poseIt.getNext();
					if (poseRef.influence > 0.0f)
						return true;
				}

			}

			return false;
		}
	}
	//---------------------------------------------------------------------
	void VertexAnimationTrack::optimise(void)
	{
		// TODO - remove sequences of duplicate pose references?


	}


}

