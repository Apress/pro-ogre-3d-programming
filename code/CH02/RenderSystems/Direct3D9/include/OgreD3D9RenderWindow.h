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
#ifndef __D3D9RENDERWINDOW_H__
#define __D3D9RENDERWINDOW_H__

#include "OgreD3D9Prerequisites.h"
#include "OgreRenderWindow.h"
#include "OgreD3D9Driver.h"

namespace Ogre 
{
	class D3D9RenderWindow : public RenderWindow
	{
	public:
		/** Constructor.
		@param instance The application instance
		@param driver The root driver
		@param deviceIfSwapChain The existing D3D device to create an additional swap chain from, if this is not
			the first window.
		*/
		D3D9RenderWindow(HINSTANCE instance, D3D9Driver *driver, LPDIRECT3DDEVICE9 deviceIfSwapChain = 0);
		~D3D9RenderWindow();
		void create(const String& name, unsigned int width, unsigned int height,
	            bool fullScreen, const NameValuePairList *miscParams);
		void destroy(void);
		bool isVisible() const;
		bool isClosed() const { return mClosed; }
		void reposition(int left, int top);
		void resize(unsigned int width, unsigned int height);
		void swapBuffers( bool waitForVSync = true );
		HWND getWindowHandle() const { return mHWnd; }

		D3D9Driver* getDirectD3DDriver() { return mDriver; }
		// changed to access driver member
		LPDIRECT3DDEVICE9 getD3DDevice() { return mDriver->getD3DDevice(); }

		void getCustomAttribute( const String& name, void* pData );
		/** Overridden - see RenderTarget.
		*/
		void writeContentsToFile(const String& filename);
		bool requiresTextureFlipping() const { return false; }

		// Method for dealing with resize / move & 3d library
		void windowMovedOrResized();

		/// Get the presentation parameters used with this window
		D3DPRESENT_PARAMETERS* getPresentationParameters(void) 
		{ return &md3dpp; }

		/// @copydoc RenderTarget::update
		void update(bool swap);

		/** Create (or recreate) the D3D device or SwapChain for this window.
		*/
		void createD3DResources();
	
		/** Destroy the D3D device or SwapChain for this window.
		*/
		void destroyD3DResources();
	
		/// Accessor for render surface
		LPDIRECT3DSURFACE9 getRenderSurface() { return mpRenderSurface; }
	protected:
		HINSTANCE mInstance;			// Process instance
		D3D9Driver *mDriver;			// D3D9 driver
		HWND	mHWnd;					// Win32 Window handle
		bool	mIsExternal;			// window not created by Ogre
		bool	mSizing;
		bool	mClosed;
		bool	mIsSwapChain;			// Is this a secondary window?

		static LRESULT CALLBACK WndProc(
			HWND hWnd,
			UINT uMsg,
			WPARAM wParam,
			LPARAM lParam );

		// -------------------------------------------------------
		// DirectX-specific
		// -------------------------------------------------------

		// Pointer to swap chain, only valid if mIsSwapChain
		LPDIRECT3DSWAPCHAIN9 mpSwapChain;
		D3DPRESENT_PARAMETERS md3dpp;
		LPDIRECT3DSURFACE9 mpRenderSurface;
		LPDIRECT3DSURFACE9 mpRenderZBuffer;
		D3DMULTISAMPLE_TYPE mFSAAType;
		DWORD mFSAAQuality;
		bool mVSync;
		bool mUseNVPerfHUD;

		// just check if the multisampling requested is supported by the device
		bool _checkMultiSampleQuality(D3DMULTISAMPLE_TYPE type, DWORD *outQuality, D3DFORMAT format, UINT adapterNum, D3DDEVTYPE deviceType, BOOL fullScreen);

	};
}
#endif
