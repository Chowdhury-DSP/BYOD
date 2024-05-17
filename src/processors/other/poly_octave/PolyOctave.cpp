#include "PolyOctave.h"
#include "PolyOctaveV1FilterBankImpl.h"
#include "PolyOctaveV2FilterBankImpl.h"
#include "processors/ParameterHelpers.h"

namespace PolyOctaveTags
{
const String dryTag = "dry";
const String upOctaveTag = "up_octave";
const String up2OctaveTag = "up2_octave";
const String downOctaveTag = "down_octave";
const String v1Tag = "v1_mode";
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
    setupGainParam (PolyOctaveTags::downOctaveTag, downOctaveGain);
    setupGainParam (PolyOctaveTags::dryTag, dryGain);
    setupGainParam (PolyOctaveTags::upOctaveTag, upOctaveGain);
    setupGainParam (PolyOctaveTags::up2OctaveTag, up2OctaveGain);

    ParameterHelpers::loadParameterPointer (v1_mode, vts, PolyOctaveTags::v1Tag);
    addPopupMenuParameter (PolyOctaveTags::v1Tag);

    uiOptions.backgroundColour = Colour { 0xff9fa09d };
    uiOptions.powerColour = Colour { 0xffe70510 };
    uiOptions.info.description = "A \"polyphonic octave generator\" effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

#if JUCE_INTEL
    if (juce::SystemStats::hasAVX() && juce::SystemStats::hasFMA3())
    {
        juce::Logger::writeToLog ("Using Poly Octave with AVX SIMD instructions!");
        use_avx = true;
    }
#endif
}

ParamLayout PolyOctave::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, PolyOctaveTags::downOctaveTag, "-1 Oct", 0.0f);
    createPercentParameter (params, PolyOctaveTags::dryTag, "+0 Oct", 0.5f);
    createPercentParameter (params, PolyOctaveTags::upOctaveTag, "+1 Oct", 0.0f);
    createPercentParameter (params, PolyOctaveTags::up2OctaveTag, "+2 Oct", 0.0f);
    emplace_param<chowdsp::BoolParameter> (params, PolyOctaveTags::v1Tag, "V1 Mode", false); // @TODO: version streaming

    return { params.begin(), params.end() };
}

void PolyOctave::prepare (double sampleRate, int samplesPerBlock)
{
    dryGain.prepare (sampleRate, samplesPerBlock);
    up2OctaveGain.prepare (sampleRate, samplesPerBlock);
    upOctaveGain.prepare (sampleRate, samplesPerBlock);
    downOctaveGain.prepare (sampleRate, samplesPerBlock);

    doubleBuffer.setMaxSize (2, samplesPerBlock);
    up2OctaveBuffer_double.setMaxSize (2, 2 * samplesPerBlock); // allocate extra space for SIMD
    upOctaveBuffer_double.setMaxSize (2, 2 * samplesPerBlock); // allocate extra space for SIMD
    downOctaveBuffer_double.setMaxSize (2, samplesPerBlock);

    poly_octave_v2::design_filter_bank<poly_octave_v2::N1> (octaveUpFilterBank, 2.0, 5.0, 4.5, sampleRate);
    poly_octave_v2::design_filter_bank<poly_octave_v2::N1> (octaveUp2FilterBank, 3.0, 6.0, 2.75, sampleRate);
    for (auto& shifter : downOctavePitchShifters)
    {
        shifter.prepare (sampleRate);
        shifter.set_pitch_factor (0.5f);
    }

    poly_octave_v1::designFilterBank (octaveUpFilterBank_v1, 2.0, 5.0, 6.0, sampleRate);
    poly_octave_v1::designFilterBank (octaveUp2FilterBank_v1, 3.0, 7.0, 4.0, sampleRate);

    for (auto& shifter : downOctavePitchShifters_v1)
    {
        shifter.prepare (sampleRate);
        shifter.set_pitch_factor (0.5);
    }

    for (auto& busDCBlocker : dcBlocker)
    {
        busDCBlocker.prepare (2);
        busDCBlocker.calcCoefs (20.0f, (float) sampleRate);
    }

    mixOutBuffer.setSize (2, samplesPerBlock);
    up1OutBuffer.setSize (2, 8 * samplesPerBlock + 32); // padding for SIMD
    up2OutBuffer.setSize (2, 8 * samplesPerBlock + 32); // padding for SIMD
    down1OutBuffer.setSize (2, samplesPerBlock);
}

