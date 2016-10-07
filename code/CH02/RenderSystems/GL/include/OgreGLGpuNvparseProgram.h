/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.stevestreeting.com/ogre/

Copyright (c) 2000-2005 The OGRE Teameeting
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/gpl.html.
-----------------------------------------------------------------------------
*/

#ifndef __GLGpuNvparseProgram_H__
#define __GLGpuNvparseProgram_H__

#include "OgreGLPrerequisites.h"
#include "OgreGLGpuProgram.h"

namespace Ogre {

class GLGpuNvparseProgram : public GLGpuProgram
{
public:
    GLGpuNvparseProgram(ResourceManager* creator, 
        const String& name, ResourceHandle handle, 
        const String& group, bool isManual, ManualResourceLoader* loader);
    virtual ~GLGpuNvparseProgram();


    /// Execute the binding functions for this program
    void bindProgram(void);
    /// Execute the unbinding functions for this program
    void unbindProgram(void);
    /// Execute the param binding functions for this program
    void bindProgramParameters(GpuProgramParametersSharedPtr params);

    /// Get the assigned GL program id
    const GLuint getProgramID(void) const
    { return mProgramID; }

protected:
    /// @copydoc Resource::unload
    void unloadImpl(void);
    void loadFromSource(void);

private:
    GLuint mProgramID;
    GLenum mProgramType;
};

}; // namespace Ogre

#endif // __GLGpuNvparseProgram_H__
