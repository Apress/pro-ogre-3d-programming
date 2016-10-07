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

#ifndef __Animation_H__
#define __Animation_H__

#include "OgrePrerequisites.h"
#include "OgreString.h"
#include "OgreIteratorWrappers.h"
#include "OgreAnimable.h"
#include "OgreAnimationTrack.h"


namespace Ogre {

    /** An animation sequence. 
    @remarks
        This class defines the interface for a sequence of animation, whether that
        be animation of a mesh, a path along a spline, or possibly more than one
        type of animation in one. An animation is made up of many 'tracks', which are
        the more specific types of animation.
    @par
        You should not create these animations directly. They will be created via a parent
        object which owns the animation, e.g. Skeleton.
    */
    class _OgreExport Animation
    {

    public:
        /** The types of animation interpolation available. */
        enum InterpolationMode
        {
            /** Values are interpolated along straight lines. */
            IM_LINEAR,
            /** Values are interpolated along a spline, resulting in smoother changes in direction. */
            IM_SPLINE
        };

        /** The types of rotational interpolation available. */
        enum RotationInterpolationMode
        {
            /** Values are interpolated linearly. This is faster but does not 
                necessarily give a completely accurate result.
            */
            RIM_LINEAR,
            /** Values are interpolated spherically. This is more accurate but
                has a higher cost.
            */
            RIM_SPHERICAL
        };
        /** You should not use this constructor directly, use the parent object such as Skeleton instead.
        @param name The name of the animation, should be unique within it's parent (e.g. Skeleton)
        @param length The length of the animation in seconds.
        */
        Animation(const String& name, Real length);
        virtual ~Animation();

        /** Gets the name of this animation. */
        const String& getName(void) const;

        /** Gets the total length of the animation. */
        Real getLength(void) const;

        /** Creates a NodeAnimationTrack for animating a Node.
        @param handle Handle to give the track, used for accessing the track later. 
            Must be unique within this Animation.
        */
        NodeAnimationTrack* createNodeTrack(unsigned short handle);

		/** Creates a NumericAnimationTrack for animating any numeric value.
		@param handle Handle to give the track, used for accessing the track later. 
		Must be unique within this Animation.
		*/
		NumericAnimationTrack* createNumericTrack(unsigned short handle);

		/** Creates a VertexAnimationTrack for animating vertex position data.
		@param handle Handle to give the track, used for accessing the track later. 
		Must be unique within this Animation, and is used to identify the target. For example
		when applied to a Mesh, the handle must reference the index of the geometry being 
		modified; 0 for the shared geometry, and 1+ for SubMesh geometry with the same index-1.
		@param animType Either morph or pose animation, 
		*/
		VertexAnimationTrack* createVertexTrack(unsigned short handle, VertexAnimationType animType);

		/** Creates a new AnimationTrack automatically associated with a Node. 
        @remarks
            This method creates a standard AnimationTrack, but also associates it with a
            target Node which will receive all keyframe effects.
        @param handle Numeric handle to give the track, used for accessing the track later. 
            Must be unique within this Animation.
        @param node A pointer to the Node object which will be affected by this track
        */
        NodeAnimationTrack* createNodeTrack(unsigned short handle, Node* node);

		/** Creates a NumericAnimationTrack and associates it with an animable. 
		@param handle Handle to give the track, used for accessing the track later. 
		@param anim Animable object link
		Must be unique within this Animation.
		*/
		NumericAnimationTrack* createNumericTrack(unsigned short handle, 
			const AnimableValuePtr& anim);

		/** Creates a VertexAnimationTrack and associates it with VertexData. 
		@param handle Handle to give the track, used for accessing the track later. 
		@param data VertexData object link
		@param animType The animation type 
		Must be unique within this Animation.
		*/
		VertexAnimationTrack* createVertexTrack(unsigned short handle, 
			VertexData* data, VertexAnimationType animType);

		/** Gets the number of NodeAnimationTrack objects contained in this animation. */
        unsigned short getNumNodeTracks(void) const;

