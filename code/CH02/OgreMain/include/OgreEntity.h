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
#ifndef __Entity_H__
#define __Entity_H__

#include "OgrePrerequisites.h"
#include "OgreCommon.h"

#include "OgreString.h"
#include "OgreMovableObject.h"
#include "OgreQuaternion.h"
#include "OgreVector3.h"
#include "OgreHardwareBufferManager.h"
#include "OgreMesh.h"

namespace Ogre {
	/** Defines an instance of a discrete, movable object based on a Mesh.
	@remarks
	Ogre generally divides renderable objects into 2 groups, discrete
	(separate) and relatively small objects which move around the world,
	and large, sprawling geometry which makes up generally immovable
	scenery, aka 'level geometry'.
	@par
	The Mesh and SubMesh classes deal with the definition of the geometry
	used by discrete movable objects. Entities are actual instances of
	objects based on this geometry in the world. Therefore there is
	usually a single set Mesh for a car, but there may be multiple
	entities based on it in the world. Entities are able to override
	aspects of the Mesh it is defined by, such as changing material
	properties per instance (so you can have many cars using the same
	geometry but different textures for example). Because a Mesh is split
	into SubMeshes for this purpose, the Entity class is a grouping class
	(much like the Mesh class) and much of the detail regarding
	individual changes is kept in the SubEntity class. There is a 1:1
	relationship between SubEntity instances and the SubMesh instances
	associated with the Mesh the Entity is based on.
	@par
	Entity and SubEntity classes are never created directly. Use the
	createEntity method of the SceneManager (passing a model name) to
	create one.
	@par
	Entities are included in the scene by associating them with a
	SceneNode, using the attachEntity method. See the SceneNode class
	for full information.
	@note
	No functions were declared virtual to improve performance.
	*/
	class _OgreExport Entity: public MovableObject
	{
		// Allow EntityFactory full access
		friend class EntityFactory;
		friend class SubEntity;
	public:
		typedef std::set<Entity*> EntitySet;

	protected:

		/** Private constructor (instances cannot be created directly).
		*/
		Entity();
		/** Private constructor - specify name (the usual constructor used).
		*/
		Entity( const String& name, MeshPtr& mesh);

		/** The Mesh that this Entity is based on.
		*/
		MeshPtr mMesh;

		/** List of SubEntities (point to SubMeshes).
		*/
		typedef std::vector<SubEntity*> SubEntityList;
		SubEntityList mSubEntityList;


		/// State of animation for animable meshes
		AnimationStateSet* mAnimationState;


		/// Temp buffer details for software skeletal anim of shared geometry
		TempBlendedBufferInfo mTempSkelAnimInfo;
		/// Vertex data details for software skeletal anim of shared geometry
		VertexData* mSkelAnimVertexData;
		/// Temp buffer details for software vertex anim of shared geometry
		TempBlendedBufferInfo mTempVertexAnimInfo;
		/// Vertex data details for software vertex anim of shared geometry
		VertexData* mSoftwareVertexAnimVertexData;
		/// Vertex data details for hardware vertex anim of shared geometry
		/// - separate since we need to s/w anim for shadows whilst still altering
		///   the vertex data for hardware morphing (pos2 binding)
		VertexData* mHardwareVertexAnimVertexData;
		/// Have we applied any vertex animation to shared geometry?
		bool mVertexAnimationAppliedThisFrame;
        /// Have the temp buffers already had their geometry prepared for use in rendering shadow volumes?
        bool mPreparedForShadowVolumes;

		/** Internal method - given vertex data which could be from the Mesh or 
		any submesh, finds the temporary blend copy. */
		const VertexData* findBlendedVertexData(const VertexData* orig);
		/** Internal method - given vertex data which could be from the Mesh or 
		any SubMesh, finds the corresponding SubEntity. */
		SubEntity* findSubEntityForVertexData(const VertexData* orig);

