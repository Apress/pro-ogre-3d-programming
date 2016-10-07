/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://ogre.sourceforge.net/

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
#ifndef __Pass_H__
#define __Pass_H__

#include "OgrePrerequisites.h"
#include "OgreGpuProgram.h"
#include "OgreColourValue.h"
#include "OgreBlendMode.h"
#include "OgreCommon.h"
#include "OgreLight.h"

namespace Ogre {
    /** Class defining a single pass of a Technique (of a Material), ie
        a single rendering call. 
    @remarks
        Rendering can be repeated with many passes for more complex effects.
        Each pass is either a fixed-function pass (meaning it does not use
        a vertex or fragment program) or a programmable pass (meaning it does
        use either a vertex and fragment program, or both). 
    @par
        Programmable passes are complex to define, because they require custom
        programs and you have to set all constant inputs to the programs (like
        the position of lights, any base material colours you wish to use etc), but
        they do give you much total flexibility over the algorithms used to render your
        pass, and you can create some effects which are impossible with a fixed-function pass.
        On the other hand, you can define a fixed-function pass in very little time, and
        you can use a range of fixed-function effects like environment mapping very
        easily, plus your pass will be more likely to be compatible with older hardware.
        There are pros and cons to both, just remember that if you use a programmable
        pass to create some great effects, allow more time for definition and testing.
    */
    class _OgreExport Pass
    {
    protected:
        Technique* mParent;
        unsigned short mIndex; // pass index
        String mName; // optional name for the pass
        uint32 mHash; // pass hash
        //-------------------------------------------------------------------------
        // Colour properties, only applicable in fixed-function passes
        ColourValue mAmbient;
        ColourValue mDiffuse;
        ColourValue mSpecular;    
        ColourValue mEmissive;
        Real mShininess;
        TrackVertexColourType mTracking;
        //-------------------------------------------------------------------------

        //-------------------------------------------------------------------------
        // Blending factors
        SceneBlendFactor mSourceBlendFactor;    
        SceneBlendFactor mDestBlendFactor;
        //-------------------------------------------------------------------------

        //-------------------------------------------------------------------------    
        // Depth buffer settings
        bool mDepthCheck;
        bool mDepthWrite;
        CompareFunction mDepthFunc;
        ushort mDepthBias;

        // Colour buffer settings
        bool mColourWrite;
		
		// Alpha reject settings
		CompareFunction mAlphaRejectFunc;
		unsigned char mAlphaRejectVal;
        //-------------------------------------------------------------------------    

        //-------------------------------------------------------------------------
        // Culling mode
        CullingMode mCullMode;
        ManualCullingMode mManualCullMode;
        //-------------------------------------------------------------------------

        /// Lighting enabled?
        bool mLightingEnabled;
        /// Max simultaneous lights
        unsigned short mMaxSimultaneousLights;
		/// Run this pass once per light?
		bool mIteratePerLight;
        // Should it only be run for a certain light type?
        bool mRunOnlyForOneLightType;
        Light::LightTypes mOnlyLightType;

        /// Shading options
        ShadeOptions mShadeOptions;
		/// Polygon mode
		PolygonMode mPolygonMode;

        //-------------------------------------------------------------------------    
        // Fog
        bool mFogOverride;
        FogMode mFogMode;
        ColourValue mFogColour;
        Real mFogStart;
        Real mFogEnd;
        Real mFogDensity;
        //-------------------------------------------------------------------------    

        /// Storage of texture unit states 
        typedef std::vector<TextureUnitState*> TextureUnitStates;
        TextureUnitStates mTextureUnitStates;    

		// Vertex program details
		GpuProgramUsage *mVertexProgramUsage;
        // Vertex program details
        GpuProgramUsage *mShadowCasterVertexProgramUsage;
        // Vertex program details
        GpuProgramUsage *mShadowReceiverVertexProgramUsage;
		// Fragment program details
		GpuProgramUsage *mFragmentProgramUsage;
		// Fragment program details
		GpuProgramUsage *mShadowReceiverFragmentProgramUsage;
        // Is this pass queued for deletion?
        bool mQueuedForDeletion;
        // number of pass iterations to perform
        size_t mPassIterationCount;
		// point size, applies when not using per-vertex point size
		Real mPointSize;
		Real mPointMinSize;
		Real mPointMaxSize;
		bool mPointSpritesEnabled;
		bool mPointAttenuationEnabled;
		// constant, linear, quadratic coeffs
		Real mPointAttenuationCoeffs[3];
	public:
		typedef std::set<Pass*> PassSet;
    protected:
		/// List of Passes whose hashes need recalculating
		static PassSet msDirtyHashList;
        /// The place where passes go to die
        static PassSet msPassGraveyard;
    public:
        /// Default constructor
		Pass(Technique* parent, unsigned short index);
        /// Copy constructor
        Pass(Technique* parent, unsigned short index, const Pass& oth );
        /// Operator = overload
        Pass& operator=(const Pass& oth);
        ~Pass();

