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
#include "OgreStringInterface.h"

namespace Ogre {

    ParamDictionaryMap StringInterface::msDictionary;


    const ParameterList& StringInterface::getParameters(void) const
    {
        static ParameterList emptyList;

        const ParamDictionary* dict = getParamDictionary();
        if (dict)
            return dict->getParameters();
        else
            return emptyList;

    };

    bool StringInterface::setParameter(const String& name, const String& value)
    {
        // Get dictionary
        ParamDictionary* dict = getParamDictionary();

        if (dict)
        {
            // Look up command object
            ParamCommand* cmd = dict->getParamCommand(name);
            if (cmd)
            {
                cmd->doSet(this, value);
                return true;
            }
        }
        // Fallback
        return false;
    }
	//-----------------------------------------------------------------------
	void StringInterface::setParameterList(const NameValuePairList& paramList)
	{
		NameValuePairList::const_iterator i, iend;
		iend = paramList.end();
		for (i = paramList.begin(); i != iend; ++i)
		{
			setParameter(i->first, i->second);
		}
	}
    //-----------------------------------------------------------------------
    void StringInterface::cleanupDictionary () {
      msDictionary.clear();
    }
}
