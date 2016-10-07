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

#include "windows.h"
#include "max.h"
#include "plugapi.h"
#include "stdmat.h"
#include "impexp.h"
#include "CS/BipedApi.h"
#include "CS/KeyTrack.h"
#include "CS/phyexp.h"
#include "iparamb2.h"
#include "iskin.h"
#include "OgreExport.h"
#include "resource.h"

#include <string>
#include <fstream>
#include <list>
#include <queue>

static OgreMaxExport* _exp = 0;

INT_PTR CALLBACK ExportPropertiesDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	std::string filename;

	switch(message) {
		case WM_INITDIALOG:
			_exp = (OgreMaxExport*) lParam;

			if (_exp == 0) {
				MessageBox(NULL, "Error: Cannot initialize exporter options dialog, aborting", "Error", MB_ICONEXCLAMATION);
				EndDialog(hDlg, 0);
				return TRUE;
			}

			_exp->m_hWndDlgExport = hDlg;
    		
			CenterWindow(hDlg,GetParent(hDlg));

			// initialize controls on the dialog
			EnableWindow(GetDlgItem(hDlg, IDC_CHK_SHARE_SKELETON), FALSE);
			CheckDlgButton(hDlg, IDC_RADIO_EXPORT_SUBMESHES, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_CHK_SHARE_SKELETON, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_CHK_FLIP_YZ, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_CHK_REBUILD_NORMALS, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_UV, BST_CHECKED);

			SendMessage(GetDlgItem(hDlg, IDC_TXT_SCALE), WM_SETTEXT, 0, (LPARAM)_T("1.0"));
			SendMessage(GetDlgItem(hDlg, IDC_TXT_DEFAULT_MATERIAL), WM_SETTEXT, 0, (LPARAM)_T("DefaultMaterial"));

			// populate the output directory box
			filename = _exp->m_filename;
			_exp->m_exportPath = filename.substr(0, filename.find_last_of("\\"));
			_exp->m_exportFilename = filename.substr(filename.find_last_of("\\") + 1);

			_exp->m_materialFilename = _exp->m_exportFilename;
			_exp->m_materialFilename = _exp->m_materialFilename.substr(0, _exp->m_materialFilename.find(".mesh.xml")) + ".material";
			_exp->m_skeletonFilename = _exp->m_exportFilename.substr(0, _exp->m_exportFilename.find(".mesh.xml")) + ".skeleton.xml";
			SendMessage(GetDlgItem(hDlg, IDC_TXT_MATERIAL_FILENAME), WM_SETTEXT, 0, (LPARAM)_exp->m_materialFilename.c_str());
			SendMessage(GetDlgItem(hDlg, IDC_TXT_SKELETON_FILENAME), WM_SETTEXT, 0, (LPARAM)_exp->m_skeletonFilename.c_str());

			SendMessage(GetDlgItem(hDlg, IDC_TXT_EXPORT_DIR), WM_SETTEXT, 0, (LPARAM)_exp->m_exportPath.c_str());
			EnableWindow(GetDlgItem(hDlg, IDC_TXT_EXPORT_DIR), FALSE);

			// set up animation listbox
			{
				int frameStart = _exp->m_i->GetAnimRange().Start();
				int frameEnd = _exp->m_i->GetAnimRange().End();

				HWND anims = GetDlgItem(hDlg, IDC_LIST_ANIMATIONS);

				Rect r;
				GetWindowRect(anims, &r);

				LVCOLUMN lvc;
				ZeroMemory(&lvc, sizeof(LVCOLUMN));
				lvc.mask = LVCF_TEXT | LVCF_WIDTH;
				lvc.cx = r.w() * 0.6;
				lvc.pszText = "Animation";
				ListView_InsertColumn(anims, 0, &lvc);
				lvc.cx = r.w() * 0.2;
				lvc.pszText = "Begin";
				ListView_InsertColumn(anims, 1, &lvc);
				lvc.pszText = "End";
				ListView_InsertColumn(anims, 2, &lvc);

				// add a spanning entry to the animation list as a default
				LVITEM lvi;
				char buf[32];
				ZeroMemory(&lvi, sizeof(LVITEM));

				lvi.mask = LVIF_TEXT;
				lvi.pszText = "Animation";
				lvi.iItem = 10000;
				int idx = ListView_InsertItem(anims, &lvi);

				sprintf(buf, "%d", frameStart / GetTicksPerFrame());
				lvi.iItem = idx;
				lvi.iSubItem = 1;
				lvi.pszText = buf;
				ListView_SetItem(anims, &lvi);

				sprintf(buf, "%d", frameEnd / GetTicksPerFrame());
				lvi.iSubItem = 2;
				lvi.pszText = buf;
				ListView_SetItem(anims, &lvi);

				// populate the frame range info box
				sprintf(buf, "%d to %d", frameStart / GetTicksPerFrame(), frameEnd / GetTicksPerFrame());
				SendMessage(GetDlgItem(hDlg, IDC_TXT_FRAME_RANGE), WM_SETTEXT, 0, (LPARAM)buf);
			}

			return TRUE;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_SELECT_EXPORT_DIR:
					break;
				case IDC_RADIO_EXPORT_FILES:
				case IDC_RADIO_EXPORT_SUBMESHES:
					_exp->updateExportOptions(hDlg);
					break;
				case IDOK:
				case IDC_EXPORT:
					if (_exp->export())
						EndDialog(hDlg, 1);
					else
						EndDialog(hDlg, 2);
					return TRUE;
				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;

				case IDC_CMD_ADD_ANIMATION:
					_exp->addAnimation();
					break;

				case IDC_CMD_DELETE_ANIMATION:
					_exp->deleteAnimation();
					break;
			}
			break;
	}
	return FALSE;

}

