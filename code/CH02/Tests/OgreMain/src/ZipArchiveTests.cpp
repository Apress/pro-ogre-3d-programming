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
#include "ZipArchiveTests.h"
#include "OgreZip.h"

using namespace Ogre;

// Regsiter the suite
CPPUNIT_TEST_SUITE_REGISTRATION( ZipArchiveTests );

void ZipArchiveTests::setUp()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    testPath = "../../../../Tests/OgreMain/misc/ArchiveTest.zip";
#else
    testPath = "../../../Tests/OgreMain/misc/ArchiveTest.zip";
#endif
}
void ZipArchiveTests::tearDown()
{
}

void ZipArchiveTests::testListNonRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    StringVectorPtr vec = arch.list(false);

    CPPUNIT_ASSERT_EQUAL((size_t)2, vec->size());
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), vec->at(0));
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), vec->at(1));

}
void ZipArchiveTests::testListRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    StringVectorPtr vec = arch.list(true);

    CPPUNIT_ASSERT_EQUAL((size_t)6, vec->size());
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file.material"), vec->at(0));
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file2.material"), vec->at(1));
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file3.material"), vec->at(2));
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file4.material"), vec->at(3));
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), vec->at(4));
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), vec->at(5));
}
void ZipArchiveTests::testListFileInfoNonRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    FileInfoListPtr vec = arch.listFileInfo(false);

    CPPUNIT_ASSERT_EQUAL((size_t)2, vec->size());
    FileInfo& fi1 = vec->at(0);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi1.path);
    CPPUNIT_ASSERT_EQUAL((size_t)40, fi1.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)130, fi1.uncompressedSize);

    FileInfo& fi2 = vec->at(1);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi2.path);
    CPPUNIT_ASSERT_EQUAL((size_t)45, fi2.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)156, fi2.uncompressedSize);
}
void ZipArchiveTests::testListFileInfoRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    FileInfoListPtr vec = arch.listFileInfo(true);

    CPPUNIT_ASSERT_EQUAL((size_t)6, vec->size());
    FileInfo& fi3 = vec->at(0);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file.material"), fi3.filename);
    CPPUNIT_ASSERT_EQUAL(String("file.material"), fi3.basename);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/"), fi3.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi3.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi3.uncompressedSize);

    FileInfo& fi4 = vec->at(1);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file2.material"), fi4.filename);
    CPPUNIT_ASSERT_EQUAL(String("file2.material"), fi4.basename);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/"), fi4.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi4.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi4.uncompressedSize);


    FileInfo& fi5 = vec->at(2);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file3.material"), fi5.filename);
    CPPUNIT_ASSERT_EQUAL(String("file3.material"), fi5.basename);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/"), fi5.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi5.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi5.uncompressedSize);

    FileInfo& fi6 = vec->at(3);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file4.material"), fi6.filename);
    CPPUNIT_ASSERT_EQUAL(String("file4.material"), fi6.basename);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/"), fi6.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi6.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi6.uncompressedSize);

    FileInfo& fi1 = vec->at(4);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi1.path);
    CPPUNIT_ASSERT_EQUAL((size_t)40, fi1.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)130, fi1.uncompressedSize);

    FileInfo& fi2 = vec->at(5);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi2.path);
    CPPUNIT_ASSERT_EQUAL((size_t)45, fi2.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)156, fi2.uncompressedSize);

}
void ZipArchiveTests::testFindNonRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    StringVectorPtr vec = arch.find("*.txt", false);

    CPPUNIT_ASSERT_EQUAL((size_t)2, vec->size());
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), vec->at(0));
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), vec->at(1));
}
void ZipArchiveTests::testFindRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    StringVectorPtr vec = arch.find("*.material", true);

    CPPUNIT_ASSERT_EQUAL((size_t)4, vec->size());
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file.material"), vec->at(0));
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file2.material"), vec->at(1));
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file3.material"), vec->at(2));
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file4.material"), vec->at(3));
}
void ZipArchiveTests::testFindFileInfoNonRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    FileInfoListPtr vec = arch.findFileInfo("*.txt", false);

    CPPUNIT_ASSERT_EQUAL((size_t)2, vec->size());
    FileInfo& fi1 = vec->at(0);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile.txt"), fi1.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi1.path);
    CPPUNIT_ASSERT_EQUAL((size_t)40, fi1.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)130, fi1.uncompressedSize);

    FileInfo& fi2 = vec->at(1);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.filename);
    CPPUNIT_ASSERT_EQUAL(String("rootfile2.txt"), fi2.basename);
    CPPUNIT_ASSERT_EQUAL(StringUtil::BLANK, fi2.path);
    CPPUNIT_ASSERT_EQUAL((size_t)45, fi2.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)156, fi2.uncompressedSize);
}
void ZipArchiveTests::testFindFileInfoRecursive()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();
    FileInfoListPtr vec = arch.findFileInfo("*.material", true);

    CPPUNIT_ASSERT_EQUAL((size_t)4, vec->size());

    FileInfo& fi3 = vec->at(0);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file.material"), fi3.filename);
    CPPUNIT_ASSERT_EQUAL(String("file.material"), fi3.basename);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/"), fi3.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi3.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi3.uncompressedSize);

    FileInfo& fi4 = vec->at(1);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/file2.material"), fi4.filename);
    CPPUNIT_ASSERT_EQUAL(String("file2.material"), fi4.basename);
    CPPUNIT_ASSERT_EQUAL(String("level1/materials/scripts/"), fi4.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi4.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi4.uncompressedSize);


    FileInfo& fi5 = vec->at(2);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file3.material"), fi5.filename);
    CPPUNIT_ASSERT_EQUAL(String("file3.material"), fi5.basename);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/"), fi5.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi5.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi5.uncompressedSize);

    FileInfo& fi6 = vec->at(3);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/file4.material"), fi6.filename);
    CPPUNIT_ASSERT_EQUAL(String("file4.material"), fi6.basename);
    CPPUNIT_ASSERT_EQUAL(String("level2/materials/scripts/"), fi6.path);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi6.compressedSize);
    CPPUNIT_ASSERT_EQUAL((size_t)0, fi6.uncompressedSize);
}
void ZipArchiveTests::testFileRead()
{
    ZipArchive arch(testPath, "Zip");
    arch.load();

    DataStreamPtr stream = arch.open("rootfile.txt");
    CPPUNIT_ASSERT_EQUAL(String("this is line 1 in file 1"), stream->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 2 in file 1"), stream->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 3 in file 1"), stream->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 4 in file 1"), stream->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 5 in file 1"), stream->getLine());
    CPPUNIT_ASSERT(stream->eof());

}
void ZipArchiveTests::testReadInterleave()
{
    // Test overlapping reads from same archive

    ZipArchive arch(testPath, "Zip");
    arch.load();

    // File 1
    DataStreamPtr stream1 = arch.open("rootfile.txt");
    CPPUNIT_ASSERT_EQUAL(String("this is line 1 in file 1"), stream1->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 2 in file 1"), stream1->getLine());

    // File 2
    DataStreamPtr stream2 = arch.open("rootfile2.txt");
    CPPUNIT_ASSERT_EQUAL(String("this is line 1 in file 2"), stream2->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 2 in file 2"), stream2->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 3 in file 2"), stream2->getLine());


    // File 1
    CPPUNIT_ASSERT_EQUAL(String("this is line 3 in file 1"), stream1->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 4 in file 1"), stream1->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 5 in file 1"), stream1->getLine());
    CPPUNIT_ASSERT(stream1->eof());


    // File 2
    CPPUNIT_ASSERT_EQUAL(String("this is line 4 in file 2"), stream2->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 5 in file 2"), stream2->getLine());
    CPPUNIT_ASSERT_EQUAL(String("this is line 6 in file 2"), stream2->getLine());
    CPPUNIT_ASSERT(stream2->eof());

}
