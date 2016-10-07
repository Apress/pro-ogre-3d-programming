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

#include "OgreImage.h"
#include "OgreCodec.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreILImageCodec.h"
#include "OgreILCodecs.h"

#include <IL/il.h>
#include <IL/ilu.h>

namespace Ogre {
    std::list<ILImageCodec*> ILCodecs::codeclist;

	// Return IL type for a certain extension
	ILenum ogreIlTypeFromExt(const String &ext)
	{
#ifdef IL_TGA
		if (ext=="tga" || ext=="vda" ||
			ext=="icb" || ext=="vst")
			return IL_TGA;
#endif
#ifdef IL_JPG
		if (ext=="jpg" || ext=="jpe" || ext=="jpeg")
			return IL_JPG;
#endif
#ifdef IL_DDS
		if (ext=="dds")
			return IL_DDS;
#endif
#ifdef IL_PNG
		if (ext=="png")
			return IL_PNG;
#endif
#ifdef IL_BMP
		if (ext=="bmp" || ext=="dib")
			return IL_BMP;
#endif
#ifdef IL_GIF
		if (ext=="gif")
			return IL_GIF;
#endif
#ifdef IL_CUT
		if (ext=="cut")
			return IL_CUT;
#endif
#ifdef IL_HDR
		if (ext=="hdr")
			return IL_HDR;
#endif
#ifdef IL_ICO
		if (ext=="ico" || ext=="cur")
			return IL_ICO;
#endif
#ifdef IL_JNG
		if (ext=="jng")
			return IL_JNG;
#endif
#ifdef IL_LIF
		if (ext=="lif")
			return IL_LIF;
#endif
#ifdef IL_MDL
		if (ext=="mdl")
			return IL_MDL;
#endif
#ifdef IL_MNG
		if (ext=="mng" || ext=="jng")
			return IL_MNG;
#endif
#ifdef IL_PCD
		if (ext=="pcd")
			return IL_PCD;
#endif
#ifdef IL_PCX
		if (ext=="pcx")
			return IL_PCX;
#endif
#ifdef IL_PIC
		if (ext=="pic")
			return IL_PIC;
#endif
#ifdef IL_PIX
		if (ext=="pix")
			return IL_PIX;
#endif
#ifdef IL_PNM
		if (ext=="pbm" || ext=="pgm" ||
			ext=="pnm" || ext=="ppm")
			return IL_PNM;
#endif
#ifdef IL_PSD
		if (ext=="psd" || ext=="pdd")
			return IL_PSD;
#endif
#ifdef IL_PSP
		if (ext=="psp")
			return IL_PSP;
#endif
#ifdef IL_PXR
		if (ext=="pxr")
			return IL_PXR;
#endif
#ifdef IL_SGI
		if (ext=="sgi" || ext=="bw" ||
			ext=="rgb" || ext=="rgba")
			return IL_SGI;
#endif
#ifdef IL_TIF
		if (ext=="tif" || ext=="tiff")
			return IL_TIF;
#endif
#ifdef IL_WAL
		if (ext=="wal")
			return IL_WAL;
#endif
#ifdef IL_XPM
		if (ext=="xpm")
			return IL_XPM;
#endif
	
		return IL_TYPE_UNKNOWN;
	}

	
    void ILCodecs::registerCodecs(void) {
		const char *il_version = ilGetString ( IL_VERSION_NUM );
		if( ilGetError() != IL_NO_ERROR )
		{
			// IL defined the version number as IL_VERSION in older versions, so we have to scan for it
			for(int ver=150; ver<170; ver++)
			{
				il_version = ilGetString ( ver ); 
				if(ilGetError() == IL_NO_ERROR)
					break;
				else
					il_version = "Unknown";
			}
		}
		LogManager::getSingleton().logMessage(
         LML_NORMAL,
            "DevIL version: " + String(il_version));
        const char *il_extensions = ilGetString ( IL_LOAD_EXT );
		if( ilGetError() != IL_NO_ERROR )
			il_extensions = "";
        
        std::stringstream ext;
        String str, all;
        ext << il_extensions;
        while(ext >> str)
        {
#if 0
			String fileName = "dummy." + str;
			// DevIL doesn't export this under windows -- how weird
			int ilType = ilTypeFromext(const_cast<char*>(fileName.c_str()));
#endif
			int ilType = ogreIlTypeFromExt(str);
            ILImageCodec *codec = new ILImageCodec(str, ilType);
            Codec::registerCodec(codec);
            codeclist.push_back(codec);
			//all += str+String("(")+StringConverter::toString(ilType)+String(") ");
			all += str+String(" ");
        }
		// Raw format, missing in image formats string
		ILImageCodec *cod = new ILImageCodec("raw", IL_RAW);
		Codec::registerCodec(cod);
		codeclist.push_back(cod);
		//all += String("raw")+"("+StringConverter::toString(IL_RAW)+String(") ");
		all += String("raw ");
		LogManager::getSingleton().logMessage(
         LML_NORMAL,
            "DevIL image formats: " + all);
    }

    void ILCodecs::deleteCodecs(void) {
        for(std::list<ILImageCodec*>::const_iterator i = codeclist.begin(); i != codeclist.end(); ++i)
        {
            Codec::unRegisterCodec(*i);   
            delete *i;
        }
        codeclist.clear();
    }

}

