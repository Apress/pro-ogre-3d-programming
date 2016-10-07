#!BPY

"""
Name: 'Ogre XML'
Blender: 240
Group: 'Export'
Tooltip: 'Exports selected meshes with armature animations to Ogre3D'
"""

__author__ = ['Michael Reimpell', 'Jeff Doyle (nfz)', 'Jens Hoffmann', 'et al.']
__version__ = ''
__url__ = ['OGRE website, http://www.ogre3d.org',
    'OGRE forum, http://www.ogre3d.org/phpBB2/']
__bpydoc__ = """\
Exports selected meshes with armature animations to Ogre3D.

Read the script manual for further information.
"""

# Blender to Ogre Mesh and Skeleton Exporter
# url: http://www.ogre3d.org

# Ogre exporter written by Jens Hoffmann and Michael Reimpell
# based on the Cal3D exporter v0.5 written by Jean-Baptiste LAMY
# modified by Jeff Doyle (nfz) to work with Blender 2.4

# Copyright (C) 2004-2005 Michael Reimpell -- <M.Reimpell@tu-bs.de>
# Copyright (C) 2003 Jens Hoffmann -- <hoffmajs@gmx.de>
# Copyright (C) 2003 Jean-Baptiste LAMY -- jiba@tuxfamily.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# this export script is assumed to be used with the latest blender version.

# KEEP_SETTINGS (enable = 1, disable = 0)
#  transparently load and save settings to the text 'ogreexport.cfg'
#  inside the current .blend file.
KEEP_SETTINGS = 1

# OGRE_XML_CONVERTER
#  the command line used to run the OgreXMLConverter tool.
#  Set to '' to disable automatical conversion of XML files.
OGRE_XML_CONVERTER = ''

# OGRE_VERTEXCOLOUR_BGRA
#  workaround for Ogre's vertex colour conversion bug.
#  Set to 0 for RGBA, 1 for BGRA.
OGRE_OPENGL_VERTEXCOLOUR = 0

#######################################################################################
## Code starts here.

# epydoc doc format
__docformat__ = "javadoc en"

######
# imports
######
import Blender, sys, os, math, string
if KEEP_SETTINGS:
    try:
        import pickle
    except ImportError:
        Blender.Draw.PupMenu("Can't import pickle module!%t|Permanent settings disabled.")
        KEEP_SETTINGS = 0

######
# namespaces
######
from Blender import Draw
from Blender import Mathutils
from Blender.BGL import *

######
# Classes
######
class ReplacementScrollbar:
    """Scrollbar replacement for Draw.Scrollbar
       <ul>
       <li> import Blender
       <li> call eventFilter and buttonFilter in registered callbacks
       </ul>
       
       @author Michael Reimpell
    """
    def __init__(self, initialValue, minValue, maxValue, buttonUpEvent, buttonDownEvent):
        """Constructor   
           
           @param initialValue    inital value
           @param minValue        minimum value
           @param maxValue        maxium value
           @param buttonUpEvent   unique event number
           @param buttonDownEvent unique event number
        """
        self.currentValue = initialValue
        self.minValue = minValue
        if maxValue > minValue:
            self.maxValue = maxValue
        else:
            self.maxValue = self.minValue
            self.minValue = maxValue
        self.buttonUpEvent = buttonUpEvent
        self.buttonDownEvent = buttonDownEvent
        # private
        self.guiRect = [0,0,0,0]
        self.positionRect = [0,0,0,0]
        self.markerRect = [0,0,0,0]
        self.mousePressed = 0
        self.mouseFocusX = 0
        self.mouseFocusY = 0
        self.markerFocusY = 0
        self.mousePositionY = 0
        return
    
    def getCurrentValue(self):
        """current marker position
        """
        return self.currentValue
        
    def up(self, steps=1):
        """move scrollbar up
        """
        if (steps > 0):
            if ((self.currentValue - steps) > self.minValue):
                self.currentValue -= steps
            else:
                self.currentValue = self.minValue
        return
    
    def down(self, steps=1):
        """move scrollbar down
        """
        if (steps > 0):
            if ((self.currentValue + steps) < self.maxValue): 
                self.currentValue += steps
            else:
                self.currentValue = self.maxValue
        return
    
    def draw(self, x, y, width, height):
        """draw scrollbar
        """
        # get size of the GUI window to translate MOUSEX and MOUSEY events
        guiRectBuffer = Blender.BGL.Buffer(GL_FLOAT, 4)
        Blender.BGL.glGetFloatv(Blender.BGL.GL_SCISSOR_BOX, guiRectBuffer)
        self.guiRect = [int(guiRectBuffer.list[0]), int(guiRectBuffer.list[1]), \
                        int(guiRectBuffer.list[2]), int(guiRectBuffer.list[3])]
        # relative position
        self.positionRect = [ x, y, x + width, y + height]
        # check minimal size:
        # 2 square buttons,4 pixel borders and 1 pixel inside for inner and marker rectangles
        if ((height > (2*(width+5))) and (width > 2*5)):
            # keep track of remaining area
            remainRect = self.positionRect[:]
            # draw square buttons
            Blender.Draw.Button("/\\", self.buttonUpEvent, x, y + (height-width), width, width, "scroll up") 
            remainRect[3] -=  width + 2
            Blender.Draw.Button("\\/", self.buttonDownEvent, x, y, width, width, "scroll down") 
            remainRect[1] +=  width + 1
            # draw inner rectangle
            Blender.BGL.glColor3f(0.13,0.13,0.13) # dark grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
            remainRect[0] += 1
            remainRect[3] -= 1
            Blender.BGL.glColor3f(0.78,0.78,0.78) # light grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
            remainRect[1] += 1
            remainRect[2] -= 1
            Blender.BGL.glColor3f(0.48,0.48,0.48) # grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
            # draw marker rectangle
            # calculate marker rectangle
            innerHeight = remainRect[3]-remainRect[1]
            markerHeight = innerHeight/(self.maxValue-self.minValue+1.0)
            # markerRect 
            self.markerRect[0] = remainRect[0]
            self.markerRect[1] = remainRect[1] + (self.maxValue - self.currentValue)*markerHeight
            self.markerRect[2] = remainRect[2]
            self.markerRect[3] = self.markerRect[1] + markerHeight
            # clip markerRect to innerRect (catch all missed by one errors)
            if self.markerRect[1] > remainRect[3]:
                self.markerRect[1] = remainRect[3]
            if self.markerRect[3] > remainRect[3]:
                self.markerRect[3] = remainRect[3]
            # draw markerRect
            remainRect = self.markerRect
            Blender.BGL.glColor3f(0.78,0.78,0.78) # light grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
            remainRect[0] += 1
            remainRect[3] -= 1
            Blender.BGL.glColor3f(0.13,0.13,0.13) # dark grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
            remainRect[1] += 1
            remainRect[2] -= 1
            # check if marker has foucs
            if (self.mouseFocusX and self.mouseFocusY and (self.mousePositionY > self.markerRect[1]) and (self.mousePositionY < self.markerRect[3])):
                Blender.BGL.glColor3f(0.64,0.64,0.64) # marker focus grey
            else:
                Blender.BGL.glColor3f(0.60,0.60,0.60) # marker grey
            Blender.BGL.glRectf(remainRect[0], remainRect[1], remainRect[2], remainRect[3])
        else:
            print "scrollbar draw size too small!"
        return
        
    def eventFilter(self, event, value):
        """event filter for keyboard and mouse input events
           call it inside the registered event function
        """
        if (value != 0):
            # Buttons
            if (event == Blender.Draw.PAGEUPKEY):
                self.up(3)
                Blender.Draw.Redraw(1)
            elif (event == Blender.Draw.PAGEDOWNKEY):
                self.down(3)
                Blender.Draw.Redraw(1)
            elif (event == Blender.Draw.UPARROWKEY):
                self.up(1)
                Blender.Draw.Redraw(1)
            elif (event == Blender.Draw.DOWNARROWKEY):
                self.down(1)
                Blender.Draw.Redraw(1)
            # Mouse
            elif (event == Blender.Draw.MOUSEX):
                # check if mouse is inside positionRect
                if (value >= (self.guiRect[0] + self.positionRect[0])) and (value <= (self.guiRect[0] + self.positionRect[2])):
                    # redraw if marker got focus
                    if (not self.mouseFocusX) and self.mouseFocusY:
                        Blender.Draw.Redraw(1)
                    self.mouseFocusX = 1
                else:
                    # redraw if marker lost focus
                    if self.mouseFocusX and self.mouseFocusY:
                        Blender.Draw.Redraw(1)
                    self.mouseFocusX = 0
            elif (event == Blender.Draw.MOUSEY):
                # check if mouse is inside positionRect
                if (value >= (self.guiRect[1] + self.positionRect[1])) and (value <= (self.guiRect[1] + self.positionRect[3])):
                    self.mouseFocusY = 1
                    # relative mouse position
                    self.mousePositionY = value - self.guiRect[1]
                    if ((self.mousePositionY > self.markerRect[1]) and (self.mousePositionY < self.markerRect[3])):
                        # redraw if marker got focus
                        if self.mouseFocusX and (not self.markerFocusY):
                            Blender.Draw.Redraw(1)
                        self.markerFocusY = 1
                    else:
                        # redraw if marker lost focus
                        if self.mouseFocusX and self.markerFocusY:
                            Blender.Draw.Redraw(1)
                        self.markerFocusY = 0
                    # move marker
                    if (self.mousePressed == 1):
                        # calculate step from distance to marker
                        if (self.mousePositionY > self.markerRect[3]):
                            # up
                            self.up(1)
                            Blender.Draw.Draw()
                        elif (self.mousePositionY < self.markerRect[1]):
                            # down
                            self.down(1)
                            Blender.Draw.Draw()
                        # redraw if marker lost focus
                        if self.mouseFocusX and self.mouseFocusY:
                            Blender.Draw.Redraw(1)
                else:
                    # redraw if marker lost focus
                    if self.mouseFocusX and self.markerFocusY:
                        Blender.Draw.Redraw(1)
                    self.markerFocusY = 0
                    self.mouseFocusY = 0
            elif ((event == Blender.Draw.LEFTMOUSE) and (self.mouseFocusX == 1) and (self.mouseFocusY == 1)):
                self.mousePressed = 1
                # move marker
                if (self.mousePositionY > self.markerRect[3]):
                    # up
                    self.up(1)
                    Blender.Draw.Redraw(1)
                elif (self.mousePositionY < self.markerRect[1]):
                    # down
                    self.down(1)
                    Blender.Draw.Redraw(1)
            elif (Blender.Get("version") >= 234):
                if (event == Blender.Draw.WHEELUPMOUSE):
                    self.up(1)
                    Blender.Draw.Redraw(1)
                elif (event == Blender.Draw.WHEELDOWNMOUSE):
                    self.down(1)
                    Blender.Draw.Redraw(1)
        else: # released keys and buttons
            if (event == Blender.Draw.LEFTMOUSE):
                self.mousePressed = 0
                
        return
        
    def buttonFilter(self, event):
        """button filter for Draw Button events
           call it inside the registered button function
        """
        if (event  == self.buttonUpEvent):
            self.up()
            Blender.Draw.Redraw(1)
        elif (event == self.buttonDownEvent):
            self.down()
            Blender.Draw.Redraw(1)
        return

class ArmatureAction:
    """Resembles Blender's actions
       <ul>
       <li> import Blender, string
       </ul>
       
       @author Michael Reimpell
    """
    def __init__(self, name="", ipoDict=None):
        """Constructor
        
           @param name    the action name
           @param ipoDict a dictionary with bone names as keys and action Ipos as values
        """
        self.firstKeyFrame = None
        self.lastKeyFrame = None
        self.name = name
        # ipoDict[boneName] = Blender.Ipo
        if ipoDict is None:
            self.ipoDict = {}
        else:
            self.ipoDict = ipoDict
            self._updateKeyFrameRange()
        return
    
    # private method    
    def _updateKeyFrameRange(self):
        """Updates firstKeyFrame and lastKeyFrame considering the current IpoCurves.
        """
        self.firstKeyFrame = None
        self.lastKeyFrame = None
        if self.ipoDict is not None:
            # check all bone Ipos
            for ipo in self.ipoDict.values():
                # check all IpoCurves
                for ipoCurve in ipo.getCurves():
                    # check first and last keyframe
                    for bezTriple in ipoCurve.getPoints():
                        iFrame = bezTriple.getPoints()[0]
                        if ((iFrame < self.firstKeyFrame) or (self.firstKeyFrame is None)):
                            self.firstKeyFrame = iFrame
                        if ((iFrame > self.lastKeyFrame) or (self.lastKeyFrame is None)):
                            self.lastKeyFrame = iFrame
        if self.firstKeyFrame == None:
            self.firstKeyFrame = 1
        if self.lastKeyFrame == None:
            self.lastKeyFrame = 1
        return
    
    # static method
    def createArmatureActionDict(object):
        """Creates a dictionary of possible actions belonging to an armature.
           Static call with: ArmatureAction.createArmatureActionDict(object)
           
           @param object a Blender.Object of type Armature
           @return a dictionary of ArmatureAction objects with name as key and ArmatureAction as value
        """
        # create bone dict
        #boneQueue = object.getData().bones
        boneDict = object.getData().bones

        boneNameList = boneDict.keys()
        # check for available actions
        armatureActionDict = {}
        ## get linked action first
        linkedAction = object.getAction()
        if linkedAction is not None:
            # check all bones
            linkedActionIpoDict = linkedAction.getAllChannelIpos()
            hasValidChannel = 0 # false
            iBone = 0
            while ((not hasValidChannel) and (iBone < len(boneNameList))):
                if (linkedActionIpoDict.keys().count(boneNameList[iBone]) == 1):
                    if linkedActionIpoDict[boneNameList[iBone]].getNcurves():
                        hasValidChannel = 1 # true
                    else:
                        iBone += 1
                else:
                    iBone += 1
            if hasValidChannel:
                # add action
                armatureActionDict[linkedAction.getName()] = ArmatureAction(linkedAction.getName(), linkedActionIpoDict)
        ## get other actions != linked action
        actionDict = Blender.Armature.NLA.GetActions()
        for actionName in actionDict.keys():
            # check if action is not linked action
            if actionDict[actionName] is not linkedAction:
                # check all bones
                actionIpoDict = actionDict[actionName].getAllChannelIpos()
                hasValidChannel = 0 # false
                iBone = 0
                while ((not hasValidChannel) and (iBone < len(boneNameList))):
                    if (actionIpoDict.keys().count(boneNameList[iBone]) == 1):
                        if actionIpoDict[boneNameList[iBone]].getNcurves():
                            hasValidChannel = 1 # true
                        else:
                            iBone += 1
                    else:
                        iBone += 1
                if hasValidChannel:
                    
                    # add action
                    armatureActionDict[actionName] = ArmatureAction(actionName, actionIpoDict)
        return armatureActionDict
    createArmatureActionDict = staticmethod(createArmatureActionDict)

class ArmatureActionActuator:
    """Resembles Blender's action actuators.
    
       @author Michael Reimpell
    """
    def __init__(self, name, startFrame, endFrame, armatureAction):
        """Constructor
           
           @param name           Animation name
           @param startFrame     first frame of the animation
           @param endFrame       last frame of the animation
           @param armatureAction ArmatureAction object of the animation
        """
        self.name = name
        self.startFrame = startFrame
        self.endFrame = endFrame
        self.armatureAction = armatureAction
        return

