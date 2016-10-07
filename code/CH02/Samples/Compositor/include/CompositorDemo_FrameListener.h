/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 The OGRE Team
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/

#ifndef _CompositorDemo_FrameListener_H_
#define _CompositorDemo_FrameListener_H_

#include <OgreFrameListener.h>
#include <OgreEventQueue.h>
#include <OgreKeyEvent.h>
#include <OgreEventListeners.h>

#include "ItemSelectorViewManager.h"

//---------------------------------------------------------------------------
    class HeatVisionListener: public Ogre::CompositorInstance::Listener
    {
    public:
        HeatVisionListener();
        virtual ~HeatVisionListener();
        virtual void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
        virtual void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
    protected:
        Ogre::GpuProgramParametersSharedPtr fpParams;
        float start, end, curr;
        Ogre::Timer *timer;
    };
	//---------------------------------------------------------------------------
	class HDRListener: public Ogre::CompositorInstance::Listener
	{
	protected:
		int mVpWidth, mVpHeight;
		int mLumSize[5];
		int mBloomSize;
		// Array params - have to pack in groups of 4 since this is how Cg generates them
		// also prevents dependent texture read problems if ops don't require swizzle
		float mBloomTexWeights[15][4];
		float mBloomTexOffsetsHorz[15][4];
		float mBloomTexOffsetsVert[15][4];
	public:
		HDRListener();
		virtual ~HDRListener();
		void notifyViewportSize(int width, int height);
		void notifyCompositor(Ogre::CompositorInstance* instance);
		virtual void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
		virtual void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
	};
	//---------------------------------------------------------------------------
	class GaussianListener: public Ogre::CompositorInstance::Listener
	{
	protected:
		int mVpWidth, mVpHeight;
		// Array params - have to pack in groups of 4 since this is how Cg generates them
		// also prevents dependent texture read problems if ops don't require swizzle
		float mBloomTexWeights[15][4];
		float mBloomTexOffsetsHorz[15][4];
		float mBloomTexOffsetsVert[15][4];
	public:
		GaussianListener();
		virtual ~GaussianListener();
		void notifyViewportSize(int width, int height);
		virtual void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
		virtual void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
	};
//---------------------------------------------------------------------------
    class CompositorDemo;

    class CompositorDemo_FrameListener : Ogre::FrameListener, Ogre::KeyListener,
        Ogre::MouseMotionListener, Ogre::MouseListener, ItemSelectorInterface
    {
    #define MINSPEED .150f
    #define MOVESPEED 30
    #define MAXSPEED 1.800f


    protected:
        CompositorDemo* mMain;
        HeatVisionListener *hvListener;
		HDRListener *hdrListener;
		GaussianListener *gaussianListener;
        Ogre::Vector3 mTranslateVector;
        bool mStatsOn;
        unsigned int mNumScreenShots;
        bool mWriteToFile;
        float mSkipCount;
        float mUpdateFreq;
        int mSceneDetailIndex;
        Ogre::TextureFilterOptions mFiltering;
        int mAniso;
        bool mQuit;

        float mMoveScale;
        float mRotScale;
        float mSpeed;
        float mAvgFrameTime;
        Ogre::Real mMoveSpeed;
        Ogre::Real mRotateSpeed;
        CEGUI::Point mLastMousePosition;
        bool mLastMousePositionSet;
        // just to stop toggles flipping too fast
        Ogre::Real mTimeUntilNextToggle ;
        float mRotX, mRotY;
        bool mProcessMovement;
        bool mUpdateMovement;
        bool mLMBDown;
        bool mRMBDown;
        bool mMoveFwd;
        bool mMoveBck;
        bool mMoveLeft;
        bool mMoveRight;
		Ogre::SceneNode* mSpinny;

        ItemSelectorViewManager* mCompositorSelectorViewManager;

        Ogre::EventProcessor* mEventProcessor;
        Ogre::InputReader* mInputDevice;


        CEGUI::Renderer* mGuiRenderer;
        CEGUI::Window* mGuiAvg;
        CEGUI::Window* mGuiCurr;
        CEGUI::Window* mGuiBest;
        CEGUI::Window* mGuiWorst;
        CEGUI::Window* mGuiTris;
        CEGUI::Window* mGuiDbg;
        CEGUI::Window* mRoot;
		CEGUI::Listbox* mDebugRTTListbox;
		CEGUI::StaticImage* mDebugRTTStaticImage;
		typedef std::vector<CEGUI::Imageset*> ImageSetList;
		ImageSetList mDebugRTTImageSets;

        CEGUI::MouseButton convertOgreButtonToCegui(int ogre_button_id);
        void CheckMovementKeys( CEGUI::Key::Scan keycode, bool state );
        void updateStats(void);
        void registerCompositors(void);
		void initDebugRTTWindow(void);
		void updateDebugRTTWindow(void);

    public:
        CompositorDemo_FrameListener(CompositorDemo* main);
        virtual ~CompositorDemo_FrameListener();
		void setSpinningNode(Ogre::SceneNode* node) { mSpinny = node; }

    private:
        void connectEventHandlers(void);
        virtual void mouseMoved (Ogre::MouseEvent *e);
        virtual void mouseDragged (Ogre::MouseEvent *e);
        virtual void keyPressed (Ogre::KeyEvent *e);
        virtual void keyReleased (Ogre::KeyEvent *e);
        virtual void mousePressed (Ogre::MouseEvent *e);
        virtual void mouseReleased (Ogre::MouseEvent *e);

        // do-nothing events
        virtual void keyClicked (Ogre::KeyEvent *e) {}
        virtual void mouseClicked (Ogre::MouseEvent *e) {}
        virtual void mouseEntered (Ogre::MouseEvent *e) {}
        virtual void mouseExited (Ogre::MouseEvent *e) {}

        // Event handlers
        bool frameStarted(const Ogre::FrameEvent& evt);
        bool handleMouseMove(const CEGUI::EventArgs& e);
        bool handleMouseButtonUp(const CEGUI::EventArgs& e);
        bool handleMouseButtonDown(const CEGUI::EventArgs& e);
        bool handleMouseWheelEvent(const CEGUI::EventArgs& e);
        bool handleKeyDownEvent(const CEGUI::EventArgs& e);
        bool handleKeyUpEvent(const CEGUI::EventArgs& e);
		bool handleRttSelection(const CEGUI::EventArgs& e);
        void itemStateChanged(const size_t index, const bool state);
    };

#endif
