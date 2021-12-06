#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <queue>
#include <array>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <functional>
#include <windows.h>
#include <iostream>
#include "decoder.h"
#include "sampler.h"
//#include "sampler2.h"
#include <stdlib.h>
#include "processor.h"
#include "toolbox.h"
//Private deklarationer

namespace decoder
{
    using namespace std::chrono;


    std::atomic<state>					status = state::uninitialized;
    std::deque<std::vector<short>>		queue;
    std::vector<short>					buffer;
    std::mutex							queueMutex;
    std::condition_variable				queueCondition;
    std::thread							worker;
    sampler*							rec1;
    std::vector<short>                  endelig;
    int count=0;
    bool								allowPlayback = false;
    std::atomic<bool>					running;

    std::array<float, 8>				previousGoertzelArray;
    bool								previousThresholdBroken;
    int									previousToneId;

    high_resolution_clock				clock;
    time_point<high_resolution_clock>	debounce;

    std::function<void(uint)>			callback;

    // Private metoder
    void								threadWindowed();

    void								decode(std::vector<short> &samples);
    void								decode2(std::vector<short> &samples);

    void								appendQueue(std::vector<short> samples);

    bool								thresholdTest(std::array<float, 8> goertzelArray);
    std::array<int, 2>					extractIndexes(std::array<float, 8> &goertzelArray);
    int									extractToneId(std::array<int, 2> &indexes);
    void								yndlings();

}

//Lav og kør ny sampler, og start ny thread
void decoder::run(bool allowPlayback)
{
    // initialize variables
    decoder::callback		= callback;
    decoder::rec1			= new sampler(&decoder::appendQueue, allowPlayback);
    decoder::allowPlayback	= allowPlayback;

    // start sampler
    decoder::rec1->start(SAMPLE_RATE);
    decoder::status			= state::running;
    std::cout << "stratsample" << std::endl;

    // start worker thread
    decoder::previousToneId = -1;
    decoder::debounce		= decoder::clock.now();
    decoder::running		= true;
    decoder::worker			= std::thread(&decoder::threadWindowed);
    //decoder::worker			= std::thread(&decoder::yndlings);
    std::cout << "lavet worker" << std::endl;

}


// End the decoder
void decoder::end()
{
    // stop and delete sampler
    decoder::rec1->stop();
    delete decoder::rec1;

    // stop worker thread
    decoder::status		= state::uninitialized;
    decoder::running	= false;
    decoder::worker.join();
}


// Thread function (step window decoding / chunk combination)
void decoder::threadWindowed()
{
    while (decoder::running)
    {
        // check status
        if (decoder::status != state::running)
        {
            continue;
        }

        // queue critical section
        decoder::queueMutex.lock();

        // check that queue has enough elements
        if (decoder::queue.size() < STEP_WINDOW_SIZE)
        {
            decoder::queueMutex.unlock();
            continue;
        }

        // clear buffer
        decoder::buffer.clear();

        // compose buffer from queue
        for (const auto& sampleChunks : decoder::queue)
        {
            decoder::buffer.insert(decoder::buffer.end(), sampleChunks.begin(), sampleChunks.end());
        }

        // pop first element of queue
        decoder::queue.pop_front();

        decoder::queueMutex.unlock();

        // decode the copied samples
        decoder::decode(buffer);
    }
}

// Thread function (instant decoding)
void decoder::threadInstant()
{
    while (decoder::running)
    {
        /*
        The method is protected by a mutex, which is locked using scope guard std::unique_lock. If the queue is empty,
        we wait on the condition variable queueCondition. This releases the lock to other threads and blocks until we are
        notified that the condition has been met.
        https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
        */

        // acquire queue mutex
        std::unique_lock<std::mutex> queueLock(decoder::queueMutex);

        // check if queue empty; wait for conditional variable if false
        while (queue.empty())
        {
            decoder::queueCondition.wait(queueLock);
            //std::cout << "tom" << std::endl;
        }

        // make copy of first element in queue
        auto samplesCopy = decoder::queue.front();

        // pop the element
        decoder::queue.pop_front();

        // unclock queue
        queueLock.unlock();

        // decode the copied samples
        decoder::decode(samplesCopy);

    }
}

