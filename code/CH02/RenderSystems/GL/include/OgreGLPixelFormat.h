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
#ifndef __GLPIXELFORMAT_H__
#define __GLPIXELFORMAT_H__

#include "OgreGLPrerequisites.h"
#include "OgrePixelFormat.h"
namespace Ogre {
    
	/**
	* Class to do pixel format mapping between GL and OGRE
	*/
	class GLPixelUtil {
	public:
		/** Takes the OGRE pixel format and returns the appropriate GL one
			@returns a GLenum describing the format, or 0 if there is no exactly matching 
			one (and conversion is needed)
		*/
		static GLenum getGLOriginFormat(PixelFormat mFormat);
	
		/** Takes the OGRE pixel format and returns type that must be provided
			to GL as data type for reading it into the GPU
			@returns a GLenum describing the data type, or 0 if there is no exactly matching 
			one (and conversion is needed)
		*/
		static GLenum getGLOriginDataType(PixelFormat mFormat);
        
        /**	Takes the OGRE pixel format and returns the type that must be provided
			to GL as internal format. GL_NONE if no match exists.
		*/
		static GLenum getGLInternalFormat(PixelFormat mFormat);
	
		/**	Takes the OGRE pixel format and returns the type that must be provided
			to GL as internal format. If no match exists, returns the closest match.
		*/
		static GLenum getClosestGLInternalFormat(PixelFormat mFormat);
		
		/**	Function to get the closest matching OGRE format to an internal GL format. To be
			precise, the format will be chosen that is most efficient to transfer to the card 
			without losing precision.
			@remarks It is valid for this function to always return PF_A8R8G8B8.
		*/
		static PixelFormat getClosestOGREFormat(GLenum fmt);
	
		/** Returns the maximum number of Mipmaps that can be generated until we reach
			the mininum format possible. This does not count the base level.
			@param width
				The width of the area
			@param height
				The height of the area
			@param depth
				The depth of the area
			@param format
				The format of the area
			@remarks
				In case that the format is non-compressed, this simply returns
				how many times we can divide this texture in 2 until we reach 1x1.
				For compressed formats, constraints apply on minimum size and alignment
				so this might differ.
		*/
		static size_t getMaxMipmaps(size_t width, size_t height, size_t depth, PixelFormat format);
        
        /** Returns next power-of-two size if required by render system, in case
            RSC_NON_POWER_OF_2_TEXTURES is supported it returns value as-is.
        */
        static size_t optionalPO2(size_t value);
	};
};

#endif