		/** Internal method for extracting metadata out of source vertex data
		for fast assignment of temporary buffers later. */
		void extractTempBufferInfo(VertexData* sourceData, TempBlendedBufferInfo* info);
		/** Internal method to clone vertex data definitions but to remove blend buffers. */
		VertexData* cloneVertexDataRemoveBlendInfo(const VertexData* source);
		/** Internal method for preparing this Entity for use in animation. */
		void prepareTempBlendBuffers(void);
		/** Mark all vertex data as so far unanimated. 
		*/
		void markBuffersUnusedForAnimation(void);
		/** Internal method to restore original vertex data where we didn't 
			perform any vertex animation this frame.
		*/
		void restoreBuffersForUnusedAnimation(bool hardwareAnimation);

		/// Cached bone matrices, including any world transform
        Matrix4 *mBoneWorldMatrices;
        /// Cached bone matrices in skeleton local space, might shares with other entity instances.
		Matrix4 *mBoneMatrices;
		unsigned short mNumBoneMatrices;
		/// Records the last frame in which animation was updated
		unsigned long mFrameAnimationLastUpdated;

		/// Perform all the updates required for an animated entity
		void updateAnimation(void);

		/// Records the last frame in which the bones was updated
		/// It's a pointer because it can be shared between different entities with
		/// a shared skeleton.
		unsigned long *mFrameBonesLastUpdated;

		/**
		* A set of all the entities which shares a single SkeletonInstance.
		* This is only created if the entity is in fact sharing it's SkeletonInstance with
		* other Entities.
		*/
		EntitySet* mSharedSkeletonEntities;

		/// Private method to cache bone matrices from skeleton
		void cacheBoneMatrices(void);

		/// Flag determines whether or not to display skeleton
		bool mDisplaySkeleton;
		/// Flag indicating whether hardware animation is supported by this entities materials
		bool mHardwareAnimation;
		/// Number of hardware poses supported by materials
		ushort mHardwarePoseCount;
		/// Flag indicating whether we have a vertex program in use on any of our subentities
		bool mVertexProgramInUse;
        /// Counter indicating number of requests for software animation.
        int mSoftwareAnimationRequests;
        /// Counter indicating number of requests for software blended normals.
        int mSoftwareAnimationNormalsRequests;


		/// The LOD number of the mesh to use, calculated by _notifyCurrentCamera
		ushort mMeshLodIndex;

		/// LOD bias factor, inverted for optimisation when calculating adjusted depth
		Real mMeshLodFactorInv;
		/// Index of minimum detail LOD (NB higher index is lower detail)
		ushort mMinMeshLodIndex;
		/// Index of maximum detail LOD (NB lower index is higher detail)
		ushort mMaxMeshLodIndex;

		/// LOD bias factor, inverted for optimisation when calculating adjusted depth
		Real mMaterialLodFactorInv;
		/// Index of minimum detail LOD (NB higher index is lower detail)
		ushort mMinMaterialLodIndex;
		/// Index of maximum detail LOD (NB lower index is higher detail)
		ushort mMaxMaterialLodIndex;

		/** List of LOD Entity instances (for manual LODs).
		We don't know when the mesh is using manual LODs whether one LOD to the next will have the
		same number of SubMeshes, therefore we have to allow a separate Entity list 
		with each alternate one.
		*/
		typedef std::vector<Entity*> LODEntityList;
		LODEntityList mLodEntityList;

		/** This Entity's personal copy of the skeleton, if skeletally animated
		*/
		SkeletonInstance* mSkeletonInstance;

		/// Last parent xform
		Matrix4 mLastParentXform;

		/** Builds a list of SubEntities based on the SubMeshes contained in the Mesh. */
		void buildSubEntityList(MeshPtr& mesh, SubEntityList* sublist);

		/// internal implementation of attaching a 'child' object to this entity and assign the parent node to the child entity
		void attachObjectImpl(MovableObject *pMovable, TagPoint *pAttachingPoint);

		/// internal implementation of detaching a 'child' object of this entity and clear the parent node of the child entity
		void detachObjectImpl(MovableObject* pObject);

		/// internal implementation of detaching all 'child' objects of this entity
		void detachAllObjectsImpl(void);

		/// Trigger reevaluation of the kind of vertex processing in use
		void reevaluateVertexProcessing(void);

