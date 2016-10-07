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
#ifndef __D3D9HARWAREBUFFERMANAGER_H__
#define __D3D9HARWAREBUFFERMANAGER_H__

#include "OgreD3D9Prerequisites.h"
#include "OgreHardwareBufferManager.h"

namespace Ogre {

    /** Implementation of HardwareBufferManager for D3D9. */
    class D3D9HardwareBufferManager : public HardwareBufferManager
    {
    protected:
        LPDIRECT3DDEVICE9 mlpD3DDevice;

        /// Internal method for creates a new vertex declaration, may be overridden by certain rendering APIs
        VertexDeclaration* createVertexDeclarationImpl(void);
        /// Internal method for destroys a vertex declaration, may be overridden by certain rendering APIs
        void destroyVertexDeclarationImpl(VertexDeclaration* decl);

    public:
        D3D9HardwareBufferManager(LPDIRECT3DDEVICE9 device);
        ~D3D9HardwareBufferManager();
        /// Creates a vertex buffer
		HardwareVertexBufferSharedPtr 
            createVertexBuffer(size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer = false);
		/// Create a hardware vertex buffer
		HardwareIndexBufferSharedPtr 
            createIndexBuffer(HardwareIndexBuffer::IndexType itype, size_t numIndexes, HardwareBuffer::Usage usage, bool useShadowBuffer = false);

		/** Release all buffers in the default memory pool. 
		@remarks
			Method for dealing with lost devices.
		*/
		void releaseDefaultPoolResources(void);
		/** Recreate all buffers in the default memory pool. 
		@remarks
			Method for dealing with lost devices.
		*/
		void recreateDefaultPoolResources(void);


    };

}


#endif
