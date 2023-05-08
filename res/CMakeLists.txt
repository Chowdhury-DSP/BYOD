set(binary_data_files
    ui_assets/PowerButton.svg
    ui_assets/info.svg
    ui_assets/swap.svg
    ui_assets/undo-solid.png
    ui_assets/redo-solid.png
    ui_assets/LeftArrow.svg
    ui_assets/RightArrow.svg
    ui_assets/x-solid.svg
    ui_assets/cog-solid.svg
    ui_assets/magnifying-glass-minus-solid.svg
    ui_assets/magnifying-glass-plus-solid.svg

    guitar_ml_models/BluesJrAmp_VolKnob.json
    guitar_ml_models/MesaRecMini_ModernChannel_GainKnob.json
    guitar_ml_models/TS9_DriveKnob.json
    guitar_ml_models/metal_face_model.json
    guitar_ml_models/junior_1_stage.json
    guitar_ml_models/bass_face_model_88_2k.json
    guitar_ml_models/bass_face_model_96k.json
    guitar_ml_models/centaur/centaur_0.json
    guitar_ml_models/centaur/centaur_25.json
    guitar_ml_models/centaur/centaur_50.json
    guitar_ml_models/centaur/centaur_75.json
    guitar_ml_models/centaur/centaur_100.json

    "amp_irs/Fender.wav"
    "amp_irs/Marshall.wav"
    "amp_irs/Bogner.wav"
    "amp_irs/Bass.wav"

    "lofi_irs/Casio 1.wav"
    "lofi_irs/Casio 2.wav"
    "lofi_irs/Cassette Speaker.wav"
    "lofi_irs/Toy Circuits.wav"
    "lofi_irs/Toy Mic.wav"
    "lofi_irs/Toy Speaker.wav"
    "lofi_irs/Yamaha 1.wav"
    "lofi_irs/Yamaha 2.wav"
    "lofi_irs/Yamaha 3.wav"
    "lofi_irs/Yamaha 4.wav"
)

file(GLOB PRESET_FILES presets/*.chowpreset)
list(APPEND binary_data_files ${PRESET_FILES})

file(GLOB SCHEMATIC_FILES schematics/*.svg)
list(APPEND binary_data_files ${SCHEMATIC_FILES})

juce_add_binary_data(BinaryData SOURCES ${binary_data_files})

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
