#include "ofMain.h"

#ifndef _bFFT
#define _bFFT

#ifndef M_PI
#define	M_PI		3.14159265358979323846  /* pi */
#endif


struct Spectrum{
    float power;
    float db;
    float Hz;
};

class bFFT {
	
	public:
		
	bFFT();
	~bFFT();
    void setup(int _bufsize, int _sampling_rate);
    void update( float *_input_sound );
    
    /* Calculate the power spectrum */
    void powerSpectrum(int start, int half, float *data, int windowSize,float *magnitude,float *phase, float *power, float *avg_power);
    /* ... the inverse */
    void inversePowerSpectrum(int start, int half, int windowSize, float *finalOut,float *magnitude,float *phase);
    
    // get freqency spectrum. if there is no data, make interpolate and get estimated value spectrume.
    int getFreqSpectrum(float freq, int half, float *magnitude);
    
    // get step size of frequency
    float getFreqStep(int sampling_rate, int buffer_size);
    float getFreqStep();
  
    int bufsize,sampling_rate;
    float *magnitude;  // 振幅
    float *phase;      //
    float *power;      // 振幅×振幅
    float avg_power;
    vector<Spectrum>spectrum;
};


#endif	
