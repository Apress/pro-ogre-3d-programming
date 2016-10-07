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

#include "OgreMaterial.h"

#include "OgreSceneManagerEnumerator.h"
#include "OgreMaterialManager.h"
#include "OgreIteratorWrappers.h"
#include "OgreTechnique.h"
#include "OgreLogManager.h"
#include "OgreException.h"
#include "OgreStringConverter.h"

namespace Ogre {

    //-----------------------------------------------------------------------
	Material::Material(ResourceManager* creator, const String& name, ResourceHandle handle,
		const String& group, bool isManual, ManualResourceLoader* loader)
		:Resource(creator, name, handle, group, isManual, loader),
         mReceiveShadows(true),
         mTransparencyCastsShadows(false),
         mCompilationRequired(true)
    {
		// Override isManual, not applicable for Material (we always want to call loadImpl)
		if(isManual)
		{
			mIsManual = false;
			LogManager::getSingleton().logMessage("Material " + name + 
				" was requested with isManual=true, but this is not applicable " 
				"for materials; the flag has been reset to false");
		}

		mLodDistances.push_back(0.0f);

		applyDefaults();

		/* For consistency with StringInterface, but we don't add any parameters here
		That's because the Resource implementation of StringInterface is to
		list all the options that need to be set before loading, of which 
		we have none as such. Full details can be set through scripts.
		*/ 
		createParamDictionary("Material");
    }
    //-----------------------------------------------------------------------
    Material::~Material()
    {
        removeAllTechniques();
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload(); 
    }
    //-----------------------------------------------------------------------
    Material& Material::operator=(const Material& rhs)
    {
	    mName = rhs.mName;
		mGroup = rhs.mGroup;
		mCreator = rhs.mCreator;
		mIsManual = rhs.mIsManual;
		mLoader = rhs.mLoader;
	    mHandle = rhs.mHandle;
        mSize = rhs.mSize;
        mReceiveShadows = rhs.mReceiveShadows;
        mTransparencyCastsShadows = rhs.mTransparencyCastsShadows;

        mIsLoaded = rhs.mIsLoaded;

        // Copy Techniques
        this->removeAllTechniques();
        Techniques::const_iterator i, iend;
        iend = rhs.mTechniques.end();
        for(i = rhs.mTechniques.begin(); i != iend; ++i)
        {
            Technique* t = this->createTechnique();
            *t = *(*i);
            if ((*i)->isSupported())
            {
				insertSupportedTechnique(t);
            }
        }

		// Also copy LOD information
		mLodDistances = rhs.mLodDistances;
        mCompilationRequired = rhs.mCompilationRequired;
        // illumination passes are not compiled right away so
        // mIsLoaded state should still be the same as the original material
        assert(mIsLoaded == rhs.mIsLoaded);

	    return *this;
    }


