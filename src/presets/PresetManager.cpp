#include "PresetManager.h"
#include "PresetComp.h"
#include "processors/ProcessorChain.h"

namespace
{
static String userPresetPath = "ChowdhuryDSP/BYOD/UserPresets.txt";
static String presetTag = "preset";

class PresetParameter : public AudioParameterInt
{
public:
    PresetParameter (const String& parameterID, const String& parameterName, int minValue, int maxValue, int defaultValue) : AudioParameterInt (parameterID, parameterName, minValue, maxValue, defaultValue)
    {
    }

    bool isAutomatable() const override { return false; }
};

} // namespace

PresetManager::PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vts) : procChain (chain)
{
    presetParam = dynamic_cast<AudioParameterInt*> (vts.getParameter (presetTag));
    jassert (presetParam);

    loadingFuture = std::async (std::launch::async, &PresetManager::loadPresets, this);
}

StringArray PresetManager::getPresetChoices()
{
    StringArray choices;
    for (auto p : presetMap)
        choices.add (p.second->name);

    return choices;
}

void PresetManager::loadPresets()
{
    presets.add (std::make_unique<Preset> ("Default.chowpreset"));
    presets.add (std::make_unique<Preset> ("ZenDrive.chowpreset"));
    numFactoryPresets = presets.size();

    for (auto* p : presets)
    {
        jassert (presetMap.find (p->index) == presetMap.end()); // no two presets can have the same index
        presetMap.insert ({ p->index, p });
        maxIdx = jmax (maxIdx, p->index);
    }

    maxIdx++;

    updateUserPresets();
}

void PresetManager::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<PresetParameter> (presetTag, "Preset", 0, 1000, 0));
}

String PresetManager::getPresetName (int idx)
{
    if (! isPositiveAndBelow (idx, presets.size())) // invalid index
    {
        jassertfalse;
        return {};
    }

    return presetMap[idx]->name;
}

bool PresetManager::setPreset (int idx)
{
    if (! isPositiveAndBelow (idx, presets.size())) // invalid index
    {
        jassertfalse;
        return false;
    }

    if (auto xmlState = presetMap[idx]->state->getChildByName ("proc_chain"))
        procChain->loadProcChain (xmlState);

    presetParam->setValueNotifyingHost (presetParam->convertTo0to1 ((float) idx));
    listeners.call (&Listener::presetUpdated);

    if (idx < numFactoryPresets)
        hostUpdateFunc();

    return true;
}

bool PresetManager::saveUserPreset (const String& name, int& newPresetIdx)
{
    auto xmlState = procChain->saveProcChain();

    if (! userPresetFolder.isDirectory()) // if not set, choose preset folder
        chooseUserPresetFolder();

    if (! userPresetFolder.isDirectory()) // user doesn't want to choose preset folder, cancelling...
        return false;

    // create file to save preset
    File saveFile = userPresetFolder.getChildFile (name + ".chowpreset");
    saveFile.deleteFile();
    auto result = saveFile.create();

    if (result.failed()) // unable to create file;
        return false;

    // create preset XML
    auto presetXml = std::make_unique<XmlElement> ("Preset");
    const auto presetName = "User_" + name;
    presetXml->setAttribute ("name", presetName);
    presetXml->addChildElement (xmlState.release());

    saveFile.replaceWithText (presetXml->toString());
    updateUserPresets();

    for (auto& p : presetMap)
        if (p.second->name == presetName)
            newPresetIdx = p.first;

    return true;
}

File PresetManager::getUserPresetConfigFile() const
{
    File updatePresetFile = File::getSpecialLocation (File::userApplicationDataDirectory);
    return updatePresetFile.getChildFile (userPresetPath);
}

void PresetManager::chooseUserPresetFolder()
{
    FileChooser chooser ("Choose preset folder");
    if (chooser.browseForDirectory())
    {
        auto result = chooser.getResult();
        auto config = getUserPresetConfigFile();
        config.deleteFile();
        config.create();
        config.replaceWithText (result.getFullPathName());
        updateUserPresets();
    }
}

void PresetManager::loadPresetFolder (PopupMenu& menu, File& directory)
{
    Array<File> presetFiles;
    for (auto& userPreset : directory.findChildFiles (File::findFilesAndDirectories, false))
    {
        if (userPreset.isDirectory())
        {
            auto relativePath = userPreset.getRelativePathFrom (userPresetFolder);
            auto firstSubfolder = relativePath.fromLastOccurrenceOf (File::getSeparatorString(), false, false);

            PopupMenu subMenu;
            loadPresetFolder (subMenu, userPreset);
            menu.addSubMenu (firstSubfolder, subMenu);
        }

        if (userPreset.hasFileExtension (".chowpreset"))
            presetFiles.add (userPreset);
    }

    for (auto& userPreset : presetFiles)
    {
        auto relativePath = userPreset.getRelativePathFrom (userPresetFolder);
        auto newPreset = presets.add (std::make_unique<Preset> (userPreset));
        newPreset->index = maxIdx;
        presetMap.insert ({ newPreset->index, newPreset });
        menu.addItem (newPreset->index + 1, newPreset->name.fromFirstOccurrenceOf ("User_", false, false));
        maxIdx++;
    }
}

const PopupMenu& PresetManager::getUserPresetMenu (const PresetComp* comp) const
{
    PopupMenu::MenuItemIterator iter (userPresetMenu, true);
    while (iter.next())
        iter.getItem().action = std::bind (&PresetComp::menuItemAction, comp);

    return userPresetMenu;
}

void PresetManager::updateUserPresets()
{
    // set preset folder
    auto config = getUserPresetConfigFile();
    if (config.existsAsFile())
        userPresetFolder = File (config.loadFileAsString());
    else
        userPresetFolder = File();

    // remove existing user presets
    presets.removeRange (numFactoryPresets, maxIdx - numFactoryPresets);
    for (; maxIdx > numFactoryPresets; maxIdx--)
        presetMap.erase (maxIdx - 1);
    userPresetMenu.clear();

    if (userPresetFolder.isDirectory())
        loadPresetFolder (userPresetMenu, userPresetFolder);
}
