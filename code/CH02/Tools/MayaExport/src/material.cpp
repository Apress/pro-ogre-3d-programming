#include "material.h"

namespace OgreMayaExporter
{
	// Constructor
	Material::Material()
	{
		clear();
	}


	// Destructor
	Material::~Material()
	{
	}


	// Get material name
	MString& Material::name()
	{
		return m_name;
	}


	// Clear data
	void Material::clear()
	{
		m_name = "";
		m_type = MT_LAMBERT;
		m_lightingOff = false;
		m_isTransparent = false;
		m_isTextured = false;
		m_isMultiTextured = false;
		m_ambient = MColor(0,0,0,0);
		m_diffuse = MColor(0,0,0,0);
		m_specular = MColor(0,0,0,0);
		m_emissive = MColor(0,0,0,0);
		m_textures.clear();
	}


	// Load material data
	MStatus Material::load(MFnLambertShader* pShader,MStringArray& uvsets,ParamList& params)
	{
		MStatus stat;
		clear();
		//read material name, adding the requested prefix
		m_name = params.matPrefix;
		if (m_name != "")
			m_name += "/";
		m_name += pShader->name();
		//check if we want to export with lighting off option
		m_lightingOff = params.lightingOff;

		MFnPhongShader* pPhong = NULL;
		MFnBlinnShader* pBlinn = NULL;
		MPlugArray colorSrcPlugs;
		MPlugArray texSrcPlugs;
		MPlugArray placetexSrcPlugs;

		// GET MATERIAL DATA

		// Check material type
		if (pShader->object().hasFn(MFn::kPhong))
		{
			m_type = MT_PHONG;
			pPhong = new MFnPhongShader(pShader->object());
		}
		else if (pShader->object().hasFn(MFn::kBlinn))
		{
			m_type = MT_BLINN;
			pBlinn = new MFnBlinnShader(pShader->object());
		}

		// Check if material is textured
		pShader->findPlug("color").connectedTo(colorSrcPlugs,true,false);
		for (int i=0; i<colorSrcPlugs.length(); i++)
		{
			if (colorSrcPlugs[i].node().hasFn(MFn::kFileTexture))
			{
				m_isTextured = true;
				continue;
			}
			else if (colorSrcPlugs[i].node().hasFn(MFn::kLayeredTexture))
			{
				m_isTextured = true;
				m_isMultiTextured = true;
				continue;
			}
		}

		// Check if material is transparent
		if (pShader->findPlug("transparency").isConnected() || pShader->transparency().r>0.0f)
			m_isTransparent = true;

		// Get material colours
		//diffuse colour
		if (m_isTextured)
			m_diffuse = MColor(1.0,1.0,1.0,1.0);
		else
		{
			m_diffuse = pShader->color();
			m_diffuse.a = 1.0 - pShader->transparency().r;
		}
		//ambient colour
		m_ambient = m_diffuse;
		//emissive colour
		m_emissive = pShader->incandescence();
		//specular colour
		switch(m_type)
		{
		case MT_PHONG:
			m_specular = pPhong->specularColor();
			m_specular.a = pPhong->cosPower();
			break;
		case MT_BLINN:
			m_specular = pBlinn->specularColor();
			m_specular.a = 1.0 / pBlinn->eccentricity();
			break;
		default:
			m_specular = MColor(0.0,0.0,0.0,0.0);
		}

		// Get textures data
		if (m_isTextured)
		{
			// Translate multiple textures if material is multitextured
			if (m_isMultiTextured)
			{
				// Get layered texture node
				MFnDependencyNode* pLayeredTexNode = NULL;
				pShader->findPlug("color").connectedTo(colorSrcPlugs,true,false);
				for (i=0; i<colorSrcPlugs.length(); i++)
				{
					if (colorSrcPlugs[i].node().hasFn(MFn::kLayeredTexture))
					{
						pLayeredTexNode = new MFnDependencyNode(colorSrcPlugs[i].node());
						continue;
					}
				}

				// Get inputs to layered texture
				MPlug inputsPlug = pLayeredTexNode->findPlug("inputs");

				// Scan inputs and export textures
				for (i=inputsPlug.numElements()-1; i>=0; i--)
				{
					MFnDependencyNode* pTextureNode = NULL;
					// Search for a connected texture
					inputsPlug[i].child(0).connectedTo(colorSrcPlugs,true,false);
					for (int j=0; j<colorSrcPlugs.length(); j++)
					{
						if (colorSrcPlugs[j].node().hasFn(MFn::kFileTexture))
						{
							pTextureNode = new MFnDependencyNode(colorSrcPlugs[j].node());
							continue;
						}
					}

					// Translate the texture if it was found
					if (pTextureNode)
					{
						// Get blend mode
						TexOpType opType;
						short bm;
						inputsPlug[i].child(2).getValue(bm);
						switch(bm)
						{				
						case 0:
							opType = TOT_REPLACE;
							break;
						case 1:
							opType = TOT_ALPHABLEND;
							break;				
						case 4:
							opType = TOT_ADD;
							break;
						case 6:
							opType = TOT_MODULATE;
							break;
						default:
							opType = TOT_MODULATE;
						}

						stat = loadTexture(pTextureNode,opType,uvsets,params);
						delete pTextureNode;
						if (MS::kSuccess != stat)
						{
							std::cout << "Error loading layered texture\n";
							delete pLayeredTexNode;
							return MS::kFailure;
						}
					}
				}
				if (pLayeredTexNode)
					delete pLayeredTexNode;
			}
			// Else translate the single texture
			else
			{
				// Get texture node
				MFnDependencyNode* pTextureNode = NULL;
				pShader->findPlug("color").connectedTo(colorSrcPlugs,true,false);
				for (i=0; i<colorSrcPlugs.length(); i++)
				{
					if (colorSrcPlugs[i].node().hasFn(MFn::kFileTexture))
					{
						pTextureNode = new MFnDependencyNode(colorSrcPlugs[i].node());
						continue;
					}
				}
				if (pTextureNode)
				{
					TexOpType opType = TOT_MODULATE;
					stat = loadTexture(pTextureNode,opType,uvsets,params);
					delete pTextureNode;
					if (MS::kSuccess != stat)
					{
						std::cout << "Error loading texture\n";
						return MS::kFailure;
					}
				}
			}
		}
		// Free up memory
		if (pPhong)
			delete pPhong;
		if (pBlinn)
			delete pBlinn;

		return MS::kSuccess;
	}


