#pragma once

#include "ofMain.h"
#include "FFT.h"

#define OFXBSU_LOUDNESS_TYPE_POWER 1
#define OFXBSU_LOUDNESS_TYPE_DB 2

class ofxbSoundUtils{
public:
    ofxbSoundUtils();
    ~ofxbSoundUtils();
    
    void setup(int _bufsize);
    void setLoudnessType(int _type);
    void audioIn(ofSoundBuffer & input);
    void update();
    void drawSpectrum(int _x, int _y, int _w, int _h);
    void drawSpectrogram(int _x, int _y, int _w, int _h);


    ofSoundStream soundStream;
    ofSoundStreamSettings settings;
    float **buf_spectrogram;

    FFT fft;
    int bufsize;
    ofPixels pixels_spectrogram;
    ofFbo fbo_spectrum_power;
    ofFbo fbo_spectrum_db;
    ofFbo fbo_spectrogram;

    int loudness_type;
    float *sound;
    string string_device_info;
};
