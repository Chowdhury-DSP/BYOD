#include "RONN.h"

namespace
{
int randomSeeds[5] = { 1, 101, 2048, 5005, 9001 };

using Vec = std::vector<float>;
using Vec2 = std::vector<Vec>;
using Vec3 = std::vector<Vec2>;

template <typename DistType>
Vec createRandomVec (std::default_random_engine& generator, DistType& distribution, int size)
{
    Vec v ((size_t) size, 0.0f);
    for (int i = 0; i < size; ++i)
        v[(size_t) i] = distribution (generator);

    return v;
}

template <typename DistType>
Vec2 createRandomVec2 (std::default_random_engine& generator, DistType& distribution, int size1, int size2)
{
    Vec2 v ((size_t) size1, Vec ((size_t) size2, 0.0f));
    for (int i = 0; i < size1; ++i)
        for (int j = 0; j < size2; ++j)
            v[(size_t) i][(size_t) j] = distribution (generator);

    return v;
}

struct Orthogonal
{
};
template <>
Vec2 createRandomVec2<Orthogonal> (std::default_random_engine& generator, Orthogonal&, int size1, int size2)
{
    static std::normal_distribution<float> gaussian (0.0f, 1.0f);

    using namespace Eigen;
    const auto dim = jmax (size1, size2);
    MatrixXf X = MatrixXf::Zero (dim, dim).unaryExpr ([&generator] (double) { return gaussian (generator); });
    MatrixXf XtX = X.transpose() * X;
    SelfAdjointEigenSolver<MatrixXf> es (XtX);
    MatrixXf S = es.operatorInverseSqrt();
    MatrixXf R = X * S;

    Vec2 v ((size_t) size1, Vec ((size_t) size2, 0.0f));
    for (int i = 0; i < size1; ++i)
        std::copy (R.col (i).data(), R.col (i).data() + size2, v[(size_t) i].data());

    return v;
}

struct GlorotUniform
{
    std::unique_ptr<std::uniform_real_distribution<float>> dist;

    void initialise (float min, float max)
    {
        dist = std::make_unique<std::uniform_real_distribution<float>> (min, max);
    }
};

template <>
Vec2 createRandomVec2<GlorotUniform> (std::default_random_engine& generator, GlorotUniform& glorot, int size1, int size2)
{
    auto limit = std::sqrt (6.0f / float (size1 + size2));
    glorot.initialise (-limit, limit);
    return createRandomVec2 (generator, *glorot.dist, size1, size2);
}

template <typename DistType>
Vec3 createRandomVec3 (std::default_random_engine& generator, DistType& distribution, int size1, int size2, int size3)
{
    Vec3 v ((size_t) size1, Vec2 ((size_t) size2, Vec ((size_t) size3, 0.0f)));
    for (int i = 0; i < size1; ++i)
        for (int j = 0; j < size2; ++j)
            for (int k = 0; k < size3; ++k)
                v[(size_t) i][(size_t) j][(size_t) k] = distribution (generator);

    return v;
}

template <>
Vec3 createRandomVec3<GlorotUniform> (std::default_random_engine& generator, GlorotUniform& glorot, int size1, int size2, int size3)
{
    int fan_in = size2 * size3;
    int fan_out = size1 * size3;
    auto limit = std::sqrt (6.0f / float (fan_in + fan_out));
    glorot.initialise (-limit, limit);
    return createRandomVec3 (generator, *glorot.dist, size1, size2, size3);
}
} // namespace

RONN::RONN (UndoManager* um) : BaseProcessor ("RONN", createParameterLayout(), um)
{
    inGainDbParam = vts.getRawParameterValue ("gain_db");
    vts.addParameterListener ("seed", this);
    parameterChanged ("seed", vts.getRawParameterValue ("seed")->load());

    uiOptions.backgroundColour = Colours::indianred;
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "RONN is a \"Randomised Overdrive Neural Network\", first proposed by Christian Steinmetz. This implementation uses a convolutional recurrent neural net.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    uiOptions.info.infoLink = URL { "https://github.com/csteinmetz1/ronn" };
}