	// Load texture data from a texture node
	MStatus Material::loadTexture(MFnDependencyNode* pTexNode,TexOpType& opType,MStringArray& uvsets,ParamList& params)
	{
		texture tex;
		// Get texture filename
		MString filename, absFilename;
		MRenderUtil::exactFileTextureName(pTexNode->object(),absFilename);
		filename = absFilename.substring(absFilename.rindex('/')+1,absFilename.length()-1);
		MString command = "toNativePath(\"";
		command += absFilename;
		command += "\")";
		MGlobal::executeCommand(command,absFilename);
		tex.absFilename = absFilename;
		tex.filename = filename;
		tex.uvsetIndex = 0;
		tex.uvsetName = "";
		// Set texture operation type
		tex.opType = opType;
		// Get connections to uvCoord attribute of texture node
		MPlugArray texSrcPlugs;
		pTexNode->findPlug("uvCoord").connectedTo(texSrcPlugs,true,false);
		// Get place2dtexture node (if connected)
		MFnDependencyNode* pPlace2dTexNode = NULL;
		for (int j=0; j<texSrcPlugs.length(); j++)
		{
			if (texSrcPlugs[j].node().hasFn(MFn::kPlace2dTexture))
			{
				pPlace2dTexNode = new MFnDependencyNode(texSrcPlugs[j].node());
				continue;
			}
		}
		// Get uvChooser node (if connected)
		MFnDependencyNode* pUvChooserNode = NULL;
		if (pPlace2dTexNode)
		{
			MPlugArray placetexSrcPlugs;
			pPlace2dTexNode->findPlug("uvCoord").connectedTo(placetexSrcPlugs,true,false);
			for (j=0; j<placetexSrcPlugs.length(); j++)
			{
				if (placetexSrcPlugs[j].node().hasFn(MFn::kUvChooser))
				{
					pUvChooserNode = new MFnDependencyNode(placetexSrcPlugs[j].node());
					continue;
				}
			}
		}
		// Get uvset index
		if (pUvChooserNode)
		{
			bool foundMesh = false;
			bool foundUvset = false;
			MPlug uvsetsPlug = pUvChooserNode->findPlug("uvSets");
			MPlugArray uvsetsSrcPlugs;
			for (int i=0; i<uvsetsPlug.evaluateNumElements() && !foundMesh; i++)
			{
				uvsetsPlug[i].connectedTo(uvsetsSrcPlugs,true,false);
				for (j=0; j<uvsetsSrcPlugs.length() && !foundMesh; j++)
				{
					if (uvsetsSrcPlugs[j].node().hasFn(MFn::kMesh))
					{
						uvsetsSrcPlugs[j].getValue(tex.uvsetName);
						for (int k=0; k<uvsets.length() && !foundUvset; k++)
						{
							if (uvsets[k] == tex.uvsetName)
							{
								tex.uvsetIndex = k;
								foundUvset = true;
							}
						}
					}
				}
			}
		}
		// add texture to material texture list
		m_textures.push_back(tex);
		// free up memory
		if (pUvChooserNode)
			delete pUvChooserNode;
		if (pPlace2dTexNode)
			delete pPlace2dTexNode;

		return MS::kSuccess;
	}


