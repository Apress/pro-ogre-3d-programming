/*-------------------------------------------------------------------------
This source file is a part of OGRE
(Object-oriented Graphics Rendering Engine)

For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License (LGPL) as 
published by the Free Software Foundation; either version 2.1 of the 
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
License for more details.

You should have received a copy of the GNU Lesser General Public License 
along with this library; if not, write to the Free Software Foundation, 
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA or go to
http://www.gnu.org/copyleft/lesser.txt
-------------------------------------------------------------------------*/
#include "OgreStableHeaders.h"

#include "OgreTextAreaOverlayElement.h"
#include "OgreRoot.h"
#include "OgreLogManager.h"
#include "OgreOverlayManager.h"
#include "OgreHardwareBufferManager.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreException.h"
#include "OgreStringConverter.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

namespace Ogre {

#define DEFAULT_INITIAL_CHARS 12
    //---------------------------------------------------------------------
    String TextAreaOverlayElement::msTypeName = "TextArea";
    TextAreaOverlayElement::CmdCharHeight TextAreaOverlayElement::msCmdCharHeight;
    TextAreaOverlayElement::CmdSpaceWidth TextAreaOverlayElement::msCmdSpaceWidth;
    TextAreaOverlayElement::CmdFontName TextAreaOverlayElement::msCmdFontName;
    TextAreaOverlayElement::CmdColour TextAreaOverlayElement::msCmdColour;
    TextAreaOverlayElement::CmdColourBottom TextAreaOverlayElement::msCmdColourBottom;
    TextAreaOverlayElement::CmdColourTop TextAreaOverlayElement::msCmdColourTop;
    TextAreaOverlayElement::CmdAlignment TextAreaOverlayElement::msCmdAlignment;
    //---------------------------------------------------------------------
    #define POS_TEX_BINDING 0
    #define COLOUR_BINDING 1
    //---------------------------------------------------------------------
    TextAreaOverlayElement::TextAreaOverlayElement(const String& name)
        : OverlayElement(name)
    {
        mTransparent = false;
        mAlignment = Left;

        mColourTop = ColourValue::White;
        mColourBottom = ColourValue::White;
        mColoursChanged = true;

        mAllocSize = 0;

        mCharHeight = 0.02;
		mPixelCharHeight = 12;
		mSpaceWidth = 0;
		mPixelSpaceWidth = 0;
		mViewportAspectCoef = 1;

        if (createParamDictionary("TextAreaOverlayElement"))
        {
            addBaseParameters();
        }
    }

