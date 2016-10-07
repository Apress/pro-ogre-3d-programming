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
#include "OgreColourFaderAffector2.h"
#include "OgreParticleSystem.h"
#include "OgreStringConverter.h"
#include "OgreParticle.h"


namespace Ogre {
    
    // init statics
	// Phase 1
    ColourFaderAffector2::CmdRedAdjust1 ColourFaderAffector2::msRedCmd1;
    ColourFaderAffector2::CmdGreenAdjust1 ColourFaderAffector2::msGreenCmd1;
    ColourFaderAffector2::CmdBlueAdjust1 ColourFaderAffector2::msBlueCmd1;
    ColourFaderAffector2::CmdAlphaAdjust1 ColourFaderAffector2::msAlphaCmd1;

	// Phase 2
	ColourFaderAffector2::CmdRedAdjust2 ColourFaderAffector2::msRedCmd2;
    ColourFaderAffector2::CmdGreenAdjust2 ColourFaderAffector2::msGreenCmd2;
    ColourFaderAffector2::CmdBlueAdjust2 ColourFaderAffector2::msBlueCmd2;
    ColourFaderAffector2::CmdAlphaAdjust2 ColourFaderAffector2::msAlphaCmd2;

	ColourFaderAffector2::CmdStateChange ColourFaderAffector2::msStateCmd;


