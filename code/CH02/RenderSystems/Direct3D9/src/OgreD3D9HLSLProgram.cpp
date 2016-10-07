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
#include "OgreD3D9HLSLProgram.h"
#include "OgreGpuProgramManager.h"
#include "OgreStringConverter.h"
#include "OgreD3D9GpuProgram.h"
#include "OgreGpuProgram.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreRenderSystemCapabilities.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    D3D9HLSLProgram::CmdEntryPoint D3D9HLSLProgram::msCmdEntryPoint;
    D3D9HLSLProgram::CmdTarget D3D9HLSLProgram::msCmdTarget;
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::loadFromSource(void)
    {
        LPD3DXBUFFER errors = 0;

        // Compile & assemble into microcode
        HRESULT hr = D3DXCompileShader(
            mSource.c_str(),
            static_cast<UINT>(mSource.length()),
            NULL, //no preprocessor defines
            NULL, //no includes
            mEntryPoint.c_str(),
            mTarget.c_str(),
            NULL, // no compile flags
            &mpMicroCode,
            &errors,
            &mpConstTable);

        if (FAILED(hr))
        {
            String message = "Cannot assemble D3D9 high-level shader " + mName + " Errors:\n" +
                static_cast<const char*>(errors->GetBufferPointer());
            errors->Release();
            OGRE_EXCEPT(hr, message,
                "D3D9HLSLProgram::loadFromSource");
        }


    }
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::createLowLevelImpl(void)
    {
        // Create a low-level program, give it the same name as us
        mAssemblerProgram = 
            GpuProgramManager::getSingleton().createProgramFromString(
                mName, 
                mGroup,
                "",// dummy source, since we'll be using microcode
                mType, 
                mTarget);
        static_cast<D3D9GpuProgram*>(mAssemblerProgram.get())->setExternalMicrocode(mpMicroCode);

    }
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::unloadHighLevelImpl(void)
    {
        SAFE_RELEASE(mpMicroCode);
        SAFE_RELEASE(mpConstTable);

    }
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::populateParameterNames(GpuProgramParametersSharedPtr params)
    {
        // Derive parameter names from const table
        assert(mpConstTable && "Program not loaded!");
        // Get contents of the constant table
        D3DXCONSTANTTABLE_DESC desc;
        HRESULT hr = mpConstTable->GetDesc(&desc);

        if (FAILED(hr))
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, 
                "Cannot retrieve constant descriptions from HLSL program.", 
                "D3D9HLSLProgram::populateParameterNames");
        }
        // Iterate over the constants
        for (unsigned int i = 0; i < desc.Constants; ++i)
        {
            // Recursively descend through the structure levels
            // Since D3D9 has no nice 'leaf' method like Cg (sigh)
            processParamElement(NULL, "", i, params);
        }

        
    }
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::processParamElement(D3DXHANDLE parent, String prefix, 
        unsigned int index, GpuProgramParametersSharedPtr params)
    {
        D3DXHANDLE hConstant = mpConstTable->GetConstant(parent, index);

        // Since D3D HLSL doesn't deal with naming of array and struct parameters
        // automatically, we have to do it by hand

        D3DXCONSTANT_DESC desc;
        unsigned int numParams = 1;
        HRESULT hr = mpConstTable->GetConstantDesc(hConstant, &desc, &numParams);
        if (FAILED(hr))
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, 
                "Cannot retrieve constant description from HLSL program.", 
                "D3D9HLSLProgram::processParamElement");
        }

        String paramName = desc.Name;
        // trim the odd '$' which appears at the start of the names in HLSL
        if (paramName.at(0) == '$')
            paramName.erase(paramName.begin());

        // If it's an array, elements will be > 1
        for (unsigned int e = 0; e < desc.Elements; ++e)
        {
            if (desc.Class == D3DXPC_STRUCT)
            {
                // work out a new prefix for nested members, if it's an array, we need an index
                if (desc.Elements > 1)
                    prefix = prefix + paramName + "[" + StringConverter::toString(e) + "].";
                else
                    prefix = prefix + paramName + ".";
                // Cascade into struct
                for (unsigned int i = 0; i < desc.StructMembers; ++i)
                {
                    processParamElement(hConstant, prefix, i, params);
                }
            }
            else
            {
                // Process params
                if (desc.Type == D3DXPT_FLOAT || desc.Type == D3DXPT_INT || desc.Type == D3DXPT_BOOL)
                {
                    size_t paramIndex = desc.RegisterIndex + e * desc.RegisterCount / desc.Elements;
                    String name = prefix + paramName;
                    // If this is an array, need to append element index
                    if (desc.Elements > 1)
                        name += "[" + StringConverter::toString(e) + "]";
                    
                    params->_mapParameterNameToIndex(name, paramIndex);
                    // setup constant definition
                    // is it float or int
                    GpuProgramParameters::ElementType elementType = GpuProgramParameters::ET_INT;
                    if (desc.Type == D3DXPT_FLOAT)
                        elementType = GpuProgramParameters::ET_REAL;
                    params->addConstantDefinition(name, paramIndex, 0, elementType);
                }
            }
        }
            
    }
    //-----------------------------------------------------------------------
    D3D9HLSLProgram::D3D9HLSLProgram(ResourceManager* creator, const String& name, 
        ResourceHandle handle, const String& group, bool isManual, 
        ManualResourceLoader* loader)
        : HighLevelGpuProgram(creator, name, handle, group, isManual, loader)
        , mpMicroCode(NULL), mpConstTable(NULL)
    {
        if (createParamDictionary("D3D9HLSLProgram"))
        {
            setupBaseParamDictionary();
            ParamDictionary* dict = getParamDictionary();

            dict->addParameter(ParameterDef("entry_point", 
                "The entry point for the HLSL program.",
                PT_STRING),&msCmdEntryPoint);
            dict->addParameter(ParameterDef("target", 
                "Name of the assembler target to compile down to.",
                PT_STRING),&msCmdTarget);
        }
        
    }
    //-----------------------------------------------------------------------
    D3D9HLSLProgram::~D3D9HLSLProgram()
    {
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if (mIsLoaded)
        {
            unload();
        }
        else
        {
            unloadHighLevel();
        }
    }
    //-----------------------------------------------------------------------
    bool D3D9HLSLProgram::isSupported(void) const
    {
		// If skeletal animation is being done, we need support for UBYTE4
		if (isSkeletalAnimationIncluded() && 
			!Root::getSingleton().getRenderSystem()->getCapabilities()
				->hasCapability(RSC_VERTEX_FORMAT_UBYTE4))
		{
			return false;
		}

        return GpuProgramManager::getSingleton().isSyntaxSupported(mTarget);
    }
    //-----------------------------------------------------------------------
    GpuProgramParametersSharedPtr D3D9HLSLProgram::createParameters(void)
    {
        // Call superclass
        GpuProgramParametersSharedPtr params = HighLevelGpuProgram::createParameters();

        // D3D HLSL uses column-major matrices
        params->setTransposeMatrices(true);

        return params;
    }
    //-----------------------------------------------------------------------
    void D3D9HLSLProgram::setTarget(const String& target)
    {
        mTarget = target;
    }

    //-----------------------------------------------------------------------
    const String& D3D9HLSLProgram::getLanguage(void) const
    {
        static const String language = "hlsl";

        return language;
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String D3D9HLSLProgram::CmdEntryPoint::doGet(const void *target) const
    {
        return static_cast<const D3D9HLSLProgram*>(target)->getEntryPoint();
    }
    void D3D9HLSLProgram::CmdEntryPoint::doSet(void *target, const String& val)
    {
        static_cast<D3D9HLSLProgram*>(target)->setEntryPoint(val);
    }
    //-----------------------------------------------------------------------
    String D3D9HLSLProgram::CmdTarget::doGet(const void *target) const
    {
        return static_cast<const D3D9HLSLProgram*>(target)->getTarget();
    }
    void D3D9HLSLProgram::CmdTarget::doSet(void *target, const String& val)
    {
        static_cast<D3D9HLSLProgram*>(target)->setTarget(val);
    }

}
