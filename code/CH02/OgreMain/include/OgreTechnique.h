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
#ifndef __Technique_H__
#define __Technique_H__

#include "OgrePrerequisites.h"
#include "OgreIteratorWrappers.h"
#include "OgreBlendMode.h"
#include "OgreCommon.h"
#include "OgrePass.h"

namespace Ogre {
    /** Class representing an approach to rendering this particular Material. 
    @remarks
        Ogre will attempt to use the best technique supported by the active hardware, 
        unless you specifically request a lower detail technique (say for distant
        rendering).
    */
    class _OgreExport Technique
    {
    protected:
        // illumination pass state type
        enum IlluminationPassesState
        {
            IPS_COMPILE_DISABLED = -1,
            IPS_NOT_COMPILED = 0,
            IPS_COMPILED = 1
        };

        typedef std::vector<Pass*> Passes;
        /// List of primary passes
        Passes mPasses;
        /// List of derived passes, categorised into IlluminationStage (ordered)
        IlluminationPassList mIlluminationPasses;
        Material* mParent; // raw pointer since we don't want child to stop parent's destruction
        bool mIsSupported;
        IlluminationPassesState mIlluminationPassesCompilationPhase;
		/// LOD level
        unsigned short mLodIndex;
		/** Scheme index, derived from scheme name but the names are held on
			MaterialManager, for speed an index is used here.
		*/
		unsigned short mSchemeIndex;
        String mName; // optional name for the technique

        /// Internal method for clearing illumination pass list
        void clearIlluminationPasses(void);
    public:
        /// Constructor
        Technique(Material* parent);
        /// Copy constructor
        Technique(Material* parent, const Technique& oth);
        ~Technique();
        /** Indicates if this technique is supported by the current graphics card.
        @remarks
            This will only be correct after the Technique has been compiled, which is
            usually done from Material::compile.
        */
        bool isSupported(void) const;
        /** Internal compilation method; see Material::compile. */
        void _compile(bool autoManageTextureUnits);
        /** Internal method for splitting the passes into illumination passes. */        
        void _compileIlluminationPasses(void);


        /** Creates a new Pass for this Technique.
        @remarks
            A Pass is a single rendering pass, ie a single draw of the given material.
            Note that if you create a pass without a fragment program, during compilation of the
            material the pass may be split into multiple passes if the graphics card cannot
            handle the number of texture units requested. For passes with fragment programs, however, 
            the number of passes you create will never be altered, so you have to make sure 
            that you create an alternative fallback Technique for if a card does not have 
            enough facilities for what you're asking for.
        */
        Pass* createPass(void);
        /** Retrieves the Pass with the given index. */
        Pass* getPass(unsigned short index);
        /** Retrieves the Pass matching name.
            Returns 0 if name match is not found.
        */
        Pass* getPass(const String& name);
        /** Retrieves the number of passes. */
        unsigned short getNumPasses(void) const;
        /** Removes the Pass with the given index. */
        void removePass(unsigned short index);
        /** Removes all Passes from this Technique. */
        void removeAllPasses(void);
        /** Move a pass from source index to destination index.
            If successful then returns true.
        */
        bool movePass(const unsigned short sourceIndex, const unsigned short destinationIndex);
        typedef VectorIterator<Passes> PassIterator;
        /** Gets an iterator over the passes in this Technique. */
        const PassIterator getPassIterator(void);
        typedef VectorIterator<IlluminationPassList> IlluminationPassIterator;
        /** Gets an iterator over the illumination-stage categorised passes. */
        const IlluminationPassIterator getIlluminationPassIterator(void);
        /// Gets the parent Material
        Material* getParent(void) const { return mParent; }

        /** Overloaded operator to copy on Technique to another. */
        Technique& operator=(const Technique& rhs);

		/// Gets the resource group of the ultimate parent Material
		const String& getResourceGroup(void) const;

		/** Returns true if this Technique involves transparency. 
		@remarks
			This basically boils down to whether the first pass
			has a scene blending factor. Even if the other passes 
			do not, the base colour, including parts of the original 
			scene, may be used for blending, therefore we have to treat
			the whole Technique as transparent.
		*/
		bool isTransparent(void) const;

        /** Internal load method, derived from call to Material::load. */
        void _load(void);
        /** Internal unload method, derived from call to Material::unload. */
        void _unload(void);

        // Is this loaded?
        bool isLoaded(void) const;

        /** Tells the technique that it needs recompilation. */
        void _notifyNeedsRecompile(void);


        // -------------------------------------------------------------------------------
        // The following methods are to make migration from previous versions simpler
        // and to make code easier to write when dealing with simple materials
        // They set the properties which have been moved to Pass for all Techniques and all Passes

        /** Sets the point size properties for every Pass in this Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setPointSize
        */
        void setPointSize(Real ps);

        /** Sets the ambient colour reflectance properties for every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setAmbient
        */
        void setAmbient(Real red, Real green, Real blue);

        /** Sets the ambient colour reflectance properties for every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setAmbient
        */
        void setAmbient(const ColourValue& ambient);

        /** Sets the diffuse colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDiffuse
        */
        void setDiffuse(Real red, Real green, Real blue, Real alpha);

        /** Sets the diffuse colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDiffuse
        */
        void setDiffuse(const ColourValue& diffuse);

        /** Sets the specular colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSpecular
        */
        void setSpecular(Real red, Real green, Real blue, Real alpha);

        /** Sets the specular colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSpecular
        */
        void setSpecular(const ColourValue& specular);

        /** Sets the shininess properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setShininess
        */
        void setShininess(Real val);

        /** Sets the amount of self-illumination of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSelfIllumination
        */
        void setSelfIllumination(Real red, Real green, Real blue);

