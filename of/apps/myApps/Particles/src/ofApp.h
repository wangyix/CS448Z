#pragma once

#include <mutex>

#include "ofMain.h"
#include "RingBuffer.h"


#define PIXELS_PER_METER 50.0

#define BOX_ZMIN (-400.0 / PIXELS_PER_METER)
#define BOX_ZMAX 0.0

#define GRAVITY_MAG 6.0

#define AUDIO_SAMPLE_RATE 44100
#define CHANNELS 2

#define WAV_SAMPLES 4876

#define NUM_MATERIALS 4

#define MOUSE_CURSOR_MASS 80.0    // how attractive the cursor is when held down; unit is abitrary

struct Material {
    Material(float density, float yMod, float pRatio, const ofColor& color)
        : density(density), yMod(yMod), pRatio(pRatio), color(color) {}
    float density;
    float yMod;
    float pRatio;
    ofColor color;
};

class ofApp : public ofBaseApp{
public:
	void setup() override;
    void exit() override;
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseMoved(int x, int y) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void windowResized(int w, int h) override;
    void dragEvent(ofDragInfo dragInfo) override;
    void gotMessage(ofMessage msg) override;

    void audioOut(float* output, int bufferSize, int nChannels) override;

private:

private:
    ofLight pointLight;
    ofPlanePrimitive leftWall, rightWall, bottomWall, topWall, backWall;

    ofVec3f pMin, pMax;                 // scene bounds

    vector<ofMesh> meshes;

    ofVec3f gravity;                    // acceleration due to gravity
    
    bool attract;
    ofVec3f attractPos;
    
    RingBuffer<float, CHANNELS * AUDIO_SAMPLE_RATE> audioBuffer;
    std::mutex audioBufferLock;

    ofVec3f listenPos;
};
