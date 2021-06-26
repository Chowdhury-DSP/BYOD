target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/KnobsComponent.cpp
    gui/ProcessorEditor.cpp

    processors/BaseProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorStore.cpp

    processors/drive/diode_circuits/DiodeClipper.cpp
    processors/drive/diode_circuits/DiodeRectifier.cpp
    processors/drive/zen_drive/ZenDrive.cpp

    processors/tone/HighCut.cpp
    processors/tone/TrebleBooster.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    