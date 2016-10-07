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


#include "Ogre.h"
#include "OgreXMLMeshSerializer.h"
#include "OgreMeshSerializer.h"
#include "OgreXMLSkeletonSerializer.h"
#include "OgreSkeletonSerializer.h"
#include "OgreXMLPrerequisites.h"
#include "OgreDefaultHardwareBufferManager.h"
#include <iostream>
#include <sys/stat.h>

using namespace std;
using namespace Ogre;

struct XmlOptions
{
    String source;
    String dest;
    String sourceExt;
    String destExt;
    String logFile;
    bool interactiveMode;
    unsigned short numLods;
    Real lodDist;
    Real lodPercent;
    size_t lodFixed;
    bool usePercent;
    bool generateEdgeLists;
    bool generateTangents;
    bool reorganiseBuffers;
	bool optimiseAnimations;
	bool quietMode;
	bool d3d;
	bool gl;
	Serializer::Endian endian;
};

void help(void)
{
    // Print help message
    cout << endl << "OgreXMLConvert: Converts data between XML and OGRE binary formats." << endl;
    cout << "Provided for OGRE by Steve Streeting 2002" << endl << endl;
    cout << "Usage: OgreXMLConverter [options] sourcefile [destfile] " << endl;
	cout << endl << "Available options:" << endl;
    cout << "-i             = interactive mode - prompt for options" << endl;
    cout << "(The next 4 options are only applicable when converting XML to Mesh)" << endl;
    cout << "-l lodlevels   = number of LOD levels" << endl;
    cout << "-d loddist     = distance increment to reduce LOD" << endl;
    cout << "-p lodpercent  = Percentage triangle reduction amount per LOD" << endl;
    cout << "-f lodnumtris  = Fixed vertex reduction per LOD" << endl;
    cout << "-e             = DON'T generate edge lists (for stencil shadows)" << endl;
    cout << "-r             = DON'T reorganise vertex buffers to OGRE recommended format." << endl;
    cout << "-t             = Generate tangents (for normal mapping)" << endl;
    cout << "-o             = DON'T optimise out redundant tracks & keyframes" << endl;
	cout << "-d3d           = Prefer D3D packed colour formats (default on Windows)" << endl;
	cout << "-gl            = Prefer GL packed colour formats (default on non-Windows)" << endl;
	cout << "-E endian      = Set endian mode 'big' 'little' or 'native' (default)" << endl;
	cout << "-q             = Quiet mode, less output" << endl;
    cout << "-log filename  = name of the log file (default: 'OgreXMLConverter.log')" << endl;
    cout << "sourcefile     = name of file to convert" << endl;
    cout << "destfile       = optional name of file to write to. If you don't" << endl;
    cout << "                 specify this OGRE works it out through the extension " << endl;
    cout << "                 and the XML contents if the source is XML. For example" << endl;
    cout << "                 test.mesh becomes test.xml, test.xml becomes test.mesh " << endl;
    cout << "                 if the XML document root is <mesh> etc."  << endl;

    cout << endl;
}


