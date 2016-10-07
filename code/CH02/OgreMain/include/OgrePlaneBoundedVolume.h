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
#ifndef __PlaneBoundedVolume_H_
#define __PlaneBoundedVolume_H_

// Precompiler options
#include "OgrePrerequisites.h"
#include "OgreAxisAlignedBox.h"
#include "OgreSphere.h"
#include "OgreMath.h"
#include "OgrePlane.h"

namespace Ogre {

    /** Represents a convex volume bounded by planes.
    */
    class _OgreExport PlaneBoundedVolume
    {
    public:
        typedef std::vector<Plane> PlaneList;
        /// Publicly accessible plane list, you can modify this direct
        PlaneList planes;
        Plane::Side outside;

        PlaneBoundedVolume() :outside(Plane::NEGATIVE_SIDE) {}
        /** Constructor, determines which side is deemed to be 'outside' */
        PlaneBoundedVolume(Plane::Side theOutside) 
            : outside(theOutside) {}

        /** Intersection test with AABB
        @remarks May return false positives but will never miss an intersection.
        */
        inline bool intersects(const AxisAlignedBox& box) const
        {
            if (box.isNull()) return false;
            // If all points are on outside of any plane, we fail
            const Vector3* points = box.getAllCorners();
            PlaneList::const_iterator i, iend;
            iend = planes.end();
            for (i = planes.begin(); i != iend; ++i)
            {
                const Plane& plane = *i;

                // Test which side of the plane the corners are
                // Intersection fails when at all corners are on the
                // outside of one plane
                bool splittingPlane = true;
                for (int corner = 0; corner < 8; ++corner)
                {
                    if (plane.getSide(points[corner]) != outside)
                    {
                        // this point is on the wrong side
                        splittingPlane = false;
                        break;
                    }
                }
                if (splittingPlane)
                {
                    // Found a splitting plane therefore return not intersecting
                    return false;
                }
            }

            // couldn't find a splitting plane, assume intersecting
            return true;

        }
        /** Intersection test with Sphere
        @remarks May return false positives but will never miss an intersection.
        */
        inline bool intersects(const Sphere& sphere) const
        {
            PlaneList::const_iterator i, iend;
            iend = planes.end();
            for (i = planes.begin(); i != iend; ++i)
            {
                const Plane& plane = *i;

                // Test which side of the plane the sphere is
                Real d = plane.getDistance(sphere.getCenter());
                // Negate d if planes point inwards
                if (outside == Plane::NEGATIVE_SIDE) d = -d;

                if ( (d - sphere.getRadius()) > 0)
                    return false;
            }

            return true;

        }

        /** Intersection test with a Ray
        @returns std::pair of hit (bool) and distance
        @remarks May return false positives but will never miss an intersection.
        */
        inline std::pair<bool, Real> intersects(const Ray& ray)
        {
            return Math::intersects(ray, planes, outside == Plane::POSITIVE_SIDE);
        }

    };

    typedef std::vector<PlaneBoundedVolume> PlaneBoundedVolumeList;


}

#endif

