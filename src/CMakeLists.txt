target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/ProcessorEditor.cpp

    processors/BaseProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorStore.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    