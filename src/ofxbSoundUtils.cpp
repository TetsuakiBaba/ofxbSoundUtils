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

void ofxbSoundUtils::drawSettings(int _x, int _y)
{
    ofDrawBitmapString(string_device_info, _x, _y);
}

ofPixels ofxbSoundUtils::getPixelsFromSpectrogram()
{
    ofPixels p;
    fbo_spectrogram.readToPixels(p);
    return p;
}


void ofxbSoundUtils::drawSpectrogram(int _x, int _y, int _w, int _h)
{
    fbo_spectrogram.draw(_x,_y, _w, _h);
}


void ofxbSoundUtils::updateFbo()
{
    fbo_spectrum_power.begin();
    {
        ofClear(0);
        ofNoFill();
        ofSetColor(255);
        ofBeginShape();
        for( int i = 0; i < fft.spectrum.size(); i++ ){
            float y = bufsize/2-fft.spectrum[i].power;
            if( y >= fft.spectrum.size() ){
                ofVertex(i, fft.spectrum.size()-1);
            }
            else if( y < 0 ){
                ofVertex(i, 0);
            }
            else{
                ofVertex(i, bufsize/2-fft.spectrum[i].power);
            }
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
    
    fbo_spectrogram.begin();
    glBegin(GL_POINTS);
    for( int i = 0; i < framesize; i++ ){
        for( int j = 0; j < framesize; j++ ){
            float p = buf_spectrogram[i][j];
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

void ofxbSoundUtils::update()
{
    if( count_should_be_updated <= 0 ){
        return;
    }
    updateFbo();
    count_should_be_updated--;
   
}


void ofxbSoundUtils::setup(int _bufsize, int _sampling_rate, bool _use_output)
{
    auto devices = soundStream.getDeviceList();
    for( int i = 0; i < devices.size(); i++ ){
        if( devices[i].isDefaultInput ){
            string_device_info += "Input Device: "+devices[i].name + "\n";
            settings.setInDevice(devices[i]);
            string_device_info += " - Configurable Sampling Rate: ";
            for( int j = 0; j < devices[i].sampleRates.size(); j++){
                string_device_info += ofToString(devices[i].sampleRates[j])+",";
            }
            if( _sampling_rate == 0 ){
                settings.sampleRate = devices[i].sampleRates[0];
            }
            else{
                settings.sampleRate = _sampling_rate;
            }
            settings.numInputChannels = devices[i].inputChannels;
            
        }
        if( devices[i].isDefaultOutput && _use_output ){
            settings.setOutDevice(devices[i]);
            settings.numOutputChannels = devices[i].outputChannels;
            string_device_info += "\nOutput Device: "+devices[i].name+"\n";
            string_device_info += " - Configurable Sampling Rate: ";
            for( int j = 0; j < devices[i].sampleRates.size(); j++){
                string_device_info += ofToString(devices[i].sampleRates[j])+",";
            }
        }
        
    }
    string_device_info += "\n";
    string_device_info += "Sampling Rate: " + ofToString(settings.sampleRate);
    string_device_info += ", Buffer Size: " + ofToString(_bufsize);
    string_device_info += ", Callback Freq: " + ofToString(settings.sampleRate/_bufsize);
    
    settings.bufferSize = bufsize = _bufsize;
    
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
    
    settings.setInListener(this);
    soundStream.setup(settings);
    count_should_be_updated = 0;
}

void ofxbSoundUtils::setup(int _bufsize, int _sampling_rate)
{
    setup(_bufsize, _sampling_rate, false);
}
void ofxbSoundUtils::setup(int _bufsize, bool _use_output)
{
    setup(_bufsize, 0, _use_output);
}
void ofxbSoundUtils::setup(int _bufsize)
{
    setup(_bufsize, false);
}

void ofxbSoundUtils::audioIn(ofSoundBuffer &input)
{
    for (int i = 0; i < input.getBuffer().size(); i++){
        sound[i] = input.getSample(i,0);
    }
    
    fft.update(sound);    
    count_should_be_updated++;
}


void ofxbSoundUtils::audioOut(ofSoundBuffer &output)
{
}
