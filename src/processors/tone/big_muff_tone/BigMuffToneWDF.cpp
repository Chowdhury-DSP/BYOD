#include "BigMuffToneWDF.h"

void BigMuffToneWDF::prepare (double fs)
{
    C8.prepare ((float) fs);
    C9.prepare ((float) fs);
}

void BigMuffToneWDF::setParams (float tone, float center)
{
    constexpr auto RTone = 100.0e3f;
    RTm.setResistanceValue (RTone * tone);
    RTp.setResistanceValue (RTone * (1.0f - tone));
}