AudioProcessorValueTreeState::ParameterLayout RONN::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createGainDBParameter (params, "gain_db", "Gain", -12.0f, 12.0f, 0.0f);

    StringArray seeds;
    for (int sInt : randomSeeds)
        seeds.add (String (sInt));
    params.push_back (std::make_unique<AudioParameterChoice> ("seed", "Random Seed", seeds, 0));

    return { params.begin(), params.end() };
}

void RONN::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID != "seed")
        return;

    auto seedIdx = int (newValue);
    reloadModel (randomSeeds[seedIdx]);
}

void RONN::reloadModel (int randomSeed)
{
    // set up random distributions
    std::default_random_engine generator ((std::default_random_engine::result_type) randomSeed);
    static std::normal_distribution<float> normal (0.0f, 0.05f);
    static Orthogonal ortho;
    static GlorotUniform glorot;

    auto denseInWeights = createRandomVec2 (generator, normal, 8, 1);
    auto denseInBias = createRandomVec (generator, normal, 8);

    auto convWeights = createRandomVec3 (generator, glorot, 4, 8, 3);
    auto convBias = createRandomVec (generator, normal, 4);

    auto gruKernel = createRandomVec2 (generator, glorot, 4, 3 * 8);
    auto gruRecurrent = createRandomVec2 (generator, ortho, 8, 3 * 8);
    auto gruBias = createRandomVec2 (generator, normal, 2, 3 * 8);

    auto denseOutWeights = createRandomVec2 (generator, ortho, 1, 8);
    auto denseOutBias = createRandomVec (generator, normal, 1);

    SpinLock::ScopedLockType modelLoadingScopedLock (modelLoadingLock);
    for (auto& nn : neuralNet)
    {
        nn.get<0>().setWeights (denseInWeights);
        nn.get<0>().setBias (denseInBias.data());

        nn.get<2>().setWeights (convWeights);
        nn.get<2>().setBias (convBias);

        nn.get<4>().setWVals (gruKernel);
        nn.get<4>().setUVals (gruRecurrent);
        nn.get<4>().setBVals (gruBias);

        nn.get<5>().setWeights (denseOutWeights);
        nn.get<5>().setBias (denseOutBias.data());

        nn.reset();

        float input[] = { 1.0f };
        makeupGain = 1.0f / std::abs (nn.forward (input));
        nn.reset();
    }
}

void RONN::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
    inputGain.prepare (spec);
    inputGain.setRampDurationSeconds (0.05);

    neuralNet[0].reset();
    neuralNet[1].reset();

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void RONN::processAudio (AudioBuffer<float>& buffer)
{
    SpinLock::ScopedTryLockType modelLoadingTryLock (modelLoadingLock);
    if (! modelLoadingTryLock.isLocked())
        return;

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    inputGain.setGainDecibels (inGainDbParam->load() + 25.0f);
    inputGain.process (context);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float input[] = { x[n] };
            x[n] = neuralNet[ch].forward (input);
        }
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (makeupGain);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto numSamples = buffer.getNumSamples();
        auto* x = buffer.getWritePointer (ch);

        using x_type = xsimd::simd_type<float>;
        const auto inc = (int) x_type::size;

        // size for which the vectorization is possible
        const auto vec_size = numSamples - numSamples % inc;
        for (int i = 0; i < vec_size; i += inc)
        {
            x_type xvec = xsimd::load_unaligned (&x[i]);
            auto yvec = xsimd::tanh (xvec);
            xsimd::store_unaligned (&x[i], yvec);
        }

        // Remaining part that cannot be vectorized
        for (int i = vec_size; i < numSamples; ++i)
            x[i] = std::tanh (x[i]);
    }

    buffer.applyGain (0.8f);
}
