target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/KnobsComponent.cpp
    gui/ProcessorEditor.cpp

    processors/BaseProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorStore.cpp

    processors/drive/DiodeClipper.cpp

    processors/tone/HighCut.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    