        /// Returns true if this pass is programmable ie includes either a vertex or fragment program.
        bool isProgrammable(void) const { return mVertexProgramUsage || mFragmentProgramUsage; }
        /// Returns true if this pass uses a programmable vertex pipeline
        bool hasVertexProgram(void) const { return mVertexProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable fragment pipeline
        bool hasFragmentProgram(void) const { return mFragmentProgramUsage != NULL; }
        /// Returns true if this pass uses a shadow caster vertex program
        bool hasShadowCasterVertexProgram(void) const { return mShadowCasterVertexProgramUsage != NULL; }
        /// Returns true if this pass uses a shadow receiver vertex program
        bool hasShadowReceiverVertexProgram(void) const { return mShadowReceiverVertexProgramUsage != NULL; }
        /// Returns true if this pass uses a shadow receiver fragment program
        bool hasShadowReceiverFragmentProgram(void) const { return mShadowReceiverFragmentProgramUsage != NULL; }


        /// Gets the index of this Pass in the parent Technique
        unsigned short getIndex(void) const { return mIndex; }
        /* Set the name of the pass
        @remarks
        The name of the pass is optional.  Its usefull in material scripts where a material could inherit
        from another material and only want to modify a particalar pass.
        */
        void setName(const String& name);
        /// get the name of the pass
        const String& getName(void) const { return mName; }

        /** Sets the ambient colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much ambient light (directionless global light) is reflected. The default is full white, meaning
        objects are completely globally illuminated. Reduce this if you want to see diffuse or specular light
        effects, or change the blend of colours to make the object have a base colour other than white.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setAmbient(Real red, Real green, Real blue);

        /** Sets the ambient colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much ambient light (directionless global light) is reflected. The default is full white, meaning
        objects are completely globally illuminated. Reduce this if you want to see diffuse or specular light
        effects, or change the blend of colours to make the object have a base colour other than white.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */

        void setAmbient(const ColourValue& ambient);

        /** Sets the diffuse colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much diffuse light (light from instances of the Light class in the scene) is reflected. The default
        is full white, meaning objects reflect the maximum white light they can from Light objects.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setDiffuse(Real red, Real green, Real blue, Real alpha);

        /** Sets the diffuse colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much diffuse light (light from instances of the Light class in the scene) is reflected. The default
        is full white, meaning objects reflect the maximum white light they can from Light objects.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setDiffuse(const ColourValue& diffuse);

        /** Sets the specular colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much specular light (highlights from instances of the Light class in the scene) is reflected.
        The default is to reflect no specular light.
        @note
        The size of the specular highlights is determined by the separate 'shininess' property.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setSpecular(Real red, Real green, Real blue, Real alpha);

        /** Sets the specular colour reflectance properties of this pass.
        @remarks
        The base colour of a pass is determined by how much red, green and blue light is reflects
        (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines how
        much specular light (highlights from instances of the Light class in the scene) is reflected.
        The default is to reflect no specular light.
        @note
        The size of the specular highlights is determined by the separate 'shininess' property.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setSpecular(const ColourValue& specular);

        /** Sets the shininess of the pass, affecting the size of specular highlights.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setShininess(Real val);

        /** Sets the amount of self-illumination an object has.
        @remarks
        If an object is self-illuminating, it does not need external sources to light it, ambient or
        otherwise. It's like the object has it's own personal ambient light. This property is rarely useful since
        you can already specify per-pass ambient light, but is here for completeness.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setSelfIllumination(Real red, Real green, Real blue);

        /** Sets the amount of self-illumination an object has.
        @remarks
        If an object is self-illuminating, it does not need external sources to light it, ambient or
        otherwise. It's like the object has it's own personal ambient light. This property is rarely useful since
        you can already specify per-pass ambient light, but is here for completeness.
        @note
        This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
        or if this is a programmable pass.
        */
        void setSelfIllumination(const ColourValue& selfIllum);

        /** Sets which material properties follow the vertex colour
         */
        void setVertexColourTracking(TrackVertexColourType tracking);

        /** Gets the point size of the pass.
		@remarks
			This property determines what point size is used to render a point 
			list.
        */
        Real getPointSize(void) const;

		/** Sets the point size of this pass.
		@remarks
			This setting allows you to change the size of points when rendering 
			a point list, or a list of point sprites. The interpretation of this 
			command depends on the Pass::setPointSizeAttenuation option - if it 
			is off (the default), the point size is in screen pixels, if it is on, 
			it expressed as normalised screen coordinates (1.0 is the height of 
			the screen) when the point is at the origin.
		@note 
			Some drivers have an upper limit on the size of points they support 
			- this can even vary between APIs on the same card! Don't rely on 
			point sizes that cause the point sprites to get very large on screen, 
			since they may get clamped on some cards. Upper sizes can range from 
			64 to 256 pixels.
		*/
		void setPointSize(Real ps);
		
		/** Sets whether or not rendering points using OT_POINT_LIST will 
			render point sprites (textured quads) or plain points (dots).
		@param enabled True enables point sprites, false returns to normal
			point rendering.
		*/	
		void setPointSpritesEnabled(bool enabled);

		/** Returns whether point sprites are enabled when rendering a 
			point list.
		*/
		bool getPointSpritesEnabled(void) const;

		/** Sets how points are attenuated with distance.
		@remarks
			When performing point rendering or point sprite rendering,
			point size can be attenuated with distance. The equation for
			doing this is attenuation = 1 / (constant + linear * dist + quadratic * d^2).
		@par
			For example, to disable distance attenuation (constant screensize) 
			you would set constant to 1, and linear and quadratic to 0. A
			standard perspective attenuation would be 0, 1, 0 respectively.
		@note 
			The resulting size is clamped to the minimum and maximum point
			size.
		@param enabled Whether point attenuation is enabled
		@param constant, linear, quadratic Parameters to the attentuation 
			function defined above
		*/
		void setPointAttenuation(bool enabled, 
			Real constant = 0.0f, Real linear = 1.0f, Real quadratic = 0.0f);

		/** Returns whether points are attenuated with distance. */
		bool isPointAttenuationEnabled(void) const;

		/** Returns the constant coefficient of point attenuation. */
		Real getPointAttenuationConstant(void) const;
		/** Returns the linear coefficient of point attenuation. */
		Real getPointAttenuationLinear(void) const;
		/** Returns the quadratic coefficient of point attenuation. */
		Real getPointAttenuationQuadratic(void) const;

		/** Set the minimum point size, when point attenuation is in use. */
		void setPointMinSize(Real min);
		/** Get the minimum point size, when point attenuation is in use. */
		Real getPointMinSize(void) const;
		/** Set the maximum point size, when point attenuation is in use. 
		@remarks Setting this to 0 indicates the max size supported by the card.
		*/
		void setPointMaxSize(Real max);
		/** Get the maximum point size, when point attenuation is in use. 
		@remarks 0 indicates the max size supported by the card.
		*/
		Real getPointMaxSize(void) const;

		/** Gets the ambient colour reflectance of the pass.
        */
        const ColourValue& getAmbient(void) const;

        /** Gets the diffuse colour reflectance of the pass.
        */
        const ColourValue& getDiffuse(void) const;

        /** Gets the specular colour reflectance of the pass.
        */
        const ColourValue& getSpecular(void) const;

        /** Gets the self illumination colour of the pass.
        */
        const ColourValue& getSelfIllumination(void) const;

        /** Gets the 'shininess' property of the pass (affects specular highlights).
        */
        Real getShininess(void) const;
        
        /** Gets which material properties follow the vertex colour
         */
        TrackVertexColourType getVertexColourTracking(void) const;

        /** Inserts a new TextureUnitState object into the Pass.
        @remarks
        This unit is is added on top of all previous units. 
        */
        TextureUnitState* createTextureUnitState(void);
        /** Inserts a new TextureUnitState object into the Pass.
        @remarks
        This unit is is added on top of all previous units. 
        @param
        name The basic name of the texture e.g. brickwall.jpg, stonefloor.png
        @param
        texCoordSet The index of the texture coordinate set to use.
        @note 
        Applies to both fixed-function and programmable passes.
        */
        TextureUnitState* createTextureUnitState( const String& textureName, unsigned short texCoordSet = 0);
		/** Adds the passed in TextureUnitState, to the existing Pass. 
        @param
        state The Texture Unit State to be attached to this pass.  It must not be attached to another pass.
        @note
            Throws an exception if the TextureUnitState is attached to another Pass.*/
		void addTextureUnitState(TextureUnitState* state);
        /** Retrieves a pointer to a texture unit state so it may be modified.
        */
        TextureUnitState* getTextureUnitState(unsigned short index);
        /** Retrieves the Texture Unit State matching name.
            Returns 0 if name match is not found.
        */
        TextureUnitState* getTextureUnitState(const String& name);
		/** Retrieves a const pointer to a texture unit state.
		*/
		const TextureUnitState* getTextureUnitState(unsigned short index) const;
		/** Retrieves the Texture Unit State matching name.
		Returns 0 if name match is not found.
		*/
		const TextureUnitState* getTextureUnitState(const String& name) const;

        /**  Retrieve the index of the Texture Unit State in the pass.
        @param
        state The Texture Unit State this is attached to this pass.
        @note
            Throws an exception if the state is not attached to the pass.
        */
        unsigned short getTextureUnitStateIndex(const TextureUnitState* state);

        typedef VectorIterator<TextureUnitStates> TextureUnitStateIterator;
        /** Get an iterator over the TextureUnitStates contained in this Pass. */
        TextureUnitStateIterator getTextureUnitStateIterator(void);

		typedef ConstVectorIterator<TextureUnitStates> ConstTextureUnitStateIterator;
		/** Get an iterator over the TextureUnitStates contained in this Pass. */
		ConstTextureUnitStateIterator getTextureUnitStateIterator(void) const;

		/** Removes the indexed texture unit state from this pass.
        @remarks
            Note that removing a texture which is not the topmost will have a larger performance impact.
        */
        void removeTextureUnitState(unsigned short index);

        /** Removes all texture unit settings.
        */
        void removeAllTextureUnitStates(void);

        /** Returns the number of texture unit settings.
        */
        size_t getNumTextureUnitStates(void) const
        {
            return mTextureUnitStates.size();
        }

        /** Sets the kind of blending this pass has with the existing contents of the scene.
        @remarks
        Wheras the texture blending operations seen in the TextureUnitState class are concerned with
        blending between texture layers, this blending is about combining the output of the Pass
        as a whole with the existing contents of the rendering target. This blending therefore allows
        object transparency and other special effects. If all passes in a technique have a scene
        blend, then the whole technique is considered to be transparent.
        @par
        This method allows you to select one of a number of predefined blending types. If you require more
        control than this, use the alternative version of this method which allows you to specify source and
        destination blend factors.
        @note
        This method is applicable for both the fixed-function and programmable pipelines.
        @param
        sbt One of the predefined SceneBlendType blending types
        */
        void setSceneBlending( const SceneBlendType sbt );

        /** Allows very fine control of blending this Pass with the existing contents of the scene.
        @remarks
        Wheras the texture blending operations seen in the TextureUnitState class are concerned with
        blending between texture layers, this blending is about combining the output of the material
        as a whole with the existing contents of the rendering target. This blending therefore allows
        object transparency and other special effects.
        @par
        This version of the method allows complete control over the blending operation, by specifying the
        source and destination blending factors. The result of the blending operation is:
        <span align="center">
        final = (texture * sourceFactor) + (pixel * destFactor)
        </span>
        @par
        Each of the factors is specified as one of a number of options, as specified in the SceneBlendFactor
        enumerated type.
        @param
        sourceFactor The source factor in the above calculation, i.e. multiplied by the texture colour components.
        @param
        destFactor The destination factor in the above calculation, i.e. multiplied by the pixel colour components.
        @note
        This method is applicable for both the fixed-function and programmable pipelines.
        */
        void setSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor);

