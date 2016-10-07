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
#ifndef __SimpleRenderable_H__
#define __SimpleRenderable_H__

#include "OgrePrerequisites.h"

#include "OgreMovableObject.h"
#include "OgreRenderable.h"
#include "OgreAxisAlignedBox.h"
#include "OgreMaterial.h"

namespace Ogre {

    class _OgreExport SimpleRenderable : public MovableObject, public Renderable
    {
    protected:
        RenderOperation mRenderOp;

        Matrix4 m_matWorldTransform;
        AxisAlignedBox mBox;

        String m_strMatName;
        MaterialPtr m_pMaterial;

        /// The scene manager for the current frame.
        SceneManager *m_pParentSceneManager;

        /// The camera for the current frame.
        Camera *m_pCamera;

        /// Static member used to automatically generate names for SimpleRendaerable objects.
        static uint ms_uGenNameCount;

    public:
        SimpleRenderable();

        void setMaterial( const String& matName );
        virtual const MaterialPtr& getMaterial(void) const;

        virtual void setRenderOperation( const RenderOperation& rend );
        virtual void getRenderOperation(RenderOperation& op);

        void setWorldTransform( const Matrix4& xform );
        virtual void getWorldTransforms( Matrix4* xform ) const;
        /** @copydoc Renderable::getWorldOrientation */
        const Quaternion& getWorldOrientation(void) const;
        /** @copydoc Renderable::getWorldPosition */
        const Vector3& getWorldPosition(void) const;


        virtual void _notifyCurrentCamera(Camera* cam);

        void setBoundingBox( const AxisAlignedBox& box );
        virtual const AxisAlignedBox& getBoundingBox(void) const;

        virtual void _updateRenderQueue(RenderQueue* queue);

        virtual ~SimpleRenderable();


        /** Overridden from MovableObject */
        virtual const String& getMovableType(void) const;

        /** @copydoc Renderable::getLights */
        const LightList& getLights(void) const;

    };
}

#endif

