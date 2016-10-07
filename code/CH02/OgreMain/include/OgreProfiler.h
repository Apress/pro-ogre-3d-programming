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
/*

    Although the code is original, many of the ideas for the profiler were borrowed from 
"Real-Time In-Game Profiling" by Steve Rabin which can be found in Game Programming
Gems 1.

    This code can easily be adapted to your own non-Ogre project. The only code that is 
Ogre-dependent is in the visualization/logging routines and the use of the Timer class.

    Enjoy!

*/

#ifndef __Profiler_H__
#define __Profiler_H__

#include "OgrePrerequisites.h"
#include "OgreSingleton.h"
#include "OgreString.h"
#include "OgreOverlay.h"

#if OGRE_PROFILING == 1
#   if OGRE_COMPILER != OGRE_COMPILER_BORL
#       define OgreProfile( a ) Ogre::Profile _OgreProfileInstance( (a) )
#       define OgreProfileBegin( a ) Ogre::Profiler::getSingleton().beginProfile( (a) )
#       define OgreProfileEnd( a ) Ogre::Profiler::getSingleton().endProfile( (a) )
#   else
#	    define OgreProfile( a ) Ogre::Profile _OgreProfileInstance( __FUNC__ )
#	    define OgreProfileBegin( a ) Ogre::Profiler::getSingleton().beginProfile( __FUNC__ )
#	    define OgreProfileEnd( a ) Ogre::Profiler::getSingleton().endProfile( __FUNC__ )
#   endif
#else
#   define OgreProfile( a )
#   define OgreProfileBegin( a )
#   define OgreProfileEnd( a )
#endif

namespace Ogre {

    /** An individual profile that will be processed by the Profiler
        @remarks
            Use the macro OgreProfile(name) instead of instantiating this profile directly
        @remarks
            We use this Profile to allow scoping rules to signify the beginning and end of
            the profile. Use the Profiler singleton (through the macro OgreProfileBegin(name)
            and OgreProfileEnd(name)) directly if you want a profile to last
            outside of a scope (ie the main game loop).
        @author Amit Mathew (amitmathew (at) yahoo (dot) com)
    */
    class _OgreExport Profile {

        public:
            Profile(const String& profileName);
            ~Profile();

        protected:

            /// The name of this profile
            String mName;
			

    };

    /** The profiler allows you to measure the performance of your code
        @remarks
            Do not create profiles directly from this unless you want a profile to last
            outside of its scope (ie the main game loop). For most cases, use the macro
            OgreProfile(name) and braces to limit the scope. You must enable the Profile
            before you can used it with setEnabled(true). If you want to disable profiling
            in Ogre, simply set the macro OGRE_PROFILING to 0.
        @author Amit Mathew (amitmathew (at) yahoo (dot) com)
        @todo resolve artificial cap on number of profiles displayed
        @todo fix display ordering of profiles not called every frame
    */
    class _OgreExport Profiler : public Singleton<Profiler> {

        public:
            Profiler();
            ~Profiler();

            /** Sets the timer for the profiler */
            void setTimer(Timer* t);

            /** Retrieves the timer for the profiler */
            Timer* getTimer();

            /** Begins a profile
            @remarks 
                Use the macro OgreProfileBegin(name) instead of calling this directly 
                so that profiling can be ignored in the release version of your app. 
            @remarks 
                You only use the macro (or this) if you want a profile to last outside
                of its scope (ie the main game loop). If you use this function, make sure you 
                use a corresponding OgreProfileEnd(name). Usually you would use the macro 
                OgreProfile(name). This function will be ignored for a profile that has been 
                disabled or if the profiler is disabled.
            @param profileName Must be unique and must not be an empty string
            */
            void beginProfile(const String& profileName);

            /** Ends a profile
            @remarks 
                Use the macro OgreProfileEnd(name) instead of calling this directly so that
                profiling can be ignored in the release version of your app.
            @remarks
                This function is usually not called directly unless you want a profile to
                last outside of its scope. In most cases, using the macro OgreProfile(name) 
                which will call this function automatically when it goes out of scope. Make 
                sure the name of this profile matches its corresponding beginProfile name. 
                This function will be ignored for a profile that has been disabled or if the
                profiler is disabled.
            */
            void endProfile(const String& profileName);

            /** Sets whether this profiler is enabled. Only takes effect after the
                the frame has ended.
                @remarks When this is called the first time with the parameter true,
                it initializes the GUI for the Profiler
            */
            void setEnabled(bool enabled);

            /** Gets whether this profiler is enabled */
            bool getEnabled() const;

            /** Enables a previously disabled profile 
            @remarks Only enables the profile if this function is not 
            called during the profile it is trying to enable.
            */
            void enableProfile(const String& profileName);

            /** Disables a profile
            @remarks Only disables the profile if this function is not called during
            the profile it is trying to disable.
            */
            void disableProfile(const String& profileName);

            /** Returns true if the specified profile reaches a new frame time maximum
            @remarks If this is called during a frame, it will be reading the results
            from the previous frame. Therefore, it is best to use this after the frame
            has ended.
            */
            bool watchForMax(const String& profileName);

            /** Returns true if the specified profile reaches a new frame time minimum
            @remarks If this is called during a frame, it will be reading the results
            from the previous frame. Therefore, it is best to use this after the frame
            has ended.
            */
            bool watchForMin(const String& profileName);

