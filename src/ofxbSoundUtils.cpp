#include "ofxbSoundUtils.h"

ofxbSoundUtils::ofxbSoundUtils()
{
    
}

ofxbSoundUtils::~ofxbSoundUtils()
{
    
}

void ofxbSoundUtils::setLoudnessType(int _type)
{
    loudness_type = _type;
}

void ofxbSoundUtils::drawSpectrum(int _x, int _y, int _w, int _h)
{
    if( loudness_type == OFXBSU_LOUDNESS_TYPE_POWER ){
        fbo_spectrum_power.draw(_x, _y, _w, _h);
    }
    else if( loudness_type == OFXBSU_LOUDNESS_TYPE_DB ){
        fbo_spectrum_db.draw(_x, _y, _w, _h);
    }
}



void ofxbSoundUtils::drawSpectrogram(int _x, int _y, int _w, int _h)
{
    fbo_spectrogram.draw(_x,_y, _w, _h);
}

void ofxbSoundUtils::update()
{
    fbo_spectrum_power.begin();
    {
        ofClear(0);
        ofNoFill();
        ofSetColor(255);
        ofBeginShape();
        for( int i = 0; i < fft.spectrum.size(); i++ ){
            ofVertex(i, bufsize/2-fft.spectrum[i].power);
        }
        ofEndShape();
    }
    fbo_spectrum_power.end();
    
    fbo_spectrum_db.begin();
    {
        ofClear(0);
        ofNoFill();
        ofSetColor(255);
        ofBeginShape();
        for( int i = 0; i < fft.spectrum.size(); i++ ){
            ofVertex(i, 40-fft.spectrum[i].db);
        }
        ofEndShape();
    }
    fbo_spectrum_db.end();
    
    int framesize = bufsize/2;
    for( int i = framesize-1; i >= 1; i-- ){
        for( int j = 0; j < bufsize/2; j++ ){
            buf_spectrogram[j][i] = buf_spectrogram[j][i-1];
        }
    }
    for( int j = 0; j < framesize; j++ ){
        if( loudness_type == OFXBSU_LOUDNESS_TYPE_POWER){
            buf_spectrogram[j][0] = fft.spectrum[framesize-1-j].power;
        }
        else if( loudness_type == OFXBSU_LOUDNESS_TYPE_DB){
            buf_spectrogram[j][0] = fft.spectrum[framesize-1-j].db;
        }
    }
    
    float max = -1000.0;
    float min = 10000.0;
    fbo_spectrogram.begin();
    glBegin(GL_POINTS);
    for( int i = 0; i < framesize; i++ ){
        for( int j = 0; j < framesize; j++ ){
            float p = buf_spectrogram[i][j];
            if( p > max )max = p;
            if( p < min )min = p;
            if( loudness_type == OFXBSU_LOUDNESS_TYPE_POWER) p = ofMap(p, 0.0, 10.0, 0, 255);
            if( loudness_type == OFXBSU_LOUDNESS_TYPE_DB) p = ofMap(p, -20, 20, 0, 255);

            if( p > 255 )p = 255;
            if( p < 0 ) p = 0;
            ofSetColor(p);
            glVertex2f(j,i);
        }
    }
    glEnd();
    fbo_spectrogram.end();
    
   
}
void ofxbSoundUtils::setup(int _bufsize)
{
    int number_output_channels = 0;
    int number_input_channels  = 1;
    auto devices = soundStream.getDeviceList();
    for( int i = 0; i < devices.size(); i++ ){
        if( devices[i].isDefaultInput ){
            string_device_info += "Input Device: "+devices[i].name + "\n";
            settings.setInDevice(devices[i]);
            string_device_info += " - Configurable Sampling Rate: ";
            for( int j = 0; j < devices[i].sampleRates.size(); j++){
                string_device_info += ofToString(devices[i].sampleRates[j])+",";
            }
            string_device_info += "\n";
        }
    }
    string_device_info += "Sampling Rate: " + ofToString(settings.sampleRate) +"\n";
    settings.setInListener(this);
    settings.numOutputChannels = 0;
    settings.numInputChannels = number_input_channels;
    settings.bufferSize = bufsize = _bufsize;
    soundStream.setup(settings);
    
    buf_spectrogram = new float*[_bufsize/2];
    for( int i = 0; i < _bufsize/2; i++){
        buf_spectrogram[i] = new float[_bufsize];
    }
    fft.setup(settings.bufferSize, settings.sampleRate);
    fbo_spectrum_power.allocate(_bufsize/2, _bufsize/2);
    fbo_spectrum_db.allocate(_bufsize/2, _bufsize/2);
    fbo_spectrogram.allocate(_bufsize/2, _bufsize/2);
    sound = new float[_bufsize];
    loudness_type = OFXBSU_LOUDNESS_TYPE_POWER;
}

void ofxbSoundUtils::audioIn(ofSoundBuffer &input)
{
    for (int i = 0; i < input.getBuffer().size(); i++){
        sound[i] = input.getSample(i,0);
    }
    
    fft.update(sound);
    
}