		/// Apply vertex animation
		void applyVertexAnimation(bool hardwareAnimation, bool stencilShadows);
		/// Initialise the hardware animation elements for given vertex data
		void initHardwareAnimationElements(VertexData* vdata, 
			ushort numberOfElements);
		/// Are software vertex animation temp buffers bound?
		bool tempVertexAnimBuffersBound(void) const;
        /// Are software skeleton animation temp buffers bound?
        bool tempSkelAnimBuffersBound(bool requestNormals) const;

	public:
		/// Contains the child objects (attached to bones) indexed by name
		typedef std::map<String, MovableObject*> ChildObjectList;
	protected:
		ChildObjectList mChildObjectList;


		/// Bounding box that 'contains' all the mesh of each child entity
		mutable AxisAlignedBox mFullBoundingBox;

		bool mNormaliseNormals;

		ShadowRenderableList mShadowRenderables;

		/** Nested class to allow entity shadows. */
		class _OgreExport EntityShadowRenderable : public ShadowRenderable
		{
		protected:
			Entity* mParent;
			// Shared link to position buffer
			HardwareVertexBufferSharedPtr mPositionBuffer;
			// Shared link to w-coord buffer (optional)
			HardwareVertexBufferSharedPtr mWBuffer;
			// Link to current vertex data used to bind (maybe changes)
			const VertexData* mCurrentVertexData;
			// Original position buffer source binding
			unsigned short mOriginalPosBufferBinding;
			/// Link to SubEntity, only present if SubEntity has it's own geometry
			SubEntity* mSubEntity;


		public:
			EntityShadowRenderable(Entity* parent, 
				HardwareIndexBufferSharedPtr* indexBuffer, const VertexData* vertexData, 
				bool createSeparateLightCap, SubEntity* subent, bool isLightCap = false);
			~EntityShadowRenderable();
			/// Overridden from ShadowRenderable
			void getWorldTransforms(Matrix4* xform) const;
			/// Overridden from ShadowRenderable
			const Quaternion& getWorldOrientation(void) const;
			/// Overridden from ShadowRenderable
			const Vector3& getWorldPosition(void) const;
			HardwareVertexBufferSharedPtr getPositionBuffer(void) { return mPositionBuffer; }
			HardwareVertexBufferSharedPtr getWBuffer(void) { return mWBuffer; }
			/// Rebind the source positions (for temp buffer users)
			void rebindPositionBuffer(const VertexData* vertexData, bool force);
			/// Overridden from ShadowRenderable
			bool isVisible(void) const;

		};
	public:
		/** Default destructor.
		*/
		~Entity();

		/** Gets the Mesh that this Entity is based on.
		*/
		const MeshPtr& getMesh(void) const;

		/** Gets a pointer to a SubEntity, ie a part of an Entity.
		*/
		SubEntity* getSubEntity(unsigned int index) const;

		/** Gets a pointer to a SubEntity by name
		@remarks - names should be initialized during a Mesh creation.
		*/
		SubEntity* getSubEntity( const String& name ) const;

		/** Retrieves the number of SubEntity objects making up this entity.
		*/
		unsigned int getNumSubEntities(void) const;

		/** Clones this entity and returns a pointer to the clone.
		@remarks
		Useful method for duplicating an entity. The new entity must be
		given a unique name, and is not attached to the scene in any way
		so must be attached to a SceneNode to be visible (exactly as
		entities returned from SceneManager::createEntity).
		@param
		newName Name for the new entity.
		*/
		Entity* clone( const String& newName ) const;

		/** Sets the material to use for the whole of this entity.
		@remarks
		This is a shortcut method to set all the materials for all
		subentities of this entity. Only use this method is you want to
		set the same material for all subentities or if you know there
		is only one. Otherwise call getSubEntity() and call the same
		method on the individual SubEntity.
		*/
		void setMaterialName(const String& name);

		/** Overridden - see MovableObject.
		*/
		void _notifyCurrentCamera(Camera* cam);

		/// Overridden - see MovableObject.
		void setRenderQueueGroup(uint8 queueID);

