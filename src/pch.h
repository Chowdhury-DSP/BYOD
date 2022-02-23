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

#include "clap-juce-extensions/clap-juce-extensions.h"

// Any other widely used headers that don't change...
#include <Eigen/Dense>
#include <RTNeural/RTNeural.h>
#include <magic_enum.hpp>
#include <rapidfuzz/fuzz.hpp>
#include <sst/cpputils.h>

// global definitions
using Parameters = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;
namespace wdft = chowdsp::WDFT;

// Useful for creating Listener patterns
#define CREATE_LISTENER(ListenerName, listName, funcs)                   \
public:                                                                  \
    struct ListenerName                                                  \
    {                                                                    \
        virtual ~ListenerName() {}                                       \
        funcs                                                            \
    };                                                                   \
    void add##ListenerName (ListenerName* l) { listName.add (l); }       \
    void remove##ListenerName (ListenerName* l) { listName.remove (l); } \
                                                                         \
private:                                                                 \
    ListenerList<ListenerName> listName;
