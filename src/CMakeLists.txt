add_subdirectory(headless)

target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/InfoComponent.cpp
    gui/KnobsComponent.cpp
    gui/ProcessorEditor.cpp

    processors/BaseProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorChainActions.cpp
    processors/ProcessorStore.cpp

    processors/drive/diode_circuits/DiodeClipper.cpp
    processors/drive/diode_circuits/DiodeRectifier.cpp
    processors/drive/mxr_distortion/MXRDistortion.cpp
    processors/drive/tube_screamer/TubeScreamer.cpp
    processors/drive/zen_drive/ZenDrive.cpp
    processors/drive/RONN.cpp

    processors/tone/bassman/BassmanTone.cpp
    processors/tone/BassCleaner.cpp
    processors/tone/HighCut.cpp
    processors/tone/TrebleBooster.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    