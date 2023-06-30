#include "MIDIModulator.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String bipolarTag = "bipolar";
const String midiMapTag = "midi_map_cc";
} // namespace

MidiModulator::MidiModulator (UndoManager* um)
    : BaseProcessor (
        "MIDI Modulator",
        createParameterLayout(),
        NullPort {},
        BasicOutputPort {},
        um,
        [] (auto)
        { return PortType::audio; },
        [] (auto)
        { return PortType::modulation; })
{
    midiModSmooth.setRampLength (0.025);
    ParameterHelpers::loadParameterPointer (bipolarParam, vts, bipolarTag);

    uiOptions.backgroundColour = Colours::forestgreen.brighter (0.1f);
    uiOptions.powerColour = Colours::whitesmoke;
    uiOptions.info.description = "Module that allows MIDI controller changes to be used as a modulation source.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (bipolarTag);
}

ParamLayout MidiModulator::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    emplace_param<chowdsp::BoolParameter> (params, bipolarTag, "Bipolar", false);
    return { params.begin(), params.end() };
}

void MidiModulator::prepare (double sampleRate, int samplesPerBlock)
{
    midiModSmooth.prepare (sampleRate, samplesPerBlock);
    modControlValue = 0;
    modOutBuffer.setSize (1, samplesPerBlock);
}

static auto getModFloatValue (int val, bool isBipolar)
{
    if (isBipolar)
        return ((float) val / 63.5f) - 1.0f;
    else
        return (float) val / 127.0f;
}

void MidiModulator::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    if (! isLearning.load())
    {
        for (const auto& midiEvent : *midiBuffer)
        {
            const auto& message = midiEvent.getMessage();
            if (message.isController() && message.getControllerNumber() == mappedModController)
                modControlValue = message.getControllerValue();
        }
    }
    else
    {
        for (const auto& midiEvent : *midiBuffer)
        {
            const auto& message = midiEvent.getMessage();
            if (message.isController())
            {
                mappedModController = message.getControllerNumber();
                modControlValue = message.getControllerValue();
            }
        }
    }

    midiModSmooth.process (getModFloatValue (modControlValue.load(), bipolarParam->get()), numSamples);

    modOutBuffer.setSize (1, numSamples, false, false, true);
    modOutBuffer.copyFrom (0, 0, midiModSmooth.getSmoothedBuffer(), numSamples, 1.0f);

    outputBuffers.getReference (0) = &modOutBuffer;
}

void MidiModulator::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    modOutBuffer.setSize (1, numSamples, false, false, true);
    modOutBuffer.clear();

    outputBuffers.getReference (0) = &modOutBuffer;
}

//===================================================================
std::unique_ptr<XmlElement> MidiModulator::toXML()
{
    auto xml = BaseProcessor::toXML();
    xml->setAttribute (midiMapTag, mappedModController);
    return xml;
}

void MidiModulator::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);
    mappedModController = xml->getIntAttribute (midiMapTag, 1);
}