class ArmatureActionActuatorListView:
    """Mangages a list of ArmatureActionActuators.
       <ul>
       <li> import Blender
       <li> call eventFilter and buttonFilter in registered callbacks
       </ul>
       
       @author Michael Reimpell
    """
    def __init__(self, armatureActionDict, maxActuators, buttonEventRangeStart, armatureAnimationDictList=None):
        """Constructor.
           
           @param armatureActionDict        possible armature actuator actions
           @param maxActuators              maximal number of actuator list elements
           @param buttonEventRangeStart     first button event number.
                                            The number of used event numbers is (3 + maxActuators*5)
           @param armatureAnimationDictList list of armature animations (see getArmatureAnimationDictList())
        """
        self.armatureActionDict = armatureActionDict
        self.maxActuators = maxActuators
        self.buttonEventRangeStart = buttonEventRangeStart
        self.armatureActionActuatorList = []
        self.armatureActionMenuList = []
        self.startFrameNumberButtonList = []
        self.endFrameNumberButtonList = []
        self.animationNameStringButtonList = []
        # scrollbar values:
        #   0:(len(self.armatureActionActuatorList)-1) = listIndex
        #   len(self.armatureActionActuatorList) = addbuttonline
        self.scrollbar = ReplacementScrollbar(0,0,0, self.buttonEventRangeStart+1, self.buttonEventRangeStart+2)
        if armatureAnimationDictList is not None:
            # rebuild ArmatureActionActuators for animationList animations
            for animationDict in armatureAnimationDictList:
                # check if Action is available
                if self.armatureActionDict.has_key(animationDict['actionKey']):
                    armatureActionActuator = ArmatureActionActuator(animationDict['name'], \
                                                                    animationDict['startFrame'], \
                                                                    animationDict['endFrame'], \
                                                                    self.armatureActionDict[animationDict['actionKey']])
                    self._addArmatureActionActuator(armatureActionActuator)
        else:
            # create default ArmatureActionActuators
            for armatureAction in self.armatureActionDict.values():
                # add default action
                armatureActionActuator = ArmatureActionActuator(armatureAction.name, armatureAction.firstKeyFrame, armatureAction.lastKeyFrame, armatureAction)
                self._addArmatureActionActuator(armatureActionActuator)
        return
        
    def refresh(self, armatureActionDict):
        """Delete ArmatureActionActuators for removed Actions.
            
           @param armatureActionDict possible ArmatureActuator actions
        """
        self.armatureActionDict = armatureActionDict
        # delete ArmatureActionActuators for removed Actions
        for armatureActionActuator in self.armatureActionActuatorList[:]:
            # check if action is still available
            if not self.armatureActionDict.has_key(armatureActionActuator.armatureAction.name):
                # remove armatureActionActuator from lists
                listIndex = self.armatureActionActuatorList.index(armatureActionActuator)
                self._deleteArmatureActionActuator(listIndex)
        Blender.Draw.Redraw(1)
        return
        
    def draw(self, x, y, width, height):
        """draw actuatorList
           use scrollbar if needed
        """
        # black border
        minX = x
        minY = y
        maxX = x + width
        maxY = y + height
        minWidth = 441
        if ((width - 5) > minWidth):
            glColor3f(0.0,0.0,0.0)
            glRectf(minX, minY, maxX - 22, maxY)
            glColor3f(0.6,0.6,0.6) # Background: grey
            glRectf(minX + 1, minY + 1, maxX - 23, maxY - 1)
            x += 3
            y += 3
            width -= 5
            height -= 6
        else:
            print "ArmatureActionActuatorListView draw size to small!"
            glColor3f(0.0,0.0,0.0)
            glRectf(minX, minY, maxX, maxY)
            glColor3f(0.6,0.6,0.6) # Background: grey
            glRectf(minX + 1, minY + 1, maxX, maxY - 1)
            x += 3
            y += 3
            width -= 5
            height -= 6
        # Layout:
        # |---- 105 ---|2|----80---|2|---80---|2|---- >80 ----|2|---60---|2|----20---|
        # actionName   | startFrame | endFrame | animationName | [delete] | scrollbar
        # [ add ]                                                         | scrollbar
        if (len(self.armatureActionDict.keys()) > 0):
            # construct actionMenu name
            menuValue = 0
            menuName = ""
            for key in self.armatureActionDict.keys():
                menuName += key + " %x" + ("%d" % menuValue) + "|"
                menuValue +=1
            # first line
            lineY = y + height - 20
            lineX = x
            listIndex = self.scrollbar.getCurrentValue()
            while ((listIndex < len(self.armatureActionActuatorList)) and (lineY >= y)):
                # still armatureActionActuators left to draw
                lineX = x
                armatureActionActuator = self.armatureActionActuatorList[listIndex]
                # draw actionMenu
                event = self.buttonEventRangeStart + 3 + listIndex
                menuValue = self.armatureActionDict.keys().index(armatureActionActuator.armatureAction.name)
                self.armatureActionMenuList[listIndex] = Blender.Draw.Menu(menuName,event, x, lineY, 105, 20, menuValue, "Action name")
                lineX += 107
                # draw startFrameNumberButton
                event = self.buttonEventRangeStart + 3 + self.maxActuators + listIndex
                self.startFrameNumberButtonList[listIndex] = Blender.Draw.Number("Sta: ", event, lineX, lineY, 80, 20, \
                                                         armatureActionActuator.startFrame, -18000, 18000, "Start frame")
                lineX += 82
                # draw endFrameNumberButton
                event = self.buttonEventRangeStart + 3 + 2*self.maxActuators + listIndex
                self.endFrameNumberButtonList[listIndex] = Blender.Draw.Number("End: ", event, lineX, lineY, 80, 20, \
                                                         armatureActionActuator.endFrame, -18000, 18000, "End frame")
                lineX += 82
                # compute animationNameWidht
                animationNameWidth = width - 271 - 85
                if (animationNameWidth < 80):
                    animationNameWidth = 80
                # draw animationNameStringButton
                event = self.buttonEventRangeStart + 3 + 3*self.maxActuators + listIndex
                self.animationNameStringButtonList[listIndex] = Blender.Draw.String("",event, lineX, lineY, animationNameWidth, 20, \
                                                                armatureActionActuator.name, 1000, "Animation export name") 
                lineX += animationNameWidth + 2
                # draw deleteButton
                event = self.buttonEventRangeStart + 3 + 4*self.maxActuators + listIndex
                Draw.Button("Delete", event, lineX, lineY, 60, 20, "Delete export animation")
                lineX += 62
                # inc line
                lineY -= 22
                listIndex += 1
            # draw add button
            if (lineY >= y):
                Draw.Button("Add", self.buttonEventRangeStart, x, lineY, 60, 20, "Add new export animation")
        # draw scrollbar
        if (width > minWidth):
            # align left
            self.scrollbar.draw(maxX - 20, minY, 20, (maxY - minY))
        return
    
    def eventFilter(self, event, value):
        """event filter for keyboard and mouse input events
           call it inside the registered event function
        """
        self.scrollbar.eventFilter(event,value)
        return
        
    def buttonFilter(self, event):
        """button filter for Draw Button events
           call it inside the registered button function
        """
        # button numbers = self.buttonEventRangeStart + buttonNumberOffset
        # buttonNumberOffsets:
        # addButton: 0 
        # scrollbar: 1 and 2
        # actionMenu range: 3 <= event < 3 + maxActuators
        # startFrameNumberButton range:  3 + maxActuators <= event < 3 + 2*maxActuators
        # endFrameNumberButton range: 3 + 2*maxActuators <= event < 3 + 3*maxActuators
        # animationNameStringButton range: 3 + 3*maxActuators <= event < 3 + 4*maxActuators
        # deleteButton range: 3 + 4*maxActuators <= event < 3 + 5*maxActuators
        self.scrollbar.buttonFilter(event)
        relativeEvent = event - self.buttonEventRangeStart
        if (relativeEvent == 0):
            # add button pressed
            if (len(self.armatureActionDict.keys()) > 0):
                # add default ArmatureActionActuator
                armatureAction = self.armatureActionDict[self.armatureActionDict.keys()[0]]
                armatureActionActuator = ArmatureActionActuator(armatureAction.name, armatureAction.firstKeyFrame, armatureAction.lastKeyFrame, armatureAction)
                self._addArmatureActionActuator(armatureActionActuator)
                Blender.Draw.Redraw(1)
        elif ((3 <= relativeEvent) and (relativeEvent < (3 + self.maxActuators))):
            # actionMenu
            listIndex = relativeEvent - 3
            armatureActionActuator = self.armatureActionActuatorList[listIndex]
            # button value is self.actionDict.keys().index
            keyIndex = self.armatureActionMenuList[listIndex].val
            key = self.armatureActionDict.keys()[keyIndex]
            armatureActionActuator.armatureAction = self.armatureActionDict[key]
            armatureActionActuator.startFrame = self.armatureActionDict[key].firstKeyFrame
            armatureActionActuator.endFrame = self.armatureActionDict[key].lastKeyFrame
            armatureActionActuator.name = self.armatureActionDict[key].name
            self.armatureActionActuatorList[listIndex] = armatureActionActuator
            Blender.Draw.Redraw(1)
        elif (((3 + self.maxActuators) <= relativeEvent) and (relativeEvent < (3 + 2*self.maxActuators))):
            # startFrameNumberButton
            listIndex = relativeEvent - (3 + self.maxActuators)
            armatureActionActuator = self.armatureActionActuatorList[listIndex]
            armatureActionActuator.startFrame = self.startFrameNumberButtonList[listIndex].val
            self.armatureActionActuatorList[listIndex] = armatureActionActuator
        elif (((3 + 2*self.maxActuators) <= relativeEvent) and (relativeEvent < (3 + 3*self.maxActuators))):
            # endFrameNumberButton
            listIndex = relativeEvent - (3 + 2*self.maxActuators)
            armatureActionActuator = self.armatureActionActuatorList[listIndex]
            armatureActionActuator.endFrame = self.endFrameNumberButtonList[listIndex].val
            self.armatureActionActuatorList[listIndex] = armatureActionActuator
        elif (((3 + 3*self.maxActuators) <= relativeEvent) and (relativeEvent < (3 + 4*self.maxActuators))):
            # animationNameStringButton
            listIndex = relativeEvent - (3 + 3*self.maxActuators)
            armatureActionActuator = self.armatureActionActuatorList[listIndex]
            armatureActionActuator.name = self.animationNameStringButtonList[listIndex].val
            self.armatureActionActuatorList[listIndex] = armatureActionActuator
        elif (((3 + 4*self.maxActuators) <= relativeEvent) and (relativeEvent < (3 + 5*self.maxActuators))):
            # deleteButton
            listIndex = relativeEvent - (3 + 4*self.maxActuators)
            self._deleteArmatureActionActuator(listIndex)
            Blender.Draw.Redraw(1)
        return
        
    def getArmatureAnimationDictList(self):
        """serialize the armatureActionActuatorList into a pickle storable list
           Each item of the returned list is a dictionary with key-value pairs:
           <ul>
           <li>    name - ArmatureActionActuator.name
           <li>    startFrame - ArmatureActionActuator.startFrame
           <li>    endFrame - ArmatureActionActuator.endFrame
           <li>    armatureActionKey - ArmatureActionActuator.armatureAction.name
           </ul>
           
           @return serialized actionActuatorList
        """
        animationDictList = []
        for armatureActionActuator in self.armatureActionActuatorList:
            # create animationDict
            animationDict = {}
            animationDict['name'] = armatureActionActuator.name
            animationDict['startFrame'] = armatureActionActuator.startFrame
            animationDict['endFrame'] = armatureActionActuator.endFrame
            animationDict['actionKey'] = armatureActionActuator.armatureAction.name
            animationDictList.append(animationDict)
        return animationDictList
        
    def setAnimationDictList(self, animationDictList):
        """loads old AnimationDictList with actionKey = (ipoPrefix, ipoPostfix)
           
           @see #getArmatureAnimationDictList()
        """
        # rebuild ArmatureActionActuators for animationList animations
        for animationDict in animationDictList:
            # check if Action is available
            prefix, postfix = animationDict['actionKey']
            armatureActionName = prefix+postfix
            if self.armatureActionDict.has_key(armatureActionName):
                armatureActionActuator = ArmatureActionActuator(animationDict['name'], \
                                                                animationDict['startFrame'], \
                                                                animationDict['endFrame'], \
                                                                self.armatureActionDict[armatureActionName])
                self._addArmatureActionActuator(armatureActionActuator)
        return
        
    # private methods
    def _addArmatureActionActuator(self, armatureActionActuator):
        """adds an ArmatureActionActuator to the list
           <ul>
           <li> call Blender.Draw.Redraw(1) afterwards
           </ul>
        """
        if (len(self.armatureActionActuatorList) < self.maxActuators):
            # check if armatureActionActuator.action is available
            if armatureActionActuator.armatureAction.name in self.armatureActionDict.keys():
                # create armatureActionMenu
                # get ArmatureAction index in armatureActionDict.keys() list
                armatureActionMenu = Draw.Create(self.armatureActionDict.keys().index(armatureActionActuator.armatureAction.name))
                self.armatureActionMenuList.append(armatureActionMenu)
                # create startFrameNumberButton
                startFrameNumberButton = Draw.Create(int(armatureActionActuator.startFrame))
                self.startFrameNumberButtonList.append(startFrameNumberButton)
                # create endFrameNumberButton
                endFrameNumberButton = Draw.Create(int(armatureActionActuator.endFrame))
                self.endFrameNumberButtonList.append(endFrameNumberButton)
                # create animationNameStringButton
                animationNameStringButton = Draw.Create(armatureActionActuator.name)
                self.animationNameStringButtonList.append(animationNameStringButton)
                # append to armatureActionActuatorList
                self.armatureActionActuatorList.append(armatureActionActuator)
                # adjust scrollbar
                scrollbarPosition = self.scrollbar.getCurrentValue()
                self.scrollbar = ReplacementScrollbar(scrollbarPosition,0,len(self.armatureActionActuatorList), self.buttonEventRangeStart+1, self.buttonEventRangeStart+2)
                # TODO: change scrollbarPosition in a way, such that the new actuator is visible
            else:
                print "Error: Could not add ArmatureActionActuator because ArmatureAction is not available!"
        return
        
    def _deleteArmatureActionActuator(self, listIndex):
        """removes an ArmatureActionActuator from the list
           <ul>
           <li> call Blender.Draw.Redraw(1) afterwards
           </ul>
        """
        # check listIndex
        if ((len(self.armatureActionActuatorList) > 0) and (listIndex >= 0) and (listIndex < len(self.armatureActionActuatorList))):
            # remove armatureActionMenu
            self.armatureActionMenuList.pop(listIndex)
            # remove startFrameNumberButton
            self.startFrameNumberButtonList.pop(listIndex)
            # remove endFrameNumberButton
            self.endFrameNumberButtonList.pop(listIndex)
            # remove animationNameStringButton
            self.animationNameStringButtonList.pop(listIndex)
            # remove armatureActionActuator
            self.armatureActionActuatorList.pop(listIndex)
            # adjust scrollbar
            scrollbarPosition = self.scrollbar.getCurrentValue()
            if (scrollbarPosition > len(self.armatureActionActuatorList)):
                scrollbarPosition = len(self.armatureActionActuatorList)
            self.scrollbar = ReplacementScrollbar(scrollbarPosition,0,len(self.armatureActionActuatorList), self.buttonEventRangeStart+1, self.buttonEventRangeStart+2)
            return

class Logger:
    """Logs messages and status.
    
       Logs messages as a list of strings and keeps track of the status.
       Possible status values are info, warning and error.
       
       @cvar INFO info status
       @cvar WARNING warning status
       @cvar ERROR error status
    """
    INFO, WARNING, ERROR = range(3)
    def __init__(self):
        """Constructor.
        """
        self.messageList = []
        self.status = Logger.INFO
        return
    def logInfo(self, message):
        """Logs an info message.
        
           @param message message string
        """
        self.messageList.append((Logger.INFO, message))
        return        
    def logWarning(self, message):
        """Logs a warning message.
        
           The status is set to <code>Logger.WARNING</code> if it is not already <code>Logger.ERROR</code>.
           
           @param message message string
        """
        self.messageList.append((Logger.WARNING, "Warning: "+message))
        if not self.status == Logger.ERROR:
            self.status = Logger.WARNING
        return
    def logError(self, message):
        """Logs an error message.
        
           The status is set to <code>Logger.ERROR</code>.
           
           @param message message string
        """
        self.messageList.append((Logger.ERROR, "Error: "+message))
        self.status = Logger.ERROR
        return
    def getStatus(self):
        """Gets the current status.
        
           The status can be
           <ul>
           <li><code>Logger.INFO</code>
           <li><code>Logger.WARNING</code>
           <li><code>Logger.ERROR</code>
           </ul>
           
           @return status
        """
        return self.status
    def getMessageList(self):
        """Returns the list of log messages.
        
           @return list of tuples (status, message)
        """
        return self.messageList

class LogInterface:
    def __init__(self):
        self.loggerList = []
    def addLogger(self, logger):
        self.loggerList.append(logger)
        return
    def removeLogger(self, logger):
        self.loggerList.remove(logger)
        return
    # protected
    def _logInfo(self, message):
        for logger in self.loggerList:
            logger.logInfo(message)
        return
    def _logWarning(self, message):
        for logger in self.loggerList:
            logger.logWarning(message)
        return
    def _logError(self, message):
        for logger in self.loggerList:
            logger.logWarning(message)
        return

class PathName(LogInterface):
    """Splits a pathname independent of the underlying os.
    
       Blender saves pathnames in the os specific manner. Using os.path may result in problems
       when the export is done on a different os than the creation of the .blend file.       
    """
    def __init__(self, pathName):
        self.pathName = pathName
        LogInterface.__init__(self)
        return
    def dirname(self):
        return os.path.dirname(self.pathName) 
    def basename(self):
        baseName = os.path.basename(self.pathName)
        # split from non-os directories
        # \\
        baseName = baseName.split('\\').pop()
        # /
        baseName = baseName.split('/').pop()
        if (baseName != baseName.replace(' ','_')):
            # replace whitespace with underscore
            self._logWarning("Whitespaces in filename \"%s\" replaced with underscores." % baseName)
            baseName = baseName.replace(' ','_')
        return baseName
    def path(self):
        return self.pathName

class ExportOptions:
    """Encapsulates export options common to all objects.
    """
    # TODO: Model for GUI
    def __init__(self, rotXAngle, rotYAngle, rotZAngle, scale, useWorldCoordinates, colouredAmbient, exportPath, materialFilename):
        """Constructor.
        """
        # floating point accuracy
        self.accuracy = 1e-6
        # export transformation
        self.rotXAngle = rotXAngle
        self.rotYAngle = rotYAngle
        self.rotZAngle = rotZAngle
        self.scale = scale
        self.useWorldCoordinates = useWorldCoordinates
        self.colouredAmbient = colouredAmbient
        # file settings
        self.exportPath = exportPath
        self.materialFilename = materialFilename
        return
    
    def transformationMatrix(self):
        """Returns the matrix representation for the additional transformation on export.
        """
        rotationMatrix = Mathutils.RotationMatrix(self.rotXAngle,4,'x')
        rotationMatrix *= Mathutils.RotationMatrix(self.rotYAngle,4,'y')
        rotationMatrix *= Mathutils.RotationMatrix(self.rotZAngle,4,'z')
        scaleMatrix = Mathutils.Matrix([self.scale,0,0],[0,self.scale,0],[0,0,self.scale])
        scaleMatrix.resize4x4()
        return rotationMatrix*scaleMatrix

class ObjectExporter:
    """Interface. Exports a Blender object to Ogre.
    """
    def __init__(self, object):
        """Constructor.
           
           @param object Blender object to export.
        """
        self.object = object
        return
    
    def getName(self):
        """Returns the name of the object.
        """
        return self.object.getName()
    
    def getObjectMatrix(self):
        """Returns the object matrix in worldspace.
        """
        return self.object.matrixWorld
    
    
class MeshExporter(ObjectExporter):
    """
    """
    def getName(self):
        return self.object.getData().name
        
