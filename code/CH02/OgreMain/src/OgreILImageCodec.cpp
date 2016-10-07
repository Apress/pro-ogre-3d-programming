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

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreILImageCodec.h"
#include "OgreImage.h"
#include "OgreException.h"
#include "OgreILUtil.h"

#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include <IL/il.h>
#include <IL/ilu.h>

namespace Ogre {

    bool ILImageCodec::_is_initialised = false;    
    //---------------------------------------------------------------------

    ILImageCodec::ILImageCodec(const String &type, unsigned int ilType):
        mType(type),
        mIlType(ilType)
    { 
        initialiseIL();
    }

    //---------------------------------------------------------------------
    DataStreamPtr ILImageCodec::code(MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData) const
    {        
        OgreGuard( "ILCodec::code" );

        OGRE_EXCEPT(Exception::UNIMPLEMENTED_FEATURE, "code to memory not implemented",
            "ILCodec::code");

        OgreUnguard();

    }
    //---------------------------------------------------------------------
    void ILImageCodec::codeToFile(MemoryDataStreamPtr& input, 
        const String& outFileName, Codec::CodecDataPtr& pData) const
    {
        OgreGuard( "ILImageCodec::codeToFile" );

        ILuint ImageName;

        ilGenImages( 1, &ImageName );
        ilBindImage( ImageName );

		ImageData* pImgData = static_cast< ImageData * >( pData.getPointer() );
		PixelBox src(pImgData->width, pImgData->height, pImgData->depth, pImgData->format, input->getPtr());

		// Convert image from OGRE to current IL image
		ILUtil::fromOgre(src);

        iluFlipImage();

        // Implicitly pick DevIL codec
        ilSaveImage(const_cast< char * >( outFileName.c_str() ) );
	
        // Check if everything was ok
        ILenum PossibleError = ilGetError() ;
        if( PossibleError != IL_NO_ERROR ) {
           ilDeleteImages(1, &ImageName);
           OGRE_EXCEPT( Exception::UNIMPLEMENTED_FEATURE,
                "IL Error, could not save file: " + outFileName,
                iluErrorString(PossibleError) ) ;
        }

        ilDeleteImages(1, &ImageName);

        OgreUnguard();
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult ILImageCodec::decode(DataStreamPtr& input) const
    {
        OgreGuard( "ILImageCodec::decode" );

        // DevIL variables
        ILuint ImageName;

        ILint ImageFormat, BytesPerPixel, ImageType;
        ImageData* imgData = new ImageData();
        MemoryDataStreamPtr output;

        // Load the image
        ilGenImages( 1, &ImageName );
        ilBindImage( ImageName );

        // Put it right side up
        ilEnable(IL_ORIGIN_SET);
        ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_UPPER_LEFT);

        // Keep DXTC(compressed) data if present
        ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);

        // Load image from stream, cache into memory
        MemoryDataStream memInput(input);
        ilLoadL( 
            mIlType, 
            memInput.getPtr(), 
            static_cast< ILuint >(memInput.size()));

        // Check if everything was ok
        ILenum PossibleError = ilGetError() ;
        if( PossibleError != IL_NO_ERROR ) {
            OGRE_EXCEPT( Exception::UNIMPLEMENTED_FEATURE,
                "IL Error",
                iluErrorString(PossibleError) ) ;
        }

        ImageFormat = ilGetInteger( IL_IMAGE_FORMAT );
        ImageType = ilGetInteger( IL_IMAGE_TYPE );

        // Convert image if ImageType is incompatible with us (double or long)
        if(ImageType != IL_BYTE && ImageType != IL_UNSIGNED_BYTE && 
			ImageType != IL_FLOAT &&
			ImageType != IL_UNSIGNED_SHORT && ImageType != IL_SHORT) {
            ilConvertImage(ImageFormat, IL_FLOAT);
			ImageType = IL_FLOAT;
        }
		// Converted paletted images
		if(ImageFormat == IL_COLOUR_INDEX)
		{
			ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
			ImageFormat = IL_BGRA;
			ImageType = IL_UNSIGNED_BYTE;
		}

        // Now sets some variables
        BytesPerPixel = ilGetInteger( IL_IMAGE_BYTES_PER_PIXEL ); 

        imgData->format = ILUtil::ilFormat2OgreFormat( ImageFormat, ImageType );
        imgData->width = ilGetInteger( IL_IMAGE_WIDTH );
        imgData->height = ilGetInteger( IL_IMAGE_HEIGHT );
        imgData->depth = ilGetInteger( IL_IMAGE_DEPTH );
        imgData->num_mipmaps = ilGetInteger ( IL_NUM_MIPMAPS );
        imgData->flags = 0;
		
