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

#include "OgreGLFBORenderTexture.h"
#include "OgreGLPixelFormat.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreRoot.h"
#include "OgreGLHardwarePixelBuffer.h"
#include "OgreGLFBOMultiRenderTarget.h"

namespace Ogre {

//-----------------------------------------------------------------------------    
    GLFBORenderTexture::GLFBORenderTexture(GLFBOManager *manager, const String &name,
        const GLSurfaceDesc &target):
        GLRenderTexture(name, target),
        mFB(manager)
    {
        // Bind target to surface 0 and initialise
        mFB.bindSurface(0, target);
        // Get attributes
        mWidth = mFB.getWidth();
        mHeight = mFB.getHeight();
    }

    void GLFBORenderTexture::getCustomAttribute(const String& name, void* pData)
    {
        if(name=="FBO")
        {
            *static_cast<GLFrameBufferObject **>(pData) = &mFB;
        }
    }
   
/// Size of probe texture
#define PROBE_SIZE 256
/// Stencil and depth formats to be tried
GLenum stencilFormats[]={
    GL_NONE,                    // No stencil
    GL_STENCIL_INDEX1_EXT,
    GL_STENCIL_INDEX4_EXT,
    GL_STENCIL_INDEX8_EXT,
    GL_STENCIL_INDEX16_EXT
};
size_t stencilBits[] = {
    0, 1, 4, 8, 16
};
#define STENCILFORMAT_COUNT (sizeof(stencilFormats)/sizeof(GLenum))
GLenum depthFormats[]={
    GL_NONE,
    GL_DEPTH_COMPONENT16,
    GL_DEPTH_COMPONENT24,    // Prefer 24 bit depth
    GL_DEPTH_COMPONENT32,
    GL_DEPTH24_STENCIL8_EXT // packed depth / stencil
};
size_t depthBits[] = {
    0,16,24,32,24
};


#define DEPTHFORMAT_COUNT (sizeof(depthFormats)/sizeof(GLenum))
    
	GLFBOManager::GLFBOManager(bool atimode):
		mATIMode(atimode)
    {
        detectFBOFormats();
        
        glGenFramebuffersEXT(1, &mTempFBO);
    }

	GLFBOManager::~GLFBOManager()
	{
		if(!mRenderBufferMap.empty())
		{
			LogManager::getSingleton().logMessage("GL: Warning! GLFBOManager destructor called, but not all renderbuffers were released.");
		}
        
        glDeleteFramebuffersEXT(1, &mTempFBO);      
	}

    /** Try a certain FBO format, and return the status. Also sets mDepthRB and mStencilRB.
        @returns true    if this combo is supported
                 false   if this combo is not supported
    */
    GLuint GLFBOManager::_tryFormat(GLenum depthFormat, GLenum stencilFormat)
    {
        GLuint status, depthRB, stencilRB;
        bool failed = false; // flag on GL errors

        if(depthFormat != GL_NONE)
        {
            /// Generate depth renderbuffer
            glGenRenderbuffersEXT(1, &depthRB);
            /// Bind it to FBO
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthRB);
            
            /// Allocate storage for depth buffer
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthFormat,
                                PROBE_SIZE, PROBE_SIZE);
            
