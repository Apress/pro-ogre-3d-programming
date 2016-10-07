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
#include "OgreHardwareVertexBuffer.h"
#include "OgreColourValue.h"
#include "OgreException.h"
#include "OgreStringConverter.h"
#include "OgreHardwareBufferManager.h"
#include "OgreDefaultHardwareBufferManager.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"

namespace Ogre {

    //-----------------------------------------------------------------------------
    HardwareVertexBuffer::HardwareVertexBuffer(size_t vertexSize,  
        size_t numVertices, HardwareBuffer::Usage usage, 
        bool useSystemMemory, bool useShadowBuffer) 
        : HardwareBuffer(usage, useSystemMemory, useShadowBuffer), 
          mNumVertices(numVertices),
          mVertexSize(vertexSize)
    {
        // Calculate the size of the vertices
        mSizeInBytes = mVertexSize * numVertices;

        // Create a shadow buffer if required
        if (mUseShadowBuffer)
        {
            mpShadowBuffer = new DefaultHardwareVertexBuffer(mVertexSize, 
                    mNumVertices, HardwareBuffer::HBU_DYNAMIC);
        }

    }
    //-----------------------------------------------------------------------------
    HardwareVertexBuffer::~HardwareVertexBuffer()
    {
		HardwareBufferManager* mgr = HardwareBufferManager::getSingletonPtr();
		if (mgr)
		{
			mgr->_notifyVertexBufferDestroyed(this);
		}
        if (mpShadowBuffer)
        {
            delete mpShadowBuffer;
        }
    }
    //-----------------------------------------------------------------------------
    VertexElement::VertexElement(unsigned short source, size_t offset, 
        VertexElementType theType, VertexElementSemantic semantic, unsigned short index)
        : mSource(source), mOffset(offset), mType(theType), 
        mSemantic(semantic), mIndex(index)
    {
    }
    //-----------------------------------------------------------------------------
	size_t VertexElement::getSize(void) const
	{
		return getTypeSize(mType);
	}
	//-----------------------------------------------------------------------------
	size_t VertexElement::getTypeSize(VertexElementType etype)
	{
		switch(etype)
		{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return sizeof(RGBA);
		case VET_FLOAT1:
			return sizeof(float);
		case VET_FLOAT2:
			return sizeof(float)*2;
		case VET_FLOAT3:
			return sizeof(float)*3;
		case VET_FLOAT4:
			return sizeof(float)*4;
		case VET_SHORT1:
			return sizeof(short);
		case VET_SHORT2:
			return sizeof(short)*2;
		case VET_SHORT3:
			return sizeof(short)*3;
		case VET_SHORT4:
			return sizeof(short)*4;
        case VET_UBYTE4:
            return sizeof(unsigned char)*4;
		}
		return 0;
	}
	//-----------------------------------------------------------------------------
	unsigned short VertexElement::getTypeCount(VertexElementType etype)
	{
		switch (etype)
		{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return 1;
		case VET_FLOAT1:
			return 1;
		case VET_FLOAT2:
			return 2;
		case VET_FLOAT3:
			return 3;
		case VET_FLOAT4:
			return 4;
		case VET_SHORT1:
			return 1;
		case VET_SHORT2:
			return 2;
		case VET_SHORT3:
			return 3;
		case VET_SHORT4:
			return 4;
        case VET_UBYTE4:
            return 4;
		}
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Invalid type", 
			"VertexElement::getTypeCount");
	}
	//-----------------------------------------------------------------------------
	VertexElementType VertexElement::multiplyTypeCount(VertexElementType baseType, 
		unsigned short count)
	{
		switch (baseType)
		{
		case VET_FLOAT1:
			switch(count)
			{
			case 1:
				return VET_FLOAT1;
			case 2:
				return VET_FLOAT2;
			case 3:
				return VET_FLOAT3;
			case 4:
				return VET_FLOAT4;
            default:
                break;
			}
			break;
		case VET_SHORT1:
			switch(count)
			{
			case 1:
				return VET_SHORT1;
			case 2:
				return VET_SHORT2;
			case 3:
				return VET_SHORT3;
			case 4:
				return VET_SHORT4;
            default:
                break;
			}
			break;
        default:
            break;
		}
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Invalid base type", 
			"VertexElement::multiplyTypeCount");
	}
	//--------------------------------------------------------------------------
	VertexElementType VertexElement::getBestColourVertexElementType(void)
	{
		// Use the current render system to determine if possible
		if (Root::getSingletonPtr() && Root::getSingletonPtr()->getRenderSystem())
		{
			return Root::getSingleton().getRenderSystem()->getColourVertexElementType();
		}
		else
		{
			// We can't know the specific type right now, so pick a type
			// based on platform
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
			return VET_COLOUR_ARGB; // prefer D3D format on windows
#else
			return VET_COLOUR_ABGR; // prefer GL format on everything else
#endif

		}
	}
	//--------------------------------------------------------------------------
	void VertexElement::convertColourValue(VertexElementType srcType, 
		VertexElementType dstType, uint32* ptr)
	{
		if (srcType == dstType)
			return;

		// Conversion between ARGB and ABGR is always a case of flipping R/B
		*ptr = 
		   ((*ptr&0x00FF0000)>>16)|((*ptr&0x000000FF)<<16)|(*ptr&0xFF00FF00);				
	}
	//--------------------------------------------------------------------------
	uint32 VertexElement::convertColourValue(const ColourValue& src, 
		VertexElementType dst)
	{
		switch(dst)
		{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        default:
#endif
		case VET_COLOUR_ARGB:
			return src.getAsARGB();
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
        default:
#endif
		case VET_COLOUR_ABGR: 
			return src.getAsABGR();
		};

	}
	//-----------------------------------------------------------------------------
	VertexElementType VertexElement::getBaseType(VertexElementType multiType)
	{
		switch (multiType)
		{
			case VET_FLOAT1:
			case VET_FLOAT2:
			case VET_FLOAT3:
			case VET_FLOAT4:
				return VET_FLOAT1;
			case VET_COLOUR:
				return VET_COLOUR;
			case VET_COLOUR_ABGR:
				return VET_COLOUR_ABGR;
			case VET_COLOUR_ARGB:
				return VET_COLOUR_ARGB;
			case VET_SHORT1:
			case VET_SHORT2:
			case VET_SHORT3:
			case VET_SHORT4:
				return VET_SHORT1;
			case VET_UBYTE4:
				return VET_UBYTE4;
		};
        // To keep compiler happy
        return VET_FLOAT1;
	}
	//-----------------------------------------------------------------------------
    VertexDeclaration::VertexDeclaration()
    {
    }
    //-----------------------------------------------------------------------------
    VertexDeclaration::~VertexDeclaration()
    {
    }
    //-----------------------------------------------------------------------------
    const VertexDeclaration::VertexElementList& VertexDeclaration::getElements(void) const
    {
        return mElementList;
    }
    //-----------------------------------------------------------------------------
    const VertexElement& VertexDeclaration::addElement(unsigned short source, 
        size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index)
    {
		// Refine colour type to a specific type
		if (theType == VET_COLOUR)
		{
			theType = VertexElement::getBestColourVertexElementType();
		}
        mElementList.push_back(
            VertexElement(source, offset, theType, semantic, index)
            );
		return mElementList.back();
    }
    //-----------------------------------------------------------------------------
    const VertexElement& VertexDeclaration::insertElement(unsigned short atPosition,
        unsigned short source, size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index)
    {
        if (atPosition >= mElementList.size())
        {
            return addElement(source, offset, theType, semantic, index);
        }

        VertexElementList::iterator i = mElementList.begin();
        for (unsigned short n = 0; n < atPosition; ++n)
            ++i;

        i = mElementList.insert(i, 
            VertexElement(source, offset, theType, semantic, index));
        return *i;

    }
    //-----------------------------------------------------------------------------
    const VertexElement* VertexDeclaration::getElement(unsigned short index)
    {
        assert(index < mElementList.size() && "Index out of bounds");

        VertexElementList::iterator i = mElementList.begin();
        for (unsigned short n = 0; n < index; ++n)
            ++i;

        return &(*i);

    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::removeElement(unsigned short elem_index)
    {
        assert(elem_index < mElementList.size() && "Index out of bounds");
        VertexElementList::iterator i = mElementList.begin();
        for (unsigned short n = 0; n < elem_index; ++n)
            ++i;
        mElementList.erase(i);
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::removeElement(VertexElementSemantic semantic, unsigned short index)
    {
		VertexElementList::iterator ei, eiend;
		eiend = mElementList.end();
		for (ei = mElementList.begin(); ei != eiend; ++ei)
		{
			if (ei->getSemantic() == semantic && ei->getIndex() == index)
			{
				mElementList.erase(ei);
                break;
			}
		}
    }
	//-----------------------------------------------------------------------------
	void VertexDeclaration::removeAllElements(void)
	{
		mElementList.clear();
	}
    //-----------------------------------------------------------------------------
    void VertexDeclaration::modifyElement(unsigned short elem_index, 
        unsigned short source, size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index)
    {
        assert(elem_index < mElementList.size() && "Index out of bounds");
        VertexElementList::iterator i = mElementList.begin();
        std::advance(i, elem_index);
        (*i) = VertexElement(source, offset, theType, semantic, index);
    }
    //-----------------------------------------------------------------------------
	const VertexElement* VertexDeclaration::findElementBySemantic(
		VertexElementSemantic sem, unsigned short index)
	{
		VertexElementList::const_iterator ei, eiend;
		eiend = mElementList.end();
		for (ei = mElementList.begin(); ei != eiend; ++ei)
		{
			if (ei->getSemantic() == sem && ei->getIndex() == index)
			{
				return &(*ei);
			}
		}

		return NULL;


	}
	//-----------------------------------------------------------------------------
	VertexDeclaration::VertexElementList VertexDeclaration::findElementsBySource(
		unsigned short source)
	{
		VertexElementList retList;
		VertexElementList::const_iterator ei, eiend;
		eiend = mElementList.end();
		for (ei = mElementList.begin(); ei != eiend; ++ei)
		{
			if (ei->getSource() == source)
			{
				retList.push_back(*ei);
			}
		}
		return retList;

	}

	//-----------------------------------------------------------------------------
	size_t VertexDeclaration::getVertexSize(unsigned short source)
	{
		VertexElementList::const_iterator i, iend;
		iend = mElementList.end();
		size_t sz = 0;

		for (i = mElementList.begin(); i != iend; ++i)
		{
			if (i->getSource() == source)
			{
				sz += i->getSize();

			}
		}
		return sz;
	}
    //-----------------------------------------------------------------------------
    VertexDeclaration* VertexDeclaration::clone(void)
    {
        VertexDeclaration* ret = HardwareBufferManager::getSingleton().createVertexDeclaration();

		VertexElementList::const_iterator i, iend;
		iend = mElementList.end();
		for (i = mElementList.begin(); i != iend; ++i)
		{
            ret->addElement(i->getSource(), i->getOffset(), i->getType(), i->getSemantic(), i->getIndex());
        }
        return ret;
    }
    //-----------------------------------------------------------------------------
    // Sort routine for VertexElement
    bool VertexDeclaration::vertexElementLess(const VertexElement& e1, const VertexElement& e2)
    {
        // Sort by source first
        if (e1.getSource() < e2.getSource())
        {
            return true;
        }
        else if (e1.getSource() == e2.getSource())
        {
            // Use ordering of semantics to sort
            if (e1.getSemantic() < e2.getSemantic())
            {
                return true;
            }
            else if (e1.getSemantic() == e2.getSemantic())
            {
                // Use index to sort
                if (e1.getIndex() < e2.getIndex())
                {
                    return true;
                }
            }
        }
        return false;
    }
    void VertexDeclaration::sort(void)
    {
        mElementList.sort(VertexDeclaration::vertexElementLess);
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::closeGapsInSource(void)
    {
        if (mElementList.empty())
            return;

        // Sort first
        sort();

        VertexElementList::iterator i, iend;
        iend = mElementList.end();
        unsigned short targetIdx = 0;
        unsigned short lastIdx = getElement(0)->getSource();
        unsigned short c = 0;
        for (i = mElementList.begin(); i != iend; ++i, ++c)
        {
            VertexElement& elem = *i;
            if (lastIdx != elem.getSource())
            {
                targetIdx++;
                lastIdx = elem.getSource();
            }
            if (targetIdx != elem.getSource())
            {
                modifyElement(c, targetIdx, elem.getOffset(), elem.getType(), 
                    elem.getSemantic(), elem.getIndex());
            }

        }

    }
    //-----------------------------------------------------------------------
    VertexDeclaration* VertexDeclaration::getAutoOrganisedDeclaration(
		bool skeletalAnimation, bool vertexAnimation)
    {
        VertexDeclaration* newDecl = this->clone();
        // Set all sources to the same buffer (for now)
        const VertexDeclaration::VertexElementList& elems = newDecl->getElements();
        VertexDeclaration::VertexElementList::const_iterator i;
        unsigned short c = 0;
        for (i = elems.begin(); i != elems.end(); ++i, ++c)
        {
            const VertexElement& elem = *i;
            // Set source & offset to 0 for now, before sort
            newDecl->modifyElement(c, 0, 0, elem.getType(), elem.getSemantic(), elem.getIndex());
        }
        newDecl->sort();
        // Now sort out proper buffer assignments and offsets
        size_t offset = 0;
        c = 0;
		unsigned short buffer = 0;
        for (i = elems.begin(); i != elems.end(); ++i, ++c)
        {
            const VertexElement& elem = *i;
			if (vertexAnimation && elem.getSemantic() == VES_NORMAL)
			{
				// For morph animation, we need positions on their own
				++buffer;
				offset = 0;
			}
            if ((skeletalAnimation || vertexAnimation) &&
                elem.getSemantic() != VES_POSITION && elem.getSemantic() != VES_NORMAL)
            {
				// All animated meshes have to split after normal
				++buffer;
				offset = 0;
            }
			newDecl->modifyElement(c, buffer, offset, 
				elem.getType(), elem.getSemantic(), elem.getIndex());
			offset += elem.getSize();
        }

        return newDecl;


    }
    //-----------------------------------------------------------------------------
    unsigned short VertexDeclaration::getMaxSource(void) const
    {
        VertexElementList::const_iterator i, iend;
        iend = mElementList.end();
        unsigned short ret = 0;
        for (i = mElementList.begin(); i != iend; ++i)
        {
            if (i->getSource() > ret)
            {
                ret = i->getSource();
            }

        }
        return ret;
    }
    //-----------------------------------------------------------------------------
	VertexBufferBinding::VertexBufferBinding() : mHighIndex(0)
	{
	}
    //-----------------------------------------------------------------------------
	VertexBufferBinding::~VertexBufferBinding()
	{
        unsetAllBindings();
	}
    //-----------------------------------------------------------------------------
	void VertexBufferBinding::setBinding(unsigned short index, HardwareVertexBufferSharedPtr buffer)
	{
        // NB will replace any existing buffer ptr at this index, and will thus cause
        // reference count to decrement on that buffer (possibly destroying it)
		mBindingMap[index] = buffer;
		mHighIndex = std::max(mHighIndex, (unsigned short)(index+1));
	}
    //-----------------------------------------------------------------------------
	void VertexBufferBinding::unsetBinding(unsigned short index)
	{
		VertexBufferBindingMap::iterator i = mBindingMap.find(index);
		if (i == mBindingMap.end())
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
				"Cannot find buffer binding for index " + StringConverter::toString(index),
				"VertexBufferBinding::unsetBinding");
		}
		mBindingMap.erase(i);
	}
    //-----------------------------------------------------------------------------
    void VertexBufferBinding::unsetAllBindings(void)
    {
        mBindingMap.clear();
    }
    //-----------------------------------------------------------------------------
	const VertexBufferBinding::VertexBufferBindingMap& 
	VertexBufferBinding::getBindings(void) const
	{
		return mBindingMap;
	}
    //-----------------------------------------------------------------------------
	HardwareVertexBufferSharedPtr VertexBufferBinding::getBuffer(unsigned short index)
	{
		VertexBufferBindingMap::iterator i = mBindingMap.find(index);
		if (i == mBindingMap.end())
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "No buffer is bound to that index.",
				"VertexBufferBinding::getBuffer");
		}
		return i->second;
	}
	//-----------------------------------------------------------------------------
	bool VertexBufferBinding::isBufferBound(unsigned short index)
	{
		return mBindingMap.find(index) != mBindingMap.end();
	}
    //-----------------------------------------------------------------------------
    HardwareVertexBufferSharedPtr::HardwareVertexBufferSharedPtr(HardwareVertexBuffer* buf)
        : SharedPtr<HardwareVertexBuffer>(buf)
    {

    }




}