class ArmatureExporter:
    """Exports an armature of a mesh.
    """
    # TODO: Provide bone ids for vertex influences.
    def __init__(self, meshObject, armatureObject):
        """Constructor.
        
          @param meshObject ObjectExporter.
          @param armatureObject Blender armature object.
        """
        self.meshObject = meshObject
        self.armatureObject = armatureObject
        self.skeleton = None
        return
    
    def export(self, actionActuatorList, exportOptions, logger):
        """Exports the armature.
        
           @param actionActuatorList list of animations to export.
           @param exportOptions global export options.
           @param logger Logger Logger for log messages.            
        """
        # convert Armature into Skeleton
        name = None
        if exportOptions.useWorldCoordinates:
            name = self.armatureObject.getData().name
        else:
            name = self.meshObject.getName() + "-" + self.armatureObject.getData().name
        skeleton = Skeleton(name)
        skeleton = self._convertRestpose(skeleton, exportOptions, logger)
        
        # convert ActionActuators into Animations
        self._convertAnimations(skeleton, actionActuatorList, exportOptions, exportLogger)
        
        # write to file
        self._toFile(skeleton, exportOptions, exportLogger)
        
        self.skeleton = skeleton
        return
    
    def _convertAnimations(self, skeleton, armatureActionActuatorList, exportOptions, exportLogger):
        """Converts ActionActuators to Ogre animations.
        """
        # frames per second
        fps = Blender.Scene.GetCurrent().getRenderingContext().framesPerSec()
        # map armatureActionActuatorList to skeleton.animationsDict
        for armatureActionActuator in armatureActionActuatorList:
            # map armatureActionActuator to animation
            if (not skeleton.animationsDict.has_key(armatureActionActuator.name)):
                # create animation
                animation = Animation(armatureActionActuator.name)
                # map bones to tracks
                for boneName in armatureActionActuator.armatureAction.ipoDict.keys():
                    if (not(animation.tracksDict.has_key(boneName))):
                        # get bone object
                        if skeleton.bonesDict.has_key(boneName):
                            # create track
                            track = Track(animation, skeleton.bonesDict[boneName])
                            # map ipocurves to keyframes
                            # get ipo for that bone
                            ipo = armatureActionActuator.armatureAction.ipoDict[boneName]
                            # map curve names to curvepos
                            curveId = {}
                            index = 0
                            have_quat = 0
                            for curve in ipo.getCurves():
                                try:
                                    name = curve.getName()
                                    if (name == "LocX" or name == "LocY" or name == "LocZ" or \
                                    name == "SizeX" or name == "SizeY" or name == "SizeZ" or \
                                    name == "QuatX" or name == "QuatY" or name == "QuatZ" or name == "QuatW"):
                                        curveId[name] = index
                                        index += 1
                                    else:
                                    # bug: 2.28 does not return "Quat*"...
                                        if not have_quat:
                                            curveId["QuatX"] = index
                                            curveId["QuatY"] = index+1
                                            curveId["QuatZ"] = index+2
                                            curveId["QuatW"] = index+3
                                            index += 4
                                            have_quat = 1
                                except TypeError:
                                    # blender 2.32 does not implement IpoCurve.getName() for action Ipos
                                    if not have_quat:
                                        # no automatic assignments so far
                                        # guess Ipo Names       
                                        nIpoCurves = ipo.getNcurves()
                                        if nIpoCurves in [4,7,10]:
                                            exportLogger.logWarning("IpoCurve.getName() not available!")
                                            exportLogger.logWarning("The exporter tries to guess the IpoCurve names.")
                                            if (nIpoCurves >= 7):
                                                # not only Quats
                                                # guess: Quats and Locs
                                                curveId["LocX"] = index
                                                curveId["LocY"] = index+1
                                                curveId["LocZ"] = index+2
                                                index += 3      
                                            if (nIpoCurves == 10):
                                                # all possible Action IpoCurves
                                                curveId["SizeX"] = index
                                                curveId["SizeY"] = index+1
                                                curveId["SizeZ"] = index+2
                                                index += 3
                                            if (nIpoCurves >= 4):
                                                # at least 4 IpoCurves
                                                # guess: 4 Quats
                                                curveId["QuatX"] = index
                                                curveId["QuatY"] = index+1
                                                curveId["QuatZ"] = index+2
                                                curveId["QuatW"] = index+3
                                                index += 4
                                            have_quat = 1
                                        else:
                                            exportLogger.logError("IpoCurve.getName() not available!")
                                            exportLogger.logError("Could not guess the IpoCurve names. Other Blender versions may work.")
                            # get all frame numbers between startFrame and endFrame where this ipo has a point in one of its curves
                            frameNumberDict = {}
                            for curveIndex in range(ipo.getNcurves()):
                                for bez in range(ipo.getNBezPoints(curveIndex)):
                                    frame = int(ipo.getCurveBeztriple(curveIndex, bez)[3])
                                    frameNumberDict[frame] = frame
                            frameNumberDict[armatureActionActuator.startFrame] = armatureActionActuator.startFrame
                            frameNumberDict[armatureActionActuator.endFrame] = armatureActionActuator.endFrame
                            # remove frame numbers not in the startFrame endFrame range
                            if (armatureActionActuator.startFrame > armatureActionActuator.endFrame):
                                minFrame = armatureActionActuator.endFrame
                                maxFrame = armatureActionActuator.startFrame
                            else:
                                minFrame = armatureActionActuator.startFrame
                                maxFrame = armatureActionActuator.endFrame
                            for frameNumber in frameNumberDict.keys()[:]:
                                if ((frameNumber < minFrame) or (frameNumber > maxFrame)):
                                    del frameNumberDict[frameNumber]
                            frameNumberList = frameNumberDict.keys()
                            # convert frame numbers to seconds
                            # frameNumberDict: key = export time, value = frame number
                            frameNumberDict = {}
                            for frameNumber in frameNumberList:
                                if  (armatureActionActuator.startFrame <= armatureActionActuator.endFrame):
                                    # forward animation
                                    time = float(frameNumber-armatureActionActuator.startFrame)/fps
                                else:
                                    # backward animation
                                    time = float(armatureActionActuator.endFrame-frameNumber)/fps
                                # update animation duration
                                if animation.duration < time:
                                    animation.duration = time
                                frameNumberDict[time] = frameNumber
                            # create key frames
                            timeList = frameNumberDict.keys()
                            timeList.sort()
                            for time in timeList:
                                # Blender's ordering of transformations is deltaR*deltaS*deltaT
                                # in the bone's coordinate system.
                                frame = frameNumberDict[time]
                                loc = ( 0.0, 0.0, 0.0 )
                                rotQuat = Mathutils.Quaternion([1.0, 0.0, 0.0, 0.0])
                                sizeX = 1.0
                                sizeY = 1.0
                                sizeZ = 1.0
                                blenderLoc = [0, 0, 0]
                                hasLocKey = 0 #false
                                if curveId.has_key("LocX"):
                                    blenderLoc[0] = ipo.EvaluateCurveOn(curveId["LocX"], frame)
                                    hasLocKey = 1 #true
                                if curveId.has_key("LocY"):
                                    blenderLoc[1] = ipo.EvaluateCurveOn(curveId["LocY"], frame)
                                    hasLocKey = 1 #true
                                if curveId.has_key("LocZ"):
                                    blenderLoc[2] = ipo.EvaluateCurveOn(curveId["LocZ"], frame)
                                    hasLocKey = 1 #true
                                if hasLocKey:
                                    # Ogre's deltaT is in the bone's parent coordinate system
                                    loc = point_by_matrix(blenderLoc, skeleton.bonesDict[boneName].conversionMatrix)
                                if curveId.has_key("QuatX") and curveId.has_key("QuatY") and curveId.has_key("QuatZ") and curveId.has_key("QuatW"):
                                    if not (Blender.Get("version") == 234):
                                        rot = [ ipo.EvaluateCurveOn(curveId["QuatW"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatX"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatY"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatZ"], frame) ]
                                        rotQuat = Mathutils.Quaternion(rot)
                                    else:
                                        # Blender 2.34 quaternion naming bug
                                        rot = [ ipo.EvaluateCurveOn(curveId["QuatX"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatY"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatZ"], frame), \
                                                ipo.EvaluateCurveOn(curveId["QuatW"], frame) ]
                                        rotQuat = Mathutils.Quaternion(rot)
                                    rotQuat.normalize()
                                if curveId.has_key("SizeX"):
                                    sizeX = ipo.EvaluateCurveOn(curveId["SizeX"], frame)
                                if curveId.has_key("SizeY"):
                                    sizeY = ipo.EvaluateCurveOn(curveId["SizeY"], frame)
                                if curveId.has_key("SizeZ"):
                                    sizeZ = ipo.EvaluateCurveOn(curveId["SizeZ"], frame)
                                size = (sizeX, sizeY, sizeZ)
                                KeyFrame(track, time, loc, rotQuat, size)
                            # append track
                            animation.tracksDict[boneName] = track
                        else:
                            # ipo name contains bone but armature doesn't
                            exportLogger.logWarning("Unused action channel \"%s\" in action \"%s\" for skeleton \"%s\"." \
                                             % (boneName, armatureActionActuator.armatureAction.name, skeleton.name))
                    else:
                        # track for that bone already exists
                        exportLogger.logError("Ambiguous bone name \"%s\", track already exists." % boneName)
                # append animation
                skeleton.animationsDict[armatureActionActuator.name] = animation
            else:
                # animation export name already exists
                exportLogger.logError("Ambiguous animation name \"%s\"." % armatureActionActuator.name)
        return
    
    def _convertRestpose(self, skeleton, exportOptions, logger):
        """Calculate inital bone positions and rotations.
        """
        print "convert posed started"
        obj = self.armatureObject
        stack = []
        matrix = None
        matrix_one = Mathutils.Matrix([1.0, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1])
        if exportOptions.useWorldCoordinates:
            # world coordinates
            matrix = obj.getMatrix("worldspace")
        else:
            # local mesh coordinates
            armatureMatrix = obj.getMatrix("worldspace")
            # Blender returns direct access to matrix but we only want a copy
            # since we will be inverting it
            inverseMeshMatrix = matrix_one * self.meshObject.getObjectMatrix()
            inverseMeshMatrix.invert()
            matrix = armatureMatrix*inverseMeshMatrix
        # apply additional export transformation
        matrix = matrix*exportOptions.transformationMatrix()
        loc = [ 0.0, 0, 0 ]
        parent = None
        
        # get parent bones
        #note: in Blender 2.4, bones returns a dictionary of all bones in an armature
        boneDict = obj.getData().bones
        for bbone in boneDict.values():
            #print bbone, bbone.parent
            if bbone.parent == None:
                stack.append([bbone, parent, matrix, loc, 0, matrix_one])
                
        # iterate through bones and build ogre equivalent bones
        # blend bone matrix in armature space is perfect for ogre equivalency
        # 
        while len(stack):
            bbone, parent, accu_mat, parent_pos, parent_ds, invertedOgreTransformation = stack.pop()
            # preconditions: (R : rotation, T : translation, S : scale, M: general transformation matrix)
            #   accu_mat
            #     points to the tail of the parents bone, i.e. for root bones
            #     accu_mat = M_{object}*R_{additional on export}
            #     and for child bones
            #     accu_mat = T_{length of parent}*R_{parent}*T_{to head of parent}*M_{parent's parent}
            #  invertedOgreTransformation
            #    inverse of transformation done in Ogre so far, i.e. identity for root bones,
            #    M^{-1}_{Ogre, parent's parent}*T^{-1}_{Ogre, parent}*R^{-1}_{Ogre, parent} for child bones.
            
            head = bbone.head['BONESPACE']
            tail = bbone.tail['BONESPACE']
            
            # get the restmat 
            R_bmat = bbone.matrix['BONESPACE'].rotationPart()
            rotQuat = R_bmat.toQuat()
            R_bmat.resize4x4()
            
            #get the bone's root offset (in the parent's coordinate system)
            T_root = [ [       1,       0,       0,      0 ],
            [       0,       1,       0,      0 ],
            [       0,       0,       1,      0 ],
            [ head[0], head[1], head[2],      1 ] ]
            
            # get the bone length translation (length along y axis)
            dx, dy, dz = tail[0] - head[0], tail[1] - head[1], tail[2] - head[2]
            ds = math.sqrt(dx*dx + dy*dy + dz*dz)
            T_len = [ [ 1,  0,  0,  0 ],
                [ 0,  1,  0,  0 ],
                [ 0,  0,  1,  0 ],
                [ 0, ds,  0,  1 ] ]
            
            # calculate bone points in world coordinates
            accu_mat = matrix_multiply(accu_mat, T_root)
            pos = point_by_matrix([ 0, 0, 0 ], accu_mat)
            
            accu_mat = tmp_mat = matrix_multiply(accu_mat, R_bmat)
            # tmp_mat = R_{bone}*T_{to head}*M_{parent}
            accu_mat = matrix_multiply(accu_mat, T_len)
            
            # local rotation and distance from parent bone
            if parent:
                rotQuat = bbone.matrix['BONESPACE'].rotationPart().toQuat()
            else:
                rotQuat = (bbone.matrix['BONESPACE'].rotationPart().resize4x4()*matrix).toQuat()
                
            x, y, z = pos
            # pos = loc * M_{Ogre}
            loc = point_by_matrix([x, y, z], invertedOgreTransformation)
            x, y, z = loc
            ogreTranslationMatrix = [[ 1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [x, y, z, 1]]
            
            # R_{Ogre} is either
            # the rotation part of R_{bone}*T_{to_head}*M_{parent} for root bones or
            # the rotation part of R_{bone}*T_{to_head} of child bones
            ogreRotationMatrix = rotQuat.toMatrix()
            ogreRotationMatrix.resize4x4()
            invertedOgreTransformation = matrix_multiply(matrix_invert(ogreTranslationMatrix), invertedOgreTransformation)
            parent = Bone(skeleton, parent, bbone.name, loc, rotQuat, matrix_multiply(invertedOgreTransformation, tmp_mat))
            #print "bone created:", parent
            # matrix_multiply(invertedOgreTransformation, tmp_mat) is R*T*M_{parent} M^{-1}_{Ogre}T^{-1}_{Ogre}.
            # Necessary, since Ogre's delta location is in the Bone's parent coordinate system, i.e.
            # deltaT_{Blender}*R*T*M = deltaT_{Ogre}*T_{Ogre}*M_{Ogre}
            invertedOgreTransformation = matrix_multiply(matrix_invert(ogreRotationMatrix), invertedOgreTransformation)
            if bbone.children is not None:
                for child in bbone.children:
                    # make sure child bone is attached to current parent
                    if child.parent is not None:
                        if child.parent.name == bbone.name:
                            stack.append([child, parent, accu_mat, pos, ds, invertedOgreTransformation])
        return skeleton
        
    def _toFile(self, skeleton, exportOptions, exportLogger):
        """Writes converted skeleton to file.
        """
        file = skeleton.name+".skeleton.xml"
        exportLogger.logInfo("Skeleton \"%s\"" % file)
        f = open(os.path.join(exportOptions.exportPath, file), "w")
        f.write(tab(0)+"<skeleton>\n")
        f.write(tab(1)+"<bones>\n")
        for bone in skeleton.bones:
            f.write(tab(2)+"<bone id=\"%d\" name=\"%s\">\n" % (bone.id, bone.name))

            x, y, z = bone.loc
            f.write(tab(3)+"<position x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % (x, y, z))

            f.write(tab(3)+"<rotation angle=\"%.6f\">\n" % (bone.rotQuat.angle/360*2*math.pi))
            f.write(tab(4)+"<axis x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % tuple(bone.rotQuat.axis))
            f.write(tab(3)+"</rotation>\n")
            f.write(tab(2)+"</bone>\n")
        f.write(tab(1)+"</bones>\n")
        
        f.write(tab(1)+"<bonehierarchy>\n")
        for bone in skeleton.bones:
            parent = bone.parent
            if parent:
                f.write(tab(2)+"<boneparent bone=\"%s\" parent=\"%s\"/>\n" % (bone.name, parent.name))
        f.write(tab(1)+"</bonehierarchy>\n")

        f.write(tab(1)+"<animations>\n")

        for animation in skeleton.animationsDict.values():
            name = animation.name
            
            f.write(tab(2)+"<animation")
            f.write(" name=\"%s\"" % name)
            f.write(" length=\"%f\">\n" % animation.duration )
            
            f.write(tab(3)+"<tracks>\n")
            for track in animation.tracksDict.values():
                f.write(tab(4)+"<track bone=\"%s\">\n" % track.bone.name)
                f.write(tab(5)+"<keyframes>\n")
                
                for keyframe in track.keyframes:
                    f.write(tab(6)+"<keyframe time=\"%f\">\n" % keyframe.time)
                    x, y, z = keyframe.loc
                    f.write(tab(7)+"<translate x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % (x, y, z))
                    f.write(tab(7)+"<rotate angle=\"%.6f\">\n" % (keyframe.rotQuat.angle/360*2*math.pi))
                    
                    f.write(tab(8)+"<axis x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % tuple(keyframe.rotQuat.axis))
                    f.write(tab(7)+"</rotate>\n")
                    
                    f.write(tab(7)+"<scale x=\"%f\" y=\"%f\" z=\"%f\"/>\n" % keyframe.scale)
                    
                    f.write(tab(6)+"</keyframe>\n")
                
                f.write(tab(5)+"</keyframes>\n")
                f.write(tab(4)+"</track>\n");
            
            f.write(tab(3)+"</tracks>\n");
            f.write(tab(2)+"</animation>\n")
            
        f.write(tab(1)+"</animations>\n")
        f.write(tab(0)+"</skeleton>\n")
        f.close()
        convertXMLFile(os.path.join(exportOptions.exportPath, file))
        return
        
class ArmatureMeshExporter(ObjectExporter):
    """Exports an armature object as mesh.
    
       Converts a Blender armature into an animated Ogre mesh.
    """
    # TODO:    Use observer pattern for progress bar.
    # TODO: Get bone ids from skeletonExporter class.
    def __init__(self, armatureObject):
        """Constructor.
        
           @param armatureObject armature object to export.
        """
        # call base class constructor
        ObjectExporter.__init__(self, armatureObject)
        self.skeleton = None
        return
    
    def export(self, materialsDict, actionActuatorList, exportOptions, logger):
        """Exports the mesh object.
           
           @param materialsDict dictionary that contains already existing materials.
           @param actionActuatorList list of animations to export.
           @param exportOptions global export options.
           @param logger Logger for log messages.
           @return materialsDict with the new materials added.
        """
        # export skeleton
        armatureExporter = ArmatureExporter(self, self.object)
        armatureExporter.export(actionActuatorList, exportOptions, logger)
        self.skeleton = armatureExporter.skeleton
        self._convertToMesh(materialsDict, exportOptions, logger)
        return materialsDict
    def getName(self):
        return self.object.getData().name
    
    def _convertToMesh(self, materialsDict, exportOptions, logger):
        """Creates meshes in form of the armature bones.
        """
        obj = self.object
        stack = []
        # list of bone data (boneName, startPosition, endPosition)
        boneMeshList = []        
        matrix = None
        if exportOptions.useWorldCoordinates:
            # world coordinates
            matrix = obj.getMatrix("worldspace")
        else:
            # local mesh coordinates
            armatureMatrix = obj.getMatrix("worldspace")
            matrix_one = Mathutils.Matrix([1.0, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1])
            # note that blender returns direct access to the objects matrix but we only want
            # a copy so force a copy
            inverseMeshMatrix = matrix_one * self.getObjectMatrix()
            inverseMeshMatrix.invert()
            matrix = armatureMatrix * inverseMeshMatrix
        # apply additional export transformation
        matrix = matrix*exportOptions.transformationMatrix()
        loc = [ 0.0, 0, 0 ]
        parent = None
        
        # get parent bones
        boneDict = obj.getData().bones
        for bbone in boneDict.values():
            if bbone.parent == None:
                stack.append([bbone, parent, matrix, loc, 0])
        
        while len(stack):
            bbone, parent, accu_mat, parent_pos, parent_ds = stack.pop()
            # preconditions: (R : rotation, T : translation, S : scale, M: general transformation matrix)
            #   accu_mat
            #     points to the tail of the parents bone, i.e. for root bones
            #     accu_mat = M_{object}*R_{additional on export}
            #     and for child bones
            #     accu_mat = T_{length of parent}*R_{parent}*T_{to head of parent}*M_{parent's parent}
            #  invertedOgreTransformation
            #    inverse of transformation done in Ogre so far, i.e. identity for root bones,
            #    M^{-1}_{Ogre, parent's parent}*T^{-1}_{Ogre, parent}*R^{-1}_{Ogre, parent} for child bones.
            
            head = bbone.head['BONESPACE']
            tail = bbone.tail['BONESPACE']
            
            # get the restmat 
            R_bmat = bbone.matrix['BONESPACE'].rotationPart()
            rotQuat = R_bmat.toQuat()
            R_bmat.resize4x4()
            
            # get the bone's root offset (in the parent's coordinate system)
            T_root = [ [       1,       0,       0,      0 ],
            [       0,       1,       0,      0 ],
            [       0,       0,       1,      0 ],
            [ head[0], head[1], head[2],      1 ] ]
            
            # get the bone length translation (length along y axis)
            dx, dy, dz = tail[0] - head[0], tail[1] - head[1], tail[2] - head[2]
            ds = math.sqrt(dx*dx + dy*dy + dz*dz)
            T_len = [ [ 1,  0,  0,  0 ],
                [ 0,  1,  0,  0 ],
                [ 0,  0,  1,  0 ],
                [ 0, ds,  0,  1 ] ]
            
            # calculate bone points in world coordinates
            accu_mat = matrix_multiply(accu_mat, T_root)
            pos = point_by_matrix([ 0, 0, 0 ], accu_mat)
            
            accu_mat = tmp_mat = matrix_multiply(accu_mat, R_bmat)
            # tmp_mat = R_{bone}*T_{to head}*M_{parent}
            accu_mat = matrix_multiply(accu_mat, T_len)
            pos2 = point_by_matrix([ 0, 0, 0 ], accu_mat)
            boneMeshList.append([bbone.name, pos, pos2])
            if bbone.children is not None:
                for child in bbone.children:
                    # make sure child bone is attached to current parent
                    if child.parent is not None:
                        if child.parent.name == bbone.name:
                            stack.append([child, parent, accu_mat, pos, ds])
                            
        self._createMeshFromBoneList(materialsDict, boneMeshList)
        return

    def _makeFace(self, submesh, name, p1, p2, p3):
        normal = vector_normalize(vector_crossproduct(
                [ p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2] ],
                [ p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2] ]))
        v1 = Vertex(submesh, XMLVertex(p1, normal))
        v2 = Vertex(submesh, XMLVertex(p2, normal))
        v3 = Vertex(submesh, XMLVertex(p3, normal))

        id = self.skeleton.bonesDict[name]
        v1.influences.append(Influence(id, 1.0))
        v2.influences.append(Influence(id, 1.0))
        v3.influences.append(Influence(id, 1.0))

        Face(submesh, v1, v2, v3)
        return
    
    def _createMeshFromBoneList(self, materialsDict, boneMeshList):
        matName = "SkeletonMaterial"
        material = materialsDict.get(matName)
        if not material:
            material = DefaultMaterial(matName)
            materialsDict[matName] = material

        submesh = SubMesh(material)
        for name, p1, p2 in boneMeshList:
            axis = blender_bone2matrix(p1, p2, 0)
            axis = matrix_translate(axis, p1)
            dx, dy, dz = p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]
            ds = math.sqrt(dx*dx + dy*dy + dz*dz)
            d = 0.1 + 0.2 * (ds / 10.0)
            c1 = point_by_matrix([-d, 0,-d], axis)
            c2 = point_by_matrix([-d, 0, d], axis)
            c3 = point_by_matrix([ d, 0, d], axis)
            c4 = point_by_matrix([ d, 0,-d], axis)
            
            self._makeFace(submesh, name, p2, c1, c2)
            self._makeFace(submesh, name, p2, c2, c3)
            self._makeFace(submesh, name, p2, c3, c4)
            self._makeFace(submesh, name, p2, c4, c1)
            self._makeFace(submesh, name, c3, c2, c1)
            self._makeFace(submesh, name, c1, c4, c3)
        mesh = Mesh([submesh], self.skeleton)
        mesh.name = self.getName()
        mesh.write()
        return
    
######
# global variables
######
gameEngineMaterialsToggle = Draw.Create(0)
armatureToggle = Draw.Create(1)
worldCoordinatesToggle = Draw.Create(0)
ambientToggle = Draw.Create(0)
pathString = Draw.Create(os.path.dirname(Blender.Get('filename')))
materialString = Draw.Create(Blender.Scene.GetCurrent().getName()+".material")
scaleNumber = Draw.Create(1.0)
fpsNumber = Draw.Create(25)
# first rotation, around X-axis
rotXNumber = Draw.Create(0.0)
# second rotation, around Y-axis
rotYNumber = Draw.Create(0.0)
# third rotation, around Z-axis
rotZNumber = Draw.Create(0.0)
selectedObjectsList = Blender.Object.GetSelected()
selectedObjectsMenu = Draw.Create(0)
scrollbar = ReplacementScrollbar(0,0,0,0,0)
# key: objectName, value: armatureName
armatureDict = {}
# key: armatureName, value: armatureActionActuatorListView
# does only contain keys for the current selected objects
armatureActionActuatorListViewDict = {}
# key: armatureName, value: animationDictList
armatureAnimationDictListDict = {}
MAXACTUATORS = 100

# button event numbers:
BUTTON_EVENT_OK = 101
BUTTON_EVENT_QUIT = 102
BUTTON_EVENT_EXPORT = 103
BUTTON_EVENT_GAMEENGINEMATERIALSTOGGLE = 104
BUTTON_EVENT_ARMATURETOGGLE = 105
BUTTON_EVENT_WORLDCOORDINATESTOGGLE = 106
BUTTON_EVENT_AMBIENTTOGGLE = 107
BUTTON_EVENT_PATHSTRING = 108
BUTTON_EVENT_PATHBUTTON = 109
BUTTON_EVENT_MATERIALSTRING = 1010
BUTTON_EVENT_SCALENUMBER = 1011
BUTTON_EVENT_ROTXNUMBER = 1012
BUTTON_EVENT_ROTYNUMBER = 1013
BUTTON_EVENT_ROTZNUMBER = 1014
BUTTON_EVENT_FPSNUMBER = 1015
BUTTON_EVENT_SCROLLBAR = 1016
BUTTON_EVENT_SCROLLBARUP = 1017
BUTTON_EVENT_SRCROLLBARDOWN = 1018
BUTTON_EVENT_UPDATEBUTTON = 1019
BUTTON_EVENT_SELECTEDOBJECTSMENU = 1020
BUTTON_EVENT_ACTUATOR_RANGESTART = 1021

exportLogger = Logger()

OGRE_LOGO = Buffer(GL_BYTE, [48,122*4],[[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,64,0,0,0,95,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,0,0,0,127,0,1,0,127,0,2,0,127,2,5,2,127,2,5,2,127,4,6,4,127,5,8,5,127,8,11,8,127,8,11,8,127,3,5,3,127,2,3,2,127,0,1,0,127,0,1,0,127,0,1,0,127,0,1,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,1,2,1,127,4,6,4,127,10,13,10,127,18,22,18,127,23,28,23,127,24,30,24,127,25,31,25,127,25,31,25,127,26,32,26,127,26,32,26,127,26,32,26,127,25,31,25,127,24,30,24,127,18,23,18,127,3,5,3,127,4,6,4,127,8,11,8,127,9,12,9,127,13,17,13,127,17,22,17,127,15,19,15,127,7,9,7,127,1,2,1,127,0,0,0,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,2,4,2,127,4,6,4,127,18,22,18,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,18,22,18,127,15,19,15,127,20,26,20,127,25,31,25,127,26,32,26,127,26,32,26,127,25,31,25,127,25,31,25,127,25,31,25,127,26,32,26,127,24,30,24,127,16,20,16,127,4,5,4,127,0,0,0,95,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,1,1,1,127,13,15,13,127,12,15,12,127,24,29,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,29,23,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,3,5,3,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,1,1,1,127,19,24,19,127,11,15,11,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,17,21,17,127,22,28,22,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,3,5,3,127,0,0,0,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,20,24,20,127,16,20,16,127,20,25,20,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,22,28,22,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,3,5,3,127,0,0,0,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,64,5,7,5,127,26,32,26,127,15,19,15,127,41,48,41,127,38,45,38,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,3,4,3,127,0,0,0,127,58,66,58,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,127,20,24,20,127,27,34,27,127,26,32,26,127,47,55,47,127,47,55,47,127,39,46,39,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,11,16,11,127,0,1,0,127,3,3,3,127,94,106,94,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,127,33,39,33,127,45,52,45,127,28,32,28,127,47,55,47,127,44,51,44,127,39,46,39,127,27,33,27,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,21,26,21,127,0,2,0,127,0,0,0,127,23,26,23,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,127,24,28,24,127,33,40,33,127,18,22,18,127,29,35,29,127,25,31,25,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,5,8,5,127,1,2,1,127,0,0,0,127,70,79,70,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,94,105,94,127,70,79,70,127,76,86,76,127,90,101,90,127,103,116,103,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,64,0,0,0,127,4,6,4,127,12,16,12,127,22,27,22,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,29,23,127,28,34,28,127,35,42,35,127,28,35,28,127,25,31,25,127,23,29,23,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,17,21,17,127,0,2,0,127,0,0,0,127,31,36,31,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,100,112,100,127,92,103,92,127,103,116,103,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,100,112,100,127,81,92,81,127,68,77,68,127,65,73,65,127,65,73,65,127,76,86,76,127,78,88,78,127,83,94,83,127,92,103,92,127,85,95,85,127,31,35,31,127,6,7,6,127,6,7,6,127,13,14,13,127,13,14,13,127,19,21,19,127,26,29,26,127,26,29,26,127,48,54,48,127,96,108,96,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,70,78,70,127,3,3,3,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,12,13,11,127,23,26,23,127,36,40,36,127,49,55,49,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,64,0,0,0,127,2,4,2,127,16,20,16,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,26,33,26,127,59,68,59,127,81,91,81,127,87,98,87,127,86,96,86,127,80,90,80,127,71,79,71,127,59,66,59,127,36,41,35,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,31,24,127,26,32,26,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,5,8,5,127,0,1,0,127,18,20,18,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,91,103,91,127,58,65,58,127,29,33,29,127,6,7,6,127,0,0,0,127,0,0,0,127,1,2,1,127,22,24,22,127,54,61,54,127,94,106,94,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,88,99,88,127,51,58,51,127,18,21,18,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,17,19,17,127,48,54,48,127,80,91,80,127,102,115,102,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,29,33,29,127,0,0,0,127,41,31,14,127,33,25,11,127,18,14,6,127,2,2,1,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,127,2,3,2,127,24,29,24,127,26,32,26,127,24,30,24,127,25,31,25,127,24,30,24,127,24,30,24,127,24,30,24,127,23,29,23,127,34,41,34,127,78,88,78,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,97,87,127,87,97,87,127,84,93,84,127,62,69,62,127,34,40,34,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,28,23,127,26,30,26,127,36,38,36,127,47,50,46,127,39,42,37,127,34,40,34,127,30,37,30,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,15,19,15,127,0,1,0,127,0,0,0,127,102,115,102,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,94,106,94,127,43,48,43,127,4,5,4,127,0,0,0,127,0,0,0,127,0,0,0,127,6,5,2,127,16,12,5,127,2,2,1,127,0,0,0,127,0,0,0,127,7,8,7,127,58,65,58,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,96,108,96,127,41,47,41,127,1,1,1,127,0,0,0,127,0,0,0,127,6,5,2,127,27,21,9,127,42,33,14,127,46,36,16,127,46,36,16,127,33,25,11,127,31,24,11,127,25,19,9,127,16,12,5,127,12,9,4,127,0,0,0,127,107,82,36,127,115,88,38,127,107,82,36,127,107,82,36,127,100,76,33,127,92,71,31,127,88,68,30,127,0,0,0,127,4,3,2,127,0,0,0,127,0,0,0,127,0,0,0,127,13,15,13,127,65,73,65,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,13,14,13,127,0,0,0,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,109,84,36,127,96,73,32,127,80,62,27,127,65,50,22,127,52,40,17,127,37,28,12,127,21,16,7,127,2,2,1,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,127,9,11,9,127,48,56,48,127,45,53,45,127,41,48,41,127,33,40,33,127,34,41,34,127,37,44,37,127,54,62,54,127,77,87,77,127,87,97,87,127,87,97,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,79,88,79,127,61,69,61,127,25,31,25,127,25,31,25,127,23,28,23,127,19,23,19,127,42,43,41,127,60,60,59,127,61,61,59,127,61,61,59,127,63,63,61,127,35,37,34,127,38,45,38,127,33,39,33,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,15,19,15,127,0,1,0,127,0,0,0,127,102,115,102,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,81,91,81,127,9,11,9,127,0,0,0,127,2,2,1,127,44,34,15,127,86,66,29,127,115,88,38,127,122,94,41,127,122,94,41,127,121,92,40,127,94,72,31,127,39,30,13,127,0,0,0,127,0,0,0,127,40,45,40,127,101,114,101,127,105,118,105,127,105,118,105,127,105,118,105,127,85,95,85,127,11,13,11,127,0,0,0,127,4,3,2,127,50,38,17,127,94,72,31,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,92,71,31,127,0,0,0,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,2,2,1,127,105,81,35,127,98,75,33,127,60,46,20,127,23,18,8,127,0,0,0,127,1,1,1,127,90,102,90,127,105,118,105,127,105,118,105,127,105,118,105,127,6,7,6,127,0,0,0,127,115,88,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,8,6,3,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,127,3,5,3,127,45,53,45,127,46,54,46,127,46,54,46,127,47,55,47,127,46,54,46,127,68,78,68,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,98,87,127,67,76,67,127,38,46,38,127,21,26,21,127,50,52,50,127,60,60,59,127,61,61,59,127,60,60,58,127,60,60,58,127,60,60,58,127,61,61,59,127,39,41,38,127,52,59,52,127,67,76,67,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,15,19,15,127,0,1,0,127,0,0,0,127,102,115,102,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,59,67,59,127,1,1,1,127,0,0,0,127,35,27,12,127,105,81,35,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,86,66,29,127,8,6,3,127,0,0,0,127,36,40,36,127,105,118,105,127,105,118,105,127,82,92,82,127,7,7,7,127,0,0,0,127,31,24,10,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,80,62,27,127,0,0,0,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,82,63,28,127,46,36,16,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,27,21,9,127,0,0,0,127,78,88,78,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,127,0,0,0,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,127,0,2,0,127,41,49,41,127,46,54,46,127,46,54,46,127,49,56,49,127,77,87,77,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,85,96,85,127,55,64,55,127,44,52,44,127,23,28,23,127,17,22,17,127,90,92,90,127,84,84,82,127,60,60,58,127,60,60,58,127,60,60,58,127,60,60,58,127,61,61,59,127,39,41,38,127,54,62,54,127,62,71,62,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,15,20,15,127,0,1,0,127,0,0,0,127,102,115,102,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,81,90,81,127,1,1,1,127,0,0,0,127,61,47,21,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,103,79,34,127,12,9,4,127,0,0,0,127,47,52,47,127,93,104,93,127,8,9,8,127,0,0,0,127,52,40,17,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,0,0,0,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,63,49,21,127,105,81,35,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,0,0,0,127,9,11,9,127,101,113,101,127,105,118,105,127,105,118,105,127,105,118,105,127,0,0,0,127,0,0,0,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,69,53,23,127,0,0,0,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,127,0,1,0,127,37,44,37,127,46,54,46,127,49,57,49,127,79,89,79,127,87,97,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,56,64,56,127,46,53,46,127,25,31,25,127,22,27,22,127,25,31,25,127,44,47,44,127,116,116,115,127,59,59,57,127,60,60,58,127,60,60,58,127,60,60,58,127,61,61,59,127,38,41,37,127,69,78,69,127,45,53,45,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,15,20,15,127,0,0,0,127,5,6,5,127,104,117,104,127,105,118,105,127,105,118,105,127,105,118,105,127,93,104,93,127,8,9,8,127,0,0,0,127,61,47,21,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,96,73,32,127,2,2,1,127,0,0,0,127,24,28,24,127,0,0,0,127,37,28,12,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,10,8,3,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,88,68,30,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,39,30,13,127,0,0,0,127,43,49,43,127,105,118,105,127,105,118,105,127,105,118,105,127,93,105,93,127,0,0,0,127,14,11,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,39,30,13,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,64,0,1,0,127,21,25,21,127,48,57,49,127,82,92,82,127,87,97,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,97,87,127,87,98,87,127,60,69,60,127,43,50,43,127,29,36,29,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,116,116,116,127,71,71,70,127,60,60,58,127,60,60,58,127,60,60,58,127,62,62,60,127,30,32,29,127,75,85,75,127,29,36,29,127,25,31,25,127,24,30,24,127,24,30,24,127,23,28,23,127,10,14,10,127,0,0,0,127,40,45,40,127,105,118,105,127,105,118,105,127,105,118,105,127,105,118,105,127,33,38,33,127,0,0,0,127,39,30,13,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,67,52,23,127,0,0,0,127,0,0,0,127,10,8,3,127,113,87,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,107,82,36,127,84,65,28,127,71,54,24,127,115,88,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,67,51,22,127,16,12,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,105,81,35,127,2,2,1,127,0,0,0,127,0,0,0,127,18,21,18,127,61,69,61,127,102,115,102,127,92,103,92,127,0,0,0,127,16,12,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,60,46,20,127,52,40,17,127,69,53,23,127,86,66,29,127,10,8,3,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,127,2,5,2,127,49,57,49,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,98,87,127,86,97,86,127,75,84,75,127,53,61,53,127,34,41,34,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,22,28,22,127,96,97,96,127,93,93,92,127,59,59,58,127,60,60,58,127,60,60,58,127,61,61,59,127,34,39,34,127,74,84,74,127,23,29,23,127,25,31,25,127,37,39,34,127,47,47,41,127,44,45,39,127,17,18,16,127,0,0,0,127,52,59,52,127,105,118,105,127,105,118,105,127,105,118,105,127,81,92,81,127,0,0,0,127,8,6,3,127,111,85,37,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,50,38,17,127,16,12,5,127,33,25,11,127,103,79,34,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,23,18,8,127,0,0,0,127,69,53,23,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,27,21,9,127,0,0,0,127,0,0,0,127,0,0,0,127,92,71,31,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,61,47,21,127,18,14,6,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,117,90,39,127,88,68,29,127,54,41,18,127,14,11,5,127,0,0,0,127,0,0,0,127,17,18,17,127,68,76,68,127,0,0,0,127,21,16,7,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,31,24,11,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,95,0,0,0,127,37,43,37,127,89,100,89,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,97,87,127,88,99,88,127,82,92,82,127,61,69,61,127,36,42,36,127,27,32,27,127,23,29,23,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,29,23,127,78,80,76,127,102,102,102,127,58,58,57,127,60,60,58,127,60,60,58,127,58,58,56,127,40,47,40,127,56,64,56,127,24,29,23,127,44,45,40,127,49,49,43,127,49,49,43,127,46,46,41,127,41,42,37,127,0,0,0,127,38,43,38,127,105,118,105,127,105,118,105,127,105,118,105,127,33,37,33,127,0,0,0,127,61,47,21,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,0,0,0,127,0,0,0,127,0,0,0,127,12,9,4,127,113,87,38,127,122,94,41,127,122,94,41,127,122,94,41,127,84,65,28,127,4,3,2,127,115,88,38,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,42,33,14,127,0,0,0,127,119,91,40,127,102,78,34,127,75,57,25,127,52,40,17,127,88,68,29,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,61,47,21,127,31,24,11,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,84,65,28,127,19,15,7,127,0,0,0,127,4,5,4,127,0,0,0,127,31,24,11,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,111,85,37,127,115,88,38,127,122,94,41,127,122,94,41,127,48,37,16,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,32,0,0,0,127,6,7,5,127,67,75,67,127,89,100,89,127,87,97,87,127,87,97,87,127,87,98,87,127,88,99,88,127,88,98,88,127,80,90,80,127,62,71,62,127,45,52,45,127,39,46,39,127,57,65,57,127,65,74,65,127,59,67,59,127,54,61,54,127,55,61,55,127,28,34,28,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,64,67,64,127,109,109,108,127,58,58,57,127,60,60,58,127,61,60,59,127,50,50,47,127,47,55,47,127,33,39,33,127,44,44,39,127,48,48,42,127,48,48,42,127,28,30,25,127,36,37,31,127,48,48,42,127,1,2,1,127,36,41,36,127,105,118,105,127,105,118,105,127,99,111,99,127,4,5,4,127,2,2,1,127,113,87,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,65,50,22,127,0,0,0,127,30,34,30,127,27,30,27,127,0,0,0,127,67,51,22,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,58,44,19,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,71,54,24,127,0,0,0,127,18,14,6,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,54,41,18,127,31,24,11,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,56,43,19,127,0,0,0,127,0,0,0,127,31,24,11,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,37,28,12,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,95,0,0,0,127,2,3,2,127,28,32,28,127,58,65,58,127,56,64,56,127,50,57,50,127,46,54,46,127,42,49,42,127,43,50,43,127,62,71,62,127,80,90,80,127,87,98,87,127,87,98,87,127,87,97,87,127,87,98,87,127,86,97,87,127,78,85,78,127,46,52,46,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,64,67,64,127,104,104,104,127,58,58,57,127,60,60,58,127,62,61,60,127,34,38,33,127,37,43,37,127,50,51,44,127,48,48,42,127,48,48,42,127,23,27,22,127,32,36,30,127,95,95,82,127,43,45,39,127,0,0,0,127,45,51,45,127,105,118,105,127,105,118,105,127,71,80,71,127,0,0,0,127,35,27,12,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,103,79,35,127,2,2,1,127,0,0,0,127,11,13,11,127,0,0,0,127,65,50,22,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,23,18,8,127,0,0,0,127,35,27,12,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,46,36,16,127,41,31,14,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,119,91,40,127,37,28,12,127,50,38,17,127,73,56,24,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,69,53,23,127,0,0,0,127,44,34,15,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,27,21,9,127,0,0,0,127,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,8,10,8,127,51,59,51,127,84,95,84,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,87,127,63,71,63,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,23,29,23,127,76,78,75,127,100,100,99,127,58,58,57,127,61,60,59,127,53,54,51,127,24,30,24,127,29,33,28,127,77,76,63,127,47,48,42,127,29,32,27,127,24,30,24,127,30,35,29,127,90,91,84,127,28,29,25,127,0,0,0,127,77,86,76,127,105,118,105,127,105,118,105,127,44,50,44,127,0,0,0,127,69,53,23,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,81,62,27,127,4,3,2,127,0,0,0,127,12,9,4,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,4,3,2,127,0,0,0,127,54,41,18,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,46,36,16,127,46,36,16,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,48,37,16,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,42,33,14,127,46,36,16,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,16,12,5,127,0,0,0,127,0,0,0,127,4,3,2,127,6,5,2,127,0,0,0,95,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,64,0,0,0,95,1,1,1,127,60,68,60,127,87,98,87,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,97,87,127,73,82,73,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,22,28,22,127,89,92,89,127,87,87,86,127,59,59,58,127,60,59,58,127,31,35,31,127,25,31,25,127,43,45,38,127,74,74,62,127,43,43,38,127,22,28,22,127,25,31,25,127,24,30,24,127,26,32,26,127,13,14,12,127,0,0,0,127,100,113,100,127,105,118,105,127,105,118,105,127,21,24,21,127,0,0,0,127,98,75,33,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,113,87,38,127,92,71,31,127,117,90,39,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,19,15,7,127,0,0,0,127,71,54,24,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,39,30,13,127,50,38,17,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,82,63,28,127,0,0,0,127,23,26,23,127,38,42,38,127,5,7,5,127,0,0,0,127,96,73,32,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,111,85,37,127,54,41,18,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,82,63,28,127,16,12,5,127,16,12,5,127,16,12,5,127,12,9,4,127,46,35,16,127,82,63,28,127,117,90,39,127,46,36,16,127,0,0,0,127,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,33,38,33,127,89,99,89,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,84,94,84,127,28,35,28,127,25,31,25,127,25,31,25,127,25,31,25,127,22,28,22,127,100,101,100,127,73,73,71,127,61,60,59,127,35,38,35,127,24,30,24,127,24,30,24,127,48,51,41,127,69,69,57,127,36,37,32,127,24,30,24,127,28,34,28,127,25,31,25,127,25,31,25,127,17,21,17,127,0,0,0,127,80,90,80,127,105,118,105,127,105,118,105,127,6,7,6,127,0,0,0,127,115,88,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,56,43,19,127,0,0,0,127,88,68,29,127,117,90,39,127,107,82,36,127,92,71,31,127,80,62,27,127,69,53,23,127,60,46,20,127,46,36,16,127,33,25,11,127,23,18,8,127,4,3,2,127,61,47,21,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,65,50,22,127,0,0,0,127,20,22,20,127,26,30,26,127,0,0,0,127,2,2,1,127,109,84,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,21,16,7,127,60,46,20,127,94,72,31,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,54,41,18,127,0,0,0,127,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,6,7,6,127,81,91,81,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,88,98,88,127,60,68,60,127,27,33,27,127,24,30,24,127,25,31,25,127,22,28,22,127,91,91,91,127,57,58,56,127,31,36,31,127,24,30,24,127,25,31,25,127,25,31,25,127,27,31,26,127,70,71,58,127,41,42,36,127,37,43,37,127,66,74,66,127,23,29,23,127,25,31,25,127,19,22,19,127,0,0,0,127,75,84,75,127,105,118,105,127,102,114,102,127,0,0,0,127,4,3,2,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,117,90,39,127,31,24,10,127,2,2,1,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,4,3,2,127,61,47,21,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,46,36,16,127,0,0,0,127,0,0,0,127,0,0,0,127,8,6,3,127,73,56,24,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,61,47,21,127,0,0,0,127,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,45,52,45,127,87,98,88,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,82,92,82,127,46,54,46,127,34,41,34,127,25,31,25,127,25,31,25,127,26,30,26,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,33,37,31,127,48,48,42,127,43,43,38,127,66,74,65,127,23,29,23,127,25,31,25,127,20,25,20,127,0,0,0,127,70,78,70,127,105,118,105,127,92,103,92,127,0,0,0,127,16,12,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,86,66,29,127,48,37,16,127,31,24,11,127,16,12,5,127,23,18,8,127,33,25,11,127,52,40,17,127,71,54,24,127,96,73,32,127,117,90,39,127,63,49,21,127,73,56,24,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,88,68,29,127,77,59,26,127,77,59,26,127,90,69,30,127,117,90,39,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,73,56,24,127,0,0,0,127,0,0,0,32],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,25,28,25,127,88,99,88,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,86,97,86,127,87,98,87,127,70,79,70,127,46,54,46,127,47,55,47,127,45,52,45,127,30,37,30,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,44,52,44,127,72,81,72,127,70,79,70,127,23,29,23,127,25,31,25,127,21,25,21,127,0,0,0,127,66,73,65,127,105,118,105,127,92,103,92,127,0,0,0,127,16,12,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,80,62,27,127,77,59,26,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,77,59,26,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,1,0,127,64,72,64,127,87,97,87,127,86,97,86,127,86,97,86,127,87,97,87,127,86,97,86,127,86,96,86,127,85,95,85,127,71,80,71,127,47,55,47,127,46,54,46,127,46,54,46,127,46,54,46,127,47,55,47,127,31,38,31,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,23,29,23,127,59,67,59,127,77,87,77,127,58,66,58,127,25,31,25,127,25,31,25,127,22,27,22,127,0,0,0,127,48,54,48,127,105,118,105,127,92,103,92,127,0,0,0,127,16,12,5,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,80,62,27,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,92,71,31,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,127,14,16,14,127,88,99,88,127,88,98,88,127,88,98,88,127,72,82,72,127,51,59,51,127,52,61,52,127,55,63,55,127,47,55,47,127,45,53,45,127,45,53,45,127,46,54,46,127,46,54,46,127,46,54,46,127,45,53,45,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,37,44,37,127,76,86,76,127,73,82,73,127,32,39,32,127,23,29,23,127,2,2,2,127,30,34,30,95,105,118,105,64,98,111,98,64,0,0,0,95,4,3,2,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,115,88,38,127,92,71,31,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,119,91,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,127,21,24,21,127,55,62,55,127,51,57,50,127,64,72,64,127,86,96,86,127,85,95,85,127,84,94,84,127,86,96,86,127,84,95,84,127,82,92,82,127,75,85,75,127,52,60,52,127,46,54,46,127,46,54,46,127,45,53,45,127,26,32,26,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,29,36,29,127,28,34,28,127,24,30,24,127,62,71,62,127,88,99,88,127,66,75,66,127,24,30,24,127,8,11,8,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,105,81,35,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,100,76,33,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,107,82,36,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,31,36,31,127,35,40,35,127,33,36,32,127,31,34,31,127,47,55,47,127,51,59,51,127,47,55,47,127,39,46,39,127,29,36,29,127,37,43,37,127,52,60,52,127,77,87,77,127,49,58,49,127,46,54,46,127,40,48,40,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,29,35,29,127,80,90,80,127,59,67,59,127,24,30,24,127,24,30,24,127,76,86,76,127,87,98,87,127,39,46,39,127,17,22,17,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,75,57,25,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,79,60,26,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,113,87,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,71,55,24,127,103,79,35,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,117,90,39,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,127,41,48,41,127,69,79,69,127,39,45,39,127,47,54,47,127,77,87,77,127,86,97,86,127,88,97,87,127,87,97,86,127,82,93,83,127,57,65,57,127,25,31,25,127,24,30,24,127,26,32,26,127,26,32,26,127,26,32,26,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,75,85,75,127,87,98,87,127,67,75,67,127,23,29,23,127,23,29,23,127,56,64,56,127,85,95,85,127,75,84,75,127,24,30,24,127,3,3,3,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,127,29,22,10,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,119,91,40,127,8,6,3,127,109,84,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,119,91,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,6,5,2,127,107,82,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,0,0,0,127,0,0,0,64],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,12,15,12,127,45,53,46,127,48,56,48,127,65,72,63,127,98,81,79,127,123,119,119,127,117,108,108,127,94,79,76,127,88,88,80,127,64,73,64,127,24,30,24,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,35,41,35,127,86,96,86,127,87,98,87,127,61,69,61,127,23,29,23,127,24,30,24,127,46,53,46,127,84,94,84,127,87,98,87,127,55,63,55,127,10,12,10,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,92,71,31,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,75,57,25,127,0,0,0,127,52,40,17,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,12,9,4,127,0,0,0,127,109,84,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,14,11,5,127,0,0,0,95],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,22,26,22,127,30,37,30,127,23,29,23,127,41,40,35,127,91,73,72,127,113,103,103,127,100,75,75,127,87,58,58,127,83,72,66,127,54,63,55,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,30,25,127,34,41,34,127,69,78,69,127,81,91,81,127,34,41,34,127,25,31,25,127,23,29,23,127,61,69,61,127,82,92,82,127,75,85,75,127,82,92,82,127,24,29,24,127,1,1,1,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,127,23,18,8,127,119,91,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,17,14,6,127,0,0,0,127,2,2,1,127,96,73,32,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,75,58,25,127,6,5,2,127,0,0,0,127,0,0,0,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,18,14,6,127,0,0,0,127],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,24,29,24,127,48,56,48,127,28,34,28,127,24,30,24,127,25,31,25,127,36,37,32,127,68,55,52,127,82,63,62,127,80,52,52,127,81,82,74,127,28,34,28,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,23,29,23,127,25,31,25,127,24,30,24,127,25,31,25,127,24,29,24,127,56,64,56,127,87,97,87,127,70,79,70,127,88,99,88,127,49,57,49,127,10,12,10,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,44,34,15,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,67,52,23,127,0,0,0,127,0,0,0,95,0,0,0,127,12,9,4,127,109,84,36,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,100,76,33,127,33,25,11,127,0,0,0,127,0,0,0,127,0,0,0,95,0,0,0,127,107,82,36,127,117,90,39,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,31,24,11,127,0,0,0,127],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,14,16,14,127,81,91,81,127,72,81,72,127,43,51,43,127,23,29,23,127,24,30,24,127,23,30,24,127,23,30,23,127,25,31,25,127,26,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,26,32,26,127,32,39,32,127,30,37,30,127,24,30,24,127,25,31,25,127,25,31,25,127,25,32,25,127,83,93,83,127,77,86,77,127,87,97,87,127,80,90,80,127,22,27,22,127,1,1,1,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,46,35,15,127,121,92,40,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,96,73,32,127,4,3,2,127,0,0,0,95,0,0,0,0,0,0,0,32,0,0,0,127,12,9,4,127,98,75,33,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,96,73,32,127,40,31,14,127,2,2,1,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,32,0,0,0,127,0,0,0,127,0,0,0,127,2,2,1,127,16,12,5,127,25,19,9,127,33,25,11,127,46,36,16,127,56,43,19,127,61,47,21,127,77,59,26,127,84,65,28,127,92,71,31,127,107,82,36,127,115,88,38,127,122,94,41,127,122,94,41,127,39,30,13,127,0,0,0,127],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,18,21,18,127,83,93,83,127,89,100,89,127,71,81,71,127,54,61,54,127,37,44,37,127,24,30,24,127,23,29,23,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,24,30,24,127,42,50,42,127,70,79,70,127,87,98,87,127,74,83,74,127,28,35,28,127,25,31,25,127,24,30,24,127,42,49,42,127,76,86,76,127,86,97,86,127,88,99,88,127,41,49,41,127,11,14,11,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,27,21,9,127,105,81,35,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,98,75,33,127,12,9,4,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,2,2,1,127,58,44,19,127,113,87,38,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,105,81,35,127,63,49,21,127,21,16,7,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,16,12,5,127,6,5,2,127,0,0,0,95],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,13,17,13,127,61,70,61,127,85,96,85,127,89,100,89,127,88,98,88,127,77,87,77,127,60,67,60,127,26,32,26,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,25,31,25,127,12,16,12,127,12,15,12,127,40,46,40,127,80,90,80,127,80,89,80,127,34,40,34,127,24,30,24,127,23,29,23,127,51,59,51,127,88,99,88,127,86,97,86,127,76,85,76,127,22,27,22,127,1,2,1,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,4,3,2,127,59,46,20,127,111,85,37,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,119,91,40,127,65,50,22,127,4,3,2,127,0,0,0,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,95,0,0,0,127,4,3,2,127,44,34,15,127,80,62,27,127,111,85,37,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,122,94,41,127,121,92,40,127,100,76,33,127,75,57,25,127,48,37,16,127,18,13,6,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,127,0,0,0,64,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,127,19,23,19,127,46,53,46,127,64,72,64,127,80,90,80,127,85,96,85,127,74,84,74,127,28,34,28,127,25,31,25,127,25,31,25,127,25,30,25,127,25,31,25,127,25,31,25,127,25,31,25,127,17,21,17,127,1,3,1,127,0,1,0,127,0,0,0,127,9,11,9,127,51,59,52,127,82,93,83,127,45,52,45,127,23,29,23,127,24,30,24,127,59,67,59,127,88,99,88,127,85,96,85,127,30,37,30,127,12,15,12,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,4,3,2,127,42,33,14,127,82,63,28,127,107,82,36,127,103,79,35,127,84,65,28,127,54,41,18,127,12,9,4,127,0,0,0,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,0,0,0,127,0,0,0,127,10,8,3,127,25,19,9,127,31,24,11,127,31,24,11,127,31,24,11,127,31,24,11,127,18,14,6,127,35,27,12,127,105,81,35,127,80,62,27,127,54,41,18,127,29,22,10,127,6,5,2,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,95,0,0,0,64,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,8,10,8,127,33,39,33,127,44,51,44,127,46,53,46,127,44,52,44,127,39,46,39,127,25,30,25,127,25,31,25,127,25,31,25,127,24,30,24,127,15,19,15,127,5,7,5,127,0,1,0,127,0,0,0,127,0,0,0,95,0,0,0,64,0,0,0,64,0,1,0,127,21,24,21,127,66,74,66,127,57,66,57,127,24,30,24,127,23,29,23,127,52,60,52,127,40,47,40,127,24,30,24,127,23,28,23,127,1,2,1,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,95,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,64,0,0,0,95,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,64,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,11,13,11,127,23,28,23,127,33,39,33,127,36,43,36,127,23,29,23,127,20,26,20,127,11,15,11,127,3,4,3,127,0,1,0,127,0,0,0,127,0,0,0,95,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,3,5,3,127,37,41,37,127,58,66,58,127,27,33,27,127,24,30,24,127,26,32,26,127,25,31,25,127,25,31,25,127,8,9,8,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,64,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,0,0,0,127,0,0,0,127,0,0,0,127,0,1,0,127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,127,12,15,12,127,42,49,42,127,32,39,32,127,24,30,24,127,25,31,25,127,25,31,25,127,18,22,18,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,2,2,2,127,23,27,23,127,37,43,37,127,26,33,26,127,25,31,25,127,24,30,24,127,4,4,4,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,4,5,4,127,24,28,23,127,29,35,29,127,25,31,25,127,12,16,12,127,0,0,0,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,95,4,4,4,127,11,14,11,127,16,20,16,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,64,0,0,0,127,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]])