	// Write material data to Ogre XML file
	MStatus Material::writeXML(ParamList &params)
	{
		//Start material description
		params.outMaterial << "material " << m_name.asChar() << "\n";
		params.outMaterial << "{\n";

		//Start technique description
		params.outMaterial << "\ttechnique\n";
		params.outMaterial << "\t{\n";

		//Start render pass description
		params.outMaterial << "\t\tpass\n";
		params.outMaterial << "\t\t{\n";
		//set lighting off option if requested
		if (m_lightingOff)
			params.outMaterial << "\t\t\tlighting off\n\n";
		//ambient colour
		params.outMaterial << "\t\t\tambient " << m_ambient.r << " " << m_ambient.g << " " << m_ambient.b
			<< " " << m_ambient.a << "\n";
		//diffuse colour
		params.outMaterial << "\t\t\tdiffuse " << m_diffuse.r << " " << m_diffuse.g << " " << m_diffuse.b
			<< " " << m_diffuse.a << "\n";
		//specular colour
		params.outMaterial << "\t\t\tspecular " << m_specular.r << " " << m_specular.g << " " << m_specular.b
			<< " " << m_specular.a << "\n";
		//emissive colour
		params.outMaterial << "\t\t\temissive " << m_emissive.r << " " << m_emissive.g << " " 
			<< m_emissive.b << "\n";
		//if material is transparent set blend mode and turn off depth_writing
		if (m_isTransparent)
		{
			params.outMaterial << "\n\t\t\tscene_blend alpha_blend\n";
			params.outMaterial << "\t\t\tdepth_write off\n";
		}
		//write texture units
		for (int i=0; i<m_textures.size(); i++)
		{
			//start texture unit description
			params.outMaterial << "\n\t\t\ttexture_unit\n";
			params.outMaterial << "\t\t\t{\n";
			//write texture name
			params.outMaterial << "\t\t\t\ttexture " << m_textures[i].filename.asChar() << "\n";
			//write texture coordinate index
			params.outMaterial << "\t\t\t\ttex_coord_set " << m_textures[i].uvsetIndex << "\n";
			//write colour operation
			switch (m_textures[i].opType)
			{
			case TOT_REPLACE:
				params.outMaterial << "\t\t\t\tcolour_op replace\n";
				break;
			case TOT_ADD:
				params.outMaterial << "\t\t\t\tcolour_op add\n";
				break;
			case TOT_MODULATE:
				params.outMaterial << "\t\t\t\tcolour_op modulate\n";
				break;
			case TOT_ALPHABLEND:
				params.outMaterial << "\t\t\t\tcolour_op alpha_blend\n";
				break;
			}
			//end texture unit desription
			params.outMaterial << "\t\t\t}\n";
		}

		//End render pass description
		params.outMaterial << "\t\t}\n";

		//End technique description
		params.outMaterial << "\t}\n";

		//End material description
		params.outMaterial << "}\n";

		//Copy textures to output dir if required
		if (params.copyTextures)
			copyTextures(params);

		return MS::kSuccess;
	}


	// Copy textures to path specified by params
	MStatus Material::copyTextures(ParamList &params)
	{
		for (int i=0; i<m_textures.size(); i++)
		{
			// Copy file texture to output dir
			MString command = "copy \"";
			command += m_textures[i].absFilename;
			command += "\" \"";
			command += params.texOutputDir;
			command += "\"";
			system(command.asChar());
		}
		return MS::kSuccess;
	}

};	//end namespace