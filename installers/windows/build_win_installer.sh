#!/bin/sh

app_version=$(cut -f 2 -d '=' <<< "$(grep 'CMAKE_PROJECT_VERSION:STATIC' ../../build/CMakeCache.txt)")
echo "Setting app version: $app_version..."

echo "Creating 64-bit installer..."
script_file=BYOD_Install_Script.iss
sed -i "s/##APPVERSION##/${app_version}/g" $script_file
$"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" $script_file
sed -i "s/${app_version}/##APPVERSION##/g" $script_file

echo "Creating 32-bit installer..."
script_file=BYOD_Install_Script_32bit.iss
sed -i "s/##APPVERSION##/${app_version}/g" $script_file
$"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" $script_file
sed -i "s/${app_version}/##APPVERSION##/g" $script_file

echo SUCCESS
