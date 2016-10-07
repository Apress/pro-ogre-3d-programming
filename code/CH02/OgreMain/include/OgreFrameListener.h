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
#ifndef __FrameListener_H__
#define __FrameListener_H__


#include "OgrePrerequisites.h"

namespace Ogre {

    /** Struct containing information about a frame event.
    */
    struct FrameEvent
    {
        /** Elapsed time in seconds since the last event.
            This gives you time between frame start & frame end,
            and between frame end and next frame start.
            @remarks
                This may not be the elapsed time but the average
                elapsed time between recently fired events.
        */
        Real timeSinceLastEvent;
        /** Elapsed time in seconds since the last event of the same type,
            i.e. time for a complete frame.
            @remarks
                This may not be the elapsed time but the average
                elapsed time between recently fired events of the same type.
        */
        Real timeSinceLastFrame;
    };


    /** A interface class defining a listener which can be used to receive
        notifications of frame events.
        @remarks
            A 'listener' is an interface designed to be called back when
            particular events are called. This class defines the
            interface relating to frame events. In order to receive
            notifications of frame events, you should create a subclass of
            FrameListener and override the methods for which you would like
            to customise the resulting processing. You should then call
            Root::addFrameListener passing an instance of this class.
            There is no limit to the number of frame listeners you can register,
            allowing you to register multiple listeners for different purposes.
            Frame events only occur when Ogre is in continuous rendering mode,
            ie. after Root::startRendering is called. If the application is
            doing ad-hoc rendering without entering a rendering loop, frame
            events are not generated. Note that a frame event occurs once for
            all rendering targets, not once per target.
    */
    class _OgreExport FrameListener
    {
        /*
        Note that this could have been an abstract class, but I made
        the explicit choice not to do this, because I wanted to give
        people the option of only implementing the methods they wanted,
        rather than having to create 'do nothing' implementations for
        those they weren't interested in. As such this class follows
        the 'Adapter' classes in Java rather than pure interfaces.
        */
    public:
        /** Called when a frame is about to begin rendering.
            @return
                True to go ahead, false to abort rendering and drop
                out of the rendering loop.
        */
        virtual bool frameStarted(const FrameEvent& evt) { return true; }
        /** Called just after a frame has been rendered.
            @return
                True to continue with the next frame, false to drop
                out of the rendering loop.
        */
        virtual bool frameEnded(const FrameEvent& evt) { return true; }

		virtual ~FrameListener() {}
		
    };
}

#endif
