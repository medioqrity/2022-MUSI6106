#if !defined(__CombFilter_hdr__)
#define __CombFilter_hdr__

#include "CombFilterIf.h"

class FIRCombFilter : public CCombFilterBase {
public:
    FIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~FIRCombFilter();

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames);
};

class IIRCombFilter : public CCombFilterBase {
public:
    IIRCombFilter (float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels);
    virtual ~IIRCombFilter();

    Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames);
};

#endif // #if !defined(__CombFilter_hdr__)
