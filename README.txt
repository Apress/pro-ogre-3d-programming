Contents of this package:

CH02/ contains the Ogre 1.2.2 source code as downloaded from the Downloads section of the www.Ogre3d.com website. This code can be compiled on Windows, Linux and MacOSX (with XCode):

For Windows: open the Ogre.sln (or Ogre_VC8.sln for Visual C++ 2005) solution file in Visual Studio, and perform "Build Solution". All Ogre libraries, plugins and Samples will build. Samples will be installed to Samples/Common/bin/Release or Samples/Common/bin/Debug, matching the solution configuraiton you selected when building the solution.

For Linux: From the command line (Bash prompt, etc.), run "bootstrap", then "configure" (possibly with optional switches -- run "configure --help" to see which options are available), then "make" and then (as root) "make install". All Ogre libraries, plugins and Samples will build (except those that you might have disabled with 'configure' options). Samples will install to Samples/Common/bin in the source tree; library files and headers will install to wherever you instructed 'configure' (via the --prefix configure option), or to /usr/local/lib if you did not provide a --prefix option. Change to the Samples/Common/bin directory to run the Samples. When running the Samples, you may need to adjust the "PlugInPath" directive in the plugins.cfg file Samples/Common/bin to match any alternate location for the library files that you specified. 

For Mac OSX: Unforunately I do not have a Mac on which to build Ogre, so I refer you to the Ogre Wiki and Forums for assistance with building a Mac Universal Binary package under XCode.



CH04/ contains the two small minimal example applications discussed in Chapter 4. "Manual" is an example of starting up an Ogre application without using the "ExampleApplication" framework; "QuickStart" is an ExampleApplication-based skeleton application. You may use either of these as the foundation for your own Ogre-based application if you wish -- the code comes only with a copyright: no license, no warranty and no implication of suitability for any purpose, and so on.


