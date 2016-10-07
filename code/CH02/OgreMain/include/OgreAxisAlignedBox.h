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
#ifndef __AxisAlignedBox_H_
#define __AxisAlignedBox_H_

// Precompiler options
#include "OgrePrerequisites.h"

#include "OgreVector3.h"
#include "OgreMatrix4.h"

namespace Ogre {

    /** A 3D box aligned with the x/y/z axes.
	    @remarks
		    This class represents a simple box which is aligned with the
		    axes. Internally it only stores 2 points as the extremeties of
		    the box, one which is the minima of all 3 axes, and the other
		    which is the maxima of all 3 axes. This class is typically used
		    for an axis-aligned bounding box (AABB) for collision and
		    visibility determination.
    */
    class _OgreExport AxisAlignedBox
    {
    protected:
	    Vector3 mMinimum;
	    Vector3 mMaximum;
	    bool mNull;

	    Vector3 mCorners[8];

	    /** Internal method for updating corner points.
	    */
	    void updateCorners(void)
	    {
		    // The order of these items is, using right-handed co-ordinates:
		    // Minimum Z face, starting with Min(all), then anticlockwise
		    //   around face (looking onto the face)
		    // Maximum Z face, starting with Max(all), then anticlockwise
		    //   around face (looking onto the face)
		    mCorners[0] = mMinimum;
		    mCorners[1].x = mMinimum.x; mCorners[1].y = mMaximum.y; mCorners[1].z = mMinimum.z;
		    mCorners[2].x = mMaximum.x; mCorners[2].y = mMaximum.y; mCorners[2].z = mMinimum.z;
		    mCorners[3].x = mMaximum.x; mCorners[3].y = mMinimum.y; mCorners[3].z = mMinimum.z;            

		    mCorners[4] = mMaximum;
		    mCorners[5].x = mMinimum.x; mCorners[5].y = mMaximum.y; mCorners[5].z = mMaximum.z;
		    mCorners[6].x = mMinimum.x; mCorners[6].y = mMinimum.y; mCorners[6].z = mMaximum.z;
		    mCorners[7].x = mMaximum.x; mCorners[7].y = mMinimum.y; mCorners[7].z = mMaximum.z;            
	    }        

    public:
	    inline AxisAlignedBox()
	    {
		    // Default to null box
		    setMinimum( -0.5, -0.5, -0.5 );
		    setMaximum( 0.5, 0.5, 0.5 );
		    mNull = true;
	    }

	    inline AxisAlignedBox( const Vector3& min, const Vector3& max )
	    {
		    setExtents( min, max );
	    }

	    inline AxisAlignedBox(
		    Real mx, Real my, Real mz,
		    Real Mx, Real My, Real Mz )
	    {
		    setExtents( mx, my, mz, Mx, My, Mz );
	    }

	    /** Gets the minimum corner of the box.
	    */
	    inline const Vector3& getMinimum(void) const
	    { 
		    return mMinimum; 
	    }

	    /** Gets the maximum corner of the box.
	    */
	    inline const Vector3& getMaximum(void) const
	    { 
		    return mMaximum;
	    }

	    /** Sets the minimum corner of the box.
	    */
	    inline void setMinimum( const Vector3& vec )
	    {
		    mNull = false;
		    mMinimum = vec;
		    updateCorners();
	    }

	    inline void setMinimum( Real x, Real y, Real z )
	    {
		    mNull = false;
		    mMinimum.x = x;
		    mMinimum.y = y;
		    mMinimum.z = z;
		    updateCorners();
	    }

	    /** Sets the maximum corner of the box.
	    */
	    inline void setMaximum( const Vector3& vec )
	    {
		    mNull = false;
		    mMaximum = vec;
		    updateCorners();
	    }

	    inline void setMaximum( Real x, Real y, Real z )
	    {
		    mNull = false;
		    mMaximum.x = x;
		    mMaximum.y = y;
		    mMaximum.z = z;
		    updateCorners();
	    }