#######################################################################################
## math functions

def matrix_translate(m, v):
  m[3][0] += v[0]
  m[3][1] += v[1]
  m[3][2] += v[2]
  return m

def matrix_multiply(b, a):
  """ matrix_multiply(b, a) = a*b
  """
  return [ [
    a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0],
    a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1],
    a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2],
    0.0,
    ], [
    a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0],
    a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1],
    a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2],
    0.0,
    ], [
    a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0],
    a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1],
    a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2],
     0.0,
    ], [
    a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + b[3][0],
    a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + b[3][1],
    a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + b[3][2],
    1.0,
    ] ]

def matrix_invert(m):
  det = (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
       - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
       + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
  if det == 0.0: return None
  det = 1.0 / det
  r = [ [
      det * (m[1][1] * m[2][2] - m[2][1] * m[1][2]),
    - det * (m[0][1] * m[2][2] - m[2][1] * m[0][2]),
      det * (m[0][1] * m[1][2] - m[1][1] * m[0][2]),
      0.0,
    ], [
    - det * (m[1][0] * m[2][2] - m[2][0] * m[1][2]),
      det * (m[0][0] * m[2][2] - m[2][0] * m[0][2]),
    - det * (m[0][0] * m[1][2] - m[1][0] * m[0][2]),
      0.0
    ], [
      det * (m[1][0] * m[2][1] - m[2][0] * m[1][1]),
    - det * (m[0][0] * m[2][1] - m[2][0] * m[0][1]),
      det * (m[0][0] * m[1][1] - m[1][0] * m[0][1]),
      0.0,
    ] ]
  r.append([
    -(m[3][0] * r[0][0] + m[3][1] * r[1][0] + m[3][2] * r[2][0]),
    -(m[3][0] * r[0][1] + m[3][1] * r[1][1] + m[3][2] * r[2][1]),
    -(m[3][0] * r[0][2] + m[3][1] * r[1][2] + m[3][2] * r[2][2]),
    1.0,
    ])
  return r

def matrix_transpose(m):
  return [ [ m[0][0], m[1][0], m[2][0], m[3][0] ],
           [ m[0][1], m[1][1], m[2][1], m[3][1] ],
           [ m[0][2], m[1][2], m[2][2], m[3][2] ],
           [ m[0][3], m[1][3], m[2][3], m[3][3] ] ]

def matrix_rotate(axis, angle):
  vx  = axis[0]
  vy  = axis[1]
  vz  = axis[2]
  vx2 = vx * vx
  vy2 = vy * vy
  vz2 = vz * vz
  cos = math.cos(angle)
  sin = math.sin(angle)
  co1 = 1.0 - cos
  return [
    [vx2 * co1 + cos,          vx * vy * co1 + vz * sin, vz * vx * co1 - vy * sin, 0.0],
    [vx * vy * co1 - vz * sin, vy2 * co1 + cos,          vy * vz * co1 + vx * sin, 0.0],
    [vz * vx * co1 + vy * sin, vy * vz * co1 - vx * sin, vz2 * co1 + cos,          0.0],
    [0.0, 0.0, 0.0, 1.0],
    ]

def point_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0] + m[3][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1] + m[3][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2] + m[3][2]]

