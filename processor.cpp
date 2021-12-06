#include <string>
#include <vector>
#include <array>
#include <iostream>
#include "processor.h"

// Get average magnitue of Goertzel array; return float
float processor::getAverageAmplitude(std::array<float, 8> &sampleArray)
{
    float sum = 0.f;

    for (const auto& s : sampleArray)
    {
        sum += s;
    }

    return sum/8.f;
}

// Hanning window; perform on vector by reference
void processor::hannWindow(std::vector<short> &samples)
{
    int		size = samples.size();
    double	multiplier;

    for (int i = 0; i < samples.size(); i++)
    {
        multiplier = 0.5 * (1.0 - cos((2.0 * PI * i) / (size - 1.0)));
        samples[i] = samples[i] * multiplier;
    }
}

// Zero padding with a multiplier; perform on vector by reference
void processor::zeroPadding(std::vector<short> &samples, float multiplier)
{
    const int size = samples.size() * multiplier;
    samples.resize(450, 0);
}

// Zero padding with fixed resize; perform on vector by reference
void processor::zeroPadding(std::vector<short> &samples, int resize)
{
    samples.resize(resize, 0);
}

// Run goertzel algorithm on chunk of samples for a specific frequency; return float
float processor::goertzel(std::vector<short> &samples, int targetFrequency)
{
    //float   omega, sine, cosine, coeff, w0, w1, w2, magnitude, real, imag;

    //Difference equation for Goertzel:
    //N = numSamples
    //k = (N*targetFreq)/sampleFreq
    //w(n)=2cos(2*pi*k/N)*w(n-1)-w(n-2)+x(n)

    // variable definitions
    int		numSamples			= samples.size();
    float   numSamplesFloat		= (float)numSamples;
    float   scalingFactor		= numSamples / 2.f;
    int		k					= (int)(0.5f + ((numSamplesFloat * targetFrequency) / SAMPLE_RATE));

    float	omega				= (2.f * PI * k) / numSamplesFloat;
    float	sine				= sin(omega);
    float	cosine				= cos(omega);

    //Difference equation with new omega:
    //w(n)=2cos(omega)*w(n-1)-w(n-2)+x(n)

    float	w0					= 0.f;
    float	w1					= 0.f;
    float	w2					= 0.f;

    //w0 =w(n)
    //w1 =w(n-1)
    //w2 =w(n-2)

    for (const auto &s : samples)
    {
        w0 = (2.f * cosine ) * w1 - w2 + s;
        w2 = w1;
        w1 = w0;
    }

    // calculate the real and imaginary results + magnitude
    // scaling appropriately
    float	real				= (w1 - w2 * cosine) / scalingFactor;
    float	imag				= (w2 * sine) / scalingFactor;
    float	magnitude			= sqrtf(real * real + imag * imag);

    // return the magnitude
    return magnitude;
}

// ...
void processor::fft(cArray& x)
{

    const size_t N = x.size();
    if (N <= 1) return;

    // divide
    cArray even = x[std::slice(0, N / 2, 2)];
    cArray  odd = x[std::slice(1, N / 2, 2)];

    // conquer
    fft(even);
    fft(odd);

    // combine
    for (size_t k = 0; k < N / 2; ++k)
    {
        std::complex<double> t = std::polar(1.0, -2 * PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }

};

// ...
cArray processor::fft(std::vector<short>& samples)
{
    // inspired by
    // https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B

    // variables
    std::vector<double>					zeros(samples.size(), 0.);
    cArray								cvec(samples.size());

    // create complex array from samples & zeros vectors
    std::transform(samples.begin(), samples.end(), zeros.begin(), std::begin(cvec), [](double da, double db) {
        return std::complex<double>(da, db);
    });

    // perform fft on cvec
    processor::fft(cvec);

    // return result
    return cvec;
};

// ...
void processor::fft2(cArray& x)
{
    // DFT
    unsigned int N = x.size(), k = N, n;
    double thetaT = 3.14159265358979323846264338328L / N;
    std::complex<double> phiT = std::complex<double>(cos(thetaT), -sin(thetaT)), T;
    while (k > 1)
    {
        n = k;
        k >>= 1;
        phiT = phiT * phiT;
        T = 1.0L;
        for (unsigned int l = 0; l < k; l++)
        {
            for (unsigned int a = l; a < N; a += n)
            {
                unsigned int b = a + k;
                std::complex<double> t = x[a] - x[b];
                x[a] += x[b];
                x[b] = t * T;
            }
            T *= phiT;
        }
    }
    // Decimate
    unsigned int m = (unsigned int)log2(N);
    for (unsigned int a = 0; a < N; a++)
    {
        unsigned int b = a;
        // Reverse bits
        b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
        b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
        b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
        b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
        b = ((b >> 16) | (b << 16)) >> (32 - m);
        if (b > a)
        {
            std::complex<double> t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }
    //// Normalize (This section make it not working correctly)
    //Complex f = 1.0 / sqrt(N);
    //for (unsigned int i = 0; i < N; i++)
    //	x[i] *= f;
};

// ...
cArray processor::fft2(std::vector<short>& samples)
{
    // inspired by
    // https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B

    // variables
    std::vector<double>					zeros(samples.size(), 0.);
    cArray								cvec(samples.size());

    // create complex array from samples & zeros vectors
    std::transform(samples.begin(), samples.end(), zeros.begin(), std::begin(cvec), [](double da, double db) {
        return std::complex<double>(da, db);
    });

    // perform fft2 on cvec
    processor::fft2(cvec);

    // return result
    return cvec;
};

// Run goertzel algorithm on chunk of samples for all DTMF frequncies; return array[8] of magnitudes (float)
std::array<float, 8> processor::goertzelArray(std::vector<short> &samples)
{
    std::array<float, 8> tempArray = {};

    for (int i = 0; i < 8; i++)
    {
        tempArray[i] = processor::goertzel(samples, freq[i]);
    }

    return tempArray;
}

// Print magnitudes of goertzelArray
void processor::printGoertzelArray(std::array<float, 8> &sampleArray)
{
    std::cout << "Goertzel array:\n";

    for (int i = 0; i < 8; i++)
    {
        std::cout << freq[i] << " : " << sampleArray[i] << " | ";
    }

    std::cout << "\n\n";
}