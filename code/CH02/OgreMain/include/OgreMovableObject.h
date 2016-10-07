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

#ifndef __MovableObject_H__
#define __MovableObject_H__

// Precompiler options
#include "OgrePrerequisites.h"
#include "OgreRenderQueue.h"
#include "OgreAxisAlignedBox.h"
#include "OgreSphere.h"
#include "OgreShadowCaster.h"
#include "OgreFactoryObj.h"
#include "OgreAnimable.h"
#include "OgreAny.h"
#include "OgreUserDefinedObject.h"

namespace Ogre {

	// Forward declaration
	class MovableObjectFactory;

    /** Abstract class definining a movable object in a scene.
        @remarks
            Instances of this class are discrete, relatively small, movable objects
            which are attached to SceneNode objects to define their position.
    */
    class _OgreExport MovableObject : public ShadowCaster, public AnimableObject
    {
    protected:
		/// Name of this object
		String mName;
		/// Creator of this object (if created by a factory)
		MovableObjectFactory* mCreator;
		/// SceneManager holding this object (if applicable)
		SceneManager* mManager;
        /// node to which this object is attached
        Node* mParentNode;
        bool mParentIsTagPoint;
        /// Is this object visible?
        bool mVisible;
		/// Upper distance to still render
		Real mUpperDistance;
		Real mSquaredUpperDistance;
		/// Hidden because of distance?
		bool mBeyondFarDistance;
		/// User defined link to another object / value / whatever
		Any mUserAny;
        /// The render queue to use when rendering this object
        uint8 mRenderQueueID;
		/// Flags whether the RenderQueue's default should be used.
		bool mRenderQueueIDSet;
        /// Flags determining whether this object is included / excluded from scene queries
        uint32 mQueryFlags;
        /// Flags determining whether this object is visible (compared to SceneManager mask)
        uint32 mVisibilityFlags;
        /// Cached world AABB of this object
        mutable AxisAlignedBox mWorldAABB;
		// Cached world bounding sphere
		mutable Sphere mWorldBoundingSphere;
        /// World space AABB of this object's dark cap
        mutable AxisAlignedBox mWorldDarkCapBounds;
        /// Does this object cast shadows?
        bool mCastShadows;

		// Static members
		/// Default query flags
		static uint32 msDefaultQueryFlags;
		/// Default visibility flags
		static uint32 msDefaultVisibilityFlags;



    public:
        /// Constructor
        MovableObject();

		/// Named constructor
		MovableObject(const String& name);
        /** Virtual destructor - read Scott Meyers if you don't know why this is needed.
        */
        virtual ~MovableObject();

		/** Notify the object of it's creator (internal use only) */
		virtual void _notifyCreator(MovableObjectFactory* fact) { mCreator = fact; }
		/** Get the creator of this object, if any (internal use only) */
		virtual MovableObjectFactory*  _getCreator(void) const { return mCreator; }
		/** Notify the object of it's manager (internal use only) */
		virtual void _notifyManager(SceneManager* man) { mManager = man; }
		/** Get the manager of this object, if any (internal use only) */
		virtual SceneManager* _getManager(void) const { return mManager; }

        /** Returns the name of this object. */
		virtual const String& getName(void) const { return mName; }

        /** Returns the type name of this object. */
        virtual const String& getMovableType(void) const = 0;

        /** Returns the node to which this object is attached.
        @remarks
            A MovableObject may be attached to either a SceneNode or to a TagPoint, 
            the latter case if it's attached to a bone on an animated entity. 
            Both are Node subclasses so this method will return either.
        */
        virtual Node* getParentNode(void) const;

        /** Returns the scene node to which this object is attached.
        @remarks
            A MovableObject may be attached to either a SceneNode or to a TagPoint, 
            the latter case if it's attached to a bone on an animated entity. 
            This method will return the scene node of the parent entity 
            if the latter is true.
        */
        virtual SceneNode* getParentSceneNode(void) const;

        /** Internal method called to notify the object that it has been attached to a node.
        */
        virtual void _notifyAttached(Node* parent, bool isTagPoint = false);

