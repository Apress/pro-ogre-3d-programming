#include "mesh.h"

namespace OgreMayaExporter
{
	/***** Class Mesh *****/
	// constructor
	Mesh::Mesh(const MString& name)
	{
		m_name = name;
		m_numTriangles = 0;
		m_pSkinCluster = NULL;
		m_pSkeleton = NULL;
	}

	// destructor
	Mesh::~Mesh()
	{
		clear();
	}

	// clear data
	void Mesh::clear()
	{
		m_name = "";
		m_numTriangles = 0;
		for (int i=0; i<m_submeshes.size(); i++)
			delete m_submeshes[i];
		m_vertices.clear();
		m_uvsets.clear();
		m_submeshes.clear();
		if (m_pSkinCluster)
			delete m_pSkinCluster;
		m_pSkinCluster = NULL;
		if (m_pSkeleton)
			delete m_pSkeleton;
		m_pSkeleton = NULL;
	}

	// get pointer to linked skeleton
	Skeleton* Mesh::getSkeleton()
	{
		return m_pSkeleton;
	}

	/*******************************************************************************
	 *                    Load mesh data from a maya Fn                            *
	 *******************************************************************************/
	MStatus Mesh::load(MDagPath& meshDag,ParamList &params)
	{
		MStatus stat;
		// check that given DagPath corresponds to a mesh node
		if (!meshDag.hasFn(MFn::kMesh))
			return MS::kFailure;

		// get the mesh object
		MFnMesh mesh(meshDag);
		//initialise variables
		std::vector<MFloatArray> weights;
		std::vector<MIntArray> jointIds;
		unsigned int numJoints = 0;
		std::vector<vertexInfo> vertices;
		MFloatPointArray points;
		MFloatVectorArray normals;
		vertices.resize(mesh.numVertices());
		weights.resize(mesh.numVertices());
		jointIds.resize(mesh.numVertices());
		// get uv texture coordinate sets' names
		MStringArray uvsets;
		if (mesh.numUVSets() > 0)
		{
			stat = mesh.getUVSetNames(uvsets);
			if (MS::kSuccess != stat)
			{
				std::cout << "Error retrieving UV sets names\n";
				return MS::kFailure;
			}
		}
		//Save uvsets info
		for (int i=m_uvsets.size(); i<uvsets.length(); i++)
		{
			uvset uv;
			uv.size = 2;
			m_uvsets.push_back(uv);
		}
		
		// check the "opposite" attribute to see if we have to flip normals
		bool opposite = false;
		mesh.findPlug("opposite",true).getValue(opposite);

		if (params.exportVBA || params.exportSkeleton)
		{
			// get connected skin cluster (if present)
			bool foundSkinCluster = false;
			MItDependencyNodes kDepNodeIt( MFn::kSkinClusterFilter );            
			for( ;!kDepNodeIt.isDone() && !foundSkinCluster; kDepNodeIt.next()) 
			{            
				MObject kObject = kDepNodeIt.item();
				m_pSkinCluster = new MFnSkinCluster(kObject);
				unsigned int uiNumGeometries = m_pSkinCluster->numOutputConnections();
				for(unsigned int uiGeometry = 0; uiGeometry < uiNumGeometries; ++uiGeometry ) 
				{
					unsigned int uiIndex = m_pSkinCluster->indexForOutputConnection(uiGeometry);
					MObject kOutputObject = m_pSkinCluster->outputShapeAtIndex(uiIndex);
					if(kOutputObject == mesh.object()) 
					{
						std:: cout << "Found skin cluster " << m_pSkinCluster->name().asChar() << " for mesh " 
							<< mesh.name().asChar() << "\n"; 
						foundSkinCluster = true;
					}
					else
					{
						delete m_pSkinCluster;
						m_pSkinCluster = NULL;
					}
				}
			}

			// load connected skeleton (if present)
			if (m_pSkinCluster)
			{
				std::cout << "Loading skeleton data...\n";
				if (!m_pSkeleton)
					m_pSkeleton = new Skeleton();
				stat = m_pSkeleton->load(m_pSkinCluster,params);
				if (MS::kSuccess != stat)
				{
					std::cout << "Error loading skeleton data\n";
				}
				else
					std::cout << "OK\n";
			}
		}
		// get connected shaders
		MObjectArray shaders;
		MIntArray shaderPolygonMapping;
		stat = mesh.getConnectedShaders(0,shaders,shaderPolygonMapping);
		if (MS::kSuccess != stat)
		{
			std::cout << "Error getting connected shaders\n";
			return MS::kFailure;
		}
		std::cout << "Found " << shaders.length() << " connected shaders\n";
		if (shaders.length() <= 0)
		{
			std::cout << "No connected shaders, skipping mesh\n";
			return MS::kFailure;
		}

		// create a series of arrays of faces for each different submesh
		std::vector<faceArray> polygonSets;
		polygonSets.resize(shaders.length());

		// Get faces data
		// prepare vertex table
		for (int i=0; i<vertices.size(); i++)
			vertices[i].next = -2;
		//get vertex positions from mesh data
		if (params.exportWorldCoords)
			mesh.getPoints(points,MSpace::kWorld);
		else
			mesh.getPoints(points,MSpace::kTransform);
		//get list of normals from mesh data
		if (params.exportWorldCoords)
			mesh.getNormals(normals,MSpace::kWorld);
		else
			mesh.getNormals(normals,MSpace::kTransform);
		//get list of vertex weights
		if (m_pSkinCluster)
		{
			std::cout << "Get vbas\n";
			MItGeometry iterGeom(meshDag);
			for (i=0; !iterGeom.isDone(); iterGeom.next(), i++)
			{
				MObject component = iterGeom.component();
				MFloatArray vertexWeights;
				stat=m_pSkinCluster->getWeights(meshDag,component,vertexWeights,numJoints);
				weights[i]=vertexWeights;
				if (MS::kSuccess != stat)
				{
					std::cout << "Error retrieving vertex weights\n";
				}
				// get ids for the joints
				if (m_pSkeleton)
				{
					MDagPathArray influenceObjs;
					m_pSkinCluster->influenceObjects(influenceObjs,&stat);
					if (MS::kSuccess != stat)
					{
						std::cout << "Error retrieving influence objects for given skin cluster\n";
					}
					jointIds[i].setLength(weights[i].length());
					for (int j=0; j<influenceObjs.length(); j++)
					{
						bool foundJoint = false;
						for (int k=0; k<m_pSkeleton->getJoints().size() && !foundJoint; k++)
						{
							if (influenceObjs[j].partialPathName() == m_pSkeleton->getJoints()[k].name)
							{
								foundJoint=true;
								jointIds[i][j] = m_pSkeleton->getJoints()[k].id;
							}
						}
					}
				}
			}
		}
		// create an iterator to go through mesh polygons
		if (mesh.numPolygons() > 0)
		{
			std::cout << "Iterate over mesh polygons\n";
			MItMeshPolygon faceIter(mesh.object(),&stat);
			if (MS::kSuccess != stat)
			{
				std::cout << "Error accessing mesh polygons\n";
				return MS::kFailure;
			}
			std::cout << "num polygons = " << mesh.numPolygons() << "\n";
			// iterate over mesh polygons
			for (; !faceIter.isDone(); faceIter.next())
			{
				int numTris=0;
				faceIter.numTriangles(numTris);
				// for every triangle composing current polygon extract triangle info
				for (int iTris=0; iTris<numTris; iTris++)
				{
					MPointArray triPoints;
					MIntArray tempTriVertexIdx,triVertexIdx;
					int idx;
					// create a new face to store triangle info
					face newFace;
					// extract triangle vertex indices
					faceIter.getTriangle(iTris,triPoints,tempTriVertexIdx);
					// convert indices to face-relative indices
					MIntArray polyIndices;
					faceIter.getVertices(polyIndices);
					unsigned int iPoly, iObj;
					for (iObj=0; iObj < tempTriVertexIdx.length(); ++iObj)
					{
						// iPoly is face-relative vertex index
						for (iPoly=0; iPoly < polyIndices.length(); ++iPoly)
						{
							if (tempTriVertexIdx[iObj] == polyIndices[iPoly]) 
							{
								triVertexIdx.append(iPoly);
								break;
							}
						}
					}
					// iterate over triangle's vertices
					for (int i=0; i<3; i++)
					{
						bool different = true;
						int vtxIdx = faceIter.vertexIndex(triVertexIdx[i]);
						int nrmIdx = faceIter.normalIndex(triVertexIdx[i]);

						// get vertex color
						MColor color;
						if (faceIter.hasColor(triVertexIdx[i]))
						{
							stat = faceIter.getColor(color,triVertexIdx[i]);
							if (MS::kSuccess != stat)
							{
								color = MColor(1,1,1,1);
							}
							if (color.r > 1)
								color.r = 1;
							if (color.g > 1)
								color.g = 1;
							if (color.b > 1)
								color.b = 1;
							if (color.a > 1)
								color.a = 1;
						}
						else
						{
							color = MColor(1,1,1,1);
						}
						if (vertices[vtxIdx].next == -2)	// first time we encounter a vertex in this position
						{
							// save vertex position
							points[vtxIdx].cartesianize();
							vertices[vtxIdx].pointIdx = vtxIdx;
							// save vertex normal
							vertices[vtxIdx].normalIdx = nrmIdx;
							// save vertex colour
							vertices[vtxIdx].r = color.r;
							vertices[vtxIdx].g = color.g;
							vertices[vtxIdx].b = color.b;
							vertices[vtxIdx].a = color.a;
							// save vertex texture coordinates
							vertices[vtxIdx].u.resize(uvsets.length());
							vertices[vtxIdx].v.resize(uvsets.length());
							// save vbas
							vertices[vtxIdx].vba.resize(weights[vtxIdx].length());
							for (int j=0; j<weights[vtxIdx].length(); j++)
							{
								vertices[vtxIdx].vba[j] = (weights[vtxIdx])[j];
							}
							// save joint ids
							vertices[vtxIdx].jointIds.resize(jointIds[vtxIdx].length());
							for (int j=0; j<jointIds[vtxIdx].length(); j++)
							{
								vertices[vtxIdx].jointIds[j] = (jointIds[vtxIdx])[j];
							}
							// save uv sets data
							for (int j=0; j<uvsets.length(); j++)
							{
								float2 uv;
								stat = faceIter.getUV(triVertexIdx[i],uv,&uvsets[j]);
								if (MS::kSuccess != stat)
								{
									uv[0] = 0;
									uv[1] = 0;
								}
								vertices[vtxIdx].u[j] = uv[0];
								vertices[vtxIdx].v[j] = (-1)*(uv[1]-1);
							}
							// save vertex index in face info
							newFace.v[i] = m_vertices.size() + vtxIdx;
							// update value of index to next vertex info (-1 means nothing next)
							vertices[vtxIdx].next = -1;
						}
						else	// already found at least 1 vertex in this position
						{
							// check if a vertex with same attributes has been saved already
							for (int k=vtxIdx; k!=-1 && different; k=vertices[k].next)
							{
								different = false;

								if (params.exportVertNorm)
								{
									MFloatVector n1 = normals[vertices[k].normalIdx];
									MFloatVector n2 = normals[nrmIdx];
									if (n1.x!=n2.x || n1.y!=n2.y || n1.z!=n2.z)
									{
										different = true;
									}
								}

								if ((params.exportVertCol) &&
									(vertices[k].r!=color.r || vertices[k].g!=color.g || vertices[k].b!= color.b || vertices[k].a!=color.a))
								{
									different = true;
								}

								if (params.exportTexCoord)
								{
									for (int j=0; j<uvsets.length(); j++)
									{
										float2 uv;
										stat = faceIter.getUV(triVertexIdx[i],uv,&uvsets[j]);
										if (MS::kSuccess != stat)
										{
											uv[0] = 0;
											uv[1] = 0;
										}
										uv[1] = (-1)*(uv[1]-1);
										if (vertices[k].u[j]!=uv[0] || vertices[k].v[j]!=uv[1])
										{
											different = true;
										}
									}
								}
								idx = k;
							}
							// if no identical vertex has been saved, then save the vertex info
							if (different)
							{
								vertexInfo vtx;
								// save vertex position
								vtx.pointIdx = vtxIdx;
								// save vertex normal
								vtx.normalIdx = nrmIdx;
								// save vertex colour
								vtx.r = color.r;
								vtx.g = color.g;
								vtx.b = color.b;
								vtx.a = color.a;
								// save vertex vba
								vtx.vba.resize(weights[vtxIdx].length());
								for (int j=0; j<weights[vtxIdx].length(); j++)
								{
									vtx.vba[j] = (weights[vtxIdx])[j];
								}
								// save joint ids
								vtx.jointIds.resize(jointIds[vtxIdx].length());
								for (j=0; j<jointIds[vtxIdx].length(); j++)
								{
									vtx.jointIds[j] = (jointIds[vtxIdx])[j];
								}
								// save vertex texture coordinates
								vtx.u.resize(uvsets.length());
								vtx.v.resize(uvsets.length());
								for (j=0; j<uvsets.length(); j++)
								{
									float2 uv;
									stat = faceIter.getUV(triVertexIdx[i],uv,&uvsets[j]);
									if (MS::kSuccess != stat)
									{
										uv[0] = 0;
										uv[1] = 0;
									}
									vtx.u[j] = uv[0];
									vtx.v[j] = (-1)*(uv[1]-1);
								}
								vtx.next = -1;
								vertices.push_back(vtx);
								// save vertex index in face info
								newFace.v[i] = m_vertices.size() + vertices.size()-1;
								vertices[idx].next = vertices.size()-1;
							}
							else
							{
								newFace.v[i] = m_vertices.size() + idx;
							}
						}
					} // end iteration of triangle vertices
					// add face info to the array corresponding to the submesh it belongs
					// skip faces with no shaders assigned
					if (shaderPolygonMapping[faceIter.index()] >= 0)
						polygonSets[shaderPolygonMapping[faceIter.index()]].push_back(newFace);
				} // end iteration of triangles
			}
		}
		std::cout << "done reading mesh triangles\n";

		// if we are using shared geometry, then create a list of vertices for the whole mesh
		if (params.useSharedGeom)
		{
			std::cout << "Create list of shared vertices\n";
			for (i=0; i<vertices.size(); i++)
			{
				vertex v;
				vertexInfo vInfo = vertices[i];
				// save vertex coordinates
				v.x = points[vInfo.pointIdx].x;
				v.y = points[vInfo.pointIdx].y;
				v.z = points[vInfo.pointIdx].z;
				// save vertex normal
				if (opposite)
				{
					v.n.x = -normals[vInfo.normalIdx].x;
					v.n.y = -normals[vInfo.normalIdx].y;
					v.n.z = -normals[vInfo.normalIdx].z;
				}
				else
				{
					v.n.x = normals[vInfo.normalIdx].x;
					v.n.y = normals[vInfo.normalIdx].y;
					v.n.z = normals[vInfo.normalIdx].z;
				}
				v.n.normalize();
				// save vertex color
				v.r = vInfo.r;
				v.g = vInfo.g;
				v.b = vInfo.b;
				v.a = vInfo.a;
				// save vertex bone assignements
				for (int k=0; k<vInfo.vba.size(); k++)
				{
					vba newVba;
					newVba.jointIdx = vInfo.jointIds[k];
					newVba.weight = vInfo.vba[k];
					v.vbas.push_back(newVba);
				}
				// save texture coordinates
				for (k=0; k<vInfo.u.size(); k++)
				{
					texcoords newTexCoords;
					newTexCoords.u = vInfo.u[k];
					newTexCoords.v = vInfo.v[k];
					newTexCoords.w = 0;
					v.texcoords.push_back(newTexCoords);
				}
				// add newly created vertex to vertices list
				m_vertices.push_back(v);
			}
			std::cout << "done creating vertices list\n";
		}

		// create a submesh for every different shader linked to the mesh
		for (i=0; i<shaders.length(); i++)
		{
			// check if the submesh has at least 1 triangle
			if (polygonSets[i].size() > 0)
			{
				//create new submesh
				Submesh* pSubmesh = new Submesh();

				//load linked shader
				stat = pSubmesh->loadMaterial(shaders[i],uvsets,params);
				if (stat != MS::kSuccess)
				{
					MFnDependencyNode shadingGroup(shaders[i]);
					std::cout << "Error loading submesh linked to shader " << shadingGroup.name().asChar() << "\n";
					return MS::kFailure;
				}

				//load vertex and face data
				stat = pSubmesh->load(polygonSets[i],vertices,points,normals,uvsets,params,opposite);

				//add submesh to current mesh
				m_submeshes.push_back(pSubmesh);
				//update number of triangles composing the mesh
				m_numTriangles += pSubmesh->numTriangles();
			}
		}

		return MS::kSuccess;
	}