        /** Sets the amount of self-illumination of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSelfIllumination
        */
        void setSelfIllumination(const ColourValue& selfIllum);

		/** Sets whether or not each Pass renders with depth-buffer checking on or not.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthCheckEnabled
        */
        void setDepthCheckEnabled(bool enabled);

        /** Sets whether or not each Pass renders with depth-buffer writing on or not.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthWriteEnabled
        */
        void setDepthWriteEnabled(bool enabled);

        /** Sets the function used to compare depth values when depth checking is on.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthFunction
        */
        void setDepthFunction( CompareFunction func );

		/** Sets whether or not colour buffer writing is enabled for each Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setColourWriteEnabled
		*/
		void setColourWriteEnabled(bool enabled);

        /** Sets the culling mode for each pass  based on the 'vertex winding'.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setCullingMode
        */
        void setCullingMode( CullingMode mode );

        /** Sets the manual culling mode, performed by CPU rather than hardware.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setManualCullingMode
        */
        void setManualCullingMode( ManualCullingMode mode );

        /** Sets whether or not dynamic lighting is enabled for every Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setLightingEnabled
        */
        void setLightingEnabled(bool enabled);

        /** Sets the type of light shading required
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setShadingMode
        */
        void setShadingMode( ShadeOptions mode );

        /** Sets the fogging mode applied to each pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setFog
        */
        void setFog(
            bool overrideScene,
            FogMode mode = FOG_NONE,
            const ColourValue& colour = ColourValue::White,
            Real expDensity = 0.001, Real linearStart = 0.0, Real linearEnd = 1.0 );

        /** Sets the depth bias to be used for each Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthBias
        */
        void setDepthBias(ushort bias);

        /** Set texture filtering for every texture unit in every Pass
        @note
            This property actually exists on the TextureUnitState class
            For simplicity, this method allows you to set these properties for 
            every current TeextureUnitState, If you need more precision, retrieve the  
            Pass and TextureUnitState instances and set the property there.
        @see TextureUnitState::setTextureFiltering
        */
        void setTextureFiltering(TextureFilterOptions filterType);
        /** Sets the anisotropy level to be used for all textures.
        @note
            This property has been moved to the TextureUnitState class, which is accessible via the 
            Technique and Pass. For simplicity, this method allows you to set these properties for 
            every current TeextureUnitState, If you need more precision, retrieve the Technique, 
            Pass and TextureUnitState instances and set the property there.
        @see TextureUnitState::setTextureAnisotropy
        */
        void setTextureAnisotropy(unsigned int maxAniso);

        /** Sets the kind of blending every pass has with the existing contents of the scene.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSceneBlending
        */
        void setSceneBlending( const SceneBlendType sbt );

        /** Allows very fine control of blending every Pass with the existing contents of the scene.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSceneBlending
        */
        void setSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor);

        /** Assigns a level-of-detail (LOD) index to this Technique.
        @remarks
            As noted previously, as well as providing fallback support for various
            graphics cards, multiple Technique objects can also be used to implement
            material LOD, where the detail of the material diminishes with distance to 
            save rendering power.
        @par
            By default, all Techniques have a LOD index of 0, which means they are the highest
            level of detail. Increasing LOD indexes are lower levels of detail. You can 
            assign more than one Technique to the same LOD index, meaning that the best 
            Technique that is supported at that LOD index is used. 
        @par
            You should not leave gaps in the LOD sequence; Ogre will allow you to do this
            and will continue to function as if the LODs were sequential, but it will 
            confuse matters.
        */
        void setLodIndex(unsigned short index);
        /** Gets the level-of-detail index assigned to this Technique. */
        unsigned short getLodIndex(void) const { return mLodIndex; }

		/** Set the 'scheme name' for this technique. 
		@remarks
			Material schemes are used to control top-level switching from one
			set of techniques to another. For example, you might use this to 
			define 'high', 'medium' and 'low' complexity levels on materials
		    to allow a user to pick a performance / quality ratio. Another
			possibility is that you have a fully HDR-enabled pipeline for top
			machines, rendering all objects using unclamped shaders, and a 
			simpler pipeline for others; this can be implemented using 
			schemes.
		@par
			Every technique belongs to a scheme - if you don't specify one, the
			Technique belongs to the scheme called 'Default', which is also the
			scheme used to render by default. The active scheme is set one of
			two ways - either by calling Viewport::setMaterialScheme, or
			by manually calling MaterialManager::setActiveScheme.
		*/
		void setSchemeName(const String& schemeName);
		/** Returns the scheme to which this technique is assigned.
			@see Technique::setSchemeName
		*/
		const String& getSchemeName(void) const;
		
		/// Internal method for getting the scheme index
		unsigned short _getSchemeIndex(void) const;
		   	
        /** Is depth writing going to occur on this technique? */
        bool isDepthWriteEnabled(void) const;

        /** Is depth checking going to occur on this technique? */
        bool isDepthCheckEnabled(void) const;

        /** Exists colour writing disabled pass on this technique? */
        bool hasColourWriteDisabled(void) const;

        /** Set the name of the technique.
        @remarks
        The use of technique name is optional.  Its usefull in material scripts where a material could inherit
        from another material and only want to modify a particalar technique.
        */
        void setName(const String& name);
        /// Gets the name of the technique
        const String& getName(void) const { return mName; }

        /** Applies texture names to Texture Unit State with matching texture name aliases.
            All passes, and Texture Unit States within the technique are checked.
            If matching texture aliases are found then true is returned.

        @param
            aliasList is a map container of texture alias, texture name pairs
        @param
            apply set true to apply the texture aliases else just test to see if texture alias matches are found.
        @return
            True if matching texture aliases were found in the Technique.
        */
        bool applyTextureAliases(const AliasTextureNamePairList& aliasList, const bool apply = true) const;


    };


}
#endif
