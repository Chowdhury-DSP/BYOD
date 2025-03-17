#include "AmpIRs.h"

namespace AmpIRFileUtils
{
constexpr chowdsp::GlobalPluginSettings::SettingID userIRFolderID { "user_ir_folder" };

struct IRFileTree : chowdsp::AbstractTree<File, IRFileTree>
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

    static File& insertElementInternal(IRFileTree& self, File&& irFile, Node& root)
    {
        return self.insertElementInternal (std::move (irFile), root);
    }

    File& insertFile (File&& file, std::vector<std::string>& subPaths, Node& root)
    {
        const auto nodeComparator = [] (const Node& el1, const Node& el2)
        {
            if (el1.value.has_value() && el2.value.is_tag())
                return false; // node 1 is a File, node 2 is a sub-dir

            if (el1.value.is_tag() && el2.value.has_value())
                return true; // node 1 is a sub-dir, node 2 is a leaf

            if (el1.value.is_tag())
                return el1.value.tag() < el2.value.tag(); // both nodes are sub-dirs

            return el1.value.leaf() < el2.value.leaf(); // both nodes are files
        };

        if (subPaths.empty())
        {
            Node* newFileNode = createLeafNode (std::move (file));
            insertNodeSorted (root, newFileNode, nodeComparator);
            return newFileNode->value.leaf();
        }

        const auto tag = subPaths.back();
        subPaths.pop_back();
        for (auto* node = root.first_child; node != nullptr; node = node->next_sibling)
        {
            if (node->value.is_tag() && node->value.tag() == tag)
                return insertFile (std::move (file), subPaths, *node);
        }

        Node* newSubDirNode = createTagNode (tag);
        insertNodeSorted (root, newSubDirNode, nodeComparator);
        return insertFile (std::move (file), subPaths, *newSubDirNode);
    }

    File& insertElementInternal (File&& irFile, Node& root)
    {
        std::vector<std::string> subPaths {};
        File pathToRootDir { irFile.getParentDirectory() };
        while (pathToRootDir != rootDirectory)
        {
            subPaths.emplace_back (pathToRootDir.getFileNameWithoutExtension().toStdString());
            pathToRootDir = pathToRootDir.getParentDirectory();
        }

        return insertFile (std::move (irFile), subPaths, root);
    }

    static PopupMenu createPopupMenu (int& menuIndex, const Node& root, AmpIRs& ampIRs, Component* topLevelComponent)
    {
        PopupMenu menu;

        for (auto* node = root.first_child;
            node != nullptr;
            node = node->next_sibling)
        {
            if (node->value.has_value())
            {
                const auto irFile = node->value.leaf();
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
                auto subMenu = createPopupMenu (menuIndex, *node, ampIRs, topLevelComponent);
                menu.addSubMenu (chowdsp::toString (node->value.tag()), std::move (subMenu));
            }
        }

        return menu;
    }

    juce::File first (const Node* root = nullptr) const
    {
        if (root == nullptr)
            root = &getRootNode();

        for (auto* node = root->first_child; node != nullptr; node = node->next_sibling)
        {
            if (node->value.is_tag())
                return first (node);

            if (node->value.has_value())
                return node->value.leaf();
        }

        return {};
    }

    juce::File last (const Node* root = nullptr) const
    {
        if (root == nullptr)
            root = &getRootNode();

        for (auto* node = root->first_child; node != nullptr; node = node->next_sibling)
        {
            if (node->next_sibling == nullptr && node->value.is_tag())
                return first (node);

            if (node->next_sibling == nullptr && node->value.has_value())
                return node->value.leaf();
        }

        return {};
    }

    const Node* find (const juce::File& file) const
    {
        for (const Node* node = &getRootNode(); node != nullptr; node = node->next_linear)
        {
            if (node->value.has_value() && node->value.leaf() == file)
                return node;
        }
        return nullptr;
    }
};
} // namespace AmpIRFileUtils

struct AmpIRsSelector : ComboBox, chowdsp::TrackedByBroadcasters
{
    AmpIRsSelector (AudioProcessorValueTreeState& vtState, AmpIRs& proc, chowdsp::HostContextProvider& hcp)
        : ampIRs (proc), vts (vtState)
    {
        pluginSettings->addProperties<&AmpIRsSelector::globalSettingChanged> ({ { AmpIRFileUtils::userIRFolderID, String() } }, *this);

        refreshUserIRs();
        refreshBox();
        refreshText();
        Component::setName (ampIRs.irTag + "__box");

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
            menu->addSubMenu ("User:", AmpIRFileUtils::IRFileTree::createPopupMenu (menuIdx, userIRFiles.getRootNode(), ampIRs, getTopLevelComponent()));

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

            const auto firstUserIR = userIRFiles.first();
            jassert (firstUserIR != juce::File{}); // we checked that the tree isn't empty, so this should not be null!
            ampIRs.loadIRFromStream (firstUserIR.createInputStream(), {}, firstUserIR, getTopLevelComponent());
            return;
        }

        // go to next user IR:
        if (auto* currentFileNode = userIRFiles.find (ampIRs.irState.file))
        {
            if (auto* nextFileNode = currentFileNode->next_sibling;
                nextFileNode != nullptr && nextFileNode->value.has_value())
            {
                const auto nextIRFile = nextFileNode->value.leaf();
                ampIRs.loadIRFromStream (nextIRFile.createInputStream(), {}, nextIRFile, getTopLevelComponent());
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

            const auto lastUserIR = userIRFiles.last();
            jassert (lastUserIR != juce::File{}); // we checked that the tree isn't empty, so this should not be null!
            ampIRs.loadIRFromStream (lastUserIR.createInputStream(), {}, lastUserIR, getTopLevelComponent());
            return;
        }

        // go to previous user IR:
        if (auto* currentFileNode = userIRFiles.find (ampIRs.irState.file))
        {
            if (auto* prevFileNode = currentFileNode->prev_sibling;
                prevFileNode != nullptr && prevFileNode->value.has_value())
            {
                const auto prevIRFile = prevFileNode->value.leaf();
                ampIRs.loadIRFromStream (prevIRFile.createInputStream(), {}, prevIRFile, getTopLevelComponent());
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
