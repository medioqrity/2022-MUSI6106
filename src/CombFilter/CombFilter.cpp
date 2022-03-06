#include "CombFilter.h"

CCombFilterBase::CCombFilterBase(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
: m_fGain(0.F), m_iNumChannels(iNumChannels), m_buffer(nullptr), m_fSampleRateInHz(fSampleRateInHz)
{
    setDelay(fMaxDelayLengthInS);
}

CCombFilterBase::~CCombFilterBase() {
    clearRingBuffer();
}

bool CCombFilterBase::isInRange(float lower, float upper, float value) {
    return (lower <= value && value < upper);
}

void CCombFilterBase::clearRingBuffer() {
    if (m_buffer != nullptr) {
        for (int i = 0; i < m_iNumChannels; ++i) {
            if (m_buffer[i] != nullptr) {
                delete m_buffer[i];
                m_buffer[i] = nullptr;
            }
        }
        delete[] m_buffer;
    }
    m_buffer = nullptr;
}

/*
Setter part
*/
Error_t CCombFilterBase::setGain(float fGain) {
    if (!isInRange(-m_kfMaxGain, m_kfMaxGain, fGain)) {
        return Error_t::kFunctionInvalidArgsError;
    }
    m_fGain = fGain;
    return Error_t::kNoError;
}

/*! Update the delay length of filter. This would cause deleting & newing CRingBuffer object. 
\param fDelay the delay time IN SECOND
*/
Error_t CCombFilterBase::setDelay(float fDelay) {
    if (!isInRange(0.F, m_kfMaxDelay, fDelay)) {
        return Error_t::kFunctionInvalidArgsError;
    }
	m_fDelay = fDelay;
	m_iDelayInSample = static_cast<int>(m_fDelay * m_fSampleRateInHz);
    clearRingBuffer();

    m_buffer = new CRingBuffer<float>*[m_iNumChannels];
    for (int i = 0; i < m_iNumChannels; ++i) {
	    m_buffer[i] = new CRingBuffer<float>(m_iDelayInSample);
        m_buffer[i]->setWriteIdx(m_iDelayInSample);
    }

    return Error_t::kNoError;
}

Error_t CCombFilterBase::setParam(CCombFilterIf::FilterParam_t eParam, float fParamValue) {
    switch (eParam)
    {
    case CCombFilterIf::kParamGain:
        return setGain(fParamValue);
    case CCombFilterIf::kParamDelay:
        return setDelay(fParamValue);
    case CCombFilterIf::kNumFilterParams:
    default:
        return Error_t::kFunctionInvalidArgsError;
    }
}

/*
Getter part
*/

float CCombFilterBase::getGain() const {
    return m_fGain;
}

float CCombFilterBase::getDelay() const {
    return m_fDelay;
}

float CCombFilterBase::getParam(CCombFilterIf::FilterParam_t eParam) const {
    switch (eParam)
    {
    case CCombFilterIf::kParamGain:
        return getGain();
    case CCombFilterIf::kParamDelay:
        return getDelay();
    case CCombFilterIf::kNumFilterParams:
        return -1.F;
    default:
        break;
    }
    return -1.F;
}

FIRCombFilter::FIRCombFilter(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
    : CCombFilterBase(fMaxDelayLengthInS, fSampleRateInHz, iNumChannels) {
}

Error_t FIRCombFilter::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
	for (int i = 0; i < m_iNumChannels; ++i) {
		for (int j = 0; j < iNumberOfFrames; ++j) {
			ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * m_buffer[i]->getPostInc();
			m_buffer[i]->putPostInc(ppfInputBuffer[i][j]);
		}
	}
    return Error_t::kNoError;
}

IIRCombFilter::IIRCombFilter(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
	: CCombFilterBase(fMaxDelayLengthInS, fSampleRateInHz, iNumChannels) { }

Error_t IIRCombFilter::process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) {
	for (int i = 0; i < m_iNumChannels; ++i) {
		for (int j = 0; j < iNumberOfFrames; ++j) {
			ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * m_buffer[i]->getPostInc();
			m_buffer[i]->putPostInc(ppfOutputBuffer[i][j]);
		}
	}
    return Error_t::kNoError;
}

