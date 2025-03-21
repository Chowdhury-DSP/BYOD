#include "ParamModulator.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/ParameterHelpers.h"

namespace ParamModulatorTags
{
const String unipolarModTag = "unipolar_mod";
const String bipolarModTag = "bipolar_mod";
const String bipolarModeTag = "bipolar_mode";
} // namespace ParamModulatorTags

ParamModulator::ParamModulator (UndoManager* um)
    : BaseProcessor (
        "Param Modulator",
        createParameterLayout(),
        NullPort {},
        BasicOutputPort {},
        um,
        [] (auto)
        { return PortType::audio; },
        [] (auto)
        { return PortType::modulation; })
{
    using namespace ParameterHelpers;
    loadParameterPointer (unipolarModParam, vts, ParamModulatorTags::unipolarModTag);
    loadParameterPointer (bipolarModParam, vts, ParamModulatorTags::bipolarModTag);
    loadParameterPointer (bipolarModeParam, vts, ParamModulatorTags::bipolarModeTag);

    uiOptions.backgroundColour = Colours::yellowgreen.darker (0.1f);
    uiOptions.powerColour = Colours::red.brighter (0.05f);
    uiOptions.info.description = "Module that uses a parameter as a modulation source.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (ParamModulatorTags::bipolarModeTag);
}

ParamLayout ParamModulator::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createPercentParameter (params, ParamModulatorTags::unipolarModTag, "Modulation", 0.0f);
    createBipolarPercentParameter (params, ParamModulatorTags::bipolarModTag, "Modulation +/-", 0.0f);
    emplace_param<chowdsp::BoolParameter> (params, ParamModulatorTags::bipolarModeTag, "Bipolar", true);
    return { params.begin(), params.end() };
}

void ParamModulator::prepare (double sampleRate, int samplesPerBlock)
{
    modSmooth.setRampLength (0.01f);
    modSmooth.prepare (sampleRate, samplesPerBlock);

    modOutBuffer.setSize (1, samplesPerBlock);
}

void ParamModulator::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    const auto modValue = bipolarModeParam->get() ? bipolarModParam->getCurrentValue() : unipolarModParam->getCurrentValue();
    modSmooth.process (modValue, numSamples);

    modOutBuffer.setSize (1, numSamples, false, false, true);
    modOutBuffer.copyFrom (0, 0, modSmooth.getSmoothedBuffer(), numSamples, 1.0f);

    outputBuffers.getReference (0) = modOutBuffer;
}

void ParamModulator::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    modOutBuffer.setSize (1, numSamples, false, false, true);
    modOutBuffer.clear();

    outputBuffers.getReference (0) = modOutBuffer;
}

bool ParamModulator::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class ControlSlider : public Slider
    {
    public:
        ControlSlider (AudioProcessorValueTreeState& vtState, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              unipolarSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, ParamModulatorTags::unipolarModTag), hcp),
              bipolarSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, ParamModulatorTags::bipolarModTag), hcp),
              unipolarAttach (vts, ParamModulatorTags::unipolarModTag, unipolarSlider),
              bipolarAttach (vts, ParamModulatorTags::bipolarModTag, bipolarSlider),
              modeAttach (
                  *vts.getParameter (ParamModulatorTags::bipolarModeTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &bipolarSlider, &unipolarSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (bipolarSlider, bipolarSlider.getParameter());
            hcp.registerParameterComponent (unipolarSlider, unipolarSlider.getParameter());

            Component::setName (ParamModulatorTags::unipolarModTag + "__" + ParamModulatorTags::bipolarModTag + "__");
        }

        void colourChanged() override
        {
            for (auto* s : { &bipolarSlider, &unipolarSlider })
            {
                for (auto colourID : { Slider::textBoxOutlineColourId,
                                       Slider::textBoxTextColourId,
                                       Slider::textBoxBackgroundColourId,
                                       Slider::textBoxHighlightColourId,
                                       Slider::thumbColourId })
                {
                    s->setColour (colourID, findColour (colourID, false));
                }
            }
        }

        void updateSliderVisibility (bool isBipolar)
        {
            unipolarSlider.setVisible (! isBipolar);
            bipolarSlider.setVisible (isBipolar);

            setName (vts.getParameter (isBipolar ? ParamModulatorTags::bipolarModTag : ParamModulatorTags::unipolarModTag)->name);
            if (auto* parent = getParentComponent())
                parent->repaint();
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (ParamModulatorTags::bipolarModeTag)->load() == 1.0f);
        }

        void resized() override
        {
            for (auto* s : { &bipolarSlider, &unipolarSlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            unipolarSlider.setBounds (getLocalBounds());
            bipolarSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        ModulatableSlider unipolarSlider, bipolarSlider;
        SliderAttachment unipolarAttach, bipolarAttach;
        ParameterAttachment modeAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlSlider)
    };

    customComps.add (std::make_unique<ControlSlider> (vts, hcp));

    return false;
}
