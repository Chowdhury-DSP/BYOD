#include "AmpIRs.h"

namespace AmpIRFileUtils
{
constexpr chowdsp::GlobalPluginSettings::SettingID userIRFolderID { "user_ir_folder" };

struct IRFileTree : chowdsp::AbstractTree<File>
{
    File rootDirectory {};
    void loadFilesFromDirectory (const File& rootDir, const AudioFormatManager& formatManager)
    {
        rootDirectory = rootDir;

        std::vector<File> irFiles;
        for (const DirectoryEntry& entry : RangedDirectoryIterator (rootDirectory, true, formatManager.getWildcardForAllFormats()))
            irFiles.emplace_back (entry.getFile());

        insertElements (std::move (irFiles));
    }

    File& insertElementInternal (File&& irFile, NodeVector& topLevelNodes) override
    {
        std::vector<std::string> subPaths {};
        File pathToRootDir { irFile.getParentDirectory() };
        while (pathToRootDir != rootDirectory)
        {
            subPaths.emplace_back (pathToRootDir.getFileNameWithoutExtension().toStdString());
            pathToRootDir = pathToRootDir.getParentDirectory();
        }

        return insertFile (std::move (irFile), subPaths, topLevelNodes);
    }

    File& insertFile (File&& file, std::vector<std::string>& subPaths, NodeVector& nodes)
    {
        const auto nodeComparator = [] (const Node& el1, const Node& el2)
        {
            if (el1.leaf.has_value() && ! el2.leaf.has_value())
                return false; // node 1 is a File, node 2 is a sub-dir

            if (! el1.leaf.has_value() && el2.leaf.has_value())
                return true; // node 1 is a sub-dir, node 2 is a leaf

            if (! el1.leaf.has_value())
                return el1.tag < el2.tag; // both nodes are sub-dirs

            return el1.leaf < el2.leaf; // both nodes are files
        };

        if (subPaths.empty())
        {
            Node newFileNode { nodeArena };
            newFileNode.leaf = std::move (file);
            return *chowdsp::VectorHelpers::insert_sorted (nodes, std::move (newFileNode), nodeComparator)->leaf;
        }

        const auto tag = subPaths.back();
        subPaths.pop_back();
        auto existingSubDirNode = std::find_if (nodes.begin(), nodes.end(), [&tag] (const Node& node)
                                                { return node.tag == tag; });
        if (existingSubDirNode != nodes.end())
        {
            return insertFile (std::move (file), subPaths, existingSubDirNode->subtree);
        }

        Node newSubDirNode { nodeArena };
        newSubDirNode.tag = tag;
        auto& insertedSubDirNode = *chowdsp::VectorHelpers::insert_sorted (nodes,
                                                                           std::move (newSubDirNode),
                                                                           nodeComparator);
        return insertFile (std::move (file), subPaths, insertedSubDirNode.subtree);
    }

    static PopupMenu createPopupMenu (int& menuIndex, const NodeVector& nodes, AmpIRs& ampIRs, Component* topLevelComponent)
    {
        PopupMenu menu;

        for (const auto& node : nodes)
        {
            if (node.leaf.has_value())
            {
                const auto irFile = *node.leaf;
                if (! irFile.existsAsFile())
                    continue;

                PopupMenu::Item fileItem;
                fileItem.text = irFile.getFileNameWithoutExtension();
                fileItem.itemID = ++menuIndex;
                fileItem.action = [&ampIRs, topLevelComponent, irFile]
                {
                    ampIRs.loadIRFromStream (irFile.createInputStream(), {}, irFile, topLevelComponent);
                };
                menu.addItem (std::move (fileItem));
            }
            else
            {
                auto subMenu = createPopupMenu (menuIndex, node.subtree, ampIRs, topLevelComponent);
                menu.addSubMenu (node.tag, std::move (subMenu));
            }
        }

        return menu;
    }
};
} // namespace

struct AmpIRsSelector : ComboBox, chowdsp::TrackedByBroadcasters
{
    AmpIRsSelector (AudioProcessorValueTreeState& vtState, AmpIRs& proc, chowdsp::HostContextProvider& hcp)
        : ampIRs (proc), vts (vtState)
    {
        pluginSettings->addProperties<&AmpIRsSelector::globalSettingChanged> ({ { AmpIRFileUtils::userIRFolderID, String() } }, *this);

        refreshUserIRs();
        refreshBox();
        refreshText();
        this->setName (ampIRs.irTag + "__box");

        hcp.registerParameterComponent (*this, *vts.getParameter (ampIRs.irTag));
        onIRChanged = ampIRs.irChangedBroadcaster.connect ([this]
                                                           { refreshText(); });
        onChange = [this]
        { refreshText(); };

        setTextWhenNothingSelected ("Custom IR");
    }