        /** Retrieves the source blending factor for the material (as set using Materiall::setSceneBlending).
        */
        SceneBlendFactor getSourceBlendFactor() const;

        /** Retrieves the destination blending factor for the material (as set using Materiall::setSceneBlending).
        */
        SceneBlendFactor getDestBlendFactor() const;

		/** Returns true if this pass has some element of transparency. */
		bool isTransparent(void) const;

		/** Sets whether or not this pass renders with depth-buffer checking on or not.
        @remarks
        If depth-buffer checking is on, whenever a pixel is about to be written to the frame buffer
        the depth buffer is checked to see if the pixel is in front of all other pixels written at that
        point. If not, the pixel is not written.
        @par
        If depth checking is off, pixels are written no matter what has been rendered before.
        Also see setDepthFunction for more advanced depth check configuration.
        @see
        setDepthFunction
        */
        void setDepthCheckEnabled(bool enabled);

        /** Returns whether or not this pass renders with depth-buffer checking on or not.
        @see
        setDepthCheckEnabled
        */
        bool getDepthCheckEnabled(void) const;

        /** Sets whether or not this pass renders with depth-buffer writing on or not.
        @remarks
        If depth-buffer writing is on, whenever a pixel is written to the frame buffer
        the depth buffer is updated with the depth value of that new pixel, thus affecting future
        rendering operations if future pixels are behind this one.
        @par
        If depth writing is off, pixels are written without updating the depth buffer Depth writing should
        normally be on but can be turned off when rendering static backgrounds or when rendering a collection
        of transparent objects at the end of a scene so that they overlap each other correctly.
        */
        void setDepthWriteEnabled(bool enabled);

