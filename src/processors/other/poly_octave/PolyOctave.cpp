#include "PolyOctave.h"
#include "PolyOctaveFilterBandHelpers.h"
#include "processors/ParameterHelpers.h"

namespace PolyOctaveTags
{
const String dryTag = "dry";
const String upOctaveTag = "up_octave";
const String up2OctaveTag = "up2_octave";
} // namespace PolyOctaveTags

PolyOctave::PolyOctave (UndoManager* um)
    : BaseProcessor (
          "Poly Octave",
          createParameterLayout(),
          BasicInputPort {},
          OutputPort {},
          um)
{
    using namespace ParameterHelpers;
    const auto setupGainParam = [this] (const juce::String& paramID,
                                        auto& gain)
    {
        gain.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, paramID));
        gain.setRampLength (0.05);
        gain.mappingFunction = [] (auto x)
        {
            return 2 * x * x;
        };
    };
    setupGainParam (PolyOctaveTags::dryTag, dryGain);
    setupGainParam (PolyOctaveTags::upOctaveTag, upOctaveGain);
    setupGainParam (PolyOctaveTags::up2OctaveTag, up2OctaveGain);

    uiOptions.backgroundColour = Colour { 0xff9fa09d };
    uiOptions.powerColour = Colour { 0xffe70510 };
    uiOptions.info.description = "A \"polyphonic octave generator\" effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout PolyOctave::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, PolyOctaveTags::dryTag, "+0 Oct", 0.5f);
    createPercentParameter (params, PolyOctaveTags::upOctaveTag, "+1 Oct", 0.0f);
    createPercentParameter (params, PolyOctaveTags::up2OctaveTag, "+2 Oct", 0.0f);

    return { params.begin(), params.end() };
}

void PolyOctave::prepare (double sampleRate, int samplesPerBlock)
{
    dryGain.prepare (sampleRate, samplesPerBlock);
    up2OctaveGain.prepare (sampleRate, samplesPerBlock);
    upOctaveGain.prepare (sampleRate, samplesPerBlock);

    doubleBuffer.setMaxSize (2, samplesPerBlock);
    up2OctaveBuffer.setMaxSize (2, static_cast<int> (ComplexERBFilterBank::float_2::size) * samplesPerBlock); // allocate extra space for SIMD
    upOctaveBuffer.setMaxSize (2, static_cast<int> (ComplexERBFilterBank::float_2::size) * samplesPerBlock); // allocate extra space for SIMD

    FilterBankHelpers::designFilterBank (octaveUpFilterBank, 2.0, 5.0, 6.0, sampleRate);
    FilterBankHelpers::designFilterBank (octaveUp2FilterBank, 3.0, 7.0, 4.0, sampleRate);

    for (auto& busDCBlocker : dcBlocker)
    {
        busDCBlocker.prepare (2);
        busDCBlocker.calcCoefs (20.0f, (float) sampleRate);
    }

    mixOutBuffer.setSize (2, samplesPerBlock);
    up1OutBuffer.setSize (2, samplesPerBlock);
    up2OutBuffer.setSize (2, samplesPerBlock);
}