def vector_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2]]

def vector_normalize(v):
  global exportLogger
  l = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
  if l <= 0.000001:
    exportLogger.logError("error in normalize")
    return [0 , l, 0]
  return [v[0] / l, v[1] / l, v[2] / l]

def normal_by_matrix(n, m):
  m = matrix_transpose(matrix_invert(m))
  return vector_normalize(vector_by_matrix(n, m))


def vector_dotproduct(v1, v2):
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]

def vector_crossproduct(v1, v2):
  return [
    v1[1] * v2[2] - v1[2] * v2[1],
    v1[2] * v2[0] - v1[0] * v2[2],
    v1[0] * v2[1] - v1[1] * v2[0],
    ]

#######################################################################################
## data structures
    
class MaterialInterface:
    def getName(self):
        """Returns the material name.
        
           @return Material name.
        """
        return
    def write(self, f):
        """Write material script entry.
        
           @param f Material script file object to write into.
        """
        return

class DefaultMaterial(MaterialInterface):
    def __init__(self, name):
        self.name = name
        return
    def getName(self):
        return self.name
    def write(self, f):
        f.write("material %s\n" % self.getName())
        f.write("{\n")
        self.writeTechniques(f)
        f.write("}\n")
        return
    def writeTechniques(self, f):
        f.write(tab(1) + "technique\n" + tab(1) + "{\n")
        f.write(tab(2) + "pass\n" + tab(2) + "{\n")
        # empty pass
        f.write(tab(2) + "}\n") # pass
        f.write(tab(1) + "}\n") # technique
        return
    
class GameEngineMaterial(DefaultMaterial):
    def __init__(self, blenderMesh, blenderFace):
        self.mesh = blenderMesh
        self.face = blenderFace
        # check if a Blender material is assigned
        try:
            blenderMaterial = self.mesh.materials[self.face.mat]
        except:
            blenderMaterial = None
        self.material = blenderMaterial
        DefaultMaterial.__init__(self, self._createName())
        return
    def writeTechniques(self, f):
        mat = self.material
        if (not(mat)
            and not(self.mesh.vertexColors)
            and not(self.mesh.vertexUV or self.mesh.faceUV)):
            # default material
            DefaultMaterial.writeTechniques(self, f)
        else:
            # default material
            # SOLID, white, no specular
            f.write(tab(1)+"technique\n")
            f.write(tab(1)+"{\n")
            f.write(tab(2)+"pass\n")
            f.write(tab(2)+"{\n")
            # ambient
            # (not used in Blender's game engine)
            if mat:
                if (not(mat.mode & Blender.Material.Modes["TEXFACE"])
                    and not(mat.mode & Blender.Material.Modes["VCOL_PAINT"])
                    and (ambientToggle.val)):
                    ambientRGBList = mat.rgbCol
                else:
                    ambientRGBList = [1.0, 1.0, 1.0]
                # ambient <- amb * ambient RGB
                ambR = clamp(mat.amb * ambientRGBList[0])
                ambG = clamp(mat.amb * ambientRGBList[1])
                ambB = clamp(mat.amb * ambientRGBList[2])
                ##f.write(tab(3)+"ambient %f %f %f\n" % (ambR, ambG, ambB))
            # diffuse
            # (Blender's game engine uses vertex colours
            #  instead of diffuse colour.
            #
            #  diffuse is defined as
            #  (mat->r, mat->g, mat->b)*(mat->emit + mat->ref)
            #  but it's not used.)
            if self.mesh.vertexColors:
                #TODO: Broken in Blender 2.36.
                # Blender does not handle "texface" mesh with vertexcolours
                f.write(tab(3)+"diffuse vertexcolour\n")
            elif mat:
                if (not(mat.mode & Blender.Material.Modes["TEXFACE"])
                    and not(mat.mode & Blender.Material.Modes["VCOL_PAINT"])):
                    # diffuse <- rgbCol
                    diffR = clamp(mat.rgbCol[0])
                    diffG = clamp(mat.rgbCol[1])
                    diffB = clamp(mat.rgbCol[2])
                    f.write(tab(3)+"diffuse %f %f %f\n" % (diffR, diffG, diffB))
                elif (mat.mode & Blender.Material.Modes["VCOL_PAINT"]):
                    f.write(tab(3)+"diffuse vertexcolour\n")
            if mat:
                # specular <- spec * specCol, hard/4.0
                specR = clamp(mat.spec * mat.specCol[0])
                specG = clamp(mat.spec * mat.specCol[1])
                specB = clamp(mat.spec * mat.specCol[2])
                specShine = mat.hard/4.0
                f.write(tab(3)+"specular %f %f %f %f\n" % (specR, specG, specB, specShine))
                # emissive
                # (not used in Blender's game engine)
                if(not(mat.mode & Blender.Material.Modes["TEXFACE"])
                    and not(mat.mode & Blender.Material.Modes["VCOL_PAINT"])):
                    # emissive <-emit * rgbCol
                    emR = clamp(mat.emit * mat.rgbCol[0])
                    emG = clamp(mat.emit * mat.rgbCol[1])
                    emB = clamp(mat.emit * mat.rgbCol[2])
                    ##f.write(tab(3)+"emissive %f %f %f\n" % (emR, emG, emB))
            # scene_blend <- transp
            if (self.face.mode == Blender.NMesh.FaceTranspModes["ALPHA"]):
                f.write(tab(3)+"scene_blend alpha_blend \n")
            elif (self.face.mode == Blender.NMesh.FaceTranspModes["ADD"]):
                #TODO: Broken in Blender 2.36.
                #f.write(tab(3)+"scene_blend add\n")
                pass
            # cull_hardware/cull_software
            if (self.face.mode & Blender.NMesh.FaceModes['TWOSIDE']):
                f.write(tab(3) + "cull_hardware none\n")
                f.write(tab(3) + "cull_software none\n")
            # shading
            # (Blender's game engine is initialized with glShadeModel(GL_FLAT))
            ##f.write(tab(3) + "shading flat\n")
            # texture
            if (self.face.mode & Blender.NMesh.FaceModes['TEX']) and (self.face.image):
                f.write(tab(3)+"texture_unit\n")
                f.write(tab(3)+"{\n")
                f.write(tab(4)+"texture %s\n" % PathName(self.face.image.filename).basename())
                f.write(tab(3)+"}\n") # texture_unit
            f.write(tab(2)+"}\n") # pass
            f.write(tab(1)+"}\n") # technique
        return
    # private
    def _createName(self):
        """Create unique material name.
        
           The name consists of several parts:
           <OL>
           <LI>rendering material name/</LI>
           <LI>blend mode (ALPHA, ADD, SOLID)</LI>
           <LI>/TEX</LI>
           <LI>/texture file name</LI>
           <LI>/VertCol</LI>
           <LI>/TWOSIDE></LI>
           </OL>
        """
        materialName = ''
        # nonempty rendering material?
        if self.material:
            materialName += self.material.getName() + '/'
        # blend mode
        if (self.face.transp == Blender.NMesh.FaceTranspModes['ALPHA']):
            materialName += 'ALPHA'
        elif (self.face.transp == Blender.NMesh.FaceTranspModes['ADD']):
            materialName += 'ADD'
        else:
            materialName += 'SOLID'
        # TEX face mode and texture?
        if (self.face.mode & Blender.NMesh.FaceModes['TEX']):
            materialName += '/TEX'
            if self.face.image:
                materialName += '/' + PathName(self.face.image.filename).basename()
        # vertex colours?
        if self.mesh.vertexColors:
            materialName += '/VertCol'
        # two sided?
        if (self.face.mode & Blender.NMesh.FaceModes['TWOSIDE']):
            materialName += '/TWOSIDE'
        return materialName

