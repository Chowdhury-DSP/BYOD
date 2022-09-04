#include "AmpIRs.h"
#include "../ParameterHelpers.h"

namespace
{
const StringArray irNames {
    "Fender",
    "Marshall",
    "Bogner",
    "Custom",
};

const String irTag = "ir";
const String mixTag = "mix";
const String gainTag = "gain";
} // namespace

AmpIRs::AmpIRs (UndoManager* um) : BaseProcessor ("Amp IRs", createParameterLayout(), um),
                                   convolution (getSharedConvolutionMessageQueue())
{
    for (const auto& irName : StringArray (irNames.begin(), irNames.size() - 1))
    {
        auto binaryName = irName.replaceCharacter (' ', '_') + "_wav";
        int binarySize;
        auto* irData = BinaryData::getNamedResource (binaryName.getCharPointer(), binarySize);

        irMap.insert (std::make_pair (irName, std::make_pair ((void*) irData, (size_t) binarySize)));
    }

    vts.addParameterListener (irTag, this);
    mixParam = vts.getRawParameterValue (mixTag);
    gainParam = vts.getRawParameterValue (gainTag);

    parameterChanged (irTag, vts.getRawParameterValue (irTag)->load());

    uiOptions.backgroundColour = Colours::darkcyan.brighter (0.1f);
    uiOptions.powerColour = Colours::red.brighter (0.0f);
    uiOptions.info.description = "A collection of impulse responses from guitar cabinets.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AmpIRs::~AmpIRs()
{
    vts.removeParameterListener (irTag, this);
}

ParamLayout AmpIRs::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    params.push_back (std::make_unique<AudioParameterChoice> (irTag,
                                                              "IR",
                                                              irNames,
                                                              0));

    createGainDBParameter (params, gainTag, "Gain", -18.0f, 18.0f, 0.0f);
    createPercentParameter (params, mixTag, "Mix", 1.0f);

    return { params.begin(), params.end() };
}

void AmpIRs::setMakeupGain (float irSampleRate)
{
    makeupGainDB.store (Decibels::gainToDecibels (std::sqrt (irSampleRate / fs)));
}

void AmpIRs::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID != irTag)
        return;

    auto irIdx = (int) newValue;
    if (irIdx >= irNames.size() - 1)
        return;

    const auto& irName = irNames[irIdx];
    auto& irData = irMap[irName];
    curFile = File();

    setMakeupGain (96000.0f);

    ScopedLock sl (irMutex);
    convolution.loadImpulseResponse (irData.first, irData.second, dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::yes, 0);
}

void AmpIRs::loadIRFromFile (const File& file)
{
    auto failToLoad = [this] (const File& f, const String& message)
    {
        irFiles.removeAllInstancesOf (f);
        AlertWindow::showMessageBoxAsync (AlertWindow::AlertIconType::WarningIcon, "Unable to load IR!", message);
        vts.getParameter (irTag)->setValueNotifyingHost (0.0f);
        curFile = File();
    };

    if (! file.existsAsFile())
    {
        failToLoad (file, "The following IR file was not found: " + file.getFullPathName());
        return;
    }

    AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> formatReader (manager.createReaderFor (std::make_unique<FileInputStream> (file)));

    if (formatReader == nullptr)
    {
        failToLoad (file, "The following IR file was not valid: " + file.getFullPathName());
        return;
    }

    const auto fileLength = static_cast<size_t> (formatReader->lengthInSamples);
    AudioBuffer<float> resultBuffer { jlimit (1, 2, static_cast<int> (formatReader->numChannels)), static_cast<int> (fileLength) };

    if (! formatReader->read (resultBuffer.getArrayOfWritePointers(),
                              resultBuffer.getNumChannels(),
                              0,
                              resultBuffer.getNumSamples()))
    {
        failToLoad (file, "Unable to read from IR file: " + file.getFullPathName());
        return;
    }

    irFiles.addIfNotAlreadyThere (file);
    curFile = file;
    irChangedBroadcaster();
    vts.getParameter (irTag)->setValueNotifyingHost (1.0f);

    setMakeupGain ((float) formatReader->sampleRate);

    ScopedLock sl (irMutex);
    convolution.loadImpulseResponse (std::move (resultBuffer), formatReader->sampleRate, dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::yes, dsp::Convolution::Normalise::yes);
}

void AmpIRs::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    convolution.prepare (spec);

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);

    dryWetMixer.prepare (spec);
    dryWetMixerMono.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    if (curFile == File {})
        parameterChanged (irTag, vts.getRawParameterValue (irTag)->load());
    else
        loadIRFromFile (curFile);
}

void AmpIRs::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    dryWet.setWetMixProportion (mixParam->load());
    gain.setGainDecibels (gainParam->load() + makeupGainDB.load());

    dryWet.pushDrySamples (block);
    convolution.process (context);
    gain.process (context);
    dryWet.mixWetSamples (block);
}

