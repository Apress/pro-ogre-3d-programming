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
#include "OgreString.h"

#include "OgreStringVector.h"

namespace Ogre {

	//-----------------------------------------------------------------------
	const String StringUtil::BLANK = String("");
	//-----------------------------------------------------------------------
    void StringUtil::trim(String& str, bool left, bool right)
    {
        /*
        size_t lspaces, rspaces, len = length(), i;

        lspaces = rspaces = 0;

        if( left )
        {
            // Find spaces / tabs on the left
            for( i = 0;
                i < len && ( at(i) == ' ' || at(i) == '\t' || at(i) == '\r');
                ++lspaces, ++i );
        }
        
        if( right && lspaces < len )
        {
            // Find spaces / tabs on the right
            for( i = len - 1;
                i >= 0 && ( at(i) == ' ' || at(i) == '\t' || at(i) == '\r');
                rspaces++, i-- );
        }

        *this = substr(lspaces, len-lspaces-rspaces);
        */
        static const String delims = " \t\r";
        if(right)
            str.erase(str.find_last_not_of(delims)+1); // trim right
        if(left)
            str.erase(0, str.find_first_not_of(delims)); // trim left
    }

    //-----------------------------------------------------------------------
    std::vector<String> StringUtil::split( const String& str, const String& delims, unsigned int maxSplits)
    {
        // static unsigned dl;
        std::vector<String> ret;
        unsigned int numSplits = 0;

        // Use STL methods 
        size_t start, pos;
        start = 0;
        do 
        {
            pos = str.find_first_of(delims, start);
            if (pos == start)
            {
                // Do nothing
                start = pos + 1;
            }
            else if (pos == String::npos || (maxSplits && numSplits == maxSplits))
            {
                // Copy the rest of the string
                ret.push_back( str.substr(start) );
                break;
            }
            else
            {
                // Copy up to delimiter
                ret.push_back( str.substr(start, pos - start) );
                start = pos + 1;
            }
            // parse up to next real data
            start = str.find_first_not_of(delims, start);
            ++numSplits;

        } while (pos != String::npos);



        return ret;
    }

    //-----------------------------------------------------------------------
    void StringUtil::toLowerCase(String& str)
    {
        std::transform(
            str.begin(),
            str.end(),
            str.begin(),
			tolower);
    }

    //-----------------------------------------------------------------------
    void StringUtil::toUpperCase(String& str) 
    {
        std::transform(
            str.begin(),
            str.end(),
            str.begin(),
			toupper);
    }
    //-----------------------------------------------------------------------
    bool StringUtil::startsWith(const String& str, const String& pattern, bool lowerCase)
    {
        size_t thisLen = str.length();
        size_t patternLen = pattern.length();
        if (thisLen < patternLen || patternLen == 0)
            return false;

        String startOfThis = str.substr(0, patternLen);
        if (lowerCase)
            StringUtil::toLowerCase(startOfThis);

        return (startOfThis == pattern);
    }
    //-----------------------------------------------------------------------
    bool StringUtil::endsWith(const String& str, const String& pattern, bool lowerCase)
    {
        size_t thisLen = str.length();
        size_t patternLen = pattern.length();
        if (thisLen < patternLen || patternLen == 0)
            return false;

        String endOfThis = str.substr(thisLen - patternLen, patternLen);
        if (lowerCase)
            StringUtil::toLowerCase(endOfThis);

        return (endOfThis == pattern);
    }
    //-----------------------------------------------------------------------
    String StringUtil::standardisePath(const String& init)
    {
        String path = init;

        std::replace( path.begin(), path.end(), '\\', '/' );
        if( path[path.length() - 1] != '/' )
            path += '/';

        return path;
    }
    //-----------------------------------------------------------------------
    void StringUtil::splitFilename(const String& qualifiedName, 
        String& outBasename, String& outPath)
    {
        String path = qualifiedName;
        // Replace \ with / first
        std::replace( path.begin(), path.end(), '\\', '/' );
        // split based on final /
        size_t i = path.find_last_of('/');

        if (i == String::npos)
        {
            outPath = "";
			outBasename = qualifiedName;
        }
        else
        {
            outBasename = path.substr(i+1, path.size() - i - 1);
            outPath = path.substr(0, i+1);
        }

    }
    //-----------------------------------------------------------------------
    bool StringUtil::match(const String& str, const String& pattern, bool caseSensitive)
    {
        String tmpStr = str;
		String tmpPattern = pattern;
        if (!caseSensitive)
        {
            StringUtil::toLowerCase(tmpStr);
            StringUtil::toLowerCase(tmpPattern);
        }

        String::const_iterator strIt = tmpStr.begin();
        String::const_iterator patIt = tmpPattern.begin();
		String::const_iterator lastWildCardIt = tmpPattern.end();
        while (strIt != tmpStr.end() && patIt != tmpPattern.end())
        {
            if (*patIt == '*')
            {
				lastWildCardIt = patIt;
                // Skip over looking for next character
                ++patIt;
                if (patIt == tmpPattern.end())
				{
					// Skip right to the end since * matches the entire rest of the string
					strIt = tmpStr.end();
				}
				else
                {
					// scan until we find next pattern character
                    while(strIt != tmpStr.end() && *strIt != *patIt)
                        ++strIt;
                }
            }
            else
            {
                if (*patIt != *strIt)
                {
					if (lastWildCardIt != tmpPattern.end())
					{
						// The last wildcard can match this incorrect sequence
						// rewind pattern to wildcard and keep searching
						patIt = lastWildCardIt;
						lastWildCardIt = tmpPattern.end();
					}
					else
					{
						// no wildwards left
						return false;
					}
                }
                else
                {
                    ++patIt;
                    ++strIt;
                }
            }

        }
		// If we reached the end of both the pattern and the string, we succeeded
		if (patIt == tmpPattern.end() && strIt == tmpStr.end())
		{
        	return true;
		}
		else
		{
			return false;
		}

    }

}
