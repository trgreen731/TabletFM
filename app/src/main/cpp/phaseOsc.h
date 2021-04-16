#include <cstdint>

#ifndef phaseOsc_h
#define phaseOsc_h

//constants
#define AMP_DATA_BITS       16
#define ADDR_BITS           12
#define TABLE_SIZE          (1<<ADDR_BITS)
#define PHASE_REG_BITS      24

  /*~~~~~~~~~~~~~~
  wavetable type
  Components:
    topFreq: highest frequency this wavetable is capable of playing
        (Long note: this doesn't really matter for sinewaves because the only
        harmonic component a sinewave contains is itself, so this value
        would always be something just slightly below the nyquist frequency.
        Other waveforms are composed of (a generally infinite) linear combo
        of harmonics that we have to decide to chop off somewhere; to make
        things more complicated, the number of harmonics we decide to make the
        cut at will differ for different frequencies.
          E.g. a square wave contains only odd frequencies; if our nyquist frequency
        is 24000Hz and we want to play an 8000.00Hz tone, we will just barely start
        to alias with only two harmonics, since the second harmonic will be at 3 times
        the frequency of the fundamental, which is 24000Hz. If we want to play
        an 800.00Hz tone we can have 14 harmonics with no aliasing and 15 with
        negligable aliasing (again the harmonics are at odd multiples of the fundamental,
        so 2n+1|_(n=14) = 29; 29*800 = 23200 < 24000)
          But yeah that's why we're only using sinewaves right now lol)

    addrBits: number of bits in memory address of wavetable (determines # of samples
        stored and helps speed up some other stuff if we define it)

    wavetable: the array of samples
  ~~~~~~~~~~~~~~~*/
typedef struct {
    double topFreq;
    float* wavetable;
} wavetable;


#define bilinearInterp 0 // interpolates between tables in a wavetable and between samples in a table
const double fsInverse = 1./48000.;


class PhaseOsc {
protected:
    static const uint16_t maxNumWavetables = 8; //max number of numWaveTableSlots
    const uint16_t phaseRegBits = PHASE_REG_BITS;
    const uint32_t phaseRegMask = (1 << phaseRegBits)-1; //this will be used to quickly clear phaseReg
    const uint16_t addrBits = ADDR_BITS;
    const uint16_t ampDataBits = AMP_DATA_BITS; //16bit unsigned amplitude

    uint32_t phaseReg;
    uint32_t freqReg;
    float ampReg; //bottom 16 bits hold amplitude

    // envelope function variables
    // A is the number of samples until reaching maximum value
    // D is the number of samples until reaching the sustain level
    // S is the sustain magnitude in float (0 to 1)
    // R is the number of samples until reaching 0 value after release
    uint32_t attack;
    uint32_t decay;
    float sustain;
    uint32_t release;

    uint16_t numWavetables;
    wavetable wavetables[maxNumWavetables];

public:
    PhaseOsc(void);
    ~PhaseOsc(void);
    void setFreq(double f);
    void setADSR(uint32_t A, uint32_t D, uint32_t S, uint32_t R);
    float setAmp(uint32_t sampleCt, bool release_flag, uint32_t releaseCt);
    uint32_t updatePhase(uint32_t fmMod);
    void clearPhase();
    uint32_t getRelease();
    uint16_t getOutput(void);
    uint16_t addWavetable(uint32_t len, float* wavetableIn, double topFreq);
    double linterp(uint16_t tIdx, uint32_t sIdxInt, double sIdxFrac);

};

inline void PhaseOsc::setFreq(double f) {
    //sets the frequency register which gets added to the phase register each sample
    uint32_t phaseIncr = (uint32_t)(f*fsInverse*(1<<phaseRegBits));
    freqReg = phaseIncr;
}

inline void PhaseOsc::setADSR(uint32_t A, uint32_t D, uint32_t S, uint32_t R){
    attack = A;
    decay = D;
    sustain = (float)S/100;
    release = R;
}

inline float PhaseOsc::setAmp(uint32_t sampleCt, bool release_flag, uint32_t releaseCt){
    // if release flag is true then the sample count is the release count
    if(sampleCt <= attack){
        ampReg = (1.0) * (float)sampleCt/attack;
    }
    else if(sampleCt <= attack + decay){
        ampReg = 1.0 - ((1.0 - sustain) * (float)(sampleCt - attack)/decay);
    }
    else if(release_flag == false){
        ampReg = sustain;
    }
    else if(release_flag == true && releaseCt <= release){
        ampReg = sustain * (float)(release - releaseCt)/release;
    }
    else{
        ampReg = 0.0;
    }
    return ampReg;
}

inline uint32_t PhaseOsc::updatePhase(uint32_t fmMod) {
    //takes in the amplitude 16 bit output and scales it up to the 24 to match the phase bits
    fmMod = fmMod << (phaseRegBits - ampDataBits);
    phaseReg += freqReg+(uint32_t)(fmMod*fsInverse);
    phaseReg &= phaseRegMask; //wrap phaseReg back down to 0 if it has exceeded 24 bits
    return phaseReg;
}

inline void PhaseOsc::clearPhase() {
    //clear the phase register when starting a new note so not filled with phase of old note
    phaseReg = 0;
}

inline uint32_t PhaseOsc::getRelease(){
    return release;
}

#endif // phaseOsc_h
