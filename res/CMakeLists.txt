juce_add_binary_data(BinaryData SOURCES
    gui.xml
    PowerButton.svg
    info.svg
    background.jpeg

    faceplates/DiodeClipper.png
)

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)
