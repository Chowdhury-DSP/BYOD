add_subdirectory(headless)

target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/BoardViewport.cpp
    gui/InfoComponent.cpp
    gui/KnobsComponent.cpp
    gui/ProcessorEditor.cpp

    presets/PresetComp.cpp
    presets/PresetManager.cpp

    processors/BaseProcessor.cpp
    processors/DryWetProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorChainActions.cpp
    processors/ProcessorStore.cpp

    processors/drive/diode_circuits/DiodeClipper.cpp
    processors/drive/diode_circuits/DiodeRectifier.cpp
    processors/drive/hysteresis/Hysteresis.cpp
    processors/drive/hysteresis/HysteresisProcessing.cpp
    processors/drive/mxr_distortion/MXRDistortion.cpp
    processors/drive/tube_amp/TubeAmp.cpp
    processors/drive/tube_screamer/TubeScreamer.cpp
    processors/drive/zen_drive/ZenDrive.cpp
    processors/drive/GuitarMLAmp.cpp
    processors/drive/RONN.cpp

    processors/other/EnvelopeFilter.cpp
    processors/other/Tremolo.cpp

    processors/tone/bassman/BassmanTone.cpp
    processors/tone/BassCleaner.cpp
    processors/tone/HighCut.cpp
    processors/tone/TrebleBooster.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    