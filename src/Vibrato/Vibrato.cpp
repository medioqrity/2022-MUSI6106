#include "Vibrato.h"

VibratoEffector::VibratoEffector(int iSampleRate, float fModulationFreqInHz, float fModulationWidthInHz)
    : m_iSampleRate(iSampleRate), 
      m_fModulationFreqInHz(fModulationFreqInHz), 
      m_fModulationWidthInHz(fModulationWidthInHz) {
    updateDelayBuffer();
    m_lfo = new CLfo(m_iSampleRate, 1 << 12, m_fModulationWidthInHz, m_fModulationFreqInHz); // TODO: how to determine the table size?
}

void VibratoEffector::deleteBuffer() {
    if (m_buffer != nullptr) {
        delete m_buffer;
        m_buffer = nullptr;
    }
}

VibratoEffector::~VibratoEffector() {
    deleteBuffer();
}

void VibratoEffector::updateDelayBuffer() {
    deleteBuffer();
    m_fDelayInSample = m_fModulationWidthInHz * static_cast<float>(m_iSampleRate);
    m_buffer = new CRingBuffer<float>(static_cast<int>(m_fDelayInSample));
    m_buffer->setReadIdx(0);
    m_buffer->setWriteIdx(static_cast<int>(m_fDelayInSample));
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

Error_t VibratoEffector::setModulationWidth(float fModulationWidthInHz) {
    // TODO: is there anyway to not re-allocate delay buffer?
    m_fModulationWidthInHz = fModulationWidthInHz;
    updateDelayBuffer();
    return Error_t::kNoError;
}

Error_t VibratoEffector::setModulationFreq(float fModulationFreqInHz) {
    m_fModulationFreqInHz = fModulationFreqInHz;
    m_lfo->setParam(CLfo::Param_t::kFrequency, m_fModulationFreqInHz);
}

float VibratoEffector::getModulationFreq() const {
    return m_fModulationFreqInHz;
}

float VibratoEffector::getModulationWidth() const {
    return m_fModulationWidthInHz;
}

Error_t VibratoEffector::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
    float fWidthInSamples = m_fModulationWidthInHz * static_cast<float>(m_iSampleRate);
    for (int i = 0; i < m_iNumChannel; ++i) {
        for (int j = 0; j < iNumberOfFrames; ++j) {
            m_buffer->putPostInc(ppfInputBuffer[i][j]); // no need to move read index: it's already full and ringbuffer have overload protection, when full and insert, the read index will increase automatically.
            ppfOutputBuffer[i][j] = m_buffer->get(m_lfo->get() + fWidthInSamples + 1.F);
        }
    }
}

