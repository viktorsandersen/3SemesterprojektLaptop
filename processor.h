#pragma once
#include <string>
#include <vector>
#include <array>
#include <valarray>
#include <ccomplex>
#include "definitioner.h"

using cArray = std::valarray<std::complex<double>>;

namespace processor
{

    void					equalize();

    void					fft(cArray& x);
    cArray					fft(std::vector<short>& samples);
    void					fft2(cArray& x);
    cArray					fft2(std::vector<short>& samples);

    void					hannWindow(std::vector<short> &samples);
    void					zeroPadding(std::vector<short> &samples, float multiplier);
    void					zeroPadding(std::vector<short> &samples, int resize);

    float					goertzel(std::vector<short> &samples, int frequency);
    std::array<float, 8>	goertzelArray(std::vector<short> &samples);

    void					printGoertzelArray(std::array<float, 8> &sampleArray);
    float					getAverageAmplitude(std::array<float, 8> &sampleArray);
}