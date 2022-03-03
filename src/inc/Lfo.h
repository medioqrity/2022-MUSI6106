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
    CLfo(int fSampleRateInHz, int iTableSize, float amplitude, float frequency)
    : m_iSampleRate(fSampleRateInHz), m_iTableSize(iTableSize), m_amplitude(amplitude), m_frequency(frequency) {
        m_fCurrentIndex = 0.f;
        m_pCRingBuffer = new CRingBuffer<float>(m_iTableSize);
        m_pCRingBuffer->setReadIdx(0);
        m_pCRingBuffer->setWriteIdx(m_iTableSize);
    }

    virtual ~CLfo() {
    }

    enum class LfoParam_t{
        kAmplitude,
        kFrequency,
    };

    Error_t setParam (LfoParam_t eParam, float fParamValue) {
        switch (eParam) {
            case LfoParam_t::kAmplitude:
                m_amplitude = fParamValue;
                break;
            case LfoParam_t::kFrequency:
                m_frequency = fParamValue;
                break;
            default:
                return Error_t::kFunctionInvalidArgsError;
        }
        return Error_t::kNoError;
    };

    float getParam (LfoParam_t eParam) const {
        switch (eParam) {
            case LfoParam_t::kAmplitude:
                return m_amplitude;
            case LfoParam_t::kFrequency:
                return m_frequency;
            default:
                break;
        }
        return -1.f;
    }

    /*! returns the current lfo value
    */
    float get() {

        float returnValue = m_pCRingBuffer->get(m_fCurrentIndex * m_iTableSize) * m_amplitude;
        m_fCurrentIndex += m_frequency / static_cast<float>(m_iSampleRate);
        if (m_fCurrentIndex >= 1.F) m_fCurrentIndex -= 1.F;
        return returnValue;
    }

    /*! set internal wave table to be exactly the same as input array
    \param arr the input array that contains wavetable information
    */
    Error_t setWaveTable(float* arr) {
        m_pCRingBuffer->setWriteIdx(0);
        for (int i = 0; i < m_iTableSize; ++i) {
            m_pCRingBuffer->putPostInc(arr[i]);
        }
        return Error_t::kNoError;
    }

    void reset() {
        m_fCurrentIndex = 0.F;
    }

private:

    // m_fCurrentIndex should be in 0~1
    float m_fCurrentIndex, m_amplitude, m_frequency;

    int m_iSampleRate;
    int m_iTableSize = 4096;

    CRingBuffer<float> *m_pCRingBuffer;
};

#endif //MUSI6106_LFO_H
