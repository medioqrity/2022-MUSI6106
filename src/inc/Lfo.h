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
    CLfo(float fSampleRateInHz, int iTableSize)
    : m_iSampleRate(fSampleRateInHz), m_iTableSize(iTableSize) {
        m_fCurrentIndex = 0.f;
        m_amplitude = 0.f;
        m_frequency = 0.f;
        m_pCRingBuffer = new CRingBuffer<float>(m_iTableSize);
    }

    virtual ~CLfo() {
    }

    enum Param_t{
        kAmplitude,
        kFrequency,
    };

    Error_t setParam (Param_t eParam, float fParamValue) {
        switch (eParam) {
            case Param_t::kAmplitude:
                m_amplitude = fParamValue;
                break;
            case Param_t::kFrequency:
                m_frequency = fParamValue;
                break;
            default:
                return Error_t::kFunctionInvalidArgsError;
        }
        return Error_t::kNoError;
    };

    float getParam (Param_t eParam) const {
        switch (eParam) {
            case Param_t::kAmplitude:
                return m_amplitude;
            case Param_t::kFrequency:
                return m_frequency;
            default:
                break;
        }
        return -1.f;
    }

    /*! returns the current lfo value
    */
    float get();

private:

    float m_fCurrentIndex, m_amplitude, m_frequency;

    int m_iSampleRate;
    int m_iTableSize = 4096;

    CRingBuffer<float> *m_pCRingBuffer;


};

#endif //MUSI6106_LFO_H
