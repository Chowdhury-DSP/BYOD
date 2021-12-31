#!/bin/bash

build64(){
    cmake -Bbuild -G"Visual Studio 16 2019" -A x64 -DBUILD_RELEASE=ON -DASIOSDK_DIR="C:\\SDKs\\ASIO_SDK"
    cmake --build build --config Release -j4
}

build32(){
    cmake -Bbuild32 -G"Visual Studio 16 2019" -A Win32 -DBUILD_RELEASE=ON -DASIOSDK_DIR="C:\\SDKs\\ASIO_SDK"
    cmake --build build32 --config Release -j4
}

# exit on failure
set -e

# clean up old builds
rm -Rf build/
rm -Rf build32/
rm -Rf bin/*Win64*
rm -Rf bin/*Win32*

# set up VST SDK path
VST_SDK="C:/SDKs/VST_SDK/VST2_SDK/"
sed -i -e "s~# juce_set_vst2_sdk_path.*~juce_set_vst2_sdk_path(${VST_SDK})~" CMakeLists.txt

# cmake new builds
build64 &
build32 &
wait

# copy builds to bin
mkdir -p bin/Win64
mkdir -p bin/Win32
declare -a plugins=("BYOD")
for plugin in "${plugins[@]}"; do
    cp -R build/${plugin}_artefacts/Release/Standalone/${plugin}.exe bin/Win64/${plugin}.exe
    cp -R build/${plugin}_artefacts/Release/VST/${plugin}.dll bin/Win64/${plugin}.dll
    cp -R build/${plugin}_artefacts/Release/VST3/${plugin}.vst3 bin/Win64/${plugin}.vst3

    cp -R build32/${plugin}_artefacts/Release/Standalone/${plugin}.exe bin/Win32/${plugin}.exe
    cp -R build32/${plugin}_artefacts/Release/VST/${plugin}.dll bin/Win32/${plugin}.dll
    cp -R build32/${plugin}_artefacts/Release/VST3/${plugin}.vst3 bin/Win32/${plugin}.vst3
done

# reset CMakeLists.txt
git restore CMakeLists.txt

# zip builds
VERSION=$(cut -f 2 -d '=' <<< "$(grep 'CMAKE_PROJECT_VERSION:STATIC' build/CMakeCache.txt)")
(
    cd bin
    rm -f "BYOD-Win64-${VERSION}.zip"
    rm -f "BYOD-Win32-${VERSION}.zip"
    zip -r "BYOD-Win64-${VERSION}.zip" Win64
    zip -r "BYOD-Win32-${VERSION}.zip" Win32
)

# create installer
echo "Creating installer..."
(
    cd installers/windows
    bash build_win_installer.sh
)
