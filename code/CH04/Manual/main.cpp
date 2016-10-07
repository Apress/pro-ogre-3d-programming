#include "Ogre.h"
#include <iostream>

#if defined(WIN32)
#include <windows.h>
#endif

using namespace Ogre;

#if defined(WIN32)
INT WINAPI WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
#else
int main(int argc, char *argv[]) 
#endif
{

	// tell Root not to load from any plugins or settings file
	Root *root = new Root("", "");

	// Load feature plugins. Scene managers will register 
	// themselves for all scene types they support
	root->loadPlugin("Plugin_CgProgramManager");
	root->loadPlugin("Plugin_OctreeSceneManager");

	// load rendersystem plugin(s). The order is important in that GL
	// should be available on on platforms, while D3D9 would be available 
	// only on Windows -- the try/catch will intercept the exception in this
	// case where D3D9 is not available and continue gracefully.
	try {
		root->loadPlugin("RenderSystem_GL");
		root->loadPlugin("RenderSystem_Direct3D9");
	}
	catch (...) {}

	try {
		// We'll simulate the selection of a rendersystem on an arbirtary basis; normally
		// you would have your own code to present the user with options and select the
		// rendersystem on that basis. Since a GUI is beyond the scope of this example, we'll
		// just assume the user selected OpenGL.
		RenderSystemList *rList = root->getAvailableRenderers();
		RenderSystemList::iterator it = rList->begin();
		RenderSystem *rSys = 0;

		while (it != rList->end()) {
			
			rSys = *(it++);
			if (rSys->getName().find("OpenGL")) {
			
				root->setRenderSystem(rSys);
				break;
			}
		}

		// check to see if a render system was selected; if we reached the end of the list
		// without selecting a render system then none was found.
		if (rSys == 0) {
			delete root;
			std::cerr << "No RenderSystem available, exiting..." << std::endl;
			return -1;
		}

		// We can initialize Root here if we want. "false" tells Root NOT to create
		// a render window for us
		root->initialise(false);

		// set up the render window with all default params
		RenderWindow *window = rSys->createRenderWindow(
			"Manual Ogre Window",	// window title
			800,					// window width, in pixels
			600,					// window height, in pixels
			false,					// fullscreen or not 
			0);						// use defaults for all other values

		// from here you can set up your camera and viewports as normal
		// get a pointer to the default base scene manager -- sufficient for our purposes
		SceneManager *sceneMgr = root->createSceneManager(ST_GENERIC);

		// create a single camera, and a viewport that takes up the whole window (default behavior)
		Camera *camera = sceneMgr->createCamera("MainCam");
		Viewport *vp = window->addViewport(camera);
		vp->setDimensions(0.0f, 0.0f, 1.0f, 1.0f);
		camera->setAspectRatio((float)vp->getActualWidth() / (float) vp->getActualHeight());
		camera->setFarClipDistance(1000.0f);
		camera->setNearClipDistance(5.0f);

		// Run the manual render loop. Since we are not using a frame listener in this case, we
		// will count to 15 seconds and then instead of exiting, we'll change the render window settings 
		// and re-initialize it.
		bool renderLoop = true;
		Timer *timer = PlatformManager::getSingleton().createTimer();
		timer->reset();
		float s = 0.0f;

		while (renderLoop && window->isActive()) {

			renderLoop = root->renderOneFrame();

			// accumulate total elapsed time
			s += (float)timer->getMilliseconds() / 1000.0f;

			// if greater than 15 seconds, break out of the loop
			if (s >= 15.0f)
				renderLoop = false;

			// we must call the windowing system's message pump each frame to 
			// allow Ogre to process messages 
			//PlatformManager::getSingleton().messagePump();
		}
	}
	catch (Exception &e) {
		std::cerr << e.getFullDescription() << std::endl;
	}

	delete root;
	return 0;
}