class RenderingMaterial(DefaultMaterial):
    def __init__(self, blenderMesh, blenderFace):
        self.mesh = blenderMesh
        self.face = blenderFace
        self.key = 0
        self.mTexUVCol = None
        self.mTexUVNor = None
        self.mTexUVCsp = None
        self.material = self.mesh.materials[self.face.mat]
        if self.material:
            self._generateKey()
            DefaultMaterial.__init__(self, self._createName())
        else:
            DefaultMaterial.__init__(self, 'None')
        return
    def writeTechniques(self, f):
        # parse material
        if self.key:
            if self.TECHNIQUES.has_key(self.key):
                techniques = self.TECHNIQUES[self.key]
                techniques(self, f)
            else:
                # default
                self.writeColours(f)
        else:
            # Halo or empty material
            DefaultMaterial('').writeTechniques(f)
        return
    def writeColours(self, f):
        global ambientToggle
        # receive_shadows
        self.writeReceiveShadows(f, 1)
        f.write(tab(1) + "technique\n" + tab(1) + "{\n")
        f.write(tab(2) + "pass\n" + tab(2) + "{\n")
        # ambient
        if (ambientToggle.val):
            col = self.material.getRGBCol()
        else:
            col = [1.0, 1.0, 1.0]
        self.writeAmbient(f, col, 3)
        # diffuse
        self.writeDiffuse(f, self.material.rgbCol, 3)
        # specular
        self.writeSpecular(f, 3)
        # emissive
        self.writeEmissive(f, self.material.rgbCol, 3)
        # blend mode
        self.writeSceneBlend(f,3)
        # options
        self.writeCommonOptions(f, 3)
        # texture units
        self.writeDiffuseTexture(f, 3)
        f.write(tab(2) + "}\n") # pass
        f.write(tab(1) + "}\n") # technique
        return
    def writeTexFace(self, f):
        # preconditions: TEXFACE set
        # 
        # Note that an additional Col texture replaces the
        # TEXFACE texture instead of blend over according to alpha.
        #
        # (amb+emit)textureCol + diffuseLight*ref*textureCol + specular
        # 
        imageFileName = None
        if self.mTexUVCol:
            # COL MTex replaces UV/Image Editor texture
            imageFileName = PathName(self.mTexUVCol.tex.getImage().getFilename()).basename()
        elif self.face.image:
            # UV/Image Editor texture 
            imageFileName = PathName(self.face.image.filename).basename()
        
        self.writeReceiveShadows(f, 1)
        f.write(tab(1) + "technique\n" + tab(1) + "{\n")
        col = [1.0, 1.0, 1.0]
        # texture pass
        f.write(tab(2) + "pass\n" + tab(2) + "{\n")
        self.writeAmbient(f, col, 3)
        self.writeDiffuse(f, col, 3)
        if not(imageFileName):
            self.writeSpecular(f, 3)
        self.writeEmissive(f, col, 3)
        self.writeSceneBlend(f,3)
        self.writeCommonOptions(f, 3)
        if imageFileName:
            f.write(tab(3) + "texture_unit\n")
            f.write(tab(3) + "{\n")
            f.write(tab(4) + "texture %s\n" % imageFileName)
            if self.mTexUVCol:
                self.writeTextureAddressMode(f, self.mTexUVCol, 4)
                self.writeTextureFiltering(f, self.mTexUVCol, 4)
            # multiply with factors
            f.write(tab(4) + "colour_op modulate\n")
            f.write(tab(3) + "}\n") # texture_unit
            f.write(tab(2) + "}\n") # texture pass
            # specular pass
            f.write(tab(2) + "pass\n" + tab(2) + "{\n")
            f.write(tab(3) + "ambient 0.0 0.0 0.0\n")
            f.write(tab(3) + "diffuse 0.0 0.0 0.0\n")
            self.writeSpecular(f, 3)
            f.write(tab(3) + "scene_blend add\n")
            hasAlpha = 0
            if (self.material.getAlpha() < 1.0):
                hasAlpha = 1
            else:
                for mtex in self.material.getTextures():
                    if mtex:
                        if ((mtex.tex.type == Blender.Texture.Types['IMAGE'])
                            and (mtex.mapto & Blender.Texture.MapTo['ALPHA'])):
                            hasAlpha = 1
            if (hasAlpha):
                f.write(tab(3) + "depth_write off\n")
            self.writeCommonOptions(f, 3)
        f.write(tab(2) + "}\n") # pass
        f.write(tab(1) + "}\n") # technique
        return
    def writeVertexColours(self, f):
        # preconditions: VCOL_PAINT set
        #
        # ambient = Amb*White resp. Amb*VCol if "Coloured Ambient"
        # diffuse = Ref*VCol
        # specular = Spec*SpeRGB, Hard/4.0
        # emissive = Emit*VCol
        # alpha = A
        # 
        # Best match without vertex shader:
        # ambient = Amb*white
        # diffuse = Ref*VCol
        # specular = Spec*SpeRGB, Hard/4.0
        # emissive = black
        # alpha = 1
        #
        self.writeReceiveShadows(f, 1)
        f.write(tab(1) + "technique\n" + tab(1) + "{\n")
        if (self.material.mode & Blender.Material.Modes['SHADELESS']):
            f.write(tab(2) + "pass\n" + tab(2) + "{\n")
            self.writeCommonOptions(f,3)
            f.write(tab(2) + "}\n")
        else:
            # vertex colour pass
            f.write(tab(2) + "pass\n" + tab(2) + "{\n")
            f.write(tab(3) + "ambient 0.0 0.0 0.0\n")
            f.write(tab(3) + "diffuse vertexcolour\n")
            self.writeCommonOptions(f, 3)
            f.write(tab(2) + "}\n") # vertex colour pass
            
            # factor pass
            f.write(tab(2) + "pass\n" + tab(2) + "{\n")
            f.write(tab(3) + "ambient 0.0 0.0 0.0\n")
            ref = self.material.getRef()
            f.write(tab(3) + "diffuse %f %f %f\n" % (ref, ref, ref))
            f.write(tab(3) + "scene_blend modulate\n")
            self.writeCommonOptions(f, 3)
            f.write(tab(2) + "}\n") # factor pass
            
            # ambient and specular pass
            f.write(tab(2) + "pass\n" + tab(2) + "{\n")
            self.writeAmbient(f, [1.0, 1.0, 1.0], 3)
            f.write(tab(3) + "diffuse 0.0 0.0 0.0\n")
            self.writeSpecular(f, 3)
            f.write(tab(3) + "scene_blend add\n")
            self.writeCommonOptions(f, 3)
            f.write(tab(2) + "}\n") # specular pass
        
        f.write(tab(1) + "}\n") # technique
        return
    def writeNormalMap(self, f):
        # preconditions COL and NOR textures
        colImage = PathName(self.mTexUVCol.tex.image.filename).basename()
        norImage = PathName(self.mTexUVNor.tex.image.filename).basename()
        f.write("""    technique
    {
        pass
        {
            ambient 1 1 1
            diffuse 0 0 0 
            specular 0 0 0 0
            vertex_program_ref Ogre/BasicVertexPrograms/AmbientOneTexture
            {
                param_named_auto worldViewProj worldviewproj_matrix
                param_named_auto ambient ambient_light_colour
            }
        }
        pass
        {
            ambient 0 0 0 
            iteration once_per_light
            scene_blend add
            vertex_program_ref Examples/BumpMapVPSpecular
            {
                param_named_auto lightPosition light_position_object_space 0
                param_named_auto eyePosition camera_position_object_space
                param_named_auto worldViewProj worldviewproj_matrix
            }
            fragment_program_ref Examples/BumpMapFPSpecular
            {
                param_named_auto lightDiffuse light_diffuse_colour 0 
                param_named_auto lightSpecular light_specular_colour 0
            }
            texture_unit
            {
                texture %s
                colour_op replace
            }
            texture_unit
            {
                cubic_texture nm.png combinedUVW
                tex_coord_set 1
                tex_address_mode clamp
            }
            texture_unit
            {
                cubic_texture nm.png combinedUVW
                tex_coord_set 2
                tex_address_mode clamp
            }
        }
        pass
        {
            lighting off
            vertex_program_ref Ogre/BasicVertexPrograms/AmbientOneTexture
            {
                param_named_auto worldViewProj worldviewproj_matrix
                param_named ambient float4 1 1 1 1
            }
            scene_blend dest_colour zero
            texture_unit
            {
                texture %s
            }
        }
    }
    technique
    {
        pass
        {
            ambient 1 1 1
            diffuse 0 0 0 
            specular 0 0 0 0
            vertex_program_ref Ogre/BasicVertexPrograms/AmbientOneTexture
            {
                param_named_auto worldViewProj worldviewproj_matrix
                param_named_auto ambient ambient_light_colour
            }
        }
        pass
        {
            ambient 0 0 0 
            iteration once_per_light
            scene_blend add
            vertex_program_ref Examples/BumpMapVP
            {
                param_named_auto lightPosition light_position_object_space 0
                param_named_auto eyePosition camera_position_object_space
                param_named_auto worldViewProj worldviewproj_matrix
            }
            texture_unit
            {
                texture %s
                colour_op replace
            }
            texture_unit
            {
                cubic_texture nm.png combinedUVW
                tex_coord_set 1
                tex_address_mode clamp
                colour_op_ex dotproduct src_texture src_current
                colour_op_multipass_fallback dest_colour zero
            }
        }
        pass
        {
            lighting off
            vertex_program_ref Ogre/BasicVertexPrograms/AmbientOneTexture
            {
                param_named_auto worldViewProj worldviewproj_matrix
                param_named ambient float4 1 1 1 1
            }
            scene_blend dest_colour zero
            texture_unit
            {
                texture %s
            }
        }
    }
""" % (norImage, colImage, norImage, colImage))    
        return
    def writeReceiveShadows(self, f, indent=0):
        if (self.material.mode & Blender.Material.Modes["SHADOW"]):
            f.write(tab(indent)+"receive_shadows on\n")
        else:
            f.write(tab(indent)+"receive_shadows off\n")
        return
    def writeAmbient(self, f, col, indent=0):
        # ambient <- amb * ambient RGB
        ambR = clamp(self.material.getAmb() * col[0])
        ambG = clamp(self.material.getAmb() * col[1])
        ambB = clamp(self.material.getAmb() * col[2])
        if len(col) < 4:
            alpha = self.material.getAlpha()
        else:
            alpha = col[3]
        f.write(tab(indent)+"ambient %f %f %f %f\n" % (ambR, ambG, ambB, alpha))
        return
    def writeDiffuse(self, f, col, indent=0):
        # diffuse = reflectivity*colour
        diffR = clamp(col[0] * self.material.getRef())
        diffG = clamp(col[1] * self.material.getRef())
        diffB = clamp(col[2] * self.material.getRef())
        if len(col) < 4:
            alpha = self.material.getAlpha()
        else:
            alpha = col[3]
        f.write(tab(indent)+"diffuse %f %f %f %f\n" % (diffR, diffG, diffB, alpha))
        return
    def writeSpecular(self, f, indent=0):
        # specular <- spec * specCol, hard/4.0
        specR = clamp(self.material.getSpec() * self.material.getSpecCol()[0])
        specG = clamp(self.material.getSpec() * self.material.getSpecCol()[1])
        specB = clamp(self.material.getSpec() * self.material.getSpecCol()[2])
        specShine = self.material.getHardness()/4.0
        alpha = self.material.getAlpha()
        f.write(tab(indent)+"specular %f %f %f %f %f\n" % (specR, specG, specB, alpha, specShine))
        return
    def writeEmissive(self, f, col, indent=0):
        # emissive <-emit * rgbCol
        emR = clamp(self.material.getEmit() * col[0])
        emG = clamp(self.material.getEmit() * col[1])
        emB = clamp(self.material.getEmit() * col[2])
        if len(col) < 4:
            alpha = self.material.getAlpha()
        else:
            alpha = col[3]
        f.write(tab(indent)+"emissive %f %f %f %f\n" % (emR, emG, emB, alpha))
        return
    def writeSceneBlend(self, f, indent=0):
        hasAlpha = 0
        if (self.material.getAlpha() < 1.0):
            hasAlpha = 1
        else:
            for mtex in self.material.getTextures():
                if mtex:
                    if ((mtex.tex.type == Blender.Texture.Types['IMAGE'])
                        and (mtex.mapto & Blender.Texture.MapTo['ALPHA'])):
                        hasAlpha = 1
        if (hasAlpha):
            f.write(tab(indent) + "scene_blend alpha_blend\n")
            f.write(tab(indent) + "depth_write off\n")
        return
    def writeCommonOptions(self, f, indent=0):
        # Shadeless, ZInvert, NoMist, Env
        # depth_func  <- ZINVERT; ENV
        if (self.material.mode & Blender.Material.Modes['ENV']):
            f.write(tab(indent)+"depth_func always_fail\n")
        elif (self.material.mode & Blender.Material.Modes['ZINVERT']):
            f.write(tab(indent)+"depth_func greater_equal\n")
        # twoside
        # 2.4 raises an exception if face.mode is accessed and mesh has no uv
        try:
            if (self.face.mode & Blender.NMesh.FaceModes['TWOSIDE']):
                f.write(tab(3) + "cull_hardware none\n")
                f.write(tab(3) + "cull_software none\n")
        except:
            pass
        # lighting <- SHADELESS
        if (self.material.mode & Blender.Material.Modes['SHADELESS']):
            f.write(tab(indent)+"lighting off\n")
        # fog_override <- NOMIST
        if (self.material.mode & Blender.Material.Modes['NOMIST']):
            f.write(tab(indent)+"fog_override true\n")
        return
    def writeDiffuseTexture(self, f, indent = 0):
        if self.mTexUVCol:
            f.write(tab(indent)+"texture_unit\n")
            f.write(tab(indent)+"{\n")
            f.write(tab(indent + 1) + "texture %s\n" % PathName(self.mTexUVCol.tex.getImage().getFilename()).basename())
            self.writeTextureAddressMode(f, self.mTexUVCol, indent + 1)
            self.writeTextureFiltering(f, self.mTexUVCol, indent + 1)            
            self.writeTextureColourOp(f, self.mTexUVCol, indent + 1)
            f.write(tab(indent)+"}\n") # texture_unit
        return
    def writeTextureAddressMode(self, f, blenderMTex, indent = 0):
        # tex_address_mode inside texture_unit
        #
        # EXTEND   | clamp 
        # CLIP     |
        # CLIPCUBE | 
        # REPEAT   | wrap
        #
        if (blenderMTex.tex.extend & Blender.Texture.ExtendModes['REPEAT']):
            f.write(tab(indent) + "tex_address_mode wrap\n")
        elif (blenderMTex.tex.extend & Blender.Texture.ExtendModes['EXTEND']):
            f.write(tab(indent) + "tex_address_mode clamp\n")        
        return
    def writeTextureFiltering(self, f, blenderMTex, indent = 0):
        # filtering inside texture_unit
        #
        # InterPol | MidMap | filtering
        # ---------+--------+----------
        #    yes   |   yes  | trilinear
        #    yes   |   no   | linear linear none
        #    no    |   yes  | bilinear
        #    no    |   no   | none
        #
        if (blenderMTex.tex.imageFlags & Blender.Texture.ImageFlags['INTERPOL']):
            if (blenderMTex.tex.imageFlags & Blender.Texture.ImageFlags['MIPMAP']):
                f.write(tab(indent) + "filtering trilinear\n")
            else:
                f.write(tab(indent) + "filtering linear linear none\n")
        else:
            if (blenderMTex.tex.imageFlags & Blender.Texture.ImageFlags['MIPMAP']):
                f.write(tab(indent) + "filtering bilinear\n")
            else:
                f.write(tab(indent) + "filtering none\n")
        return
    def writeTextureColourOp(self, f, blenderMTex, indent = 0):
        # colour_op inside texture_unit
        if ((blenderMTex.tex.imageFlags & Blender.Texture.ImageFlags['USEALPHA'])
            and not(blenderMTex.mapto & Blender.Texture.MapTo['ALPHA'])):
            f.write(tab(indent) + "colour_op alpha_blend\n")
        return
    # private
    def _createName(self):
        # must be called after _generateKey()
        materialName = self.material.getName()
        # two sided?
        try:
            if (self.face.mode & Blender.Mesh.FaceModes['TWOSIDE']):
                materialName += '/TWOSIDE'
        except:
            pass
        # use UV/Image Editor texture?
        if ((self.key & self.TEXFACE) and not(self.key & self.IMAGEUVCOL)):
            materialName += '/TEXFACE'
            if self.face.image:
                materialName += '/' + PathName(self.face.image.filename).basename()
        return materialName
    def _generateKey(self):
        # generates key and populates mTex fields
        if self.material:
            if not(self.material.mode & Blender.Material.Modes['HALO']):
                self.key |= self.NONHALO
                if (self.material.mode & Blender.Material.Modes['VCOL_LIGHT']):
                    self.key |= self.VCOLLIGHT
                if (self.material.mode & Blender.Material.Modes['VCOL_PAINT']):
                    self.key |= self.VCOLPAINT
                if (self.material.mode & Blender.Material.Modes['TEXFACE']):
                    self.key |= self.TEXFACE
                # textures
                for mtex in self.material.getTextures():
                    if mtex:
                        if (mtex.tex.type == Blender.Texture.Types['IMAGE']):
                            if (mtex.texco & Blender.Texture.TexCo['UV']):
                                if (mtex.mapto & Blender.Texture.MapTo['COL']):
                                    self.key |= self.IMAGEUVCOL
                                    self.mTexUVCol = mtex
                                if (mtex.mapto & Blender.Texture.MapTo['NOR']):
                                    # Check "Normal Map" image option
                                    if (mtex.tex.imageFlags & 2048):
                                        self.key |= self.IMAGEUVNOR
                                        self.mTexUVNor = mtex
                                    # else bumpmap
                                if (mtex.mapto & Blender.Texture.MapTo['CSP']):
                                    self.key |= self.IMAGEUVCSP
                                    self.mTexUVCsp = mtex
        return
    NONHALO = 1
    VCOLLIGHT = 2
    VCOLPAINT = 4
    TEXFACE = 8
    IMAGEUVCOL = 16
    IMAGEUVNOR = 32
    IMAGEUVCSP = 64
    # material techniques export methods
    TECHNIQUES = {
        NONHALO|IMAGEUVCOL : writeColours,
        NONHALO|IMAGEUVCOL|IMAGEUVCSP : writeColours,
        NONHALO|TEXFACE : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT : writeTexFace,
        NONHALO|TEXFACE|IMAGEUVCOL : writeTexFace,
        NONHALO|TEXFACE|IMAGEUVNOR : writeTexFace,
        NONHALO|TEXFACE|IMAGEUVCSP : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT|IMAGEUVCOL : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT|IMAGEUVNOR : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT|IMAGEUVCSP : writeTexFace,
        NONHALO|TEXFACE|IMAGEUVCOL|IMAGEUVCSP : writeTexFace,
        NONHALO|TEXFACE|IMAGEUVNOR|IMAGEUVCSP : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT|IMAGEUVCOL|IMAGEUVCSP : writeTexFace,
        NONHALO|TEXFACE|VCOLLIGHT|IMAGEUVNOR|IMAGEUVCSP : writeTexFace,
        NONHALO|VCOLPAINT : writeVertexColours,
        NONHALO|VCOLPAINT|VCOLLIGHT : writeVertexColours,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLPAINT : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|TEXFACE : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|VCOLPAINT : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|TEXFACE : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLPAINT|TEXFACE : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLPAINT|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|TEXFACE|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLPAINT|TEXFACE|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|TEXFACE|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|VCOLPAINT|IMAGEUVCSP : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|VCOLPAINT|TEXFACE : writeNormalMap,
        NONHALO|IMAGEUVCOL|IMAGEUVNOR|VCOLLIGHT|VCOLPAINT|TEXFACE|IMAGEUVCSP : writeNormalMap
        }

class Mesh:
    def __init__(self, submeshList, skeleton=None, nmesh=None):
        """Constructor.
        
           @param submeshList Submeshes.
           @param nmesh Blender NMesh.
        """
        self.name = ''
        self.submeshList = submeshList
        self.skeleton = skeleton
        # boolean
        self.vertexColours = 0
        # boolean
        self.uvCoordinates = 0
        # parse nmesh
        self._parseMesh(nmesh)
        return
    def hasVertexColours(self):
        return self.vertexColours
    def hasUVCoordinates(self):
        return self.uvCoordinates
    def write(self):
        # write_mesh(name, submeshes, skeleton):
        global pathString, exportLogger
        file = self.name + ".mesh.xml"
        exportLogger.logInfo("Mesh \"%s\"" % file)
        
        f = open(os.path.join(pathString.val, file), "w")
        f.write(tab(0)+"<mesh>\n")
        f.write(tab(1)+"<submeshes>\n")
        for submesh in self.submeshList:
            f.write(tab(2)+"<submesh")
            f.write(" material=\"%s\"" % submesh.material.name)
            f.write(" usesharedvertices=\"false\"")
            f.write(" use32bitindexes=\"false\"")
            f.write(" operationtype=\"triangle_list\"")
            f.write(">\n")
            
            f.write(tab(3)+"<faces count=\"%d\">\n" % len(submesh.faces))
            for face in submesh.faces:
                v1, v2, v3  = face.vertex1.id, face.vertex2.id, face.vertex3.id
                f.write(tab(4)+"<face v1=\"%d\" v2=\"%d\" v3=\"%d\"/>\n" % (v1, v2, v3))
            f.write(tab(3)+"</faces>\n")
    
            f.write(tab(3)+"<geometry vertexcount=\"%d\">\n" % len(submesh.vertices))
            if (armatureToggle.val):
                # use seperate vertexbuffer for position and normals when animated
                f.write(tab(4)+"<vertexbuffer positions=\"true\" normals=\"true\">\n")
                for v in submesh.vertices:
                    f.write(XMLVertexStringView(v.xmlVertex).toString(5, ['normal','position']))
                f.write(tab(4)+"</vertexbuffer>\n")
                if (self.hasUVCoordinates() and self.hasVertexColours()):
                    f.write(tab(4)+"<vertexbuffer")
                    f.write(" texture_coord_dimensions_0=\"2\" texture_coords=\"1\"")
                    f.write(" colours_diffuse=\"true\">\n")
                    for v in submesh.vertices:
                            f.write(XMLVertexStringView(v.xmlVertex).toString(5, ['texcoordList','colourDiffuse']))
                    f.write(tab(4)+"</vertexbuffer>\n")
                elif self.hasUVCoordinates():
                    f.write(tab(4)+"<vertexbuffer")
                    f.write(" texture_coord_dimensions_0=\"2\" texture_coords=\"1\">\n")
                    for v in submesh.vertices:
                            f.write(XMLVertexStringView(v.xmlVertex).toString(5, ['texcoordList']))
                    f.write(tab(4)+"</vertexbuffer>\n")
                elif self.hasVertexColours():
                    f.write(tab(4)+"<vertexbuffer")
                    f.write(" colours_diffuse=\"true\">\n")
                    for v in submesh.vertices:
                            f.write(XMLVertexStringView(v.xmlVertex).toString(5, ['colourDiffuse']))
                    f.write(tab(4)+"</vertexbuffer>\n")
            else:
                # use only one vertex buffer if mesh is not animated
                f.write(tab(4)+"<vertexbuffer ")
                f.write("positions=\"true\" ")
                f.write("normals=\"true\"")
                if self.hasUVCoordinates():
                    f.write(" texture_coord_dimensions_0=\"2\" texture_coords=\"1\"")
                if self.hasVertexColours():
                    f.write(" colours_diffuse=\"true\"")
                f.write(">\n")
                for v in submesh.vertices:
                    f.write(XMLVertexStringView(v.xmlVertex).toString(5))
                f.write(tab(4)+"</vertexbuffer>\n")
            f.write(tab(3)+"</geometry>\n")
        
            if self.skeleton:
                f.write(tab(3)+"<boneassignments>\n")
                for v in submesh.vertices:
                    for influence in v.influences:
                        f.write(tab(4)+"<vertexboneassignment ")
                        f.write("vertexindex=\"%d\" boneindex=\"%d\" weight=\"%.6f\"/>\n"
                            % (v.id, influence.bone.id, influence.weight))
                f.write(tab(3)+"</boneassignments>\n")
            f.write(tab(2)+"</submesh>\n")
        f.write(tab(1)+"</submeshes>\n")
    
        if self.skeleton:
            f.write(tab(1)+"<skeletonlink name=\"%s.skeleton\"/>\n" % self.skeleton.name) 
    
        f.write(tab(0)+"</mesh>\n")    
        f.close()
        convertXMLFile(os.path.join(pathString.val, file))
        return
    # private
    def _parseMesh(self, mesh):
        if mesh:
            self.name = mesh.name
            if mesh.vertexColors:
                self.vertexColors = 1
            if (mesh.faceUV or mesh.vertexUV):
                self.uvCoordinates = 1
        return

class SubMesh:
  def __init__(self, material):
    self.material   = material
    self.vertices   = []
    self.faces      = []

  def rename_vertices(self, new_vertices):
    # Rename (change ID) of all vertices, such as self.vertices == new_vertices.
    for i in range(len(new_vertices)): new_vertices[i].id = i
    self.vertices = new_vertices

class XMLVertex:
    """Vertex in Ogre.
    
       @cvar threshold Floating point precicsion.
    """
    threshold = 1e-6
    def __init__(self, position=None, normal=None, colourDiffuse=None, colourSpecular=None, texcoordList=None):
        """Constructor.
        
           @param position       list with x, y and z coordinate of the position
           @param normal         list with x, y and z coordinate of the normal vector
           @param colourDiffuse  list with RGBA floats
           @param colourSpecular list with RGBA floats
           @param texcoordList   list of list with u and v texture coordinates.
        """
        self.elementDict = {}
        if position:
            self.elementDict['position'] = position
        if normal:
            self.elementDict['normal'] = normal
        if colourDiffuse:
            self.elementDict['colourDiffuse'] = colourDiffuse
        if colourSpecular:
            self.elementDict['colourSpecular'] = colourSpecular
        if texcoordList:
            self.elementDict['texcoordList'] = texcoordList
        return
    def hasPosition(self):
        return self.elementDict.has_key('position')
    def hasNormal(self):
        return self.elementDict.has_key('normal')
    def hasVertexColour(self):
        return self.elementDict.has_key('colourDiffuse') or self.elementDict.has_key('colourSpecular')
    def hasDiffuseColour(self):
        return self.elementDict.has_key('colourDiffuse')
    def hasSpecularColour(self):
        return self.elementDict.has_key('colourSpecular')
    def nTextureCoordinates(self):
        nTextureCoordinates = 0
        if self.elementDict.has_key('texcoordList'):
            nTextureCoordinates = len(self.elementDict['texcoordList'])
        return nTextureCoordinates
    def __getitem__(self, key):
        return self.elementDict[key]
    def __ne__(self, other):
        """Tests if it differs from another Vertex.
           
           @param other the XMLVertex to compare with
           @return <code>true</code> if they differ, else <code>false</code>
        """
        return not self.__eq__(other)
    def __eq__(self, other):
        """Tests if it is equal to another Vertex.
        
           @param other the XMLVertex to compare with
           @return <code>true</code> if they are equal, else <code>false</code>
        """
        areEqual = 0
        if (self.getElements() == other.getElements()):
            compared = 0
            itemIterator = self.elementDict.iteritems()
            while (not compared):
                try:
                    (element, value) = itemIterator.next()
                    if element == 'position' or element == 'normal':
                        otherValue = other[element]
                        if ((math.fabs(value[0] - otherValue[0]) > XMLVertex.threshold) or
                            (math.fabs(value[1] - otherValue[1]) > XMLVertex.threshold) or
                            (math.fabs(value[2] - otherValue[2]) > XMLVertex.threshold)):
                            # test fails
                            compared = 1
                    elif element == 'colourDiffuse' or element == 'colourSpecular':
                        otherValue = other[element]
                        if ((math.fabs(value[0] - otherValue[0]) > XMLVertex.threshold) or
                            (math.fabs(value[1] - otherValue[1]) > XMLVertex.threshold) or
                            (math.fabs(value[2] - otherValue[2]) > XMLVertex.threshold) or
                            (math.fabs(value[3] - otherValue[3]) > XMLVertex.threshold)):
                            # test fails
                            compared = 1
                    elif element == 'texcoordList':
                        otherValue = other[element]
                        if len(value) == len(otherValue):
                            for uv, otherUV in zip(value, otherValue):
                                if ((math.fabs(uv[0] - otherUV[0]) > XMLVertex.threshold) or
                                    (math.fabs(uv[1] - otherUV[1]) > XMLVertex.threshold)):
                                    # test fails
                                    compared = 1
                        else:
                            # test fails
                            compared = 1
                    else:
                        # test fails, unknown element
                        compared = 1
                except StopIteration:
                    # objects are equal
                    areEqual = 1
                    compared = 1
        return areEqual
    # getter and setter
    def getElements(self):
        return self.elementDict.keys()
    def getPosition(self):
        return self.elementDict['position']
    def getNormal(self):
        return self.elementDict['normal']
    def getColourDiffuse(self):
        return self.elementDict['colourDiffuse']
    def getColourSpecular(self):
        return self.elementDict['colourSpecular']
    def getTextureCoordinatesList(self):
        return self.elementDict['texcoordList']
    def setPosition(self, position):
        if position:
            self.elementDict['position'] = position
        else:
            del self.elementDict['position']
        return
    def setNormal(self, normal):
        if normal:
            self.elementDict['normal'] = normal
        else:
            del self.elementDict['normal']
        return
    def setColourDiffuse(self, colourDiffuse):
        if colourDiffuse:
            self.elementDict['colourDiffuse'] = colourDiffuse
        else:
            del self.colourDiffuse
        return
    def setColourSpecular(self, colourSpecular):
        if colourSpecular:
            self.elementDict['colourSpecular'] = colourSpecular
        else:
            del self.elementDict['colourSpecular']
        return
    # special getter and setter
    def appendTextureCoordinates(self, uvList):
        """Appends new texture coordinate.
        
           @param uvList list with u and v coordinate
           @return list index
        """
        if self.elementDict.has_key('texcoordList'):
            self.elementDict['texcoordList'].append(uvList)
        else:
            self.elementDict['texcoordList'] = [uvList]
        return (len(self.elementDict['texcoordList']) -1 )
    def setTextureCorrdinates(self, index, uvList):
        self.elementDict['texcoordList'][index] = uvList
        return
    def getTextureCoordinates(self, index=None):
        return self.elementDict['texcoordList'][index]
    def deleteTextureCoordinates(self, index=None):
        """Delete texture coordinates.
        
           Delete a pair or all texture coordinates of the vertex.
           
           @param index the index of the texture coordinates in the vertex's list of
                        texture coordinates. If <code>None</code> the complete list
                        is deleted.
        """
        if (index != None):
            del self.elementDict['texcoordList'][index]
        else:
            del self.elementDict['texcoordList']
        return

