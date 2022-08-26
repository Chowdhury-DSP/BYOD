#pragma once

#include <pch.h>

constexpr int triodeModelNumInputs = 2;
constexpr int triodeModelNumOutputs = 2;

template <typename T, int NLayers, int HiddenSize, typename ActivationType>
struct TriodeModelType;

template <typename T, int NLayers, int HiddenSize>
using TriodeModelReLu = TriodeModelType<T, NLayers, HiddenSize, RTNeural::ReLuActivationT<T, HiddenSize>>;

template <typename T, int NLayers, int HiddenSize>
using TriodeModelELu = TriodeModelType<T, NLayers, HiddenSize, RTNeural::ELuActivationT<T, HiddenSize>>;

template <typename T, int HiddenSize, typename ActivationType>
struct TriodeModelType<T, 2, HiddenSize, ActivationType>
{
    using ModelType = RTNeural::ModelT<float,
                                       triodeModelNumInputs,
                                       triodeModelNumOutputs,
                                       RTNeural::DenseT<float, triodeModelNumInputs, HiddenSize>, // Input layer
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 1
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 2
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, triodeModelNumOutputs>>; // Output layer
};

template <typename T, int HiddenSize, typename ActivationType>
struct TriodeModelType<T, 3, HiddenSize, ActivationType>
{
    using ModelType = RTNeural::ModelT<float,
                                       triodeModelNumInputs,
                                       triodeModelNumOutputs,
                                       RTNeural::DenseT<float, triodeModelNumInputs, HiddenSize>, // Input layer
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 1
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 2
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 3
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, triodeModelNumOutputs>>; // Output layer
};

template <typename T, int HiddenSize, typename ActivationType>
struct TriodeModelType<T, 4, HiddenSize, ActivationType>
{
    using ModelType = RTNeural::ModelT<float,
                                       triodeModelNumInputs,
                                       triodeModelNumOutputs,
                                       RTNeural::DenseT<float, triodeModelNumInputs, HiddenSize>, // Input layer
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 1
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 2
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 3
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 4
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, triodeModelNumOutputs>>; // Output layer
};

template <typename T, int HiddenSize, typename ActivationType>
struct TriodeModelType<T, 5, HiddenSize, ActivationType>
{
    using ModelType = RTNeural::ModelT<float,
                                       triodeModelNumInputs,
                                       triodeModelNumOutputs,
                                       RTNeural::DenseT<float, triodeModelNumInputs, HiddenSize>, // Input layer
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 1
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 2
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 3
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 4
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, HiddenSize>, // Hidden layer 5
                                       ActivationType,
                                       RTNeural::DenseT<float, HiddenSize, triodeModelNumOutputs>>; // Output layer
};

template <typename T, typename ModelType>
class NeuralTriodeModel
{
public:
#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_IOS
    explicit NeuralTriodeModel (const char* modelData, int modelDataSize)
    {
#if JUCE_DEBUG
        constexpr bool debug = true;
        DBG ("Loading neural tube model...");
#else
        constexpr bool debug = false;
#endif

        juce::MemoryInputStream modelInputStream { modelData, (size_t) modelDataSize, false };
        auto jsonInput = nlohmann::json::parse (modelInputStream.readEntireStreamAsString().toStdString());
        removeUnknownLayerFromJson (jsonInput);
        model.parseJson (jsonInput, debug);
    }
#endif

    explicit NeuralTriodeModel (const std::string& modelFilePath)
    {
        std::ifstream model_json_stream (modelFilePath, std::ifstream::binary);
        nlohmann::json model_json;
        model_json_stream >> model_json;
        removeUnknownLayerFromJson (model_json);
        model.parseJson (model_json, true);
    }

    inline const T* compute (T* input)
    {
        model.forward (input);
        return model.getOutputs();
    }

private:
    static void removeUnknownLayerFromJson (nlohmann::json& model_json)
    {
        auto& layers_json = model_json["layers"];

        for (auto layers_iter = layers_json.begin(); layers_iter != layers_json.end();)
        {
            if (const auto layer_iter = layers_iter->find ("type"); layer_iter != layers_iter->end() && layer_iter->get<std::string>() == "unknown")
                layers_iter = layers_json.erase (layers_iter);
            else
                ++layers_iter;
        }
    }

    typename ModelType::ModelType model;
};