		/** Overridden - see MovableObject.
		*/
		const AxisAlignedBox& getBoundingBox(void) const;

		/// merge all the child object Bounds a return it
		AxisAlignedBox getChildObjectsBoundingBox(void) const;

		/** Overridden - see MovableObject.
		*/
		void _updateRenderQueue(RenderQueue* queue);

		/** Overridden from MovableObject */
		const String& getMovableType(void) const;

		/** For entities based on animated meshes, gets the AnimationState object for a single animation.
		@remarks
		You animate an entity by updating the animation state objects. Each of these represents the
		current state of each animation available to the entity. The AnimationState objects are
		initialised from the Mesh object.
		*/
		AnimationState* getAnimationState(const String& name) const;
		/** For entities based on animated meshes, gets the AnimationState objects for all animations.
		@returns
		In case the entity is animated, this functions returns the pointer to a AnimationStateSet 
		containing all animations of the entries. If the entity is not animated, it returns 0.
		@remarks
		You animate an entity by updating the animation state objects. Each of these represents the
		current state of each animation available to the entity. The AnimationState objects are
		initialised from the Mesh object.
		*/
		AnimationStateSet* getAllAnimationStates(void) const;

		/** Tells the Entity whether or not it should display it's skeleton, if it has one.
		*/
		void setDisplaySkeleton(bool display);

		/** Returns whether or not the entity is currently displaying its skeleton.
		*/
		bool getDisplaySkeleton(void) const;


        /** Gets a pointer to the entity representing the numbered manual level of detail.
        @remarks
            The zero-based index never includes the original entity, unlike
			Mesh::getLodLevel.
        */
        Entity* getManualLodLevel(size_t index) const;

        /** Returns the number of manual levels of detail that this entity supports.
        @remarks
            This number never includes the original entity, it is difference
            with Mesh::getNumLodLevels.
        */
        size_t getNumManualLodLevels(void) const;

		/** Sets a level-of-detail bias for the mesh detail of this entity.
		@remarks
		Level of detail reduction is normally applied automatically based on the Mesh 
		settings. However, it is possible to influence this behaviour for this entity
		by adjusting the LOD bias. This 'nudges' the mesh level of detail used for this 
		entity up or down depending on your requirements. You might want to use this
		if there was a particularly important entity in your scene which you wanted to
		detail better than the others, such as a player model.
		@par
		There are three parameters to this method; the first is a factor to apply; it 
		defaults to 1.0 (no change), by increasing this to say 2.0, this model would 
		take twice as long to reduce in detail, whilst at 0.5 this entity would use lower
		detail versions twice as quickly. The other 2 parameters are hard limits which 
		let you set the maximum and minimum level-of-detail version to use, after all
		other calculations have been made. This lets you say that this entity should
		never be simplified, or that it can only use LODs below a certain level even
		when right next to the camera.
		@param factor Proportional factor to apply to the distance at which LOD is changed. 
		Higher values increase the distance at which higher LODs are displayed (2.0 is 
		twice the normal distance, 0.5 is half).
		@param maxDetailIndex The index of the maximum LOD this entity is allowed to use (lower
		indexes are higher detail: index 0 is the original full detail model).
		@param minDetailIndex The index of the minimum LOD this entity is allowed to use (higher
		indexes are lower detail). Use something like 99 if you want unlimited LODs (the actual
		LOD will be limited by the number in the Mesh)
		*/
		void setMeshLodBias(Real factor, ushort maxDetailIndex = 0, ushort minDetailIndex = 99);

