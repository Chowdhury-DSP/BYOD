#pragma once

#include "../../utility/DCBlocker.h"
#include "JuniorBWDF.h"
#include "NeuralTriodeModel.h"
#include "processors/BaseProcessor.h"

class JuniorB : public BaseProcessor
{
public:
    explicit JuniorB (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* driveParamPct = nullptr;
    chowdsp::FloatParameter* blendParamPct = nullptr;
    chowdsp::ChoiceParameter* stagesParam = nullptr;

    using TriodeModel = NeuralTriodeModel<float, TriodeModelELuApprox<float, 4, 8>>;
    TriodeModel triode_model_4_8_elu { BinaryData::junior_1_stage_json, BinaryData::junior_1_stage_jsonSize };

    struct SingleStageModel
    {
        using WDF = JuniorBWDF<float, TriodeModel>;
        SingleStageModel (TriodeModel& model) // NOLINT(google-explicit-constructor)
            : wdfs { WDF { model }, WDF { model } }
        {
        }

        WDF wdfs[2];
    };

    static constexpr int maxNumStages = 4;
    SingleStageModel stages[maxNumStages] { triode_model_4_8_elu, triode_model_4_8_elu, triode_model_4_8_elu, triode_model_4_8_elu };

    chowdsp::Gain<float> driveGain, wetGain, dryGain;
    chowdsp::Buffer<float> dryBuffer;
    chowdsp::FirstOrderHPF<float> dcBlocker;

    bool preBuffering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuniorB)
};
