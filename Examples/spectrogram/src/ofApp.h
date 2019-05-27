#pragma once

#include "ofMain.h"
#include "ofxbSoundUtils.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    ofxbSoundUtils sound_utils;
};
