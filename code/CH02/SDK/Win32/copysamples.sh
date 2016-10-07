#!/bin/bash
rm -R samples/refapp
rm -R samples/scripts
rm -R samples/src
rm -R samples/include
mkdir samples/scripts
mkdir samples/refapp
mkdir samples/refapp/scripts
mkdir samples/src
mkdir samples/include

# Do the project files
/bin/find ../../samples -iname *_stlp.cbp -exec cp \{\} samples/scripts \;
/bin/find ../../samples -iname *.vcproj -exec cp \{\} samples/scripts \;
/bin/find ../../samples -iname *_vc8.vcproj.user -exec cp \{\} samples/scripts \;
cp ../../ReferenceApplication/BspCollision/scripts/*_stlp.cbp samples/scripts
cp ../../ReferenceApplication/BspCollision/scripts/*.vcproj samples/scripts
cp ../../ReferenceApplication/BspCollision/scripts/*_vc8.vcproj.user samples/scripts
cp ../../ReferenceApplication/ReferenceAppLayer/scripts/*_stlp.cbp samples/refapp/scripts
cp ../../ReferenceApplication/ReferenceAppLayer/scripts/*.vcproj samples/refapp/scripts
rm samples/scripts/OgreGUIRenderer_stlp.cbp
rm samples/scripts/OgreGUIRenderer.vcproj
rm samples/scripts/OgreGUIRenderer_vc8.vcproj
/bin/find samples/scripts/ -iname *_stlp.cbp -exec sed -i -f altersamples.sed \{\} \;
/bin/find samples/scripts/ -iname *.vcproj -exec sed -i -f altersamples.sed \{\} \;
/bin/find samples/scripts/ -iname *_vc8.vcproj.user -exec sed -i -f altersamples.sed \{\} \;
/bin/find samples/refapp/scripts/ -iname *_stlp.cbp -exec sed -i -f alterrefapp.sed \{\} \;
/bin/find samples/refapp/scripts/ -iname *.vcproj -exec sed -i -f alterrefapp.sed \{\} \;

# Combine the include / src folders; easier to do here than in setup
/bin/find ../../samples -iname *.cpp -exec cp \{\} samples/src \;
/bin/find ../../samples -iname *.h -exec cp \{\} samples/include \;
cp ../../ReferenceApplication/BspCollision/src/*.cpp samples/src

# Copy and alter resources.cfg
cp ../../Samples/Common/bin/Release/resources.cfg samples/
sed -i -e 's/\.\.\/\.\.\/\.\.\/Media/..\/..\/media/i' samples/resources.cfg