        /** Returns whether or not this pass renders with depth-buffer writing on or not.
        @see
        setDepthWriteEnabled
        */
        bool getDepthWriteEnabled(void) const;

        /** Sets the function used to compare depth values when depth checking is on.
        @remarks
        If depth checking is enabled (see setDepthCheckEnabled) a comparison occurs between the depth
        value of the pixel to be written and the current contents of the buffer. This comparison is
        normally CMPF_LESS_EQUAL, i.e. the pixel is written if it is closer (or at the same distance)
        than the current contents. If you wish you can change this comparison using this method.
        */
        void setDepthFunction( CompareFunction func );
        /** Returns the function used to compare depth values when depth checking is on.
        @see
        setDepthFunction
        */
        CompareFunction getDepthFunction(void) const;

		/** Sets whether or not colour buffer writing is enabled for this Pass.
		@remarks
			For some effects, you might wish to turn off the colour write operation
			when rendering geometry; this means that only the depth buffer will be
			updated (provided you have depth buffer writing enabled, which you 
			probably will do, although you may wish to only update the stencil
			buffer for example - stencil buffer state is managed at the RenderSystem
			level only, not the Material since you are likely to want to manage it 
			at a higher level).
		*/
		void setColourWriteEnabled(bool enabled);
		/** Determines if colour buffer writing is enabled for this pass. */
		bool getColourWriteEnabled(void) const;

        /** Sets the culling mode for this pass  based on the 'vertex winding'.
        @remarks
        A typical way for the rendering engine to cull triangles is based on the 'vertex winding' of
        triangles. Vertex winding refers to the direction in which the vertices are passed or indexed
        to in the rendering operation as viewed from the camera, and will wither be clockwise or
        anticlockwise (that's 'counterclockwise' for you Americans out there ;) The default is
        CULL_CLOCKWISE i.e. that only triangles whose vertices are passed/indexed in anticlockwise order
        are rendered - this is a common approach and is used in 3D studio models for example. You can
        alter this culling mode if you wish but it is not advised unless you know what you are doing.
        @par
        You may wish to use the CULL_NONE option for mesh data that you cull yourself where the vertex
        winding is uncertain.
        */
        void setCullingMode( CullingMode mode );

        /** Returns the culling mode for geometry rendered with this pass. See setCullingMode for more information.
        */
        CullingMode getCullingMode(void) const;

        /** Sets the manual culling mode, performed by CPU rather than hardware.
        @pemarks
        In some situations you want to use manual culling of triangles rather than sending the
        triangles to the hardware and letting it cull them. This setting only takes effect on SceneManager's
        that use it (since it is best used on large groups of planar world geometry rather than on movable
        geometry since this would be expensive), but if used can cull geometry before it is sent to the
        hardware.
        @note
        The default for this setting is MANUAL_CULL_BACK.
        @param
        mode The mode to use - see enum ManualCullingMode for details

        */
        void setManualCullingMode( ManualCullingMode mode );

