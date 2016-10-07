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
#include "OgreStableHeaders.h"
#include "OgreHardwareIndexBuffer.h"
#include "OgreHardwareBufferManager.h"
#include "OgreDefaultHardwareBufferManager.h"


namespace Ogre {

    //-----------------------------------------------------------------------------
    HardwareIndexBuffer::HardwareIndexBuffer(IndexType idxType, 
        size_t numIndexes, HardwareBuffer::Usage usage, 
        bool useSystemMemory, bool useShadowBuffer) 
        : HardwareBuffer(usage, useSystemMemory, useShadowBuffer), mIndexType(idxType), mNumIndexes(numIndexes)
    {
        // Calculate the size of the indexes
        switch (mIndexType)
        {
        case IT_16BIT:
            mIndexSize = sizeof(unsigned short);
            break;
        case IT_32BIT:
            mIndexSize = sizeof(unsigned int);
            break;
        }
        mSizeInBytes = mIndexSize * mNumIndexes;

        // Create a shadow buffer if required
        if (mUseShadowBuffer)
        {
            mpShadowBuffer = new DefaultHardwareIndexBuffer(mIndexType, 
                mNumIndexes, HardwareBuffer::HBU_DYNAMIC);
        }


    }
    //-----------------------------------------------------------------------------
    HardwareIndexBuffer::~HardwareIndexBuffer()
    {
		HardwareBufferManager* mgr = HardwareBufferManager::getSingletonPtr();
		if (mgr)
		{
			mgr->_notifyIndexBufferDestroyed(this);
		}

        if (mpShadowBuffer)
        {
            delete mpShadowBuffer;
        }
    }
    //-----------------------------------------------------------------------------
    HardwareIndexBufferSharedPtr::HardwareIndexBufferSharedPtr(HardwareIndexBuffer* buf)
        : SharedPtr<HardwareIndexBuffer>(buf)
    {

    }

}

