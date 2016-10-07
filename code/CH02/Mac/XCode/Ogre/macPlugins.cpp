/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General  License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General  License for more details.

You should have received a copy of the GNU Lesser General  License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
-----------------------------------------------------------------------------
*/

#include <CoreFoundation/CoreFoundation.h>

#include "OgreString.h"
#include "macPlugins.h"

namespace Ogre {

    CFBundleRef mac_loadExeBundle(const char *name) {
        CFBundleRef baseBundle = CFBundleGetBundleWithIdentifier(CFSTR("org.ogre3d.Ogre"));
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFStringRef nameRef = CFStringCreateWithCString(NULL, name, kCFStringEncodingASCII);
        CFURLRef bundleURL = 0; //URL of bundle to load
        CFBundleRef bundle = 0; //bundle to load
        
        //cut off .bundle if present
        if(CFStringHasSuffix(nameRef, CFSTR(".bundle"))) {
            CFStringRef nameTempRef = nameRef;
            int end = CFStringGetLength(nameTempRef) - CFStringGetLength(CFSTR(".bundle"));
            nameRef = CFStringCreateWithSubstring(NULL, nameTempRef, CFRangeMake(0, end));
            CFRelease(nameTempRef);
        }
                
        //assume relative to Resources/ directory of Main bundle
        bundleURL = CFBundleCopyResourceURL(mainBundle, nameRef, CFSTR("bundle"), NULL);
        if(bundleURL) {
            bundle = CFBundleCreate(NULL, bundleURL);
            CFRelease(bundleURL);
        }
        
        //otherwise, try Resources/ directory of Ogre Framework bundle
        if(!bundle) {
            bundleURL = CFBundleCopyResourceURL(baseBundle, nameRef, CFSTR("bundle"), NULL);
            if(bundleURL) {
               bundle = CFBundleCreate(NULL, bundleURL);
               CFRelease(bundleURL);
            }
        }
        CFRelease(nameRef);
       
        if(bundle) {
            if(CFBundleLoadExecutable(bundle)) {
                return bundle;
            }
            else {
                CFRelease(bundle);
            }
        }
        
        return 0;
    }
    
    void *mac_getBundleSym(CFBundleRef bundle, const char *name) {
        CFStringRef nameRef = CFStringCreateWithCString(NULL, name, kCFStringEncodingASCII);
        void *sym = CFBundleGetFunctionPointerForName(bundle, nameRef);
        CFRelease(nameRef);
        return sym;
    }
    
    //returns 1 on error, 0 otherwise
    bool mac_unloadExeBundle(CFBundleRef bundle) {
        if(bundle) {
            //no-op, can't unload Obj-C bundles without crashing
            return 0;
        }
        return 1;
    }
    
    const char *mac_errorBundle() {
        return "Unknown Error";
    }

}