        /** Retrieves the manual culling mode for this pass
        @see
        setManualCullingMode
        */
        ManualCullingMode getManualCullingMode(void) const;

        /** Sets whether or not dynamic lighting is enabled.
        @param
        enabled
        If true, dynamic lighting is performed on geometry with normals supplied, geometry without
        normals will not be displayed.
        @par
        If false, no lighting is applied and all geometry will be full brightness.
        */
        void setLightingEnabled(bool enabled);

        /** Returns whether or not dynamic lighting is enabled.
        */
        bool getLightingEnabled(void) const;

        /** Sets the maximum number of lights to be used by this pass. 
        @remarks
            During rendering, if lighting is enabled (or if the pass uses an automatic
            program parameter based on a light) the engine will request the nearest lights 
            to the object being rendered in order to work out which ones to use. This
            parameter sets the limit on the number of lights which should apply to objects 
            rendered with this pass. 
        */
        void setMaxSimultaneousLights(unsigned short maxLights);
        /** Gets the maximum number of lights to be used by this pass. */
        unsigned short getMaxSimultaneousLights(void) const;

        /** Sets the type of light shading required
        @note
        The default shading method is Gouraud shading.
        */
        void setShadingMode( ShadeOptions mode );

        /** Returns the type of light shading to be used.
        */
        ShadeOptions getShadingMode(void) const;

		/** Sets the type of polygon rendering required
		@note
		The default shading method is Solid 
		*/
		void setPolygonMode( PolygonMode mode );

		/** Returns the type of light shading to be used.
		*/
		PolygonMode getPolygonMode(void) const;

        /** Sets the fogging mode applied to this pass.
        @remarks
        Fogging is an effect that is applied as polys are rendered. Sometimes, you want
        fog to be applied to an entire scene. Other times, you want it to be applied to a few
        polygons only. This pass-level specification of fog parameters lets you easily manage
        both.
        @par
        The SceneManager class also has a setFog method which applies scene-level fog. This method
        lets you change the fog behaviour for this pass compared to the standard scene-level fog.
        @param
        overrideScene If true, you authorise this pass to override the scene's fog params with it's own settings.
        If you specify false, so other parameters are necessary, and this is the default behaviour for passs.
        @param
        mode Only applicable if overrideScene is true. You can disable fog which is turned on for the
        rest of the scene by specifying FOG_NONE. Otherwise, set a pass-specific fog mode as
        defined in the enum FogMode.
        @param
        colour The colour of the fog. Either set this to the same as your viewport background colour,
        or to blend in with a skydome or skybox.
        @param
        expDensity The density of the fog in FOG_EXP or FOG_EXP2 mode, as a value between 0 and 1. 
        The default is 0.001.
        @param
        linearStart Distance in world units at which linear fog starts to encroach. 
        Only applicable if mode is FOG_LINEAR.
        @param
        linearEnd Distance in world units at which linear fog becomes completely opaque.
        Only applicable if mode is FOG_LINEAR.
        */
        void setFog(
            bool overrideScene,
            FogMode mode = FOG_NONE,
            const ColourValue& colour = ColourValue::White,
            Real expDensity = 0.001, Real linearStart = 0.0, Real linearEnd = 1.0 );

        /** Returns true if this pass is to override the scene fog settings.
        */
        bool getFogOverride(void) const;

        /** Returns the fog mode for this pass.
        @note
        Only valid if getFogOverride is true.
        */
        FogMode getFogMode(void) const;

        /** Returns the fog colour for the scene.
        */
        const ColourValue& getFogColour(void) const;

        /** Returns the fog start distance for this pass.
        @note
        Only valid if getFogOverride is true.
        */
        Real getFogStart(void) const;

        /** Returns the fog end distance for this pass.
        @note
        Only valid if getFogOverride is true.
        */
        Real getFogEnd(void) const;

        /** Returns the fog density for this pass.
        @note
        Only valid if getFogOverride is true.
        */
        Real getFogDensity(void) const;

        /** Sets the depth bias to be used for this material.
        @remarks
        When polygons are coplanar, you can get problems with 'depth fighting' where
        the pixels from the two polys compete for the same screen pixel. This is particularly
        a problem for decals (polys attached to another surface to represent details such as
        bulletholes etc.).
        @par
        A way to combat this problem is to use a depth bias to adjust the depth buffer value
        used for the decal such that it is slightly higher than the true value, ensuring that
        the decal appears on top.
        @param bias The bias value, should be between 0 and 16.
        */
        void setDepthBias(ushort bias);

        /** Retrieves the depth bias value as set by setDepthValue. */
        ushort getDepthBias(void) const;

        /** Sets the way the pass will have use alpha to totally reject pixels from the pipeline.
        @remarks
			The default is CMPF_ALWAYS_PASS i.e. alpha is not used to reject pixels.
        @param func The comparison which must pass for the pixel to be written.
        @param value 1 byte value against which alpha values will be tested(0-255)
        @note
			This option applies in both the fixed function and the programmable pipeline.
        */
        void setAlphaRejectSettings(CompareFunction func, unsigned char value);

		/** Sets the alpha reject function. See setAlphaRejectSettings for more information.
		*/
		void setAlphaRejectFunction(CompareFunction func);

