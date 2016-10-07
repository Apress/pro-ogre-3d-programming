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
#include "OgreFileSystem.h"
#include "OgreLogManager.h"
#include "OgreException.h"
#include "OgreStringVector.h"
#include "OgreRoot.h"

#include <sys/types.h>
#include <sys/stat.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#   include "OgreSearchOps.h"
#   include <sys/param.h>
#   define MAX_PATH MAXPATHLEN
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#   include <windows.h>
#   include <direct.h>
#   include <io.h>
#endif

namespace Ogre {

    //-----------------------------------------------------------------------
    FileSystemArchive::FileSystemArchive(const String& name, const String& archType )
        : Archive(name, archType)
    {
    }
    //-----------------------------------------------------------------------
    bool FileSystemArchive::isCaseSensitive(void) const
    {
        #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            return false;
        #else
            return true;
        #endif

    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::findFiles(const String& pattern, bool recursive, 
        StringVector* simpleList, FileInfoList* detailList, 
        const String& currentDir)
    {
		// parsing requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

        long lHandle, res;
        struct _finddata_t tagData;

        lHandle = _findfirst(pattern.c_str(), &tagData);
        res = 0;
        while (lHandle != -1 && res != -1)
        {
            if(!(tagData.attrib & _A_SUBDIR))
            {
                if (simpleList)
                {
                    simpleList->push_back(currentDir + tagData.name);
                }
                else if (detailList)
                {
                    FileInfo fi;
					fi.archive = this;
                    fi.filename = currentDir + tagData.name;
                    fi.basename = tagData.name;
                    fi.path = currentDir;
                    fi.compressedSize = tagData.size;
                    fi.uncompressedSize = tagData.size;
                    detailList->push_back(fi);
                }
            }
            res = _findnext( lHandle, &tagData );
        }
        // Close if we found any files
        if(lHandle != -1)
        {
            _findclose(lHandle);
        }

        // Now find directories
        if (recursive)
        {

            lHandle = _findfirst("*", &tagData);
            res = 0;
            while (lHandle != -1 && res != -1)
            {
                if((tagData.attrib & _A_SUBDIR)
                    && strcmp(tagData.name, ".")
                    && strcmp(tagData.name, ".."))
                {
                    // recurse
                    String dir = currentDir + tagData.name + "/";
                    pushDirectory(tagData.name);
                    findFiles(pattern, recursive, simpleList, detailList, dir);
                    popDirectory();
                }
                res = _findnext( lHandle, &tagData );
            }
            // Close if we found any files
            if(lHandle != -1)
            {
                _findclose(lHandle);
            }

        }

    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::changeDirectory(const String& dir) const
    {
        if(chdir(dir.c_str()) == -1)
        {
            OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, 
                "Cannot open requested directory " + dir, 
                "FileSystemArchive::changeDirectory");
        }
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::pushDirectory(const String& dir) const
    {
        // get current directory and push it onto the stack
        getcwd(mTmpPath, OGRE_MAX_PATH);
        mDirectoryStack.push_back(String(mTmpPath));
        changeDirectory(dir);

    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::popDirectory(void) const
    {
        if (mDirectoryStack.empty())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                "No directories left in the stack.", 
                "FileSystemArchive::popDirectory");
        }
        changeDirectory(mDirectoryStack.back());
        mDirectoryStack.pop_back();

    }
    //-----------------------------------------------------------------------
    FileSystemArchive::~FileSystemArchive()
    {
        unload();
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::load()
    {
        mBasePath = mName;
        // Check we can change to it
        pushDirectory(mBasePath);
        // return to previous
        popDirectory();
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::unload()
    {
        // nothing to see here, move along
    }
    //-----------------------------------------------------------------------
    DataStreamPtr FileSystemArchive::open(const String& filename) const
    {
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

		pushDirectory(mBasePath);
        // Use filesystem to determine size 
        // (quicker than streaming to the end and back)
        struct stat tagStat;
        int ret = stat(filename.c_str(), &tagStat);
        assert(ret == 0 && "Problem getting file size" );


        // Always open in binary mode
        std::ifstream *origStream = new std::ifstream();
        origStream->open(filename.c_str(), std::ios::in | std::ios::binary);

        popDirectory();

        // Should check ensure open succeeded, in case fail for some reason.
        if (origStream->fail())
        {
            delete origStream;
            OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
                "Cannot open file: " + filename,
                "FileSystemArchive::open");
        }

        /// Construct return stream, tell it to delete on destroy
        FileStreamDataStream* stream = new FileStreamDataStream(filename,
            origStream, tagStat.st_size, true);
        return DataStreamPtr(stream);
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::list(bool recursive)
    {
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

		pushDirectory(mBasePath);
        StringVectorPtr ret(new StringVector());

        findFiles("*", recursive, ret.getPointer(), 0);

        popDirectory();

        return ret;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::listFileInfo(bool recursive)
    {
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

        pushDirectory(mBasePath);
        FileInfoListPtr ret(new FileInfoList());

        findFiles("*", recursive, 0, ret.getPointer());

        popDirectory();

        return ret;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::find(const String& pattern, bool recursive)
    {
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

        pushDirectory(mBasePath);
        StringVectorPtr ret(new StringVector());

        findFiles(pattern, recursive, ret.getPointer(), 0);

        popDirectory();

        return ret;

    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::findFileInfo(const String& pattern, 
        bool recursive)
    {
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

        pushDirectory(mBasePath);
        FileInfoListPtr ret(new FileInfoList());

        findFiles(pattern, recursive, 0, ret.getPointer());

        popDirectory();

        return ret;
    }
    //-----------------------------------------------------------------------
	bool FileSystemArchive::exists(const String& filename)
	{
		// directory change requires locking due to saved returns
		OGRE_LOCK_AUTO_MUTEX

		bool ret;
        pushDirectory(mBasePath);

        struct stat tagStat;
        ret = (stat(filename.c_str(), &tagStat) == 0);

		popDirectory();

		return ret;
		
	}
    //-----------------------------------------------------------------------
    const String& FileSystemArchiveFactory::getType(void) const
    {
        static String name = "FileSystem";
        return name;
    }

}
