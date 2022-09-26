#include "BassFace.h"
#include "../ParameterHelpers.h"

BassFace::BassFace (UndoManager* um) : BaseProcessor ("Bass Face", createParameterLayout(), um)
{
    gainSmoothed.setParameterHandle (dynamic_cast<chowdsp::FloatParameter*> (vts.getParameter ("gain")));

    uiOptions.backgroundColour = Colours::darkred.brighter (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "Emulation of a HEAVY bass distortion signal chain.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout BassFace::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "gain", "Gain", 0.5f);

    return { params.begin(), params.end() };
}

template <int hiddenSize, typename ModelType>
void load_model (ModelType& model, const char* rnnData, int rnnDataSize)
{
    juce::MemoryInputStream jsonInputStream (rnnData, (size_t) rnnDataSize, false);
    auto weights_json = nlohmann::json::parse (jsonInputStream.readEntireStreamAsString().toStdString());

    using Vec2d = std::vector<std::vector<float>>;
    auto transpose = [] (const Vec2d& x) -> Vec2d
    {
        auto outer_size = x.size();
        auto inner_size = x[0].size();
        Vec2d y (inner_size, std::vector<float> (outer_size, 0.0f));

        for (size_t i = 0; i < outer_size; ++i)
        {
            for (size_t j = 0; j < inner_size; ++j)
                y[j][i] = x[i][j];
        }

        return std::move (y);
    };

    auto& lstm = model.template get<0>();
    auto& dense = model.template get<1>();

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals (transpose (lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals (transpose (lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 4 * hiddenSize; ++i)
        lstm_bias_hh[(size_t) i] += lstm_bias_ih[(size_t) i];
    lstm.setBVals (lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights (dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias (dense_bias.data());
}

void BassFace::prepare (double sampleRate, int samplesPerBlock)
{
    const auto targetSampleRate = [this, sampleRate]
    {
        if ((int) sampleRate % 44100 == 0)
        {
            for (auto& m : model)
                load_model<hiddenSize> (m, BinaryData::bass_face_model_88_2k_json, BinaryData::bass_face_model_88_2k_jsonSize);
            return 88200.0;
        }

        for (auto& m : model)
            load_model<hiddenSize> (m, BinaryData::bass_face_model_96k_json, BinaryData::bass_face_model_96k_jsonSize);
        return 96000.0;
    }();

    const size_t oversamplingOrder = sampleRate <= 48000.0 ? 1 : 0;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, oversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampling->initProcessing (samplesPerBlock);
    const auto osSampleRate = sampleRate * (double) oversampling->getOversamplingFactor();
    const auto osSamplesPerBlock = samplesPerBlock * (int) oversampling->getOversamplingFactor();

    const auto rnnDelaySamples = [] (auto curFs, auto targetFs)
    {
        if (curFs <= targetFs)
            return 1;

        const auto delaySamples = std::ceil (curFs / targetFs);
        return (int) delaySamples;
    }(osSampleRate, targetSampleRate);

    for (auto& m : model)
    {
        m.get<0>().prepare (rnnDelaySamples);
        m.reset();
    }

    gainSmoothed.prepare (osSampleRate, (int) std::ceil ((float) osSamplesPerBlock));
    gainSmoothed.setRampLength (0.05);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (15.0f, chowdsp::CoefficientCalculators::butterworthQ<float>, (float) sampleRate);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void BassFace::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    {
        auto&& block = dsp::AudioBlock<float> { buffer };
        auto&& osBlock = oversampling->processSamplesUp (block);

        const auto osNumSamples = (int) osBlock.getNumSamples();
        gainSmoothed.process (osNumSamples);
        const auto* gainData = gainSmoothed.getSmoothedBuffer();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            static constexpr auto registerSize = xsimd::batch<float>::size;
            float inVec alignas (RTNEURAL_DEFAULT_ALIGNMENT)[registerSize] {};
            std::fill (inVec, inVec + registerSize, 0.0f);

            auto* x = osBlock.getChannelPointer ((size_t) ch);
            for (int i = 0; i < osNumSamples; ++i)
            {
                inVec[0] = x[i];
                inVec[1] = gainData[i];
                x[i] = model[ch].forward (inVec) + x[i];
            }
        }

        oversampling->processSamplesDown (block);
    }

    dcBlocker.processBlock (buffer);
}
