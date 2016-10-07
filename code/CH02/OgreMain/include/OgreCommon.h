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
#ifndef __Common_H__
#define __Common_H__
// Common stuff

#include <utility>

namespace Ogre {


    /** Comparison functions used for the depth/stencil buffer operations and 
		others. */
    enum CompareFunction
    {
        CMPF_ALWAYS_FAIL,
        CMPF_ALWAYS_PASS,
        CMPF_LESS,
        CMPF_LESS_EQUAL,
        CMPF_EQUAL,
        CMPF_NOT_EQUAL,
        CMPF_GREATER_EQUAL,
        CMPF_GREATER
    };

    /** High-level filtering options providing shortcuts to settings the
        minification, magnification and mip filters. */
    enum TextureFilterOptions
    {
        /// Equal to: min=FO_POINT, mag=FO_POINT, mip=FO_NONE
        TFO_NONE,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_POINT
        TFO_BILINEAR,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_LINEAR
        TFO_TRILINEAR,
        /// Equal to: min=FO_ANISOTROPIC, max=FO_ANISOTROPIC, mip=FO_LINEAR
		TFO_ANISOTROPIC
    };

    enum FilterType
    {
        /// The filter used when shrinking a texture
        FT_MIN,
        /// The filter used when magnifiying a texture
        FT_MAG,
        /// The filter used when determining the mipmap
        FT_MIP
    };
    /** Filtering options for textures / mipmaps. */
    enum FilterOptions
    {
        /// No filtering, used for FILT_MIP to turn off mipmapping
        FO_NONE,
        /// Use the closest pixel
        FO_POINT,
        /// Average of a 2x2 pixel area, denotes bilinear for MIN and MAG, trilinear for MIP
        FO_LINEAR,
        /// Similar to FO_LINEAR, but compensates for the angle of the texture plane
        FO_ANISOTROPIC
    };

    /** Light shading modes. */
    enum ShadeOptions
    {
        SO_FLAT,
        SO_GOURAUD,
        SO_PHONG
    };

    /** Fog modes. */
    enum FogMode
    {
        /// No fog. Duh.
        FOG_NONE,
        /// Fog density increases  exponentially from the camera (fog = 1/e^(distance * density))
        FOG_EXP,
        /// Fog density increases at the square of FOG_EXP, i.e. even quicker (fog = 1/e^(distance * density)^2)
        FOG_EXP2,
        /// Fog density increases linearly between the start and end distances
        FOG_LINEAR
    };

    /** Hardware culling modes based on vertex winding.
        This setting applies to how the hardware API culls triangles it is sent. */
    enum CullingMode
    {
        /// Hardware never culls triangles and renders everything it receives.
        CULL_NONE = 1,
        /// Hardware culls triangles whose vertices are listed clockwise in the view (default).
        CULL_CLOCKWISE = 2,
        /// Hardware culls triangles whose vertices are listed anticlockwise in the view.
        CULL_ANTICLOCKWISE = 3
    };

    /** Manual culling modes based on vertex normals.
        This setting applies to how the software culls triangles before sending them to the 
		hardware API. This culling mode is used by scene managers which choose to implement it -
		normally those which deal with large amounts of fixed world geometry which is often 
		planar (software culling movable variable geometry is expensive). */
    enum ManualCullingMode
    {
        /// No culling so everything is sent to the hardware.
        MANUAL_CULL_NONE = 1,
        /// Cull triangles whose normal is pointing away from the camera (default).
        MANUAL_CULL_BACK = 2,
        /// Cull triangles whose normal is pointing towards the camera.
        MANUAL_CULL_FRONT = 3
    };

