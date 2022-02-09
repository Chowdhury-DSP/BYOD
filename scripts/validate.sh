#!/bin/bash

set -e

pluginval_version="v0.3.0"

# install functions
install_pluginval_linux()
{
    curl -L "https://github.com/Tracktion/pluginval/releases/download/${pluginval_version}/pluginval_Linux.zip" -o pluginval.zip
    unzip pluginval > /dev/null
    echo "./pluginval"
}

install_pluginval_mac()
{
    curl -L "https://github.com/Tracktion/pluginval/releases/download/${pluginval_version}/pluginval_macOS.zip" -o pluginval.zip
    unzip pluginval > /dev/null
    echo "pluginval.app/Contents/MacOS/pluginval"
}

install_pluginval_win()
{
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/Tracktion/pluginval/releases/download/${pluginval_version}/pluginval_Windows.zip -OutFile pluginval.zip"
    powershell -Command "Expand-Archive pluginval.zip -DestinationPath ."
    echo "./pluginval.exe"
}

# install
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    exit 0
    # pluginval=$(install_pluginval_linux)
    # declare -a plugins=()
elif [[ "$OSTYPE" == "darwin"* ]]; then
    pluginval=$(install_pluginval_mac)
    declare -a plugins=("build/BYOD_artefacts/VST3/BYOD.vst3")
else
    pluginval=$(install_pluginval_win)
    declare -a plugins=("build/BYOD_artefacts/Debug/VST3/BYOD.vst3")
fi

echo "Pluginval installed at ${pluginval}"

# run
for plugin in "${plugins[@]}"; do
    echo "Validating ${plugin}"
    $pluginval --strictness-level 8 --validate-in-process --validate $plugin
done

# clean up
rm -Rf pluginval*
