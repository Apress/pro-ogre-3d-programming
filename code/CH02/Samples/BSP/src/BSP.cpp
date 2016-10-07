/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/

#include "Ogre.h"
#include "ExampleApplication.h"
#include "ExampleLoadingBar.h"

/**
    \file 
        BSP.cpp
    \brief
        Shows the indoor level rendering (Binary Space Partition or BSP based).
    \par
        Also demonstrates loading levels from Quake3Arena and using
        curved bezier surfaces (as demonstrated in the Bezier example)
        in a large level.
*/

class BspApplication : public ExampleApplication
{
public:
	BspApplication()
	{


	}

protected:

	String mQuakePk3;
	String mQuakeLevel;
	ExampleLoadingBar mLoadingBar;

	void loadResources(void)
	{

		mLoadingBar.start(mWindow, 1, 1, 0.75);

		// Turn off rendering of everything except overlays
		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->addSpecialCaseRenderQueue(RENDER_QUEUE_OVERLAY);
		mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_INCLUDE);

		// Set up the world geometry link
		ResourceGroupManager::getSingleton().linkWorldGeometryToResourceGroup(
			ResourceGroupManager::getSingleton().getWorldResourceGroupName(), 
			mQuakeLevel, mSceneMgr);

		// Initialise the rest of the resource groups, parse scripts etc
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		ResourceGroupManager::getSingleton().loadResourceGroup(
			ResourceGroupManager::getSingleton().getWorldResourceGroupName(),
			false, true);

		// Back to full rendering
		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_EXCLUDE);

		mLoadingBar.finish();


	}

	// Override resource sources (include Quake3 archives)
	void setupResources(void)
	{

		// Load Quake3 locations from a file
		ConfigFile cf;

		cf.load("quake3settings.cfg");

		mQuakePk3 = cf.getSetting("Pak0Location");
		mQuakeLevel = cf.getSetting("Map");

		ExampleApplication::setupResources();
		ResourceGroupManager::getSingleton().addResourceLocation(
			mQuakePk3, "Zip", ResourceGroupManager::getSingleton().getWorldResourceGroupName());

	}
	// Override scene manager (use indoor instead of generic)
	void chooseSceneManager(void)
	{
		mSceneMgr = mRoot->createSceneManager("BspSceneManager");
	}
	// Scene creation
	void createScene(void)
	{

		// modify camera for close work
		mCamera->setNearClipDistance(4);
		mCamera->setFarClipDistance(4000);

		// Also change position, and set Quake-type orientation
		// Get random player start point
		ViewPoint vp = mSceneMgr->getSuggestedViewpoint(true);
		mCamera->setPosition(vp.position);
		mCamera->pitch(Degree(90)); // Quake uses X/Y horizon, Z up
		mCamera->rotate(vp.orientation);
		// Don't yaw along variable axis, causes leaning
		mCamera->setFixedYawAxis(true, Vector3::UNIT_Z);


	}

};


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char**argv)
#endif
{
    // Create application object
    BspApplication app;

    try {
        app.go();
    } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occured: " << 
            e.getFullDescription().c_str() << std::endl;
#endif
    }

    return 0;
}