// Add a (copy of) vector of samples to decoding queue
void decoder::appendQueue(std::vector<short> samples)
{
    // critical section
    decoder::queueMutex.lock();

    //std::cout << "[QUEUE] Adding to queue with [" << decoder::queue.size() << "] elements.\n";

    // segregate chunk if it contains multiple (i.e. size > ~441)
    if (samples.size() > CHUNK_SIZE_MAX)
    {

        // variables
        std::vector<std::vector<short>>		chunks{};

        auto itr					= samples.cbegin();
        int const numberOfChunks	= samples.size() / CHUNK_SIZE;
        int const remainder			= samples.size() % CHUNK_SIZE;

        // create vector of chunks
        for (int i = 0; i < numberOfChunks; i++)
        {
            chunks.emplace_back(std::vector<short>{itr, itr + CHUNK_SIZE});
            itr += CHUNK_SIZE;
        }

        // append remaining samples to last chunk
        if (remainder > 0)
        {
            chunks[numberOfChunks - 1].insert(chunks[numberOfChunks - 1].end(), samples.end() - remainder, samples.end());
        }

        // append chunks onto queue
        for (const auto& chunk : chunks)
        {
            decoder::queue.push_back(chunk);
        }
    }
    else
    {
        // push chunk onto queue
        decoder::queue.push_back(samples);
    }

    decoder::queueMutex.unlock();

    decoder::queueCondition.notify_one();
}

// Test whether two of magnitudes surpass their according thresholds; return boolean
bool decoder::thresholdTest(std::array<float, 8> goertzelArray)
{
    int counter = 0;

    for (int i = 0; i < 8; i++)
    {
        if (goertzelArray[i] > freqThresholds[i])
        {
            counter++;
        }
    }

    return (counter == 2);
}

// Convert goertzelArray to indexes (row & column) of two most prominent frequencies; return array[2]
std::array<int, 2> decoder::extractIndexes(std::array<float, 8> &goertzelArray)
{
    float magnitudeLowMax		= 0;
    float magnitudeHighMax		= 0;

    std::array<int, 2> indexes	= { -1, -1 };

    // Iterate through array of goertzel magnitudes
    for (uint i = 0; i < goertzelArray.size(); i++)
    {
        // define magnitude & threshold for current frequency index (i)
        auto magnitude = goertzelArray[i] * freqMultiplier[i];
        auto threshold = freqThresholds[i] * ((previousToneId == -1) ? TH_MULTIPLIER_H : TH_MULTIPLIER_L);

        // low frequencies
        if (i < 4 && magnitude > threshold && magnitude > magnitudeLowMax)
        {
            magnitudeLowMax = magnitude;
            indexes[0] = i;
        }

            // high frequencies
        else if (i >= 4 && magnitude > threshold && magnitude > magnitudeHighMax)
        {
            magnitudeHighMax = magnitude;
            indexes[1] = (i - 4);
        }
    }

    // return array of indexes
    return indexes;
}

// Convert DTMF indexes (row & column) to toneID (0-15); return int
int decoder::extractToneId(std::array<int, 2> &indexes)
{
    int indexLow	= indexes[0];
    int indexHigh	= indexes[1];

    if (indexHigh < 0 || indexLow < 0)
    {
        return -1;
    }

    return (indexLow * 4 + indexHigh);
}