void PolyOctave::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    doubleBuffer.setCurrentSize (numChannels, numSamples);
    upOctaveBuffer.setCurrentSize (numChannels, numSamples);
    up2OctaveBuffer.setCurrentSize (numChannels, numSamples);

    chowdsp::BufferMath::copyBufferData (buffer, doubleBuffer);

    using float_2 = ComplexERBFilterBank::float_2;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dryData = doubleBuffer.getReadPointer (ch);

        auto* upData = upOctaveBuffer.getWritePointer (ch);
        auto* upDataSIMD = reinterpret_cast<float_2*> (upOctaveBuffer.getWritePointer (ch));
        jassert (juce::snapPointerToAlignment (upDataSIMD, xsimd::default_arch::alignment()) == upDataSIMD);
        std::fill (upDataSIMD, upDataSIMD + numSamples, float_2 {});

        auto* up2Data = up2OctaveBuffer.getWritePointer (ch);
        auto* up2DataSIMD = reinterpret_cast<float_2*> (up2OctaveBuffer.getWritePointer (ch));
        jassert (juce::snapPointerToAlignment (up2DataSIMD, xsimd::default_arch::alignment()) == upDataSIMD);
        std::fill (up2DataSIMD, up2DataSIMD + numSamples, float_2 {});

        auto& upFilterBank = octaveUpFilterBank[static_cast<size_t> (ch)];
        auto& up2FilterBank = octaveUp2FilterBank[static_cast<size_t> (ch)];
        static constexpr auto eps = std::numeric_limits<double>::epsilon();

        for (size_t k = 0; k < ComplexERBFilterBank::numFilterBands; k += float_2::size)
        {
            const auto filter_idx = k / float_2::size;
            auto& realFilter = upFilterBank.erbFilterReal[filter_idx];
            auto& imagFilter = upFilterBank.erbFilterImag[filter_idx];
            chowdsp::ScopedValue z_re { realFilter.z[0] };
            chowdsp::ScopedValue z_im { imagFilter.z[0] };
            for (int n = 0; n < numSamples; ++n)
            {
                auto x_re = FilterBankHelpers::processSample (realFilter, z_re.get(), dryData[n]);
                auto x_im = FilterBankHelpers::processSample<true> (imagFilter, z_im.get(), dryData[n]);

                auto x_re_sq = x_re * x_re;
                auto x_im_sq = x_im * x_im;
                auto x_abs_sq = x_re_sq + x_im_sq;

                auto x_abs_r = xsimd::select (x_abs_sq > eps, xsimd::rsqrt (x_abs_sq), {});
                upDataSIMD[n] += (x_re_sq - x_im_sq) * x_abs_r;
            }
        }

        // Collapsing the SIMD data into a single "channel".
        // This is a little bit dangerous since upData, and upDataSIMD
        // are aliased, but it should always be fine because we're "reading"
        // faster than we're "writing".
        for (int n = 0; n < numSamples; ++n)
            upData[n] = xsimd::reduce_add (upDataSIMD[n]);

        for (size_t k = 0; k < ComplexERBFilterBank::numFilterBands; k += float_2::size)
        {
            const auto filter_idx = k / float_2::size;
            auto& realFilter = up2FilterBank.erbFilterReal[filter_idx];
            auto& imagFilter = up2FilterBank.erbFilterImag[filter_idx];
            chowdsp::ScopedValue z_re { realFilter.z[0] };
            chowdsp::ScopedValue z_im { imagFilter.z[0] };

            for (int n = 0; n < numSamples; ++n)
            {
                auto x_re = FilterBankHelpers::processSample (realFilter, z_re.get(), dryData[n]);
                auto x_im = FilterBankHelpers::processSample<true> (imagFilter, z_im.get(), dryData[n]);

                auto x_re_sq = x_re * x_re;
                auto x_im_sq = x_im * x_im;
                auto x_abs_sq = x_re_sq + x_im_sq;

                auto x_abs_sq_r = xsimd::select (x_abs_sq > eps, xsimd::reciprocal (x_abs_sq), {});
                up2DataSIMD[n] += x_re * (x_re_sq - 3.0 * x_im_sq) * x_abs_sq_r;
            }
        }
        for (int n = 0; n < numSamples; ++n)
            up2Data[n] = xsimd::reduce_add (up2DataSIMD[n]);
    }

    chowdsp::BufferMath::applyGain (upOctaveBuffer, 2.0 / static_cast<double> (ComplexERBFilterBank::numFilterBands));
    upOctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (upOctaveBuffer, upOctaveGain);

    chowdsp::BufferMath::applyGain (up2OctaveBuffer, 2.0 / static_cast<double> (ComplexERBFilterBank::numFilterBands));
    up2OctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (up2OctaveBuffer, up2OctaveGain);

    dryGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (doubleBuffer, dryGain);
    chowdsp::BufferMath::addBufferData (up2OctaveBuffer, doubleBuffer);
    chowdsp::BufferMath::addBufferData (upOctaveBuffer, doubleBuffer);

    mixOutBuffer.setSize (numChannels, numSamples, false, false, true);
    up1OutBuffer.setSize (numChannels, numSamples, false, false, true);
    up2OutBuffer.setSize (numChannels, numSamples, false, false, true);

    chowdsp::BufferMath::copyBufferData (doubleBuffer, mixOutBuffer);
    chowdsp::BufferMath::copyBufferData (upOctaveBuffer, up1OutBuffer);
    chowdsp::BufferMath::copyBufferData (up2OctaveBuffer, up2OutBuffer);

    dcBlocker[MixOutput].processBlock (mixOutBuffer);
    dcBlocker[Up1Output].processBlock (up1OutBuffer);
    dcBlocker[Up2Output].processBlock (up2OutBuffer);

    outputBuffers.getReference (MixOutput) = &mixOutBuffer;
    outputBuffers.getReference (Up1Output) = &up1OutBuffer;
    outputBuffers.getReference (Up2Output) = &up2OutBuffer;
}

void PolyOctave::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    mixOutBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    up1OutBuffer.setSize (1, numSamples, false, false, true);
    up2OutBuffer.setSize (1, numSamples, false, false, true);

    chowdsp::BufferMath::copyBufferData (buffer, mixOutBuffer);
    up1OutBuffer.clear();
    up2OutBuffer.clear();

    outputBuffers.getReference (MixOutput) = &mixOutBuffer;
    outputBuffers.getReference (Up1Output) = &up1OutBuffer;
    outputBuffers.getReference (Up2Output) = &up2OutBuffer;
}

String PolyOctave::getTooltipForPort (int portIndex, bool isInput)
{
    if (! isInput)
    {
        switch ((OutputPort) portIndex)
        {
            case OutputPort::MixOutput:
                return "Mix Output";
            case OutputPort::Up1Output:
                return "+1 Octave Output";
            case OutputPort::Up2Output:
                return "+2 Octave Output";
        }
    }

    return BaseProcessor::getTooltipForPort (portIndex, isInput);
}
