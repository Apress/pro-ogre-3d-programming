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

#ifndef __LogManager_H__
#define __LogManager_H__

#include "OgrePrerequisites.h"

#include "OgreLog.h"
#include "OgreSingleton.h"
#include "OgreString.h"

namespace Ogre {

	/** The log listener is here to provide alternate means to write out 
	    log data. This is helpful if you want to redirect the log output
		for example to a gui window for an editing application.
	*/
	class _OgreExport LogListener
	{
	public:
		/** Called by the log system whenever a message needs to be output.
		*/
		virtual void write( const String& name,
							const String& message, 
							LogMessageLevel lml = LML_NORMAL, 
							bool maskDebug = false ) = 0;
		virtual ~LogListener();
	};

    /** The log manager handles the creation and retrieval of logs for the
        application.
        @remarks
            This class will create new log files and will retrieve instances
            of existing ones. Other classes wishing to log output can either
            create a fresh log or retrieve an existing one to output to.
            One log is the default log, and is the one written to when the
            logging methods of this class are called.
		@par
			By default, Root will instantiate a LogManager (which becomes the 
			Singleton instance) on construction, and will create a default log
			based on the Root construction parameters. If you want more control,
			for example redirecting log output right from the start or suppressing
			debug output, you need to create a LogManager yourself before creating
			a Root instance, then create a default log. Root will detect that 
			you've created one yourself and won't create one of its own, thus
			using all your logging preferences from the first instance.
    */
    class _OgreExport LogManager : public Singleton<LogManager>
    {
		friend class Log;

	protected:

		/** Internal helper method to reroute logging messages to the 
			new listener system, while maintaining full backward compatiblity.
		*/
		void _routeMessage(	const String& name,
							const String& message, 
							LogMessageLevel lml = LML_NORMAL, 
							bool maskDebug = false );

        typedef std::map<String, Log*, std::less<String> >	LogList;
		typedef std::vector<LogListener*>					LogListenerList;

        /// A list of all the logs the manager can access
        LogList mLogs;

        /// The default log to which output is done
        Log* mDefaultLog;

		/// A list of all registered external log listeners.
		LogListenerList mListeners;

    public:
        LogManager();
        ~LogManager();

		/** Adds a new listener for the logging system.
		    Ogre does not assume ownership and does not destroy the listener
			at application shutdown.
		*/
		void addListener( LogListener * listener );
		
		/** Removes a previously registered listener again, and
			returns ownership of the listener to the caller, who
			is responsible for destroying the listener again.
		*/
		void removeListener( LogListener * listener );

        /** Creates a new log with the given name.
            @param
                name The name to give the log e.g. 'Ogre.log'
            @param
                defaultLog If true, this is the default log output will be
                sent to if the generic logging methods on this class are
                used. The first log created is always the default log unless
                this parameter is set.
            @param
                debuggerOutput If true, output to this log will also be
                routed to the debugger's output window.
            @param
                suppressFileOutput If true, this is a logical rather than a physical
				log and no file output will be written. If you do this you should
				register a LogListener so log output is not lost.
        */
        Log* createLog( const String& name, bool defaultLog = false, bool debuggerOutput = true, 
			bool suppressFileOutput = false);

        /** Retrieves a log managed by this class.
        */
        Log* getLog( const String& name);

        /** Returns a pointer to the default log.
        */
        Log* getDefaultLog();

        /** Sets the passed in log as the default log.
        @returns The previous default log.
        */
        Log* setDefaultLog(Log* newLog);

        /** Log a message to the default log.
        */
        void logMessage( const String& message, LogMessageLevel lml = LML_NORMAL, 
            bool maskDebug = false);

        /** Log a message to the default log (signature for backward compatibility).
        */
        void logMessage( LogMessageLevel lml, const String& message,  
			bool maskDebug = false) { logMessage(message, lml, maskDebug); }

		/** Sets the level of detail of the default log.
        */
        void setLogDetail(LoggingLevel ll);
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static LogManager& getSingleton(void);
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static LogManager* getSingletonPtr(void);

    };

}

#endif
