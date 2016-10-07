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
#ifndef __RenderTarget_H__
#define __RenderTarget_H__

#include "OgrePrerequisites.h"

#include "OgreString.h"
#include "OgreTextureManager.h"
#include "OgreViewport.h"
#include "OgreTimer.h"

/* Define the number of priority groups for the render system's render targets. */
#ifndef OGRE_NUM_RENDERTARGET_GROUPS
	#define OGRE_NUM_RENDERTARGET_GROUPS 10
	#define OGRE_DEFAULT_RT_GROUP 4
	#define OGRE_REND_TO_TEX_RT_GROUP 2
#endif

namespace Ogre {

    /** A 'canvas' which can receive the results of a rendering
        operation.
        @remarks
            This abstract class defines a common root to all targets of rendering operations. A
            render target could be a window on a screen, or another
            offscreen surface like a texture or bump map etc.
        @author
            Steven Streeting
        @version
            1.0
     */
    class _OgreExport RenderTarget
    {
    public:
        enum StatFlags
        {
            SF_NONE           = 0,
            SF_FPS            = 1,
            SF_AVG_FPS        = 2,
            SF_BEST_FPS       = 4,
            SF_WORST_FPS      = 8,
            SF_TRIANGLE_COUNT = 16,
            SF_ALL            = 0xFFFF
        };

        struct FrameStats
        {
            float lastFPS;
            float avgFPS;
            float bestFPS;
            float worstFPS;
            unsigned long bestFrameTime;
            unsigned long worstFrameTime;
            size_t triangleCount;
        };

        RenderTarget();
        virtual ~RenderTarget();

        /// Retrieve target's name.
        virtual const String& getName(void) const;

        /// Retrieve information about the render target.
        virtual void getMetrics(unsigned int& width, unsigned int& height, unsigned int& colourDepth);

        virtual unsigned int getWidth(void) const;
        virtual unsigned int getHeight(void) const;
        virtual unsigned int getColourDepth(void) const;

        /** Tells the target to update it's contents.
            @remarks
                If OGRE is not running in an automatic rendering loop
                (started using Root::startRendering),
                the user of the library is responsible for asking each render
                target to refresh. This is the method used to do this. It automatically
                re-renders the contents of the target using whatever cameras have been
                pointed at it (using Camera::setRenderTarget).
            @par
                This allows OGRE to be used in multi-windowed utilities
                and for contents to be refreshed only when required, rather than
                constantly as with the automatic rendering loop.
        */
        virtual void update(void);

        /** Adds a viewport to the rendering target.
            @remarks
                A viewport is the rectangle into which redering output is sent. This method adds
                a viewport to the render target, rendering from the supplied camera. The
                rest of the parameters are only required if you wish to add more than one viewport
                to a single rendering target. Note that size information passed to this method is
                passed as a parametric, i.e. it is relative rather than absolute. This is to allow
                viewports to automatically resize along with the target.
            @param
                cam The camera from which the viewport contents will be rendered (mandatory)
            @param
                ZOrder The relative order of the viewport with others on the target (allows overlapping
                viewports i.e. picture-in-picture). Higher ZOrders are on top of lower ones. The actual number
                is irrelevant, only the relative ZOrder matters (you can leave gaps in the numbering)
            @param
                left The relative position of the left of the viewport on the target, as a value between 0 and 1.
            @param
                top The relative position of the top of the viewport on the target, as a value between 0 and 1.
            @param
                width The relative width of the viewport on the target, as a value between 0 and 1.
            @param
                height The relative height of the viewport on the target, as a value between 0 and 1.
        */
        virtual Viewport* addViewport(Camera* cam, int ZOrder = 0, float left = 0.0f, float top = 0.0f ,
            float width = 1.0f, float height = 1.0f);

        /** Returns the number of viewports attached to this target.*/
        virtual unsigned short getNumViewports(void) const;

        /** Retrieves a pointer to the viewport with the given index. */
        virtual Viewport* getViewport(unsigned short index);

        /** Removes a viewport at a given ZOrder.
        */
        virtual void removeViewport(int ZOrder);

        /** Removes all viewports on this target.
        */
        virtual void removeAllViewports(void);

        /** Retieves details of current rendering performance.
            @remarks
                If the user application wishes to do it's own performance
                display, or use performance for some other means, this
                method allows it to retrieve the statistics.
                @param
                    lastFPS Pointer to a float to receive the number of frames per second (FPS)
                    based on the last frame rendered.
                @param
                    avgFPS Pointer to a float to receive the FPS rating based on an average of all
                    the frames rendered since rendering began (the call to
                    Root::startRendering).
                @param
                    bestFPS Pointer to a float to receive the best FPS rating that has been achieved
                    since rendering began.
                @param
                    worstFPS Pointer to a float to receive the worst FPS rating seen so far.
        */
        virtual void getStatistics(float& lastFPS, float& avgFPS,
            float& bestFPS, float& worstFPS) const;  // Access to stats

        virtual const FrameStats& getStatistics(void) const;

        /** Individual stats access - gets the number of frames per second (FPS) based on the last frame rendered.
        */
        virtual float getLastFPS() const;

        /** Individual stats access - gets the average frames per second (FPS) since call to Root::startRendering.
        */
        virtual float getAverageFPS() const;

        /** Individual stats access - gets the best frames per second (FPS) since call to Root::startRendering.
        */
        virtual float getBestFPS() const;

        /** Individual stats access - gets the worst frames per second (FPS) since call to Root::startRendering.
        */
        virtual float getWorstFPS() const;

        /** Individual stats access - gets the best frame time
        */
        virtual float getBestFrameTime() const;

