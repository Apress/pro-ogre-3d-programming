#ifndef __OgreWin32GLSupport_H__
#define __OgreWin32GLSupport_H__

#include "OgreWin32Prerequisites.h"
#include "OgreGLSupport.h"
#include "OgreGLRenderSystem.h"

namespace Ogre
{
    
	class Win32GLSupport : public GLSupport
	{
	public:
        Win32GLSupport();
		/**
		* Add any special config values to the system.
		* Must have a "Full Screen" value that is a bool and a "Video Mode" value
		* that is a string in the form of wxhxb
		*/
		void addConfig();

		void setConfigOption(const String &name, const String &value);

		/**
		* Make sure all the extra options are valid
		*/
		String validateConfig();

		virtual RenderWindow* createWindow(bool autoCreateWindow, GLRenderSystem* renderSystem, const String& windowTitle = "OGRE Render Window");

		/// @copydoc RenderSystem::createRenderWindow
		virtual RenderWindow* newWindow(const String &name, unsigned int width, unsigned int height, 
			bool fullScreen, const NameValuePairList *miscParams = 0);

		
		/**
		* Start anything special
		*/
		void start();
		/**
		* Stop anything special
		*/
		void stop();

		/**
		* Get the address of a function
		*/
		void* getProcAddress(const String& procname);

		/**
		 * Initialise extensions
		 */
		virtual void initialiseExtensions();
		

		bool selectPixelFormat(HDC hdc, int colourDepth, int multisample = 0);

		virtual bool supportsPBuffers();
		virtual GLPBuffer *createPBuffer(PixelComponentType format, size_t width, size_t height);
	private:
		// Allowed video modes
		std::vector<DEVMODE> mDevModes;
		Win32Window *mInitialWindow;
		std::vector<int> mFSAALevels;
		bool mHasPixelFormatARB;

		void refreshConfig();
		void initialiseWGL();
		static LRESULT CALLBACK dummyWndProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp);
	};

}

#endif
