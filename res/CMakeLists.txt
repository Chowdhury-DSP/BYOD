juce_add_binary_data(BinaryData SOURCES
    gui.xml
    gui_ios.xml
    
    ui_assets/PowerButton.svg
    ui_assets/info.svg
    ui_assets/swap.svg
    ui_assets/undo-solid.png
    ui_assets/redo-solid.png
    ui_assets/LeftArrow.svg
    ui_assets/RightArrow.svg
    ui_assets/x-solid.svg

    guitar_ml_models/BluesJR_FullD.json
    guitar_ml_models/TS9_FullD.json

    "amp_irs/Fender.wav"
    "amp_irs/Marshall.wav"
    "amp_irs/Bogner.wav"

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

    presets/Centaur.chowpreset
    presets/Default.chowpreset
    presets/ZenDrive.chowpreset
)

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)
