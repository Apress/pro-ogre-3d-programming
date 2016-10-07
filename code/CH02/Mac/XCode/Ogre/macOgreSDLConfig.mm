/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://ogre.sourceforge.net/

Copyright © 2000-2002 The OGRE Team
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


#import <Cocoa/Cocoa.h>
#import "macOgreConfigWindowController.h"
#include "macOgreSDLConfig.h"



using namespace Ogre;

bool SDLConfig::display(void)
{
    OgreConfigWindowController *config = [[OgreConfigWindowController alloc] 
                                            initWithWindowNibName: [ NSString stringWithCString: "config"]];
    
	// Get the renderer
    [config setMessage: [NSString stringWithCString: "Select Renderer:"]];
    
    
    RenderSystemList* renderers = Root::getSingleton().getAvailableRenderers();
    for (RenderSystemList::iterator pRend = renderers->begin();
            pRend != renderers->end(); pRend++)
    {
        [config addOption: [NSString stringWithCString: ((*pRend)->getName()).c_str()]];
    }

    int rendererIndex = [config pickOption];
    if(rendererIndex < 0) {
        return false;
    }


    RenderSystemList::iterator pRend =  renderers->begin();

    RenderSystem* renderer = pRend[rendererIndex];

    ConfigOptionMap options = renderer->getConfigOptions();

    // Process each option
    for (ConfigOptionMap::iterator it = options.begin(); 
            it != options.end(); it++)
    {
        [config setMessage: [NSString stringWithCString: it->second.name.c_str()]];
        
        StringVector::iterator opt_it;
        for (opt_it = it->second.possibleValues.begin();
                opt_it != it->second.possibleValues.end(); opt_it++)
        {
            [config addOption: [NSString stringWithCString: (*opt_it).c_str()]];
            if ((*opt_it) == it->second.currentValue) {
                [config setDefault];
            }
        }

        int x = [config pickOption];
        if(x < 0) // x < 0 flag that user cancelled
            return false;
        
        opt_it = it->second.possibleValues.begin();
        renderer->setConfigOption(it->second.name, opt_it[x]);
    }

    // All done
    Root::getSingleton().setRenderSystem(renderer);
    Root::getSingleton().saveConfig();    
    [config release];
    return true;
}


@implementation OgreConfigWindowController : NSWindowController 

-(void) setMessage: (NSString *)message {
    [self window];//ensure window is loaded
    [messageTextField setStringValue: message];
    [optionsPopUp removeAllItems]; 
}

-(void) addOption: (NSString *)title {
    [optionsPopUp addItemWithTitle: title];
}

-(void) setDefault {
    [optionsPopUp selectItem: [optionsPopUp lastItem]];
}

-(void) okClicked: (id)sender{
    [[NSApplication sharedApplication]stopModal];
    chosen = [optionsPopUp indexOfSelectedItem];
}

-(void) cancelClicked: (id)sender {
    [[NSApplication sharedApplication]stopModal];
    chosen = -1; //flag to abort
}

-(int) pickOption {
    [self showWindow: self];
    chosen = -1; //flag to abort
    //run loop
    
    [[NSApplication sharedApplication]runModalForWindow: [self window]];

    [[self window] close];
    return chosen;
}

@end