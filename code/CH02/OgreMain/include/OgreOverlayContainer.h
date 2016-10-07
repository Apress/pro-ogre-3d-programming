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

#ifndef __OverlayContainer_H__
#define __OverlayContainer_H__

#include "OgrePrerequisites.h"
#include "OgreOverlayElement.h"
#include "OgreIteratorWrappers.h"


namespace Ogre {


    /** A 2D element which contains other OverlayElement instances.
    @remarks
        This is a specialisation of OverlayElement for 2D elements that contain other
        elements. These are also the smallest elements that can be attached directly
        to an Overlay.
    @remarks
        OverlayContainers should be managed using OverlayManager. This class is responsible for
        instantiating / deleting elements, and also for accepting new types of element
        from plugins etc.
    */
    class _OgreExport OverlayContainer : public OverlayElement
    {
    public:
        typedef std::map<String, OverlayElement*> ChildMap;
        typedef MapIterator<ChildMap> ChildIterator;
        typedef std::map<String, OverlayContainer*> ChildContainerMap;
        typedef MapIterator<ChildContainerMap> ChildContainerIterator;
    protected:
        // Map of all children
        ChildMap mChildren;
        // Map of container children (subset of mChildren)
        ChildContainerMap mChildContainers;

		bool mChildrenProcessEvents;
 
    public:
        /// Constructor: do not call direct, use OverlayManager::createOverlayElement
        OverlayContainer(const String& name);
        virtual ~OverlayContainer();

        /** Adds another OverlayElement to this container. */
        virtual void addChild(OverlayElement* elem);
        /** Adds another OverlayElement to this container. */
        virtual void addChildImpl(OverlayElement* elem);
        /** Add a nested container to this container. */
        virtual void addChildImpl(OverlayContainer* cont);
        /** Removes a named element from this container. */
        virtual void removeChild(const String& name);
        /** Gets the named child of this container. */
        virtual OverlayElement* getChild(const String& name);

		/** @copydoc OverlayElement::initialise */
		void initialise(void);

        void _addChild(OverlayElement* elem);
        void _removeChild(OverlayElement* elem) { _removeChild(elem->getName()); }
        void _removeChild(const String& name);

        /** Gets an object for iterating over all the children of this object. */
        virtual ChildIterator getChildIterator(void);

        /** Gets an iterator for just the container children of this object.
        @remarks
            Good for cascading updates without having to use RTTI
        */
        virtual ChildContainerIterator getChildContainerIterator(void);

		/** Tell the object and its children to recalculate */
		virtual void _positionsOutOfDate(void);

        /** Overridden from OverlayElement. */
        virtual void _update(void);

        /** Overridden from OverlayElement. */
        virtual void _notifyZOrder(ushort newZOrder);

        /** Overridden from OverlayElement. */
        virtual void _notifyViewport();

        /** Overridden from OverlayElement. */
        virtual void _notifyWorldTransforms(const Matrix4& xform);

        /** Overridden from OverlayElement. */
	    virtual void _notifyParent(OverlayContainer* parent, Overlay* overlay);

        /** Overridden from OverlayElement. */
        virtual void _updateRenderQueue(RenderQueue* queue);

        /** Overridden from OverlayElement. */
		inline bool isContainer() const
		{ return true; }

		/** Should this container pass events to their children */
		virtual inline bool isChildrenProcessEvents() const
		{ return true; }

		/** Should this container pass events to their children */
		virtual inline void setChildrenProcessEvents(bool val)
		{ mChildrenProcessEvents = val; }

        /** This returns a OverlayElement at position x,y. */
		virtual OverlayElement* findElementAt(Real x, Real y);		// relative to parent

	    void copyFromTemplate(OverlayElement* templateOverlay);
        virtual OverlayElement* clone(const String& instanceName);

    };



}


#endif

