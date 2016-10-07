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

#include "OgreQuake3Shader.h"
#include "OgreSceneManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreTextureUnitState.h"
#include "OgreMath.h"
#include "OgreLogManager.h"
#include "OgreTextureManager.h"
#include "OgreRoot.h"
#include "OgreMaterialManager.h"

namespace Ogre {


    //-----------------------------------------------------------------------
    Quake3Shader::Quake3Shader(const String& name)
    {
        mName = name;
        numPasses = 0;
        deformFunc = DEFORM_FUNC_NONE;
        farbox = false;
        skyDome = false;
        flags = 0;
        fog = false;
        cullMode = MANUAL_CULL_BACK;

    }
    //-----------------------------------------------------------------------
    Quake3Shader::~Quake3Shader()
    {
    }
    //-----------------------------------------------------------------------
    MaterialPtr Quake3Shader::createAsMaterial(int lightmapNumber)
    {
		String matName;
		StringUtil::StrStreamType str;
        String resourceGroup = ResourceGroupManager::getSingleton().getWorldResourceGroupName();

        str << mName << "#" << lightmapNumber;
		matName = str.str();

        MaterialPtr mat = MaterialManager::getSingleton().create(matName, 
            resourceGroup);
		Ogre::Pass* ogrePass = mat->getTechnique(0)->getPass(0);

        LogManager::getSingleton().logMessage("Using Q3 shader " + mName, LML_CRITICAL);
        for (int p = 0; p < numPasses; ++p)
        {
            TextureUnitState* t;
            // Create basic texture
            if (pass[p].textureName == "$lightmap")
            {
                StringUtil::StrStreamType str2;
				str2 << "@lightmap" << lightmapNumber;
                t = ogrePass->createTextureUnitState(str2.str());
            }
            // Animated texture support
            else if (pass[p].animNumFrames > 0)
            {
                Real sequenceTime = pass[p].animNumFrames / pass[p].animFps;
                /* Pre-load textures
                   We need to know if each one was loaded OK since extensions may change for each
                   Quake3 can still include alternate extension filenames e.g. jpg instead of tga
                   Pain in the arse - have to check for each frame as letters<n>.tga for example
                   is different per frame!
                */
                for (unsigned int alt = 0; alt < pass[p].animNumFrames; ++alt)
                {
                    if (!ResourceGroupManager::getSingleton().resourceExists(
                        resourceGroup, pass[p].frames[alt]))
                    {
                        // Try alternate extension
                        pass[p].frames[alt] = getAlternateName(pass[p].frames[alt]);
                        if (!ResourceGroupManager::getSingleton().resourceExists(
                            resourceGroup, pass[p].frames[alt]))
                        { 
                            // stuffed - no texture
                            continue;
                        }
                    }

                }

                t = ogrePass->createTextureUnitState("");
                t->setAnimatedTextureName(pass[p].frames, pass[p].animNumFrames, sequenceTime);

            }
            else
            {
                // Quake3 can still include alternate extension filenames e.g. jpg instead of tga
                // Pain in the arse - have to check for failure
                if (!ResourceGroupManager::getSingleton().resourceExists(
                    resourceGroup, pass[p].textureName))
                {
                    // Try alternate extension
                    pass[p].textureName = getAlternateName(pass[p].textureName);
                    if (!ResourceGroupManager::getSingleton().resourceExists(
                        resourceGroup, pass[p].textureName))
                    { 
                        // stuffed - no texture
                        continue;
                    }
                }
                t = ogrePass->createTextureUnitState(pass[p].textureName);
            }
            // Blending
            if (p == 0)
            {
                // scene blend
                mat->setSceneBlending(pass[p].blendSrc, pass[p].blendDest);
                if (mat->isTransparent() && 
                    pass[p].blendSrc != SBF_SOURCE_ALPHA)
                    mat->setDepthWriteEnabled(false);

                t->setColourOperation(LBO_REPLACE);
				// Alpha mode
				ogrePass->setAlphaRejectSettings(
					pass[p].alphaFunc, pass[p].alphaVal);
            }
            else
            {
                if (pass[p].customBlend)
                {
                    // Fallback for now
                    t->setColourOperation(LBO_MODULATE);
                }
                else
                {
                    // simple layer blend
                    t->setColourOperation(pass[p].blend);
                }
				// Alpha mode, prefer 'most alphary'
				CompareFunction currFunc = ogrePass->getAlphaRejectFunction();
				unsigned char currVal = ogrePass->getAlphaRejectValue();
				if (pass[p].alphaFunc > currFunc ||
					(pass[p].alphaFunc == currFunc && pass[p].alphaVal < currVal))
				{
					ogrePass->setAlphaRejectSettings(
						pass[p].alphaFunc, pass[p].alphaVal);
				}
            }
            // Tex coords
            if (pass[p].texGen == TEXGEN_BASE)
            {
                t->setTextureCoordSet(0);
            }
            else if (pass[p].texGen == TEXGEN_LIGHTMAP)
            {
                t->setTextureCoordSet(1);
            }
            else if (pass[p].texGen == TEXGEN_ENVIRONMENT)
            {
                t->setEnvironmentMap(true, TextureUnitState::ENV_PLANAR);
            }
            // Tex mod
            // Scale
            t->setTextureUScale(pass[p].tcModScale[0]);
            t->setTextureVScale(pass[p].tcModScale[1]);
            // Procedural mods
            // Custom - don't use mod if generating environment
            // Because I do env a different way it look horrible
            if (pass[p].texGen != TEXGEN_ENVIRONMENT)
            {
                if (pass[p].tcModRotate)
                {
                    t->setRotateAnimation(pass[p].tcModRotate);
                }
                if (pass[p].tcModScroll[0] || pass[p].tcModScroll[1])
                {
                    if (pass[p].tcModTurbOn)
                    {
                        // Turbulent scroll
                        if (pass[p].tcModScroll[0])
                        {
                            t->setTransformAnimation(TextureUnitState::TT_TRANSLATE_U, WFT_SINE,
                                pass[p].tcModTurb[0], pass[p].tcModTurb[3], pass[p].tcModTurb[2], pass[p].tcModTurb[1]);
                        }
                        if (pass[p].tcModScroll[1])
                        {
                            t->setTransformAnimation(TextureUnitState::TT_TRANSLATE_V, WFT_SINE,
                                pass[p].tcModTurb[0], pass[p].tcModTurb[3], pass[p].tcModTurb[2], pass[p].tcModTurb[1]);
                        }
                    }
                    else
                    {
                        // Constant scroll
                        t->setScrollAnimation(pass[p].tcModScroll[0], pass[p].tcModScroll[1]);
                    }
                }
                if (pass[p].tcModStretchWave != SHADER_FUNC_NONE)
                {
                    WaveformType wft;
                    switch(pass[p].tcModStretchWave)
                    {
                    case SHADER_FUNC_SIN:
                        wft = WFT_SINE;
                        break;
                    case SHADER_FUNC_TRIANGLE:
                        wft = WFT_TRIANGLE;
                        break;
                    case SHADER_FUNC_SQUARE:
                        wft = WFT_SQUARE;
                        break;
                    case SHADER_FUNC_SAWTOOTH:
                        wft = WFT_SAWTOOTH;
                        break;
                    case SHADER_FUNC_INVERSESAWTOOTH:
                        wft = WFT_INVERSE_SAWTOOTH;
                        break;
                    default:
                        break;
                    }
                    // Create wave-based stretcher
                    t->setTransformAnimation(TextureUnitState::TT_SCALE_U, wft, pass[p].tcModStretchParams[3],
                        pass[p].tcModStretchParams[0], pass[p].tcModStretchParams[2], pass[p].tcModStretchParams[1]);
                    t->setTransformAnimation(TextureUnitState::TT_SCALE_V, wft, pass[p].tcModStretchParams[3],
                        pass[p].tcModStretchParams[0], pass[p].tcModStretchParams[2], pass[p].tcModStretchParams[1]);
                }
            }
            // Address mode
            t->setTextureAddressingMode(pass[p].addressMode);

            //assert(!t->isBlank());


        }
        // Do farbox (create new material)

        // Set culling mode and lighting to defaults
        mat->setCullingMode(CULL_NONE);
        mat->setManualCullingMode(cullMode);
        mat->setLightingEnabled(false);
        mat->load();
        return mat;
    }
    String Quake3Shader::getAlternateName(const String& texName)
    {
        // Get alternative JPG to TGA and vice versa
        size_t pos;
        String ext, base;

        pos = texName.find_last_of(".");
        ext = texName.substr(pos, 4);
		StringUtil::toLowerCase(ext);
        base = texName.substr(0,pos);
        if (ext == ".jpg")
        {
            return base + ".tga";
        }
        else
        {
            return base + ".jpg";
        }

    }
}
