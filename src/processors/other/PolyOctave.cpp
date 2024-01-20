#include "PolyOctave.h"
#include "processors/ParameterHelpers.h"

namespace PolyOctaveTags
{
const String dryTag = "dry";
const String upOctaveTag = "up_octave";
const String up2OctaveTag = "up2_octave";
} // namespace PolyOctaveTags

namespace FilterBankHelpers
{
// Reference for filter-bank design and octave shifting:
// https://aaltodoc.aalto.fi/server/api/core/bitstreams/ff9e52cf-fd79-45eb-b695-93038244ec0e/content

using float_2 = PolyOctave::ComplexERBFilterBank::float_2;
static_assert (float_2::size == 2);
static constexpr auto vec_size = float_2::size;

template <bool isImag = false>
inline float_2 processSample (const chowdsp::IIRFilter<4, float_2>& f, std::array<float_2, 5>& z, float_2 x)
{
    auto y = z[1];
    if constexpr (! isImag)
        y += x * f.b[0]; // for the imaginary filter, we know that b[0] == 0

    z[1] = z[2] + x * f.b[1] - y * f.a[1];
    z[2] = z[3] + x * f.b[2] - y * f.a[2];
    z[3] = z[4] + x * f.b[3] - y * f.a[3];
    z[4] = x * f.b[4] - y * f.a[4];

    return y;
}

static constexpr auto q_c = 4.0;
static void designERBFilter (size_t erb_index,
                             double gamma,
                             double erb_start,
                             double q_ERB,
                             double sample_rate,
                             double (&b_coeffs_cplx_real)[5],
                             double (&b_coeffs_cplx_imag)[5],
                             double (&a_coeffs_cplx)[5])
{
    const auto q_PS = gamma;

    const auto z = erb_start + static_cast<float> (erb_index) * (q_c / q_ERB);
    const auto center_target_freq = 228.7 * (std::pow (10.0, z / 21.3) - 1.0);
    const auto filter_q = (1.0 / (q_PS * q_ERB)) * (24.7 + 0.108 * center_target_freq);

    double b_coeffs_proto[3];
    double a_coeffs_proto[3];
    chowdsp::CoefficientCalculators::calcSecondOrderBPF (b_coeffs_proto,
                                                         a_coeffs_proto,
                                                         center_target_freq / gamma,
                                                         filter_q * 0.5,
                                                         sample_rate);

    auto pole = (std::sqrt (std::pow (std::complex { a_coeffs_proto[1] }, 2.0) - 4.0 * a_coeffs_proto[2]) - a_coeffs_proto[1]) / 2.0;
    auto conj_pole = std::conj (pole);
    if (std::imag (pole) < 0.0)
        pole = conj_pole;
    else if (std::imag (conj_pole) < 0.0)
        conj_pole = pole;
    auto at = -(pole + conj_pole);
    auto att = pole * conj_pole;

    auto ar1 = std::real (at);
    auto ai1 = std::imag (at);
    auto ar2 = std::real (att);
    auto ai2 = std::imag (att);

    // a[] = 1 + 2 ar1 z + (ai1^2 + ar1^2 + 2 ar2) z^2 + (2 ai1 ai2 + 2 ar1 ar2) z^3 + (ai2^2 + ar2^2) z^4
    a_coeffs_cplx[0] = 1.0;
    a_coeffs_cplx[1] = 2.0 * ar1;
    a_coeffs_cplx[2] = ai1 * ai1 + ar1 * ar1 + 2.0 * ar2;
    a_coeffs_cplx[3] = 2.0 * ai1 * ai2 + 2.0 * ar1 * ar2;
    a_coeffs_cplx[4] = ai2 * ai2 + ar2 * ar2;

    // b_real[] = b0 + (ar1 b0 + b1) z + (ar2 b0 + ar1 b1 + b2) z^2 + (ar2 b1 + ar1 b2) z^3 + ar2 b2 z^4
    b_coeffs_cplx_real[0] = b_coeffs_proto[0];
    b_coeffs_cplx_real[1] = ar1 * b_coeffs_proto[0] + b_coeffs_proto[1];
    b_coeffs_cplx_real[2] = ar2 * b_coeffs_proto[0] + ar1 * b_coeffs_proto[1] + b_coeffs_proto[2];
    b_coeffs_cplx_real[3] = ar2 * b_coeffs_proto[1] + ar1 * b_coeffs_proto[2];
    b_coeffs_cplx_real[4] = ar2 * b_coeffs_proto[2];

    // b_imag[] = -ai1 b0 z - (ai2 b0 + ai1 b1) z^2 - (ai2 b1 + ai1 br) z^3 - ai2 br z^4
    b_coeffs_cplx_imag[0] = 0.0;
    b_coeffs_cplx_imag[1] = -ai1 * b_coeffs_proto[0];
    b_coeffs_cplx_imag[2] = -ai2 * b_coeffs_proto[0] - ai1 * b_coeffs_proto[1];
    b_coeffs_cplx_imag[3] = -ai2 * b_coeffs_proto[1] - ai1 * b_coeffs_proto[2];
    b_coeffs_cplx_imag[4] = -ai2 * b_coeffs_proto[2];
}

static void designFilterBank (std::array<PolyOctave::ComplexERBFilterBank, 2>& filterBank,
                              double gamma,
                              double erb_start,
                              double q_ERB,
                              double sampleRate)
{
    for (size_t kiter = 0; kiter < PolyOctave::ComplexERBFilterBank::numFilterBands; kiter += vec_size)
    {
        alignas (16) std::array<float_2::value_type, float_2::size> b_coeffs_cplx_real_simd[5];
        alignas (16) std::array<float_2::value_type, float_2::size> b_coeffs_cplx_imag_simd[5];
        alignas (16) std::array<float_2::value_type, float_2::size> a_coeffs_cplx_simd[5];
        for (size_t i = 0; i < float_2::size; ++i)
        {
            const auto k = kiter + i;

            double b_coeffs_cplx_real[5];
            double b_coeffs_cplx_imag[5];
            double a_coeffs_cplx[5];
            designERBFilter (k, gamma, erb_start, q_ERB, sampleRate, b_coeffs_cplx_real, b_coeffs_cplx_imag, a_coeffs_cplx);

            for (size_t c = 0; c < 5; ++c)
            {
                b_coeffs_cplx_real_simd[c][i] = static_cast<float_2::value_type> (b_coeffs_cplx_real[c]);
                b_coeffs_cplx_imag_simd[c][i] = static_cast<float_2::value_type> (b_coeffs_cplx_imag[c]);
                a_coeffs_cplx_simd[c][i] = static_cast<float_2::value_type> (a_coeffs_cplx[c]);
            }
        }

        for (size_t ch = 0; ch < 2; ++ch)
        {
            filterBank[ch].erbFilterReal[kiter / vec_size].reset();
            filterBank[ch].erbFilterReal[kiter / vec_size].setCoefs ({
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[0].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[1].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[2].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[3].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[4].data()),
                                                                     },
                                                                     {
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[0].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[1].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[2].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[3].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[4].data()),
                                                                     });
            filterBank[ch].erbFilterImag[kiter / vec_size].reset();
            filterBank[ch].erbFilterImag[kiter / vec_size].setCoefs ({
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[0].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[1].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[2].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[3].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[4].data()),
                                                                     },
                                                                     {
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[0].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[1].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[2].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[3].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[4].data()),
                                                                     });
        }
    }
}
} // namespace FilterBankHelpers

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
                                        chowdsp::SmoothedBufferValue<double>& gain)
    {
        gain.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, paramID));
        gain.setRampLength (0.05);
        gain.mappingFunction = [] (double x)
        {
            return 2.0 * x * x;
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
