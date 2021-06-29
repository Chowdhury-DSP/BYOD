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
    
    return std::move (v);
}

template <typename DistType>
Vec2 createRandomVec2 (std::default_random_engine& generator, DistType& distribution, int size1, int size2)
{
    Vec2 v ((size_t) size1, Vec ((size_t) size2, 0.0f));
    for (int i = 0; i < size1; ++i)
        for (int j = 0; j < size2; ++j)
            v[(size_t) i][(size_t) j] = distribution (generator);
    
    return std::move (v);
}

struct Orthogonal {};
template<>
Vec2 createRandomVec2<Orthogonal> (std::default_random_engine& generator, Orthogonal&, int size1, int size2)
{
    Vec2 v ((size_t) size1, Vec ((size_t) size2, 0.0f));
    static std::normal_distribution<float> gaussian (0.0f, 1.0f);

    using namespace Eigen;
    Eigen::MatrixXf R (size1, size2);
    for (int i = 0; i < size1; ++i)
        R.row(i) = VectorXf::Map(&v[(size_t) i][0], size2);

    MatrixXf X = MatrixXf::Zero (size1, size2).unaryExpr([&generator](double) { return gaussian (generator); });
    MatrixXf XtX = X.transpose() * X;
    SelfAdjointEigenSolver<MatrixXf> es (XtX);
    MatrixXf S = es.operatorInverseSqrt();
    R = X * S;
    
    return std::move (v);
}

struct GlorotUniform
{
    std::unique_ptr<std::uniform_real_distribution<float>> dist;

    void initialise (float min, float max)
    {
        dist = std::make_unique<std::uniform_real_distribution<float>> (min, max);
    }
};

template<>
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
            for (int k = 0; k < size2; ++k)
                v[(size_t) i][(size_t) j][(size_t) k] = distribution (generator);
    
    return std::move (v);
}


template<>
Vec3 createRandomVec3<GlorotUniform> (std::default_random_engine& generator, GlorotUniform& glorot, int size1, int size2, int size3)
{
    int fan_in = size1 * size3;
    int fan_out = size2 * size3;
    auto limit = std::sqrt (6.0f / float (fan_in + fan_out));
    glorot.initialise (-limit, limit);
    return createRandomVec3 (generator, *glorot.dist, size1, size2, size3);
}
}

RONN::RONN (UndoManager* um) : BaseProcessor ("RONN", createParameterLayout(), um)
{
    inGainDbParam = vts.getRawParameterValue ("gain_db");
    vts.addParameterListener ("seed", this);
    parameterChanged ("seed", vts.getRawParameterValue ("seed")->load());

    MemoryInputStream jsonStream (BinaryData::ronn_json, BinaryData::ronn_jsonSize, false);
    auto jsonInput = nlohmann::json::parse (jsonStream.readEntireStreamAsString().toStdString());
    // neuralNet[0].parseJson (jsonInput);
    // neuralNet[1].parseJson (jsonInput);

    uiOptions.backgroundColour = Colours::indianred;
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "RONN is a \"Randomised Overdrive Neural Network\", first proposed by Christian Steinmetz. This implementation uses a convolutional recurrent neural net.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
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

    auto convWeights = createRandomVec3 (generator, glorot, 4, 8, 6);
    auto convBias = createRandomVec (generator, normal, 4);

    auto gruKernel = createRandomVec2 (generator, glorot, 4, 8);
    auto gruRecurrent = createRandomVec2 (generator, ortho, 8, 8);
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
    buffer.applyGain (5.0f);
}
