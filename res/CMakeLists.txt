juce_add_binary_data(BinaryData SOURCES
    gui.xml
    PowerButton.svg
    info.svg
    background.jpeg
    undo-solid.png
    redo-solid.png
    LeftArrow.svg
    RightArrow.svg

    guitar_ml_models/BluesJR_FullD.json
    guitar_ml_models/TS9_FullD.json

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

    presets/Default.chowpreset
    presets/ZenDrive.chowpreset
)

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)
