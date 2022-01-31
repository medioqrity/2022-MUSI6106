#include "CombFilter.h"

FIRCombFilter::FIRCombFilter(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels) {
	setDelay(fMaxDelayLengthInS);
}

FIRCombFilter::~FIRCombFilter() {
	delete m_buffer;
}

Error_t FIRCombFilter::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
	for (int i = 0; i < m_iNumChannels; ++i) {
		m_buffer->reset();
		m_buffer->setWriteIdx(m_iDelayInSample);
		for (int j = 0; j < iNumberOfFrames; ++j) {
			ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * m_buffer->getPostInc();
			m_buffer->putPostInc(ppfInputBuffer[i][j]);
		}
	}
}

Error_t IIRCombFilter::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
	for (int i = 0; i < m_iNumChannels; ++i) {
		m_buffer->reset();
		m_buffer->setWriteIdx(m_iDelayInSample);
		for (int j = 0; j < iNumberOfFrames; ++j) {
			ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * m_buffer->getPostInc();
			m_buffer->putPostInc(ppfOutputBuffer[i][j]);
		}
	}
}