	// Write mesh data to Ogre XML
	MStatus Mesh::writeXML(ParamList &params)
	{
		MStatus stat;
		// start mesh description
		params.outMesh << "<mesh>\n";
		// write shared geometry (if used)
		if (params.useSharedGeom)
		{
			params.outMesh << "\t<sharedgeometry vertexcount=\"" << m_vertices.size() << "\">\n";
			params.outMesh << "\t\t<vertexbuffer positions=\"true\" normals=";
			if (params.exportVertNorm)
				params.outMesh << "\"true\"";
			else 
				params.outMesh << "\"false\"";
			params.outMesh << " colours_diffuse=";
			if (params.exportVertCol)
				params.outMesh << "\"true\"";
			else
				params.outMesh << "\"false\"";
			params.outMesh << " colours_specular=\"false\" texture_coords=\"";
			if (params.exportTexCoord)
				params.outMesh << m_uvsets.size() << "\">\n";
			else
				params.outMesh << 0 << "\">\n";
			// write vertex data
			for (int i=0; i < m_vertices.size(); i++)
			{
				params.outMesh << "\t\t\t<vertex>\n";
				//write vertex position
				params.outMesh << "\t\t\t\t<position x=\"" << m_vertices[i].x << "\" y=\"" 
					<< m_vertices[i].y << "\" " << "z=\"" << m_vertices[i].z << "\"/>\n";
				//write vertex normal
				if (params.exportVertNorm)
				{
					params.outMesh << "\t\t\t\t<normal x=\"" << m_vertices[i].n.x << "\" y=\"" 
						<< m_vertices[i].n.y << "\" " << "z=\"" << m_vertices[i].n.z << "\"/>\n";
				}
				//write vertex colour
				if (params.exportVertCol)
				{
					float r,g,b,a;
					if (params.exportVertColWhite)
					{
						r = g = b = a = 1.0f;
					}
					else
					{
						r = m_vertices[i].r;
						g = m_vertices[i].g;
						b = m_vertices[i].b;
						a = m_vertices[i].a;
					}

					params.outMesh << "\t\t\t\t<colour_diffuse value=\"" << r << " " << g 
						<< " " << b << " " << a << "\"/>\n";
				}//write vertex texture coordinates
				if (params.exportTexCoord)
				{
					for (int j=0; j<m_uvsets.size(); j++)
					{
						if (j < m_vertices[i].texcoords.size())
						{
							params.outMesh << "\t\t\t\t<texcoord u=\"" << m_vertices[i].texcoords[j].u << "\" v=\"" << 
								m_vertices[i].texcoords[j].v << "\"/>\n";
						}
						else
						{
							params.outMesh << "\t\t\t\t<texcoord u=\"0\" v=\"0\"/>\n";
						}
					}
				}
				params.outMesh << "\t\t\t</vertex>\n";
			}
			params.outMesh << "\t\t</vertexbuffer>\n";
			params.outMesh << "\t</sharedgeometry>\n";
		}
		// write submeshes data
		params.outMesh << "\t<submeshes>\n";
		for (int i=0; i < m_submeshes.size(); i++)
		{
			stat = m_submeshes[i]->writeXML(params);
			if (MS::kSuccess != stat)
			{
				std::cout << "Error writing submesh " << m_submeshes[i]->name().asChar() << ", aborting operation\n";
				return MS::kFailure;
			}
		}
		params.outMesh << "\t</submeshes>\n";
		// write skeleton link
		if (params.exportSkeleton && m_pSkeleton)
		{
			int ri = params.skeletonFilename.rindex('\\');
			int end = params.skeletonFilename.length() - 1;
			MString filename = params.skeletonFilename.substring(ri+1,end);
			if (filename.substring(filename.length()-4,filename.length()-1) == MString(".xml")
				&& filename.length() >= 5)
				filename = filename.substring(0,filename.length()-5);
			params.outMesh << "\t<skeletonlink name=\"" <<  filename.asChar() << "\"/>\n";
		}
		// Write shared geometry bone assignments
		if (params.useSharedGeom && params.exportVBA)
		{
			params.outMesh << "\t<boneassignments>\n";
			for (int i=0; i<m_vertices.size(); i++)
			{
				for (int j=0; j<m_vertices[i].vbas.size(); j++)
				{
					if (m_vertices[i].vbas[j].weight > 0.001)
					{
						params.outMesh << "\t\t<vertexboneassignment vertexindex=\"" << i 
							<< "\" boneindex=\"" << m_vertices[i].vbas[j].jointIdx << "\" weight=\"" 
							<< m_vertices[i].vbas[j].weight <<"\"/>\n";
					}
				}
			}
			params.outMesh << "\t</boneassignments>\n";
		}
		// write submesh names
		params.outMesh << "\t<submeshnames>\n";
		for (i=0; i<m_submeshes.size(); i++)
		{
			if (m_submeshes[i]->name() != "")
				params.outMesh << "\t\t<submeshname name=\"" << m_submeshes[i]->name().asChar() << "\" index=\"" << i << "\"/>\n";
		}
		params.outMesh << "\t</submeshnames>\n";
		// end mesh description
		params.outMesh << "</mesh>\n";

		return MS::kSuccess;
	}

}; //end of namespace
