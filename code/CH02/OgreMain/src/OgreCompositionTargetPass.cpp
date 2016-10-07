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
#include "OgreCompositionTargetPass.h"
#include "OgreCompositionPass.h"
#include "OgreMaterialManager.h"

namespace Ogre {

CompositionTargetPass::CompositionTargetPass(CompositionTechnique *parent):
    mParent(parent),
    mInputMode(IM_NONE),
    mOnlyInitial(false),
    mVisibilityMask(0xFFFFFFFF),
    mLodBias(1.0f),
	mMaterialScheme(MaterialManager::DEFAULT_SCHEME_NAME)
{
}
//-----------------------------------------------------------------------
CompositionTargetPass::~CompositionTargetPass()
{
    removeAllPasses();
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setInputMode(InputMode mode)
{
    mInputMode = mode;
}
//-----------------------------------------------------------------------
CompositionTargetPass::InputMode CompositionTargetPass::getInputMode() const
{
    return mInputMode;
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setOutputName(const String &out)
{
    mOutputName = out;
}
//-----------------------------------------------------------------------
const String &CompositionTargetPass::getOutputName() const
{
    return mOutputName;
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setOnlyInitial(bool value)
{
    mOnlyInitial = value;
}
//-----------------------------------------------------------------------
bool CompositionTargetPass::getOnlyInitial()
{
    return mOnlyInitial;
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setVisibilityMask(uint32 mask)
{
    mVisibilityMask = mask;
}
//-----------------------------------------------------------------------
uint32 CompositionTargetPass::getVisibilityMask()
{
    return mVisibilityMask;
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setLodBias(float bias)
{
    mLodBias = bias;
}
//-----------------------------------------------------------------------
float CompositionTargetPass::getLodBias()
{
    return mLodBias;
}
//-----------------------------------------------------------------------
void CompositionTargetPass::setMaterialScheme(const String& schemeName)
{
	mMaterialScheme = schemeName;
}
//-----------------------------------------------------------------------
const String& CompositionTargetPass::getMaterialScheme(void) const
{
	return mMaterialScheme;
}
//-----------------------------------------------------------------------
CompositionPass *CompositionTargetPass::createPass()
{
    CompositionPass *t = new CompositionPass(this);
    mPasses.push_back(t);
    return t;
}
//-----------------------------------------------------------------------

void CompositionTargetPass::removePass(size_t index)
{
    assert (index < mPasses.size() && "Index out of bounds.");
    Passes::iterator i = mPasses.begin() + index;
    delete(*i);
    mPasses.erase(i);
}
//-----------------------------------------------------------------------

CompositionPass *CompositionTargetPass::getPass(size_t index)
{
    assert (index < mPasses.size() && "Index out of bounds.");
    return mPasses[index];
}
//-----------------------------------------------------------------------

size_t CompositionTargetPass::getNumPasses()
{
    return mPasses.size();
}
//-----------------------------------------------------------------------
void CompositionTargetPass::removeAllPasses()
{
    Passes::iterator i, iend;
    iend = mPasses.end();
    for (i = mPasses.begin(); i != iend; ++i)
    {
        delete(*i);
    }
    mPasses.clear();
}
//-----------------------------------------------------------------------
CompositionTargetPass::PassIterator CompositionTargetPass::getPassIterator(void)
{
    return PassIterator(mPasses.begin(), mPasses.end());
}

//-----------------------------------------------------------------------
CompositionTechnique *CompositionTargetPass::getParent()
{
    return mParent;
}

//-----------------------------------------------------------------------
bool CompositionTargetPass::_isSupported(void)
{
    // A target pass is supported if all passes are supported

    PassIterator passi = getPassIterator();
    while (passi.hasMoreElements())
    {
        CompositionPass* pass = passi.getNext();
        if (!pass->_isSupported())
        {
            return false;
        }
    }

    return true;
}

}
