/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://ogre.sourceforge.net/

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
#include "OgreD3D9HardwareOcclusionQuery.h"
#include "OgreRenderSystemCapabilities.h"
#include "OgreException.h"

namespace Ogre {

	/**
	* This is a class that is the DirectX9 implementation of 
	* hardware occlusion testing.
	*
	* @author Lee Sandberg
	*
	* Updated on 12/7/2004 by Chris McGuirk
	* Updated on 4/8/2005 by Tuan Kuranes email: tuan.kuranes@free.fr
	*/

	/**
	* Default object constructor
	*/
    D3D9HardwareOcclusionQuery::D3D9HardwareOcclusionQuery( IDirect3DDevice9* pD3DDevice ) :
        mpDevice(pD3DDevice)
	{ 
		// create the occlusion query
		const HRESULT hr = mpDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mpQuery);

		if ( hr != D3D_OK ) 
		{	
            if( D3DERR_NOTAVAILABLE == hr)
	        {
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, 
                    "Cannot allocate a Hardware query. This video card doesn't supports it, sorry.", 
                    "D3D9HardwareOcclusionQuery::D3D9HardwareOcclusionQuery" );
            }			
		}
	}

	/**
	* Object destructor
	*/
	D3D9HardwareOcclusionQuery::~D3D9HardwareOcclusionQuery() 
	{ 
		SAFE_RELEASE(mpQuery); 
	}

	//------------------------------------------------------------------
	// Occlusion query functions (see base class documentation for this)
	//--
	void D3D9HardwareOcclusionQuery::beginOcclusionQuery() 
	{	    	
		mpQuery->Issue(D3DISSUE_BEGIN); 
        mIsQueryResultStillOutstanding = true;
        mPixelCount = 0;
	}

	void D3D9HardwareOcclusionQuery::endOcclusionQuery() 
	{ 
		mpQuery->Issue(D3DISSUE_END); 
	}

	//------------------------------------------------------------------
	bool D3D9HardwareOcclusionQuery::pullOcclusionQuery( unsigned int* NumOfFragments ) 
	{
        // in case you didn't check if query arrived and want the result now.
        if (mIsQueryResultStillOutstanding)
        {
            // Loop until the data becomes available
            DWORD pixels;
            const size_t dataSize = sizeof( DWORD );
			while (1)
            {
                const HRESULT hr = mpQuery->GetData((void *)&pixels, dataSize, D3DGETDATA_FLUSH);

                if  (hr == S_FALSE)
                    continue;
                if  (hr == S_OK)
                {
                    mPixelCount = pixels;
                    *NumOfFragments = pixels;
                    break;
                }
                if (hr == D3DERR_DEVICELOST)
                {
                    *NumOfFragments = 100000;
                    mPixelCount = 100000;
                    mpDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mpQuery);
                    break;
                }
            } 
            mIsQueryResultStillOutstanding = false;
        }
        else
        {
            // we already stored result from last frames.
            *NumOfFragments = mPixelCount;
        }		
		return true;
	}
    //------------------------------------------------------------------
    bool D3D9HardwareOcclusionQuery::isStillOutstanding(void)
    {       
        // in case you already asked for this query
        if (!mIsQueryResultStillOutstanding)
            return false;

        DWORD pixels;
        const HRESULT hr = mpQuery->GetData( (void *) &pixels, sizeof( DWORD ), D3DGETDATA_FLUSH);

        if (hr  == S_FALSE)
            return true;

        if (hr == D3DERR_DEVICELOST)
        {
            mPixelCount = 100000;
            mpDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mpQuery);
        }
        mPixelCount = pixels;
        mIsQueryResultStillOutstanding = false;
        return false;
    
    } 
}
