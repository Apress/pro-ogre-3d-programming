/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright ) 2002 Tels <http://bloodgate.com> based on CylinderEmitter
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
#ifndef __CylinderEmitter_H__
#define __CylinderEmitter_H__

#include "OgreParticleFXPrerequisites.h"
#include "OgreAreaEmitter.h"

namespace Ogre {

    /** Particle emitter which emits particles randomly from points inside a cylinder.
    @remarks
        This basic particle emitter emits particles from a cylinder area. The
        initial direction of these particles can either be a single direction
        (i.e. a line), a random scattering inside a cone, or a random
        scattering in all directions, depending the 'angle' parameter, which
        is the angle across which to scatter the particles either side of the
        base direction of the emitter. 
    */
    class _OgreParticleFXExport CylinderEmitter : public AreaEmitter
    {
    public:
        // See AreaEmitter

        CylinderEmitter(ParticleSystem* psys);

        /** See ParticleEmitter. */
        void _initParticle(Particle* pParticle);

    protected:
        // See AreaEmitter




    };

}

#endif

