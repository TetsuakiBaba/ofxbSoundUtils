#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    sound_utils.setup(1024);
    sound_utils.setLoudnessType(OFXBSU_LOUDNESS_TYPE_POWER );
}

//--------------------------------------------------------------
void ofApp::update(){
    sound_utils.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    sound_utils.drawSpectrum(0,0,ofGetWidth(), ofGetHeight()/2);
    sound_utils.drawSpectrogram(0,ofGetHeight()/2,ofGetWidth(), ofGetHeight()/2);
    sound_utils.drawSettings(20,20);
    ofDrawBitmapString(ofGetFrameRate(), ofGetWidth()-100, 20);
}


void ofApp::keyPressed(int key)
{
    // Saves spectrogram as a png file.
    if(key =='s'){
        ofPixels pixels;
        ofImage img;
        sound_utils.fbo_spectrogram.readToPixels(pixels);
        img.setFromPixels(pixels);
        string filename;
        filename = ofGetTimestampString()+".png";
        img.save(filename,OF_IMAGE_QUALITY_BEST);
    }
}
