#pragma once

/**
 * Pre-compiled headers for JUCE plugins
 */

// C++/STL headers here...
#include <future>
#include <random>
#include <unordered_map>

#include <magic_enum.hpp> // Needs to be included before JUCE

// JUCE modules
#include <JuceHeader.h>

// Any other widely used headers that don't change...
#include <FuzzySearchDatabase.hpp>
#include <RTNeural/RTNeural.h>
#include <chowdsp_wdf/chowdsp_wdf.h>
#include <ea_variant/ea_variant.h>
#include <sst/cpputils.h>

// global definitions
using Parameters = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;
using ParamLayout = AudioProcessorValueTreeState::ParameterLayout;
namespace wdft = chowdsp::wdft;

namespace chowdsp
{
using namespace VersionUtils;
}

#if ! PERFETTO
#define TRACE_DSP(...) void()
#define TRACE_COMPONENT(...) void()
#endif