	    /** Sets both minimum and maximum extents at once.
	    */
	    inline void setExtents( const Vector3& min, const Vector3& max )
	    {
		    mNull = false;
		    mMinimum = min;
		    mMaximum = max;
		    updateCorners();
	    }

	    inline void setExtents(
		    Real mx, Real my, Real mz,
		    Real Mx, Real My, Real Mz )
	    {
		    mNull = false;

		    mMinimum.x = mx;
		    mMinimum.y = my;
		    mMinimum.z = mz;

		    mMaximum.x = Mx;
		    mMaximum.y = My;
		    mMaximum.z = Mz;

		    updateCorners();
	    }

	    /** Returns a pointer to an array of 8 corner points, useful for
		    collision vs. non-aligned objects.
		    @remarks
			    If the order of these corners is important, they are as
			    follows: The 4 points of the minimum Z face (note that
			    because Ogre uses right-handed coordinates, the minimum Z is
			    at the 'back' of the box) starting with the minimum point of
			    all, then anticlockwise around this face (if you are looking
			    onto the face from outside the box). Then the 4 points of the
			    maximum Z face, starting with maximum point of all, then
			    anticlockwise around this face (looking onto the face from
			    outside the box). Like this:
			    <pre>
			       1-----2
			      /|    /|
			     / |   / |
			    5-----4  |
			    |  0--|--3
			    | /   | /
			    |/    |/
			    6-----7
			    </pre>
	    */
	    inline const Vector3* getAllCorners(void) const
	    {
		    assert( !mNull && "Can't get corners of a null AAB" );
		    return (const Vector3*)mCorners;
	    }

	    friend std::ostream& operator<<( std::ostream& o, AxisAlignedBox aab )
	    {
		    if (aab.isNull())
		    {
			    o << "AxisAlignedBox(null)";
		    }
		    else
		    {
			    o << "AxisAlignedBox(min=" << aab.mMinimum << ", max=" << aab.mMaximum;
			    o << ", corners=";
			    for (int i = 0; i < 7; ++i)
				    o << aab.mCorners[i] << ", ";
			    o << aab.mCorners[7] << ")";
		    }
		    return o;
	    }

	    /** Merges the passed in box into the current box. The result is the
		    box which encompasses both.
	    */
	    void merge( const AxisAlignedBox& rhs )
	    {
		    // Do nothing if rhs null
		    if (rhs.mNull)
		    {
			    return;
		    }
		    // Otherwise if current null, just take rhs
		    else if (mNull)
		    {
			    setExtents(rhs.mMinimum, rhs.mMaximum);
		    }
		    // Otherwise merge
		    else
		    {
			    Vector3 min = mMinimum;
			    Vector3 max = mMaximum;
			    max.makeCeil(rhs.mMaximum);
			    min.makeFloor(rhs.mMinimum);

			    setExtents(min, max);
		    }

	    }
		
		/** Extends the box to encompass the specified point (if needed).
		*/
		void merge( const Vector3& point )
		{
			if (mNull){ // if null, use this point
				setExtents(point, point);
			} else {
				mMaximum.makeCeil(point);
				mMinimum.makeFloor(point);
				updateCorners();
			}
		}

	    /** Transforms the box according to the matrix supplied.
		    @remarks
			    By calling this method you get the axis-aligned box which
			    surrounds the transformed version of this box. Therefore each
			    corner of the box is transformed by the matrix, then the
			    extents are mapped back onto the axes to produce another
			    AABB. Useful when you have a local AABB for an object which
			    is then transformed.
	    */
	    void transform( const Matrix4& matrix )
	    {
		    // Do nothing if current null
		    if( mNull )
			    return;

		    Vector3 min, max, temp;
		    bool first = true;
		    size_t i;

		    for( i = 0; i < 8; ++i )
		    {
			    // Transform and check extents
			    temp = matrix * mCorners[i];
			    if( first || temp.x > max.x )
				    max.x = temp.x;
			    if( first || temp.y > max.y )
				    max.y = temp.y;
			    if( first || temp.z > max.z )
				    max.z = temp.z;
			    if( first || temp.x < min.x )
				    min.x = temp.x;
			    if( first || temp.y < min.y )
				    min.y = temp.y;
			    if( first || temp.z < min.z )
				    min.z = temp.z;

			    first = false;
		    }

		    setExtents( min,max );

	    }