        /** Returns true if this object is attached to a SceneNode or TagPoint. */
        virtual bool isAttached(void) const;

        /** Returns true if this object is attached to a SceneNode or TagPoint, 
			and this SceneNode / TagPoint is currently in an active part of the
			scene graph. */
        virtual bool isInScene(void) const;

		/** Internal method to notify the object of the camera to be used for the next rendering operation.
            @remarks
                Certain objects may want to do specific processing based on the camera position. This method notifies
                them incase they wish to do this.
        */
        virtual void _notifyCurrentCamera(Camera* cam);

        /** Retrieves the local axis-aligned bounding box for this object.
            @remarks
                This bounding box is in local coordinates.
        */
        virtual const AxisAlignedBox& getBoundingBox(void) const = 0;

		/** Retrieves the radius of the origin-centered bounding sphere 
		 	 for this object.
		*/
		virtual Real getBoundingRadius(void) const = 0;

        /** Retrieves the axis-aligned bounding box for this object in world coordinates. */
        virtual const AxisAlignedBox& getWorldBoundingBox(bool derive = false) const;
		/** Retrieves the worldspace bounding sphere for this object. */
        virtual const Sphere& getWorldBoundingSphere(bool derive = false) const;
        /** Internal method by which the movable object must add Renderable subclass instances to the rendering queue.
            @remarks
                The engine will call this method when this object is to be rendered. The object must then create one or more
                Renderable subclass instances which it places on the passed in Queue for rendering.
        */
        virtual void _updateRenderQueue(RenderQueue* queue) = 0;

        /** Tells this object whether to be visible or not, if it has a renderable component. 
		@note An alternative approach of making an object invisible is to detach it
			from it's SceneNode, or to remove the SceneNode entirely. 
			Detaching a node means that structurally the scene graph changes. 
			Once this change has taken place, the objects / nodes that have been 
			removed have less overhead to the visbility detection pass than simply
			making the object invisible, so if you do this and leave the objects 
			out of the tree for a long time, it's faster. However, the act of 
			detaching / reattaching nodes is in itself more expensive than 
			setting an object visibility flag, since in the latter case 
			structural changes are not made. Therefore, small or frequent visbility
			changes are best done using this method; large or more longer term
			changes are best done by detaching.
		*/
        virtual void setVisible(bool visible);

        /** Gets this object whether to be visible or not, if it has a renderable component. 
        @remarks
            Returns the value set by MovableObject::setVisible only.
        */
        virtual bool getVisible(void) const;

        /** Returns whether or not this object is supposed to be visible or not. 
		@remarks
			Takes into account both upper rendering distance and visible flag.
		*/
        virtual bool isVisible(void) const;

		/** Sets the distance at which the object is no longer rendered.
		@param dist Distance beyond which the object will not be rendered 
			(the default is 0, which means objects are always rendered).
		*/
		virtual void setRenderingDistance(Real dist) { 
			mUpperDistance = dist; 
			mSquaredUpperDistance = mUpperDistance * mUpperDistance;
		}

		/** Gets the distance at which batches are no longer rendered. */
		virtual Real getRenderingDistance(void) const { return mUpperDistance; }

        /** Call this to associate your own custom user object instance with this MovableObject.
        @remarks
            By simply making your game / application object a subclass of UserDefinedObject, you
            can establish a link between an OGRE instance of MovableObject and your own application
            classes. Call this method to establish the link.
        */
        virtual void setUserObject(UserDefinedObject* obj) { mUserAny = Any(obj); }
        /** Retrieves a pointer to a custom application object associated with this movable by an earlier
            call to setUserObject.
        */
        virtual UserDefinedObject* getUserObject(void) 
		{ 
			return mUserAny.isEmpty() ? 0 : any_cast<UserDefinedObject*>(mUserAny); 
		}

		/** Sets any kind of user value on this object.
		@remarks
			This method allows you to associate any user value you like with 
			this MovableObject. This can be a pointer back to one of your own
			classes for instance.
		@note This value is shared with setUserObject so don't use both!
		*/
		virtual void setUserAny(const Any& anything) { mUserAny = anything; }

