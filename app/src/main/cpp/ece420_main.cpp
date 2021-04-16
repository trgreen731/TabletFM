//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#include <jni.h>
#include <math.h>
#include "ece420_main.h"
#include "ece420_lib.h"

// JNI Function
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_writeNewFreq(JNIEnv *env, jclass, jdouble);
}

// JNI Function to set the wavetable on startup
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initTable(JNIEnv *env, jclass, jint);
}

// JNI Function to set the amplitude envelope on startup
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initAmpEnv(JNIEnv *env, jclass, jint, jint, jint, jint);
}

// JNI Function to set the modulation envelope on startup
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModEnv(JNIEnv *env, jclass, jint, jint, jint, jint);
}

// JNI Function to set the modulation envelope on startup
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModIdx(JNIEnv *env, jclass, jfloat, jfloat);
}

// JNI Function to set the modulation envelope on startup
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModFactor(JNIEnv *env, jclass, jfloat);
}


//constants
#define FRAME_SIZE          1024
#define F_S                 48000
#define PI                  3.1415
#define AMP                 10000
#define TABLE_BITS          12
#define TABLE_SIZE          (1<<TABLE_BITS)
#define FIXED_SHIFT         24
#define DECIMAL_MASK        ((1<<FIXED_SHIFT)-1)

// envelope function variables
// A is the number of samples until reaching maximum value
// D is the number of samples until reaching the sustain level
// S is the sustain magnitude in percent (0 to 100)
// R is the number of samples until reaching 0 value after release
int AmpEnv[4] = {4800, 4800, 70, 4800};
int ModEnv[4] = {4800, 4800, 70, 4800};

//modulation index values that are also set on initialization
float modIdxMax = 5.0;
float modIdxMin = 0.0;

//mod frequency multiple set during initialization
float modFactor = 2.11;

// wavetable space - holds a simple sin wave
int wavetable[TABLE_SIZE];

// a sample counter to keep track of the note time between frames
int sample_count = 0;

// a release indicator telling whether a note is still playing despite no key being pressed
bool release_flag = false;
int release_count = 0;

// We have two variables here to ensure that we never change the desired frequency while
// processing a frame. Thread synchronization, etc. These will hold the carrier frequency
double FREQ_NEW_ANDROID = 0;
double FREQ_NEW = 0;

// frequency registers (modified on a new key press)
int car_freq = (int)((FREQ_NEW/F_S)*(1<<FIXED_SHIFT));
int mod_freq = (int)((FREQ_NEW/F_S)*(1<<FIXED_SHIFT));

// phase registers
int car_phase = 0;
int mod_phase = 0;

// intermediate calculation variables
int mod_out = 0;
int sig_out = 0;

// implement our FM synthesis algorithm right here
void FMSynthesis(float* bufferOut) {

    // if the carrier frequency is 0 and release flag indicates no note being played
    if(FREQ_NEW == 0 && release_flag == false){
        for(int i=0; i<FRAME_SIZE; i++){
            bufferOut[i] = (float)0;
        }
        //make sure all the registers hold 0's
        car_phase = 0;
        mod_phase = 0;
        sample_count = 0;
    }
    //otherwise perform the synthesis
    else{
        for(int i=0; i<FRAME_SIZE; i++){
            // set the output first based on the previous register values
            sig_out = (int)(wavetable[car_phase >> (FIXED_SHIFT - TABLE_BITS)] * getAmpVal());
            bufferOut[i] = AMP * ((float)sig_out)/(1 << (FIXED_SHIFT-1));

            //update the register values
            mod_phase += mod_freq;
            mod_phase = mod_phase & DECIMAL_MASK;

            car_phase += (mod_out + car_freq);
            car_phase = car_phase & DECIMAL_MASK;

            //update the modulation output with these new register values
            mod_out = (int)(wavetable[mod_phase >> (FIXED_SHIFT - TABLE_BITS)] * getModVal());
            mod_out = mod_out / F_S;

            //update the release count and sample count for each element
            sample_count++;
            if(release_flag == true){
                release_count--;
            }
        }

        //check whether release has ended
        if(release_flag == true && release_count <= 0){
            release_flag = false;
            sample_count = 0;
        }
    }

    //LOGD("Key Frequency Detected: %d\r\n", AmpEnv[1]);
}

