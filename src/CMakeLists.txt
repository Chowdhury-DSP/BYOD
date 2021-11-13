if(NOT (IOS OR MACOS_RELEASE))
    add_subdirectory(headless)
endif()

target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BoardComponent.cpp
    gui/BoardViewport.cpp
    gui/InfoComponent.cpp
    gui/KnobsComponent.cpp
    gui/ProcessorEditor.cpp

    presets/PresetManager.cpp

    processors/BaseProcessor.cpp
    processors/DryWetProcessor.cpp
    processors/ProcessorChain.cpp
    processors/ProcessorChainActions.cpp
    processors/ProcessorStore.cpp

    processors/drive/GuitarMLAmp.cpp
    processors/drive/RONN.cpp
    processors/drive/centaur/Centaur.cpp
    processors/drive/centaur/FeedForward2.cpp
    processors/drive/centaur/GainStageProc.cpp
    processors/drive/centaur/InputBufferProcessor.cpp
    processors/drive/centaur/OutputStageProcessor.cpp
    processors/drive/centaur/PreAmpStage.cpp
    processors/drive/diode_circuits/DiodeClipper.cpp
    processors/drive/diode_circuits/DiodeRectifier.cpp
    processors/drive/hysteresis/Hysteresis.cpp
    processors/drive/hysteresis/HysteresisProcessing.cpp
    processors/drive/mxr_distortion/MXRDistortion.cpp
    processors/drive/tube_amp/TubeAmp.cpp
    processors/drive/tube_screamer/TubeScreamer.cpp
    processors/drive/zen_drive/ZenDrive.cpp

    processors/other/Chorus.cpp
    processors/other/Delay.cpp
    processors/other/EnvelopeFilter.cpp
    processors/other/Tremolo.cpp
    processors/other/spring_reverb/SpringReverb.cpp
    processors/other/spring_reverb/SpringReverbProcessor.cpp

    processors/tone/AmpIRs.cpp
    processors/tone/BassCleaner.cpp
    processors/tone/HighCut.cpp
    processors/tone/LofiIrs.cpp
    processors/tone/TrebleBooster.cpp
    processors/tone/bassman/BassmanTone.cpp

    processors/utility/FreqBandSplitter.cpp
    processors/utility/Mixer.cpp
    processors/utility/StereoMerger.cpp
    processors/utility/StereoSplitter.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
    