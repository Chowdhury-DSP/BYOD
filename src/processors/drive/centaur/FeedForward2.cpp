#include "FeedForward2.h"

using namespace GainStageSpace;

FeedForward2WDF::FeedForward2WDF (double sampleRate) : C4 (68e-9, sampleRate),
                                                       C6 (390e-9, sampleRate),
                                                       C11 (2.2e-9, sampleRate),
                                                       C12 (27e-9, sampleRate)
{
    reset();
}

void FeedForward2WDF::reset()
{
    Vbias.setVoltage (0.0);
}

void FeedForward2WDF::setGain (float gain)
{
    RVTop.setResistanceValue (jmax ((double) gain * 100e3, 1.0));
    RVBot.setResistanceValue (jmax ((1.0 - (double) gain) * 100e3, 1.0));
}