        /** Individual stats access - gets the worst frame time
        */
        virtual float getWorstFrameTime() const;

        /** Resets saved frame-rate statistices.
        */
        virtual void resetStatistics(void);

        /** Gets a custom (maybe platform-specific) attribute.
            @remarks
                This is a nasty way of satisfying any API's need to see platform-specific details.
                It horrid, but D3D needs this kind of info. At least it's abstracted.
            @param
                name The name of the attribute.
            @param
                pData Pointer to memory of the right kind of structure to receive the info.
        */
        virtual void getCustomAttribute(const String& name, void* pData);

        /** Adds debug text to this window. */
        virtual void setDebugText(const String& text);

		/** Returns the debug text. */
		const String& getDebugText() const;

        /** Add a listener to this RenderTarget which will be called back before & after rendering.
        @remarks
            If you want notifications before and after a target is updated by the system, use
            this method to register your own custom RenderTargetListener class. This is useful
            for potentially adding your own manual rendering commands before and after the
            'normal' system rendering.
        @par NB this should not be used for frame-based scene updates, use Root::addFrameListener for that.
        */
        virtual void addListener(RenderTargetListener* listener);
        /** Removes a RenderTargetListener previously registered using addListener. */
        virtual void removeListener(RenderTargetListener* listener);
        /** Removes all listeners from this instance. */
        virtual void removeAllListeners(void);

		/** Sets the priority of this render target in relation to the others. 
        @remarks
            This can be used in order to schedule render target updates. Lower
            priorities will be rendered first. Note that the priority must be set
            at the time the render target is attached to the render system, changes
            afterwards will not affect the ordering.
        */
        virtual void setPriority( uchar priority ) { mPriority = priority; }
        /** Gets the priority of a render target. */
		virtual uchar getPriority() const { return mPriority; }

        /** Used to retrieve or set the active state of the render target.
        */
        virtual bool isActive() const;

        /** Used to set the active state of the render target.
        */
        virtual void setActive( bool state );

        /** Sets whether this target should be automatically updated if Ogre's rendering
            loop or Root::_updateAllRenderTargets is being used.
        @remarks
            By default, if you use Ogre's own rendering loop (Root::startRendering)
            or call Root::_updateAllRenderTargets, all render targets are updated 
            automatically. This method allows you to control that behaviour, if 
            for example you have a render target which you only want to update periodically.
        @param autoupdate If true, the render target is updated during the automatic render
            loop or when Root::_updateAllRenderTargets is called. If false, the 
            target is only updated when its update() method is called explicitly.
        */
        virtual void setAutoUpdated(bool autoupdate);
        /** Gets whether this target is automatically updated if Ogre's rendering
            loop or Root::_updateAllRenderTargets is being used.
        */
        virtual bool isAutoUpdated(void) const;

        /** Writes the current contents of the render target to the named file. */
        virtual void writeContentsToFile(const String& filename) = 0;

	/** Writes the current contents of the render target to the (PREFIX)(time-stamp)(SUFFIX) file.
		@returns the name of the file used.*/
	virtual String writeContentsToTimestampedFile(const String& filenamePrefix, const String& filenameSuffix);

		virtual bool requiresTextureFlipping() const = 0;

		/** Gets the number of triangles rendered in the last update() call. */
		virtual size_t getTriangleCount(void) const;
        /** Utility method to notify a render target that a camera has been removed, 
        incase it was referring to it as a viewer. 
        */
        virtual void _notifyCameraRemoved(const Camera* cam);

        /** Indicates whether this target is the primary window. The
            primary window is special in that it is destroyed when 
            ogre is shut down, and cannot be destroyed directly.
            This is the case because it holds the context for vertex,
            index buffers and textures.
        */
        virtual bool isPrimary(void) const;


        /** RenderSystem specific interface for a RenderTarget;
            this should be subclassed by RenderSystems.
        */
        class Impl
        {
        protected:
            /** Declared protected as interface is never used for destruction.
                gcc will issue a warning here: `class Impl' has virtual functions 
                but non-virtual destructor. This is no problem because this interface 
                is never used to delete an object.
            */
            ~Impl() { };
        };
        /** Get rendersystem specific interface for this RenderTarget.
            This is used by the RenderSystem to (un)bind this target, 
            and to get specific information like surfaces
            and framebuffer objects.
        */
        virtual Impl *_getImpl();
    protected:
        /// The name of this target.
        String mName;
		/// The priority of the render target.
		uchar mPriority;

        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mColourDepth;
        bool mIsDepthBuffered;

        // Stats
		FrameStats mStats;
        
        Timer* mTimer ;
        String mDebugText;
        unsigned long mLastSecond;
        unsigned long mLastTime;
        size_t mFrameCount;

        bool mActive;
        bool mAutoUpdate;

        void updateStats(void);

        typedef std::map<int, Viewport*, std::less<int> > ViewportList;
        /// List of viewports, map on Z-order
        ViewportList mViewportList;

        typedef std::vector<RenderTargetListener*> RenderTargetListenerList;
        RenderTargetListenerList mListeners;
	

        /// internal method for firing events
        virtual void firePreUpdate(void);
        /// internal method for firing events
        virtual void firePostUpdate(void);
        /// internal method for firing events
        virtual void fireViewportPreUpdate(Viewport* vp);
        /// internal method for firing events
        virtual void fireViewportPostUpdate(Viewport* vp);
		/// internal method for firing events
		virtual void fireViewportAdded(Viewport* vp);
		/// internal method for firing events
		virtual void fireViewportRemoved(Viewport* vp);
    };

} // Namespace

#endif