class XMLVertexStringView:
    """Viewer class for textual representation of a XMLVertex. 
    
       @see XMLVertex
    """
    def __init__(self, xmlVertex):
        if isinstance(xmlVertex, XMLVertex):
            self.xmlVertex = xmlVertex
        return
    def __str__(self):
        return self.toString()
    def toString(self, indent=0, keyList=None):
        """Returns textual representations of its XMLVertex.
        
           @param indent Indentation level of the string.
           @param keyList List of keys of elements to represent in the string.
           @return string String representation of the XMLVertex.
           @see XMLVertex#__init__
        """
        if not keyList:
            keyList = self.xmlVertex.getElements()
        else:
            # remove unavailable elements
            keyList = [key for key in keyList if key in self.xmlVertex.getElements()]
        s = self._indent(indent) + "<vertex>\n"
        if keyList.count('position'):
            position = self.xmlVertex.getPosition()
            s += self._indent(indent+1)+"<position x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % tuple(position)
        if keyList.count('normal'):
            normal = self.xmlVertex.getNormal()
            s += self._indent(indent+1)+"<normal x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n" % tuple(normal)
        if keyList.count('colourDiffuse'):
            colourDiffuse = self.xmlVertex.getColourDiffuse()
            if OGRE_OPENGL_VERTEXCOLOUR:
                (r, g, b, a) = tuple(colourDiffuse)
                s += self._indent(indent+1)+"<colour_diffuse value=\"%.6f %.6f %.6f %.6f\"/>\n" % (b, g, r, a)
            else:
                s += self._indent(indent+1)+"<colour_diffuse value=\"%.6f %.6f %.6f %.6f\"/>\n" % tuple(colourDiffuse)
        if keyList.count('colourSpecular'):
            colourSpecular = self.xmlVertex.getColourSpecular()
            if OGRE_OPENGL_VERTEXCOLOUR:
                (r, g, b, a) = tuple(colourSpecular)
                s += self._indent(indent+1)+"<colour_specular value=\"%.6f %.6f %.6f %.6f\"/>\n" % (b, g, r, a)
            else:
                s += self._indent(indent+1)+"<colour_specular value=\"%.6f %.6f %.6f %.6f\"/>\n" % tuple(colourSpecular)
        if keyList.count('texcoordList'):
            for uv in self.xmlVertex.getTextureCoordinatesList():
                s+=self._indent(indent+1)+"<texcoord u=\"%.6f\" v=\"%.6f\"/>\n" % tuple(uv)
        s += self._indent(indent) + "</vertex>\n"
        return s
    def _indent(self, indent):
        return "    "*indent

class Vertex:
  def __init__(self, submesh, xmlVertex):
    self.xmlVertex = xmlVertex
    self.influences = []
    
    self.cloned_from = None
    self.clones      = []
    self.submesh = submesh
    self.id = len(submesh.vertices)
    submesh.vertices.append(self)

class Influence:
  def __init__(self, bone, weight):
    self.bone   = bone
    self.weight = weight
    
class Face:
  def __init__(self, submesh, vertex1, vertex2, vertex3):
    self.vertex1 = vertex1
    self.vertex2 = vertex2
    self.vertex3 = vertex3
    self.submesh = submesh
    submesh.faces.append(self)

class Skeleton:
  def __init__(self, name):
    self.name = name
    self.bones = []
    self.bonesDict = {}
    self.animationsDict = {}

class Bone:
  def __init__(self, skeleton, parent, name, loc, rotQuat, conversionMatrix):
    self.parent = parent
    self.name   = name
    self.loc = loc # offset from parent bone
    self.rotQuat = rotQuat # axis as quaternion
    self.children = []
    self.conversionMatrix = conversionMatrix # converts Blender's local bone coordinates into Ogre's local bone coordinates

    if parent:
      parent.children.append(self)
    
    self.id = len(skeleton.bones)
    skeleton.bones.append(self)
    skeleton.bonesDict[name] =self

class Animation:
  def __init__(self, name, duration = 0.0):
    self.name     = name
    self.duration = duration
    self.tracksDict = {} # Map bone names to tracks
    
class Track:
  def __init__(self, animation, bone):
    self.bone      = bone
    self.keyframes = []
    
    self.animation = animation
    animation.tracksDict[bone.name] = self
    
class KeyFrame:
  def __init__(self, track, time, loc, rotQuat, scale):
    self.time = time
    self.loc  = loc
    self.rotQuat  = rotQuat
    self.scale = scale
    
    self.track = track
    track.keyframes.append(self)

#######################################################################################
## Armature stuff

def blender_bone2matrix(head, tail, roll):
  # Convert bone rest state (defined by bone.head, bone.tail and bone.roll)
  # to a matrix (the more standard notation).
  # Taken from blenkernel/intern/armature.c in Blender source.
  # See also DNA_armature_types.h:47.
  
  nor = vector_normalize([ tail[0] - head[0],
                           tail[1] - head[1],
                           tail[2] - head[2] ])

  # Find Axis & Amount for bone matrix
  target = [0.0, 1.0, 0.0]
  axis = vector_crossproduct(target, nor)
  
  # is nor a multiple of target?
  if vector_dotproduct(axis, axis) > 0.0000000000001:
    axis  = vector_normalize(axis)
    theta = math.acos(vector_dotproduct(target, nor))
    bMatrix = matrix_rotate(axis, theta)

  else:
    # point same direction, or opposite?
    if vector_dotproduct(target, nor) > 0.0:
      updown = 1.0    
    else:
      updown = -1.0
    
    # Quoted from Blender source : "I think this should work ..."
    bMatrix = [ [updown,    0.0, 0.0, 0.0],
                [   0.0, updown, 0.0, 0.0],
                [   0.0,    0.0, 1.0, 0.0],
                [   0.0,    0.0, 0.0, 1.0] ]

  rMatrix = matrix_rotate(nor, roll)
  return matrix_multiply(rMatrix, bMatrix)

#######################################################################################
## Mesh stuff
def process_vert_influences(mesh, skeleton):
    global verticesDict
    global exportLogger
    
    if skeleton:
        # build bone influence dictionary
        
        # get the bone names
        # build influences tuples of bone name and influence
        boneWeightDict = {}
        for bone in skeleton.bones:
            boneweights = []
            try:
                #boneweights = mesh.getVertsFromGroup(bone.name, 1, [face.v[i].index])
                # get all weights for a bone.  getting weights for a specific vertex trashes
                # blender after a few hundred calls
                #print bone.name
                boneweights = mesh.getVertsFromGroup(bone.name, 1)
                boneWeightDict[bone.name] = dict(boneweights)
            except:
                pass

        for vert_index, vertex in verticesDict.iteritems():
            influences = []
            for boneName, vertWeightDict in boneWeightDict.iteritems():
                if vertWeightDict.has_key(vert_index):
                    influences += [(boneName, vertWeightDict[vert_index])]
            if not influences:
                exportLogger.logError("Vertex in skinned mesh without influence, check your mesh!")
            # limit influences to 4 bones per vertex
            def cmpfunc(x, y):
                xname, xweight = x
                yname, yweight = y
                return cmp(yweight, xweight)
            # only want the weights with the most influence
            influences.sort(cmpfunc)
            # limits weights to maximum of 4
            influences = influences[0:4]
            # and make sure the sum is 1.0
            total = 0.0
            for name, weight in influences:
                total += weight
            for name, weight in influences:
                vertex.influences.append(Influence(skeleton.bonesDict[name], weight/total))
    
# remap vertices for faces
def process_face(face, submesh, mesh, matrix, skeleton=None):
    """Process a face of a mesh.
    
       @param face Blender.NMesh.NMFace.
       @param submesh SubMesh the face belongs to.
       @param mesh Blender.Mesh.Mesh the face belongs to.
       @param matrix Export translation.
       @param skeleton Skeleton of the mesh (if any).
    """
    global verticesDict
    global exportLogger
    # threshold to compare floats
    threshold = 1e-6
    if len(face.v) in [ 3, 4 ]:
        if not face.smooth:
            # calculate the face normal.
            p1 = face.v[0].co
            p2 = face.v[1].co
            p3 = face.v[2].co
            faceNormal = vector_crossproduct(
                [p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]],
                [p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]],
                )
            faceNormal = normal_by_matrix(faceNormal, matrix)

        face_vertices = [ 0, 0, 0, 0]
        for i in range(len(face.v)):
            # position
            position  = point_by_matrix (face.v[i].co, matrix)
            # Blender separates normal, uv coordinates and colour from vertice coordinates.
            # normal
            if face.smooth:
                normal = normal_by_matrix(face.v[i].no, matrix)
            else:
                normal = faceNormal
            xmlVertex = XMLVertex(position, normal)
            # uv coordinates
            if (mesh.vertexUV or mesh.faceUV):
                uv = [0,0]
                if mesh.vertexUV:
                    # mesh has sticky/per vertex uv coordinates
                    uv[0] = face.v[i].uvco[0]
                    # origin is now in the top-left (Ogre v0.13.0)
                    uv[1] = 1 - face.v[i].uvco[1]
                else:
                    # mesh has per face vertex uv coordinates
                    uv[0] = face.uv[i][0]
                    # origin is now in the top-left (Ogre v0.13.0)
                    uv[1] = 1 - face.uv[i][1]
                xmlVertex.appendTextureCoordinates(uv)
            # vertex colour
            if (mesh.vertexColors):
                colour = face.col[i]
                xmlVertex.setColourDiffuse([colour.r/255.0, colour.g/255.0, colour.b/255.0, colour.a/255.0])
            # check if an equal xmlVertex already exist
            # get vertex 
            if verticesDict.has_key(face.v[i].index):
                # vertex already exists
                vertex = verticesDict[face.v[i].index]
                # compare xmlVertex to vertex and its clones
                if (vertex.xmlVertex != xmlVertex):
                    vertexFound = 0
                    iClone = 0
                    while ((iClone < len(vertex.clones)) and (not vertexFound)):
                        clone = vertex.clones[iClone]
                        if (clone.xmlVertex == xmlVertex):
                            vertexFound = 1
                            vertex = clone
                        iClone += 1
                    if not vertexFound:
                        # create new clone
                        clone = Vertex(submesh, xmlVertex)
                        clone.cloned_from = vertex
                        clone.influences = vertex.influences
                        vertex.clones.append(clone)
                        # write back to dictionary
                        verticesDict[face.v[i].index] = vertex
                        vertex = clone
            else:
                # vertex does not exist yet
                # create vertex
                vertex = Vertex(submesh, xmlVertex)
                verticesDict[face.v[i].index] = vertex
            # postcondition: vertex is current vertex
            face_vertices[i] = vertex
        
        if len(face.v) == 3:
            Face(submesh, face_vertices[0], face_vertices[1], face_vertices[2])
        elif len(face.v) == 4:
            # Split faces with 4 vertices on the shortest edge
            differenceVectorList = [[0,0,0],[0,0,0]]
            for indexOffset in range(2):
                for coordinate in range(3):
                    differenceVectorList[indexOffset][coordinate] = face_vertices[indexOffset].xmlVertex.getPosition()[coordinate] \
                                                                  - face_vertices[indexOffset+2].xmlVertex.getPosition()[coordinate]
            if Mathutils.Vector(differenceVectorList[0]).length < Mathutils.Vector(differenceVectorList[1]).length:
                Face(submesh, face_vertices[0], face_vertices[1], face_vertices[2])
                Face(submesh, face_vertices[2], face_vertices[3], face_vertices[0])
            else:
                Face(submesh, face_vertices[0], face_vertices[1], face_vertices[3])
                Face(submesh, face_vertices[3], face_vertices[1], face_vertices[2])
    else:
        exportLogger.logWarning("Ignored face with %d edges." % len(face.v))
    return

def export_mesh(object, exportOptions):
    global gameEngineMaterialsToggle
    global armatureToggle
    global verticesDict
    global skeletonsDict
    global materialsDict
    global exportLogger
    
    if (object.getType() == "Mesh"):
        # is this mesh attached to an armature?
        skeleton = None
        if armatureToggle.val:
            parent = object.getParent()
            #if parent and parent.getType() == "Armature" and (not skeletonsDict.has_key(parent.getName())):
            if (parent and (parent.getType() == "Armature")):
                if armatureActionActuatorListViewDict.has_key(parent.getName()):
                    actionActuatorList = armatureActionActuatorListViewDict[parent.getName()].armatureActionActuatorList
                    armatureExporter = ArmatureExporter(MeshExporter(object), parent)
                    armatureExporter.export(actionActuatorList, exportOptions, exportLogger)
                    skeleton = armatureExporter.skeleton

        #Mesh of the object
        #for 2.4 us Mesh instead of NMesh
        data = object.getData(False, True)
        matrix = None
        if worldCoordinatesToggle.val:
            matrix = object.getMatrix("worldspace")
        else:
            matrix = Mathutils.Matrix([1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1])
        matrix = matrix*BASE_MATRIX
        # materials of the object
        # note: ogre assigns different textures and different facemodes
        #       to different materials
        objectMaterialDict = {}
        # faces assign to objectMaterial keys
        objectMaterialFacesDict = {}

        # note: these are blender materials. Even if nMaterials = 0
        #       the face can still have a texture (see above)
        # meshMaterialList = data.getMaterials(1)
        meshMaterialList = data.materials
        # note: material slots may be empty, resp. meshMaterialList entries may be None
        nMaterials = len(meshMaterialList)

        # warn if mesh is a subdivision surface
        #if (data.mode & Blender.Mesh.Modes['SUBSURF']):
            #exportLogger.logWarning("Mesh \"%s\" is a subdivision surface. Convert it to mesh, if you want to export the subdivision result." % data.name)
        # create ogre materials
        for face in data.faces:
            faceMaterial = None
            # choose "rendering materials" or "game engine materials"
            if not(gameEngineMaterialsToggle.val):
                # rendering materials
                try:
                    blenderMaterial = meshMaterialList[face.mat]
                except:
                    exportLogger.logError("Material assignment missing for object \"%s\"!" % data.name)
                    blenderMaterial = None
                if blenderMaterial:
                    # non-empty material slot
                    faceMaterial = RenderingMaterial(data, face)
                else:
                    faceMaterial = DefaultMaterial('default')
            else:
                # game engine materials
                if face.image:
                    if (not(face.mode & Blender.Mesh.FaceModes['INVISIBLE'])
                        and not(face.flag & Blender.Mesh.FaceFlags['HIDE'])):
                        faceMaterial = GameEngineMaterial(data, face)
                else:
                    # check if a Blender material is assigned
                    try:
                        blenderMaterial = meshMaterialList[face.mat]
                    except:
                        blenderMaterial = None
                    if blenderMaterial:
                        faceMaterial = GameEngineMaterial(data, face)
                    else:
                        exportLogger.logWarning("Face of object \"%s\" without material assignment! Using default material." % data.name)
                        faceMaterial = DefaultMaterial('default')
            if faceMaterial:
                # insert into Dicts
                materialName = faceMaterial.getName()
                material = objectMaterialDict.get(materialName)
                if material:
                    # append faces
                    faceList = objectMaterialFacesDict[materialName]
                    faceList.append(face)
                    objectMaterialFacesDict[materialName] = faceList
                else:
                    # create new faces list
                    objectMaterialDict[materialName] = faceMaterial
                    objectMaterialFacesDict[materialName] = [face]
        # process faces
        submeshes = []
        for materialKey in objectMaterialDict.keys():
            submesh = SubMesh(objectMaterialDict[materialKey])
            verticesDict = {}
            for face in objectMaterialFacesDict[materialKey]:
                process_face(face, submesh, data, matrix, skeleton)
            if len(submesh.faces):
                # process verticesDict and add bone influences
                process_vert_influences(data, skeleton)
                submeshes.append(submesh)
                # update global materialsDict
                material = materialsDict.get(materialKey)
                if not material:
                    materialsDict[materialKey] = objectMaterialDict[materialKey]
        # write mesh
        if len(submeshes) == 0:
            # no submeshes
            exportLogger.logWarning("Mesh %s has no visible faces!" % data.name)
        else:
            # write mesh
            mesh = Mesh(submeshes, skeleton, data)
            mesh.write()
    return

#######################################################################################
## file output

def tab(tabsize):
    return "\t" * tabsize
    
def clamp(val):
    if val < 0.0:
        val = 0.0
    if val > 1.0:
        val = 1.0
    return val

def convertXMLFile(filename):
    """Calls the OgreXMLConverter on a file.
       
       If the script variable <code>OGRE_XML_CONVERTER</code> is nonempty, the
       OgreXMLConverter is called to convert the given file.
       
       @param filename filename of the XML file to convert.
    """
    global exportLogger
    if OGRE_XML_CONVERTER != '':
        commandLine = OGRE_XML_CONVERTER + ' "' + filename + '"'
        exportLogger.logInfo("Running OgreXMLConverter: " + commandLine)
        xmlConverter = os.popen(commandLine, 'r')
        if xmlConverter == None:
            exportLogger.logError('Could not run OgreXMLConverter!')
        else:
            for line in xmlConverter:
                exportLogger.logInfo("OgreXMLConverter: " + line)
            xmlConverter.close()
    return

def write_materials():
    global ambientToggle, pathString, materialString, exportLogger
    global materialsDict
    file = materialString.val
    exportLogger.logInfo("Materials \"%s\"" % file)
    f = open(os.path.join(pathString.val, file), "w")
    for material in materialsDict.values():
        material.write(f)
    f.close()
    return

#######################################################################################
## main export

def export(selectedObjectsList):
    global pathString, scaleNumber, rotXNumber, rotYNumber, rotZNumber
    global materialsDict
    global skeletonsDict
    global BASE_MATRIX
    global exportLogger
    
    materialsDict = {}
    skeletonsDict = {}

    # default: set matrix to 90 degree rotation around x-axis
    # rotation order: x, y, z
    # WARNING: Blender uses left multiplication!
    rotationMatrix = Mathutils.RotationMatrix(rotXNumber.val,4,'x')
    rotationMatrix *= Mathutils.RotationMatrix(rotYNumber.val,4,'y')
    rotationMatrix *= Mathutils.RotationMatrix(rotZNumber.val,4,'z')
    scaleMatrix = Mathutils.Matrix([scaleNumber.val,0,0],[0,scaleNumber.val,0],[0,0,scaleNumber.val])
    scaleMatrix.resize4x4()
    BASE_MATRIX = rotationMatrix*scaleMatrix

    exportOptions = ExportOptions(rotXNumber.val, rotYNumber.val, rotZNumber.val, scaleNumber.val,
        worldCoordinatesToggle.val, ambientToggle.val, pathString.val, materialString.val)

    if not os.path.exists(pathString.val):
      exportLogger.logError("Invalid path: "+pathString.val)
    else:
      exportLogger.logInfo("Exporting selected objects into \"" + pathString.val + "\":")
      n = 0
      for obj in selectedObjectsList:
          if obj:
              if obj.getType() == "Mesh":
                  exportLogger.logInfo("Exporting object \"%s\":" % obj.getName())
                  export_mesh(obj, exportOptions)
                  n = 1
              elif obj.getType() == "Armature":
                  exportLogger.logInfo("Exporting object \"%s\":" % obj.getName())
                  actionActuatorList = armatureActionActuatorListViewDict[obj.getName()].armatureActionActuatorList
                  armatureMeshExporter = ArmatureMeshExporter(obj)
                  armatureMeshExporter.export(materialsDict, actionActuatorList, exportOptions, exportLogger)
      if n == 0:
          exportLogger.logWarning("No mesh objects selected!")
      if len(materialsDict) == 0:
          exportLogger.logWarning("No materials or textures defined!")
      else:
          write_materials()
      
      exportLogger.logInfo("Finished.")
    return exportLogger.getStatus()
    
#######################################################################################
## GUI

######
# global variables
######
# see above

######
# methods
######
def saveSettings():
    """Save all exporter settings of selected and unselected objects into a blender text object.
    
       Settings are saved to the text 'ogreexport.cfg' inside the current .blend file. Settings
       belonging to removed objects in the .blend file will not be saved.
       
       @return <code>true</code> on success, else <code>false</code>
    """
    global gameEngineMaterialsToggle
    global armatureToggle
    global worldCoordinatesToggle
    global ambientToggle
    global pathString
    global materialString
    global scaleNumber
    global rotXNumber, rotYNumber, rotZNumber
    global fpsNumber
    global selectedObjectsList
    global armatureDict
    global armatureActionActuatorListViewDict
    global armatureAnimationDictListDict
    settingsDict = {}
    success = 0
    # save general settings
    settingsDict['gameEngineMaterialsToggle'] = gameEngineMaterialsToggle.val
    settingsDict['armatureToggle'] = armatureToggle.val
    settingsDict['worldCoordinatesToggle'] = worldCoordinatesToggle.val
    settingsDict['ambientToggle'] = ambientToggle.val
    settingsDict['pathString'] = pathString.val
    settingsDict['materialString'] = materialString.val
    settingsDict['scaleNumber'] = scaleNumber.val
    settingsDict['rotXNumber'] = rotXNumber.val
    settingsDict['rotYNumber'] = rotYNumber.val
    settingsDict['rotZNumber'] = rotZNumber.val
    if (Blender.Get("version") < 233):
        settingsDict['fpsNumber'] = fpsNumber.val
    else:
        # get blender's "scene->format->frames per second" setting
        settingsDict['fpsNumber'] = Blender.Scene.GetCurrent().getRenderingContext().framesPerSec()
    # save object specific settings
    # check if armature exists (I think this is cleaner than catching NameError exceptions.)
    # create list of valid armature names
    armatureNameList = []
    for object in Blender.Object.Get():
        if (object.getType() == "Armature"):
            armatureNameList.append(object.getName())
    for armatureName in armatureAnimationDictListDict.keys():
        if not(armatureName in armatureNameList):
            # remove obsolete settings
            del armatureAnimationDictListDict[armatureName]
    # update settings
    for armatureName in armatureActionActuatorListViewDict.keys():
        armatureAnimationDictListDict[armatureName] = armatureActionActuatorListViewDict[armatureName].getArmatureAnimationDictList()
    settingsDict['armatureAnimationDictListDict'] = armatureAnimationDictListDict
        
    configTextName = 'ogreexport.cfg'
    # remove old configuration text
    if configTextName in [text.getName() for text in Blender.Text.Get()]:
        oldConfigText = Blender.Text.Get(configTextName)
        oldConfigText.setName('ogreexport.old')
        Blender.Text.unlink(oldConfigText)
    # write new configuration text
    configText = Blender.Text.New(configTextName)
    configText.write('Ogreexport configuration file.\n\nThis file is automatically created. Please don\'t edit this file directly.\n\n')
    try:
        # pickle
        configText.write(pickle.dumps(settingsDict))
    except (PickleError):
        pass
    else:
        success = 1
    return success

