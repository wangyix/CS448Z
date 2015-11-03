#include "ofApp.h"

#include <assert.h>
#include <algorithm>

static const Material STEEL_MATERIAL = Material(8940.f, 123.4f * 1e9f, 0.34f, ofColor(255, 0, 0, 255));
static const Material CERAMIC_MATERIAL = Material(2700.f, 72.f * 1e9f, 0.19f, ofColor(0, 255, 0, 255));
static const Material GLASS_MATERIAL = Material(2700.f, 62.f * 1e9f, 0.20f, ofColor(0, 0, 255, 255));
static const Material PLASTIC_MATERIAL = Material(1200.f, 2.4f * 1e9f, 0.37f, ofColor(255, 0, 255, 255));

static const Material materials[NUM_MATERIALS] = { STEEL_MATERIAL, CERAMIC_MATERIAL, GLASS_MATERIAL, PLASTIC_MATERIAL };


static const string sphereObjFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/sphere/sphere.obj";
static const string rodObjFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/rod/rod.obj";
static const string groundObjFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/ground/ground.obj";
static const string sphereModesFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/sphere/modes.txt";
static const string rodModesFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/rod/modes.txt";
static const string groundModesFileName = "C:/Users/wangyix/Desktop/GitHub/CS448Z/of/apps/myApps/Particles/models/ground/modes.txt";


static int secondsToSamples(float t) {
    return ((int)(t * AUDIO_SAMPLE_RATE)) * CHANNELS;
}

static const int AUDIO_SAMPLES_PAD = secondsToSamples(AUDIO_BUFFER_PAD_TIME);

//--------------------------------------------------------------
void ofApp::setup(){
    windowResized(ofGetWidth(), ofGetHeight());

    // initialize rigid bodies
    //bodies.emplace_back(sphereModesFileName, sphereObjFileName, materials[0], 1.f);
    //bodies.push_back(RigidBody(rodModesFileName, 7e10f, 0.3f, 1.f, 1000.f, 1e-11f, rodObjFileName, PLASTIC_MATERIAL, 20.f));
    bodies.push_back(RigidBody(groundModesFileName, 2e11f, 0.4f, 1.f, 3000.f, 0*1e-16f, groundObjFileName, STEEL_MATERIAL, 0.1f));
    bodies[0].x = 0.3f * pMin + 0.7f * pMax;// +ofVec3f(0.f, 2.f, 0.f);
    //bodies[0].rotate(PI / 6.f, ofVec3f(0.f, 0.f, 1.f));
    //bodies[0].rotate(-PI / 6.f, ofVec3f(1.f, 0.f, 0.f));
    //bodies[0].L = ofVec3f(0.f, 0.f, 1000.f);
    //bodies[0].w = bodies[0].IInv * bodies[0].L;

    //bodies[1].x = 0.7f * pMin + 0.3f * pMax + ofVec3f(0.f, 2.f, 0.f);
    //bodies[1].L = ofVec3f(0.f, 0.f, 1000.f);
    //bodies[1].w = bodies[0].IInv * bodies[0].L;


    // initialize light
    ofSetSmoothLighting(true);
    pointLight.setDiffuseColor(ofFloatColor(.85, .85, .55));
    pointLight.setSpecularColor(ofFloatColor(1.f, 1.f, 1.f));

    gravity = ofVec3f(0.f, GRAVITY_MAG, 0.f);
    attract = false;

    listenPos = ofVec3f(0.5f * (pMin.x + pMax.x), 0.5f * (pMin.y + pMax.y), BOX_ZMAX);

    ofSoundStreamSetup(CHANNELS, 0, AUDIO_SAMPLE_RATE, 256, 4);
    //ofSetFrameRate(100);

    audioBuffer.pushZeros(AUDIO_SAMPLES_PAD);
}

//--------------------------------------------------------------
void ofApp::exit() {
}

enum WALL_ID{ NONE=0, XMIN=1, XMAX=2, YMIN=3, YMAX=4, ZMIN=5, ZMAX=6 };

