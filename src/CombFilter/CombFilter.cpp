#include "CombFilter.h"

FIRCombFilter::FIRCombFilter(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels) :
	m_fDelay(fMaxDelayLengthInS), m_iNumChannels(iNumChannels), m_fGain(0), m_buffer(nullptr)
{
	setDelay(fMaxDelayLengthInS);
}

FIRCombFilter::~FIRCombFilter() {
	delete m_buffer;
}

Error_t FIRCombFilter::setGain(float fGain) {
	m_fGain = fGain;
}

/*! Update the delay length of filter. This would cause deleting & newing CRingBuffer object. 
\param fDelay the delay time IN SECOND
*/
Error_t FIRCombFilter::setDelay(float fDelay) {
	m_fDelay = fDelay;
	m_iDelayInSample = static_cast<int>(m_fDelay / m_fSampleRateInHz);
	if (m_buffer != nullptr) {
		delete m_buffer;
	}
	m_buffer = new CRingBuffer<float>(m_iDelayInSample);
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

