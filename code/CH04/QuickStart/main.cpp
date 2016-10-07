#include "Ogre.h"
#include "OgreErrorDialog.h"
#include "FrameListener.h"

#if defined(WIN32)
#	include <windows.h>
#endif

using namespace Ogre;

bool MyFrameListener::frameStarted(const FrameEvent &evt) {
	
	m_timeElapsed += evt.timeSinceLastFrame;

	if (m_timeElapsed > 15.0f) 
		return false;
	else
		return true;
}


#if defined (WIN32)
INT WINAPI WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
#else
int main (int argc, char *argv[])
#endif
{
	MyFrameListener listener;

	// We wrap the entire process in a top-level try/catch, because Ogre will
	// throw exceptions in dire circumstances. In these cases, the catch block will
	// display the exception text, which is also logged to Ogre.log in the same directory
	// as the executable.
	
	Root *root = 0;

	try {
		root = new Root;

		// try first to restore an existing config
		if (!root->restoreConfig()) {

			// if no existing config, or could not restore it, show the config dialog
			if (!root->showConfigDialog()) {

				// if the user pressed Cancel, clean up and exit
				delete root;
				return 0;
			}
		}

		// initialize Root -- have it create a render window for us
		root->initialise(true);

		// get a pointer to the auto-created window
		RenderWindow *window = root->getAutoCreatedWindow();

		// get a pointer to the default base scene manager -- sufficient for our purposes
		SceneManager *sceneMgr = root->createSceneManager(ST_GENERIC);

		// create a single camera, and a viewport that takes up the whole window (default behavior)
		Camera *camera = sceneMgr->createCamera("MainCam");
		Viewport *vp = window->addViewport(camera);
		vp->setDimensions(0.0f, 0.0f, 1.0f, 1.0f);
		camera->setAspectRatio((float)vp->getActualWidth() / (float) vp->getActualHeight());
		camera->setFarClipDistance(1000.0f);
		camera->setNearClipDistance(5.0f);

		// register our frame listener
		root->addFrameListener(&listener);

		// tell Ogre to start rendering -- our frame listener will cause the app to exit
		// after 15 seconds have elapsed
		root->startRendering();
	}
	catch (Exception &e) {
		ErrorDialog *ed = PlatformManager::getSingleton().createErrorDialog();
		ed->display(e.getFullDescription());
	}

	// clean up and exit
	delete root;
	return 0;
}