            /// Attach depth
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                    GL_RENDERBUFFER_EXT, depthRB);
        }

        if(stencilFormat != GL_NONE)
        {
            /// Generate stencil renderbuffer
            glGenRenderbuffersEXT(1, &stencilRB);
            /// Bind it to FBO
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, stencilRB);
            glGetError(); // NV hack
            /// Allocate storage for stencil buffer
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, stencilFormat,
                                PROBE_SIZE, PROBE_SIZE); 
            if(glGetError() != GL_NO_ERROR) // NV hack
                failed = true;
            /// Attach stencil
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                            GL_RENDERBUFFER_EXT, stencilRB);
            if(glGetError() != GL_NO_ERROR) // NV hack
                failed = true;
        }
        
        status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        /// If status is negative, clean up
        // Detach and destroy
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_NONE, 0);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_NONE, 0);
        glDeleteRenderbuffersEXT(1, &depthRB);
        glDeleteRenderbuffersEXT(1, &stencilRB);
        
        return status == GL_FRAMEBUFFER_COMPLETE_EXT && !failed;
    }
    
    /** Detect which internal formats are allowed as RTT
        Also detect what combinations of stencil and depth are allowed with this internal
        format.
    */
    void GLFBOManager::detectFBOFormats()
    {
        // Try all formats, and report which ones work as target
        GLuint fb, tid;
        GLenum target = GL_TEXTURE_2D;
        
        for(size_t x=0; x<PF_COUNT; ++x)
        {
            mProps[x].valid = false;

			// Fetch GL format token
			GLenum fmt = GLPixelUtil::getGLInternalFormat((PixelFormat)x);
            if(fmt == GL_NONE && x!=0)
                continue;

			// No test for compressed formats
			if(PixelUtil::isCompressed((PixelFormat)x))
				continue;

			// Buggy ATI cards *crash* on non-RGB(A) formats
			int depths[4];
			PixelUtil::getBitDepths((PixelFormat)x, depths);
			if(fmt!=GL_NONE && mATIMode && (!depths[0] || !depths[1] || !depths[2]))
				continue;

            // Create and attach framebuffer
            glGenFramebuffersEXT(1, &fb);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
            if(fmt!=GL_NONE)
            {
				// Create and attach texture
				glGenTextures(1, &tid);
				glBindTexture(target, tid);
				
                // Set some default parameters so it won't fail on NVidia cards         
                glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                            
				glTexImage2D(target, 0, fmt, PROBE_SIZE, PROBE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                target, tid, 0);
            }
			else
			{
				// Draw to nowhere -- stencil/depth only
				tid = 0;
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}
            // Check status
            GLuint status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

			// Ignore status in case of fmt==GL_NONE, because no implementation will accept
			// a buffer without *any* attachment. Buffers with only stencil and depth attachment
			// might still be supported, so we must continue probing.
            if(fmt == GL_NONE || status == GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                mProps[x].valid = true;
				StringUtil::StrStreamType str;
				str << "FBO " << PixelUtil::getFormatName((PixelFormat)x) 
					<< " depth/stencil support: ";
                
                // Continue detection
                size_t depth=0, stencil=0;
				
                while(true)
                {
					//StringUtil::StrStreamType l;
					//l << "Trying " << PixelUtil::getFormatName((PixelFormat)x) 
					//	<< " D" << depthBits[depth] 
					//	<< "S" << stencilBits[stencil];
					//LogManager::getSingleton().logMessage(l.str());

					// Only query packed depth/stencil formats for 32-bit
					// non-floating point formats (ie not R32!) 
					// Linux nVidia driver segfaults if you query others
                    if((depthFormats[depth] != GL_DEPTH24_STENCIL8_EXT || 
						(PixelUtil::getNumElemBits((PixelFormat)x) == 32) && 
						 !PixelUtil::isFloatingPoint((PixelFormat)x)) && 
						_tryFormat(depthFormats[depth], stencilFormats[stencil]))
                    {
                        /// Add mode to allowed modes
                        str << "D" << depthBits[depth] << "S" << 
							(depthFormats[depth] == GL_DEPTH24_STENCIL8_EXT ? 8 : stencilBits[stencil]) 
							<< " ";
                        FormatProperties::Mode mode;
                        mode.depth = depth;
                        mode.stencil = stencil;
                        mProps[x].modes.push_back(mode);
                    }
                    /// Try next combo
                    stencil++;
                    if(stencil == STENCILFORMAT_COUNT)
                    {
                        stencil = 0;
                        depth ++;
                        if(depth == DEPTHFORMAT_COUNT)
                        {
                            // We're done
                            break;
                        }
                    }
                }
				LogManager::getSingleton().logMessage(str.str());

            }
            // Delete texture and framebuffer
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glDeleteFramebuffersEXT(1, &fb);
            glDeleteTextures(1, &tid);
        }
        
        std::string fmtstring;
        for(size_t x=0; x<PF_COUNT; ++x)
        {
            if(mProps[x].valid)
                fmtstring += PixelUtil::getFormatName((PixelFormat)x)+" ";
        }
        LogManager::getSingleton().logMessage("[GL] : Valid FBO targets " + fmtstring);
    }
    void GLFBOManager::getBestDepthStencil(GLenum internalFormat, GLenum *depthFormat, GLenum *stencilFormat)
    {
        const FormatProperties &props = mProps[internalFormat];
        /// Decide what stencil and depth formats to use
        /// [best supported for internal format]
        size_t bestmode=0;
        int bestscore=-1;
        for(size_t mode=0; mode<props.modes.size(); mode++)
        {
#if 0
            /// Always prefer D24S8
            if(stencilBits[props.modes[mode].stencil]==8 &&
                depthBits[props.modes[mode].depth]==24)
            {
                bestmode = mode;
                break;
            }
#endif
            int desirability = 0;
            /// Find most desirable mode
            /// desirability == 0            if no depth, no stencil
            /// desirability == 1000...2000  if no depth, stencil
            /// desirability == 2000...3000  if depth, no stencil
            /// desirability == 3000+        if depth and stencil
            /// beyond this, the total numer of bits (stencil+depth) is maximised
            if(props.modes[mode].stencil)
                desirability += 1000;
            if(props.modes[mode].depth)
                desirability += 2000;
            if(depthBits[props.modes[mode].depth]==24) // Prefer 24 bit for now
                desirability += 500;
			if(depthFormats[props.modes[mode].depth]==GL_DEPTH24_STENCIL8_EXT) // Prefer 24/8 packed 
				desirability += 5000;
            desirability += stencilBits[props.modes[mode].stencil] + depthBits[props.modes[mode].depth];
            
            if(desirability>bestscore)
            {
                bestscore = desirability;
                bestmode = mode;
            }
        }
        *depthFormat = depthFormats[props.modes[bestmode].depth];
        *stencilFormat = stencilFormats[props.modes[bestmode].stencil];
    }

    GLFBORenderTexture *GLFBOManager::createRenderTexture(const String &name, const GLSurfaceDesc &target)
    {
        GLFBORenderTexture *retval = new GLFBORenderTexture(this, name, target);
        return retval;
    }
	MultiRenderTarget *GLFBOManager::createMultiRenderTarget(const String & name)
	{
		return new GLFBOMultiRenderTarget(this, name);
	}

    GLFrameBufferObject *GLFBOManager::createFrameBufferObject()
    {
        return new GLFrameBufferObject(this);
    }

    void GLFBOManager::destroyFrameBufferObject(GLFrameBufferObject * x)
    {
        delete x;
    }
    void GLFBOManager::bind(RenderTarget *target)
    {
        /// Check if the render target is in the rendertarget->FBO map
        GLFrameBufferObject *fbo = 0;
        target->getCustomAttribute("FBO", &fbo);
        if(fbo)
            fbo->bind();
        else
            // Old style context (window/pbuffer) or copying render texture
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
    
    GLSurfaceDesc GLFBOManager::requestRenderBuffer(GLenum format, size_t width, size_t height)
    {
        GLSurfaceDesc retval;
        retval.buffer = 0; // Return 0 buffer if GL_NONE is requested
        if(format != GL_NONE)
        {
            RBFormat key(format, width, height);
            RenderBufferMap::iterator it = mRenderBufferMap.find(key);
            if(it != mRenderBufferMap.end())
            {
                retval.buffer = it->second.buffer;
                retval.zoffset = 0;
                // Increase refcount
                ++it->second.refcount;
            }
            else
            {
                // New one
                GLRenderBuffer *rb = new GLRenderBuffer(format, width, height);
                mRenderBufferMap[key] = RBRef(rb);
                retval.buffer = rb;
                retval.zoffset = 0;
            }
        }
        //std::cerr << "Requested renderbuffer with format " << std::hex << format << std::dec << " of " << width << "x" << height << " :" << retval.buffer << std::endl;
        return retval;
    }
    void GLFBOManager::releaseRenderBuffer(const GLSurfaceDesc &surface)
    {
        if(surface.buffer == 0)
            return;
        RBFormat key(surface.buffer->getGLFormat(), surface.buffer->getWidth(), surface.buffer->getHeight());
        RenderBufferMap::iterator it = mRenderBufferMap.find(key);
        if(it != mRenderBufferMap.end())
		{
			// Decrease refcount
			--it->second.refcount;
			if(it->second.refcount==0)
			{
				// If refcount reaches zero, delete buffer and remove from map
				delete it->second.buffer;
				mRenderBufferMap.erase(it);
				//std::cerr << "Destroyed renderbuffer of format " << std::hex << key.format << std::dec
				//        << " of " << key.width << "x" << key.height << std::endl;
			}
		}
    }
}
