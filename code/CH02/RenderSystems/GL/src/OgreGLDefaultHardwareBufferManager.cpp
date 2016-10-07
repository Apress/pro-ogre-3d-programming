/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This program is free software you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
-----------------------------------------------------------------------------
*/
#include "OgreGLDefaultHardwareBufferManager.h"

namespace Ogre {

	GLDefaultHardwareVertexBuffer::GLDefaultHardwareVertexBuffer(size_t vertexSize, size_t numVertices, 
		HardwareBuffer::Usage usage)
        : HardwareVertexBuffer(vertexSize, numVertices, usage, true, false) // always software, never shadowed
	{
		mpData = new unsigned char[mSizeInBytes];
	}
	//-----------------------------------------------------------------------
    GLDefaultHardwareVertexBuffer::~GLDefaultHardwareVertexBuffer()
	{
		delete [] mpData;
	}
	//-----------------------------------------------------------------------
    void* GLDefaultHardwareVertexBuffer::lockImpl(size_t offset, size_t length, LockOptions options)
	{
        // Only for use internally, no 'locking' as such
		return mpData + offset;
	}
	//-----------------------------------------------------------------------
	void GLDefaultHardwareVertexBuffer::unlockImpl(void)
	{
        // Nothing to do
	}
	//-----------------------------------------------------------------------
    void* GLDefaultHardwareVertexBuffer::lock(size_t offset, size_t length, LockOptions options)
	{
        mIsLocked = true;
		return mpData + offset;
	}
	//-----------------------------------------------------------------------
	void GLDefaultHardwareVertexBuffer::unlock(void)
	{
        mIsLocked = false;
        // Nothing to do
	}
	//-----------------------------------------------------------------------
    void GLDefaultHardwareVertexBuffer::readData(size_t offset, size_t length, void* pDest)
	{
		assert((offset + length) <= mSizeInBytes);
		memcpy(pDest, mpData + offset, length);
	}
	//-----------------------------------------------------------------------
    void GLDefaultHardwareVertexBuffer::writeData(size_t offset, size_t length, const void* pSource,
			bool discardWholeBuffer)
	{
		assert((offset + length) <= mSizeInBytes);
		// ignore discard, memory is not guaranteed to be zeroised
		memcpy(mpData + offset, pSource, length);

	}
	//-----------------------------------------------------------------------

	GLDefaultHardwareIndexBuffer::GLDefaultHardwareIndexBuffer(IndexType idxType, 
		size_t numIndexes, HardwareBuffer::Usage usage) 
		: HardwareIndexBuffer(idxType, numIndexes, usage, true, false) // always software, never shadowed
	{
		mpData = new unsigned char[mSizeInBytes];
	}
	//-----------------------------------------------------------------------
    GLDefaultHardwareIndexBuffer::~GLDefaultHardwareIndexBuffer()
	{
		delete [] mpData;
	}
	//-----------------------------------------------------------------------
    void* GLDefaultHardwareIndexBuffer::lockImpl(size_t offset, size_t length, LockOptions options)
	{
        // Only for use internally, no 'locking' as such
		return mpData + offset;
	}
	//-----------------------------------------------------------------------
	void GLDefaultHardwareIndexBuffer::unlockImpl(void)
	{
        // Nothing to do
	}
	//-----------------------------------------------------------------------
    void* GLDefaultHardwareIndexBuffer::lock(size_t offset, size_t length, LockOptions options)
	{
        mIsLocked = true;
		return mpData + offset;
	}
	//-----------------------------------------------------------------------
	void GLDefaultHardwareIndexBuffer::unlock(void)
	{
        mIsLocked = false;
        // Nothing to do
	}
	//-----------------------------------------------------------------------
    void GLDefaultHardwareIndexBuffer::readData(size_t offset, size_t length, void* pDest)
	{
		assert((offset + length) <= mSizeInBytes);
		memcpy(pDest, mpData + offset, length);
	}
	//-----------------------------------------------------------------------
    void GLDefaultHardwareIndexBuffer::writeData(size_t offset, size_t length, const void* pSource,
			bool discardWholeBuffer)
	{
		assert((offset + length) <= mSizeInBytes);
		// ignore discard, memory is not guaranteed to be zeroised
		memcpy(mpData + offset, pSource, length);

	}
	//-----------------------------------------------------------------------
	
	
    //-----------------------------------------------------------------------
    GLDefaultHardwareBufferManager::GLDefaultHardwareBufferManager()
	{
	}
    //-----------------------------------------------------------------------
    GLDefaultHardwareBufferManager::~GLDefaultHardwareBufferManager()
	{
        destroyAllDeclarations();
        destroyAllBindings();
	}
    //-----------------------------------------------------------------------
	HardwareVertexBufferSharedPtr 
        GLDefaultHardwareBufferManager::createVertexBuffer(size_t vertexSize, 
		size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer)
	{
		return HardwareVertexBufferSharedPtr(
			new GLDefaultHardwareVertexBuffer(vertexSize, numVerts, usage));
	}
    //-----------------------------------------------------------------------
	HardwareIndexBufferSharedPtr 
        GLDefaultHardwareBufferManager::createIndexBuffer(HardwareIndexBuffer::IndexType itype, 
		size_t numIndexes, HardwareBuffer::Usage usage, bool useShadowBuffer)
	{
		return HardwareIndexBufferSharedPtr(
			new GLDefaultHardwareIndexBuffer(itype, numIndexes, usage) );
	}
}
