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
#ifndef __FileSystem_H__
#define __FileSystem_H__

#include "OgrePrerequisites.h"

#include "OgreArchive.h"
#include "OgreArchiveFactory.h"

namespace Ogre {

    /** Specialisation of the Archive class to allow reading of files from 
        filesystem folders / directories.
    */
    class _OgreExport FileSystemArchive : public Archive 
    {
	public:
		OGRE_AUTO_MUTEX
    protected:
        /// Base path; actually the same as mName, but for clarity
        String mBasePath;

        /// Directory stack of previous directories
        typedef std::deque<String> DirectoryStack;
        mutable DirectoryStack mDirectoryStack;

        #define OGRE_MAX_PATH 256
        /// temporary C-string to retrieve paths
        mutable char mTmpPath[OGRE_MAX_PATH];

        /** Utility method to retrieve all files in a directory matching pattern.
        @param pattern File pattern
        @param recursive Whether to cascade down directories
        @param simpleList Populated if retrieving a simple list
        @param detailList Populated if retrieving a detailed list
        @param currentDir The current directory relative to the base of the 
            archive, for file naming
        */
        void findFiles(const String& pattern, bool recursive, StringVector* simpleList,
            FileInfoList* detailList, const String& currentDir = "");

        /// Utility method to change the current directory
        void changeDirectory(const String& dir) const;
        /// Utility method to change directory and push the current directory onto a stack
        void pushDirectory(const String& dir) const;
        /// Utility method to pop a previous directory off the stack and change to it
        void popDirectory(void) const;

    public:
        FileSystemArchive(const String& name, const String& archType );
        ~FileSystemArchive();

        /// @copydoc Archive::isCaseSensitive
        bool isCaseSensitive(void) const;

        /// @copydoc Archive::load
        void load();
        /// @copydoc Archive::unload
        void unload();

        /// @copydoc Archive::open
        DataStreamPtr open(const String& filename) const;

        /// @copydoc Archive::list
        StringVectorPtr list(bool recursive = true );

        /// @copydoc Archive::listFileInfo
        FileInfoListPtr listFileInfo(bool recursive = true );

        /// @copydoc Archive::find
        StringVectorPtr find(const String& pattern, bool recursive = true);

        /// @copydoc Archive::findFileInfo
        FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true);

        /// @copydoc Archive::exists
		bool exists(const String& filename);

    };

    /** Specialisation of ArchiveFactory for FileSystem files. */
    class _OgrePrivate FileSystemArchiveFactory : public ArchiveFactory
    {
    public:
        virtual ~FileSystemArchiveFactory() {}
        /// @copydoc FactoryObj::getType
        const String& getType(void) const;
        /// @copydoc FactoryObj::createInstance
        Archive *createInstance( const String& name ) 
        {
            return new FileSystemArchive(name, "FileSystem");
        }
        /// @copydoc FactoryObj::destroyInstance
        void destroyInstance( Archive* arch) { delete arch; }
    };


}

#endif