    //-----------------------------------------------------------------------
    void Material::loadImpl(void)
    {
		// compile if required
        if (mCompilationRequired)
            compile();

        // Load all supported techniques
        Techniques::iterator i, iend;
        iend = mSupportedTechniques.end();
        for (i = mSupportedTechniques.begin(); i != iend; ++i)
        {
            (*i)->_load();
        }

    }
    //-----------------------------------------------------------------------
    void Material::unloadImpl(void)
    {
        // Unload all supported techniques
        Techniques::iterator i, iend;
        iend = mSupportedTechniques.end();
        for (i = mSupportedTechniques.begin(); i != iend; ++i)
        {
            (*i)->_unload();
        }
    }
    //-----------------------------------------------------------------------
    MaterialPtr Material::clone(const String& newName, bool changeGroup, 
		const String& newGroup) const
    {
		MaterialPtr newMat;
		if (changeGroup)
		{
			newMat = MaterialManager::getSingleton().create(newName, newGroup);
		}
		else
		{
			newMat = MaterialManager::getSingleton().create(newName, mGroup);
		}
        

        // Keep handle (see below, copy overrides everything)
        ResourceHandle newHandle = newMat->getHandle();
        // Assign values from this
        *newMat = *this;
		// Restore new group if required, will have been overridden by operator
		if (changeGroup)
		{
			newMat->mGroup = newGroup;
		}
		
        // Correct the name & handle, they get copied too
        newMat->mName = newName;
        newMat->mHandle = newHandle;

        return newMat;



    }
    //-----------------------------------------------------------------------
    void Material::copyDetailsTo(MaterialPtr& mat) const
    {
        // Keep handle (see below, copy overrides everything)
        ResourceHandle savedHandle = mat->mHandle;
        String savedName = mat->mName;
        String savedGroup = mat->mGroup;
		ManualResourceLoader* savedLoader = mat->mLoader;
		bool savedManual = mat->mIsManual;
        // Assign values from this
        *mat = *this;
        // Correct the name & handle, they get copied too
        mat->mName = savedName;
        mat->mHandle = savedHandle;
        mat->mGroup = savedGroup;
		mat->mIsManual = savedManual;
		mat->mLoader = savedLoader;

    }
    //-----------------------------------------------------------------------
    void Material::applyDefaults(void)
    {
		MaterialPtr defaults = MaterialManager::getSingleton().getDefaultSettings();

		if (!defaults.isNull())
		{
            // save name & handle
            String savedName = mName;
            String savedGroup = mGroup;
            ResourceHandle savedHandle = mHandle;
			ManualResourceLoader *savedLoader = mLoader;
			bool savedManual = mIsManual;
			*this = *defaults;
            // restore name & handle
            mName = savedName;
            mHandle = savedHandle;
            mGroup = savedGroup;
			mLoader = savedLoader;
			mIsManual = savedManual;
		}
        mCompilationRequired = true;

    }
    //-----------------------------------------------------------------------
    Technique* Material::createTechnique(void)
    {
        Technique *t = new Technique(this);
        mTechniques.push_back(t);
        mCompilationRequired = true;
        return t;
    }
    //-----------------------------------------------------------------------
    Technique* Material::getTechnique(unsigned short index)
    {
        assert (index < mTechniques.size() && "Index out of bounds.");
        return mTechniques[index];
    }
    //-----------------------------------------------------------------------
    Technique* Material::getTechnique(const String& name)
    {
        Techniques::iterator i    = mTechniques.begin();
        Techniques::iterator iend = mTechniques.end();
        Technique* foundTechnique = 0;

        // iterate through techniques to find a match
        while (i != iend)
        {
            if ( (*i)->getName() == name )
            {
                foundTechnique = (*i);
                break;
            }
            ++i;
        }

        return foundTechnique;
    }
    //-----------------------------------------------------------------------	
    unsigned short Material::getNumTechniques(void) const
    {
		return static_cast<unsigned short>(mTechniques.size());
    }
	//-----------------------------------------------------------------------
    Technique* Material::getSupportedTechnique(unsigned short index)
    {
        assert (index < mSupportedTechniques.size() && "Index out of bounds.");
        return mSupportedTechniques[index];
    }
    //-----------------------------------------------------------------------	
    unsigned short Material::getNumSupportedTechniques(void) const
    {
		return static_cast<unsigned short>(mSupportedTechniques.size());
    }
	//-----------------------------------------------------------------------
	unsigned short Material::getNumLodLevels(unsigned short schemeIndex) const
	{
		BestTechniquesBySchemeList::const_iterator i = 
			mBestTechniquesBySchemeList.find(schemeIndex);
		if (i != mBestTechniquesBySchemeList.end())
		{
			return static_cast<unsigned short>(i->second->size());
		}
		else
		{
			return 0;
		}
	}
	//-----------------------------------------------------------------------
	unsigned short Material::getNumLodLevels(const String& schemeName) const
	{
		return getNumLodLevels(
			MaterialManager::getSingleton()._getSchemeIndex(schemeName));
	}
	//-----------------------------------------------------------------------
	void Material::insertSupportedTechnique(Technique* t)
	{
		mSupportedTechniques.push_back(t);
		// get scheme
		unsigned short schemeIndex = t->_getSchemeIndex();
		BestTechniquesBySchemeList::iterator i =
			mBestTechniquesBySchemeList.find(schemeIndex);
		LodTechniques* lodtechs = 0;
		if (i == mBestTechniquesBySchemeList.end())
		{
			lodtechs = new LodTechniques();
			mBestTechniquesBySchemeList[schemeIndex] = lodtechs;
		}
		else
		{
			lodtechs = i->second;
		}

		// Insert won't replace if supported technique for this scheme/lod is
		// already there, which is what we want
		lodtechs->insert(LodTechniques::value_type(t->getLodIndex(), t));

	}
	//-----------------------------------------------------------------------------
    Technique* Material::getBestTechnique(unsigned short lodIndex)
    {
        if (mSupportedTechniques.empty())
        {
            return NULL;
        }
        else
        {
			Technique* ret = 0;
			// get scheme
			BestTechniquesBySchemeList::iterator si = 
				mBestTechniquesBySchemeList.find(
				MaterialManager::getSingleton()._getActiveSchemeIndex());
			// scheme not found?
			if (si == mBestTechniquesBySchemeList.end())
			{
				// get the first item, will be 0 (the default) if default
				// scheme techniques exist, otherwise the earliest defined
				si = mBestTechniquesBySchemeList.begin();
			}

			// get LOD
			LodTechniques::iterator li = si->second->find(lodIndex);
			// LOD not found? 
			if (li == si->second->end())
			{
				// Use the next LOD level up
				for (LodTechniques::reverse_iterator rli = si->second->rbegin(); 
					rli != si->second->rend(); ++rli)
				{
					if (rli->second->getLodIndex() < lodIndex)
					{
						ret = rli->second;
						break;
					}

				}
				if (!ret)
				{
					// shouldn't ever hit this really, unless user defines no LOD 0
					// pick the first LOD we have (must be at least one to have a scheme entry)
					ret = si->second->begin()->second;
				}

			}
			else
			{
				// LOD found
				ret = li->second;
			}

			return ret;

        }
    }
    //-----------------------------------------------------------------------
    void Material::removeTechnique(unsigned short index)
    {
        assert (index < mTechniques.size() && "Index out of bounds.");
        Techniques::iterator i = mTechniques.begin() + index;
        delete(*i);
        mTechniques.erase(i);
        mSupportedTechniques.clear();
		clearBestTechniqueList();
        mCompilationRequired = true;
    }
    //-----------------------------------------------------------------------
    void Material::removeAllTechniques(void)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            delete(*i);
        }
        mTechniques.clear();
        mSupportedTechniques.clear();
        clearBestTechniqueList();
        mCompilationRequired = true;
    }
    //-----------------------------------------------------------------------
    Material::TechniqueIterator Material::getTechniqueIterator(void) 
    {
        return TechniqueIterator(mTechniques.begin(), mTechniques.end());
    }
    //-----------------------------------------------------------------------
    Material::TechniqueIterator Material::getSupportedTechniqueIterator(void)
    {
        return TechniqueIterator(mSupportedTechniques.begin(), mSupportedTechniques.end());
    }
    //-----------------------------------------------------------------------
    bool Material::isTransparent(void) const
	{
		// Check each technique
		Techniques::const_iterator i, iend;
		iend = mTechniques.end();
		for (i = mTechniques.begin(); i != iend; ++i)
		{
			if ( (*i)->isTransparent() )
				return true;
		}
		return false;
	}
    //-----------------------------------------------------------------------
    void Material::compile(bool autoManageTextureUnits)
    {
        // Compile each technique, then add it to the list of supported techniques
        mSupportedTechniques.clear();
		clearBestTechniqueList();


        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->_compile(autoManageTextureUnits);
            if ( (*i)->isSupported() )
            {
				insertSupportedTechnique(*i);
            }
        }

        mCompilationRequired = false;

        // Did we find any?
        if (mSupportedTechniques.empty())
        {
            LogManager::getSingleton().logMessage(
                "Warning: material " + mName + " has no supportable Techniques on this "
                "hardware, it will be rendered blank.");
        }
    }
	//-----------------------------------------------------------------------
	void Material::clearBestTechniqueList(void)
	{
		for (BestTechniquesBySchemeList::iterator i = mBestTechniquesBySchemeList.begin();
			i != mBestTechniquesBySchemeList.end(); ++i)
		{
			delete i->second;
		}
		mBestTechniquesBySchemeList.clear();
	}
    //-----------------------------------------------------------------------
    void Material::setPointSize(Real ps)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setPointSize(ps);
        }

    }
    //-----------------------------------------------------------------------
    void Material::setAmbient(Real red, Real green, Real blue)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setAmbient(red, green, blue);
        }

    }
    //-----------------------------------------------------------------------
    void Material::setAmbient(const ColourValue& ambient)
    {
        setAmbient(ambient.r, ambient.g, ambient.b);
    }
    //-----------------------------------------------------------------------
    void Material::setDiffuse(Real red, Real green, Real blue, Real alpha)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setDiffuse(red, green, blue, alpha);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setDiffuse(const ColourValue& diffuse)
    {
        setDiffuse(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
    }
    //-----------------------------------------------------------------------
    void Material::setSpecular(Real red, Real green, Real blue, Real alpha)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setSpecular(red, green, blue, alpha);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setSpecular(const ColourValue& specular)
    {
        setSpecular(specular.r, specular.g, specular.b, specular.a);
    }
    //-----------------------------------------------------------------------
    void Material::setShininess(Real val)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setShininess(val);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setSelfIllumination(Real red, Real green, Real blue)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setSelfIllumination(red, green, blue);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setSelfIllumination(const ColourValue& selfIllum)
    {
        setSelfIllumination(selfIllum.r, selfIllum.g, selfIllum.b);
    }
    //-----------------------------------------------------------------------
    void Material::setDepthCheckEnabled(bool enabled)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setDepthCheckEnabled(enabled);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setDepthWriteEnabled(bool enabled)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setDepthWriteEnabled(enabled);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setDepthFunction( CompareFunction func )
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setDepthFunction(func);
        }
    }
    //-----------------------------------------------------------------------
	void Material::setColourWriteEnabled(bool enabled)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setColourWriteEnabled(enabled);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setCullingMode( CullingMode mode )
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setCullingMode(mode);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setManualCullingMode( ManualCullingMode mode )
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setManualCullingMode(mode);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setLightingEnabled(bool enabled)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setLightingEnabled(enabled);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setShadingMode( ShadeOptions mode )
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setShadingMode(mode);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setFog(bool overrideScene, FogMode mode, const ColourValue& colour,
        Real expDensity, Real linearStart, Real linearEnd)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setFog(overrideScene, mode, colour, expDensity, linearStart, linearEnd);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setDepthBias(ushort bias)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setDepthBias(bias);
        }
    }
    //-----------------------------------------------------------------------
    void Material::setTextureFiltering(TextureFilterOptions filterType)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setTextureFiltering(filterType);
        }
    }
    // --------------------------------------------------------------------
    void Material::setTextureAnisotropy(int maxAniso)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setTextureAnisotropy(maxAniso);
        }
    }
    // --------------------------------------------------------------------
    void Material::setSceneBlending( const SceneBlendType sbt )
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setSceneBlending(sbt);
        }
    }
    // --------------------------------------------------------------------
    void Material::setSceneBlending( const SceneBlendFactor sourceFactor, 
        const SceneBlendFactor destFactor)
    {
        Techniques::iterator i, iend;
        iend = mTechniques.end();
        for (i = mTechniques.begin(); i != iend; ++i)
        {
            (*i)->setSceneBlending(sourceFactor, destFactor);
        }
    }
    // --------------------------------------------------------------------
    void Material::_notifyNeedsRecompile(void)
    {
        mCompilationRequired = true;
        // Also need to unload to ensure we loaded any new items
        unload();
    }
    // --------------------------------------------------------------------
    void Material::setLodLevels(const LodDistanceList& lodDistances)
    {
        // Square the distances for the internal list
		LodDistanceList::const_iterator i, iend;
		iend = lodDistances.end();
		// First, clear and add single zero entry
		mLodDistances.clear();
		mLodDistances.push_back(0.0f);
		for (i = lodDistances.begin(); i != iend; ++i)
		{
			mLodDistances.push_back((*i) * (*i));
		}
		
    }
    // --------------------------------------------------------------------
    unsigned short Material::getLodIndex(Real d) const
    {
        return getLodIndexSquaredDepth(d * d);
    }
    // --------------------------------------------------------------------
    unsigned short Material::getLodIndexSquaredDepth(Real squaredDistance) const
    {
		LodDistanceList::const_iterator i, iend;
		iend = mLodDistances.end();
		unsigned short index = 0;
		for (i = mLodDistances.begin(); i != iend; ++i, ++index)
		{
			if (*i > squaredDistance)
			{
				return index - 1;
			}
		}

		// If we fall all the way through, use the highest value
		return static_cast<ushort>(mLodDistances.size() - 1);
    }
    // --------------------------------------------------------------------
    Material::LodDistanceIterator Material::getLodDistanceIterator(void) const
    {
        return LodDistanceIterator(mLodDistances.begin(), mLodDistances.end());
    }

    //-----------------------------------------------------------------------
    bool Material::applyTextureAliases(const AliasTextureNamePairList& aliasList, const bool apply) const
    {
        // iterate through all techniques and apply texture aliases
		Techniques::const_iterator i, iend;
		iend = mTechniques.end();
        bool testResult = false;

		for (i = mTechniques.begin(); i != iend; ++i)
		{
            if ((*i)->applyTextureAliases(aliasList, apply))
                testResult = true;
		}

        return testResult;
    }
}