    /** Enumerates the wave types usable with the Ogre engine. */
    enum WaveformType
    {
        /// Standard sine wave which smoothly changes from low to high and back again.
        WFT_SINE,
        /// An angular wave with a constant increase / decrease speed with pointed peaks.
        WFT_TRIANGLE,
        /// Half of the time is spent at the min, half at the max with instant transition between.
        WFT_SQUARE,
        /// Gradual steady increase from min to max over the period with an instant return to min at the end.
        WFT_SAWTOOTH,
        /// Gradual steady decrease from max to min over the period, with an instant return to max at the end.
        WFT_INVERSE_SAWTOOTH,
		/// Pulse Width Modulation. Works like WFT_SQUARE, except the high to low transition is controlled by duty cycle. 
		/// With a duty cycle of 50% (0.5) will give the same output as WFT_SQUARE.
		WFT_PWM
    };

    /** The polygon mode to use when rasterising. */
    enum PolygonMode
    {
		/// Only points are rendered.
        PM_POINTS = 1,
		/// Wireframe models are rendered.
        PM_WIREFRAME = 2,
		/// Solid polygons are rendered.
        PM_SOLID = 3
    };

    /** An enumeration of broad shadow techniques */
    enum ShadowTechnique
    {
        /** No shadows */
        SHADOWTYPE_NONE = 0x00,
		/** Mask for additive shadows (not for direct use, use  SHADOWTYPE_ enum instead)
		*/
		SHADOWDETAILTYPE_ADDITIVE = 0x01,
		/** Mask for modulative shadows (not for direct use, use  SHADOWTYPE_ enum instead)
		*/
		SHADOWDETAILTYPE_MODULATIVE = 0x02,
		/** Mask for stencil shadows (not for direct use, use  SHADOWTYPE_ enum instead)
		*/
		SHADOWDETAILTYPE_STENCIL = 0x10,
		/** Mask for texture shadows (not for direct use, use  SHADOWTYPE_ enum instead)
		*/
		SHADOWDETAILTYPE_TEXTURE = 0x20,
		
        /** Stencil shadow technique which renders all shadow volumes as
            a modulation after all the non-transparent areas have been 
            rendered. This technique is considerably less fillrate intensive 
            than the additive stencil shadow approach when there are multiple
            lights, but is not an accurate model. 
        */
        SHADOWTYPE_STENCIL_MODULATIVE = 0x12,
        /** Stencil shadow technique which renders each light as a separate
            additive pass to the scene. This technique can be very fillrate
            intensive because it requires at least 2 passes of the entire
            scene, more if there are multiple lights. However, it is a more
            accurate model than the modulative stencil approach and this is
            especially apparant when using coloured lights or bump mapping.
        */
        SHADOWTYPE_STENCIL_ADDITIVE = 0x11,
        /** Texture-based shadow technique which involves a monochrome render-to-texture
            of the shadow caster and a projection of that texture onto the 
            shadow receivers as a modulative pass. 
        */
        SHADOWTYPE_TEXTURE_MODULATIVE = 0x22,
		
        /** Texture-based shadow technique which involves a monochrome render-to-texture
            of the shadow caster and a projection of that texture onto the 
            shadow receivers, built up per light as additive passes. 
			This technique can be very fillrate intensive because it requires numLights + 2 
			passes of the entire scene. However, it is a more accurate model than the 
			modulative approach and this is especially apparant when using coloured lights 
			or bump mapping.
        */
        SHADOWTYPE_TEXTURE_ADDITIVE = 0x21
    };

    /** An enumeration describing which material properties should track the vertex colours */
    typedef int TrackVertexColourType;
    enum TrackVertexColourEnum {
        TVC_NONE        = 0x0,
        TVC_AMBIENT     = 0x1,        
        TVC_DIFFUSE     = 0x2,
        TVC_SPECULAR    = 0x4,
        TVC_EMISSIVE    = 0x8
    };

    /** Sort mode for billboard-set and particle-system */
    enum SortMode
    {
        /** Sort by direction of the camera */
        SM_DIRECTION,
        /** Sort by distance from the camera */
        SM_DISTANCE
    };

    /** Defines the frame buffer types. */
    enum FrameBufferType {
        FBT_COLOUR  = 0x1,
        FBT_DEPTH   = 0x2,
        FBT_STENCIL = 0x4
    };
    
	
    typedef std::vector<Light*> LightList;

