#!/bin/bash

set -e
TEAM_ID=$(more ~/Developer/mac_id)

if [ "$1" == "help" ]; then
  echo "Run bash ios_build.sh build clean"
  echo "Run bash ios_build.sh version"
  echo "Go to Xcode Archive Organizer and upload!"
  exit
fi

if [ "$1" == "build" ] || [ "$1" == "configure" ]; then
echo "Running CMake configuration..."

# clean up old builds
if [ "$2" == "clean" ]; then rm -Rf build-ios; fi

# generate new builds
cmake -Bbuild-ios -GXcode -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.4 \
    -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$TEAM_ID" \
    -DCMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY="1,2" \
    -DCMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE="NO" \
    -DBYOD_BUILD_ADD_ON_MODULES=ON

if [ "$1" == "build" ]; then
xcodebuild -project build-ios/BYOD.xcodeproj \
  -scheme BYOD_Standalone archive -configuration Release \
  -sdk iphoneos -jobs 12 -archivePath BYOD.xcarchive | xcpretty
fi
fi

if [ "$1" == "version" ]; then
  # set version number to include commit hash
  COMMIT=$(git log --pretty=format:'%h' -n 1)
  VERSION=$(cut -f 2 -d '=' <<< "$(grep 'CMAKE_PROJECT_VERSION:STATIC' build-ios/CMakeCache.txt)")
  BUILD_NUMBER="$VERSION-$COMMIT"
  echo "Setting version for archive: $BUILD_NUMBER"

  PLIST=BYOD.xcarchive/Info.plist
  /usr/libexec/Plistbuddy -c "Set ApplicationProperties:CFBundleVersion $BUILD_NUMBER" "$PLIST"

  # move to archives folder so Xcode can find it
  archive_dir="$HOME/Library/Developer/Xcode/Archives/$(date '+%Y-%m-%d')"
  echo "Moving to directory: $archive_dir"
  mkdir -p "$archive_dir"
  mv BYOD.xcarchive "$archive_dir/BYOD-$COMMIT.xcarchive"
fi

