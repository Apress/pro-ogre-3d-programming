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
#ifndef _Texture_H__
#define _Texture_H__

#include "OgrePrerequisites.h"
#include "OgreHardwareBuffer.h"
#include "OgreResource.h"
#include "OgreImage.h"

namespace Ogre {

    /** Enum identifying the texture usage
    */
    enum TextureUsage
    {
		/// @copydoc HardwareBuffer::Usage
		TU_STATIC = HardwareBuffer::HBU_STATIC,
		TU_DYNAMIC = HardwareBuffer::HBU_DYNAMIC,
		TU_WRITE_ONLY = HardwareBuffer::HBU_WRITE_ONLY,
		TU_STATIC_WRITE_ONLY = HardwareBuffer::HBU_STATIC_WRITE_ONLY, 
		TU_DYNAMIC_WRITE_ONLY = HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE = HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE,
		/// mipmaps will be automatically generated for this texture
		TU_AUTOMIPMAP = 0x100,
		/// this texture will be a render target, ie. used as a target for render to texture
		/// setting this flag will ignore all other texture usages except TU_AUTOMIPMAP
		TU_RENDERTARGET = 0x200,
		/// default to automatic mipmap generation static textures
		TU_DEFAULT = TU_AUTOMIPMAP | TU_STATIC_WRITE_ONLY
        
    };

    /** Enum identifying the texture type
    */
    enum TextureType
    {
        /// 1D texture, used in combination with 1D texture coordinates
        TEX_TYPE_1D = 1,
        /// 2D texture, used in combination with 2D texture coordinates (default)
        TEX_TYPE_2D = 2,
        /// 3D volume texture, used in combination with 3D texture coordinates
        TEX_TYPE_3D = 3,
        /// 3D cube map, used in combination with 3D texture coordinates
        TEX_TYPE_CUBE_MAP = 4
    };

	/** Enum identifying special mipmap numbers
    */
	enum TextureMipmap
	{
		/// Generate mipmaps up to 1x1
		MIP_UNLIMITED = 0x7FFFFFFF
	};

    // Forward declaration
    class TexturePtr;

    /** Abstract class representing a Texture resource.
        @remarks
            The actual concrete subclass which will exist for a texture
            is dependent on the rendering system in use (Direct3D, OpenGL etc).
            This class represents the commonalities, and is the one 'used'
            by programmers even though the real implementation could be
            different in reality. Texture objects are created through
            the 'create' method of the TextureManager concrete subclass.
     */
    class _OgreExport Texture : public Resource
    {
    public:
        Texture(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual = false, ManualResourceLoader* loader = 0);

        /** Sets the type of texture; can only be changed before load() 
        */
        virtual void setTextureType(TextureType ttype ) { mTextureType = ttype; }

        /** Gets the type of texture 
        */
        virtual TextureType getTextureType(void) const { return mTextureType; }

        /** Gets the number of mipmaps to be used for this texture.
        */
        virtual size_t getNumMipmaps(void) const {return mNumMipmaps;}

		/** Sets the number of mipmaps to be used for this texture.
            @note
                Must be set before calling any 'load' method.
        */
        virtual void setNumMipmaps(size_t num) {mNumRequestedMipmaps = mNumMipmaps = num;}

		/** Are mipmaps hardware generated?
		@remarks
			Will only be accurate after texture load, or createInternalResources
		*/
		virtual bool getMipmapsHardwareGenerated(void) const { return mMipmapsHardwareGenerated; }

        /** Returns the gamma adjustment factor applied to this texture.
        */
        virtual float getGamma(void) const { return mGamma; }

        /** Sets the gamma adjustment factor applied to this texture.
            @note
                Must be called before any 'load' method.
        */
        virtual void setGamma(float g) { mGamma = g; }

        /** Returns the height of the texture.
        */
        virtual unsigned int getHeight(void) const { return mHeight; }

        /** Returns the width of the texture.
        */
        virtual unsigned int getWidth(void) const { return mWidth; }

        /** Returns the depth of the texture (only applicable for 3D textures).
        */
        virtual unsigned int getDepth(void) const { return mDepth; }

        /** Returns the height of the original input texture (may differ due to hardware requirements).
        */
        virtual unsigned int getSrcHeight(void) const { return mSrcHeight; }

        /** Returns the width of the original input texture (may differ due to hardware requirements).
        */
        virtual unsigned int getSrcWidth(void) const { return mSrcWidth; }

        /** Returns the original depth of the input texture (only applicable for 3D textures).
        */
        virtual unsigned int getSrcDepth(void) const { return mSrcDepth; }

        /** Set the height of the texture; can only do this before load();
        */
        virtual void setHeight(unsigned int h) { mHeight = mSrcHeight = h; }

        /** Set the width of the texture; can only do this before load();
        */
        virtual void setWidth(unsigned int w) { mWidth = mSrcWidth = w; }

        /** Set the depth of the texture (only applicable for 3D textures);
            ; can only do this before load();
        */
        virtual void setDepth(unsigned int d)  { mDepth = mSrcDepth = d; }

        /** Returns the TextureUsage indentifier for this Texture
        */
        virtual int getUsage() const
        {
            return mUsage;
        }

        /** Sets the TextureUsage indentifier for this Texture; only useful before load()
			
			@param u is a combination of TU_STATIC, TU_DYNAMIC, TU_WRITE_ONLY 
				TU_AUTOMIPMAP and TU_RENDERTARGET (see TextureUsage enum). You are
            	strongly advised to use HBU_STATIC_WRITE_ONLY wherever possible, if you need to 
            	update regularly, consider HBU_DYNAMIC_WRITE_ONLY.
        */
        virtual void setUsage(int u) { mUsage = u; }

