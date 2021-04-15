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

#define FRAME_SIZE 1024
#define F_S 48000

// We have two variables here to ensure that we never change the desired frequency while
// processing a frame. Thread synchronization, etc. Setting to 300 is only an initializer.
double FREQ_NEW_ANDROID = 0;
double FREQ_NEW = 0;

void FMSynthesis(float* bufferOut) {

    // implement our FM synthesis algorithm right here

    //LOGD("Key Frequency Detected: %f\r\n", FREQ_NEW);
}

void ece420ProcessFrame(sample_buf *dataBuf) {
    // Keep in mind, we only have 20ms to process each buffer!
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    float bufferOut[FRAME_SIZE] = {};

    // Get the new desired frequency from android
    FREQ_NEW = FREQ_NEW_ANDROID;

    //perform our FMSynthesis right here
    FMSynthesis(bufferOut);

    for(int i=0; i<FRAME_SIZE; i++){
        bufferOut[i] = (float)sin(FREQ_NEW * i / F_S) * 1000;
    }

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