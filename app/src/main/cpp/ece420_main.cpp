//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#include <jni.h>
#include <cmath>
#include <cstdint>
#include "ece420_main.h"
#include "ece420_lib.h"
#include "phaseOsc.h"

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

//constants
#define FRAME_SIZE          1024
#define F_S                 48000
#define PI                  3.1415
#define AMP_CONST           10000

//oscillators
PhaseOsc* carrier;
PhaseOsc* modulator;

// a sample counter to keep track of the note time between frames
uint32_t sample_count = 0;

// a release indicator telling whether a note is still playing despite no key being pressed
bool release_flag = false;
uint32_t release_count = 0;

// We have two variables here to ensure that we never change the desired frequency while
// processing a frame. Thread synchronization, etc. These will hold the carrier frequency
double FREQ_NEW_ANDROID = 0;
double FREQ_NEW = 0;

uint32_t temp;

// implement our FM synthesis algorithm right here
void FMSynthesis(float* bufferOut) {

    // if the carrier frequency is 0 and release flag indicates no note being played
    if(FREQ_NEW == 0 && release_flag == false){
        for(int i=0; i<FRAME_SIZE; i++){
            bufferOut[i] = (float)0;
        }
        //make sure all the registers hold 0's
        carrier->clearPhase();
        modulator->clearPhase();
        sample_count = 0;
        release_count = 0;
    }
    //otherwise perform the synthesis
    else{
        for(int i=0; i<FRAME_SIZE; i++){
            // get the output first based on the previous register values (16 bit output)
            bufferOut[i] = AMP_CONST * carrier->getOutput();

            // update the two amplitude values based on position in note playback
            carrier->setAmp(sample_count, release_flag, release_count);
            modulator->setAmp(sample_count, release_flag, release_count);
            //update the modulator register (no additional modulation element)
            temp = modulator->updatePhase(0);

            //update the carrier register
            carrier->updatePhase(modulator->getOutput());

            //update the release count and the release_flag
            if(release_flag == true){
                release_count++;
            }
            sample_count++;
        }
    }

    LOGD("Key Frequency Detected: %d\r\n", temp);
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
            carrier->setFreq(FREQ_NEW_ANDROID);
            modulator->setFreq(FREQ_NEW_ANDROID);       //right now just the same as the carrier frequency
        }
        else{
            release_flag = true;
            release_count = 0;
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
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initTable(JNIEnv *env, jclass, jint wave) {
    //initialize the phase oscillators
    carrier = new PhaseOsc();
    modulator = new PhaseOsc();

    // wavetable space - holds a temporary simple sin wave
    float wavetable[TABLE_SIZE];

    //currently only support sin waves so if wave is not 0 then the wavetable is blank
    if(wave == 0){
        for(int i = 0;i<TABLE_SIZE; i++){
            wavetable[i] = (float)sin(2*PI*((double)i/TABLE_SIZE));
        }
    }
    else{
        for(int i=0; i<TABLE_SIZE; i++){
            wavetable[i] = 0.0;
        }
    }

    //add the sine wavetable to the two phase oscillators (no clue how to use the top_freq values)
    carrier->addWavetable(TABLE_SIZE, wavetable, F_S/2);
    modulator->addWavetable(TABLE_SIZE, wavetable, F_S/2);
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initAmpEnv(JNIEnv *env, jclass, jint A, jint D, jint S, jint R) {
    carrier->setADSR(A, D, S, R);
}

JNIEXPORT void JNICALL
Java_com_ece420_lab5_Piano_initModEnv(JNIEnv *env, jclass, jint A, jint D, jint S, jint R) {
    modulator->setADSR(A, D, S, R);
}