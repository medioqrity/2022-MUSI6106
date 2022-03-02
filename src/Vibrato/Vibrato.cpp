#include "Vibrato.h"

VibratoEffector::VibratoEffector(int iSampleRate, int iNumChannel, float fModulationFreqInHz, float fModulationWidthInS)
    : m_iSampleRate(iSampleRate), 
      m_iNumChannel(iNumChannel),
      m_fModulationFreqInHz(fModulationFreqInHz), 
      m_fModulationWidthInS(fModulationWidthInS) {
    updateDelayBuffer();
    updateLfo();
}

void VibratoEffector::updateDelayBuffer() {
    deleteBuffer();
    m_fDelayInSample = m_fModulationWidthInS * static_cast<float>(m_iSampleRate);
    int iDelayInSample = static_cast<int>(ceil(m_fDelayInSample * 2));

    m_buffer = new CRingBuffer<float>*[m_iNumChannel];
    for (int i = 0; i < m_iNumChannel; ++i) {
        m_buffer[i] = new CRingBuffer<float>(iDelayInSample);
        m_buffer[i]->setReadIdx(0);
        m_buffer[i]->setWriteIdx(iDelayInSample);
    }
}

void VibratoEffector::deleteBuffer() {
    if (m_buffer != nullptr) {
        for (int i = 0; i < m_iNumChannel; ++i) {
            if (m_buffer[i] != nullptr) {
                delete m_buffer[i];
            }
        }
        delete[] m_buffer;
        m_buffer = nullptr;
    }
}

void VibratoEffector::updateLfo() {
    deleteLfo();
    m_lfo = new CLfo*[m_iNumChannel];
    for (int i = 0; i < m_iNumChannel; ++i) {
        m_lfo[i] = new CLfo(m_iSampleRate, 1 << 12, 1, m_fModulationFreqInHz); // TODO: how to determine the table size?
    }
}

void VibratoEffector::deleteLfo() {
    if (m_lfo != nullptr) {
        for (int i = 0; i < m_iNumChannel; ++i) {
            if (m_lfo[i] != nullptr) {
                delete m_lfo[i];
                m_lfo[i] = nullptr;
            }
        }
        delete[] m_lfo;
        m_lfo = nullptr;
    }
}

VibratoEffector::~VibratoEffector() {
    deleteBuffer();
}

Error_t VibratoEffector::setParam(VibratoEffector::VibratoParam_t eParam, float fParamValue) {
    switch (eParam)
    {
    case VibratoEffector::VibratoParam_t::kModulationFreq:
        return setModulationFreq(fParamValue);
    case VibratoEffector::VibratoParam_t::KModulationWidth:
        return setModulationWidth(fParamValue);
    default:
        return Error_t::kFunctionInvalidArgsError;
    }
    return Error_t::kNoError;
}

float VibratoEffector::getParam(VibratoEffector::VibratoParam_t eParam) const {
    switch (eParam)
    {
    case VibratoEffector::VibratoParam_t::kModulationFreq:
        return getModulationFreq();
    case VibratoEffector::VibratoParam_t::KModulationWidth:
        return getModulationWidth();
    default:
        return -1;
    }
}

Error_t VibratoEffector::setModulationWidth(float fModulationWidthInS) {
    // TODO: is there anyway to not re-allocate delay buffer?
    m_fModulationWidthInS = fModulationWidthInS;
    updateDelayBuffer();
    return Error_t::kNoError;
}

Error_t VibratoEffector::setModulationFreq(float fModulationFreqInHz) {
    m_fModulationFreqInHz = fModulationFreqInHz;
    for (int i = 0; i < m_iNumChannel; ++i) {
        m_lfo[i]->setParam(CLfo::LfoParam_t::kFrequency, m_fModulationFreqInHz);
    }
    return Error_t::kNoError;
}

float VibratoEffector::getModulationFreq() const {
    return m_fModulationFreqInHz;
}

float VibratoEffector::getModulationWidth() const {
    return m_fModulationWidthInS;
}

Error_t VibratoEffector::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
    float fWidthInSamples = m_fModulationWidthInS * static_cast<float>(m_iSampleRate);
    for (int i = 0; i < m_iNumChannel; ++i) {
        for (int j = 0; j < iNumberOfFrames; ++j) {
            m_buffer[i]->putPostInc(ppfInputBuffer[i][j]); // no need to move read index: it's already full and ringbuffer have overload protection, when full and insert, the read index will increase automatically.
            // ppfOutputBuffer[i][j] = m_lfo[i]->get();
            // ppfOutputBuffer[i][j] = m_buffer[i]->get(m_buffer[i]->getLength() - 1 - (m_lfo[i]->get() * fWidthInSamples + fWidthInSamples + 1.F));
            std::printf("%d %d %.5f\n", m_buffer[i]->getReadIdx(), m_buffer[i]->getWriteIdx(), m_buffer[i]->getLength() - m_lfo[i]->get() * fWidthInSamples);
            // ppfOutputBuffer[i][j] = m_buffer[i]->get(m_buffer[i]->getLength() - m_lfo[i]->get() * fWidthInSamples);
            ppfOutputBuffer[i][j] = m_buffer[i]->get(m_buffer[i]->getLength() - m_lfo[i]->get() * fWidthInSamples);
        }
    }
    return Error_t::kNoError;
}

