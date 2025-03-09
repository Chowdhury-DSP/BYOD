#!/bin/bash

# # expand bash aliases
# shopt -s expand_aliases
# source ~/.bashrc

# exit on failure
set -e

TARGET_NAME="BYOD"
PLUGIN_NAME="BYOD"

# need to run in sudo mode on Mac
if [[ "$OSTYPE" == "darwin"* ]]; then
    if [ "$EUID" -ne 0 ]; then
        echo "This script must be run in sudo mode! Exiting..."
        exit 1
    fi
fi

if [[ "$*" = *debug* ]]; then
    echo "Making DEBUG build"
    build_config="Debug"
else
    echo "Making RELEASE build"
    build_config="Release"
fi

# clean up old builds
if [[ $* = *clean* ]]; then
    echo "Cleaning previous build..."
    rm -rf build-aax/
fi

sed_cmakelist()
{
    sed_args="$1"

    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "$sed_args" CMakeLists.txt
    else
        sed -i -e "$sed_args" CMakeLists.txt
    fi
}

# set up OS-dependent variables
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Building for MAC"

    AAX_PATH=~/Developer/AAX_SDK/
    ilok_pass=$(more ~/Developer/ilok_pass)
    aax_target_dir="/Library/Application Support/Avid/Audio/Plug-Ins"
    TEAM_ID=$(more ~/Developer/mac_id)
    TARGET_DIR="Mac"

else # Windows
    echo "Building for WINDOWS"

    AAX_PATH=C:/Users/Jatin/SDKs/AAX_SDK_clang/
    ilok_pass=$(cat ~/ilok_pass)
    aax_target_dir="/c/Program Files/Common Files/Avid/Audio/Plug-Ins"
    TARGET_DIR="Win64"
fi

# set up build AAX
sed_cmakelist "s~# juce_set_aax_sdk_path.*~juce_set_aax_sdk_path(${AAX_PATH})~"

# cmake new builds
if [[ "$OSTYPE" == "darwin"* ]]; then
    cmake -Bbuild-aax -GXcode -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Developer ID Application" \
        -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$TEAM_ID" \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE="Manual" \
        -D"CMAKE_OSX_ARCHITECTURES=arm64;x86_64" \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS=NO \
        -DCMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS="--timestamp" \
        -DBYOD_BUILD_ADD_ON_MODULES=ON \
        -DBUILD_RELEASE=ON

    cmake --build build-aax --config $build_config -j12 --target "${TARGET_NAME}_AAX" | xcpretty

else # Windows
    cmake -Bbuild-aax -G"Ninja Multi-Config" -DBYOD_BUILD_ADD_ON_MODULES=ON \
        -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_RC_COMPILER=llvm-rc
    cmake --build build-aax --config $build_config --parallel $(nproc) --target "${TARGET_NAME}_AAX"
fi

# sign with PACE
aax_location="build-aax/${TARGET_NAME}_artefacts/$build_config/AAX/${PLUGIN_NAME}.aaxplugin"
wcguid="3B4CFCF0-9C0A-11EC-BE44-005056920FF7"
if [[ "$OSTYPE" == "darwin"* ]]; then
    wraptool sign --verbose \
        --account chowdsp \
        --password "$ilok_pass" \
        --wcguid $wcguid \
        --dsig1-compat off \
        --signid "Developer ID Application: Jatin Chowdhury" \
        --in $aax_location \
        --out $aax_location

        # --keyfile $HOME/Downloads/jatin_aax_cert.p12 \
        # --keypassword "$ilok_pass" \
    wraptool verify --verbose --in $aax_location

else # Windows
    wraptool sign --verbose \
        --account chowdsp \
        --password "$ilok_pass" \
        --wcguid $wcguid \
        --keyfile ~/jatin_aax_cert.p12 \
        --keypassword "$ilok_pass" \
        --in $aax_location \
        --out $aax_location

    wraptool verify --verbose --in "$aax_location/Contents/x64/${PLUGIN_NAME}.aaxplugin"
fi

# reset AAX SDK field...
sed_cmakelist "s~juce_set_aax_sdk_path.*~# juce_set_aax_sdk_path(NONE)~"

rm -rf "$aax_target_dir/${PLUGIN_NAME}.aaxplugin"
cp -R "$aax_location" "$aax_target_dir/${PLUGIN_NAME}.aaxplugin"

if [[ "$*" = *deploy* ]]; then
    set +e
    if [[ "$OSTYPE" == "darwin"* ]]; then
        ssh "jatin@ccrma-gate.stanford.edu" "rm -r ~/aax_builds/${TARGET_DIR}/${PLUGIN_NAME}.aaxplugin"
        scp -r "$aax_location" "jatin@ccrma-gate.stanford.edu:~/aax_builds/${TARGET_DIR}/"
    else
        cd ~/ChowDSP/aax-builds-win64
        git pull
        cp -r "$aax_target_dir/${PLUGIN_NAME}.aaxplugin" .
        git add .
        git commit -am "Update BYOD AAX build"
        git push
    fi
fi
