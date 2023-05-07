#include "TubeScreamer.h"
#include "../diode_circuits/DiodeParameter.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

TubeScreamer::TubeScreamer (UndoManager* um)
    : BaseProcessor ("Tube Screamer", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, "gain");
    diodeTypeParam = vts.getRawParameterValue ("diode");
    loadParameterPointer (nDiodesParam, vts, "num_diodes");

    uiOptions.backgroundColour = Colours::tomato.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.2f);
    uiOptions.info.description = "Virtual analog emulation of the clipping stage from the Tube Screamer overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->addResistor (1.0e3f, "R5", [this] (const netlist::CircuitQuantity& self, void* voidWDF)
                                           { auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF); });
    netlistCircuitQuantities->addResistor (1.0e5f, "R10", [this] (const netlist::CircuitQuantity& self, void* voidWDF)
                                           { auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF); });
    netlistCircuitQuantities->addCapacitor (1.0e-6f, "C4", [this] (const netlist::CircuitQuantity& self, void* voidWDF)
                                            { auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF); });
}

ParamLayout TubeScreamer::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "gain", "Gain", 0.5f);
    DiodeParameter::createDiodeParam (params, "diode");
    DiodeParameter::createNDiodesParam (params, "num_diodes");

    return { params.begin(), params.end() };
}

void TubeScreamer::prepare (double sampleRate, int samplesPerBlock)
{
    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam, true);
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void TubeScreamer::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (Decibels::decibelsToGain (-6.0f));

    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam);
        wdf[ch].process (buffer.getWritePointer (ch), buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (Decibels::decibelsToGain (-6.0f));
}