		/** Gets the alpha reject value. See setAlphaRejectSettings for more information.
		*/
		void setAlphaRejectValue(unsigned char val);

		/** Gets the alpha reject function. See setAlphaRejectSettings for more information.
        */
		CompareFunction getAlphaRejectFunction(void) const { return mAlphaRejectFunc; }

        /** Gets the alpha reject value. See setAlphaRejectSettings for more information.
        */
		unsigned char getAlphaRejectValue(void) const { return mAlphaRejectVal; }
        /** Sets whether or not this pass should iterate per light which
		    can affect the object being rendered.
		@remarks
			The default behaviour for a pass (when this option is 'false'), is 
			for a pass to be rendered only once (or the number of times set in 
			setPassIterationCount), with all the lights which could
			affect this object set at the same time (up to the maximum lights
			allowed in the render system, which is typically 8). 
		@par
			Setting this option to 'true' changes this behaviour, such that 
			instead of trying to issue render this pass once per object, it
			is run <b>per light</b> which can affect this object, the number of
			times set in setPassIterationCount (default is once). In
			this case, only light index 0 is ever used, and is a different light
			every time the pass is issued, up to the total number of lights
			which is affecting this object. This has 2 advantages:
			<ul><li>There is no limit on the number of lights which can be 
			supported</li>
			<li>It's easier to write vertex / fragment programs for this because
			a single program can be used for any number of lights</li>
			</ul>
			However, this technique is a lot more expensive, and typically you
			will want an additional ambient pass, because if no lights are 
			affecting the object it will not be rendered at all, which will look
			odd even if ambient light is zero (imagine if there are lit objects
			behind it - the objects silhouette would not show up). Therefore,
			use this option with care, and you would be well advised to provide
			a less expensive fallback technique for use in the distance.
		@note
			The number of times this pass runs is still limited by the maximum
			number of lights allowed as set in setMaxSimultaneousLights, so
			you will never get more passes than this.
        @param enabled Whether this feature is enabled
        @param onlyForOneLightType If true, the pass will only be run for a single type
            of light, other light types will be ignored.
        @param lightType The single light type which will be considered for this pass
		*/
        void setIteratePerLight(bool enabled, 
            bool onlyForOneLightType = true, Light::LightTypes lightType = Light::LT_POINT);

        /** Does this pass run once for every light in range? */
		bool getIteratePerLight(void) const { return mIteratePerLight; }
        /** Does this pass run only for a single light type (if getIteratePerLight is true). */
        bool getRunOnlyForOneLightType(void) const { return mRunOnlyForOneLightType; }
        /** Gets the single light type this pass runs for if  getIteratePerLight and 
            getRunOnlyForOneLightType are both true. */
        Light::LightTypes getOnlyLightType() const { return mOnlyLightType; }
		
		/// Gets the parent Technique
        Technique* getParent(void) { return mParent; }

		/// Gets the resource group of the ultimate parent Material
		const String& getResourceGroup(void) const;

		/** Sets the details of the vertex program to use.
		@remarks
			Only applicable to programmable passes, this sets the details of
			the vertex program to use in this pass. The program will not be
			loaded until the parent Material is loaded.
		@param name The name of the program - this must have been 
			created using GpuProgramManager by the time that this Pass 
			is loaded. If this parameter is blank, any vertex program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them, 
            not just the names.
		*/
		void setVertexProgram(const String& name, bool resetParams = true);
		/** Sets the vertex program parameters.
		@remarks
			Only applicable to programmable passes, and this particular call is
			designed for low-level programs; use the named parameter methods
			for setting high-level program parameters.
		*/
		void setVertexProgramParameters(GpuProgramParametersSharedPtr params);
		/** Gets the name of the vertex program used by this pass. */
		const String& getVertexProgramName(void) const;
        /** Gets the vertex program parameters used by this pass. */
        GpuProgramParametersSharedPtr getVertexProgramParameters(void) const;
		/** Gets the vertex program used by this pass, only available after _load(). */
		const GpuProgramPtr& getVertexProgram(void) const;


        /** Sets the details of the vertex program to use when rendering as a 
        shadow caster.
        @remarks
        Texture-based shadows require that the caster is rendered to a texture 
        in a solid colour (the shadow colour in the case of modulative texture
        shadows). Whilst Ogre can arrange this for the fixed function 
        pipeline, passes which use vertex programs might need the vertex
        programs still to run in order to preserve any deformation etc
        that it does. However, lighting calculations must be a lot simpler, 
        with only the ambient colour being used (which the engine will ensure
        is bound to the shadow colour). 
        @par
        Therefore, it is up to implemetors of vertex programs to provide an
        alternative vertex program which can be used to render the object
        to a shadow texture. Do all the same vertex transforms, but set the
        colour of the vertex to the ambient colour, as bound using the 
        standard auto parameter binding mechanism. 
        @note
        Some vertex programs will work without doing this, because Ogre ensures
        that all lights except for ambient are set black. However, the chances
        are that your vertex program is doing a lot of unnecessary work in this
        case, since the other lights are having no effect, and it is good practice
        to supply an alternative.
        @note
        This is only applicable to programmable passes.
        @par
        The default behaviour is for Ogre to switch to fixed-function 
        rendering if an explict vertex program alternative is not set.
        */
        void setShadowCasterVertexProgram(const String& name);
        /** Sets the vertex program parameters for rendering as a shadow caster.
        @remarks
        Only applicable to programmable passes, and this particular call is
        designed for low-level programs; use the named parameter methods
        for setting high-level program parameters.
        */
        void setShadowCasterVertexProgramParameters(GpuProgramParametersSharedPtr params);
        /** Gets the name of the vertex program used by this pass when rendering shadow casters. */
        const String& getShadowCasterVertexProgramName(void) const;
        /** Gets the vertex program parameters used by this pass when rendering shadow casters. */
        GpuProgramParametersSharedPtr getShadowCasterVertexProgramParameters(void) const;
        /** Gets the vertex program used by this pass when rendering shadow casters, 
            only available after _load(). */
        const GpuProgramPtr& getShadowCasterVertexProgram(void) const;