    typedef std::map<String, bool> UnaryOptionList;
    typedef std::map<String, String> BinaryOptionList;

	/// Name / value parameter pair (first = name, second = value)
	typedef std::map<String, String> NameValuePairList;

    /// Alias / Texture name pair (first = alias, second = texture name)
    typedef std::map<String, String> AliasTextureNamePairList;

        template< typename T > struct TRect
        {
          T left, top, right, bottom;
          TRect() {}
          TRect( T const & l, T const & t, T const & r, T const & b )
            : left( l ), top( t ), right( r ), bottom( b )
          {
          }
          TRect( TRect const & o )
            : left( o.left ), top( o.top ), right( o.right ), bottom( o.bottom )
          {
          }
          TRect & operator=( TRect const & o )
          {
            left = o.left;
            top = o.top;
            right = o.right;
            bottom = o.bottom;
            return *this;
          }
          T width() const
          {
            return right - left;
          }
          T height() const
          {
            return bottom - top;
          }
        };

        /** Structure used to define a rectangle in a 2-D floating point space.
        */
        typedef TRect<float> FloatRect;

        /** Structure used to define a rectangle in a 2-D integer space.
        */
        typedef TRect< long > Rect;

        /** Structure used to define a box in a 3-D integer space.
         	Note that the left, top, and front edges are included but the right, 
         	bottom and top ones are not.
         */
        struct Box
        {
            size_t left, top, right, bottom, front, back;
			/// Parameterless constructor for setting the members manually
            Box()
            {
            }
            /** Define a box from left, top, right and bottom coordinates
            	This box will have depth one (front=0 and back=1).
            	@param	l	x value of left edge
            	@param	t	y value of top edge
            	@param	r	x value of right edge
            	@param	b	y value of bottom edge
            	@note Note that the left, top, and front edges are included 
 		           	but the right, bottom and top ones are not.
            */
            Box( size_t l, size_t t, size_t r, size_t b ):
                left(l),
                top(t),   
                right(r),
                bottom(b),
                front(0),
                back(1)
            {
          		assert(right >= left && bottom >= top && back >= front);
            }
            /** Define a box from left, top, front, right, bottom and back
            	coordinates.
            	@param	l	x value of left edge
            	@param	t	y value of top edge
            	@param  ff  z value of front edge
            	@param	r	x value of right edge
            	@param	b	y value of bottom edge
            	@param  bb  z value of back edge
            	@note Note that the left, top, and front edges are included 
 		           	but the right, bottom and top ones are not.
            */
            Box( size_t l, size_t t, size_t ff, size_t r, size_t b, size_t bb ):
                left(l),
                top(t),   
                right(r),
                bottom(b),
                front(ff),
                back(bb)
            {
          		assert(right >= left && bottom >= top && back >= front);
            }
            
            /// Return true if the other box is a part of this one
            bool contains(const Box &def) const
            {
            	return (def.left >= left && def.top >= top && def.front >= front &&
					def.right <= right && def.bottom <= bottom && def.back <= back);
            }
            
            /// Get the width of this box
            size_t getWidth() const { return right-left; }
            /// Get the height of this box
            size_t getHeight() const { return bottom-top; }
            /// Get the depth of this box
            size_t getDepth() const { return back-front; }
        };

    
	
	/** Locate command-line options of the unary form '-blah' and of the
        binary form '-blah foo', passing back the index of the next non-option.
    @param numargs, argv The standard parameters passed to the main method
    @param unaryOptList Map of unary options (ie those that do not require a parameter).
        Should be pre-populated with, for example '-e' in the key and false in the 
        value. Options which are found will be set to true on return.
    @param binOptList Map of binnary options (ie those that require a parameter
        e.g. '-e afile.txt').
        Should be pre-populated with, for example '-e' and the default setting. 
        Options which are found will have the value updated.
    */
    int _OgreExport findCommandLineOpts(int numargs, char** argv, UnaryOptionList& unaryOptList, 
        BinaryOptionList& binOptList);

}

#endif