	    /** Sets the box to a 'null' value i.e. not a box.
	    */
	    inline void setNull()
	    {
		    mNull = true;
	    }

	    /** Returns true if the box is null i.e. empty.
	    */
	    bool isNull(void) const
	    {
		    return mNull;
	    }

        /** Returns whether or not this box intersects another. */
        inline bool intersects(const AxisAlignedBox& b2) const
        {
            // Early-fail for nulls
            if (this->isNull() || b2.isNull())
                return false;

            // Use up to 6 separating planes
            if (mMaximum.x < b2.mMinimum.x)
                return false;
            if (mMaximum.y < b2.mMinimum.y)
                return false;
            if (mMaximum.z < b2.mMinimum.z)
                return false;

            if (mMinimum.x > b2.mMaximum.x)
                return false;
            if (mMinimum.y > b2.mMaximum.y)
                return false;
            if (mMinimum.z > b2.mMaximum.z)
                return false;

            // otherwise, must be intersecting
            return true;

        }

		/// Calculate the area of intersection of this box and another
		inline AxisAlignedBox intersection(const AxisAlignedBox& b2) const
		{
			if (!this->intersects(b2))
			{
				return AxisAlignedBox();
			}
			Vector3 intMin, intMax;

			const Vector3& b2max = b2.getMaximum();
			const Vector3& b2min = b2.getMinimum();

			if (b2max.x > mMaximum.x && mMaximum.x > b2min.x)
				intMax.x = mMaximum.x;
			else 
				intMax.x = b2max.x;
			if (b2max.y > mMaximum.y && mMaximum.y > b2min.y)
				intMax.y = mMaximum.y;
			else 
				intMax.y = b2max.y;
			if (b2max.z > mMaximum.z && mMaximum.z > b2min.z)
				intMax.z = mMaximum.z;
			else 
				intMax.z = b2max.z;

			if (b2min.x < mMinimum.x && mMinimum.x < b2max.x)
				intMin.x = mMinimum.x;
			else
				intMin.x= b2min.x;
			if (b2min.y < mMinimum.y && mMinimum.y < b2max.y)
				intMin.y = mMinimum.y;
			else
				intMin.y= b2min.y;
			if (b2min.z < mMinimum.z && mMinimum.z < b2max.z)
				intMin.z = mMinimum.z;
			else
				intMin.z= b2min.z;

			return AxisAlignedBox(intMin, intMax);

		}

		/// Calculate the volume of this box
		Real volume(void) const
		{
			if (mNull)
			{
				return 0.0f;
			}
			else
			{
				Vector3 diff = mMaximum - mMinimum;
				return diff.x * diff.y * diff.z;
			}

		}

        /** Scales the AABB by the vector given. */
        inline void scale(const Vector3& s)
        {
            // NB assumes centered on origin
            Vector3 min = mMinimum * s;
            Vector3 max = mMaximum * s;
            setExtents(min, max);
        }

		/** Tests whether this box intersects a sphere. */
		bool intersects(const Sphere& s) const
		{
			return Math::intersects(s, *this); 
		}
		/** Tests whether this box intersects a plane. */
		bool intersects(const Plane& p) const
		{
			return Math::intersects(p, *this);
		}
        /** Tests whether the vector point is within this box. */
        bool intersects(const Vector3& v) const
        {
			return(v.x >= mMinimum.x  &&  v.x <= mMaximum.x  && 
			    v.y >= mMinimum.y  &&  v.y <= mMaximum.y  && 
    			v.z >= mMinimum.z  &&  v.z <= mMaximum.z);
        }
		/// Gets the centre of the box
		Vector3 getCenter(void) const
		{
			return Vector3((mMaximum + mMinimum) * 0.5);
		}


    };

} // namespace Ogre

#endif