int ofApp::particleCollideWall(const ofVec3f& p, const ofVec3f& v, float tMin, float tMax, float* tc) {
    assert(tMax > 0.f);
    float tt = tMax;
    int id = NONE;
    if (v.x > 0.f) {
        float t = (pMax.x - p.x) / v.x;
        if (tMin < t && t < tt) {
            tt = t;
            id = XMAX;
        }
    } else if (v.x < 0.f) {
        float t = (pMin.x - p.x) / v.x;
        if (tMin < t && t < tt) {
            tt = t;
            id = XMIN;
        }
    }
    if (v.y > 0.f) {
        float t = (pMax.y - p.y) / v.y;
        if (tMin < t && t < tt) {
            tt = t;
            id = YMAX;
        }
    } else if (v.y < 0.f) {
        float t = (pMin.y - p.y) / v.y;
        if (tMin < t && t < tt) {
            tt = t;
            id = YMIN;
        }
    }
    if (v.z > 0.f) {
        float t = (pMax.z - p.z) / v.z;
        if (tMin < t && t < tt) {
            tt = t;
            id = ZMAX;
        }
    } else if (v.z < 0.f) {
        float t = (pMin.z - p.z) / v.z;
        if (tMin < t && t < tt) {
            tt = t;
            id = ZMIN;
        }
    }
    if (tt < tMax) {
        *tc = tt;
    }
    return id;
}

//--------------------------------------------------------------
void ofApp::update(){
    float dt = min(0.01, ofGetLastFrameTime());
    if (dt <= 0.f) return;

    // apply non-rotational forces to bodies
    for (RigidBody& body : bodies) {
        body.v += gravity * dt;
        body.P = body.m * body.v;
    }

    float qSums[AUDIO_SAMPLE_RATE];    // 1 second worth
    memset(qSums, 0, AUDIO_SAMPLE_RATE*sizeof(float));
    int qsComputed = 0;

    // compute collisions of vertices against walls
    for (RigidBody& body : bodies) {

        vector<VertexImpulse> impulses;

        int i_c = 0;                            // index of the vertex that collides
        ofVec3f ri_c(0.f, 0.f, 0.f);            // world ri of the vertex that collides
        ofVec3f vi_c(0.f, 0.f, 0.f);            // world velocity of vertex that collides
        while (true) {
            // find vertex with earliest wall collision, if any
            float dt_c = dt;  // collision will occur dt_c from now
            int wallId = NONE;
            for (int i = 0; i < body.mesh.getNumVertices(); i++) {
                ofVec3f ri = body.R * body.mesh.getVertex(i);
                ofVec3f xi = body.x + ri;
                ofVec3f vi = body.v + (body.w.crossed(ri));
                float t;
                int id = particleCollideWall(xi, vi, -100000000.f, dt, &t);
                if (id != NONE && t < dt_c) {
                    dt_c = t;
                    wallId = id;
                    i_c = i;
                    ri_c = ri;
                    vi_c = vi;
                }
            }
            
            // compute impulse imparted by collision at this vertex, if any,
            // and accumulate effect of impulse into P, L
            if (wallId != NONE) {
                ofVec3f n;
                switch (wallId) {
                case XMIN:
                    n = ofVec3f(1.f, 0.f, 0.f);
                    //printf("x0 ");
                    break;
                case XMAX:
                    n = ofVec3f(-1.f, 0.f, 0.f);
                    //printf("x1 ");
                    break;
                case YMIN:
                    n = ofVec3f(0.f, 1.f, 0.f);
                    //printf("y0 ");
                    break;
                case YMAX:
                    n = ofVec3f(0.f, -1.f, 0.f);
                    //printf("y1 ");
                    break;
                case ZMIN:
                    n = ofVec3f(0.f, 0.f, 1.f);
                    //printf("z0 ");
                    break;
                case ZMAX:
                    n = ofVec3f(0.f, 0.f, -1.f);
                    //printf("z1 ");
                    break;
                default:
                    break;
                }

                const float e = 0.5f;
                float j = -(1.f + e)*(vi_c.dot(n)) /
                    (  1.f / body.m + ( (body.IInv * (ri_c.crossed(n))).crossed(ri_c) ).dot(n)  );

                ofVec3f impulse = j*n;

                // update linear, angular momentum with impulse
                body.P += impulse;
                body.L += (ri_c.crossed(impulse));
                // update linear, angular velocities from momentum
                body.v = (body.P / body.m);
                body.w = (body.IInv * body.L);

                impulses.emplace_back(i_c, impulse);

            } else {
                // no collision
                break;
            }
        }

        body.step(dt);
        body.stepW(dt);

        int qsComputedThisBody = body.stepAudio(dt, impulses, 1.f / AUDIO_SAMPLE_RATE, qSums);

        if (qsComputedThisBody > qsComputed) {
            qsComputed = qsComputedThisBody;
        }
    }


    // scale qSums to get audio samples
    float qScale = 0.02f;
    float audioSamples[CHANNELS * AUDIO_SAMPLE_RATE];   // 1 second worth
    int audioSamplesComputed = 0;
float maxSample = 0.f;
float minSample = 0.f;
    for (int k = 0; k < qsComputed; k++) {
        for (int i = 0; i < CHANNELS; i++) {
            audioSamples[audioSamplesComputed++] = qScale * qSums[k];
        }
maxSample = max(maxSample, (qScale * qSums[k]));
minSample = min(minSample, (qScale * qSums[k]));
    }
if (maxSample > 1.f || minSample < -1.f) {
    printf("%f\t\t%f ------------------------\n", maxSample, minSample);
}else if (maxSample > 0.1f || minSample < -0.1f) {
    printf("%f\t\t%f\n", maxSample, minSample);
}
    
    // check if audioSamples has trailing zeros for sync adjustment
    const float threshold = 0.000001f;
    int trailingZerosStartAt = audioSamplesComputed;
    while (trailingZerosStartAt > 0 && abs(audioSamples[trailingZerosStartAt - 1]) < threshold) {
        trailingZerosStartAt--;
    }
    assert(trailingZerosStartAt % CHANNELS == 0);
    
    // push audio samples
    int audioSamplesToPush = audioSamplesComputed;
    audioBufferLock.lock();
    int trailingZeros = audioSamplesComputed - trailingZerosStartAt;
    if (trailingZeros > 0) {
        int bufferSizeAfterPush = audioBuffer.size() + audioSamplesToPush;
        int zerosToAdd = AUDIO_SAMPLES_PAD - bufferSizeAfterPush;
        //printf("%d\n", zerosToAdd);
        if (zerosToAdd > 0) {
            audioSamplesToPush += zerosToAdd;
            memset(&audioSamples[audioSamplesComputed], 0, zerosToAdd*sizeof(float));
        } else if (zerosToAdd < 0) {
            int zerosToRemove = -zerosToAdd;
            audioSamplesToPush -= min(zerosToRemove, trailingZeros);
        }
    } 
    audioBuffer.push(audioSamples, audioSamplesToPush);
    audioBufferLock.unlock();
}

