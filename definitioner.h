#pragma once
#include <valarray>
#include <complex>

// _ //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using			uint				= unsigned int;									// Unsigned Integer
using			uchar				= unsigned char;								// Unsigned Char
using			cArray				= std::valarray<std::complex<double>>;			// Complex Value Array

constexpr auto 	DATA_PATH			= "dat/";										// Path of data directory (default "dat/")

constexpr auto	PI					= 3.141592653589732;							// PI

constexpr auto	SAMPLE_RATE			= 44100;										// Sample Rate in [Hz] (default = 44100)
constexpr auto	SAMPLE_INTERVAL		= 20;											// Interval of sample processing [ms]

constexpr int	CHUNK_SIZE			= (SAMPLE_RATE * SAMPLE_INTERVAL * 0.001f);		// Number of samples in a chunk
constexpr auto	CHUNK_SIZE_MIN		= 458;											// Minimum number of samples in a chunk for Goertzel bin separation
constexpr auto	CHUNK_SIZE_MAX		= 1.2 * CHUNK_SIZE;								// Maximum number of samples in a chunk before segregation

constexpr auto	AMPLITUDE_MAX		= 32767;										// Maximum possible signal amplitude (100%) (SIGNED INT16 -> +- 2^15 - 1)
constexpr auto	DURATION			= 50;											// Signal duration [ms]
constexpr auto	PAUSE				= 50;											// Pause between signals [ms]
constexpr auto	FADE				= 0.10f;										// Signal fade percentage decimal (0.1 = 10%)

constexpr int	LATENCY				= 100;											// Physical latency [ms]
constexpr int	LATENCY_BUFFER		= 30;											// Average variance in latency [ms]
constexpr auto	LATENCYVARIANCE		= 30;											// Expected maximum variance in latency [ms]

constexpr int	DEBOUNCE			= DURATION + 20;								// Debounce between detection of two signals [ms]

constexpr auto	TH_MULTIPLIER_H		= 1.0;											// Threshold multiplier
constexpr auto	TH_MULTIPLIER_L		= TH_MULTIPLIER_H * 0.95;						// Threshold multiplier (for redundancy)
constexpr auto	TH_LEVELER			= 0.8;											// Threshold leveler (for calibration)

constexpr int	TIMEOUT				= 500;											// Data-link Layer Timeout

// DTMF Frequncies & Thresholds
const int		freq[8]				= { 697,	  770,	  852,	  941,	 1209,	 1336,	 1477,	 1633 };
static int		freqThresholds[8]	= { 75,		   75,	   75,	   75,	  100,	  100,	  100,	  100 };
const float		freqMultiplier[8]	= { 1.00,	 1.00,	  1.00,  1.00,	 1.00,	 1.00,	 1.00,	 1.00 };

// Deprecated
constexpr auto	STEP_WINDOW_SIZE	= 5;											// Number of minimum samples in queue (signal duration / sample interval)
constexpr int	OLD_DEBOUNCE		= (DURATION + 40) * 1;							// Default debounce value [ms] (default = 90% of DURATION + PAUSE)
