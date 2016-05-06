#! /bin/sh

os=$(uname -s)

if [ "$os" = "Darwin" ]
then

    mkdir -p BUILD
    cd BUILD && /Applications/CMake.app/Contents/bin/cmake ../
fi
