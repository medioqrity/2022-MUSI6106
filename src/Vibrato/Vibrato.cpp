#include "Vibrato.h"

VibratoEffector::VibratoEffector(int iSampleRate, float fModulationFreqInHz, float fModulationWidthInHz)
    : m_iSampleRate(iSampleRate), 
      m_fModulationFreqInHz(fModulationFreqInHz), 
      m_fModulationWidthInHz(fModulationWidthInHz) {
    updateDelayBuffer();
}

void VibratoEffector::updateDelayBuffer() {
    if (m_buffer != nullptr) {
        delete m_buffer;
        m_buffer = nullptr;
    }

    int iWidthInSample = m_fModulationWidthInHz / static_cast<float>(m_iSampleRate);
    int iDelayInSample = iWidthInSample;
    int iTotalDelayLength = 2 + iDelayInSample + (iWidthInSample << 1);
    m_buffer = new CRingBuffer<float>(iTotalDelayLength);
    m_buffer->setReadIdx(0);
    m_buffer->setWriteIdx(iTotalDelayLength);
}
