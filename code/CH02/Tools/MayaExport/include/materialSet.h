#ifndef _MATERIALSET_H
#define _MATERIALSET_H

#include "singleton.h"
#include "material.h"
#include "mayaExportLayer.h"

namespace OgreMayaExporter
{
	class MaterialSet : public Singleton<MaterialSet>
	{
	public:
		//constructor
		MaterialSet(){};
		//destructor
		~MaterialSet(){
			clear();
		}
		//clear
		void clear(){
			for (int i=0; i<m_materials.size(); i++)
				delete m_materials[i];
			m_materials.clear();
		}
		//add material
		void addMaterial(Material* pMat){
			bool found = false;
			for (int i=0; i<m_materials.size() && !found; i++)
			{
				if (m_materials[i]->name() == pMat->name())
				{
					found = true;
					delete pMat;
				}
			}
			if (!found)
				m_materials.push_back(pMat);
		}
		//get material
		Material* getMaterial(const MString& name){
			for (int i=0; i<m_materials.size(); i++)
			{
				if (m_materials[i]->name() == name)
					return m_materials[i];
			}
			return NULL;
		};
		//get material set
		static MaterialSet& getSingleton(){
			assert(ms_Singleton);  
			return (*ms_Singleton);
		};
		static MaterialSet* getSingletonPtr(){
			return ms_Singleton;
		};
		//write materials to Ogre XML
		MStatus writeXML(ParamList &params){
			MStatus stat;
			for (int i=0; i<m_materials.size(); i++)
			{
				stat = m_materials[i]->writeXML(params);
				if (MS::kSuccess != stat)
				{
					MString msg = "Error writing material ";
					msg += m_materials[i]->name();
					msg += ", aborting operation";
					MGlobal::displayInfo(msg);
				}
			}
			return MS::kSuccess;
		};

	protected:
		std::vector<Material*> m_materials;
	};

	template<> MaterialSet* Singleton<MaterialSet>::ms_Singleton = 0;

};	//end namespace

#endif