void OgreMaxExport::addAnimation() {
	char buf[256];
	int start, end;
	HWND anims = GetDlgItem(m_hWndDlgExport, IDC_LIST_ANIMATIONS);

	// ignore FPS field for now
	SendMessage(GetDlgItem(m_hWndDlgExport, IDC_TXT_FPS), WM_GETTEXT, 256, (LPARAM)buf);
	int fps = atoi(buf);

	if (fps < 0) {
		MessageBox(NULL, "FPS must be >= 0", "Invalid Entry", MB_ICONEXCLAMATION);
		return;
	}

	int minAnimTime = m_i->GetAnimRange().Start() / GetTicksPerFrame();
	int maxAnimTime = m_i->GetAnimRange().End() / GetTicksPerFrame();

	// get animation start and end times
	SendMessage(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIM_START), WM_GETTEXT, 256, (LPARAM)buf);
	start = atoi(buf);

	if (start < minAnimTime) {
		sprintf(buf, "Start time must be >= %d", start);
		MessageBox(NULL, buf, "Invalid Entry", MB_ICONEXCLAMATION);
		return;
	}

	SendMessage(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIM_END), WM_GETTEXT, 256, (LPARAM)buf);
	end = atoi(buf);

	if (end > maxAnimTime) {
		sprintf(buf, "End time must be <= %d", end);
		MessageBox(NULL, buf, "Invalid Entry", MB_ICONEXCLAMATION);
		return;
	}

	// get animation name
	SendMessage(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIMATION_NAME), WM_GETTEXT, 256, (LPARAM)buf);
	std::string name(buf);

	if (name.length() == 0) {
		MessageBox(NULL, "Animation name must not be empty", "Invalid Entry", MB_ICONEXCLAMATION);
		return;
	}

	// if, after all that, we have valid data, stick it in the listview
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));

	lvi.mask = LVIF_TEXT;
	lvi.pszText = buf;
	lvi.iItem = 10000;
	int idx = ListView_InsertItem(anims, &lvi);

	lvi.iItem = idx;
	lvi.iSubItem = 1;
	sprintf(buf, "%d", start);
	lvi.pszText = buf;
	ListView_SetItem(anims, &lvi);
	lvi.iSubItem = 2;
	sprintf(buf, "%d", end);
	lvi.pszText = buf;
	ListView_SetItem(anims, &lvi);

	// Finally, clear out the entry controls
	SetWindowText(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIMATION_NAME), "");
	SetWindowText(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIM_START), "");
	SetWindowText(GetDlgItem(m_hWndDlgExport, IDC_TXT_ANIM_END), "");
}

void OgreMaxExport::deleteAnimation() {

	HWND anims = GetDlgItem(m_hWndDlgExport, IDC_LIST_ANIMATIONS);

	// delete selected animation(s) from the listview
	int idx;
	while ((idx=ListView_GetNextItem(anims, -1, LVNI_SELECTED)) != -1)
		ListView_DeleteItem(anims, idx);
}

void OgreMaxExport::updateExportOptions(HWND hDlg) {

	// ***************************************************************************
	// adjust enabled state of share-skeleton checkbox if the user chose to 
	// export each mesh to an individual file -- this ultimately will instruct the exporter
	// not to create a .skeleton.xml file for each .mesh.xml, and instead assign a
	// common .skeleton filename to each exported .mesh.xml

	HWND hChk = GetDlgItem(hDlg, IDC_CHK_SHARE_SKELETON);
	EnableWindow(hChk, IsDlgButtonChecked(hDlg, IDC_RADIO_EXPORT_FILES));

	m_exportMultipleFiles = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_EXPORT_FILES));
	m_useSingleSkeleton = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHK_SHARE_SKELETON));
	m_rebuildNormals = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHK_REBUILD_NORMALS));
	m_invertNormals = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHK_INVERT_NORMALS));
	m_flipYZ = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHK_FLIP_YZ));
	m_exportVertexColors = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHK_VERTEX_COLORS));

	if (SendMessage(GetDlgItem(hDlg, IDC_TXT_MATERIAL_FILENAME), WM_GETTEXTLENGTH, 0, 0) == 0) {
		m_exportMaterial = false;
	}
	else {
		m_exportMaterial = true;
	}

	if (SendMessage(GetDlgItem(hDlg, IDC_TXT_SCALE), WM_GETTEXTLENGTH, 0, 0) == 0) 
		m_scale = 1.0f;
	else {
		char buf[16];
		SendMessage(GetDlgItem(hDlg, IDC_TXT_SCALE), WM_GETTEXT, 16, (LPARAM)buf);
		m_scale = atof(buf);

		if (m_scale == 0.0f)
			m_scale = 1.0f;
	}

	TCHAR buf[256];
	if (SendMessage(GetDlgItem(hDlg, IDC_TXT_DEFAULT_MATERIAL), WM_GETTEXT, 256, (LPARAM)buf) > 0)
		m_defaultMaterialName = buf;
	else
		m_defaultMaterialName = _T("DefaultMaterial");

	if (SendMessage(GetDlgItem(hDlg, IDC_TXT_SKELETON_FILENAME), WM_GETTEXT, 256, (LPARAM)buf) > 0)
		m_skeletonFilename = buf;

	if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_UV))
		m_2DTexCoord = UV;
	if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_VW))
		m_2DTexCoord = VW;
	if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_WU))
		m_2DTexCoord = WU;

	// update the list of named animations from the list view control
	int animIdx = -1;
	HWND anims = GetDlgItem(hDlg, IDC_LIST_ANIMATIONS);

	m_animations.clear();
	while ((animIdx=ListView_GetNextItem(anims, animIdx, LVNI_ALL)) != -1) {
		
		LVITEM lvi;
		NamedAnimation anim;
		char buf[256];
		ZeroMemory(&lvi, sizeof(LVITEM));

		lvi.mask = LVIF_TEXT;
		lvi.cchTextMax = 256;
		lvi.pszText = buf;
		lvi.iItem = animIdx;

		ListView_GetItem(anims, &lvi);
		anim.name = std::string(buf);

		lvi.iSubItem = 1;
		ListView_GetItem(anims, &lvi);
		anim.start = atoi(buf);

		lvi.iSubItem = 2;
		ListView_GetItem(anims, &lvi);
		anim.end = atoi(buf);

		m_animations.push_back(anim);
	}
}

OgreMaxExport::OgreMaxExport(HINSTANCE hInst) : m_exportPath(""), m_exportFilename("") {
	m_hInstance = hInst;
	m_hWndDlgExport = 0;
	m_ei = 0;
	m_i = 0;

	m_exportMultipleFiles = true;		// default is to export a file per mesh object in the scene
	m_useSingleSkeleton = true;			// default for multiple meshes is to reference a single .skeleton file where applicable
	m_rebuildNormals = false;			// rebuild the normals before exporting mesh data

	m_exportMaterial = true;			// default is to export material scripts
	m_defaultMaterialName = "DefaultMaterial";
	m_2DTexCoord = UV;					// default is UV interpretation of 2D tex coords

	m_exportOnlySelectedNodes = false;	// this corresponds to the "Export..." vs "Export Selected..." menu items
	m_invertNormals = false;			// flip normals; will also reorder triangle vertex indices
	m_flipYZ = false;					// swap X and Z axes, so that Y becomes the One True Up Vector
	m_exportVertexColors = false;		// useful for vertex painting
	m_scale = 1.0f;						// export at normal size (scale) -- all vertices get multiplied by this

	m_skeletonLink = false;				// initially we don't assume any skeletal data
	m_currentBoneIndex = 0;				// used to map bone names to bone indices for vertex assignment and later skeleton export

	m_fps = 25;							// used for controller types (such as Biped) that do not support keyframes directly -- this is the sampling rate
}