void PolyOctave::processAudio (AudioBuffer<float>& buffer)
{
    if (v1_mode->get())
    {
        processAudioV1 (buffer);
        return;
    }

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    mixOutBuffer.setSize (numChannels, numSamples, false, false, true);
    up1OutBuffer.setSize (numChannels, numSamples, false, false, true);
    up2OutBuffer.setSize (numChannels, numSamples, false, false, true);
    down1OutBuffer.setSize (numChannels, numSamples, false, false, true);

    // "down" processing
    for (auto [ch, data_in, data_out] : chowdsp::buffer_iters::zip_channels (std::as_const (buffer), down1OutBuffer))
    {
        for (auto [x, y] : chowdsp::zip (data_in, data_out))
            y = downOctavePitchShifters[(size_t) ch].process_sample (x);
    }
    downOctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (down1OutBuffer, downOctaveGain);

    // "up1" processing
    for (const auto& [ch, data_in, data_out] : chowdsp::buffer_iters::zip_channels (std::as_const (buffer), up1OutBuffer))
    {
#if JUCE_INTEL
        if (use_avx)
        {
            poly_octave_v2::process_avx<1> (octaveUpFilterBank[ch],
                                            data_in.data(),
                                            data_out.data(),
                                            numSamples);
        }
        else
#endif
        {
            poly_octave_v2::process<1> (octaveUpFilterBank[ch],
                                        data_in.data(),
                                        data_out.data(),
                                        numSamples);
        }
    }
    upOctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (up1OutBuffer, upOctaveGain);

    // "up2" processing
    for (const auto& [ch, data_in, data_out] : chowdsp::buffer_iters::zip_channels (std::as_const (buffer), up2OutBuffer))
    {
#if JUCE_INTEL
        if (use_avx)
        {
            poly_octave_v2::process_avx<2> (octaveUp2FilterBank[ch],
                                            data_in.data(),
                                            data_out.data(),
                                            numSamples);
        }
        else
#endif
        {
            poly_octave_v2::process<2> (octaveUp2FilterBank[ch],
                                        data_in.data(),
                                        data_out.data(),
                                        numSamples);
        }
    }
    up2OctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (up2OutBuffer, up2OctaveGain);

    chowdsp::BufferMath::copyBufferData (buffer, mixOutBuffer);
    dryGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (mixOutBuffer, dryGain);

    chowdsp::BufferMath::addBufferData (up1OutBuffer, mixOutBuffer);
    chowdsp::BufferMath::addBufferData (up2OutBuffer, mixOutBuffer);
    chowdsp::BufferMath::addBufferData (down1OutBuffer, mixOutBuffer);

    dcBlocker[MixOutput].processBlock (mixOutBuffer);
    dcBlocker[Up1Output].processBlock (up1OutBuffer);
    dcBlocker[Up2Output].processBlock (up2OutBuffer);
    dcBlocker[Down1Output].processBlock (down1OutBuffer);

    outputBuffers.getReference (MixOutput) = &mixOutBuffer;
    outputBuffers.getReference (Up1Output) = &up1OutBuffer;
    outputBuffers.getReference (Up2Output) = &up2OutBuffer;
    outputBuffers.getReference (Down1Output) = &down1OutBuffer;
}

