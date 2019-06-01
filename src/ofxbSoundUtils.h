#pragma once

#include "ofMain.h"
#include "bFFT.h"


#define OFXBSU_LOUDNESS_TYPE_POWER 1
#define OFXBSU_LOUDNESS_TYPE_DB 2

class ofxbSoundUtils{
public:
    ofxbSoundUtils();
    ~ofxbSoundUtils();
    
    void setup(int _bufsize);
    void setup(int _bufsize, bool _use_output);
    void setup(int _bufsize, int _sampling_rate);
    void setup(int _bufsize, int _sampling_rate, bool _use_output);
    void setLoudnessType(int _type);
    void audioIn(ofSoundBuffer & input);
    void audioOut(ofSoundBuffer & input);
    void update();

    void drawSpectrum(int _x, int _y, int _w, int _h);
    void drawSpectrogram(int _x, int _y, int _w, int _h);
    void updateFbo();
    void drawSettings(int _x, int _y);


    ofSoundStream soundStream;
    ofSoundStreamSettings settings;
    float **buf_spectrogram;

    bFFT fft;
    int bufsize;
    ofPixels pixels_spectrogram;
    ofFbo fbo_spectrum_power;
    ofFbo fbo_spectrum_db;
    ofFbo fbo_spectrogram;

    int loudness_type;
    float *sound;
    string string_device_info;
    int count_should_be_updated;
    

    
};
