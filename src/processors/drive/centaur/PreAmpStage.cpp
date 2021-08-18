#include "PreAmpStage.h"

using namespace GainStageSpace;

PreAmpWDF::PreAmpWDF (double sampleRate) : C3 (0.1e-6, sampleRate),
                                           C5 (68.0e-9, sampleRate),
                                           C16 (1.0e-6, sampleRate)
{
    reset();
}

void PreAmpWDF::setGain (float gain)
{
    Vbias.setResistanceValue ((double) gain * 100.0e3);
}

void PreAmpWDF::reset()
{
    Vbias.setVoltage (0.0f); // (4.5);
    Vbias2.setVoltage (0.0f); // (4.5);
}