		if(imgData->format == PF_UNKNOWN)
		{
			std::stringstream err;
			err << "Unsupported devil format ImageFormat=" << std::hex << ImageFormat << 
				" ImageType="<< ImageType << std::dec;
			ilDeleteImages( 1, &ImageName );
			
			OGRE_EXCEPT( Exception::UNIMPLEMENTED_FEATURE,
                err.str(),
                "ILImageCodec::decode" ) ;
		}

        // Check for cubemap
        //ILuint cubeflags = ilGetInteger ( IL_IMAGE_CUBEFLAGS );
		size_t numFaces = ilGetInteger ( IL_NUM_IMAGES ) + 1;
        if(numFaces == 6) 
			imgData->flags |= IF_CUBEMAP;
        else
            numFaces = 1; // Support only 1 or 6 face images for now
  
        // Keep DXT data (if present at all and the GPU supports it)
        ILuint dxtFormat = ilGetInteger( IL_DXTC_DATA_FORMAT );
        if(dxtFormat != IL_DXT_NO_COMP && Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability( RSC_TEXTURE_COMPRESSION_DXT ))
        {
			imgData->format = ILUtil::ilFormat2OgreFormat( dxtFormat, ImageType );
            imgData->flags |= IF_COMPRESSED;
            
            // Validate that this devil version saves DXT mipmaps
            if(imgData->num_mipmaps>0)
            {
                ilBindImage(ImageName);
                ilActiveMipmap(1);
                if((size_t)ilGetInteger( IL_DXTC_DATA_FORMAT ) != dxtFormat)
                {
                    imgData->num_mipmaps=0;
                    LogManager::getSingleton().logMessage(
                    "Warning: Custom mipmaps for compressed image "+input->getName()+" were ignored because they are not loaded by this DevIL version");
                }
            }
        }
        
        // Calculate total size from number of mipmaps, faces and size
        imgData->size = Image::calculateSize(imgData->num_mipmaps, numFaces, 
            imgData->width, imgData->height, imgData->depth, imgData->format);

        // Bind output buffer
        output.bind(new MemoryDataStream(imgData->size));
        size_t offset = 0;
        
        // Dimensions of current mipmap
        size_t width = imgData->width;
        size_t height = imgData->height;
        size_t depth = imgData->depth;
        
        // Transfer data
        for(size_t mip=0; mip<=imgData->num_mipmaps; ++mip)
        {   
            for(size_t i = 0; i < numFaces; ++i)
            {
                ilBindImage(ImageName);
                if(numFaces > 1)
                    ilActiveImage(i);
                if(imgData->num_mipmaps > 0)
                    ilActiveMipmap(mip);
                /// Size of this face
                size_t imageSize = PixelUtil::getMemorySize(
                        width, height, depth, imgData->format);
                if(imgData->flags & IF_COMPRESSED)
                {

                    // Compare DXT size returned by DevIL with our idea of the compressed size
                    if(imageSize == ilGetDXTCData(NULL, 0, dxtFormat))
                    {
                        // Retrieve data from DevIL
                        ilGetDXTCData((unsigned char*)output->getPtr()+offset, imageSize, dxtFormat);
                    } else
                    {
                        LogManager::getSingleton().logMessage(
                            "Warning: compressed image "+input->getName()+" size mismatch, devilsize="+StringConverter::toString(ilGetDXTCData(NULL, 0, dxtFormat))+" oursize="+
                            StringConverter::toString(imageSize));
                    }
                }
                else
                {
                    /// Retrieve data from DevIL
                    PixelBox dst(width, height, depth, imgData->format, (unsigned char*)output->getPtr()+offset);
                    ILUtil::toOgre(dst);
                }
                offset += imageSize;
            }
            /// Next mip
            if(width!=1) width /= 2;
            if(height!=1) height /= 2;
            if(depth!=1) depth /= 2;
        }

        // Restore IL state
        ilDisable(IL_ORIGIN_SET);
        ilDisable(IL_FORMAT_SET);

        ilDeleteImages( 1, &ImageName );

        DecodeResult ret;
        ret.first = output;
        ret.second = CodecDataPtr(imgData);


        OgreUnguardRet( ret );
    }
    //---------------------------------------------------------------------
    void ILImageCodec::initialiseIL(void)
    {
        if( !_is_initialised )
        {
            ilInit();
            ilEnable( IL_FILE_OVERWRITE );
            _is_initialised = true;
        }
    }
    //---------------------------------------------------------------------    
    String ILImageCodec::getType() const 
    {
        return mType;
    }
}
