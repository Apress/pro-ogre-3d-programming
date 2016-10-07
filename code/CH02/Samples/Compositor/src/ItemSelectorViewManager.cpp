/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 The OGRE Team
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/

#include "OgreStringConverter.h"
#include "ItemSelectorViewManager.h"

//-----------------------------------------------------------------------------------
// ItemSelectorViewManager Methods
//-----------------------------------------------------------------------------------
#define WIDGET_XPOS 0.0
#define WIDGET_YSTART 0.1f
#define WIDGET_YOFFSET 0.13f
#define ITEM_YSIZE 15
#define ITEM_YSPACING 2

    ItemSelectorViewManager::ItemSelectorViewManager(const Ogre::String& parentWindowName)
        : mItemSelectorController(0)
    {
        mParentWindow = CEGUI::WindowManager::getSingleton().getWindow(parentWindowName);
        // add a scrollable pane as a child to the parent
        mScrollablePane = (CEGUI::ScrollablePane*)CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/ScrollablePane",
                                    ("MainScrollPane"));
        mScrollablePane->setHorizontalAlignment(CEGUI::HA_CENTRE);
        mScrollablePane->setSize(CEGUI::Size(0.9, 0.75));
        mParentWindow->addChildWindow(mScrollablePane);
        mScrollablePane->setPosition(CEGUI::Point(WIDGET_XPOS, WIDGET_YSTART));
        // setup scrollable pane to resize to inside of parent window when parent resizes
        // scrollbars should only become visible when required
        // automatically handled by scrollable pane
    }
//-----------------------------------------------------------------------------------
    void ItemSelectorViewManager::addItemSelector(const Ogre::String& displayText)
    {
        // add a new item selector
        // determine new index for item
        assert(mScrollablePane);
        const size_t idx = mItemSelectorContainer.size();
        mItemSelectorContainer.push_back(ItemSelector());
        ItemSelector& item = mItemSelectorContainer.back();
        // create new checkbox
        CEGUI::Checkbox* checkbox = item.CheckBoxWidget = (CEGUI::Checkbox*)CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/Checkbox",
                                    ( ("ItemCheckbox" + Ogre::StringConverter::toString(idx)).c_str() ));
        // set checkbox ID to selector ID
        checkbox->setID(idx);
        checkbox->setMetricsMode(CEGUI::Absolute);
        checkbox->setSize(CEGUI::Size(140, ITEM_YSIZE));
        checkbox->setText(displayText.c_str());
        checkbox->setHoverTextColour(CEGUI::colour(1.0, 1.0, 0.0));
        // add event handler for when checkbox state changes
        checkbox->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged, CEGUI::Event::Subscriber(&ItemSelectorViewManager::handleCheckStateChanged, this ));
        checkbox->setPosition(CEGUI::Point(0, 12 + (ITEM_YSIZE + ITEM_YSPACING)* static_cast<float>(idx)));
        // add checkbox to the scroll pane
        mScrollablePane->addChildWindow(checkbox);
    }
//-----------------------------------------------------------------------------------
    void ItemSelectorViewManager::setItemSelectorController(ItemSelectorInterface* controller)
    {
        mItemSelectorController = controller;
    }
//-----------------------------------------------------------------------------------
    bool ItemSelectorViewManager::handleCheckStateChanged(const CEGUI::EventArgs& e)
    {
        // activate controller if set
        if (mItemSelectorController)
        {
            CEGUI::Checkbox* checkbox = static_cast<CEGUI::Checkbox*>(
                static_cast<const CEGUI::WindowEventArgs&>(e).window);
            mItemSelectorController->itemStateChanged(checkbox->getID(), checkbox->isSelected());
            float selectColour = checkbox->isSelected() ? 0.0f : 1.0f;
            checkbox->setNormalTextColour(CEGUI::colour(selectColour, 1.0f, selectColour));
        }
        return true;
    }
