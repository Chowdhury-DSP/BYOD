#ifndef GAINSTAGEPROC_H_INCLUDED
#define GAINSTAGEPROC_H_INCLUDED

#include "AmpStage.h"
#include "ClippingStage.h"
#include "FeedForward2.h"
#include "PreAmpStage.h"
#include "SummingAmp.h"

class GainStageProc
{
public:
    GainStageProc (AudioProcessorValueTreeState& vts, double sampleRate);

    void reset (double sampleRate, int samplesPerBlock);
    void processBlock (AudioBuffer<float>& buffer);

private:
    chowdsp::FloatParameter* gainParam = nullptr;

    AudioBuffer<float> ff1Buff;
    AudioBuffer<float> ff2Buff;

    GainStageSpace::PreAmpWDF preAmpL, preAmpR;
    GainStageSpace::PreAmpWDF* preAmp[2] { &preAmpL, &preAmpR };

    GainStageSpace::ClippingWDF clipL, clipR;
    GainStageSpace::ClippingWDF* clip[2] { &clipL, &clipR };

    GainStageSpace::FeedForward2WDF ff2L, ff2R;
    GainStageSpace::FeedForward2WDF* ff2[2] { &ff2L, &ff2R };

    GainStageSpace::AmpStage amp[2];
    GainStageSpace::SummingAmp sumAmp[2];
};

#endif // GAINSTAGEPROC_H_INCLUDED