        /** Creates the internal texture resources for this texture. 
        @remarks
            This method creates the internal texture resources (pixel buffers, 
            texture surfaces etc) required to begin using this texture. You do
            not need to call this method directly unless you are manually creating
            a texture, in which case something must call it, after having set the
            size and format of the texture (e.g. the ManualResourceLoader might
            be the best one to call it). If you are not defining a manual texture,
            or if you use one of the self-contained load...() methods, then it will be
            called for you.
        */
        virtual void createInternalResources(void);

        /** Frees internal texture resources for this texture. 
        */
        virtual void freeInternalResources(void);
        
		/** Copies (and maybe scales to fit) the contents of this texture to
			another texture. */
		virtual void copyToTexture( TexturePtr& target );

        /** Loads the data from an image.
        */
        virtual void loadImage( const Image &img ) = 0;
			
		/** Loads the data from a raw stream.
		*/
		virtual void loadRawData( DataStreamPtr& stream, 
			ushort uWidth, ushort uHeight, PixelFormat eFormat);

        virtual void enable32Bit( bool setting = true ) 
        {
            setting ? mFinalBpp = 32 : mFinalBpp = 16;
        }

		/** Returns the pixel format for the texture surface. */
		virtual PixelFormat getFormat() const
		{
			return mFormat;
		}

        /** Sets the pixel format for the texture surface; can only be set before load(). */
        virtual void setFormat(PixelFormat pf);

        /** Returns true if the texture has an alpha layer. */
        virtual bool hasAlpha(void) const
        {
            return mHasAlpha;
        }
        
        /** Return the number of faces this texture has. This will be 6 for a cubemap
        	texture and 1 for a 1D, 2D or 3D one.
        */
        virtual size_t getNumFaces() const;

		/** Return hardware pixel buffer for a surface. This buffer can then
			be used to copy data from and to a particular level of the texture.
			@param face 	Face number, in case of a cubemap texture. Must be 0
							for other types of textures.
                            For cubemaps, this is one of 
                            +X (0), -X (1), +Y (2), -Y (3), +Z (4), -Z (5)
			@param mipmap	Mipmap level. This goes from 0 for the first, largest
							mipmap level to getNumMipmaps()-1 for the smallest.
			@returns	A shared pointer to a hardware pixel buffer
			@remarks	The buffer is invalidated when the resource is unloaded or destroyed.
						Do not use it after the lifetime of the containing texture.
		*/
		virtual HardwarePixelBufferSharedPtr getBuffer(size_t face=0, size_t mipmap=0) = 0;

    protected:
        unsigned long mHeight;
        unsigned long mWidth;
        unsigned long mDepth;

        size_t mNumRequestedMipmaps;
		size_t mNumMipmaps;
		bool mMipmapsHardwareGenerated;
        float mGamma;

        TextureType mTextureType;
		PixelFormat mFormat;
        int mUsage; // Bit field, so this can't be TextureUsage

        unsigned short mSrcBpp;
        unsigned long mSrcWidth, mSrcHeight, mSrcDepth;
        unsigned short mFinalBpp;
        bool mHasAlpha;

		bool mInternalResourcesCreated;

		/// @copydoc Resource::calculateSize
		size_t calculateSize(void) const;
		
		/** Generic method to load the texture from a set of images. This can be
		 	used by the specific implementation for convience. Implementations
			might decide not to use this function if they can use their own image loading
			functions.
			@param images	Vector of pointers to Images. If there is only one image
			in this vector, the faces of that image will be used. If there are multiple
			images in the vector each image will be loaded as a face.
		*/
        virtual void _loadImages( const std::vector<const Image*>& images );


		/** Implementation of creating internal texture resources 
		*/
		virtual void createInternalResourcesImpl(void) = 0;

		/** Implementation of freeing internal texture resources 
		*/
		virtual void freeInternalResourcesImpl(void) = 0;

		/** Default implementation of unload which calls freeInternalResources */
		void unloadImpl(void);

    };

    /** Specialisation of SharedPtr to allow SharedPtr to be assigned to TexturePtr 
    @note Has to be a subclass since we need operator=.
    We could templatise this instead of repeating per Resource subclass, 
    except to do so requires a form VC6 does not support i.e.
    ResourceSubclassPtr<T> : public SharedPtr<T>
    */
    class _OgreExport TexturePtr : public SharedPtr<Texture> 
    {
    public:
        TexturePtr() : SharedPtr<Texture>() {}
        explicit TexturePtr(Texture* rep) : SharedPtr<Texture>(rep) {}
        TexturePtr(const TexturePtr& r) : SharedPtr<Texture>(r) {} 
        TexturePtr(const ResourcePtr& r) : SharedPtr<Texture>()
        {
			// lock & copy other mutex pointer
            OGRE_MUTEX_CONDITIONAL(r.OGRE_AUTO_MUTEX_NAME)
            {
			    OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			    OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
                pRep = static_cast<Texture*>(r.getPointer());
                pUseCount = r.useCountPointer();
                if (pUseCount)
                {
                    ++(*pUseCount);
                }
            }
        }

        /// Operator used to convert a ResourcePtr to a TexturePtr
        TexturePtr& operator=(const ResourcePtr& r)
        {
            if (pRep == static_cast<Texture*>(r.getPointer()))
                return *this;
            release();
			// lock & copy other mutex pointer
            OGRE_MUTEX_CONDITIONAL(r.OGRE_AUTO_MUTEX_NAME)
            {
			    OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			    OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
                pRep = static_cast<Texture*>(r.getPointer());
                pUseCount = r.useCountPointer();
                if (pUseCount)
                {
                    ++(*pUseCount);
                }
            }
            return *this;
        }
    };

}

#endif
