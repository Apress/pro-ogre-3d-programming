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
#include "OgreStableHeaders.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreException.h"
#include "OgreGpuProgramManager.h"

namespace Ogre
{
    //---------------------------------------------------------------------------
    HighLevelGpuProgram::HighLevelGpuProgram(ResourceManager* creator, 
        const String& name, ResourceHandle handle, const String& group, 
        bool isManual, ManualResourceLoader* loader)
        : GpuProgram(creator, name, handle, group, isManual, loader), 
        mHighLevelLoaded(false), mAssemblerProgram(0)
    {
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadImpl()
    {
        // load self 
        loadHighLevel();

        // create low-level implementation
        createLowLevelImpl();
        // load constructed assembler program (if it exists)
		if (!mAssemblerProgram.isNull())
		{
			mAssemblerProgram->load();
			mIsLoaded = true;
		}
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::unloadImpl()
    {   
        if (!mAssemblerProgram.isNull())
        {
            mAssemblerProgram->getCreator()->remove(mAssemblerProgram->getHandle());
            mAssemblerProgram.setNull();
        }

        unloadHighLevel();
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgram::~HighLevelGpuProgram()
    {
        // superclasses will trigger unload
    }
    //---------------------------------------------------------------------------
    GpuProgramParametersSharedPtr HighLevelGpuProgram::createParameters(void)
    {
        // Make sure param defs are loaded
        GpuProgramParametersSharedPtr params = GpuProgramManager::getSingleton().createParameters();
		// Only populate named parameters if we can support this program
		if (this->isSupported())
		{
			loadHighLevel();
			populateParameterNames(params);
		}
		// Copy in default parameters if present
		if (!mDefaultParams.isNull())
			params->copyConstantsFrom(*(mDefaultParams.get()));
        return params;
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadHighLevel(void)
    {
        if (!mHighLevelLoaded)
        {
            loadHighLevelImpl();
            mHighLevelLoaded = true;
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::unloadHighLevel(void)
    {
        if (mHighLevelLoaded)
        {
            unloadHighLevelImpl();
            mHighLevelLoaded = false;
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadHighLevelImpl(void)
    {
        if (mLoadFromFile)
        {
            // find & load source code
            DataStreamPtr stream = 
                ResourceGroupManager::getSingleton().openResource(
                    mFilename, mGroup, true, this);

            mSource = stream->getAsString();
        }

        loadFromSource();
    }
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	HighLevelGpuProgramPtr& HighLevelGpuProgramPtr::operator=(const GpuProgramPtr& r)
	{
		// Can assign direct
		if (pRep == static_cast<HighLevelGpuProgram*>(r.getPointer()))
			return *this;
		release();
		// lock & copy other mutex pointer
        OGRE_MUTEX_CONDITIONAL(r.OGRE_AUTO_MUTEX_NAME)
        {
		    OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
		    OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
		    pRep = static_cast<HighLevelGpuProgram*>(r.getPointer());
		    pUseCount = r.useCountPointer();
		    if (pUseCount)
		    {
			    ++(*pUseCount);
		    }
        }
		return *this;
	}


}
