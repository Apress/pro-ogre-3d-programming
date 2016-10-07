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
#ifndef __GLSLProgram_H__
#define __GLSLProgram_H__

#include "OgreGLPrerequisites.h"
#include "OgreHighLevelGpuProgram.h"

namespace Ogre {
    /** Specialisation of HighLevelGpuProgram to provide support for OpenGL 
        Shader Language (GLSL).
    @remarks
		GLSL has no target assembler or entry point specification like DirectX 9 HLSL.
		Vertex and Fragment shaders only have one entry point called "main".  
		When a shader is compiled, microcode is generated but can not be accessed by
		the application.
		GLSL also does not provide assembler low level output after compiling.  The GL Render
		system assumes that the Gpu program is a GL Gpu program so GLSLProgram will create a 
		GLSLGpuProgram that is subclassed from GLGpuProgram for the low level implementation.
		The GLSLProgram class will create a shader object and compile the source but will
		not create a program object.  It's up to GLSLGpuProgram class to request a program object
		to link the shader object to.

	@note
		GLSL supports multiple modular shader objects that can be attached to one program
		object to form a single shader.  This is supported through the "attach" material script
		command.  All the modules to be attached are listed on the same line as the attach command
		seperated by white space.
        
    */
    class GLSLProgram : public HighLevelGpuProgram
    {
    public:
        /// Command object for attaching another GLSL Program 
        class CmdAttach : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& shaderNames);
        };

        GLSLProgram(ResourceManager* creator, 
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader);
		~GLSLProgram();

		const GLhandleARB getGLHandle() const { return mGLHandle; }
		void attachToProgramObject( const GLhandleARB programObject );
		String getAttachedShaderNames() const { return mAttachedShaderNames; }

        /** Attach another GLSL Shader to this one. */
        void attachChildShader(const String& name);
        /// Overridden from GpuProgram
        const String& getLanguage(void) const;

	protected:
        static CmdAttach msCmdAttach;

        /** Internal load implementation, must be implemented by subclasses.
        */
        void loadFromSource(void);
        /** Internal method for creating a dummy low-level program for this
        high-level program.	GLSL does not give access to the low level implementation of the
		shader so this method creates an object sub-classed from GLGpuProgram just to be
		compatible with	GLRenderSystem.
		*/
		void createLowLevelImpl(void);
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl(void);
        /// Populate the passed parameters with name->index map, must be overridden
        void populateParameterNames(GpuProgramParametersSharedPtr params);
		/// compile source into shader object
		bool compile( const bool checkErrors = true);

	private:
		/// GL handle for shader object
		GLhandleARB mGLHandle;
		/// flag indicating if shader object successfully compiled
		GLint mCompiled;
		/// attached Shader names
		String mAttachedShaderNames;
		/// container of attached programs
		typedef std::vector< GLSLProgram* > GLSLProgramContainer;
		typedef GLSLProgramContainer::iterator GLSLProgramContainerIterator;
		GLSLProgramContainer mAttachedGLSLPrograms;

    };
}

#endif // __GLSLProgram_H__
