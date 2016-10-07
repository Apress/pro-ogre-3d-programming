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
#include "OgreD3D9TextureManager.h"
#include "OgreD3D9Texture.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreD3D9Mappings.h"

namespace Ogre 
{
	D3D9TextureManager::D3D9TextureManager( LPDIRECT3DDEVICE9 pD3DDevice ) : TextureManager()
	{
		mpD3DDevice = pD3DDevice;
		if( !mpD3DDevice )
			OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid Direct3DDevice passed", "D3D9TextureManager::D3D9TextureManager" );
        // register with group manager
        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
	}

	D3D9TextureManager::~D3D9TextureManager()
	{
        // unregister with group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);

	}

    Resource* D3D9TextureManager::createImpl(const String& name, 
        ResourceHandle handle, const String& group, bool isManual, 
        ManualResourceLoader* loader, const NameValuePairList* createParams)
    {
        return new D3D9Texture(this, name, handle, group, isManual, loader, mpD3DDevice); 
    }

	void D3D9TextureManager::releaseDefaultPoolResources(void)
	{
		size_t count = 0;

		ResourceMap::iterator r, rend;
		rend = mResources.end();
		for (r = mResources.begin(); r != rend; ++r)
		{
			D3D9TexturePtr t = r->second;
			if (t->releaseIfDefaultPool())
				count++;
		}
		LogManager::getSingleton().logMessage("D3D9TextureManager released:");
		LogManager::getSingleton().logMessage(
			StringConverter::toString(count) + " unmanaged textures");
	}

	void D3D9TextureManager::recreateDefaultPoolResources(void)
	{
		size_t count = 0;

		ResourceMap::iterator r, rend;
		rend = mResources.end();
		for (r = mResources.begin(); r != rend; ++r)
		{
			D3D9TexturePtr t = r->second;
			if(t->recreateIfDefaultPool(mpD3DDevice))
				count++;
		}
		LogManager::getSingleton().logMessage("D3D9TextureManager recreated:");
		LogManager::getSingleton().logMessage(
			StringConverter::toString(count) + " unmanaged textures");
	}


	PixelFormat D3D9TextureManager::getNativeFormat(TextureType ttype, PixelFormat format, int usage)
	{
		// Basic filtering
		D3DFORMAT d3dPF = D3D9Mappings::_getPF(D3D9Mappings::_getClosestSupportedPF(format));

		// Calculate usage
		DWORD d3dusage = 0;
		D3DPOOL pool = D3DPOOL_MANAGED;
		if (usage & TU_RENDERTARGET) 
		{
			d3dusage |= D3DUSAGE_RENDERTARGET;
			pool = D3DPOOL_DEFAULT;
		}
		if (usage & TU_DYNAMIC)
		{
			d3dusage |= D3DUSAGE_DYNAMIC;
			pool = D3DPOOL_DEFAULT;
		}

		// Use D3DX to adjust pixel format
		switch(ttype)
		{
		case TEX_TYPE_1D:
		case TEX_TYPE_2D:
			D3DXCheckTextureRequirements(mpD3DDevice, NULL, NULL, NULL, d3dusage, &d3dPF, pool);
			break;
		case TEX_TYPE_3D:
			D3DXCheckVolumeTextureRequirements(mpD3DDevice, NULL, NULL, NULL, NULL, d3dusage, &d3dPF, pool);
			break;
		case TEX_TYPE_CUBE_MAP:
			D3DXCheckCubeTextureRequirements(mpD3DDevice, NULL, NULL, d3dusage, &d3dPF, pool);
			break;
		};

		return D3D9Mappings::_getPF(d3dPF);


	}


}