// Decode a chunk of samples from the queue
void decoder::decode(std::vector<short> &samples)
{
    for(int i=0; i<samples.size();i++){
        endelig.push_back(samples[i]);
    }
    // check debounce
    if (static_cast<duration<double, std::milli>>(decoder::clock.now() - decoder::debounce).count() < DEBOUNCE)
    {
        std::cout << "decode1" << std::endl;
        //return;
    }


    // log
    //std::cout << "[DECODER] Decoding [" << samples.size() << "] samples...\n\n";

    // update status
    decoder::status = state::working;
    //std::cout << "status working" << std::endl;
    // apply hanning window
    //processor::hannWindow(samples);
    //Apply fft?
    //cArray f;
    //f=processor::fft(samples);
    /*
    for (int i=0; i<f.size();i++){
        std::cout << f[i] << std::endl;
    }
     */
    //for(int i=0; i<f.size();i++) {
    //std::cout << "fft'et: " << f[i] << std::endl;
    //}
    // apply zero padding if chunk too small
    if (samples.size() < CHUNK_SIZE_MIN)
    {
        processor::zeroPadding(samples, CHUNK_SIZE_MIN);
        std::cout << "samples.size() > ChunkMAX" << std::endl;
    }

    // compile goertzelArray for all DTMF frequencies
    //auto goertzelArray = processor::goertzelArray(samples);

    // extract indexes (row & column) of most prominent frequencies
    //auto indexes = decoder::extractIndexes(goertzelArray);

    // check harmonies
    ;

    // convert indexes to DTMF toneId
    //auto toneId = decoder::extractToneId(indexes);

    // redundancy check & callback
    /*
    if (toneId >= 0 && decoder::previousToneId == toneId)
    {
        decoder::debounce = decoder::clock.now();
        decoder::previousToneId = -1;
        //processor::printGoertzelArray(goertzelArray);
        decoder::callback(toneId);
    }
    else if (previousToneId == -1)
    {
        decoder::previousToneId = toneId;
    }
    else
    {
        decoder::previousToneId = -1;
    }
    */
    // update status
    decoder::status = state::running;
    //count++;
    std::cout << endelig.size() << std::endl;
    if(endelig.size()>=39000){
        //dtmf::toolbox::exportAudio(endelig, "teeeeest.ogg");
        //std::ofstream myfile;
        //myfile.open("stream.txt");
        //for(int j=0; j<endelig.size();j++){
        //    myfile << endelig[j] << " ";
        //}
        cArray f;
        f=processor::fft(endelig);
            f=f.cshift(f.size()/2);
        std::ofstream myfile;
        myfile.open("FFTstream.txt");
        for(int j=0; j<f.size();j++){
            myfile << abs(f[j]) << " ";
        }
        decoder::end();
        abort();
    }

}

// Decode a chunk of samples from the queue (with threshold breaking)
void decoder::decode2(std::vector<short> &samples)
{
    decoder::status = state::working;

    // decode
    //std::cout << "[DECODER] Decoding [" << samples.size() << "] samples...\n\n";

    // check debounce
    if ((int)static_cast<duration<double, std::milli>>(decoder::clock.now() - decoder::debounce).count() < OLD_DEBOUNCE)
    {
        decoder::status = state::running;
        std::cout << "kører" << std::endl;
        return;
    }

    // apply hanning window to samples
    processor::hannWindow(samples);

    // compile goertzelArray for all DTMF frequencies
    auto goertzelArray = processor::goertzelArray(samples);
    std::cout << "goertzel" << std::endl;
    // check if any values surpass thresholds
    bool thresholdBroken = decoder::thresholdTest(goertzelArray);

    /*
    A surpassed boolean is needed for every DTMF frequency!	Also update
    thresholdTest so only DTMF tones pass through (>4).

    */

    // check if previous treshold was surpassed and current was not
    if (previousThresholdBroken && !thresholdBroken)
    {
        // extract indexes (row & column) of most prominent frequencies of previousGoertzel
        auto indexes = decoder::extractIndexes(previousGoertzelArray);

        // convert indexes to DTMF toneID
        auto toneId = decoder::extractToneId(indexes);
        std::cout << "kører" << std::endl;
        // callback
        if (toneId >= 0)
        {
            decoder::debounce = decoder::clock.now();
            decoder::callback(toneId);
        }
    }

    // save values for next iteration
    previousGoertzelArray		= goertzelArray;
    previousThresholdBroken		= thresholdBroken;

    decoder::status = state::running;
    std::cout << "kører" << std::endl;
}