            /** Returns true if the specified profile goes over or under the given limit
                frame time
            @remarks If this is called during a frame, it will be reading the results
            from the previous frame. Therefore, it is best to use this after the frame
            has ended.
            @param limit A number between 0 and 1 representing the percentage of frame time
            @param greaterThan If true, this will return whether the limit is exceeded. Otherwise,
            it will return if the frame time has gone under this limit.
            */
            bool watchForLimit(const String& profileName, Real limit, bool greaterThan = true);

            /** Outputs current profile statistics to the log */
            void logResults();

            /** Clears the profiler statistics */
            void reset();

            /** Sets the Profiler so the display of results are updated ever n frames*/
            void setUpdateDisplayFrequency(uint freq);

            /** Gets the frequency that the Profiler display is updated */
            uint getUpdateDisplayFrequency() const;

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
            static Profiler& getSingleton(void);
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
            static Profiler* getSingletonPtr(void);

        protected:

            /** Initializes the profiler's gui elements */
            void initialize();

            /** Prints the profiling results of each frame */
            void displayResults();

            /** Processes the profiler data after each frame */
            void processFrameStats();

            /** Handles a change of the profiler's enabled state*/
            void changeEnableState();

            /** An internal function to create the container which will hold our display elements*/
            OverlayContainer* createContainer();

            /** An internal function to create a text area */
            OverlayElement* createTextArea(const String& name, Real width, Real height, Real top, Real left, 
                                       uint fontSize, const String& caption, bool show = true);

            /** An internal function to create a panel */
            OverlayElement* createPanel(const String& name, Real width, Real height, Real top, Real left, 
                                    const String& materialName, bool show = true);

            /// Represents an individual profile call
            struct ProfileInstance {

                /// The name of the profile
                String		name;

                /// The name of the parent, empty string if root
                String		parent;

                /// The time this profile was started
                ulong		currTime;

                /// Represents the total time of all child profiles to subtract
                /// from this profile
                ulong		accum;

                /// The hierarchical level of this profile, 0 being the root profile
                uint		hierarchicalLvl;
            };

            /// Represents the total timing information of a profile
            /// since profiles can be called more than once each frame
            struct ProfileFrame {
				
                /// The name of the profile
                String	name;

                /// The total time this profile has taken this frame
                ulong	frameTime;

                /// The number of times this profile was called this frame
                uint	calls;

                /// The hierarchical level of this profile, 0 being the main loop
                uint	hierarchicalLvl;

            };
			
            /// Represents a history of each profile during the duration of the app
            struct ProfileHistory {

                /// The name of the profile
                String	name;

                /// The current percentage of frame time this profile has taken
                Real	currentTime; // %

                /// The maximum percentage of frame time this profile has taken
                Real	maxTime; // %

                /// The minimum percentage of frame time this profile has taken
                Real	minTime; // %

                /// The number of times this profile has been called each frame
                uint	numCallsThisFrame;

                /// The total percentage of frame time this profile has taken
                /// (used to calculate average)
                Real	totalTime; // %

                /// The total number of times this profile was called
                /// (used to calculate average)
                ulong	totalCalls; // %

                /// The hierarchical level of this profile, 0 being the root profile
                uint	hierarchicalLvl;

			};

			
            typedef std::list<ProfileInstance> ProfileStack;
            typedef std::list<ProfileFrame> ProfileFrameList;
            typedef std::list<ProfileHistory> ProfileHistoryList;
            typedef std::map<String, ProfileHistoryList::iterator> ProfileHistoryMap;
            typedef std::map<String, bool> DisabledProfileMap;

            typedef std::list<OverlayElement*> ProfileBarList;

            /// A stack for each individual profile per frame
            ProfileStack mProfiles;

            /// Accumulates the results of each profile per frame (since a profile can be called
            /// more than once a frame)
            ProfileFrameList mProfileFrame;

            /// Keeps track of the statistics of each profile
            ProfileHistoryList mProfileHistory;

            /// We use this for quick look-ups of profiles in the history list
            ProfileHistoryMap mProfileHistoryMap;

            /// Holds the names of disabled profiles
            DisabledProfileMap mDisabledProfiles;

            /// Holds the display bars for each profile results
            ProfileBarList mProfileBars;

            /// Whether the GUI elements have been initialized
            bool mInitialized;

            /// The max number of profiles we can display
            uint maxProfiles;

            /// The overlay which contains our profiler results display
            Overlay* mOverlay;

            /// The window that displays the profiler results
            OverlayContainer* mProfileGui;

            /// The height of each bar
            Real mBarHeight;

            /// The height of the stats window
            Real mGuiHeight;

            /// The width of the stats window
            Real mGuiWidth;

            /// The size of the indent for each profile display bar
            Real mBarIndent;

            /// The width of the border between the profile window and each bar
            Real mGuiBorderWidth;

            /// The width of the min, avg, and max lines in a profile display
            Real mBarLineWidth;

            /// The number of frames that must elapse before the current
            /// frame display is updated
            uint mUpdateDisplayFrequency;

            /// The number of elasped frame, used with mUpdateDisplayFrequency
            uint mCurrentFrame;

            /// The timer used for profiling
            Timer* mTimer;

            /// The total time each frame takes
            ulong mTotalFrameTime;

            /// Whether this profiler is enabled
            bool mEnabled;

            /// Keeps track of whether this profiler has
            /// received a request to be enabled/disabled
            bool mEnableStateChangePending;

            /// Keeps track of the new enabled/disabled state that the user has requested
            /// which will be applied after the frame ends
            bool mNewEnableState;

    }; // end class

} // end namespace

#endif
