#include "TubeScreamer.h"
#include "../diode_circuits/DiodeParameter.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"

TubeScreamer::TubeScreamer (UndoManager* um) : BaseProcessor ("Tube Screamer", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, "gain");
    diodeTypeParam = vts.getRawParameterValue ("diode");
    loadParameterPointer (nDiodesParam, vts, "num_diodes");

    uiOptions.backgroundColour = Colours::tomato.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.2f);
    uiOptions.info.description = "Virtual analog emulation of the clipping stage from the Tube Screamer overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout TubeScreamer::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "gain", "Gain", 0.5f);
    DiodeParameter::createDiodeParam (params, "diode");
    DiodeParameter::createNDiodesParam (params, "num_diodes");

    return { params.begin(), params.end() };
}

void TubeScreamer::prepare (double sampleRate, int samplesPerBlock)
{
    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam, true);
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void TubeScreamer::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (Decibels::decibelsToGain (-6.0f));

    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam);
        wdf[ch].process (buffer.getWritePointer (ch), buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (Decibels::decibelsToGain (-6.0f));
}

void TubeScreamer::addToPopupMenu (PopupMenu& menu)
{
    static constexpr int rowHeight = 25;
    struct NetlistWindow : Component
    {
        NetlistWindow()
        {
            componentLabel.setText ("R5", juce::dontSendNotification);
            componentLabel.setJustificationType (Justification::centred);
            componentLabel.setColour (Label::textColourId, Colours::black);
            addAndMakeVisible (componentLabel);

            valueLabel.setText ("4.5 kOhms", juce::dontSendNotification);
            valueLabel.setJustificationType (Justification::centred);
            valueLabel.setColour (Label::textColourId, Colours::black);
            valueLabel.setColour (Label::textWhenEditingColourId, Colours::black);
            valueLabel.setColour (TextEditor::highlightColourId, Colours::black.withAlpha (0.2f));
            valueLabel.setColour (TextEditor::highlightedTextColourId, Colours::black);
            valueLabel.setColour (CaretComponent::caretColourId, Colours::black);
            valueLabel.setEditable (true);
            valueLabel.onEditorShow = [this] {
                if (auto* ed = valueLabel.getCurrentTextEditor())
                    ed->setJustification (Justification::centred);
            };
            valueLabel.onTextChange = [] {};
            addAndMakeVisible (valueLabel);

            setSize (300, 2 * rowHeight);
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::white);

            g.setColour (Colours::black);
            g.drawLine (juce::Line<float> { 0.5f * (float) getWidth(),
                                            0.0f,
                                            0.5f * (float) getWidth(),
                                            (float) getHeight() },
                        2.0f);

            g.drawLine (juce::Line<float> { 0.0f,
                                            0.5f * (float) getHeight(),
                                            (float) getWidth(),
                                            0.5f * (float) getHeight() },
                        2.0f);

            g.setFont (Font { 20.0f }.boldened());
            const auto halfWidth = proportionOfWidth (0.5f);
            g.drawFittedText ("Component", { 0, 0, halfWidth, rowHeight }, Justification::centred, 1);
            g.drawFittedText ("Value", { halfWidth, 0, halfWidth, rowHeight }, Justification::centred, 1);
        }

        void resized() override
        {
            const auto halfWidth = proportionOfWidth (0.5f);
            componentLabel.setBounds (0, rowHeight, halfWidth, rowHeight);
            valueLabel.setBounds (halfWidth, rowHeight, halfWidth, rowHeight);
        }

        Label componentLabel;
        Label valueLabel;
    };

    PopupMenu::Item item;
    item.itemID = 99091;
    item.text = "Show netlist";
    item.action = [this]
    {
        juce::Logger::writeToLog ("Showing netlist for module: " + BaseProcessor::getName());

        if (editor == nullptr)
            return;

        auto* topLevelEditor = [this]() -> AudioProcessorEditor*
        {
            Component* tlEditor = editor->getParentComponent();
            while (tlEditor != nullptr)
            {
                if (auto* editorCast = dynamic_cast<AudioProcessorEditor*> (tlEditor))
                    return editorCast;

                tlEditor = tlEditor->getParentComponent();
            }
            return nullptr;
        }();

        if (topLevelEditor == nullptr)
            return;

        netlistWindow = std::make_unique<chowdsp::WindowInPlugin<NetlistWindow>> (*topLevelEditor);
        auto* netlistWindowCast = static_cast<chowdsp::WindowInPlugin<NetlistWindow>*> (netlistWindow.get()); // NOLINT
        netlistWindowCast->setResizable (false, false);
        netlistWindowCast->show();
    };

    menu.addItem (std::move (item));
    menu.addSeparator();
}

// TODO: integrate units: https://github.com/LLNL/units
