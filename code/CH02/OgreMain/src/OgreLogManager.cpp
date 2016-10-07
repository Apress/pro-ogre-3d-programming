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

#include "OgreLogManager.h"
#include "OgreException.h"
#include <algorithm>
namespace Ogre {

    //-----------------------------------------------------------------------
    template<> LogManager* Singleton<LogManager>::ms_Singleton = 0;
    LogManager* LogManager::getSingletonPtr(void)
    {
        return ms_Singleton;
    }
    LogManager& LogManager::getSingleton(void)
    {  
        assert( ms_Singleton );  return ( *ms_Singleton );  
    }
    //-----------------------------------------------------------------------
    LogManager::LogManager()
    {
        mDefaultLog = NULL;
    }
    //-----------------------------------------------------------------------
    LogManager::~LogManager()
    {
        // Destroy all logs
        LogList::iterator i;
        for (i = mLogs.begin(); i != mLogs.end(); ++i)
        {
            delete i->second;
        }
    }
    //-----------------------------------------------------------------------
    Log* LogManager::createLog( const String& name, bool defaultLog, bool debuggerOutput, 
		bool suppressFileOutput)
    {
        Log* newLog = new Log(name, debuggerOutput, suppressFileOutput);

        if( !mDefaultLog || defaultLog )
        {
            mDefaultLog = newLog;
        }

        mLogs.insert( LogList::value_type( name, newLog ) );

        return newLog;
    }
    //-----------------------------------------------------------------------
    Log* LogManager::getDefaultLog()
    {
        return mDefaultLog;
    }
    //-----------------------------------------------------------------------
    Log* LogManager::setDefaultLog(Log* newLog)
    {
        Log* oldLog = mDefaultLog;
        mDefaultLog = newLog;
        return oldLog;
    }
    //-----------------------------------------------------------------------
    Log* LogManager::getLog( const String& name)
    {
        LogList::iterator i = mLogs.find(name);
        if (i != mLogs.end())
            return i->second;
        else
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Log not found. ", "LogManager::getLog");


    }
    //-----------------------------------------------------------------------
    void LogManager::logMessage( const String& message, LogMessageLevel lml, bool maskDebug)
    {
		if (mDefaultLog)
		{
			mDefaultLog->logMessage(message, lml, maskDebug);
		}
    }
    //-----------------------------------------------------------------------
    void LogManager::setLogDetail(LoggingLevel ll)
    {
		if (mDefaultLog)
		{
	        mDefaultLog->setLogDetail(ll);
		}
    }
	//-----------------------------------------------------------------------
	void LogManager::_routeMessage(	const String& name,
									const String& message, 
									LogMessageLevel lml, 
									bool maskDebug )
	{
		// Reroute the messages.
		for( size_t i = 0; i < mListeners.size(); i++ )
		{
			mListeners[i]->write( name,message,lml,maskDebug );
		}
	}
	//-----------------------------------------------------------------------
	void LogManager::addListener( LogListener * listener )
	{
		mListeners.push_back( listener );
	}

	//-----------------------------------------------------------------------
	void LogManager::removeListener( LogListener * listener )
	{
		mListeners.erase(std::find(mListeners.begin(), 
			mListeners.end(), 
			listener));
	}
	//-----------------------------------------------------------------------
	LogListener::~LogListener()
	{
	}
}
