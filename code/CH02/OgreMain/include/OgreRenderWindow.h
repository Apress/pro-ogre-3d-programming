/*-------------------------------------------------------------------------
This source file is a part of OGRE
(Object-oriented Graphics Rendering Engine)

For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License (LGPL) as 
published by the Free Software Foundation; either version 2.1 of the 
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
License for more details.

You should have received a copy of the GNU Lesser General Public License 
along with this library; if not, write to the Free Software Foundation, 
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA or go to
http://www.gnu.org/copyleft/lesser.txt
-------------------------------------------------------------------------*/
#ifndef __RenderWindow_H__
#define __RenderWindow_H__

#include "OgrePrerequisites.h"

#include "OgreRenderTarget.h"

namespace Ogre
{
    /** Manages the target rendering window.
        @remarks
            This class handles a window into which the contents
            of a scene are rendered. There is a many-to-1 relationship
            between instances of this class an instance of RenderSystem
            which controls the rendering of the scene. There may be
            more than one window in the case of level editor tools etc.
            This class is abstract since there may be
            different implementations for different windowing systems.
        @remarks
            Instances are created and communicated with by the render system
            although client programs can get a reference to it from
            the render system if required for resizing or moving.
            Note that you can have multiple viewpoints
            in the window for effects like rear-view mirrors and
            picture-in-picture views (see Viewport and Camera).
        @author
            Steven Streeting
        @version
            1.0
    */
    class _OgreExport RenderWindow : public RenderTarget
    {

    public:
        /** Default constructor.
        */
        RenderWindow();

        /** Creates & displays the new window.
            @param
                width The width of the window in pixels.
            @param
                height The height of the window in pixels.
            @param
                colourDepth The colour depth in bits. Ignored if
                fullScreen is false since the desktop depth is used.
            @param
                fullScreen If true, the window fills the screen,
                with no title bar or border.
            @param
                left The x-position of the window. Ignored if
                fullScreen = true.
            @param
                top The y-position of the window. Ignored if
                fullScreen = true.
            @param
                depthBuffer Specify true to include a depth-buffer.
            @param
                miscParam A variable number of pointers to platform-specific arguments. The
                actual requirements must be defined by the implementing subclasses.
        */
		virtual void create(const String& name, unsigned int width, unsigned int height,
	            bool fullScreen, const NameValuePairList *miscParams) = 0;
        
        /** Destroys the window.
        */
        virtual void destroy(void) = 0;

        /** Alter the size of the window.
        */
        virtual void resize(unsigned int width, unsigned int height) = 0;

		/** Notify that the window has been resized externally.
		@remarks
			You don't need to call this unless you created the window externally.
		*/
		virtual void windowMovedOrResized() {}

        /** Reposition the window.
        */
        virtual void reposition(int left, int top) = 0;

        /** Indicates whether the window is visible (not minimized or obscured)
        */
        virtual bool isVisible(void) const { return true; }

        /** Overridden from RenderTarget, flags invisible windows as inactive
        */
        virtual bool isActive(void) const { return mActive && isVisible(); }

        /** Indicates whether the window has been closed by the user.
        */
        virtual bool isClosed(void) const = 0;
        
        /** Indicates wether the window is the primary window. The
        	primary window is special in that it is destroyed when 
        	ogre is shut down, and cannot be destroyed directly.
        	This is the case because it holds the context for vertex,
        	index buffers and textures.
        */
        virtual bool isPrimary(void) const;

        /** Swaps the frame buffers to display the next frame.
            @remarks
                All render windows are double-buffered so that no
                'in-progress' versions of the scene are displayed
                during rendering. Once rendering has completed (to
                an off-screen version of the window) the buffers
                are swapped to display the new frame.

            @param
                waitForVSync If true, the system waits for the
                next vertical blank period (when the CRT beam turns off
                as it travels from bottom-right to top-left at the
                end of the pass) before flipping. If false, flipping
                occurs no matter what the beam position. Waiting for
                a vertical blank can be slower (and limits the
                framerate to the monitor refresh rate) but results
                in a steadier image with no 'tearing' (a flicker
                resulting from flipping buffers when the beam is
                in the progress of drawing the last frame).
        */
        virtual void swapBuffers(bool waitForVSync = true) = 0;

		/// @copydoc RenderTarget::update
        virtual void update(void);
        /** Updates the window contents.
            @remarks
                The window is updated by telling each camera which is supposed
                to render into this window to render it's view, and then
                the window buffers are swapped via swapBuffers() if requested
			@param swapBuffers If set to true, the window will immediately
				swap it's buffers after update. Otherwise, the buffers are
				not swapped, and you have to call swapBuffers yourself sometime
				later. You might want to do this on some rendersystems which 
				pause for queued rendering commands to complete before accepting
				swap buffers calls - so you could do other CPU tasks whilst the 
				queued commands complete. Or, you might do this if you want custom
				control over your windows, such as for externally created windows.
        */
        virtual void update(bool swapBuffers);

        /** Returns true if window is running in fullscreen mode.
        */
        virtual bool isFullScreen(void) const;

        /** Overloaded version of getMetrics from RenderTarget, including extra details
            specific to windowing systems.
        */
        virtual void getMetrics(unsigned int& width, unsigned int& height, unsigned int& colourDepth, 
			int& left, int& top);

    protected:
        bool mIsFullScreen;
        bool mIsPrimary;
        int mLeft;
        int mTop;
        
        /** Indicates that this is the primary window. Only to be called by
            Ogre::Root
        */
        void _setPrimary() { mIsPrimary = true; }
        
        friend class Root;
    };

    /** Defines the interface a DLL implemeting a platform-specific version must implement.
        @remarks
            Any library (.dll, .so) wishing to implement a platform-specific version of this
            dialog must export the symbol 'createRenderWindow' with the signature
            void createPlatformRenderWindow(RenderWindow** ppDlg)
    */
    typedef void (*DLL_CREATERENDERWINDOW)(RenderWindow** ppWindow);

} // Namespace
#endif