OgreMaxExport::~OgreMaxExport() {
}

int OgreMaxExport::ExtCount() {
	// only support one filename extension in this plugin
	return 1;
}

const TCHAR * OgreMaxExport::Ext(int n) {
	switch (n) {
		case 0:
			return _T("xml");
			break;
		default:
			return 0;
			break;
	}
}

const TCHAR * OgreMaxExport::LongDesc() { 
	return _T("Ogre Mesh/Animation/Material Exporter");
}

const TCHAR * OgreMaxExport::ShortDesc() {
	return _T("Ogre XML");
}

const TCHAR * OgreMaxExport::AuthorName() { 
	return _T("Gregory 'Xavier' Junker");
}

const TCHAR * OgreMaxExport::CopyrightMessage() { 
	return _T("The OGRE Team (c) 2006");
}

const TCHAR * OgreMaxExport::OtherMessage1() { 
	return 0;
}

const TCHAR * OgreMaxExport::OtherMessage2() { 
	return 0;
}

unsigned int OgreMaxExport::Version() { 
	return 100;
}

void OgreMaxExport::ShowAbout(HWND hWnd) {
	MessageBox(hWnd, "Ogre (Dagon) Mesh, Material and Animation Exporter", "About", 0);
}

int	OgreMaxExport::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options) {

	// massage the filename -- if it does not end with .mesh.xml, make it do so
	m_filename = name;
	if (m_filename.find(".mesh.xml") == std::string::npos) {
		m_filename = m_filename.substr(0, m_filename.find(".XML"));
		m_filename += ".mesh.xml";
	}

	m_ei = ei;
	m_i = i;

	// Max will supply a nonzero (specifically, SCENE_EXPORT_SELECTED) value for options if the user
	// chose "Export Selected..." instead of "Export..." from the File menu
	m_exportOnlySelectedNodes = (options == SCENE_EXPORT_SELECTED);

	int result = DialogBoxParam(m_hInstance,
									MAKEINTRESOURCE(IDD_EXPORT),
									GetActiveWindow(),
									ExportPropertiesDialogProc,
									(LPARAM) this);

	switch (result) {
		case 0:
			return IMPEXP_CANCEL;
			break;
		case 1:
			MessageBox(GetActiveWindow(), "Export Succeeded", "Sucessful Export", MB_ICONINFORMATION);
			return IMPEXP_SUCCESS;
			break;
		default:
			return IMPEXP_FAIL;
			break;
	}
}

BOOL OgreMaxExport::SupportsOptions(int ext, DWORD options) {

	// currently, only SCENE_EXPORT_SELECTED is passed to this; we support exporting
	// of selected files only, so return TRUE (if they ever add anything later, we'll 
	// either support it too, or check what they are asking and return accordingly).
	return TRUE;
}

// pulled directly from the Sparks site: 
// http://sparks.discreet.com/Knowledgebase/sdkdocs_v8/prog/cs/cs_physique_export.html
// Also available in the SDK docs. Used to find out if this node has a physique modifier or not.
// If it does, it returns a pointer to the modifier, and if not, returns NULL. This can be used to 
// determine whether a node is bone or mesh -- mesh nodes will have Physique modifiers, bone nodes
// will not.
Modifier* OgreMaxExport::FindPhysiqueModifier (INode* nodePtr)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = nodePtr->GetObjectRef();

	if (!ObjectPtr) return NULL;

	// Is derived object ?
	while (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID && ObjectPtr)
	{
		// Yes -> Cast.
		IDerivedObject *DerivedObjectPtr = (IDerivedObject *)(ObjectPtr);
						
		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		ObjectPtr = DerivedObjectPtr->GetObjRef();
	}

	// Not found.
	return NULL;
}