    //-----------------------------------------------------------------------
    ColourFaderAffector2::ColourFaderAffector2(ParticleSystem* psys) : ParticleAffector(psys)
    {
        mRedAdj1 = mGreenAdj1 = mBlueAdj1 = mAlphaAdj1 = 0;
		mRedAdj2 = mGreenAdj2 = mBlueAdj2 = mAlphaAdj2 = 0;
        mType = "ColourFader2";
		StateChangeVal = 1;	// Switch when there is 1 second left on the TTL

        // Init parameters
        if (createParamDictionary("ColourFaderAffector2"))
        {
            ParamDictionary* dict = getParamDictionary();

			// Phase 1
            dict->addParameter(ParameterDef("red1", 
                "The amount by which to adjust the red component of particles per second.",
                PT_REAL), &msRedCmd1);
            dict->addParameter(ParameterDef("green1", 
                "The amount by which to adjust the green component of particles per second.",
                PT_REAL), &msGreenCmd1);
            dict->addParameter(ParameterDef("blue1", 
                "The amount by which to adjust the blue component of particles per second.",
                PT_REAL), &msBlueCmd1);
            dict->addParameter(ParameterDef("alpha1", 
                "The amount by which to adjust the alpha component of particles per second.",
                PT_REAL), &msAlphaCmd1);

			// Phase 2
            dict->addParameter(ParameterDef("red2", 
                "The amount by which to adjust the red component of particles per second.",
                PT_REAL), &msRedCmd2);
            dict->addParameter(ParameterDef("green2", 
                "The amount by which to adjust the green component of particles per second.",
                PT_REAL), &msGreenCmd2);
            dict->addParameter(ParameterDef("blue2", 
                "The amount by which to adjust the blue component of particles per second.",
                PT_REAL), &msBlueCmd2);
            dict->addParameter(ParameterDef("alpha2", 
                "The amount by which to adjust the alpha component of particles per second.",
                PT_REAL), &msAlphaCmd2);

			// State Change Value
            dict->addParameter(ParameterDef("state_change", 
                "When the particle has this much time to live left, it will switch to state 2.",
                PT_REAL), &msStateCmd);

        }
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::_affectParticles(ParticleSystem* pSystem, Real timeElapsed)
    {
        ParticleIterator pi = pSystem->_getIterator();
        Particle *p;
        float dr1, dg1, db1, da1;
		float dr2, dg2, db2, da2;

		// Scale adjustments by time
		dr1 = mRedAdj1   * timeElapsed;
		dg1 = mGreenAdj1 * timeElapsed;
		db1 = mBlueAdj1  * timeElapsed;
		da1 = mAlphaAdj1 * timeElapsed;

		// Scale adjustments by time
		dr2 = mRedAdj2   * timeElapsed;
		dg2 = mGreenAdj2 * timeElapsed;
		db2 = mBlueAdj2  * timeElapsed;
		da2 = mAlphaAdj2 * timeElapsed;

        while (!pi.end())
        {
			p = pi.getNext();

			if( p->timeToLive > StateChangeVal )
			{
				applyAdjustWithClamp(&p->colour.r, dr1);
				applyAdjustWithClamp(&p->colour.g, dg1);
				applyAdjustWithClamp(&p->colour.b, db1);
				applyAdjustWithClamp(&p->colour.a, da1);
			}
			else
			{
				applyAdjustWithClamp(&p->colour.r, dr2);
				applyAdjustWithClamp(&p->colour.g, dg2);
				applyAdjustWithClamp(&p->colour.b, db2);
				applyAdjustWithClamp(&p->colour.a, da2);
			}
        }

    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setAdjust1(float red, float green, float blue, float alpha)
    {
        mRedAdj1 = red;
        mGreenAdj1 = green;
        mBlueAdj1 = blue;
        mAlphaAdj1 = alpha;
    }
	//-----------------------------------------------------------------------
    void ColourFaderAffector2::setAdjust2(float red, float green, float blue, float alpha)
    {
        mRedAdj2 = red;
        mGreenAdj2 = green;
        mBlueAdj2 = blue;
        mAlphaAdj2 = alpha;
    }

    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setRedAdjust1(float red)
    {
        mRedAdj1 = red;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setRedAdjust2(float red)
    {
        mRedAdj2 = red;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getRedAdjust1(void) const
    {
        return mRedAdj1;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getRedAdjust2(void) const
    {
        return mRedAdj2;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setGreenAdjust1(float green)
    {
        mGreenAdj1 = green;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setGreenAdjust2(float green)
    {
        mGreenAdj2 = green;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getGreenAdjust1(void) const
    {
        return mGreenAdj1;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getGreenAdjust2(void) const
    {
        return mGreenAdj2;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setBlueAdjust1(float blue)
    {
        mBlueAdj1 = blue;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setBlueAdjust2(float blue)
    {
        mBlueAdj2 = blue;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getBlueAdjust1(void) const
    {
        return mBlueAdj1;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getBlueAdjust2(void) const
    {
        return mBlueAdj2;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setAlphaAdjust1(float alpha)
    {
        mAlphaAdj1 = alpha;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setAlphaAdjust2(float alpha)
    {
        mAlphaAdj2 = alpha;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getAlphaAdjust1(void) const
    {
        return mAlphaAdj1;
    }
    //-----------------------------------------------------------------------
    float ColourFaderAffector2::getAlphaAdjust2(void) const
    {
        return mAlphaAdj2;
    }
    //-----------------------------------------------------------------------
    void ColourFaderAffector2::setStateChange(Real NewValue)
    {
        StateChangeVal = NewValue;
    }
    //-----------------------------------------------------------------------
    Real ColourFaderAffector2::getStateChange(void) const
    {
        return StateChangeVal;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Command objects
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ColourFaderAffector2::CmdRedAdjust1::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getRedAdjust1() );
    }
    void ColourFaderAffector2::CmdRedAdjust1::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setRedAdjust1(
            StringConverter::parseReal(val));
    }
    String ColourFaderAffector2::CmdRedAdjust2::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getRedAdjust2() );
    }
    void ColourFaderAffector2::CmdRedAdjust2::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setRedAdjust2(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String ColourFaderAffector2::CmdGreenAdjust1::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getGreenAdjust1() );
    }
    void ColourFaderAffector2::CmdGreenAdjust1::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setGreenAdjust1(
            StringConverter::parseReal(val));
    }
    String ColourFaderAffector2::CmdGreenAdjust2::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getGreenAdjust2() );
    }
    void ColourFaderAffector2::CmdGreenAdjust2::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setGreenAdjust2(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String ColourFaderAffector2::CmdBlueAdjust1::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getBlueAdjust1() );
    }
    void ColourFaderAffector2::CmdBlueAdjust1::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setBlueAdjust1(
            StringConverter::parseReal(val));
    }
    String ColourFaderAffector2::CmdBlueAdjust2::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getBlueAdjust2() );
    }
    void ColourFaderAffector2::CmdBlueAdjust2::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setBlueAdjust2(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String ColourFaderAffector2::CmdAlphaAdjust1::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getAlphaAdjust1() );
    }
    void ColourFaderAffector2::CmdAlphaAdjust1::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setAlphaAdjust1(
            StringConverter::parseReal(val));
    }
    String ColourFaderAffector2::CmdAlphaAdjust2::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getAlphaAdjust2() );
    }
    void ColourFaderAffector2::CmdAlphaAdjust2::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setAlphaAdjust2(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String ColourFaderAffector2::CmdStateChange::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const ColourFaderAffector2*>(target)->getStateChange() );
    }
    void ColourFaderAffector2::CmdStateChange::doSet(void* target, const String& val)
    {
        static_cast<ColourFaderAffector2*>(target)->setStateChange(
            StringConverter::parseReal(val));
    }
}