XmlOptions parseArgs(int numArgs, char **args)
{
    XmlOptions opts;

    opts.interactiveMode = false;
    opts.lodDist = 500;
    opts.lodFixed = 0;
    opts.lodPercent = 20;
    opts.numLods = 0;
    opts.usePercent = true;
    opts.generateEdgeLists = true;
    opts.generateTangents = false;
    opts.reorganiseBuffers = true;
	opts.optimiseAnimations = true;
	opts.quietMode = false;
	opts.endian = Serializer::ENDIAN_NATIVE;

    // ignore program name
    char* source = 0;
    char* dest = 0;

    // Set up options
    UnaryOptionList unOpt;
    BinaryOptionList binOpt;

    unOpt["-i"] = false;
    unOpt["-e"] = false;
    unOpt["-r"] = false;
    unOpt["-t"] = false;
    unOpt["-o"] = false;
	unOpt["-q"] = false;
	unOpt["-d3d"] = false;
	unOpt["-gl"] = false;
    binOpt["-l"] = "";
    binOpt["-d"] = "";
    binOpt["-p"] = "";
    binOpt["-f"] = "";
    binOpt["-E"] = "";
    binOpt["-log"] = "OgreXMLConverter.log";

    int startIndex = findCommandLineOpts(numArgs, args, unOpt, binOpt);
    UnaryOptionList::iterator ui;
    BinaryOptionList::iterator bi;

	ui = unOpt.find("-q");
	if (ui->second)
	{
		opts.quietMode = true;
	}

	ui = unOpt.find("-i");
    if (ui->second)
    {
        opts.interactiveMode = true;
    }
    else
    {
        ui = unOpt.find("-e");
        if (ui->second)
        {
            opts.generateEdgeLists = false;
        }
    
        ui = unOpt.find("-r");
        if (ui->second)
        {
            opts.reorganiseBuffers = false;
        }

        ui = unOpt.find("-t");
        if (ui->second)
        {
            opts.generateTangents = true;
        }

        ui = unOpt.find("-o");
        if (ui->second)
        {
            opts.optimiseAnimations = false;
        }

		bi = binOpt.find("-l");
        if (!bi->second.empty())
        {
            opts.numLods = StringConverter::parseInt(bi->second);
        }

        bi = binOpt.find("-d");
        if (!bi->second.empty())
        {
            opts.lodDist = StringConverter::parseReal(bi->second);
        }

        bi = binOpt.find("-p");
        if (!bi->second.empty())
        {
            opts.lodPercent = StringConverter::parseReal(bi->second);
            opts.usePercent = true;
        }


        bi = binOpt.find("-f");
        if (!bi->second.empty())
        {
            opts.lodFixed = StringConverter::parseInt(bi->second);
            opts.usePercent = false;
        }

        bi = binOpt.find("-log");
        if (!bi->second.empty())
        {
            opts.logFile = bi->second;
        }

        bi = binOpt.find("-E");
        if (!bi->second.empty())
        {
            if (bi->second == "big")
                opts.endian = Serializer::ENDIAN_BIG;
            else if (bi->second == "little")
                opts.endian = Serializer::ENDIAN_LITTLE;
            else 
                opts.endian = Serializer::ENDIAN_NATIVE;
        }

		ui = unOpt.find("-d3d");
		if (ui->second)
		{
			opts.d3d = true;
		}

		ui = unOpt.find("-gl");
		if (ui->second)
		{
			opts.gl = true;
			opts.d3d = false;
		}

	}
    // Source / dest
    if (numArgs > startIndex)
        source = args[startIndex];
    if (numArgs > startIndex+1)
        dest = args[startIndex+1];

    if (!source)
    {
        cout << "Missing source file - abort. " << endl;
        help();
        exit(1);
    }
    // Work out what kind of conversion this is
    opts.source = source;
	std::vector<String> srcparts = StringUtil::split(opts.source, ".");
    String& ext = srcparts.back();
	StringUtil::toLowerCase(ext);
    opts.sourceExt = ext;

    if (!dest)
    {
        if (opts.sourceExt == "xml")
        {
            // dest is source minus .xml
            opts.dest = opts.source.substr(0, opts.source.size() - 4);
        }
        else
        {
            // dest is source + .xml
            opts.dest = opts.source;
            opts.dest.append(".xml");
        }

    }
    else
    {
        opts.dest = dest;
    }
	std::vector<String> dstparts = StringUtil::split(opts.dest, ".");
    ext = dstparts.back();
	StringUtil::toLowerCase(ext);
    opts.destExt = ext;

    if (!opts.quietMode) 
	{
        cout << endl;
        cout << "-- OPTIONS --" << endl;
        cout << "source file      = " << opts.source << endl;
        cout << "destination file = " << opts.dest << endl;
            cout << "log file         = " << opts.logFile << endl;
        cout << "interactive mode = " << StringConverter::toString(opts.interactiveMode) << endl;
        if (opts.numLods == 0)
        {
            cout << "lod levels       = none (or use existing)" << endl;
        }
        else
        {
            cout << "lod levels       = " << opts.numLods << endl;
            cout << "lod distance     = " << opts.lodDist << endl;
            if (opts.usePercent)
            {
                cout << "lod reduction    = " << opts.lodPercent << "%" << endl;
            }
            else
            {
                cout << "lod reduction    = " << opts.lodFixed << " verts" << endl;
            }
        }
        cout << "Generate edge lists  = " << opts.generateEdgeLists << endl;
        cout << "Generate tangents = " << opts.generateTangents << endl;
        cout << "Reorganise vertex buffers = " << opts.reorganiseBuffers << endl;
    	cout << "Optimise animations = " << opts.optimiseAnimations << endl;
    	
        cout << "-- END OPTIONS --" << endl;
        cout << endl;
    }


    return opts;
}

