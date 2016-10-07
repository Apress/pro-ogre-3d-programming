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
#include "OgreCompositionTechnique.h"
#include "OgreCompositionTargetPass.h"
#include "OgreCompositorInstance.h"
#include "OgreCompositorChain.h"
#include "OgreCompositionPass.h"
#include "OgreTextureManager.h"

namespace Ogre {

CompositionTechnique::CompositionTechnique(Compositor *parent):
    mParent(parent)
{
    mOutputTarget = new CompositionTargetPass(this);
}
//-----------------------------------------------------------------------
CompositionTechnique::~CompositionTechnique()
{
	/// Destroy all instances by removing them from their chain
	/// CompositorChain::removeInstance also calls destroyInstance
	Instances copy = mInstances;
	for(Instances::iterator i=copy.begin(); i!=copy.end(); ++i)
		(*i)->getChain()->_removeInstance(*i);

    removeAllTextureDefinitions();
    removeAllTargetPasses();
    delete mOutputTarget;
}
//-----------------------------------------------------------------------
CompositionTechnique::TextureDefinition *CompositionTechnique::createTextureDefinition(const String &name)
{
    TextureDefinition *t = new TextureDefinition();
    t->name = name;
    mTextureDefinitions.push_back(t);
    return t;
}
//-----------------------------------------------------------------------

void CompositionTechnique::removeTextureDefinition(size_t index)
{
    assert (index < mTextureDefinitions.size() && "Index out of bounds.");
    TextureDefinitions::iterator i = mTextureDefinitions.begin() + index;
    delete(*i);
    mTextureDefinitions.erase(i);
}
//-----------------------------------------------------------------------

CompositionTechnique::TextureDefinition *CompositionTechnique::getTextureDefinition(size_t index)
{
    assert (index < mTextureDefinitions.size() && "Index out of bounds.");
    return mTextureDefinitions[index];
}
//-----------------------------------------------------------------------

size_t CompositionTechnique::getNumTextureDefinitions()
{
    return mTextureDefinitions.size();
}
//-----------------------------------------------------------------------
void CompositionTechnique::removeAllTextureDefinitions()
{
    TextureDefinitions::iterator i, iend;
    iend = mTextureDefinitions.end();
    for (i = mTextureDefinitions.begin(); i != iend; ++i)
    {
        delete(*i);
    }
    mTextureDefinitions.clear();
}
//-----------------------------------------------------------------------
CompositionTechnique::TextureDefinitionIterator CompositionTechnique::getTextureDefinitionIterator(void)
{
    return TextureDefinitionIterator(mTextureDefinitions.begin(), mTextureDefinitions.end());
}

//-----------------------------------------------------------------------
CompositionTargetPass *CompositionTechnique::createTargetPass()
{
    CompositionTargetPass *t = new CompositionTargetPass(this);
    mTargetPasses.push_back(t);
    return t;
}
//-----------------------------------------------------------------------

void CompositionTechnique::removeTargetPass(size_t index)
{
    assert (index < mTargetPasses.size() && "Index out of bounds.");
    TargetPasses::iterator i = mTargetPasses.begin() + index;
    delete(*i);
    mTargetPasses.erase(i);
}
//-----------------------------------------------------------------------

CompositionTargetPass *CompositionTechnique::getTargetPass(size_t index)
{
    assert (index < mTargetPasses.size() && "Index out of bounds.");
    return mTargetPasses[index];
}
//-----------------------------------------------------------------------

size_t CompositionTechnique::getNumTargetPasses()
{
    return mTargetPasses.size();
}
//-----------------------------------------------------------------------
void CompositionTechnique::removeAllTargetPasses()
{
    TargetPasses::iterator i, iend;
    iend = mTargetPasses.end();
    for (i = mTargetPasses.begin(); i != iend; ++i)
    {
        delete(*i);
    }
    mTargetPasses.clear();
}
//-----------------------------------------------------------------------
CompositionTechnique::TargetPassIterator CompositionTechnique::getTargetPassIterator(void)
{
    return TargetPassIterator(mTargetPasses.begin(), mTargetPasses.end());
}
//-----------------------------------------------------------------------
CompositionTargetPass *CompositionTechnique::getOutputTargetPass()
{
    return mOutputTarget;
}
//-----------------------------------------------------------------------
bool CompositionTechnique::isSupported(bool acceptTextureDegradation)
{
	// A technique is supported if all materials referenced have a supported
	// technique, and the intermediate texture formats requested are supported
	// Material support is a cast-iron requirement, but if no texture formats 
	// are directly supported we can let the rendersystem create the closest 
	// match for the least demanding technique
	

    // Check output target pass is supported
    if (!mOutputTarget->_isSupported())
    {
        return false;
    }

    // Check all target passes is supported
    TargetPasses::iterator pi, piend;
    piend = mTargetPasses.end();
    for (pi = mTargetPasses.begin(); pi != piend; ++pi)
    {
        CompositionTargetPass* targetPass = *pi;
        if (!targetPass->_isSupported())
        {
            return false;
        }
    }

    TextureDefinitions::iterator i, iend;
    iend = mTextureDefinitions.end();
	TextureManager& texMgr = TextureManager::getSingleton();
    for (i = mTextureDefinitions.begin(); i != iend; ++i)
    {
		TextureDefinition* td = *i;

		// Check whether equivalent supported
		if(acceptTextureDegradation)
		{
			// Don't care about exact format so long as something is supported
			if(texMgr.getNativeFormat(TEX_TYPE_2D, td->format, TU_RENDERTARGET) == PF_UNKNOWN)
			{
				return false;
			}
		}
		else
		{
			// Need a format which is the same number of bits to pass
			if (!texMgr.isEquivalentFormatSupported(TEX_TYPE_2D, td->format, TU_RENDERTARGET))
			{
				return false;
			}	
		}
	}
	
	// Must be ok
	return true;
}
//-----------------------------------------------------------------------
CompositorInstance *CompositionTechnique::createInstance(CompositorChain *chain)
{
	CompositorInstance *mew = new CompositorInstance(mParent, this, chain);
	mInstances.push_back(mew);
    return mew;
}
//-----------------------------------------------------------------------
void CompositionTechnique::destroyInstance(CompositorInstance *instance)
{
    assert(instance->getTechnique() == this);
	/// Erase from list of instances
	mInstances.erase(std::find(mInstances.begin(), mInstances.end(), instance));
    delete instance;
}
//-----------------------------------------------------------------------
Compositor *CompositionTechnique::getParent()
{
    return mParent;
}

}
