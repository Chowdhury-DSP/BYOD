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

    // @TODO: set more meaningful component value limits.
    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->addResistor (4.7e3f,
                                           "R4",
                                           [] (const netlist::CircuitQuantity& self, void* voidWDF)
                                           {
                                               auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF);
                                               wdfCast.R4.setResistanceValue (self.value.load());
                                           });
    netlistCircuitQuantities->addResistor (10.0e3f,
                                           "R5",
                                           [] (const netlist::CircuitQuantity& self, void* voidWDF)
                                           {
                                               auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF);
                                               wdfCast.R5.setResistanceValue (self.value.load());
                                           });
    netlistCircuitQuantities->addCapacitor (1.0e-6f,
                                            "C2",
                                            [] (const netlist::CircuitQuantity& self, void* voidWDF)
                                            {
                                                auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF);
                                                wdfCast.C2.setCapacitanceValue (self.value.load());
                                            });
    netlistCircuitQuantities->addCapacitor (0.047e-6f,
                                            "C3",
                                            [] (const netlist::CircuitQuantity& self, void* voidWDF)
                                            {
                                                auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF);
                                                wdfCast.C3.setCapacitanceValue (self.value.load());
                                            });
    netlistCircuitQuantities->addCapacitor (51.0e-12f,
                                            "C4",
                                            [] (const netlist::CircuitQuantity& self, void* voidWDF)
                                            {
                                                auto& wdfCast = *static_cast<TubeScreamerWDF*> (voidWDF);
                                                wdfCast.C4.setCapacitanceValue (self.value.load());
                                            });
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
    for (auto& quantity : *netlistCircuitQuantities)
    {
        if (chowdsp::AtomicHelpers::compareNegate (quantity.needsUpdate))
        {
            for (auto& wdfModel : wdf)
                quantity.setter (quantity, &wdfModel);
        }
    }

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
