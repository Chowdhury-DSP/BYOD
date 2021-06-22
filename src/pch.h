#pragma once

/**
 * Pre-compiled headers for JUCE plugins
 */

// C++/STL headers here...
#include <future>
#include <unordered_map>

// JUCE modules
#include <JuceHeader.h>

// Any other widely used headers that don't change...
#include <magic_enum.hpp>

/** Useful for creating Listener patterns */
#define CREATE_LISTENER(ListenerName, listName, funcs)\
    public:\
        struct ListenerName\
        {\
            virtual ~ListenerName() {}\
            funcs\
        };\
        void add ## ListenerName (ListenerName* l) { listName.add (l); }\
        void remove ## ListenerName (ListenerName* l) { listName.remove (l); }\
    private:\
        ListenerList<ListenerName> listName;
