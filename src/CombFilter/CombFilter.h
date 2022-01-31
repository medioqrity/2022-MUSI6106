#if !defined(__CombFilter_hdr__)
#define __CombFilter_hdr__

#include "CombFilterIf.h"
#include "RingBuffer.h"

class FIRCombFilter : public CCombFilterBase {
public:
    FIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~FIRCombFilter();

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames);
private:
    CRingBuffer<float>* m_buffer;
    float               m_fGain;
    float               m_fDelay;
    float               m_fSampleRateInHz;
    int                 m_iDelayInSample;
    int                 m_iNumChannels;
    Error_t setGain(float fGain);
    Error_t setDelay(float fDelay);
};

class IIRCombFilter : public CCombFilterBase {
public:
    IIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~IIRCombFilter();

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames);
private:
    CRingBuffer<float>* m_buffer;
    float               m_fGain;
    float               m_fDelay;
    int                 m_iDelayInSample;
    int                 m_iNumChannels;
    Error_t setGain(float fGain);
    Error_t setDelay(float fDelay);
};

#endif