std::unique_ptr<XmlElement> AmpIRs::toXML()
{
    auto xml = BaseProcessor::toXML();
    xml->setAttribute ("ir_file", curFile.getFullPathName());

    return std::move (xml);
}

void AmpIRs::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    auto irFile = File (xml->getStringAttribute ("ir_file"));
    if (irFile.getFullPathName().isNotEmpty())
        loadIRFromFile (irFile);
    else
        curFile = File();
}

//==========================================================================
bool AmpIRs::getCustomComponents (OwnedArray<Component>& customComps)
{
    struct CustomBoxAttach : private ComboBox::Listener
    {
    public:
        CustomBoxAttach (RangedAudioParameter& parameter, ComboBox& combo, UndoManager* undoManager = nullptr)
            : comboBox (combo),
              storedParameter (parameter),
              attachment (
                  parameter,
                  [this] (float f)
                  { setValue (f); },
                  undoManager)
        {
            sendInitialUpdate();
            comboBox.addListener (this);
        }

        ~CustomBoxAttach() override { comboBox.removeListener (this); }

        void sendInitialUpdate() { attachment.sendInitialUpdate(); }

    private:
        void setValue (float newValue)
        {
            const auto normValue = storedParameter.convertTo0to1 (newValue);
            const auto index = roundToInt (normValue * (float) (irNames.size() - 1));

            if (index == comboBox.getSelectedItemIndex())
                return;

            const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            comboBox.setSelectedItemIndex (index, sendNotificationSync);
        }

        void comboBoxChanged (ComboBox*) override
        {
            if (ignoreCallbacks)
                return;

            const auto numItems = irNames.size();
            const auto selected = comboBox.getSelectedItemIndex();

            if (selected >= irNames.size() - 1)
                return;

            const auto newValue = numItems > 1 ? (float) selected / (float) (numItems - 1)
                                               : 0.0f;

            attachment.setValueAsCompleteGesture (storedParameter.convertFrom0to1 (newValue));
        }

        ComboBox& comboBox;
        RangedAudioParameter& storedParameter;
        ParameterAttachment attachment;
        bool ignoreCallbacks = false;
    };

    struct IRComboBox : public ComboBox
    {
        IRComboBox (AudioProcessorValueTreeState& vtState, AmpIRs& airs) : ampIRs (airs), vts (vtState)
        {
            auto* param = vts.getParameter (irTag);
            attachment = std::make_unique<CustomBoxAttach> (*param, *this, vts.undoManager);
            refreshBox();

            setName (irTag + "__box");

            onIRChanged = ampIRs.irChangedBroadcaster.connect ([this]
                                                               { refreshBox(); });
        }

        void visibilityChanged() override
        {
            setName (vts.getParameter (irTag)->name);
        }

        void refreshBox()
        {
            clear();
            addItemList (StringArray (irNames.begin(), irNames.size() - 1), 1);
            addSeparator();

            auto* menu = getRootMenu();
            int menuIdx = getNumItems() + 1;
            int selectedFileIdx = 0;
            if (! ampIRs.irFiles.isEmpty())
            {
                for (auto& file : ampIRs.irFiles)
                {
                    PopupMenu::Item fileItem;
                    fileItem.text = file.getFileName();
                    fileItem.itemID = menuIdx++;
                    fileItem.isTicked = file == ampIRs.curFile;
                    fileItem.action = [=]
                    {
                        ampIRs.loadIRFromFile (file);
                    };
                    menu->addItem (fileItem);

                    if (fileItem.isTicked)
                        selectedFileIdx = fileItem.itemID;
                }

                addSeparator();
            }

            PopupMenu::Item customItem;
            customItem.text = "Custom";
            customItem.itemID = menuIdx++;
            customItem.action = [=]
            {
                constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
                fileChooser = std::make_shared<FileChooser> ("Custom IR", File(), "*.wav", true, false, getTopLevelComponent());
                fileChooser->launchAsync (flags,
                                          [=] (const FileChooser& fc)
                                          {
                                              if (fc.getResults().isEmpty())
                                                  return;

                                              ampIRs.loadIRFromFile (fc.getResult());
                                          });
            };
            menu->addItem (customItem);

            if (selectedFileIdx == 0)
                setSelectedId ((int) *ampIRs.vts.getRawParameterValue (irTag) + 1);
            else
                setSelectedId (selectedFileIdx);
        }

        AmpIRs& ampIRs;
        AudioProcessorValueTreeState& vts;
        std::unique_ptr<CustomBoxAttach> attachment;
        std::shared_ptr<FileChooser> fileChooser;
        chowdsp::ScopedCallback onIRChanged;
    };

    customComps.add (std::make_unique<IRComboBox> (vts, *this));

    return false;
}