    void visibilityChanged() override
    {
        setName (vts.getParameter (ampIRs.irTag)->name);
    }

    void globalSettingChanged (chowdsp::GlobalPluginSettings::SettingID settingID)
    {
        if (settingID != AmpIRFileUtils::userIRFolderID)
            return;

        refreshUserIRs();
        refreshBox();
    }

    void refreshText()
    {
        setText (ampIRs.irState.name, sendNotification);
        resized();
    }

    void refreshBox()
    {
        clear();
        auto* menu = getRootMenu();
        int menuIdx = 0;

        for (auto [idx, name] : chowdsp::enumerate (ampIRs.irNames))
        {
            if (name == "Custom")
                continue;

            PopupMenu::Item irItem;
            irItem.text = name;
            irItem.itemID = ++menuIdx;
            irItem.action = [this, index = (int) idx]
            {
                loadBuiltInIR (index);
            };
            menu->addItem (std::move (irItem));
        }

        if (userIRFiles.size() > 0)
            menu->addSubMenu ("User:", AmpIRFileUtils::IRFileTree::createPopupMenu (menuIdx, userIRFiles.getNodes(), ampIRs, getTopLevelComponent()));

        menu->addSeparator();

        PopupMenu::Item fromFileItem;
        fromFileItem.text = "Load From File";
        fromFileItem.itemID = ++menuIdx;
        fromFileItem.action = [this]
        { loadIRFromFile(); };
        menu->addItem (std::move (fromFileItem));

#if ! JUCE_IOS
        PopupMenu::Item selectUserFolderItem;
        selectUserFolderItem.text = "Select User IRs Directory";
        selectUserFolderItem.itemID = ++menuIdx;
        selectUserFolderItem.action = [this]
        { selectUserIRsDirectory(); };
        menu->addItem (std::move (selectUserFolderItem));
#endif
    }

    void loadBuiltInIR (int builtInIRParamIndex)
    {
        auto* param = vts.getParameter (ampIRs.irTag);
        const auto newParamValue = param->convertTo0to1 ((float) builtInIRParamIndex);
        param->setValueNotifyingHost (newParamValue);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::leftKey)
        {
            goToPreviousIR();
            return true;
        }

        if (key == juce::KeyPress::rightKey)
        {
            goToNextIR();
            return true;
        }

