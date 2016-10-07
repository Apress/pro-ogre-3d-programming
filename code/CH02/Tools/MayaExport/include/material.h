#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "mayaExportLayer.h"
#include "paramList.h"

namespace OgreMayaExporter
{

	typedef enum MaterialTypeTag {MT_LAMBERT,MT_PHONG,MT_BLINN} MaterialType;

	typedef enum TexOpTypeTag {TOT_REPLACE,TOT_MODULATE,TOT_ADD,TOT_ALPHABLEND} TexOpType;

	typedef struct textureTag
	{
		MString filename;
		MString absFilename;
		TexOpType opType;
		MString uvsetName;
		int uvsetIndex;
	} texture;


	/***** Class Material *****/
	class Material
	{
	public:
		//constructor
		Material();
		//destructor
		~Material();
		//get material name
		MString& name();
		//clear material data
		void clear();
		//load material data
		MStatus load(MFnLambertShader* pShader,MStringArray& uvsets,ParamList& params);
		//write material data to Ogre XML
		MStatus writeXML(ParamList &params);
		//copy textures to path specified by params
		MStatus copyTextures(ParamList &params);
	protected:
		//load texture data
		MStatus loadTexture(MFnDependencyNode* pTexNode,TexOpType& opType,MStringArray& uvsets,ParamList& params);

		MString m_name;
		MaterialType m_type;
		MColor m_ambient, m_diffuse, m_specular, m_emissive;
		bool m_lightingOff;
		bool m_isTransparent;
		bool m_isTextured;
		bool m_isMultiTextured;
		std::vector<texture> m_textures;
	};

};	//end of namespace

#endif