// "callback" is called in response to the EnumTree() call made below. That call visits every node in the 
// scene and calls this procedure for each one. 
int OgreMaxExport::callback(INode *node) {

	// SKELOBJ_CLASS_ID = 0x9125 = 37157
	// BIPED_CLASS_ID = 0x9155 = 37205
	// BIPSLAVE_CONTROL_CLASS_ID = 0x9154 = 37204
	// BIPBODY_CONTROL_CLASS_ID = 0x9156 = 37206
	// FOOTPRINT_CLASS_ID = 0x3011 = 12305
	// DUMMY_CLASS_ID = 0x876234 = 8872500

	TimeValue start = m_i->GetAnimRange().Start();
	Object *obj = node->EvalWorldState(start).obj;
	Class_ID cid = obj->ClassID();

	// nodes that have Biped controllers are bones -- ignore the ones that are dummies
	if (cid == Class_ID(DUMMY_CLASS_ID, 0))
		return TREE_CONTINUE;

	Control *c = node->GetTMController();
	if ((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
		(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ||
		(c->ClassID() == FOOTPRINT_CLASS_ID)) {

			if (node->GetParentNode() != NULL) {
				// stick this in the bone-index map for later use
				m_boneIndexMap.insert(std::map< std::string, int >::value_type(std::string(node->GetName()), m_currentBoneIndex++));
			}

			return TREE_CONTINUE;
	}
	// if the node cannot be converted to a TriObject (mesh), ignore it
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		return TREE_CONTINUE;

	// create a list of nodes to process
	if (m_exportOnlySelectedNodes) {
		if (node->Selected())
			m_nodeList.push_back(node);
	}
	else {
		m_nodeList.push_back(node);
	}

	return TREE_CONTINUE;
}

bool OgreMaxExport::export() {

	// make sure we have the latest options
	updateExportOptions(m_hWndDlgExport);

	try {

		// all options have been set when actions were taken, so we can just start exporting stuff here
		std::ofstream of;
		bool rtn = true;

		m_nodeList.clear();
		while (!m_submeshNames.empty())
			m_submeshNames.pop();

		m_ei->theScene->EnumTree(this);

		// check to see if there's anything to export
		if (m_nodeList.size() == 0) {
			MessageBox(GetActiveWindow(), "No nodes available to export, aborting...", "Nothing To Export", MB_ICONINFORMATION);
			return false;
		}

		std::string fileName;
		// if we are writing everything to one file, use the name provided when the user first started the export
		if (!m_exportMultipleFiles)
			fileName = m_filename;
		else {
			fileName = m_exportPath + "\\";
			INode *n = *(m_nodeList.begin());
			fileName += n->GetName();
			fileName += ".mesh.xml";
		}

		of.open(fileName.c_str(), std::ios::out);

		if (of.is_open())
			streamFileHeader(of);
		else {
			std::string msg("Could not open output file");
			msg += fileName;
			msg += ", aborting...";
			MessageBox(GetActiveWindow(), msg.c_str(), "File Output Error", MB_ICONEXCLAMATION);
			return false;
		}

		std::list<INode *>::iterator it = m_nodeList.begin();

		Mtl *nodeMtl = (*it)->GetMtl();
		if (nodeMtl == NULL) {
			std::string mtlName;
			mtlName = m_defaultMaterialName;
			m_materialMap.insert(std::map< std::string, Mtl * >::value_type(mtlName, NULL));
		}

		while (it != m_nodeList.end()) {

			// we already filtered out nodes that had NULL materials, and those that could
			// not be converted to TriObjects, so now we know everything we have is good

			INode *node = *it;
			std::string mtlName;
			
			Mtl *mtl = node->GetMtl();
			if (mtl == NULL)
				mtlName = m_defaultMaterialName;
			else
				mtlName = mtl->GetName();

			// duplicate map keys will cause an exception; ignore it and keep going
			try {
				// map a material name to its Mtl pointer so that we can retrieve these later
				std::string::size_type pos;

				// clean out any spaces the user left in their material name
				while ((pos = mtlName.find_first_of(' ')) != std::string::npos)
					mtlName.replace(pos, 1, _T("_"));

				m_materialMap.insert(std::map< std::string, Mtl * >::value_type(mtlName, mtl));
			} catch (...) {}

			if (streamSubmesh(of, node, mtlName))
				m_submeshNames.push(std::string((*it)->GetName()));

			it++;

			// if we are doing one mesh per file, then close this one and open a new one
			if (m_exportMultipleFiles || it == m_nodeList.end()) {
				streamFileFooter(of);
				of.close();

				if (it != m_nodeList.end()) {
					fileName = m_exportPath + "\\";
					INode *n = *it;
					fileName += n->GetName();
					fileName += ".mesh.xml";

					of.open(fileName.c_str(), std::ios::out);

					if (of.is_open())
						streamFileHeader(of);
					else {
						std::string msg("Could not open output file");
						msg += fileName;
						msg += ", aborting...";
						MessageBox(GetActiveWindow(), msg.c_str(), "File Output Error", MB_ICONEXCLAMATION);
						return false;
					}
				}
			}
		}

		// if skeleton data is present, stream skeleton file
		if (m_skeletonLink) {
			// open the skeleton.xml output file
			of.open((m_exportPath + "\\" + m_skeletonFilename).c_str(), std::ios::out);

			// stream the skeleton file
			streamSkeleton(of);
			of.close();
		}

		// stream material file(s)
		TCHAR fName[256];
		HWND hWnd = GetDlgItem(m_hWndDlgExport, IDC_TXT_MATERIAL_FILENAME);

		SendMessage(hWnd, WM_GETTEXT, 256, (LPARAM)fName);
		
		std::string mtlFilename(fName);
		mtlFilename = m_exportPath + "\\" + fName;
		of.open(mtlFilename.c_str(), std::ios::out);
		of.precision(6);
		of << std::fixed;
		streamMaterial(of);
		of.close();

		return rtn;
	}
	catch (...) {
		MessageBox(GetActiveWindow(), "An unexpected error has occurred while trying to export, aborting", "Error", MB_ICONEXCLAMATION);
		return false;
	}
}

bool OgreMaxExport::streamFileHeader(std::ostream &of) {

	// write the XML header tags
	of << "<?xml version=\"1.0\"?>" << std::endl;
	of << "<mesh>" << std::endl;

	// *************** Export Submeshes ***************
	of << "\t<submeshes>" << std::endl;

	of.precision(6);
	of << std::fixed;

	return true;
}

bool OgreMaxExport::streamFileFooter(std::ostream &of) {

	of << "\t</submeshes>" << std::endl;
	// *************** End Submeshes Export ***********

	// if there is a skeleton involved, link that filename here
	if (m_skeletonLink) {
		std::string skeletonFilename(m_skeletonFilename);
		skeletonFilename = skeletonFilename.substr(0, skeletonFilename.find(".xml"));

		of << "\t<skeletonlink name=\"" << skeletonFilename << "\" />" << std::endl;
	}


	// *************** Export Submesh Names ***************
	of << "\t<submeshnames>" << std::endl;

	int idx = 0;
	while (!m_submeshNames.empty()) {
		of << "\t\t<submeshname name=\"" << m_submeshNames.front() << "\" index=\"" << idx << "\" />" << std::endl;
		idx++;
		m_submeshNames.pop();
	}

	of << "\t</submeshnames>" << std::endl;
	// *************** End Submesh Names Export ***********

	of << "</mesh>" << std::endl;

	return true;
}

bool OgreMaxExport::streamPass(std::ostream &of, Mtl *mtl) {
	of << "\t\tpass" << std::endl;
	of << "\t\t{" << std::endl;

	BMM_Color_32 amb32, diff32, spec32, em32;
	ZeroMemory(&amb32, sizeof(BMM_Color_32));
	ZeroMemory(&diff32, sizeof(BMM_Color_32));
	ZeroMemory(&spec32, sizeof(BMM_Color_32));
	ZeroMemory(&em32, sizeof(BMM_Color_32));

	if (mtl != NULL) {
		Color ambient = mtl->GetAmbient();
		amb32 = BMM_Color_32(ambient);

		Color diffuse = mtl->GetDiffuse();
		diff32 = BMM_Color_32(diffuse);

		Color specular = mtl->GetSpecular();
		spec32 = BMM_Color_32(specular);

		Color emissive = mtl->GetSelfIllumColor();
		em32 = BMM_Color_32(emissive);
	}

	of << "\t\t\tambient " << (float)amb32.r/255.0f << " " << (float)amb32.g/255.0f << " " << (float)amb32.b/255.0f << " " << (float)amb32.a/255.0f << std::endl;
	of << "\t\t\tdiffuse " << (float)diff32.r/255.0f << " " << (float)diff32.g/255.0f << " " << (float)diff32.b/255.0f << " " << (float)diff32.a/255.0f << std::endl;
	of << "\t\t\tspecular " << (float)spec32.r/255.0f << " " << (float)spec32.g/255.0f << " " << (float)spec32.b/255.0f << " " << (float)spec32.a/255.0f << " 0.0" << std::endl;
	of << "\t\t\temissive " << (float)em32.r/255.0f << " " << (float)em32.g/255.0f << " " << (float)em32.b/255.0f << " " << (float)em32.a/255.0f << std::endl;

	if (mtl != NULL) {
		// check for diffuse texture
		Texmap *tMap = mtl->GetSubTexmap(ID_DI);
		if (tMap) {
			if (tMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {

				BitmapTex *bmt = (BitmapTex*) tMap;
				std::string mapName(bmt->GetMapName());
				mapName = mapName.substr(mapName.find_last_of('\\') + 1);

				of << "\t\t\ttexture_unit " << std::endl;
				of << "\t\t\t{" << std::endl;
				of << "\t\t\t\ttexture " << mapName << std::endl;
				of << "\t\t\t}" << std::endl;
			}
		}
	}

	of << "\t\t}" << std::endl;

	return true;
}

bool OgreMaxExport::streamMaterial(std::ostream &of) {

	// serialize this information to the material file
	std::map< std::string, Mtl * >::iterator it = m_materialMap.begin();

	while (it != m_materialMap.end()) {
		std::string matName(it->first);
		Mtl *mtl = it->second;

		of << "material " << matName << std::endl;
		of << std::showpoint;
		of << "{" << std::endl;

		of << "\ttechnique" << std::endl;
		of << "\t{" << std::endl;

		int numSubMtl = 0;
		
		if (mtl != NULL) {
			numSubMtl = mtl->NumSubMtls();

			if (numSubMtl > 0) {
				int i;
				for (i=0; i<numSubMtl; i++) {
					streamPass(of, mtl->GetSubMtl(i));
				}
			}
			else
				streamPass(of, mtl);
		}
		else {
			streamPass(of, mtl);
		}

		of << "\t}" << std::endl;
		of << "}" << std::endl;

		it++;
	}

	m_materialMap.clear();

	return true;
}

bool OgreMaxExport::streamSubmesh(std::ostream &of, INode *node, std::string &mtlName) {
	
	Object *obj = node->EvalWorldState(m_i->GetTime()).obj;
	TriObject *tri = (TriObject *) obj->ConvertToType(m_i->GetTime(), Class_ID(TRIOBJ_CLASS_ID, 0));

	int i;
	Mesh mesh = tri->GetMesh();
	Matrix3 ptm = node->GetObjTMAfterWSM(m_i->GetTime());

	int vertCount = mesh.getNumVerts();
	int faceCount = mesh.getNumFaces();

	of << "\t\t<submesh ";
	
	if (mtlName.length() > 0)
		of << "material=\"" << mtlName << "\" ";

	of << "usesharedvertices=\"false\" use32bitindexes=\"";
	of << (vertCount > 65535);
	of << "\">" << std::endl;

	// *************** Export Face List ***************
	of << "\t\t\t<faces count=\"" << faceCount << "\">" << std::endl;

	for (i=0; i<faceCount; i++) {
		int v1 = mesh.faces[i].v[0];
		int v2 = mesh.faces[i].v[1];
		int v3 = mesh.faces[i].v[2];

		if (m_invertNormals) {
			int tmp = v2;
			v2 = v3;
			v3 = tmp;
		}

		of << "\t\t\t\t<face v1=\"" << v1 << "\" v2=\"" << v2 << "\" v3=\"" << v3 << "\" />" << std::endl;
	}

	of << "\t\t\t</faces>" << std::endl;
	// *************** End Export Face List ***************


	// *************** Export Geometry ***************
	of << "\t\t\t<geometry vertexcount=\"" << vertCount << "\">" << std::endl;

	// *************** Export Vertex Buffer ***************
	if (m_rebuildNormals) {
		mesh.buildNormals();
	}

	bool exportNormals = (mesh.normalsBuilt > 0);

	of << std::boolalpha;

	// TODO: get the actual number and dimemsion of tex maps from Max -- for now, fake it
	// NB: we don't export tex coords unless we are exporting a material as well
	int numTexMaps = m_exportMaterial ? 1 : 0;
	of << "\t\t\t\t<vertexbuffer positions=\"true\" normals=\"" << exportNormals << "\" colours_diffuse=\"" << m_exportVertexColors << "\" texture_coords=\"" << numTexMaps << "\"";
	
	for (i=0; i<numTexMaps; i++)
		of << " texture_coords_dimensions_" << i << "=\"2\"";
	
	of << ">" << std::endl;

	for (i=0; i<vertCount; i++) {
		Point3 v = mesh.getVert(i);

		// transform v into parent's coord system
		v = v * ptm;

		Point3 vc;
		vc.x = 0.0f;
		vc.y = 0.0f;
		vc.z = 0.0f;

		if (mesh.vertCol != 0) {
			vc = mesh.vertCol[i];
		}

		of << "\t\t\t\t\t<vertex>" << std::endl;
		of << std::showpoint;

		float x = v.x;// * m_scale;
		float y = v.y;// * m_scale;
		float z = v.z;// * m_scale;

		if (m_flipYZ) {
			float tmp = y;
			y = z;
			z = -tmp;
		}

		of << "\t\t\t\t\t\t<position x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\" />" << std::endl;

		if (m_exportVertexColors)
			of << "\t\t\t\t\t\t<colour_diffuse value=\"\t" << vc.x << "\t" << vc.y << "\t" << vc.z << "\" />" << std::endl;
		
		if (exportNormals) {
//			Point3 n = mesh.getNormal(i);
			RVertex *rv = mesh.getRVertPtr(i);
			Point3 n = rv->rn.getNormal();
			n.Normalize();

			float x = n.x;
			float y = n.y;
			float z = n.z;

			if (m_flipYZ) {
				float tmp = y;
				y = z;
				z = -tmp;
			}
			
			if (m_invertNormals)
				of << "\t\t\t\t\t\t<normal x=\"" << -x << "\" y=\"" << -y << "\" z=\"" << -z << "\" />" << std::endl;
			else
				of << "\t\t\t\t\t\t<normal x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\" />" << std::endl;
		}

		if (i < mesh.getNumTVerts()) {
			for (int t=0; t<numTexMaps; t++) {

				UVVert uv = mesh.getTVert(i);

				switch (m_2DTexCoord) {
					case UV:
						of << "\t\t\t\t\t\t<texcoord u=\"" << uv.x << "\" v=\"" << (1.0f - uv.y) << "\" />" << std::endl; 
						break;
					case VW:
						of << "\t\t\t\t\t\t<texcoord v=\"" << uv.y << "\" w=\"" << (1.0f - uv.z) << "\" />" << std::endl; 
						break;
					case WU:
						of << "\t\t\t\t\t\t<texcoord w=\"" << uv.z << "\" u=\"" << (1.0f - uv.x) << "\" />" << std::endl; 
						break;
				}
			}
		}
		
		of << std::noshowpoint;
		of << "\t\t\t\t\t</vertex>" << std::endl;
	}

	of << "\t\t\t\t</vertexbuffer>" << std::endl;
	// *************** End Export Vertex Buffer ***************

	of << "\t\t\t</geometry>" << std::endl;
	// *************** End Export Geometry ***********

	// this skin extraction code based on an article found here:
	// http://www.cfxweb.net/modules.php?name=News&file=article&sid=1029
	Object *oRef = node->GetObjectRef();

	if (oRef->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		IDerivedObject *dObj = (IDerivedObject *)oRef;
		Modifier *oMod = dObj->GetModifier(0);

		if (oMod->ClassID() == SKIN_CLASSID) {

			// flag the export of a skeleton link element
			m_skeletonLink = true;

			// stream the boneassignments element
			streamBoneAssignments(of, oMod, node);
		}
	}

	of << "\t\t</submesh>" << std::endl;

	if (obj != tri)
		delete tri;

	return true;
}

bool OgreMaxExport::streamBoneAssignments(std::ostream &of, Modifier *oMod, INode *node) {
	

	// wrangle a pointer to the skinning data
	ISkin *skin = (ISkin *) oMod->GetInterface(I_SKIN);
	ISkinContextData *skinData = skin->GetContextInterface(node);

	// loop through all the vertices, writing out skinning data as we go
	int skinnedVertexCount = skinData->GetNumPoints();

	if (skinnedVertexCount > 0) {

		of << "\t\t\t<boneassignments>" << std::endl;

		int i;
		for(i=0; i<skinnedVertexCount; i++) {

			// grab the bone indices for this vertex
			int vertexBoneInfluences = skinData->GetNumAssignedBones(i);

			if (vertexBoneInfluences > 0) {

				int j;
				for (j=0; j < vertexBoneInfluences; j++) {

					// get weight per bone influence -- Ogre will ignore bones above
					// 4 and sum the weights to make it work, so leverage that feature
					int boneIdx = getBoneIndex(skin->GetBoneName(skinData->GetAssignedBone(i, j)));
					float vertexWeight = skinData->GetBoneWeight(i, j);

					of << "\t\t\t\t<vertexboneassignment vertexindex=\"" << i << "\" boneindex=\"" << boneIdx << "\" weight=\"" << vertexWeight << "\" />" << std::endl;
				}
			}
		}
	
		of << "\t\t\t</boneassignments>" << std::endl;
	}

	return true;
}

int OgreMaxExport::getBoneIndex(char *boneName) {

	std::map< std::string, int >::const_iterator it = m_boneIndexMap.find(std::string(boneName));
	if (it == m_boneIndexMap.end()) {
		m_boneIndexMap.insert(std::map< std::string, int >::value_type(std::string(boneName), m_currentBoneIndex));
		return m_currentBoneIndex++;
	}
	else
		return it->second;
}

// *******************************************************************************
// Skeleton streaming functions 
// *******************************************************************************

std::string OgreMaxExport::removeSpaces(const std::string &src) {
	std::string s(src);
	std::string::size_type pos;
	while ((pos=s.find_first_of(" \t\n")) != std::string::npos)
		s.replace(pos, 1, "_");

	return s;
}

bool OgreMaxExport::streamSkeleton(std::ostream &of) {

	// go through and sort out the bone hierarchy (include all of the non-null bones that were not 
	// skinned, as those could still be needed in the application)
	of << "<?xml version=\"1.0\"?>" << std::endl << "<skeleton>" << std::endl;
	of << "\t<bones>" << std::endl;

	// write out the bone rest pose data
	std::map< std::string, int >::const_iterator it = m_boneIndexMap.begin();
	while (it != m_boneIndexMap.end()) {

		INode *thisNode = m_i->GetINodeByName(it->first.c_str());

		of << "\t\t<bone id=\"" << it->second << "\" name=\"" << removeSpaces(it->first) << "\" >" << std::endl;

		// assume rest pose is at time zero
		TimeValue start = m_i->GetAnimRange().Start();
		ObjectState os = thisNode->EvalWorldState(start);
		Object *obj = os.obj;
		SClass_ID scid = obj->SuperClassID();

		// SKELOBJ_CLASS_ID = 0x9125 = 37157
		// BIPED_CLASS_ID = 0x9155 = 37205
		// BIPSLAVE_CONTROL_CLASS_ID = 0x9154 = 37204
		// BIPBODY_CONTROL_CLASS_ID = 0x9156 = 37206
		// FOOTPRINT_CLASS_ID = 0x3011 = 12305
		// DUMMY_CLASS_ID = 0x876234 = 8872500
		Matrix3 tm(thisNode->GetNodeTM(start));
		Matrix3 ptm(thisNode->GetParentTM(start));
		Control *tmc = thisNode->GetTMController();

		TCHAR *nm = thisNode->GetName();
		Class_ID cid = tmc->ClassID();

		if (cid == BIPBODY_CONTROL_CLASS_ID || cid == BIPED_CLASS_ID) {
			if (m_flipYZ) {
				Matrix3 m = RotateXMatrix(PI / 2.0f);
				tm = tm * Inverse(m);
			}
		}
		else
			tm = tm * Inverse(ptm);

		Point3 pos = tm.GetTrans();
		AngAxis aa(tm);

		of << "\t\t\t<position x=\"" << pos.x << "\" y=\"" << pos.y << "\" z=\"" << pos.z << "\" />" << std::endl;

		// there is still a lingering Max/Ogre handed-ness issue even after rotating to get the axes correct
		// so we negate the angle of rotation here
		of << "\t\t\t<rotation angle=\"" << -aa.angle << "\">" << std::endl;
		of << "\t\t\t\t<axis x=\"" << aa.axis.x << "\" y=\"" << aa.axis.y << "\" z=\"" << aa.axis.z << "\" />" << std::endl;
		of << "\t\t\t</rotation>" << std::endl;
		of << "\t\t</bone>" << std::endl;

		it++;
	}

	of << "\t</bones>" << std::endl;

	// write out the bone hierarchy
	it = m_boneIndexMap.begin();
	of << "\t<bonehierarchy>" << std::endl;
	while (it != m_boneIndexMap.end()) {
		INode *thisNode = m_i->GetINodeByName(it->first.c_str());

		if (thisNode != 0) {
			INode *parentNode = thisNode->GetParentNode();

			if (parentNode != 0 && parentNode != m_i->GetRootNode())
				of << "\t\t<boneparent bone=\"" << removeSpaces(it->first) << "\" parent=\"" << removeSpaces(std::string(parentNode->GetName())) << "\"/>" << std::endl;
		}

		it++;
	}
	of << "\t</bonehierarchy>" << std::endl;

	// the fun bits....
	// Animations are named by the user during export; Max has no concept of animation subset names, 
	// so we have to get the user to do that manually. If the user has entered anything for animations,
	// spit it all out here.
	std::list<NamedAnimation>::iterator anim = m_animations.begin();

	if (anim != m_animations.end()) {
		of << "\t<animations>" << std::endl;

		while (anim != m_animations.end()) {

			NamedAnimation a = *anim;
			anim++;

			float fps = (float)GetFrameRate();
			float length = (a.end - a.start) / fps;

			of << "\t\t<animation name=\"" << removeSpaces(a.name) << "\" length=\"" << length << "\">" << std::endl;

			streamAnimTracks(of, a.start, a.end);

			of << "\t\t</animation>" << std::endl;
		}

		of << "\t</animations>" << std::endl;
	}

	of << "</skeleton>" << std::endl;

	return true;
}

static int _compare_func(const void *a, const void *b) { return *(( int *)a) - *(( int *)b); }

bool OgreMaxExport::streamAnimTracks(std::ostream &of, int startFrame, int endFrame) {

	int start = startFrame * GetTicksPerFrame();
	int end = endFrame * GetTicksPerFrame();

	std::map< std::string, int >::const_iterator it = m_boneIndexMap.begin();

	of << "\t\t\t<tracks>" << std::endl;

	// need this for calculating keyframe values
	Matrix3 initTM, bipedMasterTM0;
	IBipMaster *bip = 0;
	bipedMasterTM0.IdentityMatrix();

	while (it != m_boneIndexMap.end()) {

		INode *thisNode = m_i->GetINodeByName(it->first.c_str());
		it++;

		Control *c = thisNode->GetTMController();
		Class_ID cid = c->ClassID(); 

		Tab<TimeValue> keyTimes;
		Interval interval(start, end);

		/*
		-- gets initial transform at frame 0f
		at time 0f (
			initTform = d.transform ;
			if (not isRootUniversal2 d) then (
				mparent = d.parent.transform ;
				initTform = initTform*inverse(mparent) ;
			)
			else if (flipYZ) then (
				if (not g_MAX) then
					format " - flipping root track..." ;
				-- we add the bip Transform
				--initTform = initTform * d.controller.rootNode.transform ;
				initTform = flipYZTransform initTform ;
			)
		)
		*/

		initTM = thisNode->GetNodeTM(0);

		// must have at least a frame at the start...
		keyTimes.Append(1, &start);

		TCHAR *tch = thisNode->GetName();

		// SKELOBJ_CLASS_ID = 0x9125 = 37157
		// BIPED_CLASS_ID = 0x9155 = 37205
		// BIPSLAVE_CONTROL_CLASS_ID = 0x9154 = 37204
		// BIPBODY_CONTROL_CLASS_ID = 0x9156 = 37206
		// FOOTPRINT_CLASS_ID = 0x3011 = 12305
		// DUMMY_CLASS_ID = 0x876234 = 8872500

		// three-part controller for Biped root -- taking this cue from the old MaxScript exporter code
		if (cid == BIPBODY_CONTROL_CLASS_ID) {

			// we deal with the initial transform as-is, except that it might need to
			// be rotated (since the root transform is in world coords)
			if (m_flipYZ)
				initTM = initTM * Inverse(RotateXMatrix(PI/2.0f));

			if (cid == BIPBODY_CONTROL_CLASS_ID) {
				// get the keys from the horiz, vert and turn controllers
				bip = GetBipMasterInterface(c);
				Control *biph = bip->GetHorizontalControl();
				Control *bipv = bip->GetVerticalControl();
				Control *bipr = bip->GetTurnControl();

				biph->GetKeyTimes(keyTimes, interval, KEYAT_POSITION | KEYAT_ROTATION);
				bipv->GetKeyTimes(keyTimes, interval, KEYAT_POSITION | KEYAT_ROTATION);
				bipr->GetKeyTimes(keyTimes, interval, KEYAT_POSITION | KEYAT_ROTATION);
			}
		}
		else if (cid == BIPSLAVE_CONTROL_CLASS_ID) {
			// slaves just have keys, apparently
			c->GetKeyTimes(keyTimes, interval, KEYAT_POSITION | KEYAT_ROTATION);
		
			// put initial transform into local coordinates -- since this is relative to the
			// parent, we don't need to sweat that possible rotations here
			initTM = initTM * Inverse(thisNode->GetParentTM(0));
		}

		// ...and stick a frame at the end as well...it will get sorted out if it is redundant
		keyTimes.Append(1, &end);

		// skip redundant key times here
		keyTimes.Sort(_compare_func);

//		if (cid == BIPSLAVE_CONTROL_CLASS_ID || cid == BIPBODY_CONTROL_CLASS_ID || cid == FOOTPRINT_CLASS_ID) {
//		
//			if (cid == BIPBODY_CONTROL_CLASS_ID) {
//				initTM = thisNode->GetNodeTM(0);
//
//				if (m_flipYZ)
//					initTM = initTM * RotateXMatrix(PI/2.0f);
//				bipedMasterTM0 = initTM;
//			}
//			else
//				initTM = bipedMasterTM0;
//
//			streamBipedKeyframes(of, bip, thisNode, keyTimes, interval, initTM);
//		}
//		else
			streamKeyframes(of, thisNode, keyTimes, interval, initTM);
	}

	of << "\t\t\t</tracks>" << std::endl;

	return true;
}

bool OgreMaxExport::streamKeyframes(std::ostream &of, INode *thisNode, Tab<TimeValue> &keyTimes, Interval &interval, Matrix3 &initTM) {

	of << "\t\t\t\t<track bone=\"" << removeSpaces(std::string(thisNode->GetName())) << "\">" << std::endl;
	of << "\t\t\t\t\t<keyframes>" << std::endl;

	int i;
	int keyTime = -1;
	int start = interval.Start();
	int end = interval.End();

	/*
	
	-- gets initial transform at frame 0f
	at time 0f (
		initTform = d.transform ;
		if (not isRootUniversal2 d) then (
			mparent = d.parent.transform ;
			initTform = initTform*inverse(mparent) ;
		)
		else if (flipYZ) then (
			if (not g_MAX) then
				format " - flipping root track..." ;
			-- we add the bip Transform
			--initTform = initTform * d.controller.rootNode.transform ;
			initTform = flipYZTransform initTform ;
		)
	)
	*/
	initTM = thisNode->GetNodeTM(0);

	Control *c = thisNode->GetTMController();
	Control *pc = thisNode->GetParentNode()->GetTMController();
	bool isRoot = false;

	if (c > 0)
		if (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)
			isRoot = true;
//	if (pc > 0)
//		if (pc->ClassID() == BIPBODY_CONTROL_CLASS_ID || pc->ClassID() == FOOTPRINT_CLASS_ID)
//			isRoot = true;

	TCHAR *tc = thisNode->GetName();
	if (!isRoot) {
		Matrix3 ptm = thisNode->GetParentTM(0);
		initTM = initTM * Inverse(ptm);
	}
	else if (m_flipYZ) {
		initTM = initTM * Inverse(RotateXMatrix(PI/2.0f));
	}

	for (i=0; i<keyTimes.Count(); i++) {
			
		// only operate within the supplied keyframe time range
		if (keyTimes[i] < start)
			continue;
		if (keyTimes[i] > end)
			break;

		// ignore key times we've already processed
		if (keyTimes[i] != keyTime) {

			keyTime = keyTimes[i];
			float keyTimef = (float) (keyTimes[i] - start) / (float)GetTicksPerFrame() / (float)GetFrameRate();

			of << "\t\t\t\t\t\t<keyframe time=\"" << keyTimef << "\">" << std::endl;

			/*

			function flipYZTransform Tform = (
				local axis1,axis2,axis3,t,m
				
				-- computes the matrix
				axis1 = point3 1 0 0 ;
				axis2 = point3 0 0 1 ;
				axis3 = point3 0 -1 0 ;
				t = point3 0 0 0 ;
				m=matrix3 axis1 axis2 axis3 t ;
				
				-- multiplies by the inverse
				Tform = Tform*inverse(m) ;

				return Tform ;
			)


			-- First, rotation which depends on initial transformation
			Tform = d.transform ;
			*/
			Matrix3 tm = thisNode->GetNodeTM(keyTime);

			/*
			-- if this is the pelvis
			if (isRootUniversal2 d) then (
				mparent = matrix3 1 ;

				if (flipYZ) then
					Tform = flipYZTransform Tform ;
			)			
			else
				mparent = d.parent.transform ; 
			*/

			// if this node's parent's controller is the biped controller, then this is either Pelvis or Footsteps,
			// and both should be treated as root nodes

			Matrix3 ident;
			Matrix3 ptm;
			ident.IdentityMatrix();
			Control *tmc = thisNode->GetTMController();
			TCHAR *tc = thisNode->GetName();

			if (tmc->ClassID() == BIPBODY_CONTROL_CLASS_ID) {

				ptm = ident;
				if (m_flipYZ) {
					tm = tm * Inverse(RotateXMatrix(PI/2.0f));
				}
			}
			else
				ptm = thisNode->GetParentNode()->GetNodeTM(keyTime);

			/*


			-- computes rotation
			mref = initTform*mparent ;	
			Tform = Tform*inverse(mref) ;
			*/

			Matrix3 mref = initTM * ptm;
			tm = tm * Inverse(mref);

			/*
			
			-- rotation part is saved.
			rot = toAngleAxis Tform.rotation ;
			axis = rot.axis;
			angle = - rot.angle;
			*/

			AngAxis aa(tm);

			/*
			-- Then, position which depends on parent			
			Tform=d.transform ;
			Tform=Tform*inverse(mparent) ;

			*/

			tm = thisNode->GetNodeTM(keyTime) * Inverse(ptm);

			/*

			-- if this is the root bone and flipYZ == true
			if (isRootUniversal2 d and flipYZ) then (
				Tform = flipYZTransform Tform ;
			)

			*/

			if (m_flipYZ && thisNode->GetParentNode()->GetParentTM(0).IsIdentity()) {
				tm = tm * Inverse(RotateXMatrix(PI/2.0f));
			}

			/*
			-- substracts position of the initial transform
			Tform.pos -= initTform.pos ;
			Tform.pos = Tform.pos * scale ;
			
			pos = Tform.pos ;
			*/
			Point3 trans = tm.GetTrans();
			trans -= initTM.GetTrans();

			of << "\t\t\t\t\t\t\t<translate x=\"" << trans.x << "\" y=\"" << trans.y << "\" z=\"" << trans.z << "\" />" << std::endl;
			of << "\t\t\t\t\t\t\t<rotate angle=\"" << -aa.angle << "\">" << std::endl;
			of << "\t\t\t\t\t\t\t\t<axis x=\"" << aa.axis.x << "\" y=\"" << aa.axis.y << "\" z=\"" << aa.axis.z << "\" />" << std::endl;
			of << "\t\t\t\t\t\t\t</rotate>" << std::endl;

			of << "\t\t\t\t\t\t</keyframe>" << std::endl;
		}
	}

	of << "\t\t\t\t\t</keyframes>" << std::endl;
	of << "\t\t\t\t</track>" << std::endl;

	return true;
}

bool OgreMaxExport::streamBipedKeyframes(std::ostream &of, IBipMaster *bip, INode *thisNode, Tab<TimeValue> &keyTimes, Interval &interval, Matrix3 &initTM) {

	of << "\t\t\t\t<track bone=\"" << removeSpaces(std::string(thisNode->GetName())) << "\">" << std::endl;
	of << "\t\t\t\t\t<keyframes>" << std::endl;

	int i;
	int keyTime = -1;
	int start = interval.Start();
	int end = interval.End();
	Matrix3 tm(thisNode->GetNodeTM(start));
	Matrix3 ptm(thisNode->GetParentTM(start));

	for (i=0; i<keyTimes.Count(); i++) {
			
		// only operate within the supplied keyframe time range
		if (keyTimes[i] < start)
			continue;
		if (keyTimes[i] > end)
			break;

		// ignore key times we've already processed
		if (keyTimes[i] != keyTime) {

			keyTime = keyTimes[i];
			float keyTimef = (float) (keyTimes[i] - start) / (float)GetTicksPerFrame() / (float)GetFrameRate();

			of << "\t\t\t\t\t\t<keyframe time=\"" << keyTimef << "\">" << std::endl;

			Control *tmc = thisNode->GetTMController();

			TCHAR *nm = thisNode->GetName();
			Class_ID cid = tmc->ClassID();

			if (cid == BIPBODY_CONTROL_CLASS_ID || cid == BIPED_CLASS_ID) {
				if (m_flipYZ) {
					Matrix3 m = RotateXMatrix(PI / 2.0f);
					tm = tm * Inverse(m);
				}
			}
			else
				tm = tm * Inverse(ptm);

			//Point3 trans = bip->GetBipedPos(keyTime, thisNode);
			//Quat q = bip->GetBipedRot(keyTime, thisNode);

			Point3 trans = tm.GetTrans();
			trans = trans * Inverse(initTM);
			trans -= initTM.GetTrans();

			//AngAxis aa(q);
			AngAxis aa(tm);
			float ang = aa.angle;
			Point3 axis = aa.axis;
        
			of << "\t\t\t\t\t\t\t<translate x=\"" << trans.x << "\" y=\"" << trans.y << "\" z=\"" << trans.z << "\" />" << std::endl;
			of << "\t\t\t\t\t\t\t<rotate angle=\"" << -ang << "\">" << std::endl;
			of << "\t\t\t\t\t\t\t\t<axis x=\"" << axis.x << "\" y=\"" << axis.y << "\" z=\"" << axis.z << "\" />" << std::endl;
			of << "\t\t\t\t\t\t\t</rotate>" << std::endl;

			of << "\t\t\t\t\t\t</keyframe>" << std::endl;
		}
	}

	of << "\t\t\t\t\t</keyframes>" << std::endl;
	of << "\t\t\t\t</track>" << std::endl;

	return true;
}
