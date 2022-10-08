#pragma once

/**
 * Pre-compiled headers for JUCE plugins
 */

// C++/STL headers here...
#include <future>
#include <random>
#include <unordered_map>

// JUCE modules
#include <JuceHeader.h>

// Any other widely used headers that don't change...
#include <Eigen/Dense>
#include <RTNeural/RTNeural.h>
#include <chowdsp_wdf/chowdsp_wdf.h>
#include <magic_enum.hpp>
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter")
#include <rapidfuzz/fuzz.hpp>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <sst/cpputils.h>

// global definitions
using Parameters = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;
using ParamLayout = AudioProcessorValueTreeState::ParameterLayout;
namespace wdft = chowdsp::wdft;

namespace chowdsp
{
using namespace VersionUtils;
}