// Crappy globals
// NB some of these are not directly used, but are required to
//   instantiate the singletons used in the dlls
LogManager* logMgr;
Math* mth;
MaterialManager* matMgr;
SkeletonManager* skelMgr;
MeshSerializer* meshSerializer;
XMLMeshSerializer* xmlMeshSerializer;
SkeletonSerializer* skeletonSerializer;
XMLSkeletonSerializer* xmlSkeletonSerializer;
DefaultHardwareBufferManager *bufferManager;
MeshManager* meshMgr;
ResourceGroupManager* rgm;


void meshToXML(XmlOptions opts)
{
    std::ifstream ifs;
    ifs.open(opts.source.c_str(), std::ios_base::in | std::ios_base::binary);
    // pass false for freeOnClose to FileStreamDataStream since ifs is created on stack
    DataStreamPtr stream(new FileStreamDataStream(opts.source, &ifs, false));

    MeshPtr mesh = MeshManager::getSingleton().create("conversion", 
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    

    meshSerializer->importMesh(stream, mesh.getPointer());
   
    xmlMeshSerializer->exportMesh(mesh.getPointer(), opts.dest);

}

void XMLToBinary(XmlOptions opts)
{
    // Read root element and decide from there what type
    String response;
    TiXmlDocument* doc = new TiXmlDocument(opts.source);
    // Some double-parsing here but never mind
    if (!doc->LoadFile())
    {
        cout << "Unable to open file " << opts.source << " - fatal error." << endl;
        exit (1);
    }
    TiXmlElement* root = doc->RootElement();
    if (!stricmp(root->Value(), "mesh"))
    {
        delete doc;
        MeshPtr newMesh = MeshManager::getSingleton().createManual("conversion", 
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		VertexElementType colourElementType;
		if (opts.d3d)
			colourElementType = VET_COLOUR_ARGB;
		else
			colourElementType = VET_COLOUR_ABGR;

        xmlMeshSerializer->importMesh(opts.source, colourElementType, newMesh.getPointer());

        // Re-jig the buffers?
        if (opts.reorganiseBuffers)
        {
            logMgr->logMessage("Reorganising vertex buffers to automatic layout..");
            // Shared geometry
            if (newMesh->sharedVertexData)
            {
                // Automatic
                VertexDeclaration* newDcl = 
                    newMesh->sharedVertexData->vertexDeclaration->getAutoOrganisedDeclaration(
                        newMesh->hasSkeleton(), newMesh->hasVertexAnimation());
                if (*newDcl != *(newMesh->sharedVertexData->vertexDeclaration))
                {
                    // Usages don't matter here since we're onlly exporting
                    BufferUsageList bufferUsages;
                    for (size_t u = 0; u <= newDcl->getMaxSource(); ++u)
                        bufferUsages.push_back(HardwareBuffer::HBU_STATIC_WRITE_ONLY);
                    newMesh->sharedVertexData->reorganiseBuffers(newDcl, bufferUsages);
                }
            }
            // Dedicated geometry
            Mesh::SubMeshIterator smIt = newMesh->getSubMeshIterator();
            unsigned short idx = 0;
            while (smIt.hasMoreElements())
            {
                SubMesh* sm = smIt.getNext();
                if (!sm->useSharedVertices)
                {
                    // Automatic
                    VertexDeclaration* newDcl = 
                        sm->vertexData->vertexDeclaration->getAutoOrganisedDeclaration(
                            newMesh->hasSkeleton(), newMesh->hasVertexAnimation());
                    if (*newDcl != *(sm->vertexData->vertexDeclaration))
                    {
                        // Usages don't matter here since we're onlly exporting
                        BufferUsageList bufferUsages;
                        for (size_t u = 0; u <= newDcl->getMaxSource(); ++u)
                            bufferUsages.push_back(HardwareBuffer::HBU_STATIC_WRITE_ONLY);
                        sm->vertexData->reorganiseBuffers(newDcl, bufferUsages);
                    }
                }
            }

        }

        // Prompt for LOD generation?
        bool genLod = false;
        bool askLodDtls = false;
        if (!opts.interactiveMode) // derive from params if in batch mode
        {
            askLodDtls = false;
            if (opts.numLods == 0)
            {
                genLod = false;
            }
            else
            {
                genLod = true;
            }
        }
        else if(opts.numLods == 0) // otherwise only ask if not specified on command line
        {
            if (newMesh->getNumLodLevels() > 1)
            {
                std::cout << "\nXML already contains level-of detail information.\n"
                    "Do you want to: (u)se it, (r)eplace it, or (d)rop it?";
                while (response == "")
                {
                    cin >> response;
					StringUtil::toLowerCase(response);
                    if (response == "u")
                    {
                        // Do nothing
                    }
                    else if (response == "d")
                    {
                        newMesh->removeLodLevels();
                    }
                    else if (response == "r")
                    {
                        genLod = true;
                        askLodDtls = true;

                    }
                    else
                    {
                        response = "";
                    }
                }// while response == ""
            }
            else // no existing LOD
            {
                std::cout << "\nWould you like to generate LOD information? (y/n)";
                while (response == "")
                {
                    cin >> response;
					StringUtil::toLowerCase(response);
                    if (response == "n")
                    {
                        // Do nothing
                    }
                    else if (response == "y")
                    {
                        genLod = true;
                        askLodDtls = true;
                    }
                }
            }
        }

        if (genLod)
        {
            unsigned short numLod;
            ProgressiveMesh::VertexReductionQuota quota;
            Real reduction;
            Mesh::LodDistanceList distanceList;

            if (askLodDtls)
            {
                cout << "\nHow many extra LOD levels would you like to generate?";
                cin >> numLod;

                cout << "\nWhat unit of reduction would you like to use:" <<
                    "\n(f)ixed or (p)roportional?";
                cin >> response;
				StringUtil::toLowerCase(response);
                if (response == "f")
                {
                    quota = ProgressiveMesh::VRQ_CONSTANT;
                    cout << "\nHow many vertices should be removed at each LOD?";
                }
                else
                {
                    quota = ProgressiveMesh::VRQ_PROPORTIONAL;
                    cout << "\nWhat percentage of remaining vertices should be removed "
                        "\at each LOD (e.g. 50)?";
                }
                cin >> reduction;
                if (quota == ProgressiveMesh::VRQ_PROPORTIONAL)
                {
                    // Percentage -> parametric
                    reduction = reduction * 0.01f;
                }

                cout << "\nEnter the distance for each LOD to come into effect.";

                Real distance;
                for (unsigned short iLod = 0; iLod < numLod; ++iLod)
                {
                    cout << "\nLOD Level " << (iLod+1) << ":";
                    cin >> distance;
                    distanceList.push_back(distance);
                }
            }
            else
            {
                numLod = opts.numLods;
                quota = opts.usePercent? 
                    ProgressiveMesh::VRQ_PROPORTIONAL : ProgressiveMesh::VRQ_CONSTANT;
                if (opts.usePercent)
                {
                    reduction = opts.lodPercent * 0.01f;
                }
                else
                {
                    reduction = opts.lodFixed;
                }
                Real currDist = 0;
                for (unsigned short iLod = 0; iLod < numLod; ++iLod)
                {
                    currDist += opts.lodDist;
                    distanceList.push_back(currDist);
                }

            }

            newMesh->generateLodLevels(distanceList, quota, reduction);
        }

        if (opts.interactiveMode)
        {
            std::cout << "\nWould you like to include edge lists to enable stencil shadows with this mesh? (y/n)";
            while (response == "")
            {
                cin >> response;
                StringUtil::toLowerCase(response);
                if (response == "y")
                {
                    // Do nothing
                }
                else if (response == "n")
                {
                    opts.generateEdgeLists = false;
                }
                else
                {
                    response = "";
                }
            }


            std::cout << "\nWould you like to generate tangents to enable normal mapping with this mesh? (y/n)";
            while (response == "")
            {
                cin >> response;
                StringUtil::toLowerCase(response);
                if (response == "y")
                {
                    opts.generateTangents = true;
                }
                else if (response == "n")
                {
                    // Do nothing
                }
                else
                {
                    response = "";
                }
            }
        }

        if (opts.generateEdgeLists)
        {
            if (!opts.quietMode) 
			{
                std::cout << "Generating edge lists...." << std::endl;
            }
            newMesh->buildEdgeList();
        }

        if (opts.generateTangents)
        {
            unsigned short srcTex, destTex;
            bool existing = newMesh->suggestTangentVectorBuildParams(srcTex, destTex);
            if (existing)
            {
                std::cout << "\nThis mesh appears to already have a set of 3D texture coordinates, " <<
                    "which would suggest tangent vectors have already been calculated. Do you really " <<
                    "want to generate new tangent vectors (may duplicate)? (y/n)";
                while (response == "")
                {
                    cin >> response;
                    StringUtil::toLowerCase(response);
                    if (response == "y")
                    {
                        // Do nothing
                    }
                    else if (response == "n")
                    {
                        opts.generateTangents = false;
                    }
                    else
                    {
                        response = "";
                    }
                }

            }
            if (opts.generateTangents)
            {
                if (!opts.quietMode) 
				{
                    std::cout << "Generating tangent vectors...." << std::endl;
                }
                newMesh->buildTangentVectors(srcTex, destTex);
            }
        }


        meshSerializer->exportMesh(newMesh.getPointer(), opts.dest, opts.endian);
    }
    else if (!stricmp(root->Value(), "skeleton"))
    {
        delete doc;
        SkeletonPtr newSkel = SkeletonManager::getSingleton().create("conversion", 
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        xmlSkeletonSerializer->importSkeleton(opts.source, newSkel.getPointer());
		if (opts.optimiseAnimations)
		{
			newSkel->optimiseAllAnimations();
		}
        skeletonSerializer->exportSkeleton(newSkel.getPointer(), opts.dest, opts.endian);
    }


}

void skeletonToXML(XmlOptions opts)
{

    std::ifstream ifs;
    ifs.open(opts.source.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!ifs)
	{
		cout << "Unable to load file " << opts.source << endl;
		exit(1);
	}

    SkeletonPtr skel = SkeletonManager::getSingleton().create("conversion", 
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    // pass false for freeOnClose to FileStreamDataStream since ifs is created localy on stack
    DataStreamPtr stream(new FileStreamDataStream(opts.source, &ifs, false));
    skeletonSerializer->importSkeleton(stream, skel.getPointer());
   
    xmlSkeletonSerializer->exportSkeleton(skel.getPointer(), opts.dest);

}

int main(int numargs, char** args)
{
    if (numargs < 2)
    {
        help();
        return -1;
    }

    XmlOptions opts = parseArgs(numargs, args);

    logMgr = new LogManager();
	logMgr->createLog(opts.logFile, false, !opts.quietMode); 
    rgm = new ResourceGroupManager();
    mth = new Math();
    meshMgr = new MeshManager();
    matMgr = new MaterialManager();
    matMgr->initialise();
    skelMgr = new SkeletonManager();
    meshSerializer = new MeshSerializer();
    xmlMeshSerializer = new XMLMeshSerializer();
    skeletonSerializer = new SkeletonSerializer();
    xmlSkeletonSerializer = new XMLSkeletonSerializer();
    bufferManager = new DefaultHardwareBufferManager(); // needed because we don't have a rendersystem



    if (opts.sourceExt == "mesh")
    {
        meshToXML(opts);
    }
    else if (opts.sourceExt == "skeleton")
    {
        skeletonToXML(opts);
    }
    else if (opts.sourceExt == "xml")
    {
        XMLToBinary(opts);
    }
    else
    {
        cout << "Unknown input type.\n";
        exit(1); 
    }

    

    delete xmlSkeletonSerializer;
    delete skeletonSerializer;
    delete xmlMeshSerializer;
    delete meshSerializer;
    delete skelMgr;
    delete matMgr;
    delete meshMgr;
	delete bufferManager;
    delete mth;
    delete rgm;
    delete logMgr;

    return 0;

}

