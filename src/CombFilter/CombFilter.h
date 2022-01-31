#if !defined(__CombFilter_hdr__)
#define __CombFilter_hdr__

#include "CombFilterIf.h"

class FIRCombFilter : public CCombFilterBase {
public:
	FIRCombFilter ();
	virtual ~FIRCombFilter();

	Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) override;
private:

};

class IIRCombFilter : public CCombFilterBase {
public:
	IIRCombFilter ();
	virtual ~IIRCombFilter();

	Error_t process(float** ppfInputBuffer, float** ppfOutputBuffer, int iNumberOfFrames) override;
};

#endif
