#pragma once

#include "ofMain.h"
#include "ofxbSoundUtils.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    
    ofxbSoundUtils sound_utils;
};
