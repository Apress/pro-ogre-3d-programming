#ifndef _MESH_H
#define _MESH_H

#include "submesh.h"
#include "skeleton.h"
#include "mayaExportLayer.h"

namespace OgreMayaExporter
{
	/***** Class Mesh *****/
	class Mesh
	{
	public:
		//constructor
		Mesh(const MString& name = "");
		//destructor
		~Mesh();
		//clear data
		void clear();
		//get pointer to linked skeleton
		Skeleton* getSkeleton();
		//load mesh data from a maya Fn
		MStatus load(MDagPath& meshDag,ParamList &params);
		//write mesh data to maya XML
		MStatus writeXML(ParamList &params);

	protected:
		//internal members
		MString m_name;
		long m_numTriangles;
		std::vector<vertex> m_vertices;
		std::vector<uvset> m_uvsets;
		std::vector<Submesh*> m_submeshes;
		MFnSkinCluster* m_pSkinCluster;
		Skeleton* m_pSkeleton;
	};

}; // end of namespace

#endif