        /** Sets the details of the vertex program to use when rendering as a 
            shadow receiver.
        @remarks
            Texture-based shadows require that the shadow receiver is rendered using
            a projective texture. Whilst Ogre can arrange this for the fixed function 
            pipeline, passes which use vertex programs might need the vertex
            programs still to run in order to preserve any deformation etc
            that it does. So in this case, we need a vertex program which does the 
            appropriate vertex transformation, but generates projective texture
            coordinates. 
        @par
            Therefore, it is up to implemetors of vertex programs to provide an
            alternative vertex program which can be used to render the object
            as a shadow receiver. Do all the same vertex transforms, but generate
            <strong>2 sets</strong> of texture coordinates using the auto parameter
            ACT_TEXTURE_VIEWPROJ_MATRIX, which Ogre will bind to the parameter name / 
            index you supply as the second parameter to this method. 2 texture 
            sets are needed because Ogre needs to use 2 texture units for some 
            shadow effects.
        @note
            This is only applicable to programmable passes.
        @par
            The default behaviour is for Ogre to switch to fixed-function 
            rendering if an explict vertex program alternative is not set.         
        */
        void setShadowReceiverVertexProgram(const String& name);
        /** Sets the vertex program parameters for rendering as a shadow receiver.
        @remarks
        Only applicable to programmable passes, and this particular call is
        designed for low-level programs; use the named parameter methods
        for setting high-level program parameters.
        */
        void setShadowReceiverVertexProgramParameters(GpuProgramParametersSharedPtr params);

		/** This method allows you to specify a fragment program for use when
			rendering a texture shadow receiver. 
		@remarks
			Texture shadows are applied by rendering the receiver. Modulative texture
			shadows are performed as a post-render darkening pass, and as such 
			fragment programs are generally not required per-object. Additive
			texture shadows, however, are applied by accumulating light masked
			out using a texture shadow (black & white by default, unless you
			customise this using SceneManager::setCustomShadowCasterMaterial).
			OGRE can do this for you for most materials, but if you use a custom
			lighting program (e.g. per pixel lighting) then you'll need to provide
			a custom version for receiving shadows. You don't need to provide
			this for shadow casters if you don't use self-shadowing since they
			will never be shadow receivers too.
		@par
			The shadow texture is always bound to texture unit 0 when rendering
			texture shadow passes. Therefore your custom shadow receiver program
			may well just need to shift it's texture unit usage up by one unit,
			and take the shadow texture into account in its calculations.
		*/
		void setShadowReceiverFragmentProgram(const String& name);
        /** Sets the fragment program parameters for rendering as a shadow receiver.
        @remarks
        Only applicable to programmable passes, and this particular call is
        designed for low-level programs; use the named parameter methods
        for setting high-level program parameters.
        */
        void setShadowReceiverFragmentProgramParameters(GpuProgramParametersSharedPtr params);

        /** Gets the name of the vertex program used by this pass when rendering shadow receivers. */
        const String& getShadowReceiverVertexProgramName(void) const;
        /** Gets the vertex program parameters used by this pass when rendering shadow receivers. */
        GpuProgramParametersSharedPtr getShadowReceiverVertexProgramParameters(void) const;
        /** Gets the vertex program used by this pass when rendering shadow receivers, 
        only available after _load(). */
        const GpuProgramPtr& getShadowReceiverVertexProgram(void) const;

		/** Gets the name of the fragment program used by this pass when rendering shadow receivers. */
		const String& getShadowReceiverFragmentProgramName(void) const;
		/** Gets the fragment program parameters used by this pass when rendering shadow receivers. */
		GpuProgramParametersSharedPtr getShadowReceiverFragmentProgramParameters(void) const;
		/** Gets the fragment program used by this pass when rendering shadow receivers, 
		only available after _load(). */
		const GpuProgramPtr& getShadowReceiverFragmentProgram(void) const;

		/** Sets the details of the fragment program to use.
		@remarks
			Only applicable to programmable passes, this sets the details of
			the fragment program to use in this pass. The program will not be
			loaded until the parent Material is loaded.
		@param name The name of the program - this must have been 
			created using GpuProgramManager by the time that this Pass 
			is loaded. If this parameter is blank, any fragment program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them, 
            not just the names.
		*/
		void setFragmentProgram(const String& name, bool resetParams = true);
		/** Sets the vertex program parameters.
		@remarks
			Only applicable to programmable passes.
		*/
		void setFragmentProgramParameters(GpuProgramParametersSharedPtr params);
		/** Gets the name of the fragment program used by this pass. */
		const String& getFragmentProgramName(void) const;
		/** Gets the vertex program parameters used by this pass. */
		GpuProgramParametersSharedPtr getFragmentProgramParameters(void) const;
		/** Gets the vertex program used by this pass, only available after _load(). */
		const GpuProgramPtr& getFragmentProgram(void) const;