static void drawCylinder(const ofVec3f& p1, const ofVec3f& p2) {
    ofVec3f targetDir = p2 - p1;
    float l = targetDir.length();
    targetDir /= l;

    ofCylinderPrimitive cylinder;
    cylinder.setPosition(0.f, 0.f, 0.f);
    cylinder.set(0.1f * PIXELS_PER_METER, l * PIXELS_PER_METER);

    ofVec3f cylDir = ofVec3f(0.f, 1.f, 0.f);
    ofVec3f rotateAxis = cylDir.crossed(targetDir);
    float degreesRotate = acosf(cylDir.dot(targetDir)) / PI * 180.f;

    cylinder.rotate(degreesRotate, rotateAxis);
    cylinder.setPosition(0.5f * (p1 + p2) * PIXELS_PER_METER);
    ofSetColor(0, 200, 200);
    cylinder.draw();
}

static void drawPoint(const ofVec3f& p, float size, const ofColor& color) {
    ofBoxPrimitive box(size*PIXELS_PER_METER, size*PIXELS_PER_METER, size*PIXELS_PER_METER);
    box.setPosition(p*PIXELS_PER_METER);
    ofSetColor(color);
    box.draw();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    ofEnableLighting();
    pointLight.enable();

    ofEnableDepthTest();
    
    ofSetColor(200, 200, 200);
    leftWall.draw();
    rightWall.draw();
    backWall.draw();
    topWall.draw();
    bottomWall.draw();
    
    for (int i = 0; i < bodies.size(); i++) {
        ofSetColor(bodies[i].material.color);
        ofPushMatrix();
        const ofMatrix3x3& R = bodies[i].R;
        const ofVec3f& T = bodies[i].x;
        ofMatrix4x4 objToWorld(R.a, R.d, R.g, 0.f,
                               R.b, R.e, R.h, 0.f,
                               R.c, R.f, R.i, 0.f,
                               T.x, T.y, T.z, 1.f / PIXELS_PER_METER);
        ofLoadMatrix(objToWorld * viewMatrix);
        bodies[i].mesh.draw();
        ofPopMatrix();
    }

    ofDisableLighting();
    ofSetColor(255, 255, 255);
    ofDrawBitmapString(ofToString(ofGetFrameRate()) + "fps", 10, 15);
}

