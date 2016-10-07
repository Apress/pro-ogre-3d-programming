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

#include "OgreMeshSerializer.h"
#include "OgreMeshFileFormat.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreSkeleton.h"

namespace Ogre {

    String MeshSerializer::msCurrentVersion = "[MeshSerializer_v1.30]";
    const unsigned short HEADER_CHUNK_ID = 0x1000;
    //---------------------------------------------------------------------
    MeshSerializer::MeshSerializer()
    {
        // Set up map
        mImplementations.insert(
            MeshSerializerImplMap::value_type("[MeshSerializer_v1.10]", 
            new MeshSerializerImpl_v1_1() ) );

        mImplementations.insert(
            MeshSerializerImplMap::value_type("[MeshSerializer_v1.20]", 
            new MeshSerializerImpl_v1_2() ) );

        mImplementations.insert(
            MeshSerializerImplMap::value_type(msCurrentVersion, 
            new MeshSerializerImpl() ) );
    }
    //---------------------------------------------------------------------
    MeshSerializer::~MeshSerializer()
    {
        // delete map
        for (MeshSerializerImplMap::iterator i = mImplementations.begin();
            i != mImplementations.end(); ++i)
        {
            delete i->second;
        }
        mImplementations.clear();

    }
    //---------------------------------------------------------------------
    void MeshSerializer::exportMesh(const Mesh* pMesh, const String& filename,
		Endian endianMode)
    {
        MeshSerializerImplMap::iterator impl = mImplementations.find(msCurrentVersion);
        if (impl == mImplementations.end())
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot find serializer implementation for "
                "current version " + msCurrentVersion, "MeshSerializer::exportMesh");
        }

        impl->second->exportMesh(pMesh, filename, endianMode);
    }
    //---------------------------------------------------------------------
    void MeshSerializer::importMesh(DataStreamPtr& stream, Mesh* pDest)
    {
        determineEndianness(stream);

        // Read header and determine the version
        unsigned short headerID;
        
        // Read header ID
        readShorts(stream, &headerID, 1);
        
        if (headerID != HEADER_CHUNK_ID)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "File header not found",
                "MeshSerializer::importMesh");
        }
        // Read version
        String ver = readString(stream);
        // Jump back to start
        stream->seek(0);

        // Find the implementation to use
        MeshSerializerImplMap::iterator impl = mImplementations.find(ver);
        if (impl == mImplementations.end())
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot find serializer implementation for "
                "current version " + ver, "MeshSerializer::importMesh");
        }

        // Call implementation
        impl->second->importMesh(stream, pDest);
        // Warn on old version of mesh
        if (ver != msCurrentVersion)
        {
            LogManager::getSingleton().logMessage("WARNING: " + pDest->getName() + 
                " is an older format (" + ver + "); you should upgrade it as soon as possible" +
                " using the OgreMeshUpgrade tool.");
        }

    }
    //---------------------------------------------------------------------

}

