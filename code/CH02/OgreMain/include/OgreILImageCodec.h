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
#ifndef _ILImageCodec_H__
#define _ILImageCodec_H__

#include "OgreImageCodec.h"

namespace Ogre {

    /** Codec specialized in images loaded using DevIL.
        @remarks
            The users implementing subclasses of ImageCodec are required to return
            a valid pointer to a ImageData class from the decode(...) function.
    */
    class _OgreExport ILImageCodec : public ImageCodec
    {
    private:
		static bool _is_initialised;
        String mType;
        unsigned int mIlType;

    public:
        ILImageCodec(const String &type, unsigned int ilType);
        virtual ~ILImageCodec() { }

        /// @copydoc Codec::code
        DataStreamPtr code(MemoryDataStreamPtr& input, CodecDataPtr& pData) const;
        /// @copydoc Codec::codeToFile
        void codeToFile(MemoryDataStreamPtr& input, const String& outFileName, CodecDataPtr& pData) const;
        /// @copydoc Codec::decode
        DecodeResult decode(DataStreamPtr& input) const;

        /// Initialise DevIL
        void initialiseIL(void);
        
        virtual String getType() const;        
    };

} // namespace

#endif
