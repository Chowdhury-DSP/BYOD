if(NOT (IOS OR BUILD_RELEASE))
    message(STATUS "Configuring BYOD headless")
    add_subdirectory(headless)
endif()

target_sources(BYOD PRIVATE
    BYOD.cpp

    gui/BYODPluginEditor.cpp
    gui/TitleBar.cpp

    gui/pedalboard/BoardComponent.cpp
    gui/pedalboard/BoardViewport.cpp
    gui/pedalboard/InfoComponent.cpp
    gui/pedalboard/cables/CableDrawingHelpers.cpp
    gui/pedalboard/cables/CableViewConnectionHelper.cpp
    gui/pedalboard/cables/CableViewPortLocationHelper.cpp
    gui/pedalboard/cables/CableView.cpp
    gui/pedalboard/editors/KnobsComponent.cpp
    gui/pedalboard/editors/Port.cpp
    gui/pedalboard/editors/ProcessorEditor.cpp

    gui/toolbar/ToolBar.cpp
    gui/toolbar/UndoRedoComponent.cpp
    gui/toolbar/SettingsButton.cpp
    gui/toolbar/CPUMeter.cpp
    gui/toolbar/GlobalParamControls.cpp
    gui/toolbar/presets/PresetsComp.cpp
    gui/toolbar/presets/PresetsLoginDialog.cpp
    gui/toolbar/presets/PresetsSaveDialog.cpp
    gui/toolbar/presets/PresetSearchWindow.cpp
    gui/toolbar/presets/PresetsSyncDialog.cpp

    gui/utils/LevelMeterComponent.cpp
    gui/utils/LookAndFeels.cpp

    state/StateManager.cpp
    state/UIState.cpp
    state/ParamForwardManager.cpp
    state/presets/PresetInfoHelpers.cpp
    state/presets/PresetManager.cpp
    state/presets/PresetsServerSyncManager.cpp
    state/presets/PresetsServerUserManager.cpp
    state/presets/PresetsServerCommunication.cpp

    processors/BaseProcessor.cpp
    processors/ProcessorStore.cpp
    
    processors/chain/ChainIOProcessor.cpp
    processors/chain/DryWetProcessor.cpp
    processors/chain/ProcessorChain.cpp
    processors/chain/ProcessorChainActions.cpp
    processors/chain/ProcessorChainActionHelper.cpp
    processors/chain/ProcessorChainPortMagnitudesHelper.cpp
    processors/chain/ProcessorChainStateHelper.cpp

    processors/drive/GuitarMLAmp.cpp
    processors/drive/MetalFace.cpp
    processors/drive/RangeBooster.cpp
    processors/drive/RONN.cpp
    processors/drive/Warp.cpp
    processors/drive/big_muff/BigMuffDrive.cpp
    processors/drive/big_muff/BigMuffClippingStage.cpp
    processors/drive/centaur/Centaur.cpp
    processors/drive/centaur/FeedForward2.cpp
    processors/drive/centaur/GainStageML.cpp
    processors/drive/centaur/GainStageProc.cpp
    processors/drive/centaur/InputBufferProcessor.cpp
    processors/drive/centaur/OutputStageProcessor.cpp
    processors/drive/centaur/PreAmpStage.cpp
    processors/drive/diode_circuits/DiodeClipper.cpp
    processors/drive/diode_circuits/DiodeRectifier.cpp
    processors/drive/hysteresis/Hysteresis.cpp
    processors/drive/hysteresis/HysteresisProcessing.cpp
    processors/drive/king_of_tone/KingOfToneDrive.cpp
    processors/drive/mxr_distortion/MXRDistortion.cpp
    processors/drive/neural_utils/ResampledRNN.cpp
    processors/drive/tube_amp/TubeAmp.cpp
    processors/drive/tube_screamer/TubeScreamer.cpp
    processors/drive/waveshaper/SurgeWaveshapers.cpp
    processors/drive/waveshaper/Waveshaper.cpp
    processors/drive/zen_drive/ZenDrive.cpp

    processors/other/Chorus.cpp
    processors/other/Compressor.cpp
    processors/other/Delay.cpp
    processors/other/EnvelopeFilter.cpp
    processors/other/Gate.cpp
    processors/other/Rotary.cpp
    processors/other/Tremolo.cpp
    processors/other/spring_reverb/SpringReverb.cpp
    processors/other/spring_reverb/SpringReverbProcessor.cpp

    processors/tone/AmpIRs.cpp
    processors/tone/BassCleaner.cpp
    processors/tone/BigMuffTone.cpp
    processors/tone/GraphicEQ.cpp
    processors/tone/HighCut.cpp
    processors/tone/LofiIrs.cpp
    processors/tone/StateVariableFilter.cpp
    processors/tone/TrebleBooster.cpp
    processors/tone/bassman/BassmanTone.cpp
    processors/tone/bassman/BassmanToneStack.cpp
    processors/tone/baxandall/BaxandallEQ.cpp
    processors/tone/baxandall/BaxandallWDF.cpp
    processors/tone/tube_screamer_tone/TubeScreamerTone.cpp

    processors/utility/FreqBandSplitter.cpp
    processors/utility/InputProcessor.cpp
    processors/utility/Mixer.cpp
    processors/utility/Oscilloscope.cpp
    processors/utility/OutputProcessor.cpp
    processors/utility/StereoMerger.cpp
    processors/utility/StereoSplitter.cpp
    processors/utility/Tuner.cpp
)

target_precompile_headers(BYOD PRIVATE pch.h)
