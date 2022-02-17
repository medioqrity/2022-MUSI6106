#if !defined(__CombFilter_hdr__)
#define __CombFilter_hdr__

#include "CombFilterIf.h"
#include "RingBuffer.h"

class CCombFilterBase {
public:
    CCombFilterBase(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~CCombFilterBase();

    virtual Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) = 0;

    // wrapper for setter and getter methods
    Error_t setParam(CCombFilterIf::FilterParam_t eParam, float fParamValue);
    float getParam(CCombFilterIf::FilterParam_t eParam) const;

protected:
    // member variables
    CRingBuffer<float>** m_buffer;
    float                m_fGain;
    float                m_fDelay;
    float                m_fSampleRateInHz;
    int                  m_iDelayInSample;
    int                  m_iNumChannels;
    
    // setter and getter methods
    Error_t setGain(float fGain);
    Error_t setDelay(float fDelay);
    float getGain() const;
    float getDelay() const;

private:
    // to check if the parameter we are trying to set is valid
    bool isInRange(float lower, float upper, float value);
    void clearRingBuffer();

    // constants representing parameter limits, which is hard coded in setGain & setDelay parameter checking.
    const float m_kfMaxGain = 5.F;
    const float m_kfMaxDelay = 1.F; // 1s
};

class FIRCombFilter : public CCombFilterBase {
public:
    FIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~FIRCombFilter() = default;

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) override;
};

class IIRCombFilter : public CCombFilterBase {
public:
    IIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~IIRCombFilter() = default;

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) override;
};

#endif // #if !defined(__CombFilter_hdr__)