		/** Sets a level-of-detail bias for the material detail of this entity.
		@remarks
		Level of detail reduction is normally applied automatically based on the Material 
		settings. However, it is possible to influence this behaviour for this entity
		by adjusting the LOD bias. This 'nudges' the material level of detail used for this 
		entity up or down depending on your requirements. You might want to use this
		if there was a particularly important entity in your scene which you wanted to
		detail better than the others, such as a player model.
		@par
		There are three parameters to this method; the first is a factor to apply; it 
		defaults to 1.0 (no change), by increasing this to say 2.0, this entity would 
		take twice as long to use a lower detail material, whilst at 0.5 this entity 
		would use lower detail versions twice as quickly. The other 2 parameters are 
		hard limits which let you set the maximum and minimum level-of-detail index 
		to use, after all other calculations have been made. This lets you say that 
		this entity should never be simplified, or that it can only use LODs below 
		a certain level even when right next to the camera.
		@param factor Proportional factor to apply to the distance at which LOD is changed. 
		Higher values increase the distance at which higher LODs are displayed (2.0 is 
		twice the normal distance, 0.5 is half).
		@param maxDetailIndex The index of the maximum LOD this entity is allowed to use (lower
		indexes are higher detail: index 0 is the original full detail model).
		@param minDetailIndex The index of the minimum LOD this entity is allowed to use (higher
		indexes are lower detail. Use something like 99 if you want unlimited LODs (the actual
		LOD will be limited by the number of lod indexes used in the Material)
		*/
		void setMaterialLodBias(Real factor, ushort maxDetailIndex = 0, ushort minDetailIndex = 99);

		/** Sets whether the polygon mode of this entire entity may be 
		overridden by the camera detail settings. 
		*/
		void setPolygonModeOverrideable(bool PolygonModeOverrideable);
		/** Attaches another object to a certain bone of the skeleton which this entity uses.
		@remarks
		This method can be used to attach another object to an animated part of this entity,
		by attaching it to a bone in the skeleton (with an offset if required). As this entity 
		is animated, the attached object will move relative to the bone to which it is attached.
		@param boneName The name of the bone (in the skeleton) to attach this object
		@param pMovable Pointer to the object to attach
		@param offsetOrientation An adjustment to the orientation of the attached object, relative to the bone.
		@param offsetPosition An adjustment to the position of the attached object, relative to the bone.
		@returns The TagPoint to which the object has been attached
		*/
		TagPoint* attachObjectToBone(const String &boneName, 
			MovableObject *pMovable, 
			const Quaternion &offsetOrientation = Quaternion::IDENTITY, 
			const Vector3 &offsetPosition = Vector3::ZERO);

		/// detach a MovableObject previously attached using attachObjectToBone
		MovableObject* detachObjectFromBone(const String &movableName);

		/** Detaches an object by pointer.
		@remarks
		This method is need when destroy a MovableObject which attached to a bone of this entity.
		But sometimes the object may be not in the child object list because it is a lod entity,
		this method can safely detect and ignore in this case.
		*/
		void detachObjectFromBone(MovableObject* obj);

		/// Detach all MovableObjects previously attached using attachObjectToBone
		void detachAllObjectsFromBone(void);

		typedef MapIterator<ChildObjectList> ChildObjectListIterator;
		/** Gets an iterator to the list of objects attached to bones on this entity. */
		ChildObjectListIterator getAttachedObjectIterator(void);
		/** @see MovableObject::getBoundingRadius */
		Real getBoundingRadius(void) const;

		/** @copy MovableObject::getWorldBoundingBox */
		const AxisAlignedBox& getWorldBoundingBox(bool derive = false) const;
		/** @copy MovableObject::getWorldBoundingSphere */
		const Sphere& getWorldBoundingSphere(bool derive = false) const;

        /** If set to true, this forces normals of this entity to be normalised
            dynamically by the hardware.
        @remarks
            This option can be used to prevent lighting variations when scaling an
            Entity using a SceneNode - normally because this scaling is hardware
            based, the normals get scaled too which causes lighting to become inconsistent.
            However, this has an overhead so only do this if you really need to.
        */
        void setNormaliseNormals(bool normalise) { mNormaliseNormals = normalise; }

        /** Returns true if this entity has auto-normalisation of normals set. */
        bool getNormaliseNormals(void) const {return mNormaliseNormals; }


        /** Overridden member from ShadowCaster. */
        EdgeData* getEdgeList(void);
        /** Overridden member from ShadowCaster. */
        ShadowRenderableListIterator getShadowVolumeRenderableIterator(
            ShadowTechnique shadowTechnique, const Light* light, 
            HardwareIndexBufferSharedPtr* indexBuffer, 
            bool extrudeVertices, Real extrusionDistance, unsigned long flags = 0 );

