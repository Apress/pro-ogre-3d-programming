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
#include "OgreWin32ErrorDialog.h"
#include "resource.h"

namespace {
    Ogre::Win32ErrorDialog* dlg;  // This is a pointer to instance, since this is a static member
}

namespace Ogre
{
    Win32ErrorDialog::Win32ErrorDialog(HINSTANCE hInst)
    {
        mHInstance = hInst;
    }



#if OGRE_ARCHITECTURE_64 == OGRE_ARCH_TYPE
    INT_PTR CALLBACK Win32ErrorDialog::DlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
#else
    BOOL CALLBACK Win32ErrorDialog::DlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
#endif
    {
        HWND hwndDlgItem;

        switch (iMsg)
        {

        case WM_INITDIALOG:
            // Center myself
            int x, y, screenWidth, screenHeight;
            RECT rcDlg;
            GetWindowRect(hDlg, &rcDlg);
            screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
            screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);

            x = (screenWidth / 2) - ((rcDlg.right - rcDlg.left) / 2);
            y = (screenHeight / 2) - ((rcDlg.bottom - rcDlg.top) / 2);

            MoveWindow(hDlg, x, y, (rcDlg.right - rcDlg.left),
                (rcDlg.bottom - rcDlg.top), TRUE);

            // Fill in details of error
            hwndDlgItem = GetDlgItem(hDlg, IDC_ERRMSG);
            SetWindowText(hwndDlgItem, dlg->mErrorMsg.c_str());

            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
        }

        return FALSE;

    }


    void Win32ErrorDialog::display(const String& errorMessage, String logName)
    {
        // Display dialog
        // Don't return to caller until dialog dismissed
        dlg = this;
        mErrorMsg = errorMessage;
        DialogBox(mHInstance, MAKEINTRESOURCE(IDD_DLG_ERROR), NULL, DlgProc);


    }
}