        return false;
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const auto isScrollingDown = wheel.deltaY < 0.0f;
        if (isScrollingDown)
            goToNextIR();
        else
            goToPreviousIR();
    }

    void goToNextIR()
    {
        // go to the next built-in IR:
        if (ampIRs.irState.paramIndex < AmpIRs::customIRIndex - 1)
        {
            loadBuiltInIR (ampIRs.irState.paramIndex + 1);
            return;
        }

        // go from the last built-in IR to the first user IR:
        if (ampIRs.irState.paramIndex == AmpIRs::customIRIndex - 1)
        {
            // there are no user IRs, so we'll cycle back around to the first built-in IR:
            if (userIRFiles.size() == 0)
            {
                loadBuiltInIR (0);
                return;
            }

            const auto firstUserIR = userIRFiles.getElementByIndex (0);
            jassert (firstUserIR != nullptr); // we checked that the tree isn't empty, so this should not be null!
            ampIRs.loadIRFromStream (firstUserIR->createInputStream(), {}, *firstUserIR, getTopLevelComponent());
            return;
        }

        // go to next user IR:
        const auto currentUserIRIndex = userIRFiles.getIndexForElement (ampIRs.irState.file);
        if (currentUserIRIndex >= 0)
        {
            const auto nextIRFile = userIRFiles.getElementByIndex (currentUserIRIndex + 1);
            if (nextIRFile != nullptr)
            {
                ampIRs.loadIRFromStream (nextIRFile->createInputStream(), {}, *nextIRFile, getTopLevelComponent());
                return;
            }
        }

        // failed to get current or next user IR from tree, so we'll go back to the first built-in IR:
        loadBuiltInIR (0);
    }

    void goToPreviousIR()
    {
        // go to the previous built-in IR:
        if (ampIRs.irState.paramIndex != AmpIRs::customIRIndex && ampIRs.irState.paramIndex > 0)
        {
            loadBuiltInIR (ampIRs.irState.paramIndex - 1);
            return;
        }

        // go from the first built-in IR to the last user IR:
        if (ampIRs.irState.paramIndex == 0)
        {
            // there are no user IRs, so we'll cycle back around to the first built-in IR:
            if (userIRFiles.size() == 0)
            {
                loadBuiltInIR (AmpIRs::customIRIndex - 1);
                return;
            }

            const auto lastUserIR = userIRFiles.getElementByIndex (userIRFiles.size() - 1);
            jassert (lastUserIR != nullptr); // we checked that the tree isn't empty, so this should not be null!
            ampIRs.loadIRFromStream (lastUserIR->createInputStream(), {}, *lastUserIR, getTopLevelComponent());
            return;
        }

        // go to previous user IR:
        const auto currentUserIRIndex = userIRFiles.getIndexForElement (ampIRs.irState.file);
        if (currentUserIRIndex > 0)
        {
            const auto prevIRFile = userIRFiles.getElementByIndex (currentUserIRIndex - 1);
            if (prevIRFile != nullptr)
            {
                ampIRs.loadIRFromStream (prevIRFile->createInputStream(), {}, *prevIRFile, getTopLevelComponent());
                return;
            }
        }

        // failed to get current or previous user IR from tree, so we'll go back to the last built-in IR:
        loadBuiltInIR (AmpIRs::customIRIndex - 1);
    }

    void refreshUserIRs()
    {
        const auto userIRsFolder = File { pluginSettings->getProperty<String> (AmpIRFileUtils::userIRFolderID) };
        if (! userIRsFolder.isDirectory())
            return;

        Logger::writeToLog ("Attempting to load user IRs from folder: " + userIRsFolder.getFullPathName());

        userIRFiles.clear();
        userIRFiles.loadFilesFromDirectory (userIRsFolder, ampIRs.audioFormatManager);
    }

    void loadIRFromFile()
    {
        constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
        fileChooser = std::make_shared<FileChooser> ("Custom IR",
                                                     File(),
                                                     ampIRs.audioFormatManager.getWildcardForAllFormats(),
                                                     true,
                                                     false,
                                                     getTopLevelComponent());
        fileChooser->launchAsync (flags,
                                  [this, safeParent = SafePointer { getParentComponent() }] (const FileChooser& fc)
                                  {
#if JUCE_IOS
                                      if (fc.getURLResults().isEmpty())
                                          return;
                                      const auto irFile = fc.getURLResult();
                                      Logger::writeToLog ("AmpIRs attempting to load IR from local file: " + irFile.getLocalFile().getFullPathName());
                                      ampIRs.loadIRFromStream (irFile.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress)),
                                                               {},
                                                               irFile.getLocalFile(),
                                                               safeParent.getComponent());
#else
                if (fc.getResults().isEmpty())
                    return;
                const auto irFile = fc.getResult();

                Logger::writeToLog ("AmpIRs attempting to load IR from local file: " + irFile.getFullPathName());
                ampIRs.loadIRFromStream (irFile.createInputStream(), {}, irFile, safeParent.getComponent());
#endif
                                  });
    }

    void selectUserIRsDirectory()
    {
        constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
        fileChooser = std::make_shared<FileChooser> ("User IR Folder", File(), "", true, false, getTopLevelComponent());
        fileChooser->launchAsync (flags,
                                  [this] (const FileChooser& fc)
                                  {
                                      if (fc.getResults().isEmpty())
                                          return;
                                      const auto irFile = fc.getResult();
                                      pluginSettings->setProperty (AmpIRFileUtils::userIRFolderID, irFile.getFullPathName());
                                  });
    }

    AmpIRs& ampIRs;
    AudioProcessorValueTreeState& vts;
    std::shared_ptr<FileChooser> fileChooser;
    chowdsp::ScopedCallback onIRChanged;

    chowdsp::SharedPluginSettings pluginSettings;
    AmpIRFileUtils::IRFileTree userIRFiles;
};

bool AmpIRs::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    customComps.add (std::make_unique<AmpIRsSelector> (vts, *this, hcp));
    return false;
}
