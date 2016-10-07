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
#include "OgreLogManager.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreImage.h"
#include "OgreTexture.h"
#include "OgreException.h"
#include "OgreResourceManager.h"

namespace Ogre {
	//--------------------------------------------------------------------------
    Texture::Texture(ResourceManager* creator, const String& name, 
        ResourceHandle handle, const String& group, bool isManual, 
        ManualResourceLoader* loader)
        : Resource(creator, name, handle, group, isManual, loader),
            // init defaults; can be overridden before load()
            mHeight(512),
            mWidth(512),
            mDepth(1),
			mNumRequestedMipmaps(0),
            mNumMipmaps(0),
			mMipmapsHardwareGenerated(false),
            mGamma(1.0f),
            mTextureType(TEX_TYPE_2D),            
            mFormat(PF_A8R8G8B8),
            mUsage(TU_DEFAULT),
            // mSrcBpp inited later on
            mSrcWidth(0),
            mSrcHeight(0), 
            mSrcDepth(0),
            // mFinalBpp inited later on by enable32bit
            mHasAlpha(false),
            mInternalResourcesCreated(false)
    {

        enable32Bit(false);

        if (createParamDictionary("Texture"))
        {
            // Define the parameters that have to be present to load
            // from a generic source; actually there are none, since when
            // predeclaring, you use a texture file which includes all the
            // information required.
        }

        
    }
	//--------------------------------------------------------------------------    //--------------------------------------------------------------------------
	void Texture::loadRawData( DataStreamPtr& stream, 
		ushort uWidth, ushort uHeight, PixelFormat eFormat)
	{
		Image img;
		img.loadRawData(stream, uWidth, uHeight, eFormat);
		loadImage(img);
	}
    //--------------------------------------------------------------------------
    void Texture::setFormat(PixelFormat pf)
    {
        mFormat = pf;
        // This should probably change with new texture access methods, but
        // no changes made for now
        mSrcBpp = PixelUtil::getNumElemBytes(mFormat);
        mHasAlpha = PixelUtil::getFlags(mFormat) & PFF_HASALPHA;
    }
    //--------------------------------------------------------------------------
	size_t Texture::calculateSize(void) const
	{
        return getNumFaces() * PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);
	}
	//--------------------------------------------------------------------------
	size_t Texture::getNumFaces(void) const
	{
		return getTextureType() == TEX_TYPE_CUBE_MAP ? 6 : 1;
	}
	//--------------------------------------------------------------------------
    void Texture::_loadImages( const std::vector<const Image*>& images )
    {
		if(images.size() < 1)
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot load empty vector of images",
			 "Texture::loadImages");
        
        if( mIsLoaded )
        {
			LogManager::getSingleton().logMessage( 
				LML_NORMAL, "Texture: "+mName+": Unloading Image");
            unload();
        }

		// Set desired texture size and properties from images[0]
		mSrcWidth = mWidth = images[0]->getWidth();
		mSrcHeight = mHeight = images[0]->getHeight();
		mSrcDepth = mDepth = images[0]->getDepth();
        if (mHasAlpha && images[0]->getFormat() == PF_L8)
        {
            mFormat = PF_A8;
            mSrcBpp = 8;
        }
        else
        {
            mFormat = images[0]->getFormat();
            mSrcBpp = PixelUtil::getNumElemBits(mFormat);
            mHasAlpha = PixelUtil::hasAlpha(mFormat);
        }

		if (mFinalBpp == 16) 
		{
			// Drop down texture internal format
			switch (mFormat) 
			{
			case PF_R8G8B8:
			case PF_X8R8G8B8:
				mFormat = PF_R5G6B5;
				break;
			
			case PF_B8G8R8:
			case PF_X8B8G8R8:
				mFormat = PF_B5G6R5;
				break;
				
			case PF_A8R8G8B8:
			case PF_R8G8B8A8:
    			case PF_A8B8G8R8:
			case PF_B8G8R8A8:
				mFormat = PF_A4R4G4B4;
				break;

			default:
				break; // use original image format
			}
		}
		
		// The custom mipmaps in the image have priority over everything
        size_t imageMips = images[0]->getNumMipmaps();

		if(imageMips > 0) {
			mNumMipmaps = images[0]->getNumMipmaps();
			// Disable flag for auto mip generation
			mUsage &= ~TU_AUTOMIPMAP;
		}
		
        // Create the texture
        createInternalResources();
		// Check if we're loading one image with multiple faces
		// or a vector of images representing the faces
		size_t faces;
		bool multiImage; // Load from multiple images?
		if(images.size() > 1)
		{
			faces = images.size();
			multiImage = true;
		}
		else
		{
			faces = images[0]->getNumFaces();
			multiImage = false;
		}
		
		// Check wether number of faces in images exceeds number of faces
		// in this texture. If so, clamp it.
		if(faces > getNumFaces())
			faces = getNumFaces();
		
		// Say what we're doing
		StringUtil::StrStreamType str;
		str << "Texture: " << mName << ": Loading " << faces << " faces"
			<< "(" << PixelUtil::getFormatName(images[0]->getFormat()) << "," <<
			images[0]->getWidth() << "x" << images[0]->getHeight() << "x" << images[0]->getDepth() <<
			") with ";
		if (!(mMipmapsHardwareGenerated && mNumMipmaps == 0))
			str << mNumMipmaps;
		if(mUsage & TU_AUTOMIPMAP)
		{
			if (mMipmapsHardwareGenerated)
				str << " hardware";

			str << " generated mipmaps";
		}
		else
		{
			str << " custom mipmaps";
		}
 		if(multiImage)
			str << " from multiple Images.";
		else
			str << " from Image.";
		// Scoped
		{
			// Print data about first destination surface
			HardwarePixelBufferSharedPtr buf = getBuffer(0, 0); 
			str << " Internal format is " << PixelUtil::getFormatName(buf->getFormat()) << 
			"," << buf->getWidth() << "x" << buf->getHeight() << "x" << buf->getDepth() << ".";
		}
		LogManager::getSingleton().logMessage( 
				LML_NORMAL, str.str());
		
		// Main loading loop
        // imageMips == 0 if the image has no custom mipmaps, otherwise contains the number of custom mips
        for(size_t mip = 0; mip<=imageMips; ++mip)
        {
            for(size_t i = 0; i < faces; ++i)
            {
                PixelBox src;
                if(multiImage)
                {
                    // Load from multiple images
                    src = images[i]->getPixelBox(0, mip);
                }
                else
                {
                    // Load from faces of images[0]
                    src = images[0]->getPixelBox(i, mip);

                    if (mHasAlpha && src.format == PF_L8)
                        src.format = PF_A8;
                }
    
                if(mGamma != 1.0f) {
                    // Apply gamma correction
                    // Do not overwrite original image but do gamma correction in temporary buffer
                    MemoryDataStreamPtr buf; // for scoped deletion of conversion buffer
                    buf.bind(new MemoryDataStream(
                        PixelUtil::getMemorySize(
                            src.getWidth(), src.getHeight(), src.getDepth(), src.format)));
                    
                    PixelBox corrected = PixelBox(src.getWidth(), src.getHeight(), src.getDepth(), src.format, buf->getPtr());
                    PixelUtil::bulkPixelConversion(src, corrected);
                    
                    Image::applyGamma(static_cast<uint8*>(corrected.data), mGamma, corrected.getConsecutiveSize(), 
                        PixelUtil::getNumElemBits(src.format));
    
                    // Destination: entire texture. blitFromMemory does the scaling to
                    // a power of two for us when needed
                    getBuffer(i, mip)->blitFromMemory(corrected);
                }
                else 
                {
                    // Destination: entire texture. blitFromMemory does the scaling to
                    // a power of two for us when needed
                    getBuffer(i, mip)->blitFromMemory(src);
                }
                
            }
        }
        // Update size (the final size, not including temp space)
        mSize = getNumFaces() * PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);

        mIsLoaded = true;
    }
	//-----------------------------------------------------------------------------
	void Texture::createInternalResources(void)
	{
		if (!mInternalResourcesCreated)
		{
			createInternalResourcesImpl();
			mInternalResourcesCreated = true;
		}
	}
	//-----------------------------------------------------------------------------
	void Texture::freeInternalResources(void)
	{
		if (mInternalResourcesCreated)
		{
			freeInternalResourcesImpl();
			mInternalResourcesCreated = false;
		}
	}
	//-----------------------------------------------------------------------------
	void Texture::unloadImpl(void)
	{
		freeInternalResources();
	}
    //-----------------------------------------------------------------------------   
    void Texture::copyToTexture( TexturePtr& target )
    {
        if(target->getNumFaces() != getNumFaces())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                "Texture types must match",
                "Texture::copyToTexture");
        }
        size_t numMips = std::min(getNumMipmaps(), target->getNumMipmaps());
        if((mUsage & TU_AUTOMIPMAP) || (target->getUsage()&TU_AUTOMIPMAP))
            numMips = 0;
        for(unsigned int face=0; face<getNumFaces(); face++)
        {
            for(unsigned int mip=0; mip<=numMips; mip++)
            {
                target->getBuffer(face, mip)->blit(getBuffer(face, mip));
            }
        }
    }


}