//===================================================================
bool MidiModulator::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&)
{
    struct MidiComp : public Component,
                      private Timer
    {
        explicit MidiComp (MidiModulator& processor)
            : proc (processor),
              bipolarAttach (*proc.bipolarParam, bipolarButton)
        {
            bipolarButton.setButtonText ("Bipolar");
            bipolarButton.setColour (TextButton::buttonColourId, Colours::transparentBlack);
            bipolarButton.setColour (ComboBox::outlineColourId, Colours::red.brighter());
            bipolarButton.setColour (TextButton::textColourOffId, Colours::red.brighter());
            bipolarButton.setColour (TextButton::buttonOnColourId, Colours::red.brighter());
            bipolarButton.setColour (TextButton::textColourOnId, Colours::black);
            bipolarButton.setClickingTogglesState (true);
            addAndMakeVisible (bipolarButton);

            learnButton.setButtonText ("Learn");
            learnButton.setColour (TextButton::buttonColourId, Colours::transparentBlack);
            learnButton.setColour (ComboBox::outlineColourId, Colours::yellow);
            learnButton.setColour (TextButton::textColourOffId, Colours::yellow);
            learnButton.setColour (TextButton::buttonOnColourId, Colours::yellow);
            learnButton.setColour (TextButton::textColourOnId, Colours::black);
            learnButton.setClickingTogglesState (true);
            learnButton.onClick = [this]
            { proc.isLearning.store (learnButton.getToggleState()); };
            addAndMakeVisible (learnButton);

            startTimerHz (24);
        }

        ~MidiComp() override
        {
            proc.isLearning.store (false);
        }

        static float getAngleForValue (float value, bool isBipolar)
        {
            if (isBipolar)
                return degreesToRadians (60.0f * value);
            else
                return degreesToRadians (120.0f * value - 60.0f);
        }

        static void drawControlVizBackground (Graphics& g, Rectangle<int> bounds, bool isBipolar)
        {
            g.setColour (Colours::grey.brighter());

            const auto height = (float) bounds.getHeight();
            auto tickLength = height * 0.15f;
            const auto bottomCentre = juce::Point { bounds.getCentreX(), bounds.getBottom() }.toFloat();

            const auto majorTicks = isBipolar ? std::initializer_list<float> { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f }
                                              : std::initializer_list<float> { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
            for (auto tick : majorTicks)
            {
                auto angle = getAngleForValue (tick, isBipolar);
                auto line = Line<float>::fromStartAndAngle (bottomCentre, height, angle);
                line = line.withShortenedStart (height - tickLength);
                g.drawLine (line, 2.5f);

                const auto labelDim = (float) bounds.getHeight() * 0.2f;
                line = line.withLengthenedEnd (labelDim * 0.75f);
                g.setFont (labelDim * 0.7f);
                g.drawFittedText (String { tick },
                                  juce::Rectangle { labelDim, labelDim }
                                      .withCentre ({ line.getEndX(), line.getEndY() })
                                      .toNearestInt(),
                                  juce::Justification::centred,
                                  1);
            }

            g.setColour (Colours::grey.withAlpha (0.65f));
            tickLength = height * 0.075f;
            const auto minorTicks = isBipolar ? std::initializer_list<float> { -0.9f, -0.8f, -0.7f, -0.6f, -0.4f, -0.3f, -0.2f, -0.1f, 0.2f, 0.3f, 0.4f, 0.6f, 0.7f, 0.8f, 0.9f }
                                              : std::initializer_list<float> { 0.05f, 0.1f, 0.15f, 0.2f, 0.3f, 0.35f, 0.4f, 0.45f, 0.55f, 0.6f, 0.65f, 0.7f, 0.8f, 0.85f, 0.9f, 0.95f };
            for (auto tick : minorTicks)
            {
                auto angle = getAngleForValue (tick, isBipolar);
                auto line = Line<float>::fromStartAndAngle (bottomCentre, height, angle);
                line = line.withShortenedStart (height - tickLength);
                g.drawLine (line, 1.5f);
            }
        }

        static void drawControlLine (Graphics& g, Rectangle<int> bounds, float value, bool isBipolar)
        {
            const auto height = (float) bounds.getHeight();
            const auto lineLength = height * 0.925f;
            const auto bottomCentre = juce::Point { bounds.getCentreX(), bounds.getBottom() }.toFloat();

            auto angle = getAngleForValue (value, isBipolar);
            auto line = Line<float>::fromStartAndAngle (bottomCentre, lineLength, angle);
            g.drawLine (line, 2.5f);
        }

        void paint (Graphics& g) override
        {
            auto b = getLocalBounds();

            g.setColour (Colours::black);
            g.fillRoundedRectangle (b.toFloat(), 20.0f);

            auto nameHeight = proportionOfHeight (0.3f);
            auto nameBounds = b.removeFromTop (nameHeight);
            auto vizBounds = b.reduced (proportionOfHeight (0.05f), 0);
            vizBounds.removeFromTop (proportionOfHeight (0.025f));

            drawControlVizBackground (g, vizBounds, proc.bipolarParam->get());

            const auto alphaMult = isEnabled() ? 1.0f : 0.5f;
            g.setColour (Colours::red.brighter().withAlpha (alphaMult));
            drawControlLine (g, vizBounds, getModFloatValue (proc.modControlValue.load(), proc.bipolarParam->get()), proc.bipolarParam->get());

            g.setFont ((float) nameHeight * 0.4f);
            const auto nameBoundsTrimmed = nameBounds.removeFromTop (proportionOfHeight (0.2f));
            g.setColour (Colours::yellow);
            const auto midiCCString = "CC " + String { chowdsp::AtomicRef { proc.mappedModController }.load() };
            g.drawFittedText (midiCCString, nameBoundsTrimmed, Justification::centred, 1);
        }

        void resized() override
        {
            const auto buttonWidth = proportionOfWidth (0.225f);
            const auto buttonHeight = proportionOfHeight (0.15f);
            const auto pad = proportionOfWidth (0.03f);

            learnButton.setBounds (pad, pad, buttonWidth, buttonHeight);
            bipolarButton.setBounds (getWidth() - (buttonWidth + pad), pad, buttonWidth, buttonHeight);
        }

        void timerCallback() override
        {
            repaint();
        }

        MidiModulator& proc;

        struct TunerButton : TextButton
        {
            void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
            {
                constexpr auto cornerSize = 3.0f;
                auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

                auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

                if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
                    baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);

                g.setColour (findColour (ComboBox::outlineColourId));
                g.drawRoundedRectangle (bounds, cornerSize, 1.0f);

                g.setColour (baseColour);
                g.fillRoundedRectangle (bounds, cornerSize);
            }

            void paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
            {
                auto& lf = getLookAndFeel();
                drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
                lf.drawButtonText (g, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
        };

        TunerButton bipolarButton;
        TunerButton learnButton;
        ButtonParameterAttachment bipolarAttach;
    };

    customComps.add (std::make_unique<MidiComp> (*this));
    return false;
}