        /** Gets a node track by it's handle. */
        NodeAnimationTrack* getNodeTrack(unsigned short handle) const;

		/** Does a track exist with the given handle? */
		bool hasNodeTrack(unsigned short handle) const;

		/** Gets the number of NumericAnimationTrack objects contained in this animation. */
		unsigned short getNumNumericTracks(void) const;

		/** Gets a numeric track by it's handle. */
		NumericAnimationTrack* getNumericTrack(unsigned short handle) const;

		/** Does a track exist with the given handle? */
		bool hasNumericTrack(unsigned short handle) const;

		/** Gets the number of VertexAnimationTrack objects contained in this animation. */
		unsigned short getNumVertexTracks(void) const;

		/** Gets a Vertex track by it's handle. */
		VertexAnimationTrack* getVertexTrack(unsigned short handle) const;

		/** Does a track exist with the given handle? */
		bool hasVertexTrack(unsigned short handle) const;
		
		/** Destroys the node track with the given handle. */
        void destroyNodeTrack(unsigned short handle);

		/** Destroys the numeric track with the given handle. */
		void destroyNumericTrack(unsigned short handle);

		/** Destroys the Vertex track with the given handle. */
		void destroyVertexTrack(unsigned short handle);

		/** Removes and destroys all tracks making up this animation. */
        void destroyAllTracks(void);

		/** Removes and destroys all tracks making up this animation. */
		void destroyAllNodeTracks(void);
		/** Removes and destroys all tracks making up this animation. */
		void destroyAllNumericTracks(void);
		/** Removes and destroys all tracks making up this animation. */
		void destroyAllVertexTracks(void);

        /** Applies an animation given a specific time point and weight.
        @remarks
            Where you have associated animation tracks with objects, you can eaily apply
            an animation to those objects by calling this method.
        @param timePos The time position in the animation to apply.
        @param weight The influence to give to this track, 1.0 for full influence, less to blend with
          other animations.
	    @param scale The scale to apply to translations and scalings, useful for 
			adapting an animation to a different size target.
        */
        void apply(Real timePos, Real weight = 1.0, bool accumulate = false, 
			Real scale = 1.0f);

        /** Applies all node tracks given a specific time point and weight to a given skeleton.
        @remarks
        Where you have associated animation tracks with Node objects, you can eaily apply
        an animation to those nodes by calling this method.
        @param timePos The time position in the animation to apply.
        @param weight The influence to give to this track, 1.0 for full influence, less to blend with
        other animations.
	    @param scale The scale to apply to translations and scalings, useful for 
			adapting an animation to a different size target.
        */
        void apply(Skeleton* skeleton, Real timePos, Real weight = 1.0, 
			bool accumulate = false, Real scale = 1.0f);

		/** Applies all vertex tracks given a specific time point and weight to a given entity.
		@remarks
		@param entity The Entity to which this animation should be applied
		@param timePos The time position in the animation to apply.
		@param weight The weight at which the animation should be applied 
			(only affects pose animation)
		@param software Whether to populate the software morph vertex data
		@param hardware Whether to populate the hardware morph vertex data
		*/
		void apply(Entity* entity, Real timePos, Real weight, bool software, 
			bool hardware);

        /** Tells the animation how to interpolate between keyframes.
        @remarks
            By default, animations normally interpolate linearly between keyframes. This is
            fast, but when animations include quick changes in direction it can look a little
            unnatural because directions change instantly at keyframes. An alternative is to
            tell the animation to interpolate along a spline, which is more expensive in terms
            of calculation time, but looks smoother because major changes in direction are 
            distributed around the keyframes rather than just at the keyframe.
        @par
            You can also change the default animation behaviour by calling 
            Animation::setDefaultInterpolationMode.
        */
        void setInterpolationMode(InterpolationMode im);

