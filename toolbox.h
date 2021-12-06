#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>

#include "definitioner.h"

//// Public Declarations [Interface] //////////////////////////////////////////////////////////////////////////////////////////////

namespace dtmf
{
    namespace toolbox
    {

        // Public Constructs
        ;

        // Public Members
        ;

        // Public Methods


        void									playbackSequence(std::string args);

        void									testGenerator();
        void									testSampler();

        void									logDecoder();
        void									logDecoderQuiet();
        void									logSequence();
        void									logGoertzel(std::string args);
        void									logGoertzelAverage(std::string args);

        void									testDecoderKeyboardReciever();
        void									testDecoderKeyboardSender();

        void									testStepWindow(std::string args);

        void									testLatency();
        double									testLatency2();
        void									averageLatency();

        void									testGeneratorGoertzel(std::string args);
        void									testGoertzel(std::string args);

        void									calibrateThresholds();
        void									calibrateThresholds2();

        void									testGoertzelVsFFT();
        void									exportSamples(std::vector<short> &samples, std::string filename = "output_samples.dat");
        template <class T>	void				exportArray(T& data, std::string filename = "output_array.dat");
        template <class key, class value> void	exportMap(std::map<key, value> map, std::string filename = "output_map.dat");
        void									exportAudio(std::vector<short> &samples, std::string filename = "output_sound.wav");
        void									plotSamples(std::vector<short> &samples, std::string filename = "samples_plot.dat", std::array<std::string, 3> labels = { "Samples Plot", "x-axis", "y-axis" });
        template <class T>	void				plotArray(T& data, std::string filename = "array_plot.dat", std::array<std::string, 3> labels = { "Samples Plot", "x-axis", "y-axis" });
        template <class key, class value> void	plotMap(std::map<key, value> &map, std::string filename = "map_plot.dat", std::array<std::string, 3> labels = { "Map Plot", "x-axis", "y-axis" });

        std::vector<short>						convertAudio(std::string filename);
        std::map<double, short>					convertLatencyMap(std::map< double, std::vector<short>> map, double startTime);
        std::vector<short>                     convertSFBuffer(sf::SoundBuffer& buffer);
        std::string								getWorkingDirectory();

    }
}