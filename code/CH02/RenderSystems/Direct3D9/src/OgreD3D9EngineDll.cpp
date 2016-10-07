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
#include "OgreD3D9RenderSystem.h"
#include "OgreRoot.h"
#include "OgreD3D9HLSLProgramFactory.h"

namespace Ogre 
{
	D3D9RenderSystem* d3dRendPlugin;
	D3D9HLSLProgramFactory* hlslProgramFactory;

	extern "C" void dllStartPlugin(void) throw()
	{
		// Create the DirectX 8 rendering api
		HINSTANCE hInst = GetModuleHandle( "RenderSystem_Direct3D9.dll" );
		d3dRendPlugin = new D3D9RenderSystem( hInst );
		// Register the render system
		Root::getSingleton().addRenderSystem( d3dRendPlugin );

        // create & register HLSL factory
        hlslProgramFactory = new D3D9HLSLProgramFactory();
        HighLevelGpuProgramManager::getSingleton().addFactory(hlslProgramFactory);

	}

	extern "C" void dllStopPlugin(void)
	{
		delete d3dRendPlugin;
		delete hlslProgramFactory;
	}
}
