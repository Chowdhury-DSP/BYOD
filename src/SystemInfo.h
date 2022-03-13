#pragma once

#include "BYOD.h"

namespace SystemInfo
{
inline String getDiagnosticsString (const BYOD& proc)
{
    String diagString;
    diagString += "Version: " + proc.getName() + " " + String (JucePlugin_VersionString) + "\n";
    diagString += "Commit: " + String (BYOD_GIT_COMMIT_HASH) + " on " + String (BYOD_GIT_BRANCH)
                  + " with JUCE version " + SystemStats::getJUCEVersion() + "\n";

    // build system info
    diagString += "Build: " + Time::getCompilationDate().toString (true, true, false, true)
                  + " on " + String (BYOD_BUILD_FQDN)
                  + " with " + String (BYOD_CXX_COMPILER_ID) + "-" + String (BYOD_CXX_COMPILER_VERSION) + "\n";

    // user system info
    diagString += "System: " + SystemStats::getDeviceDescription()
                  + " with " + SystemStats::getOperatingSystemName()
                  + (SystemStats::isOperatingSystem64Bit() ? " (64-bit)" : String())
                  + (SystemStats::isRunningInAppExtensionSandbox() ? " (Sandboxed)" : String())
                  + " on " + String (SystemStats::getNumCpus()) + " Core, " + SystemStats::getCpuModel() + "\n";

    // plugin info
    PluginHostType hostType {};
    diagString += "Plugin Info: " + String (proc.getWrapperTypeString())
                  + " running in " + String (hostType.getHostDescription())
                  + " running at " + String (proc.getSampleRate() / 1000.0, 1) + " kHz "
                  + "with block size: " + String (proc.getBlockSize()) + "\n";

    return diagString;
}
} // namespace SystemInfo
