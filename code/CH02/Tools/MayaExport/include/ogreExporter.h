
#ifndef OGRE_EXPORTER_H
#define OGRE_EXPORTER_H

#include "mesh.h"
#include "particles.h"
#include "mayaExportLayer.h"
#include <maya/MPxCommand.h>
#include <maya/MFnPlugin.h>

namespace OgreMayaExporter
{
	class OgreExporter : public MPxCommand
	{
	public:
		// public methods
		OgreExporter();
		virtual ~OgreExporter(){};
		static void* creator();
		MStatus doIt(const MArgList& args);
		bool isUndoable() const;
		MStatus translateNode(MDagPath& dagPath);
		MStatus writeAnim(MFnAnimCurve& anim);
		MStatus writeCamera(MFnCamera& camera);

	private:
		// private members
		MStatus stat;
		ParamList m_params;
		Mesh* m_pMesh;
		MaterialSet* m_pMaterialSet;

		void exit();
	};




	/*********************************************************************************************
	*                                  INLINE Functions                                         *
	*********************************************************************************************/
	// Standard constructor
	inline OgreExporter::OgreExporter()
	{
		MGlobal::displayInfo("Translating scene to OGRE format");
	}

	// Routine for creating the plug-in
	inline void* OgreExporter::creator()
	{
		return new OgreExporter();
	}

	// It tells that this command is not undoable
	inline bool OgreExporter::isUndoable() const
	{
		MGlobal::displayInfo("Command is not undoable");
		return false;
	}

	// Routine for registering the command within Maya
	MStatus initializePlugin( MObject obj )
	{
		MStatus   status;
		MFnPlugin plugin( obj, "OgreExporter", "6.5", "Any");
		status = plugin.registerCommand( "ogreExport", OgreExporter::creator );
		if (!status) {
			status.perror("registerCommand");
			return status;
		}

		return status;
	}

	// Routine for unregistering the command within Maya
	MStatus uninitializePlugin( MObject obj)
	{
		MStatus   status;
		MFnPlugin plugin( obj );
		status = plugin.deregisterCommand( "ogreExport" );
		if (!status) {
			status.perror("deregisterCommand");
			return status;
		}

		return status;
	}

}	//end namespace
#endif