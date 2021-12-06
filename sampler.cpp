#include <iostream>
#include <vector>
#include <functional>

#include "sampler.h"

//// Constructor & Destructor /////////////////////////////////////////////////////////////////////////////////////////////////////

// Default Constructor
sampler::sampler(std::function<void(std::vector<short> samples)> callback, bool allowPlayback)
        : callback(callback), rate(SAMPLE_RATE), interval(SAMPLE_INTERVAL*1000-1), status(state::idle), allowPlayback(allowPlayback)
{

    //std::vector<std::string> availableDevices = sf::SoundRecorder::getAvailableDevices();

    // choose a device
    //std::string inputDevice = availableDevices[1];
    // set processing interval                                         ];
    this->setProcessingInterval(sf::microseconds(this->interval));

    // log
    std::cout << "\nInitialized sampler with:\n";
    std::cout << "SAMPLE RATE:\t\t" << this->rate << " Hz \n";
    std::cout << "SAMPLE INTERVAL:\t" << this->interval << " mys \n\n";
}

// Destructor
sampler::~sampler()
{
    this->stop();
}

//// Method Definitions ///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Return sampler status
sampler::state sampler::getStatus()
{
    return this->status;
}

// EVENT: Start the sampler
bool sampler::onStart()
{
    this->status = state::sampling;
    return true;
}

// EVENT: Stop the sampler
void sampler::onStop()
{
    this->status = state::idle;
}

// EVENT: Process a chunk of samples
bool sampler::onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
{
    if (this->status == state::idle)
    {
        return true;
    }

    this->status = state::processing;

    //std::cout << "PROCESSING SAMPLES [" << sampleCount << "] ...\n";

    // create vector of samples
    const short* data = &samples[0]; // Int16*
    std::vector<short> samplesCopy(data, data + sampleCount);

    // callback with copy of samples chunk
    this->callback(samplesCopy);

    // continue sampling
    this->status = state::sampling;
    return true;
}


