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

#include "OgreSerializer.h"
#include "OgreLogManager.h"
#include "OgreDataStream.h"
#include "OgreException.h"
#include "OgreVector3.h"
#include "OgreQuaternion.h"


namespace Ogre {

    /// stream overhead = ID + size
    const size_t STREAM_OVERHEAD_SIZE = sizeof(uint16) + sizeof(uint32);
    const uint16 HEADER_STREAM_ID = 0x1000;
    const uint16 OTHER_ENDIAN_HEADER_STREAM_ID = 0x0010;
    //---------------------------------------------------------------------
    Serializer::Serializer()
    {
        // Version number
        mVersion = "[Serializer_v1.00]";
		mFlipEndian = false;
    }
    //---------------------------------------------------------------------
    Serializer::~Serializer()
    {
    }
    //---------------------------------------------------------------------
	void Serializer::determineEndianness(DataStreamPtr& stream)
	{
		if (stream->tell() != 0)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Can only determine the endianness of the input stream if it "
				"is at the start", "Serializer::determineEndianness");
		}
				
		uint16 dest;
		// read header id manually (no conversion)
        stream->read(&dest, sizeof(uint16));
		// skip back
		stream->skip(0 - sizeof(uint16));
		if (dest == HEADER_STREAM_ID)
		{
			mFlipEndian = false;
		}
		else if (dest == OTHER_ENDIAN_HEADER_STREAM_ID)
		{
			mFlipEndian = true;
		}
		else
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Can't find a header chunk to determine endianness",
				"Serializer::determineEndianness");
		}
	}
    //---------------------------------------------------------------------
	void Serializer::determineEndianness(Endian requestedEndian)
	{
		switch(requestedEndian)
		{
		case ENDIAN_NATIVE:
			mFlipEndian = false;
			break;
		case ENDIAN_BIG:
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
			mFlipEndian = false;
#else
			mFlipEndian = true;
#endif
			break;
		case ENDIAN_LITTLE:
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
			mFlipEndian = true;
#else
			mFlipEndian = false;
#endif
			break;
		}
	}
    //---------------------------------------------------------------------
    void Serializer::writeFileHeader(void)
    {
        
        uint16 val = HEADER_STREAM_ID;
        writeShorts(&val, 1);

        writeString(mVersion);

    }
    //---------------------------------------------------------------------
    void Serializer::writeChunkHeader(uint16 id, uint32 size)
    {
        writeShorts(&id, 1);
        writeInts(&size, 1);
    }
    //---------------------------------------------------------------------
    void Serializer::writeFloats(const float* const pFloat, size_t count)
    {
		if (mFlipEndian)
		{
            float * pFloatToWrite = (float *)malloc(sizeof(float) * count);
            memcpy(pFloatToWrite, pFloat, sizeof(float) * count);
            
            flipToLittleEndian(pFloatToWrite, sizeof(float), count);
            writeData(pFloatToWrite, sizeof(float), count);
            
            free(pFloatToWrite);
		}
		else
		{
            writeData(pFloat, sizeof(float), count);
		}
    }
    //---------------------------------------------------------------------
    void Serializer::writeFloats(const double* const pDouble, size_t count)
    {
		// Convert to float, then write
		float* tmp = new float[count];
		for (unsigned int i = 0; i < count; ++i)
		{
			tmp[i] = static_cast<float>(pDouble[i]);
		}
		if(mFlipEndian)
		{
            flipToLittleEndian(tmp, sizeof(float), count);
            writeData(tmp, sizeof(float), count);
		}
		else
		{
            writeData(tmp, sizeof(float), count);
		}
		delete [] tmp;
    }
    //---------------------------------------------------------------------
    void Serializer::writeShorts(const uint16* const pShort, size_t count = 1)
    {
		if(mFlipEndian)
		{
            unsigned short * pShortToWrite = (unsigned short *)malloc(sizeof(unsigned short) * count);
            memcpy(pShortToWrite, pShort, sizeof(unsigned short) * count);
            
            flipToLittleEndian(pShortToWrite, sizeof(unsigned short), count);
            writeData(pShortToWrite, sizeof(unsigned short), count);
            
            free(pShortToWrite);
		}
		else
		{
            writeData(pShort, sizeof(unsigned short), count);
		}
    }
    //---------------------------------------------------------------------
    void Serializer::writeInts(const uint32* const pInt, size_t count = 1)
    {
		if(mFlipEndian)
		{
            unsigned int * pIntToWrite = (unsigned int *)malloc(sizeof(unsigned int) * count);
            memcpy(pIntToWrite, pInt, sizeof(unsigned int) * count);
            
            flipToLittleEndian(pIntToWrite, sizeof(unsigned int), count);
            writeData(pIntToWrite, sizeof(unsigned int), count);
            
            free(pIntToWrite);
		}
		else
		{
            writeData(pInt, sizeof(unsigned int), count);
		}
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    void Serializer::writeBools(const bool* const pBool, size_t count = 1)
    {
    //no endian flipping for 1-byte bools
    //XXX Nasty Hack to convert to 1-byte bools
#	if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        char * pCharToWrite = (char *)malloc(sizeof(char) * count);
        for(int i = 0; i < count; i++)
        {
            *(char *)(pCharToWrite + i) = *(bool *)(pBool + i);
        }
        
        writeData(pCharToWrite, sizeof(char), count);
        
        free(pCharToWrite);
#	else
        writeData(pBool, sizeof(bool), count);
#	endif

    }
    
    //---------------------------------------------------------------------
    void Serializer::writeData(const void* const buf, size_t size, size_t count)
    {
        fwrite((void* const)buf, size, count, mpfFile);
    }
    //---------------------------------------------------------------------
    void Serializer::writeString(const String& string)
    {
        fputs(string.c_str(), mpfFile);
        // Write terminating newline char
        fputc('\n', mpfFile);
    }
    //---------------------------------------------------------------------
    void Serializer::readFileHeader(DataStreamPtr& stream)
    {
        unsigned short headerID;
        
        // Read header ID
        readShorts(stream, &headerID, 1);
        
        if (headerID == HEADER_STREAM_ID)
        {
            // Read version
            String ver = readString(stream);
            if (ver != mVersion)
            {
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, 
                    "Invalid file: version incompatible, file reports " + String(ver) +
                    " Serializer is version " + mVersion,
                    "Serializer::readFileHeader");
            }
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Invalid file: no header", 
                "Serializer::readFileHeader");
        }

    }
    //---------------------------------------------------------------------
    unsigned short Serializer::readChunk(DataStreamPtr& stream)
    {
        unsigned short id;
        readShorts(stream, &id, 1);
        
        readInts(stream, &mCurrentstreamLen, 1);
        return id;
    }
    //---------------------------------------------------------------------
    void Serializer::readBools(DataStreamPtr& stream, bool* pDest, size_t count)
    {
        //XXX Nasty Hack to convert 1 byte bools to 4 byte bools
#	if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        char * pTemp = (char *)malloc(1*count); // to hold 1-byte bools
        stream->read(pTemp, 1 * count);
        for(int i = 0; i < count; i++)
            *(bool *)(pDest + i) = *(char *)(pTemp + i);
            
        free (pTemp);
#	else
        stream->read(pDest, sizeof(bool) * count);
#	endif
        //no flipping on 1-byte datatypes
    }
    //---------------------------------------------------------------------
    void Serializer::readFloats(DataStreamPtr& stream, float* pDest, size_t count)
    {
        stream->read(pDest, sizeof(float) * count);
        flipFromLittleEndian(pDest, sizeof(float), count);
    }
    //---------------------------------------------------------------------
    void Serializer::readFloats(DataStreamPtr& stream, double* pDest, size_t count)
    {
		// Read from float, convert to double
		float* tmp = new float[count];
		float* ptmp = tmp;
        stream->read(tmp, sizeof(float) * count);
        flipFromLittleEndian(tmp, sizeof(float), count);
		// Convert to doubles (no cast required)
		while(count--)
		{
			*pDest++ = *ptmp++;
		}
		delete [] tmp;
    }
    //---------------------------------------------------------------------
    void Serializer::readShorts(DataStreamPtr& stream, unsigned short* pDest, size_t count)
    {
        stream->read(pDest, sizeof(unsigned short) * count);
        flipFromLittleEndian(pDest, sizeof(unsigned short), count);
    }
    //---------------------------------------------------------------------
    void Serializer::readInts(DataStreamPtr& stream, unsigned int* pDest, size_t count)
    {
        stream->read(pDest, sizeof(unsigned int) * count);
        flipFromLittleEndian(pDest, sizeof(unsigned int), count);
    }
    //---------------------------------------------------------------------
    String Serializer::readString(DataStreamPtr& stream, size_t numChars)
    {
        assert (numChars <= 255);
        char str[255];
        stream->read(str, numChars);
        str[numChars] = '\0';
        return str;
    }
    //---------------------------------------------------------------------
    String Serializer::readString(DataStreamPtr& stream)
    {
        return stream->getLine(false);
    }
    //---------------------------------------------------------------------
    void Serializer::writeObject(const Vector3& vec)
    {
        writeFloats(&vec.x, 1);
        writeFloats(&vec.y, 1);
        writeFloats(&vec.z, 1);

    }
    //---------------------------------------------------------------------
    void Serializer::writeObject(const Quaternion& q)
    {
        writeFloats(&q.x, 1);
        writeFloats(&q.y, 1);
        writeFloats(&q.z, 1);
        writeFloats(&q.w, 1);
    }
    //---------------------------------------------------------------------
    void Serializer::readObject(DataStreamPtr& stream, Vector3& pDest)
    {
        readFloats(stream, &pDest.x, 1);
        readFloats(stream, &pDest.y, 1);
        readFloats(stream, &pDest.z, 1);
    }
    //---------------------------------------------------------------------
    void Serializer::readObject(DataStreamPtr& stream, Quaternion& pDest)
    {
        readFloats(stream, &pDest.x, 1);
        readFloats(stream, &pDest.y, 1);
        readFloats(stream, &pDest.z, 1);
        readFloats(stream, &pDest.w, 1);
    }
    //---------------------------------------------------------------------


    void Serializer::flipToLittleEndian(void* pData, size_t size, size_t count)
    {
		if(mFlipEndian)
		{
	        flipEndian(pData, size, count);
		}
    }
    
    void Serializer::flipFromLittleEndian(void* pData, size_t size, size_t count)
    {
		if(mFlipEndian)
		{
	        flipEndian(pData, size, count);
		}
    }
    
    void Serializer::flipEndian(void * pData, size_t size, size_t count)
    {
        for(unsigned int index = 0; index < count; index++)
        {
            flipEndian((void *)((long)pData + (index * size)), size);
        }
    }
    
    void Serializer::flipEndian(void * pData, size_t size)
    {
        char swapByte;
        for(unsigned int byteIndex = 0; byteIndex < size/2; byteIndex++)
        {
            swapByte = *(char *)((long)pData + byteIndex);
            *(char *)((long)pData + byteIndex) = *(char *)((long)pData + size - byteIndex - 1);
            *(char *)((long)pData + size - byteIndex - 1) = swapByte;
        }
    }
    
}