    void TextAreaOverlayElement::initialise(void)
    {
		if (!mInitialised)
		{
			// Set up the render op
			// Combine positions and texture coords since they tend to change together
			// since character sizes are different
			mRenderOp.vertexData = new VertexData();
			VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
			size_t offset = 0;
			// Positions
			decl->addElement(POS_TEX_BINDING, offset, VET_FLOAT3, VES_POSITION);
			offset += VertexElement::getTypeSize(VET_FLOAT3);
			// Texcoords
			decl->addElement(POS_TEX_BINDING, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
			offset += VertexElement::getTypeSize(VET_FLOAT2);
			// Colours - store these in a separate buffer because they change less often
			decl->addElement(COLOUR_BINDING, 0, VET_COLOUR, VES_DIFFUSE);

			mRenderOp.operationType = RenderOperation::OT_TRIANGLE_LIST;
			mRenderOp.useIndexes = false;
			mRenderOp.vertexData->vertexStart = 0;
			// Vertex buffer will be created in checkMemoryAllocation

			checkMemoryAllocation( DEFAULT_INITIAL_CHARS );

			mInitialised = true;
		}

    }

    void TextAreaOverlayElement::checkMemoryAllocation( size_t numChars )
    {
        if( mAllocSize < numChars)
        {
            // Create and bind new buffers
            // Note that old buffers will be deleted automatically through reference counting
            
            // 6 verts per char since we're doing tri lists without indexes
            // Allocate space for positions & texture coords
            VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
            VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

            mRenderOp.vertexData->vertexCount = numChars * 6;

            // Create dynamic since text tends to change alot
            // positions & texcoords
            HardwareVertexBufferSharedPtr vbuf = 
                HardwareBufferManager::getSingleton().
                    createVertexBuffer(
                        decl->getVertexSize(POS_TEX_BINDING), 
                        mRenderOp.vertexData->vertexCount,
                        HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
            bind->setBinding(POS_TEX_BINDING, vbuf);

            // colours
            vbuf = HardwareBufferManager::getSingleton().
                    createVertexBuffer(
                        decl->getVertexSize(COLOUR_BINDING), 
                        mRenderOp.vertexData->vertexCount,
                        HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
            bind->setBinding(COLOUR_BINDING, vbuf);

            mAllocSize = numChars;
            mColoursChanged = true; // force colour buffer regeneration
        }

    }

    void TextAreaOverlayElement::updatePositionGeometry()
    {
		float *pVert;

		if (mpFont.isNull())
		{
			// not initialised yet, probably due to the order of creation in a template
			return;
		}

		size_t charlen = mCaption.size();
		checkMemoryAllocation( charlen );

		mRenderOp.vertexData->vertexCount = charlen * 6;
		// Get position / texcoord buffer
		HardwareVertexBufferSharedPtr vbuf = 
			mRenderOp.vertexData->vertexBufferBinding->getBuffer(POS_TEX_BINDING);
		pVert = static_cast<float*>(
			vbuf->lock(HardwareBuffer::HBL_DISCARD) );

		float largestWidth = 0;
		float left = _getDerivedLeft() * 2.0 - 1.0;
		float top = -( (_getDerivedTop() * 2.0 ) - 1.0 );

		// Derive space with from a capital A
		if (mSpaceWidth == 0)
		{
			mSpaceWidth = mpFont->getGlyphAspectRatio( 'A' ) * mCharHeight * 2.0 * mViewportAspectCoef;
		}

		// Use iterator
		String::iterator i, iend;
		iend = mCaption.end();
		bool newLine = true;
		for( i = mCaption.begin(); i != iend; ++i )
		{
			if( newLine )
			{
				Real len = 0.0f;
				for( String::iterator j = i; j != iend && *j != '\n'; j++ )
				{
					if (*j == ' ')
					{
						len += mSpaceWidth;
					}
					else 
					{
						len += mpFont->getGlyphAspectRatio( *j ) * mCharHeight * 2.0 * mViewportAspectCoef;
					}
				}

				if( mAlignment == Right )
					left -= len;
				else if( mAlignment == Center )
					left -= len * 0.5;

				newLine = false;
			}

			if( *i == '\n' )
			{
				left = _getDerivedLeft() * 2.0 - 1.0;
				top -= mCharHeight * 2.0;
				newLine = true;
				// Also reduce tri count
				mRenderOp.vertexData->vertexCount -= 6;
				continue;
			}

			if ( *i == ' ')
			{
				// Just leave a gap, no tris
				left += mSpaceWidth;
				// Also reduce tri count
				mRenderOp.vertexData->vertexCount -= 6;
				continue;
			}

			Real horiz_height = mpFont->getGlyphAspectRatio( *i ) * mViewportAspectCoef ;
			Real u1, u2, v1, v2; 
			mpFont->getGlyphTexCoords( *i, u1, v1, u2, v2 );

			// each vert is (x, y, z, u, v)
			//-------------------------------------------------------------------------------------
			// First tri
			//
			// Upper left
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u1;
			*pVert++ = v1;

			top -= mCharHeight * 2.0;

			// Bottom left
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u1;
			*pVert++ = v2;

			top += mCharHeight * 2.0;
			left += horiz_height * mCharHeight * 2.0;

			// Top right
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u2;
			*pVert++ = v1;
			//-------------------------------------------------------------------------------------

			//-------------------------------------------------------------------------------------
			// Second tri
			//
			// Top right (again)
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u2;
			*pVert++ = v1;

			top -= mCharHeight * 2.0;
			left -= horiz_height  * mCharHeight * 2.0;

			// Bottom left (again)
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u1;
			*pVert++ = v2;

			left += horiz_height  * mCharHeight * 2.0;

			// Bottom right
			*pVert++ = left;
			*pVert++ = top;
			*pVert++ = -1.0;
			*pVert++ = u2;
			*pVert++ = v2;
			//-------------------------------------------------------------------------------------

			// Go back up with top
			top += mCharHeight * 2.0;

			float currentWidth = (left + 1)/2 - _getDerivedLeft();
			if (currentWidth > largestWidth)
			{
				largestWidth = currentWidth;

			}
		}
		// Unlock vertex buffer
		vbuf->unlock();

		if (mMetricsMode == GMM_PIXELS)
		{
			// Derive parametric version of dimensions
			Real vpWidth;
			vpWidth = (Real) (OverlayManager::getSingleton().getViewportWidth());

			largestWidth *= vpWidth;
		};

		if (getWidth() < largestWidth)
			setWidth(largestWidth);
    }

	void TextAreaOverlayElement::updateTextureGeometry()
	{
		// Nothing to do, we combine positions and textures
	}

    void TextAreaOverlayElement::setCaption( const String& caption )
    {
        mCaption = caption;
		mGeomPositionsOutOfDate = true;
		mGeomUVsOutOfDate = true;
    }
    const String& TextAreaOverlayElement::getCaption() const
    {
        return mCaption;
    }

    void TextAreaOverlayElement::setFontName( const String& font )
    {
        mpFont = FontManager::getSingleton().getByName( font );
        if (mpFont.isNull())
			OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find font " + font,
				"TextAreaOverlayElement::setFontName" );
        mpFont->load();
        mpMaterial = mpFont->getMaterial();
        mpMaterial->setDepthCheckEnabled(false);
        mpMaterial->setLightingEnabled(false);
		
		mGeomPositionsOutOfDate = true;
		mGeomUVsOutOfDate = true;
    }
    const String& TextAreaOverlayElement::getFontName() const
    {
        return mpFont->getName();
    }

    void TextAreaOverlayElement::setCharHeight( Real height )
    {
        if (mMetricsMode != GMM_RELATIVE)
        {
            mPixelCharHeight = static_cast<unsigned>(height);
        }
        else
        {
            mCharHeight = height;
        }
        mGeomPositionsOutOfDate = true;
    }
	Real TextAreaOverlayElement::getCharHeight() const
	{
		if (mMetricsMode == GMM_PIXELS)
		{
			return mPixelCharHeight;
		}
		else
		{
			return mCharHeight;
		}
	}

    void TextAreaOverlayElement::setSpaceWidth( Real width )
    {
        if (mMetricsMode != GMM_RELATIVE)
        {
            mPixelSpaceWidth = static_cast<unsigned>(width);
        }
        else
        {
            mSpaceWidth = width;
        }

        mGeomPositionsOutOfDate = true;
    }
	Real TextAreaOverlayElement::getSpaceWidth() const
	{
		if (mMetricsMode == GMM_PIXELS)
		{
			return mPixelSpaceWidth;
		}
		else
		{
			return mSpaceWidth;
		}
	}

    //---------------------------------------------------------------------
    TextAreaOverlayElement::~TextAreaOverlayElement()
    {
        delete mRenderOp.vertexData;
    }
    //---------------------------------------------------------------------
    const String& TextAreaOverlayElement::getTypeName(void) const
    {
        return msTypeName;
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::getRenderOperation(RenderOperation& op)
    {
        op = mRenderOp;
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::setMaterialName(const String& matName)
    {
        OverlayElement::setMaterialName(matName);
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::addBaseParameters(void)
    {
        OverlayElement::addBaseParameters();
        ParamDictionary* dict = getParamDictionary();

        dict->addParameter(ParameterDef("char_height", 
            "Sets the height of the characters in relation to the screen."
            , PT_REAL),
            &msCmdCharHeight);

        dict->addParameter(ParameterDef("space_width", 
            "Sets the width of a space in relation to the screen."
            , PT_REAL),
            &msCmdSpaceWidth);

        dict->addParameter(ParameterDef("font_name", 
            "Sets the name of the font to use."
            , PT_STRING),
            &msCmdFontName);

        dict->addParameter(ParameterDef("colour", 
            "Sets the colour of the font (a solid colour)."
            , PT_STRING),
            &msCmdColour);

        dict->addParameter(ParameterDef("colour_bottom", 
            "Sets the colour of the font at the bottom (a gradient colour)."
            , PT_STRING),
            &msCmdColourBottom);

        dict->addParameter(ParameterDef("colour_top", 
            "Sets the colour of the font at the top (a gradient colour)."
            , PT_STRING),
            &msCmdColourTop);

        dict->addParameter(ParameterDef("alignment", 
            "Sets the alignment of the text: 'left', 'center' or 'right'."
            , PT_STRING),
            &msCmdAlignment);
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::setColour(const ColourValue& col)
    {
        mColourBottom = mColourTop = col;
        mColoursChanged = true;
    }
    //---------------------------------------------------------------------
    const ColourValue& TextAreaOverlayElement::getColour(void) const
    {
        // Either one
        return mColourTop;
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::setColourBottom(const ColourValue& col)
    {
        mColourBottom = col;
        mColoursChanged = true;
    }
    //---------------------------------------------------------------------
    const ColourValue& TextAreaOverlayElement::getColourBottom(void) const
    {
        return mColourBottom;
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::setColourTop(const ColourValue& col)
    {
        mColourTop = col;
        mColoursChanged = true;
    }
    //---------------------------------------------------------------------
    const ColourValue& TextAreaOverlayElement::getColourTop(void) const
    {
        return mColourTop;
    }
    //---------------------------------------------------------------------
    void TextAreaOverlayElement::updateColours(void)
    {
        // Convert to system-specific
        RGBA topColour, bottomColour;
        Root::getSingleton().convertColourValue(mColourTop, &topColour);
        Root::getSingleton().convertColourValue(mColourBottom, &bottomColour);

        HardwareVertexBufferSharedPtr vbuf = 
            mRenderOp.vertexData->vertexBufferBinding->getBuffer(COLOUR_BINDING);

        RGBA* pDest = static_cast<RGBA*>(
            vbuf->lock(HardwareBuffer::HBL_DISCARD) );

        for (size_t i = 0; i < mAllocSize; ++i)
        {
            // First tri (top, bottom, top)
            *pDest++ = topColour;
            *pDest++ = bottomColour;
            *pDest++ = topColour;
            // Second tri (top, bottom, bottom)
            *pDest++ = topColour;
            *pDest++ = bottomColour;
            *pDest++ = bottomColour;
        }
        vbuf->unlock();

    }
    //-----------------------------------------------------------------------
    void TextAreaOverlayElement::setMetricsMode(GuiMetricsMode gmm)
    {
        Real vpWidth, vpHeight;

		vpWidth = (Real) (OverlayManager::getSingleton().getViewportWidth());
        vpHeight = (Real) (OverlayManager::getSingleton().getViewportHeight());
		mViewportAspectCoef = vpHeight/vpWidth;

		OverlayElement::setMetricsMode(gmm);
        if (gmm != GMM_RELATIVE)
        {
            // Set pixel variables based on viewport multipliers
            mPixelCharHeight = static_cast<unsigned>(mCharHeight * vpHeight);
            mPixelSpaceWidth = static_cast<unsigned>(mSpaceWidth * vpHeight);
        }
    }

    //-----------------------------------------------------------------------
    void TextAreaOverlayElement::_update(void)
    {
        Real vpWidth, vpHeight;

        vpWidth = (Real) (OverlayManager::getSingleton().getViewportWidth());
        vpHeight = (Real) (OverlayManager::getSingleton().getViewportHeight());
		mViewportAspectCoef = vpHeight/vpWidth;
		
		if (mMetricsMode != GMM_RELATIVE && 
            (OverlayManager::getSingleton().hasViewportChanged() || mGeomPositionsOutOfDate))
        {
            // Recalc character size

            mCharHeight = (Real) mPixelCharHeight / vpHeight;
            mSpaceWidth = (Real) mPixelSpaceWidth / vpHeight;
			mGeomPositionsOutOfDate = true;
        }
        OverlayElement::_update();

		if (mColoursChanged && mInitialised)
		{
			updateColours();
			mColoursChanged = false;
		}
    }
    //---------------------------------------------------------------------------------------------
    // Char height command object
    //
    String TextAreaOverlayElement::CmdCharHeight::doGet( const void* target ) const
    {
        return StringConverter::toString( 
            static_cast< const TextAreaOverlayElement* >( target )->getCharHeight() );
    }
    void TextAreaOverlayElement::CmdCharHeight::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setCharHeight( 
            StringConverter::parseReal( val ) );
    }
    //---------------------------------------------------------------------------------------------
    // Space width command object
    //
    String TextAreaOverlayElement::CmdSpaceWidth::doGet( const void* target ) const
    {
        return StringConverter::toString( 
            static_cast< const TextAreaOverlayElement* >( target )->getSpaceWidth() );
    }
    void TextAreaOverlayElement::CmdSpaceWidth::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setSpaceWidth( 
            StringConverter::parseReal( val ) );
    }
    //---------------------------------------------------------------------------------------------

    //---------------------------------------------------------------------------------------------
    // Font name command object
    //
    String TextAreaOverlayElement::CmdFontName::doGet( const void* target ) const
    {
        return static_cast< const TextAreaOverlayElement* >( target )->getFontName();
    }
    void TextAreaOverlayElement::CmdFontName::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setFontName( val );
    }
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    // Colour command object
    //
    String TextAreaOverlayElement::CmdColour::doGet( const void* target ) const
    {
        return StringConverter::toString (
            static_cast< const TextAreaOverlayElement* >( target )->getColour());
    }
    void TextAreaOverlayElement::CmdColour::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setColour( 
            StringConverter::parseColourValue(val) );
    }
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    // Top colour command object
    //
    String TextAreaOverlayElement::CmdColourTop::doGet( const void* target ) const
    {
        return StringConverter::toString (
            static_cast< const TextAreaOverlayElement* >( target )->getColourTop());
    }
    void TextAreaOverlayElement::CmdColourTop::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setColourTop( 
            StringConverter::parseColourValue(val) );
    }
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    // Bottom colour command object
    //
    String TextAreaOverlayElement::CmdColourBottom::doGet( const void* target ) const
    {
        return StringConverter::toString (
            static_cast< const TextAreaOverlayElement* >( target )->getColourBottom());
    }
    void TextAreaOverlayElement::CmdColourBottom::doSet( void* target, const String& val )
    {
        static_cast< TextAreaOverlayElement* >( target )->setColourBottom( 
            StringConverter::parseColourValue(val) );
    }
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    // Alignment command object
    //
    String TextAreaOverlayElement::CmdAlignment::doGet( const void* target ) const
    {
        Alignment align = static_cast< const TextAreaOverlayElement* >( target )->getAlignment();
        switch (align)
        {
            case Left:
                return "left";
            case Center:
                return "center";
            case Right:
                return "right";
                
        }
        // To keep compiler happy
        return "left";
    }
    void TextAreaOverlayElement::CmdAlignment::doSet( void* target, const String& val )
    {
        if (val == "center")
        {
            static_cast< TextAreaOverlayElement* >( target )->setAlignment(Center);
        }
        else if (val == "right")
        {
            static_cast< TextAreaOverlayElement* >( target )->setAlignment(Right);
        }
        else
        {
            static_cast< TextAreaOverlayElement* >( target )->setAlignment(Left);
        }
    }
    //---------------------------------------------------------------------------------------------
}