		/** Internal method for retrieving bone matrix information. */
		const Matrix4* _getBoneMatrices(void) const { return mBoneMatrices;}
		/** Internal method for retrieving bone matrix information. */
		unsigned short _getNumBoneMatrices(void) const { return mNumBoneMatrices; }
		/** Returns whether or not this entity is skeletally animated. */
		bool hasSkeleton(void) const { return mSkeletonInstance != 0; }
		/** Get this Entity's personal skeleton instance. */
		SkeletonInstance* getSkeleton(void) const { return mSkeletonInstance; }
		/** Returns whether or not hardware animation is enabled.
		@remarks
		Because fixed-function indexed vertex blending is rarely supported
		by existing graphics cards, hardware animation can only be done if
		the vertex programs in the materials used to render an entity support
		it. Therefore, this method will only return true if all the materials
		assigned to this entity have vertex programs assigned, and all those
		vertex programs must support 'includes_morph_animation true' if using
        morph animation, 'includes_pose_animation true' if using pose animation
        and 'includes_skeletal_animation true' if using skeletal animation.
		*/
		bool isHardwareAnimationEnabled(void) const { return mHardwareAnimation; }

		/** Overridden from MovableObject */
		void _notifyAttached(Node* parent, bool isTagPoint = false);
        /** Returns the number of requests that have been made for software animation
        @remarks
            If non-zero then software animation will be performed in updateAnimation
            regardless of the current setting of isHardwareAnimationEnabled or any
            internal optimise for eliminate software animation. Requests for software
            animation are made by calling the addSoftwareAnimationRequest() method.
        */
        int getSoftwareAnimationRequests(void) const { return mSoftwareAnimationRequests; }
        /** Returns the number of requests that have been made for software animation of normals
        @remarks
            If non-zero, and getSoftwareAnimationRequests() also returns non-zero,
            then software animation of normals will be performed in updateAnimation
            regardless of the current setting of isHardwareAnimationEnabled or any
            internal optimise for eliminate software animation. Currently it is not
            possible to force software animation of only normals. Consequently this
            value is always less than or equal to that returned by getSoftwareAnimationRequests().
            Requests for software animation of normals are made by calling the 
            addSoftwareAnimationRequest() method with 'true' as the parameter.
        */
        int getSoftwareAnimationNormalsRequests(void) const { return mSoftwareAnimationNormalsRequests; }
        /** Add a request for software animation
        @remarks
            Tells the entity to perform animation calculations for skeletal/vertex
            animations in software, regardless of the current setting of 
            isHardwareAnimationEnabled().  Software animation will be performed
            any time one or more requests have been made.  If 'normalsAlso' is
            'true', then the entity will also do software blending on normal
            vectors, in addition to positions. This advanced method useful for 
            situations in which access to actual mesh vertices is required,
            such as accurate collision detection or certain advanced shading 
            techniques. When software animation is no longer needed, 
            the caller of this method should always remove the request by calling
            removeSoftwareAnimationRequest(), passing the same value for 
            'normalsAlso'.
        */
        void addSoftwareAnimationRequest(bool normalsAlso);
        /** Removes a request for software animation
        @remarks
            Calling this decrements the entity's internal counter of the number
            of requests for software animation.  If the counter is already zero
            then calling this method throws an exception.  The 'normalsAlso' 
            flag if set to 'true' will also decrement the internal counter of
            number of requests for software animation of normals.
        */
        void removeSoftwareAnimationRequest(bool normalsAlso);

		/** Shares the SkeletonInstance with the supplied entity.
		*   Note that in order for this to work, both entities must have the same
		*   Skeleton.
		*/
		void shareSkeletonInstanceWith(Entity* entity);

		/** Returns whether or not this entity is either morph or pose animated. 
		*/
		bool hasVertexAnimation(void) const;


		/** Stops sharing the SkeletonInstance with other entities.
		*/
		void stopSharingSkeletonInstance();