        /** Gets the current interpolation mode of this animation. 
        @remarks
            See setInterpolationMode for more info.
        */
        InterpolationMode getInterpolationMode(void) const;
        /** Tells the animation how to interpolate rotations.
        @remarks
            By default, animations interpolate lieanrly between rotations. This
            is fast but not necessarily completely accurate. If you want more 
            accurate interpolation, use spherical interpolation, but be aware 
            that it will incur a higher cost.
        @par
            You can also change the default rotation behaviour by calling 
            Animation::setDefaultRotationInterpolationMode.
        */
        void setRotationInterpolationMode(RotationInterpolationMode im);

        /** Gets the current rotation interpolation mode of this animation. 
        @remarks
            See setRotationInterpolationMode for more info.
        */
        RotationInterpolationMode getRotationInterpolationMode(void) const;

        // Methods for setting the defaults
        /** Sets the default animation interpolation mode. 
        @remarks
            Every animation created after this option is set will have the new interpolation
            mode specified. You can also change the mode per animation by calling the 
            setInterpolationMode method on the instance in question.
        */
        static void setDefaultInterpolationMode(InterpolationMode im);

        /** Gets the default interpolation mode for all animations. */
        static InterpolationMode getDefaultInterpolationMode(void);

        /** Sets the default rotation interpolation mode. 
        @remarks
            Every animation created after this option is set will have the new interpolation
            mode specified. You can also change the mode per animation by calling the 
            setInterpolationMode method on the instance in question.
        */
        static void setDefaultRotationInterpolationMode(RotationInterpolationMode im);

        /** Gets the default rotation interpolation mode for all animations. */
        static RotationInterpolationMode getDefaultRotationInterpolationMode(void);

        typedef std::map<unsigned short, NodeAnimationTrack*> NodeTrackList;
        typedef ConstMapIterator<NodeTrackList> NodeTrackIterator;

		typedef std::map<unsigned short, NumericAnimationTrack*> NumericTrackList;
		typedef ConstMapIterator<NumericTrackList> NumericTrackIterator;

		typedef std::map<unsigned short, VertexAnimationTrack*> VertexTrackList;
		typedef ConstMapIterator<VertexTrackList> VertexTrackIterator;

		/// Fast access to NON-UPDATEABLE node track list
        const NodeTrackList& _getNodeTrackList(void) const;

        /// Get non-updateable iterator over node tracks
        NodeTrackIterator getNodeTrackIterator(void) const
        { return NodeTrackIterator(mNodeTrackList.begin(), mNodeTrackList.end()); }
        
		/// Fast access to NON-UPDATEABLE numeric track list
		const NumericTrackList& _getNumericTrackList(void) const;

		/// Get non-updateable iterator over node tracks
		NumericTrackIterator getNumericTrackIterator(void) const
		{ return NumericTrackIterator(mNumericTrackList.begin(), mNumericTrackList.end()); }

		/// Fast access to NON-UPDATEABLE Vertex track list
		const VertexTrackList& _getVertexTrackList(void) const;

		/// Get non-updateable iterator over node tracks
		VertexTrackIterator getVertexTrackIterator(void) const
		{ return VertexTrackIterator(mVertexTrackList.begin(), mVertexTrackList.end()); }

		/** Optimise an animation by removing unnecessary tracks and keyframes.
		@remarks
			When you export an animation, it is possible that certain tracks
			have been keyfamed but actually don't include anything useful - the
			keyframes include no transformation. These tracks can be completely
			eliminated from the animation and thus speed up the animation. 
			In addition, if several keyframes in a row have the same value, 
			then they are just adding overhead and can be removed.
		*/
		void optimise(void);
		


    protected:
        /// Node tracks, indexed by handle
        NodeTrackList mNodeTrackList;
		/// Numeric tracks, indexed by handle
		NumericTrackList mNumericTrackList;
		/// Vertex tracks, indexed by handle
		VertexTrackList mVertexTrackList;
        String mName;

        Real mLength;

        InterpolationMode mInterpolationMode;
        RotationInterpolationMode mRotationInterpolationMode;

        static InterpolationMode msDefaultInterpolationMode;
        static RotationInterpolationMode msDefaultRotationInterpolationMode;

		void optimiseNodeTracks(void);
		void optimiseVertexTracks(void);

        
    };


}


#endif