		/** Retrieves the custom user value associated with this object.
		*/
		virtual const Any& getUserAny(void) const { return mUserAny; }

        /** Sets the render queue group this entity will be rendered through.
        @remarks
            Render queues are grouped to allow you to more tightly control the ordering
            of rendered objects. If you do not call this method, all Entity objects default
            to the default queue (RenderQueue::getDefaultQueueGroup), which is fine for most objects. You may want to alter this
            if you want this entity to always appear in front of other objects, e.g. for
            a 3D menu system or such.
        @par
            See RenderQueue for more details.
        @param queueID Enumerated value of the queue group to use.
        */
        virtual void setRenderQueueGroup(uint8 queueID);

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        virtual uint8 getRenderQueueGroup(void) const;

		/// return the full transformation of the parent sceneNode or the attachingPoint node
		virtual const Matrix4& _getParentNodeFullTransform(void) const;

        /** Sets the query flags for this object.
        @remarks
            When performing a scene query, this object will be included or excluded according
            to flags on the object and flags on the query. This is a bitwise value, so only when
            a bit on these flags is set, will it be included in a query asking for that flag. The
            meaning of the bits is application-specific.
        */
        virtual void setQueryFlags(uint32 flags) { mQueryFlags = flags; }

        /** As setQueryFlags, except the flags passed as parameters are appended to the
        existing flags on this object. */
        virtual void addQueryFlags(uint32 flags) { mQueryFlags |= flags; }
            
        /** As setQueryFlags, except the flags passed as parameters are removed from the
        existing flags on this object. */
        virtual void removeQueryFlags(unsigned long flags) { mQueryFlags &= ~flags; }
        
        /// Returns the query flags relevant for this object
        virtual uint32 getQueryFlags(void) const { return mQueryFlags; }

		/** Set the default query flags for all future MovableObject instances.
		*/
		static void setDefaultQueryFlags(uint32 flags) { msDefaultQueryFlags = flags; }

		/** Get the default query flags for all future MovableObject instances.
		*/
		static uint32 getDefaultQueryFlags(uint32 flags) { return msDefaultQueryFlags; }

		
        /** Sets the visiblity flags for this object.
        @remarks
			As well as a simple true/false value for visibility (as seen in setVisible), 
			you can also set visiblity flags which when 'and'ed with the SceneManager's
			visibility mask can also make an object invisible.
        */
        virtual void setVisibilityFlags(uint32 flags) { mVisibilityFlags = flags; }

        /** As setVisibilityFlags, except the flags passed as parameters are appended to the
        existing flags on this object. */
        virtual void addVisibilityFlags(uint32 flags) { mVisibilityFlags |= flags; }
            
        /** As setVisibilityFlags, except the flags passed as parameters are removed from the
        existing flags on this object. */
        virtual void removeVisibilityFlags(uint32 flags) { mVisibilityFlags &= ~flags; }
        
        /// Returns the visibility flags relevant for this object
        virtual uint32 getVisibilityFlags(void) const { return mVisibilityFlags; }

		/** Set the default visibility flags for all future MovableObject instances.
		*/
		static void setDefaultVisibilityFlags(uint32 flags) { msDefaultVisibilityFlags = flags; }
		
		/** Get the default visibility flags for all future MovableObject instances.
		*/
		static uint32 getDefaultVisibilityFlags(uint32 flags) { return msDefaultVisibilityFlags; }

		/// Define a default implementation of method from ShadowCaster which implements no shadows
        EdgeData* getEdgeList(void) { return NULL; }
        /// Define a default implementation of method from ShadowCaster which implements no shadows
        ShadowRenderableListIterator getShadowVolumeRenderableIterator(
            ShadowTechnique shadowTechnique, const Light* light, 
            HardwareIndexBufferSharedPtr* indexBuffer, 
            bool extrudeVertices, Real extrusionDist, unsigned long flags = 0);
		