		/**
		* Returns whether this entity shares it's SkeltonInstance with other entity instances.
		*/
		inline bool sharesSkeletonInstance() const { return mSharedSkeletonEntities != NULL; }

		/**
		* Returns a pointer to the set of entities which share a SkeletonInstance.
		* If this instance does not share it's SkeletonInstance with other instances NULL will be returned
		*/
		inline const EntitySet* getSkeletonInstanceSharingSet() const { return mSharedSkeletonEntities; }

		/** Updates the internal animation state set to include the latest 
		available animations from the attached skeleton.
		@remarks
		Use this method if you manually add animations to a skeleton, or have
		linked the skeleton to another for animation purposes since creating 
		this entity.
		@note
		If you have called getAnimationState prior to calling this method, 
		the pointers will still remain valid.
		*/
		void refreshAvailableAnimationState(void);

		/** Advanced method to perform all the updates required for an animated entity.
		@remarks
		You don't normally need to call this, but it's here incase you wish
		to manually update the animation of an Entity at a specific point in
		time. Animation will not be updated more than once a frame no matter
		how many times you call this method.
		*/
		void _updateAnimation(void);

        /** Tests if any animation applied to this entity.
        @remarks
            An entity is animated if any animation state is enabled, or any manual bone
            applied to the skeleton.
        */
        bool _isAnimated(void) const;

        /** Tests if skeleton was animated.
        */
        bool _isSkeletonAnimated(void) const;

		/** Advanced method to get the temporarily blended skeletal vertex information
		for entities which are software skinned. 
        @remarks
            Internal engine will eliminate software animation if possible, this
            information is unreliable unless added request for software animation
            via addSoftwareAnimationRequest.
        @note
            The positions/normals of the returned vertex data is in object space.
		*/
		VertexData* _getSkelAnimVertexData(void) const;
		/** Advanced method to get the temporarily blended software vertex animation information
        @remarks
            Internal engine will eliminate software animation if possible, this
            information is unreliable unless added request for software animation
            via addSoftwareAnimationRequest.
        @note
            The positions/normals of the returned vertex data is in object space.
		*/
		VertexData* _getSoftwareVertexAnimVertexData(void) const;
		/** Advanced method to get the hardware morph vertex information
        @note
            The positions/normals of the returned vertex data is in object space.
		*/
		VertexData* _getHardwareVertexAnimVertexData(void) const;
		/** Advanced method to get the temp buffer information for software 
		skeletal animation.
		*/
		TempBlendedBufferInfo* _getSkelAnimTempBufferInfo(void);
		/** Advanced method to get the temp buffer information for software 
		morph animation.
		*/
		TempBlendedBufferInfo* _getVertexAnimTempBufferInfo(void);
		/// Override to return specific type flag
		uint32 getTypeFlags(void) const;
		/// Retrieve the VertexData which should be used for GPU binding
		VertexData* getVertexDataForBinding(void);

		/// Identify which vertex data we should be sending to the renderer
		enum VertexDataBindChoice 
		{
			BIND_ORIGINAL,
			BIND_SOFTWARE_SKELETAL,
			BIND_SOFTWARE_MORPH,
			BIND_HARDWARE_MORPH
		};
		/// Choose which vertex data to bind to the renderer
		VertexDataBindChoice chooseVertexDataForBinding(bool hasVertexAnim) const;

		/** Are buffers already marked as vertex animated? */
		bool _getBuffersMarkedForAnimation(void) const { return mVertexAnimationAppliedThisFrame; }
		/** Mark just this vertex data as animated. 
		*/
		void _markBuffersUsedForAnimation(void);


	};

	/** Factory object for creating Entity instances */
	class _OgreExport EntityFactory : public MovableObjectFactory
	{
	protected:
		MovableObject* createInstanceImpl( const String& name, const NameValuePairList* params);
	public:
		EntityFactory() {}
		~EntityFactory() {}

		static String FACTORY_TYPE_NAME;

		const String& getType(void) const;
		void destroyInstance( MovableObject* obj);  

	};

} // namespace

#endif