		/** Splits this Pass to one which can be handled in the number of
			texture units specified.
		@remarks
			Only works on non-programmable passes, programmable passes cannot be
			split, it's up to the author to ensure that there is a fallback Technique
			for less capable cards.
		@param numUnits The target number of texture units
		@returns A new Pass which contains the remaining units, and a scene_blend
				setting appropriate to approximate the multitexture. This Pass will be 
				attached to the parent Technique of this Pass.
		*/
		Pass* _split(unsigned short numUnits);

		/** Internal method to adjust pass index. */
		void _notifyIndex(unsigned short index);

		/** Internal method for loading this pass. */
		void _load(void);
		/** Internal method for unloading this pass. */
		void _unload(void);
        // Is this loaded?
        bool isLoaded(void) const;

        /** Gets the 'hash' of this pass, ie a precomputed number to use for sorting
        @remarks
            This hash is used to sort passes, and for this reason the pass is hashed
            using firstly its index (so that all passes are rendered in order), then
            by the textures which it's TextureUnitState instances are using.
        */
        uint32 getHash(void) const;
		/// Mark the hash as dirty
		void _dirtyHash(void);
        /** Internal method for recalculating the hash.
		@remarks
			Do not call this unless you are sure the old hash is not still being
			used by anything. If in doubt, call _dirtyHash if you want to force
			recalculation of the has next time.
		*/
        void _recalculateHash(void);
        /** Tells the pass that it needs recompilation. */
        void _notifyNeedsRecompile(void);

        /** Update any automatic parameters (except lights) on this pass */
        void _updateAutoParamsNoLights(const AutoParamDataSource& source) const;
        /** Update any automatic light parameters on this pass */
        void _updateAutoParamsLightsOnly(const AutoParamDataSource& source) const;

        /** Set texture filtering for every texture unit 
        @note
            This property actually exists on the TextureUnitState class
            For simplicity, this method allows you to set these properties for 
            every current TeextureUnitState, If you need more precision, retrieve the  
            TextureUnitState instance and set the property there.
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
		/** Static method to retrieve all the Passes which need their
		    hash values recalculated. 
		*/
		static const PassSet& getDirtyHashList(void) 
		{ return msDirtyHashList; }
        /** Static method to retrieve all the Passes which are pending deletion.
        */
        static const PassSet& getPassGraveyard(void) 
        { return msPassGraveyard; }
		/** Static method to reset the list of passes which need their hash 
		    values recalculated. 
		@remarks
			For performance, the dirty list is not updated progressively as 
			the hashes are recalculated, instead we expect the processor of the
			dirty hash list to clear the list when they are done.
		*/
		static void clearDirtyHashList(void) { msDirtyHashList.clear(); }

        /** Process all dirty and pending deletion passes. */
        static void processPendingPassUpdates(void);

        /** Queue this pass for deletion when appropriate. */
        void queueForDeletion(void);

        /** Returns whether this pass is ambient only.
        */
        bool isAmbientOnly(void) const;

        /** set the number of iterations that this pass
        should perform when doing fast multi pass operation.
        @remarks
            Only applicable for programmable passes.
        @param count number of iterations to perform fast multi pass operations.
            A value greater than 0 will cause the pass to be executed count number of
            times without changing the render state.  This is very usefull for passes
            that use programmable shaders that have to iterate more than once but don't
            need a render state change.  Using multi pass can dramatically speed up rendering
            for materials that do things like fur, blur.
            A value of 0 turns off multi pass operation and the pass does
            the normal pass operation.
        */
        void setPassIterationCount(const size_t count) { mPassIterationCount = count; }

        /** Gets the multi pass count value.
        */
        size_t getPassIterationCount(void) const { return mPassIterationCount; }

        /** Applies texture names to Texture Unit State with matching texture name aliases.
            All Texture Unit States within the pass are checked.
            If matching texture aliases are found then true is returned.

        @param
            aliasList is a map container of texture alias, texture name pairs
        @param
            apply set true to apply the texture aliases else just test to see if texture alias matches are found.
        @return
            True if matching texture aliases were found in the pass.
        */
        bool applyTextureAliases(const AliasTextureNamePairList& aliasList, const bool apply = true) const;
        
    };

    enum IlluminationStage
    {
        /// Part of the rendering which occurs without any kind of direct lighting
        IS_AMBIENT,
        /// Part of the rendering which occurs per light
        IS_PER_LIGHT,
        /// Post-lighting rendering
        IS_DECAL
    };
    /** Struct recording a pass which can be used for a specific illumination stage.
    @remarks
        This structure is used to record categorised passes which fit into a 
        number of distinct illumination phases - ambient, diffuse / specular 
        (per-light) and decal (post-lighting texturing).
        An original pass may fit into one of these categories already, or it
        may require splitting into its component parts in order to be categorised 
        properly.
    */
    struct IlluminationPass
    {
        IlluminationStage stage;
        /// The pass to use in this stage
        Pass* pass;
        /// Whether this pass is one which should be deleted itself
        bool destroyOnShutdown;
        /// The original pass which spawned this one
        Pass* originalPass;
    };

    typedef std::vector<IlluminationPass*> IlluminationPassList;


}

#endif
