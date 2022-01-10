#include <iostream>
#include <vector>

#include "BigMuffClip.h"
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

namespace
{
constexpr double _sampleRate = 48000.0;
constexpr double _freq = 100.0;
constexpr int N = 4800;
} // namespace

int main()
{
    std::vector<float> data (N, 0.0f);
    for (int i = 0; i < N; ++i)
        data[i] = 0.8f * (float) std::sin (2.0 * M_PI * (double) i * _freq / _sampleRate);

    BigMuffClip bmc;
    BigMuffClip bmc2;
    bmc.prepare (_sampleRate);
    bmc2.prepare (_sampleRate);

    std::vector<float> out1 (N, 0.0f);
    std::vector<float> out2 (N, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        out1[i] = bmc.process (data[i]);
        out2[i] = bmc2.process (out1[i]);
    }

    plt::plot (data);
    plt::plot (out1, "--");
    plt::plot (out2, "--");
    plt::show();

    return 0;
}
