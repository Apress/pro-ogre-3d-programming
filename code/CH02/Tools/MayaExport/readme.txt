Here is the source code for the OGRE Exporter for Maya

Direcories:
+-- include: .h files containing class interfaces
|
+-- mel: .mel scripts to place in the maya/scripts directory. If you already have a userSetup.mel, 
|        just copy and paste to append these extra lines.
|        You will need to place in the same dirs the file OgreXMLConverter.exe and it's related dlls
|        that can be found or built in the rest of the ogrenew module
|
+-- project: visual studio 7.1 scripts
|
+-- src: source files containing classes implementation

Compiling:
use the maya2ogre.vcproj file to open the visual studio project,
you'll have to change some paths in the project script:

include paths:
- C:\Program Files\Alias\Maya6.5\include : change this to your maya include path, you can use different versions of maya too

link paths:
- C:\Program Files\Alias\Maya6.5\lib : change this to your maya lib path, same as above for other versions of maya.


!!!Important note!!!!
If you compile on VC7.0 or older, remember to change priority of stlport directory to lowest possible, 
otherwise it won't compile or it will require stlport.dll for running.


If you have any problems check on OGRE forum or e-mail to fra.giordana@tiscali.it