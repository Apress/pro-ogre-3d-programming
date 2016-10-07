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
#ifndef __MATERIALMANAGER_H__
#define __MATERIALMANAGER_H__

#include "OgrePrerequisites.h"

#include "OgreSingleton.h"
#include "OgreResourceManager.h"
#include "OgreMaterial.h"
#include "OgreStringVector.h"
#include "OgreMaterialSerializer.h"

namespace Ogre {


    /** Class for managing Material settings for Ogre.
        @remarks
            Materials control the eventual surface rendering properties of geometry. This class
            manages the library of materials, dealing with programmatic registrations and lookups,
            as well as loading predefined Material settings from scripts.
        @par
            When loaded from a script, a Material is in an 'unloaded' state and only stores the settings
            required. It does not at that stage load any textures. This is because the material settings may be
            loaded 'en masse' from bulk material script files, but only a subset will actually be required.
        @par
            Because this is a subclass of ResourceManager, any files loaded will be searched for in any path or
            archive added to the resource paths/archives. See ResourceManager for details.
        @par
            For a definition of the material script format, see the Tutorials/MaterialScript.html file.
    */
    class _OgreExport MaterialManager : public ResourceManager, public Singleton<MaterialManager>
    {
    protected:

        /// Default Texture filtering - minification
        FilterOptions mDefaultMinFilter;
        /// Default Texture filtering - magnification
        FilterOptions mDefaultMagFilter;
        /// Default Texture filtering - mipmapping
        FilterOptions mDefaultMipFilter;
        /// Default Texture anisotropy
        unsigned int mDefaultMaxAniso;

        /// Serializer
        MaterialSerializer mSerializer;
		/// Default settings
		MaterialPtr mDefaultSettings;
		/// Overridden from ResourceManager
		Resource* createImpl(const String& name, ResourceHandle handle, 
			const String& group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* params);

		/// Scheme name -> index. Never shrinks! Should be pretty static anyway
		typedef std::map<String, unsigned short> SchemeMap;
		/// List of material schemes
		SchemeMap mSchemes;
		/// Current material scheme
		String mActiveSchemeName;
		/// Current material scheme
		unsigned short mActiveSchemeIndex;

    public:
		/// Default material scheme
		static String DEFAULT_SCHEME_NAME;
		
        /** Default constructor.
        */
        MaterialManager();

        /** Default destructor.
        */
        virtual ~MaterialManager();

		/** Intialises the material manager, which also triggers it to 
		 * parse all available .program and .material scripts. */
		void initialise(void);
        
		/** @see ScriptLoader::parseScript
        */
        void parseScript(DataStreamPtr& stream, const String& groupName);


        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
            @note
                The default value is TFO_BILINEAR.
        */
        virtual void setDefaultTextureFiltering(TextureFilterOptions fo);
        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
        */
        virtual void setDefaultTextureFiltering(FilterType ftype, FilterOptions opts);
        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
        */
        virtual void setDefaultTextureFiltering(FilterOptions minFilter, FilterOptions magFilter, FilterOptions mipFilter);

		/// get the default texture filtering
        virtual FilterOptions getDefaultTextureFiltering(FilterType ftype) const;

        /** Sets the default anisotropy level to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
            @note
                The default value is 1 (no anisotropy).
        */
		void setDefaultAnisotropy(unsigned int maxAniso);
		/// get the default maxAnisotropy
		unsigned int getDefaultAnisotropy() const;

        /** Returns a pointer to the default Material settings.
            @remarks
                Ogre comes configured with a set of defaults for newly created
                materials. If you wish to have a different set of defaults,
                simply call this method and change the returned Material's
                settings. All materials created from then on will be configured
                with the new defaults you have specified.
            @par
                The default settings begin as a single Technique with a single, non-programmable Pass:
                <ul>
                <li>ambient = ColourValue::White</li>
                <li>diffuse = ColourValue::White</li>
                <li>specular = ColourValue::Black</li>
                <li>emmissive = ColourValue::Black</li>
                <li>shininess = 0</li>
                <li>No texture unit settings (& hence no textures)</li>
                <li>SourceBlendFactor = SBF_ONE</li>
                <li>DestBlendFactor = SBF_ZERO (no blend, replace with new
                  colour)</li>
                <li>Depth buffer checking on</li>
                <li>Depth buffer writing on</li>
                <li>Depth buffer comparison function = CMPF_LESS_EQUAL</li>
                <li>Colour buffer writing on for all channels</li>
                <li>Culling mode = CULL_CLOCKWISE</li>
                <li>Ambient lighting = ColourValue(0.5, 0.5, 0.5) (mid-grey)</li>
                <li>Dynamic lighting enabled</li>
                <li>Gourad shading mode</li>
                <li>Bilinear texture filtering</li>
                </ul>
        */
		virtual MaterialPtr getDefaultSettings(void) const { return mDefaultSettings; }

		/** Internal method - returns index for a given material scheme name.
		@see Technique::setSchemeName
		*/
		virtual unsigned short _getSchemeIndex(const String& name);
		/** Internal method - returns name for a given material scheme index.
		@see Technique::setSchemeName
		*/
		virtual const String& _getSchemeName(unsigned short index);
		/** Internal method - returns the active scheme index.
		@see Technique::setSchemeName
		*/
		virtual unsigned short _getActiveSchemeIndex(void) const;

		/** Returns the name of the active material scheme. 
		@see Technique::setSchemeName
		*/
		virtual const String& getActiveScheme(void) const;
		
		/** Sets the name of the active material scheme. 
		@see Technique::setSchemeName
		*/
		virtual void setActiveScheme(const String& schemeName);

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static MaterialManager& getSingleton(void);
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static MaterialManager* getSingletonPtr(void);

    };

}

#endif
