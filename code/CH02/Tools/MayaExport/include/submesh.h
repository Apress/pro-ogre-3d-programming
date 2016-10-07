#ifndef _SUBMESH_H
#define _SUBMESH_H

#include "mayaExportLayer.h"
#include "paramList.h"
#include "materialSet.h"

namespace OgreMayaExporter
{
	/***** structure for uvsets info *****/
	typedef struct uvsettag
	{
		short size;					//number of coordinates (between 1 and 3)
	} uvset;
	/***** structure for texture coordinates *****/
	typedef struct texcoordstag
	{
		double u, v, w;				//texture coordinates	
	} texcoords;

	/***** structure for vertex bone assignements *****/
	typedef struct vbatag
	{
		double weight;	//weight
		int jointIdx;	//index of associated joint
	} vba;

	/***** structure for vertex data *****/
	typedef struct vertextag
	{
		double x, y, z;						//vertex coordinates
		MVector n;							//vertex normal
		float r,g,b,a;						//vertex colour
		std::vector<texcoords> texcoords;	//vertex texture coordinates
		std::vector<vba> vbas;				//vertex bone assignements
	} vertex;

	/***** structure for vertex info *****/
	// used to hold indices to access MFnMesh data
	typedef struct vertexInfotag
	{
		int pointIdx;				//index to points list (position)
		int normalIdx;				//index to normals list
		float r,g,b,a;				//colour
		std::vector<float> u;		//u texture coordinates
		std::vector<float> v;		//v texture coordinates
		std::vector<float> vba;		//vertex bone assignements
		std::vector<int> jointIds;	//ids of joints affecting this vertex
		int next;					//index of next vertex with same position
	} vertexInfo;

	/***** structure for face info *****/
	typedef struct facetag
	{
		long v[3];		//vertex indices
	} face;

	/***** array of face infos *****/
	typedef std::vector<face> faceArray;

	/***** Class Submesh *****/
	class Submesh
	{
	public:
		//constructor
		Submesh(const MString& name = "");
		//destructor
		~Submesh();
		//clear data
		void clear();
		//load data
		MStatus loadMaterial(MObject& shader,MStringArray& uvsets,ParamList& params);
		MStatus load(std::vector<face>& faces, std::vector<vertexInfo>& vertInfo, MFloatPointArray& points,
			MFloatVectorArray& normals, MStringArray& texcoordsets,ParamList& params,bool opposite = false); 
		//get number of triangles composing the submesh
		long numTriangles();
		//get number of vertices
		long numVertices();
		//get submesh name
		MString& name();
		//write submesh data to Ogre XML
		MStatus writeXML(ParamList &params);

	protected:
		//internal members
		MString m_name;
		Material* m_pMaterial;
		long m_numTriangles;
		long m_numVertices;
		std::vector<vertex> m_vertices;
		std::vector<face> m_faces;
		std::vector<uvset> m_uvsets;
		bool m_use32bitIndexes;
	};

}; // end of namespace

#endif