//get a float of the current amplitude envelope value (from 0 to 1)
float getAmpVal(){
    float out;
    if(sample_count <= AmpEnv[0]){
        out = (1.0) * (float)sample_count/AmpEnv[0];
    }
    else if(sample_count <= AmpEnv[0] + AmpEnv[1]){
        out = 1.0 - ((1.0 - (float)AmpEnv[2]/100) * (float)(sample_count - AmpEnv[0])/AmpEnv[1]);
    }
    else if(release_flag == false){
        out = (float)AmpEnv[2]/100;
    }
    else if(release_flag == true && release_count > 0){
        out = ((float)AmpEnv[2]/100) * (float)release_count/AmpEnv[3];
    }
    else{
        out = 0.0;
    }
    return out;
}

//get a float of the current modulation envelope value (from min to max mod index value)
float getModVal(){
    float out;
    if(sample_count <= ModEnv[0]){
        out = (1.0) * (float)sample_count/ModEnv[0];
    }
    else if(sample_count <= ModEnv[0] + ModEnv[1]){
        out = 1.0 - ((1.0 - (float)ModEnv[2]/100) * (float)(sample_count - ModEnv[0])/ModEnv[1]);
    }
    else if(release_flag == false){
        out = (float)ModEnv[2]/100;
    }
    else if(release_flag == true && release_count > 0){
        out = ((float)ModEnv[2]/100) * (float)release_count/ModEnv[3];
    }
    else{
        out = 0.0;
    }
    //include modulation index stuff here
    return (out * (modIdxMax - modIdxMin)) + modIdxMin;
}

void ece420ProcessFrame(sample_buf *dataBuf) {
    // Keep in mind, we only have 20ms to process each buffer!
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    float bufferOut[FRAME_SIZE] = {};

    //check if a different key has been pressed or if the pressed key has been released
    if(FREQ_NEW_ANDROID != FREQ_NEW){
        if(FREQ_NEW_ANDROID != 0){
            release_flag = false;
            release_count = 0;
            sample_count = 0;
            car_freq = (int)((FREQ_NEW_ANDROID/F_S)*(1<<FIXED_SHIFT));
            mod_freq = (int)((modFactor*FREQ_NEW_ANDROID/F_S)*(1<<FIXED_SHIFT));
        }
        else{
            release_flag = true;
            release_count = AmpEnv[3];
        }
        FREQ_NEW = FREQ_NEW_ANDROID;
    }

    //perform our FMSynthesis right here
    FMSynthesis(bufferOut);

    for (int i = 0; i < FRAME_SIZE; i++) {
        int16_t newVal = (int16_t) bufferOut[i];
        uint8_t lowByte = (uint8_t) (0x00ff & newVal);
        uint8_t highByte = (uint8_t) ((0xff00 & newVal) >> 8);
        dataBuf->buf_[i * 2] = lowByte;
        dataBuf->buf_[i * 2 + 1] = highByte;
    }

    gettimeofday(&end, NULL);
    LOGD("Time delay: %ld us",  ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_writeNewFreq(JNIEnv *env, jclass, jdouble newFreq) {
    FREQ_NEW_ANDROID = (double) newFreq;
    return;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initTable(JNIEnv *env, jclass, jint wave) {
    //currently only support sin waves so wave input not used
    if(wave == 0){
        for(int i = 0;i<TABLE_SIZE; i++){
            wavetable[i] = (int)(sin(2*PI*((double)i/TABLE_SIZE)) * (1<<FIXED_SHIFT));
        }
    }
    else{
        for(int i=0; i<TABLE_SIZE; i++){
            wavetable[i] = 0;
        }
    }
    return;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initAmpEnv(JNIEnv *env, jclass, jint A, jint D, jint S, jint R) {
    AmpEnv[0] = A;
    AmpEnv[1] = D;
    AmpEnv[2] = S;
    AmpEnv[3] = R;
    return;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModEnv(JNIEnv *env, jclass, jint A, jint D, jint S, jint R) {
    ModEnv[0] = A;
    ModEnv[1] = D;
    ModEnv[2] = S;
    ModEnv[3] = R;
    return;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModIdx(JNIEnv *env, jclass, jfloat min, jfloat max) {
    modIdxMin = min;
    modIdxMax = max;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModFactor(JNIEnv *env, jclass, jfloat factor) {
    modFactor = factor;
}