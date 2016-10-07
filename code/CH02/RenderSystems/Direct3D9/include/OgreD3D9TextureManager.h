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
#ifndef __D3D9TEXTUREMANAGER_H__
#define __D3D9TEXTUREMANAGER_H__

#include "OgreD3D9Prerequisites.h"
#include "OgreTextureManager.h"

#include "OgreNoMemoryMacros.h"
#include <d3d9.h>
#include "OgreMemoryMacros.h"

namespace Ogre 
{
	class D3D9TextureManager : public TextureManager
	{
	protected:
		LPDIRECT3DDEVICE9 mpD3DDevice;
        /// @copydoc ResourceManager::createImpl
        Resource* createImpl(const String& name, ResourceHandle handle, 
            const String& group, bool isManual, ManualResourceLoader* loader, 
            const NameValuePairList* createParams);

	public:
		D3D9TextureManager( LPDIRECT3DDEVICE9 pD3DDevice );
		~D3D9TextureManager();

		/** Release all textures in the default memory pool. 
		@remarks
			Method for dealing with lost devices.
		*/
		void releaseDefaultPoolResources(void);
		/** Recreate all textures in the default memory pool. 
		@remarks
			Method for dealing with lost devices.
		*/
		void recreateDefaultPoolResources(void);

		/// @copydoc TextureManager::getNativeFormat
		PixelFormat getNativeFormat(TextureType ttype, PixelFormat format, int usage);


	};
}
#endif