def loadSettings(filename):
    """Load all exporter settings from text or file.
    
       Settings are loaded from a text object called 'ogreexport.cfg'.
       If it is not found, settings are loaded from the file with the given filename.
       <p>
       You have to create armatureActionActuatorListViews with the new
       armatuerAnimationDictListDict if you want the animation settings
       to take effect.
    
       @param filename where to store the settings
       @return <code>true</code> on success, else <code>false</code>
    """
    global gameEngineMaterialsToggle
    global armatureToggle
    global worldCoordinatesToggle
    global ambientToggle
    global pathString
    global materialString
    global scaleNumber
    global rotXNumber, rotYNumber, rotZNumber
    global fpsNumber
    global selectedObjectsList
    global armatureDict
    global armatureAnimationDictListDict
    settingsDict = {}
    success = 0
    # try open 'ogreexport.cfg' text
    configTextName = 'ogreexport.cfg'
    if configTextName in [text.getName() for text in Blender.Text.Get()]:
        configText = Blender.Text.Get(configTextName)
        # compose string from text and unpickle
        try:
            # unpickle
            settingsDict = pickle.loads(string.join(configText.asLines()[4:],'\n'))
        except (PickleError):
            pass
        else:
            success = 1        
    # else try open filename
    if not success and os.path.isfile(filename):
        # open file
        try:
            fileHandle = open(filename,'r')
        except IOError, (errno, strerror):
            print "I/O Error(%s): %s" % (errno, strerror)
        else:
            try:
                # load settings
                unpickler = pickle.Unpickler(fileHandle) 
                settingsDict = unpickler.load()
                # close file
                fileHandle.close()
            except EOFError:
                print "EOF Error"
            else:
                success = 1
    # set general settings
    if settingsDict.has_key('gameEngineMaterialsToggle'):
        gameEngineMaterialsToggle = Blender.Draw.Create(settingsDict['gameEngineMaterialsToggle'])
    if settingsDict.has_key('armatureToggle'):
        armatureToggle = Blender.Draw.Create(settingsDict['armatureToggle'])
    if settingsDict.has_key('worldCoordinatesToggle'):
        worldCoordinatesToggle = Blender.Draw.Create(settingsDict['worldCoordinatesToggle'])
    if settingsDict.has_key('ambientToggle'):
        ambientToggle = Blender.Draw.Create(settingsDict['ambientToggle'])
    elif settingsDict.has_key('armatureMeshToggle'):
        # old default was export in world coordinates
        worldCoordinatesToggle = Blender.Draw.Create(1)
    if settingsDict.has_key('pathString'):
        pathString = Blender.Draw.Create(settingsDict['pathString'])
    if settingsDict.has_key('materialString'):
        materialString = Blender.Draw.Create(settingsDict['materialString'])
    if settingsDict.has_key('scaleNumber'):
        scaleNumber = Blender.Draw.Create(settingsDict['scaleNumber'])
    if settingsDict.has_key('rotXNumber'):
        rotXNumber = Blender.Draw.Create(settingsDict['rotXNumber'])
    if settingsDict.has_key('rotYNumber'):
        rotYNumber = Blender.Draw.Create(settingsDict['rotYNumber'])
    if settingsDict.has_key('rotZNumber'):
        rotZNumber = Blender.Draw.Create(settingsDict['rotZNumber'])
    if settingsDict.has_key('fpsNumber'):
        fpsNumber = Blender.Draw.Create(settingsDict['fpsNumber'])
    # set object specific settings
    if settingsDict.has_key('armatureAnimationDictListDict'):
        armatureAnimationDictListDict = settingsDict['armatureAnimationDictListDict']
    elif settingsDict.has_key('animationDictListDict'):
        # convert old animationDictListDict
        ## create list of valid armature names
        armatureNameList = []
        for object in Blender.Object.Get():
            if (object.getType() == "Armature"):
                armatureNameList.append(object.getName())
        # create armatureAnimationDictListDict
        armatureAnimationDictListDict = {}
        animationDictListDict = settingsDict['animationDictListDict']
        for armatureName in armatureNameList:
            if animationDictListDict.has_key(armatureName):
                # convert animationDictList
                armatureActionDict = ArmatureAction.createArmatureActionDict(Blender.Object.Get(armatureName))
                armatureActionActuatorListView = ArmatureActionActuatorListView(armatureActionDict, MAXACTUATORS, BUTTON_EVENT_ACTUATOR_RANGESTART,{})
                armatureActionActuatorListView.setAnimationDictList(animationDictListDict[armatureName])
                armatureAnimationDictListDict[armatureName] = armatureActionActuatorListView.getArmatureAnimationDictList()
    return success
    
def refreshGUI():
    """refresh GUI after export and selection change
    """
    global exportLogger
    global selectedObjectsList, armatureToggle, armatureDict
    global armatureActionActuatorListViewDict
    global armatureAnimationDictListDict
    # export settings
    exportLogger = Logger()
    # synchronize armatureAnimationDictListDict
    for armatureName in armatureActionActuatorListViewDict.keys():
        armatureAnimationDictListDict[armatureName] = armatureActionActuatorListViewDict[armatureName].getArmatureAnimationDictList()
    selectedObjectsList = Blender.Object.GetSelected()
    if not selectedObjectsList:
        # called from command line
        selectedObjectsList = []
    armatureDict = {}
    # create fresh armatureDict
    for object in selectedObjectsList:
        if (object.getType() == "Armature"):
            # add armature to armatureDict
            armatureDict[object.getName()] = object.getName()
        elif (object.getType() == "Mesh"):
            parent = object.getParent()
            if parent and parent.getType() == "Armature":
                # add armature to armatureDict
                armatureDict[object.getName()] = parent.getName()
    # refresh ArmatureActionActuatorListViews
    for armatureName in armatureDict.values():
        # create armatureActionDict
        armatureActionDict = ArmatureAction.createArmatureActionDict(Blender.Object.Get(armatureName))
        # get animationDictList
        armatureAnimationDictList = None
        if armatureAnimationDictListDict.has_key(armatureName):
            armatureAnimationDictList = armatureAnimationDictListDict[armatureName]
        if armatureActionActuatorListViewDict.has_key(armatureName):
            # refresh armatureActionActuators
            armatureActionActuatorListViewDict[armatureName].refresh(armatureActionDict)
        else:
            # create armatureActionActuatorListView
            armatureActionActuatorListViewDict[armatureName] = ArmatureActionActuatorListView(armatureActionDict, MAXACTUATORS, BUTTON_EVENT_ACTUATOR_RANGESTART, armatureAnimationDictList)
    return

def initGUI():
    """initialization of the GUI
    """
    global armatureActionActuatorListViewDict
    if KEEP_SETTINGS:
        # load exporter settings
        loadSettings(Blender.Get('filename')+".ogre")
    armatureActionActuatorListViewDict = {}
    refreshGUI()
    return

def exitGUI():
    if KEEP_SETTINGS:
        # save exporter settings
        saveSettings()
    Blender.Draw.Exit()
    return

def pathSelectCallback(fileName):
    """handles FileSelector output
    """
    global pathString
    # strip path from fileName
    pathString = Blender.Draw.Create(os.path.dirname(fileName))
    return
    
def eventCallback(event,value):
    """handles keyboard and mouse events
       <p>    
       exits on ESCKEY<br>
       exits on QKEY
    """
    global scrollbar
    global selectedObjectsList, selectedObjectsMenu, armatureActionActuatorListViewDict, armatureDict
    # eventFilter for current ArmatureActionActuatorListView
    if (len(selectedObjectsList) > 0):
        selectedObjectsListIndex = selectedObjectsMenu.val
        selectedObjectName = selectedObjectsList[selectedObjectsListIndex].getName()
        if armatureDict.has_key(selectedObjectName):
            armatureName = armatureDict[selectedObjectName]
            armatureActionActuatorListViewDict[armatureName].eventFilter(event, value)
    scrollbar.eventFilter(event, value)
    if (value != 0):
        # pressed
        if (event == Draw.ESCKEY):
            exitGUI()
        if (event == Draw.QKEY):
            exitGUI()
    return

def buttonCallback(event):
    """handles button events
    """
    global materialString, doneMessageBox, eventCallback, buttonCallback, scrollbar
    global selectedObjectsList, selectedObjectsMenu, armatureActionActuatorListViewDict, armatureDict
    global fpsNumber
    # buttonFilter for current ArmatureActionActuatorListView
    if (len(selectedObjectsList) > 0):
        selectedObjectsListIndex = selectedObjectsMenu.val
        selectedObjectName = selectedObjectsList[selectedObjectsListIndex].getName()
        if armatureDict.has_key(selectedObjectName):
            armatureName = armatureDict[selectedObjectName]
            armatureActionActuatorListViewDict[armatureName].buttonFilter(event)
    scrollbar.buttonFilter(event)
    if (event == BUTTON_EVENT_OK): # Ok
        # restart
        refreshGUI()
        Draw.Register(gui, eventCallback, buttonCallback)
    elif (event == BUTTON_EVENT_UPDATEBUTTON):
        # update list of selected objects
        refreshGUI()
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_SELECTEDOBJECTSMENU):
        # selected object changed
        Draw.Redraw(1)
    elif (event  == BUTTON_EVENT_QUIT): # Quit
        exitGUI()
    elif (event == BUTTON_EVENT_GAMEENGINEMATERIALSTOGGLE):
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_ARMATURETOGGLE): # armatureToggle
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_PATHBUTTON): # pathButton
        Blender.Window.FileSelector(pathSelectCallback, "Export Directory", pathString.val)
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_MATERIALSTRING): # materialString
        materialString = Blender.Draw.Create(PathName(materialString.val).basename())
        if (len(materialString.val) == 0):
            materialString = Blender.Draw.Create(Blender.Scene.GetCurrent().getName() + ".material")
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_SCROLLBAR): # scrollbar
        Draw.Redraw(1)
    elif (event == BUTTON_EVENT_EXPORT): # export
        Draw.Register(exportMessageBox, None, None)
        Draw.Draw()
        # export
        if (Blender.Get("version") >= 233):
            # get blender's current "scene->format->frames per second" setting
            fpsNumber = Draw.Create(Blender.Scene.GetCurrent().getRenderingContext().framesPerSec())
        export(selectedObjectsList)
        # set donemessage
        scrollbar = ReplacementScrollbar(0,0,len(exportLogger.getMessageList())-1,BUTTON_EVENT_SCROLLBARUP,BUTTON_EVENT_SRCROLLBARDOWN)
        Draw.Register(doneMessageBox, eventCallback, buttonCallback)
        Draw.Redraw(1)
    return

def frameDecorator(x, y, width):
    """draws title and logo onto the frame
    
        @param x upper left x coordinate
        @param y upper left y coordinate
        @param width screen width to use
        @return used height
    """
    # title
    glColor3ub(210, 236, 210)
    glRectf(x,y-41,x+width,y-17)
    title = "Mesh and Armature Exporter"
    glColor3ub(50, 62, 50)
    glRasterPos2i(x+126, y-34)
    Draw.Text(title, "normal")
    glRasterPos2i(x+127, y-34)
    Draw.Text(title, "normal")
    
    # logo
    glRasterPos2i(x+1, y-48)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glDrawPixels(122, 48, GL_RGBA, GL_BYTE, OGRE_LOGO)
    glColor3f(0,0,0)
    return 50

def gui():
    """draws the screen
    """
    global gameEngineMaterialsToggle, armatureToggle, worldCoordinatesToggle, \
        ambientToggle, pathString, materialString, scaleNumber, fpsNumber, \
        scrollbar, rotXNumber, rotYNumber, rotZNumber
    global selectedObjectsList, selectedObjectsMenu, armatureActionActuatorListViewDict, armatureDict
    # get size of the window
    guiRectBuffer = Buffer(GL_FLOAT, 4)
    glGetFloatv(GL_SCISSOR_BOX, guiRectBuffer)
    guiRect = [0, 0, int(guiRectBuffer.list[2]), int(guiRectBuffer.list[3])]
    
    remainRect = guiRect[:]
    remainRect[0] += 10
    remainRect[1] += 10
    remainRect[2] -= 10
    remainRect[3] -= 10
    
    # clear background
    glClearColor(0.6,0.6,0.6,1) # Background: grey
    glClear(GL_COLOR_BUFFER_BIT)
    
    remainRect[3] -= frameDecorator(remainRect[0], remainRect[3], remainRect[2]-remainRect[0])
    
    # export settings
    remainRect[3] -= 5
    # first row
    materialString = Draw.String("Material File: ", BUTTON_EVENT_MATERIALSTRING, \
            remainRect[0],remainRect[3]-25, 450, 20, \
            materialString.val, 255,"all material definitions go in this file (relative to the save path)")
    remainRect[3] -= 25
    # second row
    gameEngineMaterialsToggle = Draw.Toggle("Game Engine Materials", BUTTON_EVENT_GAMEENGINEMATERIALSTOGGLE, \
                remainRect[0], remainRect[3]-25, 220, 20, \
                gameEngineMaterialsToggle.val, "export game engine materials instead of rendering materials")
    # scale settings
    scaleNumber = Draw.Number("Mesh Scale Factor: ", BUTTON_EVENT_SCALENUMBER, \
            remainRect[0]+230, remainRect[3]-25, 220, 20, \
            scaleNumber.val, 0.0, 1000.0, "scale factor")
    remainRect[3] -= 25
    # third row    
    armatureToggle = Draw.Toggle("Export Armatures", BUTTON_EVENT_ARMATURETOGGLE, \
                remainRect[0], remainRect[3]-25, 220, 20, \
                armatureToggle.val, "export skeletons and bone weights in meshes")
    rotXNumber = Draw.Number("RotX: ", BUTTON_EVENT_ROTXNUMBER, \
            remainRect[0]+230, remainRect[3]-25, 220, 20, \
            rotXNumber.val, -360.0, 360.0, "angle of the first rotation, around the x-axis")
    remainRect[3] -= 25
    # fourth row
    worldCoordinatesToggle = Draw.Toggle("World Coordinates", BUTTON_EVENT_WORLDCOORDINATESTOGGLE, \
            remainRect[0], remainRect[3]-25, 220, 20, \
            worldCoordinatesToggle.val, "use world coordinates instead of object coordinates")
    rotYNumber = Draw.Number("RotY: ", BUTTON_EVENT_ROTYNUMBER, \
            remainRect[0]+230, remainRect[3]-25, 220, 20, \
            rotYNumber.val, -360.0, 360.0, "angle of the second rotation, around the y-axis")
    remainRect[3] -= 25
    # fifth row
    ambientToggle = Draw.Toggle("Coloured Ambient", BUTTON_EVENT_AMBIENTTOGGLE, \
            remainRect[0], remainRect[3]-25, 220, 20, \
            ambientToggle.val, "use Amb factor times diffuse colour as ambient instead of Amb factor times white")
    rotZNumber = Draw.Number("RotZ: ", BUTTON_EVENT_ROTZNUMBER, \
            remainRect[0]+230, remainRect[3]-25, 220, 20, \
            rotZNumber.val, -360.0, 360.0, "angle of the third rotation, around the z-axis")
    # sixth row
    if ((armatureToggle.val == 1) and (Blender.Get("version") < 233)):
        remainRect[3] -= 25
        fpsNumber = Draw.Number("Frs/Sec: ", BUTTON_EVENT_FPSNUMBER, \
                remainRect[0]+230, remainRect[3]-25, 220, 20, \
                fpsNumber.val, 1, 120, "animation speed in frames per second")
    remainRect[3] -= 35
    
    # Path setting
    pathString = Draw.String("Path: ", BUTTON_EVENT_PATHSTRING, \
            10, 50, guiRect[2]-91, 20, \
            pathString.val, 255, "the directory where the exported files are saved")
    Draw.Button("Select", BUTTON_EVENT_PATHBUTTON, guiRect[2]-80, 50, 70, 20, "select the export directory")
    # button panel
    Draw.Button("Export", BUTTON_EVENT_EXPORT,10,10,100,30,"export selected objects")
    # Draw.Button("Help",BUTTON_EVENT_HELP ,(guiRect[2])/2-50,10,100,30,"notes on usage")    
    Draw.Button("Quit", BUTTON_EVENT_QUIT,guiRect[2]-110,10,100,30,"quit without exporting")
    remainRect[1] += 70
    
    # rename animation part    
    #if (armatureToggle.val == 1):
    animationText = "Animation settings of"
    xOffset = Draw.GetStringWidth(animationText) + 5
    selectedObjectsMenuName = ""
    selectedObjectsMenuIndex = 0
    if (len(selectedObjectsList) > 0):
        for object in selectedObjectsList:
            # add menu string
            selectedObjectsMenuName += object.getName() + " %x" + ("%d" % selectedObjectsMenuIndex) + "|"
            selectedObjectsMenuIndex += 1
    else:
        selectedObjectsMenuName = "No objects selected! %t"
    selectedObjectsMenu = Draw.Menu(selectedObjectsMenuName, BUTTON_EVENT_SELECTEDOBJECTSMENU, \
                          remainRect[0]+xOffset, remainRect[3]-20, 140, 20, \
                          selectedObjectsMenu.val, "Objects selected for export")
    xOffset += 141
    # update button
    Draw.Button("Update", BUTTON_EVENT_UPDATEBUTTON, remainRect[0]+xOffset, remainRect[3]-20, 60, 20, "update list of selected objects")
    remainRect[3] -= 25
    if (armatureToggle.val == 1):
        # draw armatureActionActuator
        if (len(selectedObjectsList) > 0):
            selectedObjectsListIndex = selectedObjectsMenu.val
            selectedObjectName = selectedObjectsList[selectedObjectsListIndex].getName()
            if armatureDict.has_key(selectedObjectName):
                glRasterPos2i(remainRect[0],remainRect[3]+10)
                Draw.Text(animationText)
                armatureName = armatureDict[selectedObjectName]
                armatureActionActuatorListViewDict[armatureName].draw(remainRect[0], remainRect[1], remainRect[2]-remainRect[0], remainRect[3]-remainRect[1])
    return

def exportMessageBox():
    """informs on the export progress
    """
    # get size of the window
    guiRectBuffer = Buffer(GL_FLOAT, 4)
    glGetFloatv(GL_SCISSOR_BOX, guiRectBuffer)
    guiRect = [0, 0, int(guiRectBuffer.list[2]), int(guiRectBuffer.list[3])]
    
    remainRect = guiRect[:]
    remainRect[0] += 10
    remainRect[1] += 10
    remainRect[2] -= 10
    remainRect[3] -= 10

    # clear background
    glClearColor(0.6,0.6,0.6,1) # Background: grey
    glClear(GL_COLOR_BUFFER_BIT)
    
    remainRect[3] -= frameDecorator(remainRect[0], remainRect[3], remainRect[2]-remainRect[0])
    
    # export information
    ## center view
    exportMessage = "Exporting, please wait!"
    exportMessageWidth = Draw.GetStringWidth(exportMessage, 'normal')
    textPosition = [0, 0]
    textPosition[0] = (remainRect[0] + remainRect[2] - exportMessageWidth)/2
    textPosition[1] = (remainRect[1] + remainRect[3])/2
    glRasterPos2i(textPosition[0], textPosition[1]) 
    glColor3f(0,0,0) # Defaul color: black
    Draw.Text(exportMessage, "normal")
    return
    
def doneMessageBox():
    """displays export message and log
    """
    global exportLogger
    EXPORT_SUCCESS_MESSAGE = "Successfully exported!"
    EXPORT_WARNING_MESSAGE = "Exported with warnings!"
    EXPORT_ERROR_MESSAGE = "Error in export!"    
    # get size of the window
    guiRectBuffer = Buffer(GL_FLOAT, 4)
    glGetFloatv(GL_SCISSOR_BOX, guiRectBuffer)
    guiRect = [0, 0, int(guiRectBuffer.list[2]), int(guiRectBuffer.list[3])]
        
    remainRect = guiRect[:]
    remainRect[0] += 10
    remainRect[1] += 10
    remainRect[2] -= 10
    remainRect[3] -= 10
    
    # clear background
    glClearColor(0.6,0.6,0.6,1) # Background: grey
    glClear(GL_COLOR_BUFFER_BIT)
    
    remainRect[3] -= frameDecorator(remainRect[0], remainRect[3], remainRect[2]-remainRect[0])
    
    # OK button
    Draw.Button("OK", BUTTON_EVENT_OK,10,10,100,30,"return to export settings")
    Draw.Button("Quit", BUTTON_EVENT_QUIT,guiRect[2]-110,10,100,30,"quit export script")
    remainRect[1] += 40
    
    # message
    status = exportLogger.getStatus()
    doneMessage = ''
    if (status == Logger.INFO):
        doneMessage= EXPORT_SUCCESS_MESSAGE
    elif (status == Logger.WARNING):
        doneMessage = EXPORT_WARNING_MESSAGE
        glColor3f(1.0,1.0,0.0)
        Blender.BGL.glRectf(remainRect[0], remainRect[3]-24, remainRect[0]+Draw.GetStringWidth(doneMessage), remainRect[3]-7)
    elif (status == Logger.ERROR):
        doneMessage = EXPORT_ERROR_MESSAGE
        glColor3f(1.0,0.0,0.0)
        Blender.BGL.glRectf(remainRect[0], remainRect[3]-24, remainRect[0]+Draw.GetStringWidth(doneMessage), remainRect[3]-7)
    remainRect[3] -= 20
    glColor3f(0.0,0.0,0.0) # Defaul color: black
    glRasterPos2i(remainRect[0],remainRect[3])
    Draw.Text(doneMessage,"normal")
    
    remainRect[3] -= 20
    glColor3f(0.0,0.0,0.0) # Defaul color: black
    glRasterPos2i(remainRect[0],remainRect[3])
    Draw.Text("Export Log:","small")
    remainRect[3] -= 4
    
    # black border
    logRect = remainRect[:]
    logRect[2] -= 22
    glColor3f(0,0,0) # Defaul color: black
    glRectf(logRect[0],logRect[1],logRect[2],logRect[3])
    logRect[0] += 1
    logRect[1] += 1
    logRect[2] -= 1
    logRect[3] -= 1
    glColor3f(0.662,0.662,0.662) # Background: grey
    glRectf(logRect[0],logRect[1],logRect[2],logRect[3])
    
    # display export log
    exportLog = exportLogger.getMessageList()
    scrollPanelRect = remainRect[:]
    loglineiMax = len(exportLog)
    loglinei = scrollbar.getCurrentValue()
    while (((logRect[3]-logRect[1]) >= 20) and ( loglinei < loglineiMax )):
        logRect[3] -= 16
        (status, message) = exportLog[loglinei]
        if (status == Logger.WARNING):
            glColor3f(1.0,1.0,0.0)
            glRecti(logRect[0],logRect[3]-4,logRect[2],logRect[3]+13)
        elif (status == Logger.ERROR):
            glColor3f(1.0,0.0,0.0)
            glRecti(logRect[0],logRect[3]-4,logRect[2],logRect[3]+13)
        glColor3f(0,0,0)
        glRasterPos2i(logRect[0]+4,logRect[3])
        Draw.Text(message)
        loglinei += 1
    # clip log text
    glColor3f(0.6,0.6,0.6) # Background: grey
    glRectf(scrollPanelRect[2]-22,scrollPanelRect[1], guiRect[2],scrollPanelRect[3])
    # draw scrollbar
    scrollbar.draw(scrollPanelRect[2]-20, scrollPanelRect[1], 20, scrollPanelRect[3]-scrollPanelRect[1])
    return

######
# Main
######
if (__name__ == "__main__"):
    initGUI()
    Draw.Register(gui, eventCallback, buttonCallback)
