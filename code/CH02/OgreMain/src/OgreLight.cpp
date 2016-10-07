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
#include "OgreStableHeaders.h"
#include "OgreLight.h"

#include "OgreException.h"
#include "OgreSceneNode.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    Light::Light()
		: mLightType(LT_POINT),
          mPosition(Vector3::ZERO),
          mDiffuse(ColourValue::White),
          mSpecular(ColourValue::Black),
          mDirection(Vector3::UNIT_Z),
		  mSpotOuter(Degree(40.0f)),
          mSpotInner(Degree(30.0f)),
          mSpotFalloff(1.0f),
		  mRange(100000),
		  mAttenuationConst(1.0f),
		  mAttenuationLinear(0.0f),
          mAttenuationQuad(0.0f),
		  mPowerScale(1.0f),
          mDerivedPosition(Vector3::ZERO),
          mDerivedDirection(Vector3::UNIT_Z)
    {
        // Default to point light, white diffuse light, linear attenuation, fair range

        mParentNode = NULL;
        mLocalTransformDirty = false;

    }
    //-----------------------------------------------------------------------
	Light::Light(const String& name) : MovableObject(name),
        mLightType(LT_POINT),
        mPosition(Vector3::ZERO),
        mDiffuse(ColourValue::White),
        mSpecular(ColourValue::Black),
        mDirection(Vector3::UNIT_Z),
		mSpotOuter(Degree(40.0f)),
        mSpotInner(Degree(30.0f)),
        mSpotFalloff(1.0f),
		mRange(100000),
		mAttenuationConst(1.0f),
		mAttenuationLinear(0.0f),
        mAttenuationQuad(0.0f),
		mPowerScale(1.0f),
        mDerivedPosition(Vector3::ZERO),
        mDerivedDirection(Vector3::UNIT_Z)
    {

        mParentNode = NULL;

    }
    //-----------------------------------------------------------------------
    Light::~Light()
    {
    }
    //-----------------------------------------------------------------------
    void Light::setType(LightTypes type)
    {
        mLightType = type;
    }
    //-----------------------------------------------------------------------
    Light::LightTypes Light::getType(void) const
    {
        return mLightType;
    }
    //-----------------------------------------------------------------------
    void Light::setPosition(Real x, Real y, Real z)
    {
        mPosition.x = x;
        mPosition.y = y;
        mPosition.z = z;
        mLocalTransformDirty = true;

    }
    //-----------------------------------------------------------------------
    void Light::setPosition(const Vector3& vec)
    {
        mPosition = vec;
        mLocalTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getPosition(void) const
    {
        return mPosition;
    }
    //-----------------------------------------------------------------------
    void Light::setDirection(Real x, Real y, Real z)
    {
        mDirection.x = x;
        mDirection.y = y;
        mDirection.z = z;
        mLocalTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    void Light::setDirection(const Vector3& vec)
    {
        mDirection = vec;
        mLocalTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDirection(void) const
    {
        return mDirection;
    }
    //-----------------------------------------------------------------------
    void Light::setSpotlightRange(const Radian& innerAngle, const Radian& outerAngle, Real falloff)
    {

        if (mLightType != LT_SPOTLIGHT)
            OGRE_EXCEPT(9999,
                "setSpotlightRange is only valid for spotlights.",
                "Light::setSpotlightRange");

        mSpotInner =innerAngle;
        mSpotOuter = outerAngle;
        mSpotFalloff = falloff;
    }
	//-----------------------------------------------------------------------
	void Light::setSpotlightInnerAngle(const Radian& val)
	{
		mSpotInner = val;
	}
	//-----------------------------------------------------------------------
	void Light::setSpotlightOuterAngle(const Radian& val)
	{
		mSpotOuter = val;
	}
	//-----------------------------------------------------------------------
	void Light::setSpotlightFalloff(Real val)
	{
		mSpotFalloff = val;
	}
    //-----------------------------------------------------------------------
    const Radian& Light::getSpotlightInnerAngle(void) const
    {
        return mSpotInner;
    }
    //-----------------------------------------------------------------------
    const Radian& Light::getSpotlightOuterAngle(void) const
    {
        return mSpotOuter;
    }
    //-----------------------------------------------------------------------
    Real Light::getSpotlightFalloff(void) const
    {
        return mSpotFalloff;
    }
    //-----------------------------------------------------------------------
    void Light::setDiffuseColour(Real red, Real green, Real blue)
    {
        mDiffuse.r = red;
        mDiffuse.b = blue;
        mDiffuse.g = green;
    }
    //-----------------------------------------------------------------------
    void Light::setDiffuseColour(const ColourValue& colour)
    {
        mDiffuse = colour;
    }
    //-----------------------------------------------------------------------
    const ColourValue& Light::getDiffuseColour(void) const
    {
        return mDiffuse;
    }
    //-----------------------------------------------------------------------
    void Light::setSpecularColour(Real red, Real green, Real blue)
    {
        mSpecular.r = red;
        mSpecular.b = blue;
        mSpecular.g = green;
    }
    //-----------------------------------------------------------------------
    void Light::setSpecularColour(const ColourValue& colour)
    {
        mSpecular = colour;
    }
    //-----------------------------------------------------------------------
    const ColourValue& Light::getSpecularColour(void) const
    {
        return mSpecular;
    }
    //-----------------------------------------------------------------------
    void Light::setAttenuation(Real range, Real constant,
                        Real linear, Real quadratic)
    {
        mRange = range;
        mAttenuationConst = constant;
        mAttenuationLinear = linear;
        mAttenuationQuad = quadratic;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationRange(void) const
    {
        return mRange;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationConstant(void) const
    {
        return mAttenuationConst;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationLinear(void) const
    {
        return mAttenuationLinear;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationQuadric(void) const
    {
        return mAttenuationQuad;
    }
    //-----------------------------------------------------------------------
	void Light::setPowerScale(Real power)
	{
		mPowerScale = power;
	}
    //-----------------------------------------------------------------------
	Real Light::getPowerScale(void) const
	{
		return mPowerScale;
	}
    //-----------------------------------------------------------------------
    void Light::update(void) const
    {
        if (mParentNode)
        {
            if (!(mParentNode->_getDerivedOrientation() == mLastParentOrientation &&
                mParentNode->_getDerivedPosition() == mLastParentPosition)
                || mLocalTransformDirty)
            {
                // Ok, we're out of date with SceneNode we're attached to
                mLastParentOrientation = mParentNode->_getDerivedOrientation();
                mLastParentPosition = mParentNode->_getDerivedPosition();
                mDerivedDirection = mLastParentOrientation * mDirection;
                mDerivedPosition = (mLastParentOrientation * mPosition) + mLastParentPosition;
            }
        }
        else
        {
            mDerivedPosition = mPosition;
            mDerivedDirection = mDirection;
        }

        mLocalTransformDirty = false;

    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& Light::getBoundingBox(void) const
    {
        // Null, lights are not visible
        static AxisAlignedBox box;
        return box;
    }
    //-----------------------------------------------------------------------
    void Light::_updateRenderQueue(RenderQueue* queue)
    {
        // Do nothing
    }
    //-----------------------------------------------------------------------
    const String& Light::getMovableType(void) const
    {
		return LightFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDerivedPosition(void) const
    {
        update();
        return mDerivedPosition;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDerivedDirection(void) const
    {
        update();
        return mDerivedDirection;
    }
    //-----------------------------------------------------------------------
    void Light::setVisible(bool visible)
    {
        MovableObject::setVisible(visible);
    }
    //-----------------------------------------------------------------------
	Vector4 Light::getAs4DVector(void) const
	{
		Vector4 ret;
        if (mLightType == Light::LT_DIRECTIONAL)
        {
            ret = -(getDerivedDirection()); // negate direction as 'position'
            ret.w = 0.0; // infinite distance
        }	
		else
        {
            ret = getDerivedPosition();
            ret.w = 1.0;
        }
		return ret;
	}
    //-----------------------------------------------------------------------
    const PlaneBoundedVolume& Light::_getNearClipVolume(const Camera* const cam) const
    {
        // First check if the light is close to the near plane, since
        // in this case we have to build a degenerate clip volume
        mNearClipVolume.planes.clear();
        mNearClipVolume.outside = Plane::NEGATIVE_SIDE;

        Real n = cam->getNearClipDistance();
        // Homogenous position
        Vector4 lightPos = getAs4DVector();
        // 3D version (not the same as _getDerivedPosition, is -direction for
        // directional lights)
        Vector3 lightPos3 = Vector3(lightPos.x, lightPos.y, lightPos.z);

        // Get eye-space light position
        // use 4D vector so directional lights still work
        Vector4 eyeSpaceLight = cam->getViewMatrix() * lightPos;
        // Find distance to light, project onto -Z axis
        Real d = eyeSpaceLight.dotProduct(
            Vector4(0, 0, -1, -n) );
        #define THRESHOLD 1e-6
        if (d > THRESHOLD || d < -THRESHOLD)
        {
            // light is not too close to the near plane
            // First find the worldspace positions of the corners of the viewport
            const Vector3 *corner = cam->getWorldSpaceCorners();
            int winding = (d < 0) ^ cam->isReflected() ? +1 : -1;
            // Iterate over world points and form side planes
            Vector3 normal;
            Vector3 lightDir;
            for (unsigned int i = 0; i < 4; ++i)
            {
                // Figure out light dir
                lightDir = lightPos3 - (corner[i] * lightPos.w);
                // Cross with anticlockwise corner, therefore normal points in
                normal = (corner[i] - corner[(i+winding)%4])
                    .crossProduct(lightDir);
                normal.normalise();
                mNearClipVolume.planes.push_back(Plane(normal, corner[i]));
            }

            // Now do the near plane plane
            normal = cam->getFrustumPlane(FRUSTUM_PLANE_NEAR).normal;
            if (d < 0)
            {
                // Behind near plane
                normal = -normal;
            }
            const Vector3& cameraPos = cam->getDerivedPosition();
            mNearClipVolume.planes.push_back(Plane(normal, cameraPos));

            // Finally, for a point/spot light we can add a sixth plane
            // This prevents false positives from behind the light
            if (mLightType != LT_DIRECTIONAL)
            {
                // Direction from light perpendicular to near plane
                mNearClipVolume.planes.push_back(Plane(-normal, lightPos3));
            }
        }
        else
        {
            // light is close to being on the near plane
            // degenerate volume including the entire scene 
            // we will always require light / dark caps
            mNearClipVolume.planes.push_back(Plane(Vector3::UNIT_Z, -n));
            mNearClipVolume.planes.push_back(Plane(-Vector3::UNIT_Z, n));
        }

        return mNearClipVolume;
    }
    //-----------------------------------------------------------------------
    const PlaneBoundedVolumeList& Light::_getFrustumClipVolumes(const Camera* const cam) const
    {

        // Homogenous light position
        Vector4 lightPos = getAs4DVector();
        // 3D version (not the same as _getDerivedPosition, is -direction for
        // directional lights)
        Vector3 lightPos3 = Vector3(lightPos.x, lightPos.y, lightPos.z);

        const Vector3 *clockwiseVerts[4];

        // Get worldspace frustum corners
        const Vector3* corners = cam->getWorldSpaceCorners();
        int winding = cam->isReflected() ? +1 : -1;

        bool infiniteViewDistance = (cam->getFarClipDistance() == 0);

        mFrustumClipVolumes.clear();
        for (unsigned short n = 0; n < 6; ++n)
        {
            // Skip far plane if infinite view frustum
            if (infiniteViewDistance && n == FRUSTUM_PLANE_FAR)
                continue;

            const Plane& plane = cam->getFrustumPlane(n);
            Vector4 planeVec(plane.normal.x, plane.normal.y, plane.normal.z, plane.d);
            // planes face inwards, we need to know if light is on negative side
            Real d = planeVec.dotProduct(lightPos);
            if (d < -1e-06)
            {
                // Ok, this is a valid one
                // clockwise verts mean we can cross-product and always get normals
                // facing into the volume we create

                mFrustumClipVolumes.push_back(PlaneBoundedVolume());
                PlaneBoundedVolume& vol = mFrustumClipVolumes.back();
                switch(n)
                {
                case(FRUSTUM_PLANE_NEAR):
                    clockwiseVerts[0] = corners + 3;
                    clockwiseVerts[1] = corners + 2;
                    clockwiseVerts[2] = corners + 1;
                    clockwiseVerts[3] = corners + 0;
                    break;
                case(FRUSTUM_PLANE_FAR):
                    clockwiseVerts[0] = corners + 7;
                    clockwiseVerts[1] = corners + 6;
                    clockwiseVerts[2] = corners + 5;
                    clockwiseVerts[3] = corners + 4;
                    break;
                case(FRUSTUM_PLANE_LEFT):
                    clockwiseVerts[0] = corners + 2;
                    clockwiseVerts[1] = corners + 6;
                    clockwiseVerts[2] = corners + 5;
                    clockwiseVerts[3] = corners + 1;
                    break;
                case(FRUSTUM_PLANE_RIGHT):
                    clockwiseVerts[0] = corners + 7;
                    clockwiseVerts[1] = corners + 3;
                    clockwiseVerts[2] = corners + 0;
                    clockwiseVerts[3] = corners + 4;
                    break;
                case(FRUSTUM_PLANE_TOP):
                    clockwiseVerts[0] = corners + 0;
                    clockwiseVerts[1] = corners + 1;
                    clockwiseVerts[2] = corners + 5;
                    clockwiseVerts[3] = corners + 4;
                    break;
                case(FRUSTUM_PLANE_BOTTOM):
                    clockwiseVerts[0] = corners + 7;
                    clockwiseVerts[1] = corners + 6;
                    clockwiseVerts[2] = corners + 2;
                    clockwiseVerts[3] = corners + 3;
                    break;
                };

                // Build a volume
                // Iterate over world points and form side planes
                Vector3 normal;
                Vector3 lightDir;
                for (unsigned int i = 0; i < 4; ++i)
                {
                    // Figure out light dir
                    lightDir = lightPos3 - (*(clockwiseVerts[i]) * lightPos.w);
                    Vector3 edgeDir = *(clockwiseVerts[i]) - *(clockwiseVerts[(i+winding)%4]);
                    // Cross with anticlockwise corner, therefore normal points in
                    normal = edgeDir.crossProduct(lightDir);
                    normal.normalise();
                    vol.planes.push_back(Plane(normal, *(clockwiseVerts[i])));
                }

                // Now do the near plane (this is the plane of the side we're 
                // talking about, with the normal inverted (d is already interpreted as -ve)
                vol.planes.push_back( Plane(-plane.normal, plane.d) );

                // Finally, for a point/spot light we can add a sixth plane
                // This prevents false positives from behind the light
                if (mLightType != LT_DIRECTIONAL)
                {
                    // re-use our own plane normal
                    vol.planes.push_back(Plane(plane.normal, lightPos3));
                }
            }
        }

        return mFrustumClipVolumes;
    }
	//-----------------------------------------------------------------------
	uint32 Light::getTypeFlags(void) const
	{
		return SceneManager::LIGHT_TYPE_MASK;
	}
	//-----------------------------------------------------------------------
	const String& Light::getAnimableDictionaryName(void) const
	{
		return LightFactory::FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------
	void Light::initialiseAnimableDictionary(StringVector& vec) const
	{
		vec.push_back("diffuseColour");
		vec.push_back("specularColour");
		vec.push_back("attenuation");
		vec.push_back("spotlightInner");
		vec.push_back("spotlightOuter");
		vec.push_back("spotlightFalloff");

	}
	//-----------------------------------------------------------------------
	class LightDiffuseColourValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightDiffuseColourValue(Light* l) :AnimableValue(COLOUR) 
		{ mLight = l; }
		void setValue(const ColourValue& val)
		{
			mLight->setDiffuseColour(val);
		}
		void applyDeltaValue(const ColourValue& val)
		{
			setValue(mLight->getDiffuseColour() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getDiffuseColour());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpecularColourValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpecularColourValue(Light* l) :AnimableValue(COLOUR) 
		{ mLight = l; }
		void setValue(const ColourValue& val)
		{
			mLight->setSpecularColour(val);
		}
		void applyDeltaValue(const ColourValue& val)
		{
			setValue(mLight->getSpecularColour() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpecularColour());
		}

	};
	//-----------------------------------------------------------------------
	class LightAttenuationValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightAttenuationValue(Light* l) :AnimableValue(VECTOR4) 
		{ mLight = l; }
		void setValue(const Vector4& val)
		{
			mLight->setAttenuation(val.x, val.y, val.z, val.w);
		}
		void applyDeltaValue(const Vector4& val)
		{
			setValue(mLight->getAs4DVector() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getAs4DVector());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightInnerValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightInnerValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightInnerAngle(Radian(val));
		}
		void applyDeltaValue(const Real& val)
		{
			setValue(mLight->getSpotlightInnerAngle().valueRadians() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightInnerAngle().valueRadians());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightOuterValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightOuterValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightOuterAngle(Radian(val));
		}
		void applyDeltaValue(const Real& val)
		{
			setValue(mLight->getSpotlightOuterAngle().valueRadians() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightOuterAngle().valueRadians());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightFalloffValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightFalloffValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightFalloff(val);
		}
		void applyDeltaValue(const Real& val)
		{
			setValue(mLight->getSpotlightFalloff() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightFalloff());
		}

	};
	//-----------------------------------------------------------------------
	AnimableValuePtr Light::createAnimableValue(const String& valueName)
	{
		if (valueName == "diffuseColour")
		{
			return AnimableValuePtr(
				new LightDiffuseColourValue(this));
		}
		else if(valueName == "specularColour")
		{
			return AnimableValuePtr(
				new LightSpecularColourValue(this));
		}
		else if (valueName == "attenuation")
		{
			return AnimableValuePtr(
				new LightAttenuationValue(this));
		}
		else if (valueName == "spotlightInner")
		{
			return AnimableValuePtr(
				new LightSpotlightInnerValue(this));
		}
		else if (valueName == "spotlightOuter")
		{
			return AnimableValuePtr(
				new LightSpotlightOuterValue(this));
		}
		else if (valueName == "spotlightFalloff")
		{
			return AnimableValuePtr(
				new LightSpotlightFalloffValue(this));
		}
		else
		{
			return MovableObject::createAnimableValue(valueName);
		}
	}
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	String LightFactory::FACTORY_TYPE_NAME = "Light";
	//-----------------------------------------------------------------------
	const String& LightFactory::getType(void) const
	{
		return FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------
	MovableObject* LightFactory::createInstanceImpl( const String& name, 
		const NameValuePairList* params)
	{

		return new Light(name);

	}
	//-----------------------------------------------------------------------
	void LightFactory::destroyInstance( MovableObject* obj)
	{
		delete obj;
	}




} // Namespace
