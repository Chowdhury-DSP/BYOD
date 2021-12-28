#include "PresetResaver.h"
#include "BYOD.h"

PresetResaver::PresetResaver()
{
    this->commandOption = "--presets-resave";
    this->argumentDescription = "--presets-resave --dir=[PRESET DIR]";
    this->shortDescription = "Resaves factory presets";
    this->longDescription = "";
    this->command = [=] (const ArgumentList& args)
    { savePresets (args); };
}

void PresetResaver::savePresets (const ArgumentList& args)
{
    File presetsDir = File::getCurrentWorkingDirectory().getChildFile ("res/presets");
    if (args.containsOption ("--dir"))
        presetsDir = args.getExistingFolderForOption ("--dir");

    if (! presetsDir.getChildFile ("Default.chowpreset").existsAsFile())
        ConsoleApplication::fail ("Incorrect presets directory! Unable to resave presets.");

    std::cout << "Resaving presets in directory: " << presetsDir.getFullPathName() << std::endl;
    /*
    std::unique_ptr<BYOD> plugin (dynamic_cast<BYOD*> (createPluginFilterOfType (AudioProcessor::WrapperType::wrapperType_Standalone)));
    auto& stateManager = plugin->getStateManager();

    for (DirectoryEntry file : RangedDirectoryIterator (presetsDir, true))
    {
        auto presetFile = file.getFile();
        if (! presetFile.hasFileExtension ("chowpreset"))
        {
            std::cout << presetFile.getFullPathName() << " is not a preset file!" << std::endl;
            continue;
        }

        std::cout << "Resaving: " << presetFile.getFileName() << std::endl;

        // reset to default state
        stateManager.getPresetManager().setPreset (0);
        MessageManager::getInstance()->runDispatchLoopUntil (10); // pump dispatch loop so changes propagate...

        // load original preset
        auto originalPresetXml = XmlDocument::parse (presetFile);
        int presetIdx = -1;
        if (auto xmlState = originalPresetXml->getChildByName ("state"))
        {
            stateManager.loadState (xmlState);

            if (auto paramsXml = xmlState->getChildByName ("Parameters"))
                if (auto presetParamXml = paramsXml->getChildByAttribute ("id", "preset"))
                    presetIdx = presetParamXml->getIntAttribute ("value");
        }

        if (presetIdx < 0)
        {
            std::cout << "Unable to get preset index!" << std::endl;
            continue;
        }

        MessageManager::getInstance()->runDispatchLoopUntil (10); // pump dispatch loop so changes propagate...

        // save as new preset
        auto newPresetXml = std::make_unique<XmlElement> ("Preset");
        newPresetXml->setAttribute ("name", originalPresetXml->getStringAttribute ("name"));

        auto newPresetXmlState = stateManager.saveState();
        if (auto paramsXml = newPresetXmlState->getChildByName ("Parameters"))
        {
            if (auto presetParamXml = paramsXml->getChildByAttribute ("id", "preset"))
                presetParamXml->setAttribute ("value", presetIdx);
        }

        newPresetXml->addChildElement (newPresetXmlState.release());
        presetFile.replaceWithText (newPresetXml->toString());
    }
*/
}
