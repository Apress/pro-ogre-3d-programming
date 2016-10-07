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
#ifndef _O_IStream_H__
#define _O_IStream_H__

#include "OgreDataStream.h"
#include <ImfInt64.h>
#include <ImfIO.h>


namespace Ogre {
   
/**
 * Adapter class for feeding OGRE data chunks as input to Imf.
 */
class O_IStream: public Imf::IStream
{
public:

    O_IStream(MemoryDataStream& stream, const char fileName[]):
        IStream (fileName), _stream(stream) {}

    virtual bool    read (char c[], int n);
    virtual Imf::Int64   tellg ();
    virtual void    seekg (Imf::Int64 pos);
    virtual void    clear ();

private:
    MemoryDataStream& _stream;
};

};

#endif
