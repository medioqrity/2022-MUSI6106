//
// Created by Rose Sun on 2/27/22.
//

#ifndef MUSI6106_LFO_H
#define MUSI6106_LFO_H

#include "ErrorDef.h"
#include "RingBuffer.h"
#include "Synthesis.h"

class CLfo {
public:

    enum Param_t{
        kAmplitude,
        kFrequency,
    };

    Error_t init (float fSampleRateInHz, int iTableSize);

    Error_t reset ();

    Error_t setParam (Param_t eParam, float fParamValue){};

    float getParam (Param_t eParam) const;

    float process();


protected:

    CLfo();

    virtual ~CLfo();

private:

    float           m_fSampleRate, m_fCurrentIndex;

    CRingBuffer<float> *m_pCRingBuffer;


};

#endif //MUSI6106_LFO_H
