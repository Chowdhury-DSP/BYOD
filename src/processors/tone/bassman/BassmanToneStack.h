#pragma once

#include <pch.h>

using namespace chowdsp::WDF;

class BassmanToneStack
{
public:
    BassmanToneStack (double sr) : Cap1 (250e-12, sr),
                                   Cap2 (20e-9, sr),
                                   Cap3 (20e-9, sr),
                                   R (std::tie (S1, S3, S2, Cap2, Res4, Cap3))
    {
        pot1Smooth.reset (sr, 0.005);
        pot2Smooth.reset (sr, 0.005);
        pot3Smooth.reset (sr, 0.005);
    }

    void setSMatrixData()
    {
        auto pot1 = pot1Smooth.getNextValue();
        Res1m.setResistanceValue (pot1 * R1);
        Res1p.setResistanceValue ((1.0 - pot1) * R1);

        auto pot2 = pot2Smooth.getNextValue();
        Res2.setResistanceValue (pot2 * R2);

        auto pot3 = pot3Smooth.getNextValue();
        Res3m.setResistanceValue (pot3 * R3);
        Res3p.setResistanceValue ((1.0 - pot3) * R3);

        double Ra = S1.R;
        double Rb = S3.R;
        R.setSMatrixData ({
            { 1 - (2 * Ra * (348190.833333 * Rb + 362427505.6386863889)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), (421032094.8051527778 * Ra) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), (2 * Ra * (560000000000000.0 * Rb + 586045987915763889.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(2 * Ra * (56000 * Rb - 151911448.611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Ra * (292190.833333 * Rb + 303822906.84711)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Ra * (348190.833333 * Rb + 151911458.23611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463) },
            { (421032094.8051527778 * Rb) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), 1 - (2 * Rb * (348190.833333 * Ra + 16544036047.4025763889)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Rb * (520.833333 * Ra + 58604598.7915763889)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Rb * (347670.0 * Ra + 16485431448.611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Rb * (520.833333 * Ra - 151911448.611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(2 * Rb * (348190.833333 * Ra + 16333520000.0)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463) },
            { (326670400000000000000.0 * Rb + 341864066590781707009260.0) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(583340.0 * (520.833333 * Ra + 58604598.7915763889)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1.0 * (2452183080555336111.0 * Ra + 164268268498194236111.0 * Rb + 2351491666670000.0 * Ra * Rb + 170932033295390853504630.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), (583340.0 * (520.8333 * Ra + 56520.8333 * Rb + Ra * Rb)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(583340.0 * (1041.666633 * Ra + 520.8333 * Rb + Ra * Rb)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1.0 * (3038229164722200000.0 * Ra - 326670400000000000000.0 * Rb)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0) },
            { -(1.0 * (583333332960000000.0 * Rb - 1582410922018507009260.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1041.666666 * (347670.0 * Ra + 16485431448.611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), (1041.666666 * (520.8333 * Ra + 56520.8333 * Rb + Ra * Rb)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1.0 * (2712788166863889.0 * Ra - 164851601794194236111.0 * Rb - 3471491666670000.0 * Ra * Rb + 791210851070853504630.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), (1041.666666 * (520.8333 * Ra + 520.8333 * Rb + Ra * Rb + 151911448.611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1.0 * (3621562497682200000.0 * Ra + 583333332960000000.0 * Rb + 170140833224443200000000.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0) },
            { -(1.0 * (327253733332960000000.0 * Rb + 340281655668763200000000.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1.0 * (583333332960000000.0 * Ra - 170140822444320000000000.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1.0 * (1166666628960000000.0 * Ra + 583333296000000000.0 * Rb + 1120000000000000.0 * Ra * Rb)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), (583333296000000000.0 * Ra + 583333296000000000.0 * Rb + 1120000000000000.0 * Ra * Rb + 170140822444320000000000.0) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), (2457608427426863889.0 * Ra - 162396706154934236111.0 * Rb + 2361908333330000.0 * Ra * Rb - 169349622373372346495370.0) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1.0 * (583333332960000000.0 * Ra + 327253733332960000000.0 * Rb + 170140833224443200000000.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0) },
            { -(1041.6666 * (348190.833333 * Rb + 151911458.23611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1041.6666 * (348190.833333 * Ra + 16333520000.0)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), -(1.0 * (5425346871527778.0 * Ra - 583333296000000000.0 * Rb)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1.0 * (3621562268220000000.0 * Ra + 583333296000000000.0 * Rb + 170140822444320000000000.0)) / (3624275056386863889.0 * Ra + 165440360474025763889.0 * Rb + 3481908333330000.0 * Ra * Rb + 170932033295390853504630.0), -(1041.6666 * (520.833333 * Ra + 292190.833333 * Rb + 151911458.23611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463), 1 - (1041.6666 * (348190.833333 * Ra + 348190.833333 * Rb + 16485431458.23611)) / (362427505.6386863889 * Ra + 16544036047.4025763889 * Rb + 348190.833333 * Ra * Rb + 17093203329539.085350463) },
        });
    }

    inline double processSample (double inSamp) noexcept
    {
        Vres.setVoltage (inSamp);
        R.incident (0.0);

        return wdft::voltage<double> (Res1m) + wdft::voltage<double> (S2) + wdft::voltage<double> (Res3m);
    }

    void process (double* buffer, const int numSamples) noexcept
    {
        bool isSmoothing = pot1Smooth.isSmoothing() || pot2Smooth.isSmoothing() || pot3Smooth.isSmoothing();
        if (isSmoothing)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                setSMatrixData();
                buffer[n] = processSample (buffer[n]);
            }

            return;
        }

        for (int n = 0; n < numSamples; ++n)
            buffer[n] = processSample (buffer[n]);
    }

    void setParams (double pot1, double pot2, double pot3, bool force = false)
    {
        if (force)
        {
            pot1Smooth.setCurrentAndTargetValue (pot1);
            pot2Smooth.setCurrentAndTargetValue (pot2);
            pot3Smooth.setCurrentAndTargetValue (pot3);

            setSMatrixData();
        }
        else
        {
            pot1Smooth.setTargetValue (pot1);
            pot2Smooth.setTargetValue (pot2);
            pot3Smooth.setTargetValue (pot3);
        }
    }

private:
    using Res = wdft::ResistorT<double>;
    using ResVs = wdft::ResistiveVoltageSourceT<double>;
    using Cap = wdft::CapacitorT<double>;

    Cap Cap1;
    Cap Cap2; // Port D
    Cap Cap3; // Port F

    Res Res1p { 1.0 };
    Res Res1m { 1.0 };
    Res Res2 { 1.0 };
    Res Res3p { 1.0 };
    Res Res3m { 1.0 };
    Res Res4 { 56e3 }; // Port E

    ResVs Vres { 1.0 };

    // Port A
    using S1Type = wdft::WDFSeriesT<double, ResVs, Res>;
    S1Type S1 { Vres, Res3m };

    // Port B
    using SeriesRes = wdft::WDFSeriesT<double, Res, Res>;
    SeriesRes S3 { Res2, Res3p };

    // Port C
    SeriesRes S4 { Res1p, Res1m };
    using S2Type = wdft::WDFSeriesT<double, Cap, SeriesRes>;
    S2Type S2 { Cap1, S4 };

    static constexpr double R1 = 250e3;
    static constexpr double R2 = 1e6;
    static constexpr double R3 = 96e3; // modified from 25e3

    wdft::RootRtypeAdaptor<double, S1Type, SeriesRes, S2Type, Cap, Res, Cap> R;

    SmoothedValue<double, ValueSmoothingTypes::Linear> pot1Smooth, pot2Smooth, pot3Smooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassmanToneStack)
};