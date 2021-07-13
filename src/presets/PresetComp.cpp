#include "PresetComp.h"

PresetComp::PresetComp (PresetManager& manager) : manager (manager),
                                                  presetsLeft ("", DrawableButton::ImageOnButtonBackground),
                                                  presetsRight ("", DrawableButton::ImageOnButtonBackground)
{
    manager.addListener (this);

    presetBox.setName ("Preset Manager");
    presetBox.setTooltip ("Use this menu to select presets, and to save and manage user presets.");

    setColour (backgroundColourId, Colour (0xFF595C6B));
    setColour (textColourId, Colours::white);

    addAndMakeVisible (presetBox);
    presetBox.setColour (ComboBox::ColourIds::backgroundColourId, Colours::transparentWhite);
    presetBox.setColour (ComboBox::ColourIds::outlineColourId, Colours::white);
    presetBox.setJustificationType (Justification::centred);
    presetBox.setTextWhenNothingSelected ("No Preset selected...");
    loadPresetChoices();

    addChildComponent (presetNameEditor);
    presetNameEditor.setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
    presetNameEditor.setColour (TextEditor::textColourId, Colours::white);
    presetNameEditor.setColour (TextEditor::highlightColourId, Colours::orange.withAlpha (0.4f));
    presetNameEditor.setColour (CaretComponent::caretColourId, Colours::orange);
    presetNameEditor.setFont (Font (16.0f).boldened());
    presetNameEditor.setMultiLine (false, false);
    presetNameEditor.setJustification (Justification::centred);

    auto setupButton = [=, &manager] (DrawableButton& button, Drawable* image, int presetOffset) {
        addAndMakeVisible (button);
        button.setImages (image, image, image);
        button.setWantsKeyboardFocus (false);
        button.setColour (ComboBox::outlineColourId, Colours::transparentBlack);
        button.setColour (TextButton::buttonColourId, Colours::transparentBlack);
        button.onClick = [=, &manager] {
            auto idx = presetBox.getSelectedId() + presetOffset;
            while (idx <= 0)
                idx += manager.getNumPresets();
            while (idx > manager.getNumPresets())
                idx -= manager.getNumPresets();

            manager.setPreset (idx - 1);
        };
    };

    std::unique_ptr<Drawable> leftImage (Drawable::createFromImageData (BinaryData::LeftArrow_svg, BinaryData::LeftArrow_svgSize));
    std::unique_ptr<Drawable> rightImage (Drawable::createFromImageData (BinaryData::RightArrow_svg, BinaryData::RightArrow_svgSize));

    setupButton (presetsLeft, leftImage.get(), -1);
    setupButton (presetsRight, rightImage.get(), 1);

    presetUpdated();
}

PresetComp::~PresetComp()
{
    manager.removeListener (this);
}

void PresetComp::paint (Graphics& g)
{
    constexpr auto cornerSize = 5.0f;

    presetBox.setColour (PopupMenu::ColourIds::backgroundColourId, findColour (backgroundColourId));
    g.setColour (findColour (backgroundColourId));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (22.0f, 0.0f), cornerSize);
}

void PresetComp::resized()
{
    auto b = getLocalBounds();
    presetsLeft.setBounds (b.removeFromLeft (20));
    presetsRight.setBounds (b.removeFromRight (20));

    Rectangle<int> presetsBound { 22, 0, getWidth() - 44, getHeight() };
    presetBox.setBounds (presetsBound);
    presetNameEditor.setBounds (presetsBound);
    repaint();
}

void PresetComp::presetUpdated()
{
    presetBox.setSelectedId (manager.getSelectedPresetIdx() + 1, dontSendNotification);
}

void PresetComp::menuItemAction() const
{
    const auto selectedId = presetBox.getSelectedId();
    if (selectedId >= 1000 || selectedId <= 0)
        return;

    manager.setPreset (selectedId - 1);
}

void PresetComp::loadPresetChoices()
{
    presetBox.getRootMenu()->clear();

    std::map<String, PopupMenu> presetChoicesMap;
    for (int i = 0; i < manager.getNumPresets(); ++i)
    {
        const String& choice = manager.getPresetName (i);
        String category = choice.upToFirstOccurrenceOf ("_", false, false);
        if (category == "User") // user presets are treated specially
            continue;
        category = (category == choice) ? "CHOW" : category;
        String presetName = choice.fromLastOccurrenceOf ("_", false, false);

        if (presetChoicesMap.find (category) == presetChoicesMap.end())
            presetChoicesMap[category] = PopupMenu();

        PopupMenu::Item presetItem { presetName };
        presetItem.itemID = i + 1;
        presetItem.action = std::bind (&PresetComp::menuItemAction, this);

        presetChoicesMap[category].addItem (presetItem);
    }

    for (auto& presetGroup : presetChoicesMap)
        presetBox.getRootMenu()->addSubMenu (presetGroup.first, presetGroup.second);

    // add user presets
    auto& userPresetMenu = manager.getUserPresetMenu (this);
    if (userPresetMenu.containsAnyActiveItems())
        presetBox.getRootMenu()->addSubMenu ("User", userPresetMenu);

    addPresetOptions();
}

void PresetComp::addPresetOptions()
{
    auto menu = presetBox.getRootMenu();
    menu->addSeparator();

    PopupMenu::Item saveItem { "Save" };
    saveItem.itemID = 1001;
    saveItem.action = [=] { saveUserPreset(); };
    menu->addItem (saveItem);

    PopupMenu::Item goToFolderItem { "Go to Preset folder..." };
    goToFolderItem.itemID = 1002;
    goToFolderItem.action = [=] {
        presetUpdated();
        auto folder = manager.getUserPresetFolder();
        if (folder.isDirectory())
            folder.startAsProcess();
        else
            manager.chooseUserPresetFolder();
    };
    menu->addItem (goToFolderItem);

    PopupMenu::Item chooseFolderItem { "Choose Preset folder..." };
    chooseFolderItem.itemID = 1003;
    chooseFolderItem.action = [=] { presetUpdated(); manager.chooseUserPresetFolder(); };
    menu->addItem (chooseFolderItem);
}

void PresetComp::saveUserPreset()
{
    presetNameEditor.setVisible (true);
    presetNameEditor.toFront (true);
    presetNameEditor.setText ("MyPreset");
    presetNameEditor.setHighlightedRegion ({ 0, 10 });

    presetNameEditor.onReturnKey = [=] {
        auto presetName = presetNameEditor.getText();
        presetNameEditor.setVisible (false);

        int pIdx = 0;
        if (manager.saveUserPreset (presetName, pIdx))
        {
            loadPresetChoices();
            manager.setPreset (pIdx);
        }
        else
        {
            presetUpdated();
        }
    };
}