//--------------------------------------------------------------
void ofApp::audioOut(float* output, int bufferSize, int nChannels) {
    audioBufferLock.lock();
    int samplesPopped = audioBuffer.pop(output, bufferSize * CHANNELS);
    audioBufferLock.unlock();
    int samplesRemaining = CHANNELS * bufferSize - samplesPopped;
    if (samplesRemaining > 0) {
        memset(&output[samplesPopped], 0, samplesRemaining * sizeof(float));
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    switch (key) {
    case OF_KEY_LEFT:
        gravity = ofVec3f(-GRAVITY_MAG, 0.f, 0.f);
        break;
    case OF_KEY_RIGHT:
        gravity = ofVec3f(GRAVITY_MAG, 0.f, 0.f);
        break;
    case OF_KEY_UP:
        gravity = ofVec3f(0.f, -GRAVITY_MAG, 0.f);
        break;
    case OF_KEY_DOWN:
        gravity = ofVec3f(0.f, GRAVITY_MAG, 0.f);
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    attractPos = ofVec3f(x / PIXELS_PER_METER, y / PIXELS_PER_METER, 0.5f * (BOX_ZMIN + BOX_ZMAX));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    attract = true;
    attractPos = ofVec3f(x / PIXELS_PER_METER, y / PIXELS_PER_METER, 0.5f * (BOX_ZMIN + BOX_ZMAX));
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    attract = false;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    ofSetupScreenPerspective(w, h, 60.f, BOX_ZMAX * PIXELS_PER_METER, BOX_ZMIN * PIXELS_PER_METER);
    viewMatrix = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW);

    pMin.x = 0.f;
    pMax.x = w / PIXELS_PER_METER;
    pMin.y = 0.f;
    pMax.y = h / PIXELS_PER_METER;
    pMin.z = BOX_ZMIN;
    pMax.z = BOX_ZMAX;

    pointLight.setPosition(w / 2, 10.f, 0.5f*(BOX_ZMIN + BOX_ZMAX)*PIXELS_PER_METER);

    // initialize walls
    leftWall = ofPlanePrimitive((BOX_ZMAX - BOX_ZMIN)*PIXELS_PER_METER, h, 8, 8);
    rightWall = leftWall;
    leftWall.setPosition(0.f, h / 2, 0.5f*(BOX_ZMIN + BOX_ZMAX)*PIXELS_PER_METER);
    leftWall.rotate(90.f, 0.f, 1.f, 0.f);
    rightWall.setPosition(w, h / 2, 0.5f*(BOX_ZMIN + BOX_ZMAX)*PIXELS_PER_METER);
    rightWall.rotate(-90.f, 0.f, 1.f, 0.f);

    backWall = ofPlanePrimitive(w, h, 8, 8);
    backWall.setPosition(w / 2, h / 2, BOX_ZMIN*PIXELS_PER_METER);

    topWall = ofPlanePrimitive(w, (BOX_ZMAX - BOX_ZMIN)*PIXELS_PER_METER, 8, 8);
    bottomWall = topWall;
    topWall.setPosition(w / 2, 0.f, 0.5f*(BOX_ZMIN + BOX_ZMAX)*PIXELS_PER_METER);
    topWall.rotate(-90.f, 1.f, 0.f, 0.f);
    bottomWall.setPosition(w / 2, h, 0.5f*(BOX_ZMIN + BOX_ZMAX)*PIXELS_PER_METER);
    bottomWall.rotate(90.f, 1.f, 0.f, 0.f);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
