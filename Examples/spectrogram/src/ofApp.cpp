#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    sound_utils.setup(512*2);
    sound_utils.setLoudnessType(OFXBSU_LOUDNESS_TYPE_DB);
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
    ofDrawBitmapString(sound_utils.string_device_info, 20,100);

}