void PolyOctave::processAudioV1 (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    doubleBuffer.setCurrentSize (numChannels, numSamples);
    upOctaveBuffer_double.setCurrentSize (numChannels, numSamples);
    up2OctaveBuffer_double.setCurrentSize (numChannels, numSamples);
    downOctaveBuffer_double.setCurrentSize (numChannels, numSamples);

    chowdsp::BufferMath::copyBufferData (buffer, doubleBuffer);

    // "down" processing
    for (auto [ch, data_in, data_out] : chowdsp::buffer_iters::zip_channels (std::as_const (doubleBuffer), downOctaveBuffer_double))
    {
        for (auto [x, y] : chowdsp::zip (data_in, data_out))
            y = downOctavePitchShifters_v1[(size_t) ch].process_sample (x);
    }

    // "up" processing
    for (int ch = 0; ch < numChannels; ++ch)
    {
        using poly_octave_v1::float_2;
        auto* dryData = doubleBuffer.getReadPointer (ch);

        auto* upData = upOctaveBuffer_double.getWritePointer (ch);
        auto* upDataSIMD = reinterpret_cast<float_2*> (upOctaveBuffer_double.getWritePointer (ch));
        jassert (juce::snapPointerToAlignment (upDataSIMD, xsimd::default_arch::alignment()) == upDataSIMD);
        std::fill (upDataSIMD, upDataSIMD + numSamples, float_2 {});

        auto* up2Data = up2OctaveBuffer_double.getWritePointer (ch);
        auto* up2DataSIMD = reinterpret_cast<float_2*> (up2OctaveBuffer_double.getWritePointer (ch));
        jassert (juce::snapPointerToAlignment (up2DataSIMD, xsimd::default_arch::alignment()) == up2DataSIMD);
        std::fill (up2DataSIMD, up2DataSIMD + numSamples, float_2 {});

        auto& upFilterBank = octaveUpFilterBank_v1[static_cast<size_t> (ch)];
        auto& up2FilterBank = octaveUp2FilterBank_v1[static_cast<size_t> (ch)];
        static constexpr auto eps = std::numeric_limits<double>::epsilon();

        for (size_t k = 0; k < poly_octave_v1::ComplexERBFilterBank::numFilterBands; k += float_2::size)
        {
            const auto filter_idx = k / float_2::size;
            auto& realFilter = upFilterBank.erbFilterReal[filter_idx];
            auto& imagFilter = upFilterBank.erbFilterImag[filter_idx];
            chowdsp::ScopedValue z_re { realFilter.z[0] };
            chowdsp::ScopedValue z_im { imagFilter.z[0] };
            for (int n = 0; n < numSamples; ++n)
            {
                auto x_re = poly_octave_v1::processSample (realFilter, z_re.get(), dryData[n]);
                auto x_im = poly_octave_v1::processSample<true> (imagFilter, z_im.get(), dryData[n]);

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

        for (size_t k = 0; k < poly_octave_v1::ComplexERBFilterBank::numFilterBands; k += float_2::size)
        {
            const auto filter_idx = k / float_2::size;
            auto& realFilter = up2FilterBank.erbFilterReal[filter_idx];
            auto& imagFilter = up2FilterBank.erbFilterImag[filter_idx];
            chowdsp::ScopedValue z_re { realFilter.z[0] };
            chowdsp::ScopedValue z_im { imagFilter.z[0] };

            for (int n = 0; n < numSamples; ++n)
            {
                auto x_re = poly_octave_v1::processSample (realFilter, z_re.get(), dryData[n]);
                auto x_im = poly_octave_v1::processSample<true> (imagFilter, z_im.get(), dryData[n]);

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

    chowdsp::BufferMath::applyGain (upOctaveBuffer_double, 2.0 / static_cast<double> (poly_octave_v1::ComplexERBFilterBank::numFilterBands));
    chowdsp::BufferMath::applyGain (up2OctaveBuffer_double, 2.0 / static_cast<double> (poly_octave_v1::ComplexERBFilterBank::numFilterBands));
    chowdsp::BufferMath::applyGain (downOctaveBuffer_double, juce::Decibels::decibelsToGain (3.0f));

    mixOutBuffer.setSize (numChannels, numSamples, false, false, true);
    up1OutBuffer.setSize (numChannels, numSamples, false, false, true);
    up2OutBuffer.setSize (numChannels, numSamples, false, false, true);
    down1OutBuffer.setSize (numChannels, numSamples, false, false, true);

    chowdsp::BufferMath::copyBufferData (doubleBuffer, mixOutBuffer);
    chowdsp::BufferMath::copyBufferData (upOctaveBuffer_double, up1OutBuffer);
    chowdsp::BufferMath::copyBufferData (up2OctaveBuffer_double, up2OutBuffer);
    chowdsp::BufferMath::copyBufferData (downOctaveBuffer_double, down1OutBuffer);

    upOctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (up1OutBuffer, upOctaveGain);
    up2OctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (up2OutBuffer, up2OctaveGain);
    downOctaveGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (down1OutBuffer, downOctaveGain);

    dryGain.process (numSamples);
    chowdsp::BufferMath::applyGainSmoothedBuffer (mixOutBuffer, dryGain);
    chowdsp::BufferMath::addBufferData (up1OutBuffer, mixOutBuffer);
    chowdsp::BufferMath::addBufferData (up2OutBuffer, mixOutBuffer);
    chowdsp::BufferMath::addBufferData (down1OutBuffer, mixOutBuffer);

    dcBlocker[MixOutput].processBlock (mixOutBuffer);
    dcBlocker[Up1Output].processBlock (up1OutBuffer);
    dcBlocker[Up2Output].processBlock (up2OutBuffer);
    dcBlocker[Down1Output].processBlock (down1OutBuffer);

    outputBuffers.getReference (MixOutput) = &mixOutBuffer;
    outputBuffers.getReference (Up1Output) = &up1OutBuffer;
    outputBuffers.getReference (Up2Output) = &up2OutBuffer;
    outputBuffers.getReference (Down1Output) = &down1OutBuffer;
}

void PolyOctave::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    mixOutBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    up1OutBuffer.setSize (1, numSamples, false, false, true);
    up2OutBuffer.setSize (1, numSamples, false, false, true);
    down1OutBuffer.setSize (1, numSamples, false, false, true);

    chowdsp::BufferMath::copyBufferData (buffer, mixOutBuffer);
    up1OutBuffer.clear();
    up2OutBuffer.clear();
    down1OutBuffer.clear();

    outputBuffers.getReference (MixOutput) = &mixOutBuffer;
    outputBuffers.getReference (Up1Output) = &up1OutBuffer;
    outputBuffers.getReference (Up2Output) = &up2OutBuffer;
    outputBuffers.getReference (Down1Output) = &down1OutBuffer;
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
            case OutputPort::Down1Output:
                return "-1 Octave Output";
        }
    }

    return BaseProcessor::getTooltipForPort (portIndex, isInput);
}

void PolyOctave::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    using namespace std::string_view_literals;
#if JUCE_IOS
    if (version <= chowdsp::Version { "2.1.0"sv })
#else
    if (version <= chowdsp::Version { "1.3.0"sv })
#endif
    {
        // For versions before 1.3.0, default to v1 mode
        v1_mode->setValueNotifyingHost (1.0f);
    }
}