        /** Overridden member from ShadowCaster. */
        const AxisAlignedBox& getLightCapBounds(void) const;
        /** Overridden member from ShadowCaster. */
        const AxisAlignedBox& getDarkCapBounds(const Light& light, Real dirLightExtrusionDist) const;
        /** Sets whether or not this object will cast shadows.
        @remarks
        This setting simply allows you to turn on/off shadows for a given object.
        An object will not cast shadows unless the scene supports it in any case
        (see SceneManager::setShadowTechnique), and also the material which is
        in use must also have shadow casting enabled. By default all entities cast
        shadows. If, however, for some reason you wish to disable this for a single 
        object then you can do so using this method.
        @note This method normally refers to objects which block the light, but
        since Light is also a subclass of MovableObject, in that context it means
        whether the light causes shadows itself.
        */
        void setCastShadows(bool enabled) { mCastShadows = enabled; }
        /** Returns whether shadow casting is enabled for this object. */
        bool getCastShadows(void) const { return mCastShadows; }
        /** Get the distance to extrude for a point/spot light */
        Real getPointExtrusionDistance(const Light* l) const;
		/** Get the 'type flags' for this MovableObject.
		@remarks
			A type flag identifies the type of the MovableObject as a bitpattern. 
			This is used for categorical inclusion / exclusion in SceneQuery
			objects. By default, this method returns all ones for objects not 
			created by a MovableObjectFactory (hence always including them); 
			otherwise it returns the value assigned to the MovableObjectFactory.
			Custom objects which don't use MovableObjectFactory will need to 
			override this if they want to be included in queries.
		*/
		virtual uint32 getTypeFlags(void) const;





    };

	/** Interface definition for a factory class which produces a certain
		kind of MovableObject, and can be registered with Root in order
		to allow all clients to produce new instances of this object, integrated
		with the standard Ogre processing.
	*/
	class _OgreExport MovableObjectFactory 
	{
	protected:
		/// Type flag, allocated if requested
		unsigned long mTypeFlag;

		/// Internal implementation of create method - must be overridden
		virtual MovableObject* createInstanceImpl(
			const String& name, const NameValuePairList* params = 0) = 0;
	public:
		MovableObjectFactory() : mTypeFlag(0xFFFFFFFF) {}
		virtual ~MovableObjectFactory() {}
		/// Get the type of the object to be created
		virtual const String& getType(void) const = 0;

		/** Create a new instance of the object.
		@param name The name of the new object
		@param manager The SceneManager instance that will be holding the
			instance once created.
		@param params Name/value pair list of additional parameters required to 
			construct the object (defined per subtype). Optional.
		*/
		virtual MovableObject* createInstance(
			const String& name, SceneManager* manager, 
			const NameValuePairList* params = 0);
		/** Destroy an instance of the object */
		virtual void destroyInstance(MovableObject* obj) = 0;

		/** Does this factory require the allocation of a 'type flag', used to 
			selectively include / exclude this type from scene queries?
		@remarks
			The default implementation here is to return 'false', ie not to 
			request a unique type mask from Root. For objects that
			never need to be excluded in SceneQuery results, that's fine, since
			the default implementation of MovableObject::getTypeFlags is to return
			all ones, hence matching any query type mask. However, if you want the
			objects created by this factory to be filterable by queries using a 
			broad type, you have to give them a (preferably unique) type mask - 
			and given that you don't know what other MovableObject types are 
			registered, Root will allocate you one. 
		*/
		virtual bool requestTypeFlags(void) const { return false; }
		/** Notify this factory of the type mask to apply. 
		@remarks
			This should normally only be called by Root in response to
			a 'true' result from requestTypeMask. However, you can actually use
			it yourself if you're careful; for example to assign the same mask
			to a number of different types of object, should you always wish them
			to be treated the same in queries.
		*/
		void _notifyTypeFlags(unsigned long flag) { mTypeFlag = flag; }

		/** Gets the type flag for this factory.
		@remarks
			A type flag is like a query flag, except that it applies to all instances
			of a certain type of object.
		*/
		unsigned long getTypeFlags(void) const { return mTypeFlag; }

	};

}
#endif
