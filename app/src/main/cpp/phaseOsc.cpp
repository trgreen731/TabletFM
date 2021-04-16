#include "phaseOsc.h"
#include <cmath>
#include <cstdint>

//constructor
PhaseOsc::PhaseOsc(void) {
    phaseReg = 0;
    freqReg = 0;
    ampReg = 0;
    numWavetables = 0;
    for (int i = 0; i < maxNumWavetables; i++) {
        wavetables[i].topFreq = 0.;
        wavetables[i].wavetable = NULL;
    }
}

PhaseOsc::~PhaseOsc(void) {
    for (int i = 0; i <maxNumWavetables; i++) {
        float* temp = wavetables[i].wavetable;
        if (temp) {delete [] temp;}
    }
}

uint16_t PhaseOsc::addWavetable(uint32_t len, float* wavetableIn, double topFreq) {
    if (this->numWavetables < this->maxNumWavetables) {

        float* wavetable = this->wavetables[this->numWavetables].wavetable = new float[len];
        this->wavetables[this->numWavetables].topFreq = topFreq;

        for (uint32_t idx = 0; idx < len; idx++)
            wavetable[idx] = wavetableIn[idx];

        this->numWavetables++;
    }
    return this->numWavetables;
}

double PhaseOsc::linterp(uint16_t tIdx, uint32_t sIdxInt, double sIdxFrac) {
    sIdxInt %= (1 << addrBits); //unnecessary prolly
    float l = this->wavetables[tIdx].wavetable[sIdxInt];
    float r = this->wavetables[tIdx].wavetable[sIdxInt+1];
    return l + sIdxFrac*(r-l);
}


uint16_t PhaseOsc::getOutput() {
#if !bilinearInterp

    uint16_t tableIdx = 0;
    while ((this->freqReg >= this->wavetables[tableIdx].topFreq) && (tableIdx < (this->numWavetables-1))) {
      ++tableIdx;
    }
    wavetable* table = &this->wavetables[tableIdx];
    //get memory address from top 12 bits of phaseReg
    float samp = table->wavetable[(phaseReg >> (this->phaseRegBits - this->addrBits))];
    return (uint32_t)(samp*ampReg);

#else
    // get value of frequency register in terms of freqRatio = (actual_freq)/(samplerate)
    double freqRatio = ((double)freqReg/(1 << phaseRegBits))*((double)this->numWavetables/2);
    // get value of phase register in terms of how many samples into waveform we are
    double phaseSamps = ((double)phaseReg/(1 << phaseRegBits))*(1 << addrBits);
    double sIdxFrac, sIdxInt, tIdxFrac, tIdxInt,flevel;
    //get fractional and integer sample indices from  phaseReg*(2^addrBits)/(2^phaseRegBits)
    sIdxFrac = modf(phaseSamps, &sIdxInt);
    //get fractional and integer table indices through magic
    flevel = -log2(freqRatio);
    tIdxFrac = modf(flevel, &tIdxInt);
    sIdxInt = (uint32_t)sIdxInt;
    tIdxInt = (uint16_t)tIdxInt;
    double HV = 0;
    double LV = 0;
    if (flevel < 0) {
        HV = linterp(this->numWavetables-1,sIdxInt,sIdxFrac);
    }
    else if (flevel < this->numWavetables-1) {
        LV = linterp(tIdxInt,sIdxInt,sIdxFrac);
        HV = linterp(tIdxInt+1,sIdxInt,sIdxFrac);
    }
    else {
        range_correction = tIdxInt - this->numWavetables + 1;
        LV = self.linterp(0,(sIdxInt << range_correction),sIdxFrac);
        HV = linterp(0,(sIdxInt << range_correction+1),sIdxFrac);
    }

    double sample = (LV + tIdxFrac*(HV-LV));
    return (sample*ampReg)*(1 << phaseRegBits);
#endif
}
